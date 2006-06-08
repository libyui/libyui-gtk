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
		GtkTreeStore *tree = gtk_tree_store_new (1, G_TYPE_STRING);
		gtk_tree_view_set_model(GTK_TREE_VIEW(getWidget()), GTK_TREE_MODEL(tree));

		gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW(getWidget()),
			0, "(no title)", gtk_cell_renderer_text_new(), "text", 0, NULL);
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (getWidget()), FALSE);

		// Events
		if (opt.notifyMode.value()) {
			g_signal_connect (G_OBJECT (getWidget()), "row-activated",
			                  G_CALLBACK (activated_cb), this);
			// TODO: add the selected signal -- after adding it to YGTable
		}
	}

	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_KEYBOARD_FOCUS
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YTree)

	GtkTreeStore *getStore()
	{
		return GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(getWidget())));
	}

	// YTree

	void addItems (const YTreeItemList &items, GtkTreeIter *parent)
	{
		GtkTreeIter iter;
		for (unsigned int i = 0; i < items.size(); i++) {
			gtk_tree_store_append (getStore(), &iter, parent);
			gtk_tree_store_set (getStore(), &iter, 0,
				items[i]->getText()->value_cstr(), -1);

			if (parent && items[i]->parent()->isOpenByDefault()) {
				GtkTreePath *path = gtk_tree_model_get_path (
					GTK_TREE_MODEL (getStore()), &iter);
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
	virtual const YTreeItem * getCurrentItem() const
	{
		return NULL;
#if 0
		GtkTreePath *path;
		GtkTreeViewColumn *column;
		gtk_tree_view_get_cursor (GTK_TREE_VIEW(getWidget()),
		                                     &path, &column);
		if (path == NULL || column == NULL)
			return NULL;

		const gint *indices = gtk_tree_path_get_indices (path);

		if (*indices == -1)
			return NULL;

		const YTreeItem* item = items[*indices];
		while (true) {
			if (*(++indices) == -1)
{
printf("current: %s\n", item->getText()->value_cstr());
				return item;
}
printf("going to item: %d\n", *indices);
			item = item->itemList()[*indices];
		}
#endif
	}

	virtual void setCurrentItem (YTreeItem *item) IMPL;
#if 0
	string getPath (const YTreeItemList &items, YTreeItem* item, string path) const
	{
fprintf(stderr, "getPath(): '%s'\n", path.c_str());
		for (unsigned int i = 0; i < items.size(); i++) {
fprintf(stderr, "iteration %d\n", i);
			if (item->getId()->equal(items[i]->getId()))
{
fprintf(stderr, "found at: '%s'\n", path.c_str());
fprintf(stderr, "%s == %s\n", item->getText()->value_cstr(), items[i]->getText()->value_cstr());
				return (path + ":" + YGUtils::intToStr(i)).substr(1);
}
			string res = getPath (items[i]->itemList(), item, path + ":" + YGUtils::intToStr(i));
			if (res[0] != ':')  // not very pretty
				return res;
		}
	return "";
	}

	virtual void setCurrentItem (YTreeItem *item)
	{
		string path = getPath (items, item, "");
		if (path.length() == 0) {
			y2error ("setCurrentItem(): item %s doesn't exist on %s - ignoring",
			         item->getText()->value_cstr(), widgetClass());
			return;
			}

fprintf (stderr, "found %s at path %s\n", item->getText()->value_cstr(), path.c_str());

		GtkTreePath *tree_path = gtk_tree_path_new_from_string (path.c_str());
		gtk_tree_view_set_cursor (GTK_TREE_VIEW(getWidget()), tree_path, NULL, FALSE);
		gtk_tree_path_free (tree_path);
	}
#endif
	virtual void deleteAllItems()
	{
		gtk_tree_store_clear (getStore());
		YTree::deleteAllItems();
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
