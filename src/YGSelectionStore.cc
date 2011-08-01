/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#define YUILogComponent "gtk"
#include "config.h"
#include <gtk/gtk.h>
#include <YItem.h>
#include <YSelectionWidget.h>
#include "YGUtils.h"
#include "YGSelectionStore.h"

static inline int getYItemCol (GtkTreeModel *model)
{ return gtk_tree_model_get_n_columns (model) - 2; }

static inline int getRowIdCol (GtkTreeModel *model)
{ return gtk_tree_model_get_n_columns (model) - 1; }

YGSelectionStore::YGSelectionStore (bool tree)
: isTree (tree), m_nextRowId (0)
{}

YGSelectionStore::~YGSelectionStore()
{ g_object_unref (G_OBJECT (m_model)); }

void YGSelectionStore::createStore (int cols, const GType types[])
{
	int _cols = cols + 2;
	GType _types[_cols];
	for (int i = 0; i < cols; i++)
		_types[i] = types[i];
	_types[cols+0] = G_TYPE_POINTER;  // pointer to YItem
	_types[cols+1] = G_TYPE_POINTER;  // some unique value identifying this row

	if (isTree)
		m_model = GTK_TREE_MODEL (gtk_tree_store_newv (_cols, _types));
	else
		m_model = GTK_TREE_MODEL (gtk_list_store_newv (_cols, _types));
}

void YGSelectionStore::addRow (YItem *item, GtkTreeIter *iter, GtkTreeIter *parent)
{
	if (isTree) {
		GtkTreeStore *store = GTK_TREE_STORE (m_model);
		gtk_tree_store_append (store, iter, parent);
		gtk_tree_store_set (store, iter, getYItemCol (m_model), item,
			getRowIdCol (m_model), m_nextRowId, -1);
	}
	else {
		GtkListStore *store = getListStore();
		gtk_list_store_append (store, iter);
		gtk_list_store_set (store, iter, getYItemCol (m_model), item,
			getRowIdCol (m_model), m_nextRowId, -1);
	}
	item->setData (m_nextRowId);
	m_nextRowId = GINT_TO_POINTER (GPOINTER_TO_INT (m_nextRowId) + 1);
}

void YGSelectionStore::setRowText (GtkTreeIter *iter, int iconCol, const std::string &icon, int labelCol, const std::string &label, const YSelectionWidget *widget)
{
	GdkPixbuf *pixbuf = 0;
	if (!icon.empty()) {
		std::string path (widget->iconFullPath (icon));
		pixbuf = YGUtils::loadPixbuf (path);
	}

	if (isTree)
		gtk_tree_store_set (getTreeStore(), iter, iconCol, pixbuf,
			labelCol, label.c_str(), -1);
	else
		gtk_list_store_set (getListStore(), iter, iconCol, pixbuf,
			labelCol, label.c_str(), -1);
}

void YGSelectionStore::setRowMark (GtkTreeIter *iter, int markCol, bool mark)
{
	if (isTree)
		gtk_tree_store_set (getTreeStore(), iter, markCol, mark, -1);
	else
		gtk_list_store_set (getListStore(), iter, markCol, mark, -1);
}

void YGSelectionStore::doDeleteAllItems()
{
	if (isTree)
		gtk_tree_store_clear (getTreeStore());
	else
		gtk_list_store_clear (getListStore());
	m_nextRowId = 0;
}

YItem *YGSelectionStore::getYItem (GtkTreeIter *iter)
{
	gpointer ptr;
	gtk_tree_model_get (m_model, iter, getYItemCol (m_model), &ptr, -1);
	return (YItem *) ptr;
}

static GtkTreeIter *found_iter;

void YGSelectionStore::getTreeIter (const YItem *item, GtkTreeIter *iter)
{
	struct inner {
		static gboolean foreach_find (
			GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer find_id)
		{
			gpointer id;
			gtk_tree_model_get (model, iter, getRowIdCol (model), &id, -1);
			if (id == find_id) {
				found_iter = gtk_tree_iter_copy (iter);
				return TRUE;
			}
			return FALSE;
		}
	};

	found_iter = NULL;
	gtk_tree_model_foreach (m_model, inner::foreach_find, item->data());
	*iter = *found_iter;
	gtk_tree_iter_free (found_iter);
}

GtkListStore *YGSelectionStore::getListStore()
{ return GTK_LIST_STORE (m_model); }

GtkTreeStore *YGSelectionStore::getTreeStore()
{ return GTK_TREE_STORE (m_model); }

bool YGSelectionStore::isEmpty()
{
	GtkTreeIter iter;
	return !gtk_tree_model_get_iter_first (m_model, &iter);
}

int YGSelectionStore::getTreeDepth()
{
	struct inner {
		static gboolean foreach_max_depth (
			GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer _depth)
		{
			int *depth = (int *) _depth;
			*depth = MAX (*depth, gtk_tree_path_get_depth (path));
			return FALSE;
		}
	};

	int depth = 0;
	gtk_tree_model_foreach (m_model, inner::foreach_max_depth, &depth);
	return depth;
}

#include <string.h>
static int find_col;

bool YGSelectionStore::findLabel (int labelCol, const std::string &label, GtkTreeIter *iter)
{
	struct inner {
		static gboolean foreach_find (
			GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer _value)
		{
			gchar *value = (gchar *) _value;
			gchar *v;
			gtk_tree_model_get (model, iter, find_col, &v, -1);
			if (!strcmp (v, value)) {
				found_iter = gtk_tree_iter_copy (iter);
				return TRUE;
			}
			return FALSE;
		}
	};

	find_col = labelCol;
	found_iter = NULL;
	gtk_tree_model_foreach (m_model, inner::foreach_find, (gpointer) label.c_str());
	if (found_iter) {
		*iter = *found_iter;
		gtk_tree_iter_free (found_iter);
	}
	return found_iter != NULL;
}

