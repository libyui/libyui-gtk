/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YGWidget.h"
#include "YSelectionWidget.h"
#include "YGSelectionModel.h"

/* A generic widget for table related widgets. */
class YGTableView : public YGScrolledWidget, public YGSelectionModel
{
protected:
	int m_colsNb;

public:
	YGTableView (YWidget *ywidget, YWidget *parent, const string &label,
	             bool ordinaryModel, bool isTree)
	: YGScrolledWidget (ywidget, parent, label, YD_VERT, true,
	                    GTK_TYPE_TREE_VIEW, NULL)
	, YGSelectionModel (ordinaryModel, isTree)
	{
		IMPL
		if (ordinaryModel) {
			addColumn (G_TYPE_STRING, "", YAlignUnchanged, YGSelectionModel::LABEL_COLUMN);
			addColumn (GDK_TYPE_PIXBUF, "", YAlignUnchanged, YGSelectionModel::ICON_COLUMN);
			gtk_tree_view_set_model (getView(), getModel());
		}
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (getWidget()), FALSE);

		/* Yast tools expect the user to be unable to un-select the row. They
		   generally don't check to see if the returned value is -1. So, just
		   disallow un-selection. */
		GtkTreeSelection *selection = gtk_tree_view_get_selection (getView());
		gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

		// let the derivates do the event hooks. They have subtile differences.
	}

	inline GtkTreeView *getView()
	{ return GTK_TREE_VIEW (getWidget()); }

	void addColumn (GType type, string header, YAlignmentType header_align,
	                int col_nb, bool isEllipsize = false)
	{
		IMPL
		GtkTreeViewColumn *column = 0;

		// The allignment of the column items
		gfloat xalign = -1;
		switch (header_align) {
			case YAlignBegin:
				xalign = 0.0;
				break;
			case YAlignCenter:
				xalign = 0.5;
				break;
			case YAlignEnd:
				xalign = 1.0;
				break;
			case YAlignUnchanged:
				break;
		}

		GtkCellRenderer *renderer = 0;
		if (type == G_TYPE_STRING) {
			renderer = gtk_cell_renderer_text_new();
			// set the last column, the expandable one, as wrapable
			if (isEllipsize)
				g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
			column = gtk_tree_view_column_new_with_attributes (header.c_str(),
			             renderer, "text", col_nb, NULL);
		}
		else if (type == GDK_TYPE_PIXBUF) {
			renderer = gtk_cell_renderer_pixbuf_new();
			column = gtk_tree_view_column_new_with_attributes (header.c_str(),
				renderer, "pixbuf", col_nb, NULL);
		}
		else if (type == G_TYPE_BOOLEAN) {  // toggle button
			renderer = gtk_cell_renderer_toggle_new();
			g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (col_nb));
			column = gtk_tree_view_column_new_with_attributes (header.c_str(),
				renderer, "active", col_nb, NULL);

			g_signal_connect (G_OBJECT (renderer), "toggled",
			                  G_CALLBACK (toggled_cb), this);
		}
		else
			g_error ("YGTable: no support for column of given type");

		if (renderer && xalign != -1)
			g_object_set (renderer, "xalign", xalign, NULL);

		gtk_tree_view_column_set_resizable (column, TRUE);
		gtk_tree_view_append_column (getView(), column);
	}

	virtual void setModel (GtkTreeModel *model)
	{ gtk_tree_view_set_model (GTK_TREE_VIEW (getWidget()), model); }

	YItem *getFocusItem()
	{
		GtkTreeIter iter;
		GtkTreePath *path;
		gtk_tree_view_get_cursor (getView(), &path, NULL);
		gtk_tree_model_get_iter (getModel(), &iter, path);
		gtk_tree_path_free (path);
		return getItem (&iter);
	}

	virtual void focusItem (GtkTreeIter *iter)
	{
		blockEvents();
		GtkTreePath *path = gtk_tree_model_get_path (getModel(), iter);
		gtk_tree_view_set_cursor (GTK_TREE_VIEW (getWidget()), path, NULL, false);
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (getWidget()), path, NULL,
		                              TRUE, 0.5, 0.5);
		gtk_tree_path_free (path);
		unblockEvents();
	}

	virtual void unfocusAllItems()
	{
		blockEvents();
		GtkTreeSelection *selection = gtk_tree_view_get_selection (getView());
		gtk_tree_selection_unselect_all (selection);
		unblockEvents();
	}

protected:
	void blockEvents()
	{
		g_signal_handlers_block_by_func (getWidget(), (gpointer) selected_cb, this);
		g_signal_handlers_block_by_func (getWidget(), (gpointer) selected_delayed_cb, this);
	}
	void unblockEvents()
	{
		g_signal_handlers_unblock_by_func (getWidget(), (gpointer) selected_cb, this);
		g_signal_handlers_unblock_by_func (getWidget(), (gpointer) selected_delayed_cb, this);
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
		state = !state;
		gtk_list_store_set (GTK_LIST_STORE (getModel()), &iter, column, state, -1);

		((YSelectionWidget *) m_ywidget)->selectItem (getItem (&iter), state);
		emitEvent (YEvent::ValueChanged);
	}

	static void selected_cb (GtkTreeView *tree_view, YGTableView* pThis)
	{
		IMPL
		pThis->emitEvent (YEvent::SelectionChanged);
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
	YGTable (YWidget *parent, YTableHeader *_header)
	: YTable (NULL, _header)
	, YGTableView (this, parent, string(), false, false)
	{
		IMPL
		gtk_tree_view_set_headers_visible (getView(), TRUE);
		if (columns() >= 3)
			gtk_tree_view_set_rules_hint (getView(), TRUE);

		vector <GType> types;
		types.assign (columns(), G_TYPE_STRING);
/*		for (int i = 0; i < columns(); i++)
			if (item->hasIconName (i))
				types[i] = GDK_TYPE_PIXBUF;*/
		createModel (types);
		for (int i = 0; i < columns(); i++) {
			bool ellipsize = columns() >= 3 && i == columns()-1;
			addColumn (types[i], header (i), alignment (i), i, ellipsize);
		}
		gtk_tree_view_set_model (getView(), getModel());

		g_signal_connect (G_OBJECT (getWidget()), "row-activated",
		                  G_CALLBACK (activated_cb), (YGTableView*) this);
		if (immediateMode())
			g_signal_connect (G_OBJECT (getWidget()), "cursor-changed",
			                  G_CALLBACK (selected_cb), (YGTableView*) this);
	}

	virtual void setKeepSorting (bool keepSorting)
	{
		// TODO: allow to toggle modes
		if (!keepSorting)
			YGUtils::tree_view_set_sortable (GTK_TREE_VIEW (getWidget()), 0);
	}

    virtual void addItem (YItem *_item)
    {
    	YTableItem *item = dynamic_cast <YTableItem *> (_item);
    	if (item) {
			GtkTreeIter iter;
			addRow (&iter, _item);
   			for (int i = 0; i < columns(); i++) {
/*   				if (item->hasIconName (i))
		    		setCellIcon (&iter, i, item->iconName (i));
		    	else*/
		    		setCellLabel (&iter, i, item->label (i));
	    	}
    	}
    	else
    		y2error ("Can only add YTableItems to a YTable.");
    	YTable::addItem (_item);
    }

	virtual void cellChanged (const YTableCell *cell)
	{
		GtkTreeIter iter;
		if (getIter (cell->parent(), &iter)) {
			if (cell->hasIconName())
				setCellIcon (&iter, cell->column(), cell->iconName());
			else
				setCellLabel (&iter, cell->column(), cell->label());
		}
	}

	YGWIDGET_IMPL_COMMON
	YGSELECTION_WIDGET_IMPL_CLEAR (YTable)
	YGSELECTION_WIDGET_IMPL_SELECT (YTable)
};

YTable *YGWidgetFactory::createTable (YWidget *parent, YTableHeader *header)
{
	return new YGTable (parent, header);	
}


#include "YSelectionBox.h"

class YGSelectionBox : public YSelectionBox, public YGTableView
{
public:
	YGSelectionBox (YWidget *parent, const string &label)
	: YSelectionBox (NULL, label),
	  YGTableView (this, parent, label, true, false)
	{
		g_signal_connect (G_OBJECT (getWidget()), "row-activated",
		                  G_CALLBACK (activated_cb), (YGTableView*) this);
		if (immediateMode())
			g_signal_connect (G_OBJECT (getWidget()), "cursor-changed",
			                  G_CALLBACK (selected_cb), (YGTableView*) this);
		else
			g_signal_connect (G_OBJECT (getWidget()), "cursor-changed",
			                  G_CALLBACK (selected_delayed_cb), (YGTableView*) this);
	}

	YGWIDGET_IMPL_COMMON
	YGSELECTION_WIDGET_IMPL_ALL (YSelectionBox)
};

YSelectionBox *YGWidgetFactory::createSelectionBox (YWidget *parent, const string &label)
{
	IMPL
	return new YGSelectionBox (parent, label);
}

#include "YMultiSelectionBox.h"

class YGMultiSelectionBox : public YMultiSelectionBox, public YGTableView
{
public:
	YGMultiSelectionBox (YWidget *parent, const string &label)
	: YMultiSelectionBox (NULL, label),
	  YGTableView (this, parent, label, false, false)
	{
		vector <GType> types;
		types.push_back (G_TYPE_BOOLEAN);
		types.push_back (GDK_TYPE_PIXBUF);
		types.push_back (G_TYPE_STRING);
		createModel (types);
		addColumn (G_TYPE_BOOLEAN, "", YAlignUnchanged, 0);
		addColumn (GDK_TYPE_PIXBUF, "", YAlignUnchanged, 1);
		addColumn (G_TYPE_STRING, "", YAlignUnchanged, 2);
		gtk_tree_view_set_model (getView(), getModel());

		g_signal_connect (G_OBJECT (getWidget()), "cursor-changed",
		                  G_CALLBACK (selected_cb), (YGTableView*) this);
		// Let the user toggle, using space/enter or double click (not an event).
		g_signal_connect_after (G_OBJECT (getWidget()), "row-activated",
		                        G_CALLBACK (multi_activated_cb), this);
	}

    virtual void addItem (YItem *item)
    {
    	GtkTreeIter iter;
		addRow (&iter, item);
   		setCellToggle (&iter, 0, item->selected());
   		setCellIcon (&iter, 1, item->iconName());
   		setCellLabel (&iter, 2, item->label());
    	YMultiSelectionBox::addItem (item);
    }

	virtual void selectItem (YItem *item)
	{
		GtkTreeIter iter;
		if (getIter (item, &iter))
			setCellToggle (&iter, 0, item->selected());
		YMultiSelectionBox::selectItem (item);
	}

	virtual void deselectAllItems()
	{
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter_first (getModel(), &iter)) {
			do {
				setCellToggle (&iter, 0, false);
			} while (gtk_tree_model_iter_next (getModel(), &iter));
		}
		YMultiSelectionBox::deselectAllItems();
	}

	virtual YItem *currentItem()
	{
		return getFocusItem();
	}

	virtual void setCurrentItem (YItem *item)
	{
		GtkTreeIter iter;
		if (getIter (item, &iter))
			focusItem (&iter);
	}

	static void multi_activated_cb (GtkTreeView *tree_view, GtkTreePath *path,
	                                GtkTreeViewColumn *column, YGMultiSelectionBox* pThis)
	{
		IMPL
		pThis->toggle (path, 1);
	}

	YGWIDGET_IMPL_COMMON
	YGSELECTION_WIDGET_IMPL_CLEAR (YMultiSelectionBox)
};

YMultiSelectionBox *YGWidgetFactory::createMultiSelectionBox (YWidget *parent,
                                                              const string &label)
{
	IMPL
	return new YGMultiSelectionBox (parent, label);
}

#include "YTree.h"

class YGTree : public YTree, public YGTableView
{
public:
	YGTree (YWidget *parent, const string &label)
	: YTree (NULL, label)
	, YGTableView (this, parent, label, false, true)
	{
		// Events
		if (notify()) {
			g_signal_connect (G_OBJECT (getWidget()), "row-activated",
			                  G_CALLBACK (activated_cb), (YGTableView*) this);
			g_signal_connect (G_OBJECT (getWidget()), "cursor-changed",
			                  G_CALLBACK (selected_delayed_cb), (YGTableView*) this);
		}
		else {
			GtkTreeSelection *selection = gtk_tree_view_get_selection (getView());
			gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);
		}
	}

	// YTree
	virtual void rebuildTree()
	{
		// we are already implement addItem()...
	}

	YGWIDGET_IMPL_COMMON
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YTree)
	YGSELECTION_WIDGET_IMPL_ALL (YTree)
};

YTree *YGWidgetFactory::createTree (YWidget *parent, const string &label)
{
	return new YGTree (parent, label);
}

