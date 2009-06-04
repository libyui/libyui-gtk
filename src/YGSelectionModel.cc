/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#define YUILogComponent "gtk"
#include <config.h>
#include <gtk/gtk.h>
#include <YTreeItem.h>
#include "YGSelectionModel.h"
#include "YGUtils.h"

YGSelectionModel::YGSelectionModel (YSelectionWidget *ywidget, bool ordinaryModel, bool isTree)
	: isTree (isTree), ywidget (ywidget), dataIndex (0)
{
	if (ordinaryModel) {
		vector <GType> cols;
		cols.push_back (G_TYPE_STRING);
		cols.push_back (GDK_TYPE_PIXBUF);
		createModel (cols);
	}
}

YGSelectionModel::~YGSelectionModel()
{ g_object_unref (G_OBJECT (m_model)); }

void YGSelectionModel::createModel (const vector <GType> &types)
{
	int colsNb = types.size()+2;
	GType types_array [colsNb];
	int i = 0;
	for (vector <GType>::const_iterator it = types.begin(); it != types.end(); it++)
		types_array [i++] = *it;
	types_array[colsNb-2] = G_TYPE_POINTER;  // pointer to YItem
	types_array[colsNb-1] = G_TYPE_POINTER;  // unique id -- using GtkTreeRowReference is expensive

	if (isTree)
		m_model = GTK_TREE_MODEL (gtk_tree_store_newv (colsNb, types_array));
	else
		m_model = GTK_TREE_MODEL (gtk_list_store_newv (colsNb, types_array));
}

GtkTreeModel *YGSelectionModel::getModel()
{ return m_model; }
GtkListStore *YGSelectionModel::getListStore()
{ return m_model ? GTK_LIST_STORE (m_model) : NULL; }
GtkTreeStore *YGSelectionModel::getTreeStore()
{ return m_model ? GTK_TREE_STORE (m_model) : NULL; }

bool YGSelectionModel::isEmpty()
{
	GtkTreeIter iter;
	return !gtk_tree_model_get_iter_first (getModel(), &iter);
}

void YGSelectionModel::doAddItem (YItem *item)
{
	GtkTreeIter iter;
	bool empty = isEmpty();
	addRow (&iter, item, false);
	setCellLabel (&iter, LABEL_COLUMN, item->label());
	setCellIcon (&iter, ICON_COLUMN, item->iconName());
	for (YItemIterator it = item->childrenBegin(); it != item->childrenEnd(); it++)
		doAddItem (*it);
	if (empty || item->selected())
		doSelectItem (&iter);

	YTreeItem *tree_item = dynamic_cast <YTreeItem *> (item);
	if (tree_item && tree_item->isOpen()) {
		// only expand if all parent's are expand too
		YTreeItem *i;
		for (i = tree_item->parent(); i; i = i->parent())
			if (!i->isOpen())
				break;
		if (!i)
			expand (&iter);
	}
}

void YGSelectionModel::doDeleteAllItems()
{
	if (isTree)
		gtk_tree_store_clear (getTreeStore());
	else
		gtk_list_store_clear (getListStore());
	dataIndex = 0;
}

int YGSelectionModel::getPtrCol()
{ return gtk_tree_model_get_n_columns (getModel()) - 2; }

static int getIdCol (GtkTreeModel *model)
{ return gtk_tree_model_get_n_columns (model) - 1; }
int YGSelectionModel::getIdCol() { return ::getIdCol (getModel()); }

YItem *YGSelectionModel::getItem (GtkTreeIter *iter)
{
	gpointer ptr;
	gtk_tree_model_get (getModel(), iter, getPtrCol(), &ptr, -1);
	return (YItem *) ptr;
}

bool YGSelectionModel::getIter (const YItem *item, GtkTreeIter *iter)
{
	if (!item)
		return false;

	struct inner {
		static void free_iter (gpointer iter)
		{ gtk_tree_iter_free ((GtkTreeIter *) iter); }

		static gboolean find_iter (GtkTreeModel *model, GtkTreePath *path,
		                           GtkTreeIter *iter, gpointer data)
		{
			gpointer iter_data = 0;
			gtk_tree_model_get (model, iter, ::getIdCol (model), &iter_data, -1);
			if (data == iter_data) {
				g_object_set_data_full (G_OBJECT (model), "found",
					gtk_tree_iter_copy (iter), free_iter);
				return TRUE;
			}
			return FALSE;
		}
	};

	GtkTreeModel *model = getModel();
	g_object_set_data (G_OBJECT (model), "found", 0);  // hacky
	gtk_tree_model_foreach (model, inner::find_iter, item->data());
	GtkTreeIter *found = (GtkTreeIter *) g_object_get_data (G_OBJECT (model), "found");
	if (found) {
		*iter = *found;
		return TRUE;
	}
	return FALSE;
}

void YGSelectionModel::implSelectItem (YItem *item)
{
	GtkTreeIter iter;
	if (getIter (item, &iter))
		doSelectItem (&iter);
}

void YGSelectionModel::addRow (GtkTreeIter *iter, YItem *item, bool honor_select)
{
	struct inner {
		static void setItemData (YGSelectionModel *model, GtkTreeIter *iter, YItem *item)
		{
			item->setData (model->dataIndex);
			model->dataIndex = GINT_TO_POINTER (GPOINTER_TO_INT (model->dataIndex) + 1);
		}
	};

	if (isTree) {
		GtkTreeStore *store = getTreeStore();
		if (item->parent()) {
			GtkTreeIter parent;
			getIter (item->parent(), &parent);
			gtk_tree_store_append (store, iter, &parent);
		}
		else
			gtk_tree_store_append (store, iter, NULL);
		gtk_tree_store_set (store, iter, getPtrCol(), item, getIdCol(), dataIndex, -1);
		inner::setItemData (this, iter, item);
	}
	else {
		GtkListStore *store = getListStore();
		gtk_list_store_append (store, iter);
		gtk_list_store_set (store, iter, getPtrCol(), item, getIdCol(), dataIndex, -1);
		inner::setItemData (this, iter, item);
	}

	if (honor_select && item->selected())
		doSelectItem (iter);
}

void YGSelectionModel::setCellLabel (GtkTreeIter *iter, int col, const string &label)
{
	if (isTree)
		gtk_tree_store_set (getTreeStore(), iter, col, label.c_str(), -1);
	else
		gtk_list_store_set (getListStore(), iter, col, label.c_str(), -1);
}

void YGSelectionModel::setCellIcon (GtkTreeIter *iter, int col, const string &icon)
{
	string path = ywidget->iconFullPath (icon);
	GdkPixbuf *pixbuf = YGUtils::loadPixbuf (path.c_str());
	if (isTree)
		gtk_tree_store_set (getTreeStore(), iter, col, pixbuf, -1);
	else
		gtk_list_store_set (getListStore(), iter, col, pixbuf, -1);
}

void YGSelectionModel::setCellToggle (GtkTreeIter *iter, int col, bool select)
{
	if (isTree)
		gtk_tree_store_set (getTreeStore(), iter, col, select, -1);
	else
		gtk_list_store_set (getListStore(), iter, col, select, -1);
}

static int getChildrenDepth (GtkTreeModel *model, GtkTreeIter *parent, int *rows)
{
	int depth = 0;
	GtkTreeIter iter;
	if (gtk_tree_model_iter_children (model, &iter, parent)) {
		do {
			depth = MAX (depth, getChildrenDepth (model, &iter, rows));
			(*rows)++;
		} while (gtk_tree_model_iter_next (model, &iter));
		return depth+1;
	}
	return 0;
}

int YGSelectionModel::getMaxDepth (int *rows)
{
	GtkTreeModel *model = getModel();
	int depth = 0; *rows = 0;
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			depth = MAX (depth, getChildrenDepth (model, &iter, rows));
			(*rows)++;
		} while (gtk_tree_model_iter_next (model, &iter));
	}
	return depth;
}

extern "C" {
    struct FindClosure {
        const string &text;
        bool         found;
        GtkTreeIter *output;
        FindClosure(const string &_text, GtkTreeIter *_output) :
            text (_text),
            found (false),
            output (_output) {}
    };
    static gboolean find_text (GtkTreeModel *model, GtkTreePath *path,
                               GtkTreeIter *iter, gpointer data)
    {
        FindClosure *cl = (FindClosure *)data;

        gchar *label;
        gtk_tree_model_get (model, iter, YGSelectionModel::LABEL_COLUMN, &label, -1);

        if (cl->text == label) {
            *(cl->output) = *iter;
            cl->found = true;
        }
        g_free (label);

        return cl->found;
    }
};

bool YGSelectionModel::findByText (const string &text, GtkTreeIter *iter)
{
    FindClosure cl (text, iter);
    gtk_tree_model_foreach (getModel(), find_text, (gpointer) &cl);
    return cl.found;
}

