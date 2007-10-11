/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

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

		/* Yast tools expect the user to be unable to un-select the row. They
		   generally don't check to see if the returned value is -1. So, just
		   disallow un-selection. */
		gtk_tree_selection_set_mode (gtk_tree_view_get_selection (
			GTK_TREE_VIEW (getWidget())), GTK_SELECTION_BROWSE);

		// let the derivates do the event hooks. They have subtile differences.
	}

	void initModel (const vector <GType> &types, bool show_headers)
	{
		IMPL
		// turn vector into an array; last column dedicated to the row id
		m_colsNb = types.size();
		GType types_array [m_colsNb+1];
		for (unsigned int i = 0; i < types.size(); i++)
			types_array [i] = types[i];
		types_array[m_colsNb] = G_TYPE_INT;

		GtkListStore *list = gtk_list_store_newv (m_colsNb+1, types_array);
		gtk_tree_view_set_model (GTK_TREE_VIEW (getWidget()), GTK_TREE_MODEL (list));
		g_object_unref (G_OBJECT (list));

		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (getWidget()), show_headers);
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
			// set the last column, the expandable one, as wrapable
			if (col_nb == m_colsNb-1 && col_nb >= 3)
				g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
			column = gtk_tree_view_column_new_with_attributes (header.c_str(),
			             renderer, "text", col_nb, NULL);
		}
		else if (type == G_TYPE_BOOLEAN) {  // toggle button
			GtkCellRenderer *renderer = gtk_cell_renderer_toggle_new();
			g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (col_nb));
			g_object_set (renderer, "xalign", xalign, NULL);
			column = gtk_tree_view_column_new_with_attributes (header.c_str(),
				renderer, "active", col_nb, NULL);

			g_signal_connect (G_OBJECT (renderer), "toggled",
			                  G_CALLBACK (toggled_cb), this);
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
	}

	void setSearchCol (int col)
	{
		gtk_tree_view_set_search_column (GTK_TREE_VIEW (getWidget()), col);
	}

protected:
	inline GtkListStore *getStore()
	{ return GTK_LIST_STORE (getModel()); }
	inline GtkTreeModel *getModel()
	{ return gtk_tree_view_get_model (GTK_TREE_VIEW (getWidget())); }

protected:
	/* NOTE: gtk_list_store_insert() doesn't necessarly put the row in the
	   requested position. For instance, if the number of rows is smaller than
	   the one requested, it just appends it. So, we must translate YCP rows
	   numbering to the ones we set. We'll call YCP rows as ids.
	   Besides, if we do sorting, paths will get changed. */

	bool getRowOf (GtkTreeIter *iter, int id) {
		if (!gtk_tree_model_get_iter_first (getModel(), iter))
			return false;

		int _id = -1;
		do {
			gtk_tree_model_get (getModel(), iter, m_colsNb, &_id, -1);
			if (_id == id)
				return true;
		} while (gtk_tree_model_iter_next (getModel(), iter));
		return false;
	}

	int getIdOf (int row) {
		GtkTreeIter iter;
		if (!gtk_tree_model_iter_nth_child (getModel(), &iter, NULL, row))
			return -1;

		int id;
		gtk_tree_model_get (getModel(), &iter, m_colsNb, &id, -1);
		return id;
	}

	void addRow (int id)
	{
		IMPL
		GtkTreeIter iter;
		gtk_list_store_append (getStore(), &iter);
		gtk_list_store_set (getStore(), &iter, m_colsNb, id, -1);
	}

	void setItemText (string text, int id, int col)
	{
		IMPL
		GtkTreeIter iter;
		if (getRowOf (&iter, id))
			gtk_list_store_set (getStore(), &iter, col, text.c_str(), -1);
	}

	void setItemBool (gboolean state, int id, int col)
	{
		IMPL
		GtkTreeIter iter;
		if (getRowOf (&iter, id))
			gtk_list_store_set (getStore(), &iter, col, state, -1);
	}

	void setItemIcon (string icon, int id, int col)
	{
		IMPL
		GtkTreeIter iter;
		if (getRowOf (&iter, id)) {
			GdkPixbuf *pixbuf;
			if (icon[0] != '/')
				icon = ICON_DIR + icon;

			GError *error = 0;
			pixbuf = gdk_pixbuf_new_from_file (icon.c_str(), &error);
			if (!pixbuf)
				y2warning ("YGTable: Could not load icon: %s.\n"
				           "Because %s", icon.c_str(), error->message);

			gtk_list_store_set (getStore(), &iter, col, pixbuf, -1);
		}
	}

	void deleteRows()
	{
		IMPL
		gtk_list_store_clear (getStore());
	}

	int getCurrentRow()
	{
		IMPL
		GtkTreePath *path;
		gtk_tree_view_get_cursor (GTK_TREE_VIEW(getWidget()), &path, NULL);
		if (path == NULL)
			return -1;

		GtkTreeIter iter;
		gtk_tree_model_get_iter (getModel(), &iter, path);
		gtk_tree_path_free (path);

		int id;
		gtk_tree_model_get (getModel(), &iter, m_colsNb, &id, -1);
		return id;
	}

	void setCurrentRow (int id)
	{
		IMPL
		GtkTreeIter iter;
		if (getRowOf (&iter, id)) {
			GtkTreePath *path = gtk_tree_model_get_path (getModel(), &iter);

			g_signal_handlers_block_by_func (getWidget(), (gpointer) selected_cb, this);
			g_signal_handlers_block_by_func (getWidget(), (gpointer) selected_delayed_cb, this);
			gtk_tree_view_set_cursor (GTK_TREE_VIEW (getWidget()), path, NULL, false);
			gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (getWidget()), path, NULL,
			                              TRUE, 0.5, 0.5);
			g_signal_handlers_unblock_by_func (getWidget(), (gpointer) selected_cb, this);
			g_signal_handlers_unblock_by_func (getWidget(), (gpointer) selected_delayed_cb, this);
	
			gtk_tree_path_free (path);
		}
	}

	// toggled by user (through clicking on the renderer or some other action)
	void toggle (GtkTreePath *path, gint column)
	{
		IMPL
		GtkTreeIter iter;
		if (!gtk_tree_model_get_iter (getModel(), &iter, path))
			return;
		gboolean state;

		gtk_tree_model_get (getModel(), &iter, column, &state, -1);
		gtk_list_store_set (getStore(), &iter, column, !state, -1);

		emitEvent (YEvent::ValueChanged);
	}

	static void selected_cb (GtkTreeView *tree_view, YGTableView* pThis)
	{
		IMPL
		pThis->emitEvent (YEvent::SelectionChanged);//, true, true, true);
	}

	static void selected_delayed_cb (GtkTreeView *tree_view, YGTableView* pThis)
	{
		IMPL
		pThis->emitEvent (YEvent::SelectionChanged, true, true, false);
	}

	static void activated_cb (GtkTreeView *tree_view, GtkTreePath *path,
	                          GtkTreeViewColumn *column, YGTableView* pThis)
	{
		IMPL
		pThis->emitEvent (YEvent::Activated);
	}

	static void toggled_cb (GtkCellRendererToggle *renderer, gchar *path_str,
	                        YGTableView *pThis)
	{
		IMPL
		GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
		gint column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (renderer), "column"));
		pThis->toggle (path, column);
		gtk_tree_path_free (path);
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
		initModel (types, true);
		for (int i = 0; i < numCols(); i++)
			insertColumn (i, headers[i], types[i]);

		setLabelVisible (false);

		if (!opt.keepSorting.value())
			YGUtils::tree_view_set_sortable (GTK_TREE_VIEW (getWidget()), 0);
		if (numCols() >= 3)
			gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (getWidget()), TRUE);

		g_signal_connect (G_OBJECT (getWidget()), "row-activated",
		                  G_CALLBACK (activated_cb), (YGTableView*) this);
		if (opt.immediateMode.value())
			g_signal_connect (G_OBJECT (getWidget()), "cursor-changed",
			                  G_CALLBACK (selected_cb), (YGTableView*) this);
	}

	virtual void itemAdded (vector<string> elements, int index)
	{
		IMPL
		addRow (index);
		for (unsigned int c = 0; c < elements.size(); c++)
			setItemText (elements[c], index, c);

		// always have one row selected
		if (getCurrentRow() == -1)
			setCurrentRow (index);
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
		initModel (types, false);

		insertColumn (1, "", G_TYPE_STRING);
		setSearchCol (1);
		// pixbuf column will be added later, if needed

		g_signal_connect (G_OBJECT (getWidget()), "row-activated",
		                  G_CALLBACK (activated_cb), (YGTableView*) this);
		if (opt.immediateMode.value())
			g_signal_connect (G_OBJECT (getWidget()), "cursor-changed",
			                  G_CALLBACK (selected_cb), (YGTableView*) this);
		else
			g_signal_connect (G_OBJECT (getWidget()), "cursor-changed",
			                  G_CALLBACK (selected_delayed_cb), (YGTableView*) this);				
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
		initModel (types, false);

		insertColumn (0, "", G_TYPE_BOOLEAN);
		insertColumn (2, "", G_TYPE_STRING);
		setSearchCol (2);
		// pixbuf column will be added later, if needed

		g_signal_connect (G_OBJECT (getWidget()), "cursor-changed",
		                  G_CALLBACK (selected_cb), (YGTableView*) this);
		// Let the user toggle, using space/enter or double click (not an event).
		g_signal_connect_after (G_OBJECT (getWidget()), "row-activated",
		                        G_CALLBACK (multi_activated_cb), this);
	}

	// YMultiSelectionBox
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
		GtkTreeIter iter;
		if (!getRowOf (&iter, id))
			throw "Row doesn't exist";

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

	static void multi_activated_cb (GtkTreeView *tree_view, GtkTreePath *path,
	                                GtkTreeViewColumn *column, YGMultiSelectionBox* pThis)
	{
		IMPL
		pThis->toggle (path, 0);
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
