#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YTree.h"
#include "YGUtils.h"
#include "YGWidget.h"

class YGTree : public YTree, public YGScrolledWidget
{
public:
	YGTree (const YWidgetOpt &opt, YGWidget *parent, YCPString label)
	: YTree (opt, label)
	, YGScrolledWidget (this, parent, label, YD_VERT, true,
	                   GTK_TYPE_TREE_VIEW, NULL)
	{
		GtkTreeStore *tree = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
		gtk_tree_view_set_model(GTK_TREE_VIEW(getWidget()), GTK_TREE_MODEL(tree));

		gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW(getWidget()),
			0, "(no title)", gtk_cell_renderer_text_new(), "text", 0, NULL);
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (getWidget()), FALSE);

		// Events
		if (opt.notifyMode.value()) {
			g_signal_connect (G_OBJECT (getWidget()), "row-activated",
			                  G_CALLBACK (activated_cb), this);
			g_signal_connect (G_OBJECT (getWidget()), "cursor-changed",
			                  G_CALLBACK (selected_cb), this);
		}
	}

	void freeItemsData (const YTreeItemList &items)
	{
		for (YTreeItemListConstIterator it = items.begin();
		     it != items.end(); it++) {
			gtk_tree_path_free ((GtkTreePath*)(*it)->data());
			freeItemsData ((*it)->itemList());
		}
	}

	virtual ~YGTree()
	{ freeItemsData (items); }

	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_KEYBOARD_FOCUS
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YTree)

	GtkTreeStore *getStore()
	{ return GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(getWidget()))); }
	GtkTreeModel *getModel()
	{ return GTK_TREE_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(getWidget()))); }

	// YTree

	void addItems (const YTreeItemList &items, GtkTreeIter *parent)
	{
		GtkTreeIter iter;
		for (unsigned int i = 0; i < items.size(); i++) {
			gtk_tree_store_append (getStore(), &iter, parent);
			gtk_tree_store_set (getStore(), &iter, 0,
				items[i]->getText()->value_cstr(), 1, items[i], -1);

			// store pointer to GtkTreePath for use in setCurrentItem()
			items[i]->setData (gtk_tree_model_get_path (getModel(), &iter));

			if (parent && items[i]->parent()->isOpenByDefault()) {
				GtkTreePath *path = gtk_tree_model_get_path (getModel(), &iter);
				gtk_tree_view_expand_to_path (GTK_TREE_VIEW (getWidget()), path);
				gtk_tree_path_free(path);
				}

			addItems (items[i]->itemList(), &iter);
		}
	}

	virtual void rebuildTree()
	{
		gtk_tree_store_clear (getStore());
		addItems (items, NULL);
	}

protected:
	virtual const YTreeItem *getCurrentItem() const
	{
		GtkTreePath *path;
		GtkTreeViewColumn *column;
		gtk_tree_view_get_cursor
			(GTK_TREE_VIEW(const_cast<YGTree *>(this)->getWidget()),
			&path, &column);
		if (path == NULL || column == NULL)
			return NULL;

		GtkTreeIter iter;
		YTreeItem *item = 0;
		gtk_tree_model_get_iter
			(const_cast<YGTree *>(this)->getModel(), &iter, path);
		gtk_tree_model_get
			(const_cast<YGTree *>(this)->getModel(), &iter, 1, &item, -1);

		return item;
	}

	virtual void setCurrentItem (YTreeItem *item)
	{
		IMPL
		GtkTreePath *path = (GtkTreePath*) item->data();
		gtk_tree_view_expand_to_path (GTK_TREE_VIEW (getWidget()), path);
		gtk_tree_view_set_cursor (GTK_TREE_VIEW(getWidget()), path, NULL, FALSE);
	}

	virtual void deleteAllItems()
	{
		gtk_tree_store_clear (getStore());
		YTree::deleteAllItems();
	}

	static void selected_cb (GtkTreeView *tree_view, YWidget* pThis)
	{
		if (pThis->getNotify() &&  !YGUI::ui()->eventPendingFor(pThis))
			YGUI::ui()->sendEvent (new YWidgetEvent (pThis, YEvent::SelectionChanged));
	}

	static void activated_cb (GtkTreeView *tree_view, GtkTreePath *path,
	                          GtkTreeViewColumn *column, YGTree* pThis)
	{
		if (pThis->getNotify())
			YGUI::ui()->sendEvent (new YWidgetEvent (pThis, YEvent::Activated));
	}
};

YWidget *
YGUI::createTree (YWidget *parent, YWidgetOpt & opt, const YCPString & label)
{
	return new YGTree (opt, YGWidget::get (parent), label);
}
