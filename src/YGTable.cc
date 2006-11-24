//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YGWidget.h"

/* A generic widget for table related widgets. */
class YGTableView : public YGScrolledWidget
{
protected:
	int m_colsNb;

public:
	YGTableView (YWidget *y_widget, YGWidget *parent,
	             const YWidgetOpt &opt, YCPString label)
	: YGScrolledWidget (y_widget, parent, label, YD_VERT, true,
	                    GTK_TYPE_TREE_VIEW, NULL)
	{
		IMPL
		m_colsNb = 0;
		// Events
		if (opt.notifyMode.value()) {
			g_signal_connect (G_OBJECT (getWidget()), "row-activated",
			                  G_CALLBACK (activated_cb), y_widget);
			if (opt.immediateMode.value())
				g_signal_connect (G_OBJECT (getWidget()), "cursor-changed",
				                  G_CALLBACK (selected_cb), y_widget);
		}
	}

	void initModel (const vector <GType> &types,
	                bool show_headers, bool clickable_headers)
	{
		IMPL
		// turn vector into an array
		GType types_array [types.size()];
		for (unsigned int i = 0; i < types.size(); i++)
			types_array [i] = types[i];

		GtkListStore *list = gtk_list_store_newv (types.size(), types_array);
		gtk_tree_view_set_model (GTK_TREE_VIEW (getWidget()), GTK_TREE_MODEL (list));
		g_object_unref (G_OBJECT (list));

		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (getWidget()), show_headers);
		if (clickable_headers && show_headers)
			YGUtils::tree_view_set_sortable (GTK_TREE_VIEW (getWidget()));
	}

	void insertColumn (int col_nb, string header, GType type)
	{
		IMPL
		GtkTreeViewColumn *column = 0;

		// The allignment of the column items
		gfloat xalign = 0.0;
		if (header.length()) {
			if (header[0] == 'R')
				xalign = 1.0;
			else if (header[0] == 'C')
				xalign = 0.5;
			// else if (header[0] == 'L')
			header = header.substr(1);
			}

		if (type == G_TYPE_STRING) {
			GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
			g_object_set (renderer, "xalign", xalign, NULL);
			column = gtk_tree_view_column_new_with_attributes (header.c_str(),
			             renderer, "text", col_nb, NULL);
		}
		else if (type == G_TYPE_BOOLEAN) {  // toggle button
			GtkCellRenderer *renderer = gtk_cell_renderer_toggle_new();
			g_object_set (renderer, "xalign", xalign, NULL);
			column = gtk_tree_view_column_new_with_attributes (header.c_str(),
				renderer, "active", col_nb, NULL);

			g_signal_connect (G_OBJECT (renderer), "toggled",
				G_CALLBACK (toggled_cb), YGWidget::get (m_y_widget));
		}
		else if (type == GDK_TYPE_PIXBUF) {
			GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
			g_object_set (renderer, "xalign", xalign, NULL);
			column = gtk_tree_view_column_new_with_attributes (header.c_str(),
				renderer, "pixbuf", col_nb, NULL);
		}
		else
			g_error ("YGTable: no support for column of given type");

		gtk_tree_view_column_set_resizable (column, TRUE);
		gtk_tree_view_insert_column (GTK_TREE_VIEW (getWidget()), column, col_nb);
		m_colsNb++;
	}

protected:
	GtkListStore *getStore()
	{ return GTK_LIST_STORE (getModel()); }
	GtkTreeModel *getModel()
	{ return gtk_tree_view_get_model (GTK_TREE_VIEW (getWidget())); }

	/* NOTE: gtk_list_store_insert() doesn't necessarly put the row in the
	   requested position. For instance, if the number of rows is smaller than
	   the one requested, it just appends it. So, we must translate YCP rows
	   numbering to the ones we set. We'll call YCP rows as ids. */
	map <int, int> id2row, row2id;  // double hash
	// no need to add access methods for this...

	void addRow (int id)
	{
		IMPL
		GtkTreeIter iter;
		gtk_list_store_append (getStore(), &iter);

		// Get the position of the row
		int row;
		GtkTreePath *path = gtk_tree_model_get_path (getModel(), &iter);
		row = gtk_tree_path_get_indices (path) [0];
		gtk_tree_path_free (path);

		id2row [id]  = row;
		row2id [row] = id;
	}

	void setItemText (string text, int id, int col)
	{
		IMPL
		int row = id2row [id];

		GtkTreeIter iter;
		GtkTreePath *path = gtk_tree_path_new_from_indices (row, -1);
		gtk_tree_model_get_iter (getModel(), &iter, path);
		gtk_tree_path_free (path);

		gtk_list_store_set (getStore(), &iter, col, text.c_str(), -1);
	}

	void setItemBool (gboolean state, int id, int col)
	{
		IMPL
		int row = id2row [id];

		GtkTreeIter iter;
		GtkTreePath *path = gtk_tree_path_new_from_indices (row, -1);
		gtk_tree_model_get_iter (getModel(), &iter, path);
		gtk_tree_path_free (path);

		gtk_list_store_set (getStore(), &iter, col, state, -1);
	}

	void setItemIcon (string icon, int id, int col)
	{
		IMPL
		int row = id2row [id];

		GdkPixbuf *pixbuf;
		if (icon[0] != '/')
			icon = ICON_DIR + icon;

		GError *error = 0;
		pixbuf = gdk_pixbuf_new_from_file (icon.c_str(), &error);
		if (!pixbuf)
			y2warning ("YGTable: Could not load icon: %s.\n"
			           "Because %s", icon.c_str(), error->message);

		GtkTreeIter iter;
		GtkTreePath *path = gtk_tree_path_new_from_indices (row, -1);
		gtk_tree_model_get_iter (getModel(), &iter, path);
		gtk_tree_path_free (path);
		gtk_list_store_set (getStore(), &iter, col, pixbuf, -1);
	}

	void deleteRows()
	{
		IMPL
		id2row.clear();
		row2id.clear();
		gtk_list_store_clear (getStore());
	}

	int getCurrentRow()
	{
		IMPL
		GtkTreePath *path;
		gtk_tree_view_get_cursor (GTK_TREE_VIEW(getWidget()), &path, NULL);
		if (path == NULL)
			return -1;

		int row = gtk_tree_path_get_indices (path) [0];
		gtk_tree_path_free (path);

		return row2id [row];
	}

	void setCurrentRow (int id)
	{
		IMPL
		int row = id2row [id];

		GtkTreePath *path = gtk_tree_path_new_from_indices (row, -1);

		g_signal_handlers_block_by_func (getWidget(), (gpointer) selected_cb, this);
		gtk_tree_view_set_cursor (GTK_TREE_VIEW (getWidget()), path, NULL, false);
		g_signal_handlers_unblock_by_func (getWidget(), (gpointer) selected_cb, this);

		gtk_tree_path_free (path);
	}

	static void selected_cb (GtkTreeView *tree_view, YWidget* pThis)
	{
		IMPL
		if (pThis->getNotify() &&  !YGUI::ui()->eventPendingFor(pThis))
			YGUI::ui()->sendEvent (new YWidgetEvent (pThis, YEvent::SelectionChanged));
	}

	static void activated_cb (GtkTreeView *tree_view, GtkTreePath *path,
	                          GtkTreeViewColumn *column, YWidget* pThis)
	{
		IMPL
		if (pThis->getNotify())
			YGUI::ui()->sendEvent (new YWidgetEvent (pThis, YEvent::Activated));
	}

	static void toggled_cb (GtkCellRendererToggle *renderer, gchar *path_str,
	                        YGWidget *pThis)
	{
		IMPL
		// Toggle the box
		GtkTreeModel* model = ((YGTableView*) pThis)->getModel();
		GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
		gint *column = (gint*) g_object_get_data (G_OBJECT (renderer), "column");
		GtkTreeIter iter;
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_path_free (path);

		gboolean state;
		gtk_tree_model_get (model, &iter, column, &state, -1);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, !state, -1);

		pThis->emitEvent (YEvent::ValueChanged);
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
		initModel (types, true, !opt.keepSorting.value());
		for (int i = 0; i < numCols(); i++)
			insertColumn (i, headers[i], types[i]);

#if GTK_CHECK_VERSION(2,10,0)
		gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (getWidget()),
		                              GTK_TREE_VIEW_GRID_LINES_BOTH);
#endif
		setLabelVisible (false);
	}

	virtual void itemAdded (vector<string> elements, int index)
	{
		IMPL
		addRow (index);
		for (unsigned int c = 0; c < elements.size(); c++)
			setItemText (elements[c], index, c);
	}

	virtual void itemsCleared()
	{ IMPL; deleteRows(); }

	virtual void cellChanged (int index, int colnum, const YCPString& text)
	{ IMPL; setItemText (text->value(), index, colnum); }

	virtual int getCurrentItem()
	{ IMPL; return getCurrentRow(); }

	virtual void setCurrentItem (int index)
	{ IMPL; setCurrentRow (index); }

	YGWIDGET_IMPL_COMMON
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
		/* we'll have to wait for 09 to get nicer to construct vectors :P */
		vector <GType> types;
		types.push_back (GDK_TYPE_PIXBUF);
		types.push_back (G_TYPE_STRING);
		initModel (types, false, false);

		insertColumn (1, "", G_TYPE_STRING);
		// pixbuf column will be added later, if needed
	}

	// YSelectionBox
	virtual int getCurrentItem()
	{ IMPL; return getCurrentRow(); }

	virtual void setCurrentItem (int index)
	{ IMPL; setCurrentRow (index); }

	// YSelectionWidget
	virtual void itemAdded (const YCPString &str, int index, bool selected)
	{
		IMPL
		addRow (index);

		if (hasIcons()) {
			if (m_colsNb == 1)  // make space for pixbufs
				insertColumn (0, "", GDK_TYPE_PIXBUF);
			setItemIcon (itemIcon (index)->value(), index, 0);
		}
		setItemText (str->value(), index, 1);

		if (selected)
			setCurrentRow (index);
	}

	virtual void deleteAllItems()
	{ IMPL; deleteRows(); }

	YGWIDGET_IMPL_COMMON
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
		vector <GType> types;
		types.push_back (G_TYPE_BOOLEAN);
		types.push_back (GDK_TYPE_PIXBUF);
		types.push_back (G_TYPE_STRING);
		initModel (types, false, false);

		insertColumn (0, "", G_TYPE_BOOLEAN);
		insertColumn (2, "", G_TYPE_STRING);
		// pixbuf column will be added later, if needed
	}

	// YSelectionBox
	virtual int getCurrentItem()
	{ IMPL; return getCurrentRow(); }

	virtual void setCurrentItem (int index)
	{ IMPL; setCurrentRow (index); }

	// YSelectionWidget
	virtual void itemAdded (const YCPString &str, int index, bool selected)
	{
		IMPL
		addRow (index);

		if (hasIcons()) {
			if (m_colsNb == 2)  // make space for pixbufs
				insertColumn (1, "", GDK_TYPE_PIXBUF);
			setItemIcon (itemIcon (index)->value(), index, 1);
		}
		setItemBool (selected, index, 0);
		setItemText (str->value(), index, 2);
	}

	virtual void deleteAllItems()
	{ IMPL; deleteRows(); }

	// Toggle related methods follow
	/* Returns the state of the toggle at the row "index".
	   Pass -1 for query only. 0 and 1 to set it false or true. */
	bool itemSelected (int id, int state)
	{
		IMPL
		int row = id2row [id];

		GtkTreeIter iter;
		GtkTreePath *path = gtk_tree_path_new_from_indices (row, -1);
		if (!gtk_tree_model_get_iter (getModel(), &iter, path)) {
			gtk_tree_path_free (path);
			throw "Row doesn't exist";
		}
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
		for (int i = 0; ; i++)
			try { itemSelected (i, false); }
				catch (...) { break; }
	}

	YGWIDGET_IMPL_COMMON
};

YWidget *
YGUI::createMultiSelectionBox (YWidget *parent, YWidgetOpt &opt,
                          const YCPString &label)
{
	IMPL
	return new YGMultiSelectionBox (opt, YGWidget::get (parent), label);
}
