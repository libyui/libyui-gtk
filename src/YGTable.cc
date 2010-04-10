/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#define YUILogComponent "gtk"
#include "config.h"
#include "YGUI.h"
#include "YGUtils.h"
#include "YGWidget.h"
#include "YSelectionWidget.h"
#include "YGSelectionModel.h"
#include "ygtkcellrenderertextpixbuf.h"
#include "ygtktreeview.h"

/* A generic widget for table related widgets. */
class YGTableView : public YGScrolledWidget, public YGSelectionModel
{
protected:
	int m_colsNb;
	guint m_blockTimeout;

public:
	YGTableView (YWidget *ywidget, YWidget *parent, const string &label,
	             bool ordinaryModel, bool isTree)
	: YGScrolledWidget (ywidget, parent, label, YD_VERT,
	                    YGTK_TYPE_TREE_VIEW, NULL)
	, YGSelectionModel ((YSelectionWidget *) ywidget, ordinaryModel, isTree)
	{
		if (ordinaryModel) {
			appendIconTextColumn ("", YAlignUnchanged, YGSelectionModel::ICON_COLUMN,
			                      YGSelectionModel::LABEL_COLUMN);
			setModel();
		}
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (getWidget()), FALSE);

		/* Yast tools expect the user to be unable to un-select the row. They
		   generally don't check to see if the returned value is -1. So, just
		   disallow un-selection. */
		gtk_tree_selection_set_mode (getSelection(), GTK_SELECTION_BROWSE);

		// let the derivates do the event hooks. They have subtile differences.

		m_blockTimeout = 0;  // GtkTreeSelection idiotically fires when showing widget
		g_signal_connect (getWidget(), "map", G_CALLBACK (block_init_cb), this);
	}

	virtual ~YGTableView()
	{
		if (m_blockTimeout) g_source_remove (m_blockTimeout);
	}

	inline GtkTreeView *getView()
	{ return GTK_TREE_VIEW (getWidget()); }
	inline GtkTreeSelection *getSelection()
	{ return gtk_tree_view_get_selection (getView()); }

	void appendIconTextColumn (string header, YAlignmentType align, int icon_col, int text_col)
	{
		GtkCellRenderer *renderer = ygtk_cell_renderer_text_pixbuf_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (
			header.c_str(), renderer, "pixbuf", icon_col, "text", text_col, NULL);

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
		if (xalign != -1) {
			g_object_set (renderer, "xalign", xalign, NULL);
			//gtk_tree_view_column_set_alignment (column, xalign);
		}

		gtk_tree_view_column_set_resizable (column, TRUE);
		ygtk_tree_view_append_column (YGTK_TREE_VIEW (getView()), column);
	}

	void appendCheckColumn (string header, int bool_col)
	{
		GtkCellRenderer *renderer = gtk_cell_renderer_toggle_new();
		g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (bool_col));
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (
			header.c_str(), renderer, "active", bool_col, NULL);
		g_signal_connect (G_OBJECT (renderer), "toggled",
		                  G_CALLBACK (toggled_cb), this);

		gtk_tree_view_column_set_resizable (column, TRUE);
		ygtk_tree_view_append_column (YGTK_TREE_VIEW (getView()), column);
	}

	void appendEmptyColumn()
	{ gtk_tree_view_append_column (getView(), gtk_tree_view_column_new()); }

	void setModel()
	{ gtk_tree_view_set_model (GTK_TREE_VIEW (getWidget()), getModel()); }

	virtual bool immediateEvent() { return true; }

	static gboolean block_selected_timeout_cb (gpointer data)
	{
		YGTableView *pThis = (YGTableView *) data;
		pThis->m_blockTimeout = 0;
		return FALSE;
	}

	void blockSelected()
	{  // GtkTreeSelection only fires when idle; so set a timeout
		if (m_blockTimeout) g_source_remove (m_blockTimeout);
		m_blockTimeout = g_timeout_add_full (G_PRIORITY_LOW, 250, block_selected_timeout_cb, this, NULL);
	}

	// YGSelectionModel
	virtual void doSelectItem (GtkTreeIter *iter)
	{
		if (!gtk_tree_selection_iter_is_selected (getSelection(), iter)) {
			blockSelected();
			GtkTreePath *path = gtk_tree_model_get_path (getModel(), iter);
			gtk_tree_view_expand_to_path (getView(), path);

			if (gtk_tree_selection_get_mode (getSelection()) != GTK_SELECTION_MULTIPLE)
				gtk_tree_view_scroll_to_cell (getView(), path, NULL, TRUE, 0.5, 0);
			gtk_tree_path_free (path);

			gtk_tree_selection_select_iter (getSelection(), iter);
		}
	}

	virtual void doUnselectAll()
	{
		if (gtk_tree_selection_count_selected_rows (getSelection())) {
			blockSelected();
			gtk_tree_selection_unselect_all (getSelection());
		}
	}

	virtual YItem *doSelectedItem()
	{
		GtkTreeIter iter;
		if (gtk_tree_selection_get_selected (getSelection(), NULL, &iter))
			return getItem (&iter);
		return NULL;
	}

	// peculiarities
	virtual bool toggleMode() const
	{ return false; }  // YMultiSelectionBox

	virtual bool isShrinkable() { return false; }

	virtual unsigned int getMinSize (YUIDimension dim)
	{
		if (dim == YD_VERT) {
			int height = YGUtils::getCharsHeight (getWidget(), isShrinkable() ? 2 : 5);
			return MAX (80, height);
		}
		return 80;
	}

	// GTK callbacks
	static void block_init_cb (GtkWidget *widget, YGTableView *pThis)
	{ pThis->blockSelected(); }

	// toggled by user (through clicking on the renderer or some other action)
	void toggle (GtkTreePath *path, gint column)
	{
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

	static void selection_changed_cb (GtkTreeSelection *selection, YGTableView *pThis)
	{
		if (pThis->m_blockTimeout)
			return;
		if (!pThis->toggleMode()) {
			GtkTreeSelection *selection = pThis->getSelection();
			for (YItemConstIterator it = pThis->itemsBegin(); it != pThis->itemsEnd(); it++) {
				GtkTreeIter iter;
				if (pThis->getIter (*it, &iter)) {
					bool sel = gtk_tree_selection_iter_is_selected (selection, &iter);
					(*it)->setSelected (sel);
				}
			}
		}
		if (pThis->immediateEvent())
			pThis->emitEvent (YEvent::SelectionChanged, IF_NOT_PENDING_EVENT);
	}

	static void activated_cb (GtkTreeView *tree_view, GtkTreePath *path,
	                          GtkTreeViewColumn *column, YGTableView* pThis)
	{
		pThis->emitEvent (YEvent::Activated);
	}

	static void toggled_cb (GtkCellRendererToggle *renderer, gchar *path_str,
	                        YGTableView *pThis)
	{
		GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
		gint column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (renderer), "column"));
		pThis->toggle (path, column);
		gtk_tree_path_free (path);
	}

#if YAST2_VERSION > 2018003
	static void right_click_cb (YGtkTreeView *view, gboolean outreach, YGTableView *pThis)
	{ pThis->emitEvent (YEvent::ContextMenuActivated); }
#endif
};

#include "YTable.h"
#include "YGDialog.h"
#include <gdk/gdkkeysyms.h>
#include <string.h>

class YGTable : public YTable, public YGTableView
{
public:
#if YAST2_VERSION >= 2017005
	YGTable (YWidget *parent, YTableHeader *headers, bool multiSelection)
	: YTable (NULL, headers, multiSelection)
#else
	YGTable (YWidget *parent, YTableHeader *headers)
	: YTable (NULL, headers)
#endif
	, YGTableView (this, parent, string(), false, false)
	{
		gtk_tree_view_set_headers_visible (getView(), TRUE);
		gtk_tree_view_set_rules_hint (getView(), columns() > 1);
#if YAST2_VERSION >= 2017005
		if (multiSelection)
			gtk_tree_selection_set_mode (getSelection(), GTK_SELECTION_MULTIPLE);
#endif
		vector <GType> types;
		for (int i = 0; i < columns(); i++) {
			types.push_back (GDK_TYPE_PIXBUF);
			types.push_back (G_TYPE_STRING);
		}
		createModel (types);
		for (int i = 0; i < columns(); i++) {
			int col = i*2;
			YAlignmentType align = alignment (i);
			appendIconTextColumn (header (i), align, col, col+1);
			if ((align == YAlignCenter || align == YAlignEnd) && i == columns()-1)
				// if last col is aligned: add another so that it doesn't expand.
				appendEmptyColumn();
		}
		setModel();
		if (!keepSorting())
			setSortable (true);

		connect (getWidget(), "row-activated", G_CALLBACK (activated_cb), (YGTableView *) this);
		connect (getSelection(), "changed", G_CALLBACK (selection_changed_cb), (YGTableView *) this);
		connect (getWidget(), "right-click", G_CALLBACK (hack_right_click_cb), this);
		connect (getWidget(), "key-press-event", G_CALLBACK (key_press_event_cb), this);
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

	virtual void cellChanged (const YTableCell *cell)
	{
		GtkTreeIter iter;
		if (getIter (cell->parent(), &iter))
			setCell (&iter, cell->column(), cell);
	}

	void setCell (GtkTreeIter *iter, int column, const YTableCell *cell)
	{
		int index = column * 2;
		std::string icon, label;
		if (cell) {
			icon = cell->iconName();
			label = cell->label();
	   		if (label == "X")
	   			label = YUI::app()->glyph (YUIGlyph_CheckMark);
		}
   		setCellIcon (iter, index, icon);
   		setCellLabel (iter, index+1, label);
	}

	void setSortable (bool sortable)
	{
		if (!sortable && !GTK_WIDGET_REALIZED (getWidget()))
			return;
		int n = 0;
		GList *columns = gtk_tree_view_get_columns (getView());
		for (GList *i = columns; i; i = i->next, n++) {
			GtkTreeViewColumn *column = (GtkTreeViewColumn *) i->data;
			if (n >= this->columns())
				break;
			if (sortable) {
				int index = (n*2)+1;
				if (!sortable)
					index = -1;
				gtk_tree_sortable_set_sort_func (
					GTK_TREE_SORTABLE (getModel()), index, tree_sort_cb,
					GINT_TO_POINTER (index), NULL);
				gtk_tree_view_column_set_sort_column_id (column, index);
			}
			else
				gtk_tree_view_column_set_sort_column_id (column, -1);
		}
		g_list_free (columns);
	}

	virtual bool immediateEvent()
	{ return immediateMode(); }

	// YGWidget

	virtual unsigned int getMinSize (YUIDimension dim)
	{ return 30; }

	// YGSelectionModel

	virtual void doAddItem (YItem *_item)
	{
    	YTableItem *item = dynamic_cast <YTableItem *> (_item);
    	if (item) {
			GtkTreeIter iter;
			addRow (&iter, _item, true);
   			for (int i = 0; i < columns(); i++)
   				setCell (&iter, i, item->cell (i));
    	}
    	else
			yuiError() << "Can only add YTableItems to a YTable.\n";
    }

	// callbacks

	// hack up a popup menu and honor the delete key as suggested at:
	// http://mvidner.blogspot.com/2009/01/yast-ui-table-usability.html
	static void activateButton (YWidget *button)
	{
		YWidgetEvent *event = new YWidgetEvent (button, YEvent::Activated);
		YGUI::ui()->sendEvent (event);
	}

	static void hack_right_click_cb (YGtkTreeView *view, gboolean outreach, YGTable *pThis)
	{
#if YAST2_VERSION > 2018003
		if (pThis->notifyContextMenu())
			return YGTableView::right_click_cb (view, outreach, pThis);
#endif
		GtkWidget *menu = gtk_menu_new();

		if (!YGDialog::currentDialog()->getFunctionWidget (5) ||
				YGDialog::currentDialog()->getClassWidgets ("YTable").size() > 1)
			// more than one table exists, or no function key 5 assigned
			gtk_widget_error_bell (GTK_WIDGET (view));
		else {
			struct inner {
				static void key_activate_cb (GtkMenuItem *item, YWidget *button)
				{ activateButton (button); }
				static void appendItem (GtkWidget *menu, const gchar *stock, int key)
				{
					YWidget *button = YGDialog::currentDialog()->getFunctionWidget (key);
					if (button) {
						GtkWidget *item;
						item = gtk_image_menu_item_new_from_stock (stock, NULL);
						gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
						g_signal_connect (G_OBJECT (item), "activate",
										  G_CALLBACK (key_activate_cb), button);
					}
				}
			};

			if (outreach)
				inner::appendItem (menu, GTK_STOCK_ADD, 3);
			else {
				inner::appendItem (menu, GTK_STOCK_EDIT, 4);
				inner::appendItem (menu, GTK_STOCK_DELETE, 5);
			}
		}

		menu = ygtk_tree_view_append_show_columns_item (YGTK_TREE_VIEW (view), menu);
		ygtk_tree_view_popup_menu (view, menu);
	}

	static gboolean key_press_event_cb (GtkWidget *widget, GdkEventKey *event, YGTable *pThis)
	{
		if (event->keyval == GDK_Delete) {
			YWidget *button = YGDialog::currentDialog()->getFunctionWidget (5);
			if (button)
				activateButton (button);
			else
				gtk_widget_error_bell (widget);
			return TRUE;
		}
		return FALSE;
	}

	static gint tree_sort_cb (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b,
	                           gpointer _index)
	{
		int index = GPOINTER_TO_INT (_index);
		gchar *str_a, *str_b;
		gtk_tree_model_get (model, a, index, &str_a, -1);
		gtk_tree_model_get (model, b, index, &str_b, -1);
		int ret = strcmp (str_a, str_b);
		g_free (str_a);
		g_free (str_b);
		return ret;
	}

	YGLABEL_WIDGET_IMPL (YTable)
	YGSELECTION_WIDGET_IMPL (YTable)
};

#if YAST2_VERSION >= 2017005
YTable *YGWidgetFactory::createTable (YWidget *parent, YTableHeader *headers,
                                      bool multiSelection)
{
	return new YGTable (parent, headers, multiSelection);
}
#else
YTable *YGWidgetFactory::createTable (YWidget *parent, YTableHeader *headers)
{
	return new YGTable (parent, headers);	
}
#endif

#include "YSelectionBox.h"

class YGSelectionBox : public YSelectionBox, public YGTableView
{
public:
	YGSelectionBox (YWidget *parent, const string &label)
	: YSelectionBox (NULL, label),
	  YGTableView (this, parent, label, true, false)
	{
		connect (getWidget(), "row-activated", G_CALLBACK (activated_cb), (YGTableView *) this);
		connect (getSelection(), "changed", G_CALLBACK (selection_changed_cb), (YGTableView *) this);
#if YAST2_VERSION > 2018003
		connect (getWidget(), "right-click", G_CALLBACK (right_click_cb), this);
#endif
	}

	virtual bool isShrinkable() { return shrinkable(); }

	YGLABEL_WIDGET_IMPL (YSelectionBox)
	YGSELECTION_WIDGET_IMPL (YSelectionBox)
};

YSelectionBox *YGWidgetFactory::createSelectionBox (YWidget *parent, const string &label)
{ return new YGSelectionBox (parent, label); }

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
		gtk_tree_view_set_search_column (getView(), 2);
		setModel();

		connect (getSelection(), "changed", G_CALLBACK (selection_changed_cb), (YGTableView *) this);
		// Let the user toggle, using space/enter or double click (not an event).
		connect (getWidget(), "row-activated", G_CALLBACK (multi_activated_cb), this);
#if YAST2_VERSION > 2018003
		connect (getWidget(), "right-click", G_CALLBACK (right_click_cb), this);
#endif
	}

	// YMultiSelectionBox
	virtual void doAddItem (YItem *item)
	{
    	GtkTreeIter iter;
		addRow (&iter, item, false);
   		setCellToggle (&iter, 0, item->selected());
   		setCellIcon   (&iter, 1, item->iconName());
   		setCellLabel  (&iter, 2, item->label());
    }

	virtual YItem *currentItem()
	{ return doSelectedItem(); }

	virtual void setCurrentItem (YItem *item)
	{
		GtkTreeIter iter;
		if (getIter (item, &iter))
			YGTableView::doSelectItem (&iter);
	}

	// YGTableView
	virtual bool toggleMode() const
	{ return true; }

	// Events
	static void multi_activated_cb (GtkTreeView *tree_view, GtkTreePath *path,
	                                GtkTreeViewColumn *column, YGMultiSelectionBox* pThis)
	{ pThis->toggle (path, 0); }

	// YGSelectionModel
	virtual void doSelectItem (GtkTreeIter *iter)
	{
		setCellToggle (iter, 0, true);
	}

	virtual void doUnselectAll()
	{
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter_first (getModel(), &iter)) {
			do {
				setCellToggle (&iter, 0, false);
			} while (gtk_tree_model_iter_next (getModel(), &iter));
		}
	}

	// YGWidget
	virtual bool isShrinkable() { return shrinkable(); }

	YGLABEL_WIDGET_IMPL (YMultiSelectionBox)
	YGSELECTION_WIDGET_IMPL (YMultiSelectionBox)
};

YMultiSelectionBox *YGWidgetFactory::createMultiSelectionBox (YWidget *parent, const string &label)
{ return new YGMultiSelectionBox (parent, label); }

#include "YTree.h"
#include "YTreeItem.h"

class YGTree : public YTree, public YGTableView
{
public:
	YGTree (YWidget *parent, const string &label)
	: YTree (NULL, label)
	, YGTableView (this, parent, label, true, true)
	{
		connect (getWidget(), "row-collapsed", G_CALLBACK (row_collapsed_cb), this);
		connect (getWidget(), "row-expanded", G_CALLBACK (row_expanded_cb), this);
		connect (getWidget(), "cursor-changed", G_CALLBACK (row_selected_cb), this);
		connect (getWidget(), "row-activated", G_CALLBACK (activated_cb), (YGTableView *) this);
#if YAST2_VERSION > 2018003
		connect (getWidget(), "right-click", G_CALLBACK (right_click_cb), this);
#endif
	}

	// YTree
	virtual void rebuildTree()
	{
		doDeleteAllItems();
		for (YItemConstIterator it = YTree::itemsBegin(); it != YTree::itemsEnd(); it++)
			doAddItem (*it);
		int depth, rows;
		depth = getMaxDepth (&rows);
		gtk_tree_view_set_show_expanders (getView(), depth > 0);
		gtk_tree_view_set_enable_tree_lines (getView(), depth > 3 && rows > 100);
	}

	virtual const YTreeItem *getCurrentItem() const
	{
		YGTree *pThis = const_cast <YGTree *> (this);
		return (YTreeItem *) pThis->doSelectedItem();
	}

	virtual void setCurrentItem (YTreeItem *item)
	{ implSelectItem ((YItem *) item); }

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

	static void row_selected_cb (GtkTreeView *view, YGTree *pThis)
	{
		// expand selected row
		GtkTreeSelection *selection = pThis->getSelection();
		GtkTreeIter iter;
		if (gtk_tree_selection_get_selected (selection, NULL, &iter))
			pThis->expand (&iter);
		YGTable::selection_changed_cb (selection, pThis);
	}

	virtual unsigned int getMinSize (YUIDimension dim)
	{ return 80; }

	YGLABEL_WIDGET_IMPL (YTree)
	YGSELECTION_WIDGET_IMPL (YTree)
};

YTree *YGWidgetFactory::createTree (YWidget *parent, const string &label)
{
	return new YGTree (parent, label);
}

