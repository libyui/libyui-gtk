/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YGWidget.h"
#include "YSelectionWidget.h"
#include "YGSelectionModel.h"
#include "ygtkcellrenderertextpixbuf.h"

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
	, YGSelectionModel ((YSelectionWidget *) ywidget, ordinaryModel, isTree)
	{
		IMPL
		if (ordinaryModel) {
			appendIconTextColumn ("", YAlignUnchanged, YGSelectionModel::ICON_COLUMN,
			                      YGSelectionModel::LABEL_COLUMN);
			setModel();
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

	void appendIconTextColumn (string header, YAlignmentType align, int icon_col, int text_col)
	{
		IMPL
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;

		renderer = ygtk_cell_renderer_text_pixbuf_new();
		column = gtk_tree_view_column_new_with_attributes (header.c_str(),
			renderer, "pixbuf", icon_col, "text", text_col, NULL);

		gfloat xalign = -1;
		switch (align) {
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
		if (xalign != -1)
			g_object_set (renderer, "xalign", xalign, NULL);

		gtk_tree_view_column_set_resizable (column, TRUE);
		gtk_tree_view_append_column (getView(), column);
	}

	void appendCheckColumn (string header, int bool_col)
	{
		IMPL
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;

		renderer = gtk_cell_renderer_toggle_new();
		g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (bool_col));
		column = gtk_tree_view_column_new_with_attributes (header.c_str(),
			renderer, "active", bool_col, NULL);

		g_signal_connect (G_OBJECT (renderer), "toggled",
		                  G_CALLBACK (toggled_cb), this);

		gtk_tree_view_column_set_resizable (column, TRUE);
		gtk_tree_view_append_column (getView(), column);
	}

	void setModel()
	{ gtk_tree_view_set_model (GTK_TREE_VIEW (getWidget()), getModel()); }

	virtual bool immediateEvent() { return true; }

	// YGSelectionModel
	virtual void setFocusItem (GtkTreeIter *iter)
	{
		blockEvents();
		GtkTreePath *path = gtk_tree_model_get_path (getModel(), iter);

		gtk_tree_view_expand_to_path (getView(), path);

		GtkTreeSelection *selection = gtk_tree_view_get_selection (getView());
		gtk_tree_selection_select_path (selection, path);
		gtk_tree_view_set_cursor (getView(), path, NULL, FALSE);
		gtk_tree_view_scroll_to_cell (getView(), path, NULL, TRUE, 0.5, 0.5);
		gtk_tree_path_free (path);
		unblockEvents();
	}

	virtual void unsetFocus()
	{
		blockEvents();
		GtkTreeSelection *selection = gtk_tree_view_get_selection (getView());
		gtk_tree_selection_unselect_all (selection);
		unblockEvents();
	}

	virtual YItem *focusItem()
	{
		GtkTreeSelection *selection = gtk_tree_view_get_selection (getView());
		GtkTreeIter iter;
		if (gtk_tree_selection_get_selected (selection, NULL, &iter))
			return getItem (&iter);
		return NULL;
	}

protected:
	void blockEvents()
	{
		g_signal_handlers_block_by_func (getWidget(), (gpointer) selected_cb, this);
	}
	void unblockEvents()
	{
		g_signal_handlers_unblock_by_func (getWidget(), (gpointer) selected_cb, this);
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
		getItem (&iter)->setSelected (state);
		emitEvent (YEvent::ValueChanged);
	}

	static void selected_cb (GtkTreeView *tree_view, YGTableView* pThis)
	{
		IMPL
		if (pThis->immediateEvent())
			pThis->emitEvent (YEvent::SelectionChanged, true, true);
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
		gtk_tree_view_set_rules_hint (getView(), columns() > 1);

		vector <GType> types;
		for (int i = 0; i < columns(); i++) {
			types.push_back (GDK_TYPE_PIXBUF);
			types.push_back (G_TYPE_STRING);
		}
		createModel (types);
		for (int i = 0; i < columns(); i++) {
			int col = i*2;

			YAlignmentType align = alignment (i);
			// last column is expandable, so don't let it be aligned to the right
			if (i == columns()-1)
				align = YAlignBegin;

			appendIconTextColumn (header (i), align, col, col+1);
		}
		setModel();

		g_signal_connect (G_OBJECT (getWidget()), "row-activated",
		                  G_CALLBACK (activated_cb), (YGTableView*) this);
		g_signal_connect (G_OBJECT (getWidget()), "cursor-changed",
		                  G_CALLBACK (selected_cb), (YGTableView*) this);
		if (!keepSorting())
			setSortable (true);
	}

	virtual void setKeepSorting (bool keepSorting)
	{
		YTable::setKeepSorting (keepSorting);
		setSortable (!keepSorting);
		if (!keepSorting) {
			GtkTreeViewColumn *column = gtk_tree_view_get_column (getView(), 0);
			if (column)
				gtk_tree_view_column_clicked (column);
		}
	}

	virtual void addItem (YItem *_item)
	{
    	YTable::addItem (_item);
    	YTableItem *item = dynamic_cast <YTableItem *> (_item);
    	if (item) {
			GtkTreeIter iter;
			addRow (&iter, _item, true);
   			for (int i = 0; i < columns(); i++)
   				setCell (&iter, item->cell (i));
    	}
    	else
			yuiError() << "Can only add YTableItems to a YTable.\n";
    }

	virtual void cellChanged (const YTableCell *cell)
	{
		GtkTreeIter iter;
		if (getIter (cell->parent(), &iter))
			setCell (&iter, cell);
	}

	void setCell (GtkTreeIter *iter, const YTableCell *cell)
	{
		int index = cell->column() * 2;
   		setCellIcon (iter, index, cell->iconName());
   		setCellLabel (iter, index+1, cell->label());
	}

	void setSortable (bool sortable)
	{
		IMPL
		int n = 0;
		GList *columns = gtk_tree_view_get_columns (getView());
		for (GList *i = columns; i; i = i->next, n++) {
			GtkTreeViewColumn *column = (GtkTreeViewColumn *) i->data;
			if (sortable) {
				int index = (n*2)+1;
				if (!sortable)
					index = -1;
				gtk_tree_view_column_set_sort_column_id (column, index);
			}
			else
				gtk_tree_view_column_set_clickable (column, FALSE);
		}
		g_list_free (columns);
	}

	virtual bool immediateEvent()
	{ return immediateMode(); }

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
		g_signal_connect (G_OBJECT (getWidget()), "cursor-changed",
		                  G_CALLBACK (selected_cb), (YGTableView*) this);
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
		appendCheckColumn ("", 0);
		appendIconTextColumn ("", YAlignUnchanged, 1, 2);
		setModel();

		g_signal_connect (G_OBJECT (getWidget()), "cursor-changed",
		                  G_CALLBACK (selected_cb), (YGTableView*) this);
		// Let the user toggle, using space/enter or double click (not an event).
		g_signal_connect_after (G_OBJECT (getWidget()), "row-activated",
		                        G_CALLBACK (multi_activated_cb), this);
	}

	// YMultiSelectionBox
    virtual void addItem (YItem *item)
    {
    	GtkTreeIter iter;
		addRow (&iter, item, false);
   		setCellToggle (&iter, 0, item->selected());
   		setCellIcon   (&iter, 1, item->iconName());
   		setCellLabel  (&iter, 2, item->label());
    	YMultiSelectionBox::addItem (item);
    }

	virtual void selectItem (YItem *item, bool select)
	{
		GtkTreeIter iter;
		if (getIter (item, &iter))
			setCellToggle (&iter, 0, select);
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
	{ return focusItem(); }

	virtual void setCurrentItem (YItem *item)
	{ implFocusItem (item); }

	// Events
	static void multi_activated_cb (GtkTreeView *tree_view, GtkTreePath *path,
	                                GtkTreeViewColumn *column, YGMultiSelectionBox* pThis)
	{
		IMPL
		pThis->toggle (path, 0);
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
#include "YTreeItem.h"

class YGTree : public YTree, public YGTableView
{
public:
	YGTree (YWidget *parent, const string &label)
	: YTree (NULL, label)
	, YGTableView (this, parent, label, true, true)
	{
		gtk_tree_view_set_enable_tree_lines (getView(), TRUE);
		g_signal_connect (G_OBJECT (getWidget()), "row-activated",
		                  G_CALLBACK (activated_cb), (YGTableView*) this);
		g_signal_connect (G_OBJECT (getWidget()), "cursor-changed",
		                  G_CALLBACK (selected_cb), (YGTableView*) this);
		g_signal_connect (G_OBJECT (getWidget()), "row-collapsed",
		                  G_CALLBACK (row_collapsed_cb), this);
		g_signal_connect (G_OBJECT (getWidget()), "row-expanded",
		                  G_CALLBACK (row_expanded_cb), this);
	}

	// YTree
	virtual void rebuildTree()
	{
		doDeleteAllItems();
		for (YItemConstIterator it = itemsBegin(); it != itemsEnd(); it++)
			doAddItem (*it);
	}

	virtual const YTreeItem *getCurrentItem() const
	{
		YGTree *pThis = const_cast <YGTree *> (this);
		return (YTreeItem *) pThis->focusItem();
	}

	virtual void setCurrentItem (YTreeItem *item)
	{ implFocusItem ((YItem *) item); }

	// YGSelectionModel
	virtual void expand (GtkTreeIter *iter)
	{
		GtkTreePath *path = gtk_tree_model_get_path (getModel(), iter);
		gtk_tree_view_expand_to_path (getView(), path);
		gtk_tree_path_free (path);
	}

	// callbacks
	void setRowOpen (GtkTreeIter *iter, bool open)
	{
		YTreeItem *item = static_cast <YTreeItem *> (getItem (iter));
		item->setOpen (open);
	}
	static void row_collapsed_cb (GtkTreeView *view, GtkTreeIter *iter,
	                              GtkTreePath *path, YGTree *pThis)
	{
		pThis->setRowOpen (iter, false);
	}
	static void row_expanded_cb (GtkTreeView *view, GtkTreeIter *iter,
	                             GtkTreePath *path, YGTree *pThis)
	{
		pThis->setRowOpen (iter, true);

		// we do a bit of a work-around here to mimic -qt behavior... A node can
		// be initialized as open, yet its parent, or some grand-parent, be closed.
		// We thus honor the open state when its parent gets open.
		YTreeItem *item = static_cast <YTreeItem *> (pThis->getItem (iter));
		for (YItemConstIterator it = item->childrenBegin();
		     it != item->childrenEnd(); it++) {
			const YTreeItem *child = static_cast <YTreeItem *> (*it);
			if (child->isOpen()) {
				GtkTreeIter iter;
				if (pThis->getIter (child, &iter))
					pThis->expand (&iter);
			}
		}
	}

	YGWIDGET_IMPL_COMMON
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YTree)
	YGSELECTION_WIDGET_IMPL_CLEAR (YTree)
	YGSELECTION_WIDGET_IMPL_SELECT (YTree)
};

YTree *YGWidgetFactory::createTree (YWidget *parent, const string &label)
{
	return new YGTree (parent, label);
}

