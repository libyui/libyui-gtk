#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YGWidget.h"

/* A generic widget for table related widgets. */
class YGTableView : public YGScrolledWidget
{
struct Pointers {
	GtkTreeModel *model;
	YWidget *y_widget;
  } pointers;

public:
	YGTableView (YWidget *y_widget, YGWidget *parent,
	             const YWidgetOpt &opt, YCPString label)
	: YGScrolledWidget (y_widget, parent, label, YD_VERT, true,
	                    GTK_TYPE_TREE_VIEW, NULL)
	{
		IMPL
		pointers.y_widget = y_widget;

		// Events
		if (opt.notifyMode.value()) {
			g_signal_connect (G_OBJECT (getWidget()), "row-activated",
			                  G_CALLBACK (activated_cb), y_widget);
			if (opt.immediateMode.value())
				g_signal_connect (G_OBJECT (getWidget()), "cursor-changed",
				                  G_CALLBACK (selected_cb), y_widget);
		}
	}

	void insertColumns (vector <string> headers, vector <GType> types,
	                    bool show_headers)
	{
		IMPL
		// Set the model
		GType types_array [types.size()];
		for (unsigned int i = 0; i < types.size(); i++)
			types_array [i] = types[i];
		GtkListStore *list = gtk_list_store_newv (headers.size(), types_array);
		gtk_tree_view_set_model(GTK_TREE_VIEW(getWidget()), GTK_TREE_MODEL(list));
		pointers.model = GTK_TREE_MODEL (list);

		// Set renderers
		for (unsigned int c = 0; c < headers.size(); c++) {
			GtkTreeViewColumn *column;

			// The allignment of the column items
			gfloat xalign = 0.0;
			if (headers[c].length()) {
				if (headers[c][0] == 'R')
					xalign = 1.0;
				else if (headers[c][0] == 'C')
					xalign = 0.5;
				// else if (headers[c][0] == 'L')
				headers[c] = headers[c].substr(1);
				}

			if (types[c] == G_TYPE_STRING) {
				GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
				g_object_set (renderer, "xalign", xalign, NULL);
				column = gtk_tree_view_column_new_with_attributes (headers[c].c_str(),
					renderer, "text", c, NULL);
			}
			else { // G_TYPE_BOOLEAN -- toggle button
				GtkCellRenderer *renderer = gtk_cell_renderer_toggle_new();
				g_object_set (renderer, "xalign", xalign, NULL);
				column = gtk_tree_view_column_new_with_attributes (headers[c].c_str(),
					renderer, "active", c, NULL);

				g_signal_connect (G_OBJECT (renderer), "toggled",
				                  G_CALLBACK (value_changed_cb), &pointers);
			}

			gtk_tree_view_insert_column (GTK_TREE_VIEW (getWidget()), column, c);
		}
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (getWidget()), show_headers);
	}

protected:
	GtkListStore *getStore()
	{ return GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(getWidget()))); }
	GtkTreeModel *getModel()
	{ return GTK_TREE_MODEL(gtk_tree_view_get_model (GTK_TREE_VIEW(getWidget()))); }

	virtual char* widgetClass()
	{ return "YGTableView"; }

	void addRow (int row)
	{
		IMPL
		GtkTreeIter iter;
		gtk_list_store_insert (getStore(), &iter, row);
	}

	void setItem (gboolean state, int row, int col)
	{
		IMPL
		GtkTreeIter iter;
		GtkTreePath *path = gtk_tree_path_new_from_indices (row, -1);
		gtk_tree_model_get_iter (getModel(), &iter, path);
		gtk_tree_path_free (path);

		gtk_list_store_set (getStore(), &iter, col, state, -1);
	}

	void setItem (string text, int row, int col)
	{
		IMPL
		GtkTreeIter iter;
		GtkTreePath *path = gtk_tree_path_new_from_indices (row, -1);
		gtk_tree_model_get_iter (getModel(), &iter, path);
		gtk_tree_path_free (path);

		gtk_list_store_set (getStore(), &iter, col, text.c_str(), -1);
	}

	void deleteRows()
	{
		gtk_list_store_clear (getStore());
	}

	int getCurrentRow()
	{
		GtkTreePath *path;
		GtkTreeViewColumn *column;
		gtk_tree_view_get_cursor (GTK_TREE_VIEW(getWidget()),
		                                     &path, &column);
		if (path == NULL || column == NULL)
			return -1;

		return *gtk_tree_path_get_indices (path);
	}

	void setCurrentRow (int row)
	{
		GtkTreePath *path = gtk_tree_path_new_from_indices (row, -1);
		gtk_tree_view_set_cursor (GTK_TREE_VIEW(getWidget()), path, NULL, false);
		gtk_tree_path_free (path);
	}

	static void selected_cb (GtkTreeView *tree_view, YWidget* pThis)
	{
		if (pThis->getNotify() &&  !YGUI::ui()->eventPendingFor(pThis))
			YGUI::ui()->sendEvent (new YWidgetEvent (pThis, YEvent::SelectionChanged));
	}

	static void activated_cb (GtkTreeView *tree_view, GtkTreePath *path,
	                          GtkTreeViewColumn *column, YWidget* pThis)
	{
		if (pThis->getNotify())
			YGUI::ui()->sendEvent (new YWidgetEvent (pThis, YEvent::Activated));
	}

	static void value_changed_cb (GtkCellRendererToggle *renderer, gchar *path_str,
	                              Pointers* pointers)
	{
	// Toggle the box
	GtkTreeModel* model = pointers->model;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	gint *column = (gint*) g_object_get_data (G_OBJECT (renderer), "column");
	GtkTreeIter iter;
	gtk_tree_model_get_iter (model, &iter, path);

	gboolean state;
	gtk_tree_model_get (model, &iter, column, &state, -1);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, !state, -1);

	gtk_tree_path_free (path);

		if (pointers->y_widget->getNotify())
			YGUI::ui()->sendEvent (new YWidgetEvent (pointers->y_widget, YEvent::ValueChanged));
	}
};

#include "YTable.h"

class YGTable : public YTable, public YGTableView
{
public:
	YGTable (const YWidgetOpt &opt, YGWidget *parent,
	         vector <string> headers)
	: YTable (opt, headers.size()),
	  YGTableView (this, parent, opt, YCPString(""))
	{
		IMPL
		vector <GType> types;
		types.assign (numCols(), G_TYPE_STRING);
		insertColumns (headers, types, true);

		setLabelVisible (false);
	}

	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_KEYBOARD_FOCUS

	virtual void itemAdded (vector<string> elements, int index)
	{
		IMPL
		addRow (index);
		for (unsigned int c = 0; c < elements.size(); c++)
			setItem (elements[c], index, c);
	}

	virtual void itemsCleared()
	{ IMPL deleteRows(); }

	virtual void cellChanged (int index, int colnum, const YCPString& text)
	{ IMPL setItem (text->value(), index, colnum); }

	virtual int getCurrentItem()
	{ IMPL return getCurrentRow (); }

	virtual void setCurrentItem (int index)
	{ IMPL setCurrentRow (index); }
};

YWidget *
YGUI::createTable (YWidget *parent, YWidgetOpt & opt,
	                 vector<string> headers)
{
	return new YGTable (opt, YGWidget::get (parent), headers);
}

#include "YSelectionBox.h"

class YGSelectionBox : public YSelectionBox, public YGTableView
{
public:
	YGSelectionBox (const YWidgetOpt &opt, YGWidget *parent,
	                const YCPString &label)
	: YSelectionBox (opt, label),
	  YGTableView (this, parent, opt, label)
	{
		vector <string> headers;
		headers.assign (1, "");
		vector <GType> types;
		types.assign (1, G_TYPE_STRING);
		/* we'll have to wait for 09 to get nicer to construct vectors :P */
		insertColumns (headers, types, false);
	}

	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_KEYBOARD_FOCUS

	// YSelectionBox
	virtual int getCurrentItem()
	{ IMPL; return getCurrentRow (); }

	virtual void setCurrentItem (int index)
	{ IMPL; setCurrentRow (index); }

	// YSelectionWidget
	virtual void itemAdded (const YCPString &str, int index, bool selected)
	{
		IMPL
		addRow (index);
		setItem (str->value(), index, 0);
		if (selected)
			setCurrentRow (index);
	}

	virtual void deleteAllItems()
	{ IMPL; deleteRows(); }
};

YWidget *
YGUI::createSelectionBox (YWidget *parent, YWidgetOpt &opt,
                          const YCPString &label)
{
	IMPL
	return new YGSelectionBox (opt, YGWidget::get (parent), label);
}


#include "YMultiSelectionBox.h"

class YGMultiSelectionBox : public YMultiSelectionBox, public YGTableView
{
public:
	YGMultiSelectionBox (const YWidgetOpt &opt, YGWidget *parent,
	                const YCPString &label)
	: YMultiSelectionBox (opt, label),
	  YGTableView (this, parent, opt, label)
	{
		vector <string> headers;
		headers.assign (2, "");
		vector <GType> types;
		types.push_back (G_TYPE_BOOLEAN);
		types.push_back (G_TYPE_STRING);
		insertColumns (headers, types, false);
	}

	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_KEYBOARD_FOCUS

	// YSelectionBox
	virtual int getCurrentItem()
	{ IMPL; return getCurrentRow (); }

	virtual void setCurrentItem (int index)
	{ IMPL; setCurrentRow (index); }

	// YSelectionWidget
	virtual void itemAdded (const YCPString &str, int index, bool selected)
	{
		IMPL
		addRow (index);
		setItem (selected, index, 0);
		setItem (str->value(), index, 1);
	}

	virtual void deleteAllItems()
	{ IMPL; deleteRows(); }

	// Toggle related methods follow
	/* Returns the state of the toggle at the row "index".
	   Pass -1 for query only. 0 and 1 to set it false or true. */
	bool itemSelected (int index, int state)
	{
		IMPL
		GtkTreeIter iter;
		GtkTreePath *path = gtk_tree_path_new_from_indices (index, -1);
		if (!gtk_tree_model_get_iter (getModel(), &iter, path))
			throw "Row doesn't exist";
		gtk_tree_path_free (path);

		if (state != -1)
			gtk_list_store_set (getStore(), &iter, 0, state, -1);

		gtk_tree_model_get (getModel(), &iter, 0, &state, -1);
		return state;
	}

	virtual bool itemIsSelected (int index)
	{
		IMPL
		bool state = 0;
		try { state = itemSelected (index, -1); }
			catch (char *str) { y2error ("%s - itemIsSelected(%d): %s - ignoring",
			                    YMultiSelectionBox::widgetClass(), index, str); }
		return state;
	}

	virtual void selectItem (int index)
	{
		IMPL
		try { itemSelected (index, 1); }
			catch (char *str) { y2error ("%s - selectItem(%d): %s - ignoring",
			                    YMultiSelectionBox::widgetClass(), index, str); }
	}

	virtual void deselectAllItems()
	{
		for (int i = 0; ; i++) {
			try { itemSelected (i, false); }
				catch (...) { break; }
		}
	}
};

YWidget *
YGUI::createMultiSelectionBox (YWidget *parent, YWidgetOpt &opt,
                          const YCPString &label)
{
	IMPL
	return new YGMultiSelectionBox (opt, YGWidget::get (parent), label);
}
