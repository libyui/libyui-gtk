/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>
#include <gtk/gtk.h>
#include <YTreeItem.h>
#include "YGSelectionModel.h"
#include "YGUtils.h"

YGSelectionModel::YGSelectionModel (YSelectionWidget *ywidget, bool ordinaryModel, bool isTree)
	: isTree (isTree), ywidget (ywidget)
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
	int colsNb = types.size()+1;
	GType types_array [colsNb];
	int i = 0;
	for (vector <GType>::const_iterator it = types.begin(); it != types.end(); it++)
		types_array [i++] = *it;
	types_array[colsNb-1] = G_TYPE_POINTER;

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
		setFocusItem (&iter);
}

void YGSelectionModel::doDeleteAllItems()
{
	if (isTree)
		gtk_tree_store_clear (getTreeStore());
	else
		gtk_list_store_clear (getListStore());
}

YItem *YGSelectionModel::getItem (GtkTreeIter *iter)
{
	gpointer ptr;
	gtk_tree_model_get (getModel(), iter, getPtrCol(), &ptr, -1);
	return (YItem *) ptr;
}

bool YGSelectionModel::getIter (YItem *item, GtkTreeIter *iter)
{
	if (!item)
		return false;
	GtkTreePath *path = gtk_tree_path_new();
	for (; item; item = item->parent()) {
		int index = GPOINTER_TO_INT (item->data());
		gtk_tree_path_prepend_index (path, index);
	}

	bool ret = gtk_tree_model_get_iter (getModel(), iter, path);
	gtk_tree_path_free (path);
	return ret;
}

void YGSelectionModel::implFocusItem (YItem *item)
{
	GtkTreeIter iter;
	if (getIter (item, &iter))
		setFocusItem (&iter);
}

int YGSelectionModel::getPtrCol()
{
	return gtk_tree_model_get_n_columns (getModel()) - 1;
}

void YGSelectionModel::addRow (GtkTreeIter *iter, YItem *item, bool honor_select)
{
	struct inner {
		static void setItemData (GtkTreeModel *model, GtkTreeIter *iter, YItem *item)
		{
			int index;
			GtkTreePath *path = gtk_tree_model_get_path (model, iter);

			int depth = gtk_tree_path_get_depth (path);
			int *path_int = gtk_tree_path_get_indices (path);
			g_assert (path_int != NULL);
			index = path_int [depth-1];

			gtk_tree_path_free (path);
			g_assert (index != -1);
			item->setData (GINT_TO_POINTER (index));
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
		gtk_tree_store_set (store, iter, getPtrCol(), item, -1);
		inner::setItemData (getModel(), iter, item);

		YTreeItem *tree_item = dynamic_cast <YTreeItem *> (item);
		if (tree_item && tree_item->isOpen())
			expand (iter);
	}
	else {
		GtkListStore *store = getListStore();
		gtk_list_store_append (store, iter);
		gtk_list_store_set (store, iter, getPtrCol(), item, -1);
		inner::setItemData (getModel(), iter, item);
	}

	if (honor_select && item->selected())
		setFocusItem (iter);
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

/*
extern "C" {
    struct FindClosure {
        const string &text;
        bool         found;
        GtkTreeIter *output;
    };
    static gboolean find_text (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
    {
		gtk_tree_store_get (getTreeStore(), iter, col, label.c_str(), -1);
    }
};
*/

bool YGSelectionModel::findByText (const string &text, GtkTreeIter *iter)
{
    g_return_val_if_fail (!isTree, false);

    return false;
/*
    FindClosure cl;
    cl.text = text;
    cl.found = false;
    cl.output = iter;
    gtk_tree_model_foreach (getModel(), find_text,
						  GtkTreeModelForeachFunc  func,
						  gpointer                 user_data);

    gtk_list_store_set (getListStore(), iter, col, label.c_str(), -1);
*/
}
