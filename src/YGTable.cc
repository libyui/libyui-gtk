#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YTable.h"
#include "YGLabeledWidget.h"

/* A generic widget for table related widgets. */
class YGTableView : public YGLabeledWidget
{
public:
	YGTableView (YWidget *y_widget, YGWidget *parent,
	             const YWidgetOpt &opt, YCPString label)
	: YGLabeledWidget (y_widget, parent, label, YD_VERT, true,
	                   GTK_TYPE_TREE_VIEW, NULL)
	{
		// Events
		if (opt.notifyMode.value()) {
			g_signal_connect (G_OBJECT (getWidget()), "row-activated",
			                  G_CALLBACK (activated_cb), y_widget);
			if (opt.immediateMode.value())
				// FIXME: doesn't seem to work
				g_signal_connect (G_OBJECT (getWidget()), "cursor-changed",
				                  G_CALLBACK (selected_cb), y_widget);
		}
	}

	void insertColumns (vector <string> headers, GType types [], bool show_headers)
	{
		GtkListStore *list = gtk_list_store_newv (headers.size(), types);
		gtk_tree_view_set_model(GTK_TREE_VIEW(getWidget()), GTK_TREE_MODEL(list));
		for (unsigned int c = 0; c < headers.size(); c++)
			// TODO: check what headers[c][0] is about
			gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW(getWidget()),
			                    c, headers[c].substr(1).c_str(), gtk_cell_renderer_text_new(),
			                    "text", c, NULL);

		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (getWidget()), show_headers);
	}

protected:
	inline GtkListStore *getStore()
	{
		return GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(getWidget())));
	}

	virtual char* widgetClass()
	{ return "YGTableView"; }

	// Add a full row
	void addItems (vector<string> elements, int index)
	{
		GtkListStore *list = getStore();
		GtkTreeIter iter;
		gtk_list_store_insert (list, &iter, index);
		for(unsigned int c = 0; c < elements.size(); c++)
			gtk_list_store_set (list, &iter, c, elements[c].c_str(), -1);
	}

	void clearItems()
	{
		gtk_list_store_clear (getStore());
	}

	void changeCell (int index, int colnum, const YCPString & text)
	{
		GtkTreeIter iter;
		GtkListStore *list = getStore();

		// Get the intended row
		GtkTreePath *path = gtk_tree_path_new_from_indices (index, -1);
		gtk_tree_model_get_iter (GTK_TREE_MODEL(list), &iter, path);
		gtk_tree_path_free (path);

		// Set the intended column
		gtk_list_store_set (list, &iter, colnum, text->value_cstr(), -1);
	}

	int getCurrentItem()
	{
		GtkTreePath *path;
		GtkTreeViewColumn *column;
		gtk_tree_view_get_cursor (GTK_TREE_VIEW(getWidget()),
		                                     &path, &column);
		if (path == NULL || column == NULL)
			return -1;

		return *gtk_tree_path_get_indices (path);
	}

	void setCurrentItem (int index)
	{
		GtkTreeModel *model = GTK_TREE_MODEL(gtk_tree_view_get_model(
		                      GTK_TREE_VIEW(getWidget())));
		GtkTreeIter it;

		if (!gtk_tree_model_get_iter_first (model, &it))
			goto setitem_error;

		while(true) {
			gchar* cur_item_str = gtk_tree_model_get_string_from_iter (model, &it);
			int cur_item = atoi (cur_item_str);
			g_free(cur_item_str);

			if (cur_item == index) {
				// TODO: test if we also need to scroll to item to ensure visibility
				// (can't currently do it since window can't be shrinked.)
				GtkTreePath *path = gtk_tree_model_get_path (model, &it);
				gtk_tree_view_set_cursor (GTK_TREE_VIEW(getWidget()), path, NULL, false);
				gtk_tree_path_free(path);
				return;
			}

			if (!gtk_tree_model_iter_next (model, &it))
				break;
		}

	setitem_error:
		y2error ("setCurrentItem(): item %d doesn't exist on %s - ignoring",
		         index, widgetClass());
	}

	static void activated_cb (GtkTreeView *tree_view, GtkTreePath *path,
	                          GtkTreeViewColumn *column, YWidget* pThis)
	{
		if (pThis->getNotify())
			YGUI::ui()->sendEvent (new YWidgetEvent (pThis, YEvent::Activated));
	}

	static void selected_cb (GtkTreeView *tree_view, YWidget* pThis)
	{
printf("TABLE SELECTED CB!!\n");
		if (pThis->getNotify() &&  !YGUI::ui()->eventPendingFor(pThis))
			YGUI::ui()->sendEvent (new YWidgetEvent (pThis, YEvent::Activated));
	}
};

class YGTable : public YTable, public YGTableView
{
public:
	YGTable (const YWidgetOpt &opt, YGWidget *parent,
	         vector <string> headers)
		: YTable (opt, headers.size()),
		  YGTableView (this, parent, opt, YCPString("<no label>"))
	{
		IMPL
		GType types [numCols()];
		for (int c = 0; c < numCols(); c++)
			types [c] = G_TYPE_STRING;
		insertColumns (headers, types, true);

		setLabelVisible (false);
	}

	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_KEYBOARD_FOCUS

	virtual void itemAdded (vector<string> elements, int index)
	{ IMPL YGTableView::addItems (elements, index); }

	virtual void itemsCleared()
	{ IMPL YGTableView::clearItems(); }

	virtual void cellChanged (int index, int colnum, const YCPString& text)
	{ IMPL YGTableView::changeCell (index, colnum, text); }

	virtual int getCurrentItem()
	{ IMPL return YGTableView::getCurrentItem (); }

	virtual void setCurrentItem (int index)
	{ IMPL YGTableView::setCurrentItem (index); }
};

YWidget *
YGUI::createTable (YWidget *parent, YWidgetOpt & opt,
	                 vector<string> headers)
{
	return new YGTable (opt, YGWidget::get (parent), headers);
}
