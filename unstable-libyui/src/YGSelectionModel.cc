/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>
#include <gtk/gtk.h>
#include "YGSelectionModel.h"
#include "YGUtils.h"

YGSelectionModel::YGSelectionModel (bool ordinaryModel, bool isTree)
	: isTree (isTree)
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
	return gtk_tree_model_get_iter_first (getModel(), &iter);
}

void YGSelectionModel::doAddItem (YItem *item)
{
	GtkTreeIter iter;
	addRow (&iter, item);
	setCellLabel (&iter, LABEL_COLUMN, item->label());
	setCellIcon (&iter, ICON_COLUMN, item->iconName());
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
	if (!gtk_tree_model_get_iter_first (getModel(), iter))
		return false;

	struct inner {
		static bool getIter (GtkTreeModel *model, int col, YItem *item, GtkTreeIter *iter)
		{
			gpointer ptr;
			gtk_tree_model_get (model, iter, col, &ptr, -1);
			if (((YItem *) ptr)->index() == item->index())
				return true;

			GtkTreeIter child;
			if (gtk_tree_model_iter_children (model, &child, iter)) {
				if (getIter (model, col, item, &child)) {
					*iter = child;
					return true;
				}
			}
			if (gtk_tree_model_iter_next (model, iter))
				return getIter (model, col, item, iter);
			return false;
		}
	};

	return inner::getIter (getModel(), getPtrCol(), item, iter);
}

void YGSelectionModel::implFocusItem (YItem *item)
{
	GtkTreeIter iter;
	if (getIter (item, &iter))
		focusItem (&iter);
}

int YGSelectionModel::getPtrCol()
{
	return gtk_tree_model_get_n_columns (getModel()) - 1;
}

void YGSelectionModel::addRow (GtkTreeIter *iter, YItem *item)
{
	bool empty = isEmpty();

	if (isTree) {
		GtkTreeIter *parent = NULL;
		getIter (item->parent(), parent);
		GtkTreeStore *store = getTreeStore();
		gtk_tree_store_append (store, iter, parent);
		gtk_tree_store_set (store, iter, getPtrCol(), item, -1);
	}
	else {
		GtkListStore *store = getListStore();
		gtk_list_store_append (store, iter);
		gtk_list_store_set (store, iter, getPtrCol(), item, -1);
	}

	if (item->selected() || empty)
		implFocusItem (item);
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
	GdkPixbuf *pixbuf = YGUtils::loadPixbuf (icon);
	if (isTree)
		gtk_tree_store_set (getTreeStore(), iter, col, pixbuf, -1);
	else
		gtk_list_store_set (getListStore(), iter, col, pixbuf, -1);
}

void YGSelectionModel::setCellToggle (GtkTreeIter *iter, int col, bool selected)
{
	if (isTree)
		gtk_tree_store_set (getTreeStore(), iter, col, selected, -1);
	else
		gtk_list_store_set (getListStore(), iter, col, selected, -1);
}

