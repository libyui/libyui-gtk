/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* YGtkPkgRpmGroupsView, rpm-groups tree-view */
// check the header file for information about this widget

/*
  Textdomain "gtk"
 */

#include "YGi18n.h"
#include "YGUI.h"
#include "config.h"
#include "YGUtils.h"
#include "ygtkpkgrpmgroupsview.h"
#include "ygtkpkglistview.h"
#include <YRpmGroupsTree.h>

enum {
	TEXT_COLUMN, DATA_COLUMN, TOTAL_COLUMNS
};

static void addNode (GtkTreeStore *store, GtkTreeIter *parent, const YStringTreeItem *item)
{
	if (!item) return;

	GtkTreeIter iter;
	gtk_tree_store_append (store, &iter, parent);
	gtk_tree_store_set (store, &iter,
		TEXT_COLUMN, item->value().translation().c_str(), DATA_COLUMN, item, -1);

	addNode (store, &iter, item->firstChild());
	addNode (store, parent, item->next());
}

struct YGtkPkgRpmGroupsModel {
	GtkTreeStore *store;
	YRpmGroupsTree *tree;

	GtkTreeModel *getModel()
	{ return GTK_TREE_MODEL (store); }

	YGtkPkgRpmGroupsModel()
	{
		store = gtk_tree_store_new (TOTAL_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
		tree = new YRpmGroupsTree();

		zypp::sat::LookupAttr rpmgroups (zypp::sat::SolvAttr::group);
		for (zypp::sat::LookupAttr::iterator it = rpmgroups.begin();
			 it != rpmgroups.end(); it++)
			tree->addRpmGroup (it.asString());

		GtkTreeIter iter;
		gtk_tree_store_append (store, &iter, NULL);
		gtk_tree_store_set (store, &iter,
			TEXT_COLUMN, _("All packages"), DATA_COLUMN, NULL, -1);

		addNode (store, NULL, tree->root()->firstChild());
	}

	~YGtkPkgRpmGroupsModel()
	{
		delete tree;
		g_object_unref (G_OBJECT (store));
	}

	bool writeQuery (Ypp::PoolQuery &query, GtkTreeIter *iter)
	{
		gpointer data;
		gtk_tree_model_get (getModel(), iter, DATA_COLUMN, &data, -1);
		if (data) {
			YStringTreeItem *group = (YStringTreeItem *) data;
			std::string rpm_group (tree->rpmGroup (group));
			query.addCriteria (new Ypp::RpmGroupMatch (rpm_group));
			return true;
		}
		return false;
	}
};


struct YGtkPkgRpmGroupsView::Impl {
	GtkWidget *scroll, *view;
	YGtkPkgRpmGroupsModel *model;

	Impl (YGtkPkgRpmGroupsModel *model) : model (model) {}
	~Impl() { delete model; }
};

static void selection_changed_cb (GtkTreeSelection *selection, YGtkPkgRpmGroupsView *pThis)
{
	if (gtk_tree_selection_get_selected (selection, NULL, NULL))
		pThis->notify();
}

YGtkPkgRpmGroupsView::YGtkPkgRpmGroupsView()
: YGtkPkgQueryWidget(), impl (new Impl (new YGtkPkgRpmGroupsModel()))
{
	impl->view = gtk_tree_view_new_with_model (impl->model->getModel());

	GtkTreeView *view = GTK_TREE_VIEW (impl->view);
	gtk_tree_view_set_headers_visible (view, FALSE);
	gtk_tree_view_set_search_column (view, TEXT_COLUMN);
	gtk_tree_view_set_enable_tree_lines (view, TRUE);
	gtk_tree_view_expand_all (view);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		NULL, renderer, "text", TEXT_COLUMN, NULL);
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_tree_view_append_column (view, column);

	GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
	g_signal_connect (G_OBJECT (selection), "changed",
	                  G_CALLBACK (selection_changed_cb), this);
	clearSelection();

	impl->scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (impl->scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (
		GTK_SCROLLED_WINDOW (impl->scroll), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (impl->scroll), impl->view);
	gtk_widget_show_all (impl->scroll);
}

YGtkPkgRpmGroupsView::~YGtkPkgRpmGroupsView()
{ delete impl; }

GtkWidget *YGtkPkgRpmGroupsView::getWidget()
{ return impl->scroll; }

void YGtkPkgRpmGroupsView::clearSelection()
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (impl->view));
	g_signal_handlers_block_by_func (selection, (gpointer) selection_changed_cb, this);
	{
		GtkTreeIter iter;
		gtk_tree_model_get_iter_first (impl->model->getModel(), &iter);
		gtk_tree_selection_select_iter (selection, &iter);
	}
	g_signal_handlers_unblock_by_func (selection, (gpointer) selection_changed_cb, this);
}

bool YGtkPkgRpmGroupsView::writeQuery (Ypp::PoolQuery &query)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (impl->view));
	if (gtk_tree_selection_get_selected (selection, &model, &iter))
		return impl->model->writeQuery (query, &iter);
	else {
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter_first (model, &iter)) {
			g_signal_handlers_block_by_func (selection, (gpointer) selection_changed_cb, this);
			gtk_tree_selection_select_iter (selection, &iter);
			g_signal_handlers_unblock_by_func (selection, (gpointer) selection_changed_cb, this);
		}
	}
	return false;
}

