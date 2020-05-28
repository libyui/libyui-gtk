/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/*
  Textdomain "gtk"
 */

#include "YGi18n.h"
#define YUILogComponent "gtk"
#include <yui/Libyui_config.h>
#include "YGUI.h"
#include "YGUtils.h"
#include "YGWidget.h"
#include "YSelectionWidget.h"
#include "YGSelectionStore.h"
#include "ygtktreeview.h"
#include "YGMacros.h"
#include <string.h>

/* A generic widget for table related widgets. */

class YGTreeView : public YGScrolledWidget, public YGSelectionStore
{
protected:
	guint m_blockTimeout;
	int markColumn;
	GtkWidget *m_count;

public:
	YGTreeView (YWidget *ywidget, YWidget *parent, const std::string &label, bool tree)
	: YGScrolledWidget (ywidget, parent, label, YD_VERT, YGTK_TYPE_TREE_VIEW, NULL),
	  YGSelectionStore (tree)
	{
		gtk_tree_view_set_headers_visible (getView(), FALSE);

		/* Yast tools expect the user to be unable to un-select the row. They
		   generally don't check to see if the returned value is -1. So, just
		   disallow un-selection. */
		gtk_tree_selection_set_mode (getSelection(), GTK_SELECTION_BROWSE);

		connect (getSelection(), "changed", G_CALLBACK (selection_changed_cb), this);
		connect (getWidget(), "row-activated", G_CALLBACK (activated_cb), this);
		connect (getWidget(), "right-click", G_CALLBACK (right_click_cb), this);

		m_blockTimeout = 0;  // GtkTreeSelection idiotically fires when showing widget
		markColumn = -1; m_count = NULL;
		blockSelected();
		g_signal_connect (getWidget(), "map", G_CALLBACK (block_init_cb), this);
	}

	virtual ~YGTreeView()
	{ if (m_blockTimeout) g_source_remove (m_blockTimeout); }

	inline GtkTreeView *getView()
	{ return GTK_TREE_VIEW (getWidget()); }
	inline GtkTreeSelection *getSelection()
	{ return gtk_tree_view_get_selection (getView()); }

	void addTextColumn (int iconCol, int textCol)
	{ addTextColumn ("", YAlignUnchanged, iconCol, textCol); }

	void addTextColumn (const std::string &header, YAlignmentType align, int icon_col, int text_col)
	{
		gfloat xalign = -1;
		switch (align) {
			case YAlignBegin:  xalign = 0.0; break;
			case YAlignCenter: xalign = 0.5; break;
			case YAlignEnd:    xalign = 1.0; break;
			case YAlignUnchanged: break;
		}

		GtkTreeViewColumn *column = gtk_tree_view_column_new();
		gtk_tree_view_column_set_title (column, header.c_str());

		GtkCellRenderer *renderer;
		renderer = gtk_cell_renderer_pixbuf_new();
		gtk_tree_view_column_pack_start (column, renderer, FALSE);
		gtk_tree_view_column_set_attributes (column, renderer, "pixbuf", icon_col, NULL);

		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start (column, renderer, TRUE);
		gtk_tree_view_column_set_attributes (column, renderer, "text", text_col, NULL);
		if (xalign != -1)
			g_object_set (renderer, "xalign", xalign, NULL);

		gtk_tree_view_column_set_resizable (column, TRUE);
		gtk_tree_view_append_column (getView(), column);
		if (gtk_tree_view_get_search_column (getView()) == -1)
			gtk_tree_view_set_search_column (getView(), text_col);
	}

	void addCheckColumn (int check_col)
	{
		GtkCellRenderer *renderer = gtk_cell_renderer_toggle_new();
		g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (check_col));
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (
			NULL, renderer, "active", check_col, NULL);
		gtk_tree_view_column_set_cell_data_func (column, renderer, inconsistent_mark_cb, this, NULL);
		g_signal_connect (G_OBJECT (renderer), "toggled",
		                  G_CALLBACK (toggled_cb), this);

		gtk_tree_view_column_set_resizable (column, TRUE);
		gtk_tree_view_append_column (getView(), column);
		if (markColumn == -1)
			markColumn = check_col;
	}

	void readModel()
	{ gtk_tree_view_set_model (getView(), getModel()); }

	void addCountWidget (YWidget *yparent)
	{
		bool mainWidget = !yparent || !strcmp (yparent->widgetClass(), "YVBox") || !strcmp (yparent->widgetClass(), "YReplacePoint");
		if (mainWidget) {
			m_count = gtk_label_new ("0");
			GtkWidget *hbox = YGTK_HBOX_NEW(4);
			gtk_box_set_homogeneous (GTK_BOX (hbox), FALSE);

			GtkWidget *label = gtk_label_new (_("Total selected:"));
			//gtk_box_pack_start (GTK_BOX (hbox), gtk_event_box_new(), TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (hbox), m_count, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (YGWidget::getWidget()), hbox, FALSE, TRUE, 0);
			gtk_widget_show_all (hbox);
		}
	}

	void syncCount()
	{
		if (!m_count) return;

		struct inner {
			static gboolean foreach (
				GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer _pThis)
			{
				YGTreeView *pThis = (YGTreeView *) _pThis;
				gboolean mark;
				gtk_tree_model_get (model, iter, pThis->markColumn, &mark, -1);
				if (mark) {
					int count = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (model), "count"));
					g_object_set_data (G_OBJECT (model), "count", GINT_TO_POINTER (count+1));
				}
				return FALSE;
			}
		};

		GtkTreeModel *model = getModel();
		g_object_set_data (G_OBJECT (model), "count", 0);
		gtk_tree_model_foreach (model, inner::foreach, this);

		int count = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (model), "count"));
		gchar *str = g_strdup_printf ("%d", count);
		gtk_label_set_text (GTK_LABEL (m_count), str);
		g_free (str);
	}

	void focusItem (YItem *item, bool select)
	{
		GtkTreeIter iter;
		getTreeIter (item, &iter);
		blockSelected();

		if (select) {
			GtkTreePath *path = gtk_tree_model_get_path (getModel(), &iter);
			gtk_tree_view_expand_to_path (getView(), path);

			if (gtk_tree_selection_get_mode (getSelection()) != GTK_SELECTION_MULTIPLE)
				gtk_tree_view_scroll_to_cell (getView(), path, NULL, TRUE, 0.5, 0);
			gtk_tree_path_free (path);

			gtk_tree_selection_select_iter (getSelection(), &iter);
		}
		else
			gtk_tree_selection_unselect_iter (getSelection(), &iter);
	}

	void unfocusAllItems()
	{
		blockSelected();
		gtk_tree_selection_unselect_all (getSelection());
	}

	void unmarkAll()
	{
		struct inner {
			static gboolean foreach_unmark (
				GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer _pThis)
			{
				YGTreeView *pThis = (YGTreeView *) _pThis;
				pThis->setRowMark (iter, pThis->markColumn, FALSE);
				return FALSE;
			}
		};

		gtk_tree_model_foreach (getModel(), inner::foreach_unmark, this);
	}

	YItem *getFocusItem()
	{
		GtkTreeIter iter;
		if (gtk_tree_selection_get_selected (getSelection(), NULL, &iter))
			return getYItem (&iter);
		return NULL;
	}

	virtual bool _immediateMode() { return true; }
	virtual bool _shrinkable() { return false; }
	virtual bool _recursiveSelection() { return false; }

	void setMark (GtkTreeIter *iter, YItem *yitem, gint column, bool state, bool recursive)
	{
		setRowMark (iter, column, state);
		yitem->setSelected (state);

		if (recursive)
			for (YItemConstIterator it = yitem->childrenBegin();
				 it != yitem->childrenEnd(); it++) {
				GtkTreeIter _iter;
				getTreeIter (*it, &_iter);
				setMark (&_iter, *it, column, state, true);
			}
	}

	void toggleMark (GtkTreePath *path, gint column)
	{
		GtkTreeIter iter;
		if (!gtk_tree_model_get_iter (getModel(), &iter, path))
			return;
		gboolean state;
		gtk_tree_model_get (getModel(), &iter, column, &state, -1);
		state = !state;

		YItem *yitem = getYItem (&iter);
		setMark (&iter, yitem, column, state, _recursiveSelection());
		syncCount();
		emitEvent (YEvent::ValueChanged);
	}

	// YGWidget

	virtual unsigned int getMinSize (YUIDimension dim)
	{
		if (dim == YD_VERT)
			return YGUtils::getCharsHeight (getWidget(), _shrinkable() ? 2 : 5);
		return 80;
	}

protected:
	static gboolean block_selected_timeout_cb (gpointer data)
	{
		YGTreeView *pThis = (YGTreeView *) data;
		pThis->m_blockTimeout = 0;
		return FALSE;
	}

	void blockSelected()
	{  // GtkTreeSelection only fires when idle; so set a timeout
		if (m_blockTimeout) g_source_remove (m_blockTimeout);
		m_blockTimeout = g_timeout_add_full (G_PRIORITY_LOW, 50, block_selected_timeout_cb, this, NULL);
	}

	static void block_init_cb (GtkWidget *widget, YGTreeView *pThis)
	{ pThis->blockSelected(); }

	// callbacks

	static bool all_marked (GtkTreeModel *model, GtkTreeIter *iter, int mark_col)
	{
		gboolean marked;
		GtkTreeIter child_iter;
		if (gtk_tree_model_iter_children (model, &child_iter, iter))
			do {
				gtk_tree_model_get (model, &child_iter, mark_col, &marked, -1);
				if (!marked) return false;
				all_marked (model, &child_iter, mark_col);
			} while (gtk_tree_model_iter_next (model, &child_iter));
		return true;
	}

	static void inconsistent_mark_cb (GtkTreeViewColumn *column,
		GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
	{  // used for trees -- show inconsistent if one node is check but another isn't
		YGTreeView *pThis = (YGTreeView *) data;
		gboolean marked;
		gtk_tree_model_get (model, iter, pThis->markColumn, &marked, -1);
		gboolean consistent = !marked || all_marked (model, iter, pThis->markColumn);
		g_object_set (G_OBJECT (cell), "inconsistent", !consistent, NULL);
	}

	static void selection_changed_cb (GtkTreeSelection *selection, YGTreeView *pThis)
	{
		struct inner {
			static gboolean foreach_sync_select (
				GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer _pThis)
			{
				YGTreeView *pThis = (YGTreeView *) _pThis;
				GtkTreeSelection *selection = pThis->getSelection();
				bool sel = gtk_tree_selection_iter_is_selected (selection, iter);
				pThis->getYItem (iter)->setSelected (sel);
				return FALSE;
			}
		};

		if (pThis->m_blockTimeout) return;
		if (pThis->markColumn == -1)
			gtk_tree_model_foreach (pThis->getModel(), inner::foreach_sync_select, pThis);
		if (pThis->_immediateMode())
			pThis->emitEvent (YEvent::SelectionChanged, IF_NOT_PENDING_EVENT);
	}

	static void activated_cb (GtkTreeView *tree_view, GtkTreePath *path,
	                          GtkTreeViewColumn *column, YGTreeView* pThis)
	{
		if (pThis->markColumn >= 0)
			pThis->toggleMark (path, pThis->markColumn);
		else {
			// for tree - expand/collpase double-clicked rows
			if (gtk_tree_view_row_expanded (tree_view, path))
				gtk_tree_view_collapse_row (tree_view, path);
			else
				gtk_tree_view_expand_row (tree_view, path, FALSE);

			pThis->emitEvent (YEvent::Activated);
		}
	}

	static void toggled_cb (GtkCellRendererToggle *renderer, gchar *path_str,
	                        YGTreeView *pThis)
	{
		GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
		gint column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (renderer), "column"));
		pThis->toggleMark (path, column);
		gtk_tree_path_free (path);

		// un/marking a sub-node can cause changes of "inconsistency"
		if (gtk_tree_path_get_depth (path) >= 2)
			gtk_widget_queue_draw (pThis->getWidget());
	}


	static void right_click_cb (YGtkTreeView *view, gboolean outreach, YGTreeView *pThis)
	{ pThis->emitEvent (YEvent::ContextMenuActivated); }
};

#include "YTable.h"
#include "YGDialog.h"
#include <gdk/gdkkeysyms.h>
#include <string.h>

class YGTable : public YTable, public YGTreeView
{
public:
	YGTable (YWidget *parent, YTableHeader *headers, bool multiSelection)
	: YTable (NULL, headers, multiSelection),
	  YGTreeView (this, parent, std::string(), false)
	{
		gtk_tree_view_set_headers_visible (getView(), TRUE);

#		if GTK_CHECK_VERSION (3, 14, 0)
#		else
		gtk_tree_view_set_rules_hint (getView(), columns() > 1);
#		endif

		ygtk_tree_view_set_empty_text (YGTK_TREE_VIEW (getView()), _("No entries."));
		if (multiSelection)
			gtk_tree_selection_set_mode (getSelection(), GTK_SELECTION_MULTIPLE);

		GType types [columns()*2];
		for (int i = 0; i < columns(); i++) {
			int t = i*2;
			types[t+0] = GDK_TYPE_PIXBUF;
			types[t+1] = G_TYPE_STRING;
			addTextColumn (header(i), alignment (i), t, t+1);
		}
		createStore (columns()*2, types);
		readModel();
		if (!keepSorting())
			setSortable (true);

		// if last col is aligned: add some dummy so that it doesn't expand.
		YAlignmentType lastAlign = alignment (columns()-1);
		if (lastAlign == YAlignCenter || lastAlign == YAlignEnd)
			gtk_tree_view_append_column (getView(), gtk_tree_view_column_new());

		g_signal_connect (getWidget(), "key-press-event", G_CALLBACK (key_press_event_cb), this);
	}

	void setSortable (bool sortable)
	{
		int n = 0;
		GList *columns = gtk_tree_view_get_columns (getView());
		for (GList *i = columns; i; i = i->next, n++) {
			GtkTreeViewColumn *column = (GtkTreeViewColumn *) i->data;
			if (n >= YGTable::columns())
				break;
			if (sortable) {
				int index = (n*2)+1;
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

	void setCell (GtkTreeIter *iter, int column, const YTableCell *cell)
	{
		if (!cell) return;
		std::string label (cell->label());
		if (label == "X")
			label = YUI::app()->glyph (YUIGlyph_CheckMark);

		int index = column * 2;
		setRowText (iter, index, cell->iconName(), index+1, label, this);
	}

	// YGTreeView

	virtual bool _immediateMode() { return immediateMode(); }

	// YTable

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
		getTreeIter (cell->parent(), &iter);
		setCell (&iter, cell->column(), cell);
	}

	// YGSelectionStore

	void doAddItem (YItem *_item)
	{
    	YTableItem *item = dynamic_cast <YTableItem *> (_item);
    	if (item) {
			GtkTreeIter iter;
			addRow (item, &iter);
			int i = 0;
			for (YTableCellIterator it = item->cellsBegin();
			     it != item->cellsEnd(); it++)
				if (i >= columns())
				{
					yuiWarning() << "Item contains too many columns, current column is (starting from 0) " << i 
						<< " but only " << columns() << " columns are configured. Skipping..." << std::endl;
				}
				else
	   				setCell (&iter, i++, *it);
			if (item->selected())
				focusItem (item, true);
    	}
    	else
			yuiError() << "Can only add YTableItems to a YTable.\n";
    }

	void doSelectItem (YItem *item, bool select)
	{ focusItem (item, select); }

	void doDeselectAllItems()
	{ unfocusAllItems(); }

	// callbacks

	static void activateButton (YWidget *button)
	{
		YWidgetEvent *event = new YWidgetEvent (button, YEvent::Activated);
		YGUI::ui()->sendEvent (event);
	}

	static void hack_right_click_cb (YGtkTreeView *view, gboolean outreach, YGTable *pThis)
	{
		if (pThis->notifyContextMenu())
			return YGTreeView::right_click_cb (view, outreach, pThis);

		// If no context menu is specified, hack one ;-)

		struct inner {
			static void key_activate_cb (GtkMenuItem *item, YWidget *button)
			{ activateButton (button); }
			static void appendItem (GtkWidget *menu, const gchar *stock, int key)
			{
				YWidget *button = YGDialog::currentDialog()->getFunctionWidget (key);
				if (button) {
					GtkWidget *item;
					item = gtk_menu_item_new_with_mnemonic (stock);
					gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
					g_signal_connect (G_OBJECT (item), "activate",
									  G_CALLBACK (key_activate_cb), button);
				}
			}
		};

		GtkWidget *menu = gtk_menu_new();
		YGDialog *dialog = YGDialog::currentDialog();
		if (dialog->getClassWidgets ("YTable").size() == 1) {
			// if more than one table exists, function keys would be ambiguous
			if (outreach) {
				if (dialog->getFunctionWidget(3))
					inner::appendItem (menu, "list-add", 3);
			}
			else {
				if (dialog->getFunctionWidget(4))
					inner::appendItem (menu, "edit-cut", 4);
				if (dialog->getFunctionWidget(5))
					inner::appendItem (menu, "list-remove", 5);
			}
		}

		menu = ygtk_tree_view_append_show_columns_item (YGTK_TREE_VIEW (view), menu);
		ygtk_tree_view_popup_menu (view, menu);
	}

	static gboolean key_press_event_cb (GtkWidget *widget, GdkEventKey *event, YGTable *pThis)
	{
		if (event->keyval == GDK_KEY_Delete) {
			YWidget *button = YGDialog::currentDialog()->getFunctionWidget (5);
			if (button)
				activateButton (button);
			else
				gtk_widget_error_bell (widget);
			return TRUE;
		}
		return FALSE;
	}

	static gint tree_sort_cb (
		GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer _index)
	{
		int index = GPOINTER_TO_INT (_index);
		gchar *str_a, *str_b;
		gtk_tree_model_get (model, a, index, &str_a, -1);
		gtk_tree_model_get (model, b, index, &str_b, -1);
		if (!str_a) str_a = g_strdup ("");
		if (!str_b) str_b = g_strdup ("");
		int ret = strcmp (str_a, str_b);
		g_free (str_a); g_free (str_b);
		return ret;
	}

	YGLABEL_WIDGET_IMPL (YTable)
	YGSELECTION_WIDGET_IMPL (YTable)
};

YTable *YGWidgetFactory::createTable (YWidget *parent, YTableHeader *headers,
                                      bool multiSelection)
{
	return new YGTable (parent, headers, multiSelection);
}

#include "YSelectionBox.h"

class YGSelectionBox : public YSelectionBox, public YGTreeView
{
public:
	YGSelectionBox (YWidget *parent, const std::string &label)
	: YSelectionBox (NULL, label),
	  YGTreeView (this, parent, label, false)
	{
		GType types [2] = { GDK_TYPE_PIXBUF, G_TYPE_STRING };
		addTextColumn (0, 1);
		createStore (2, types);
		readModel();
	}

	// YGTreeView

	virtual bool _shrinkable() { return shrinkable(); }

	// YGSelectionStore

	void doAddItem (YItem *item)
	{
		GtkTreeIter iter;
		addRow (item, &iter);
		setRowText (&iter, 0, item->iconName(), 1, item->label(), this);
		if (item->selected())
			focusItem (item, true);
    }

	void doSelectItem (YItem *item, bool select)
	{ focusItem (item, select); }

	void doDeselectAllItems()
	{ unfocusAllItems(); }

	YGLABEL_WIDGET_IMPL (YSelectionBox)
	YGSELECTION_WIDGET_IMPL (YSelectionBox)
};

YSelectionBox *YGWidgetFactory::createSelectionBox (YWidget *parent, const std::string &label)
{ return new YGSelectionBox (parent, label); }

#include "YMultiSelectionBox.h"

class YGMultiSelectionBox : public YMultiSelectionBox, public YGTreeView
{
public:
	YGMultiSelectionBox (YWidget *parent, const std::string &label)
	: YMultiSelectionBox (NULL, label),
	  YGTreeView (this, parent, label, false)
	{
		GType types [3] = { G_TYPE_BOOLEAN, GDK_TYPE_PIXBUF, G_TYPE_STRING };
		addCheckColumn (0);
		addTextColumn (1, 2);
		createStore (3, types);
		readModel();
		addCountWidget (parent);
	}

	// YGTreeView

	virtual bool _shrinkable() { return shrinkable(); }

	// YGSelectionStore

	void doAddItem (YItem *item)
	{
		GtkTreeIter iter;
		addRow (item, &iter);
		setRowMark (&iter, 0, item->selected());
		setRowText (&iter, 1, item->iconName(), 2, item->label(), this);
		syncCount();
    }

	void doSelectItem (YItem *item, bool select)
	{
		GtkTreeIter iter;
		getTreeIter (item, &iter);
		setRowMark (&iter, 0, select);
		syncCount();
	}

	void doDeselectAllItems()
	{ unmarkAll(); syncCount(); }

	// YMultiSelectionBox

	virtual YItem *currentItem()
	{ return getFocusItem(); }

	virtual void setCurrentItem (YItem *item)
	{ focusItem (item, true); }

	YGLABEL_WIDGET_IMPL (YMultiSelectionBox)
	YGSELECTION_WIDGET_IMPL (YMultiSelectionBox)
};

YMultiSelectionBox *YGWidgetFactory::createMultiSelectionBox (YWidget *parent, const std::string &label)
{ return new YGMultiSelectionBox (parent, label); }

#include "YTree.h"
#include "YTreeItem.h"

class YGTree : public YTree, public YGTreeView
{
public:
	YGTree (YWidget *parent, const std::string &label, bool multiselection, bool recursiveSelection)
	: YTree (NULL, label, multiselection, recursiveSelection),
	  YGTreeView (this, parent, label, true)
	{
		if (multiselection) {
			GType types [3] = { GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_BOOLEAN };
			addCheckColumn (2);
			addTextColumn (0, 1);
			createStore (3, types);
			addCountWidget (parent);
		}
		else
		{
			GType types [2] = { GDK_TYPE_PIXBUF, G_TYPE_STRING };
			addTextColumn (0, 1);
			createStore (2, types);
		}
		readModel();

		g_signal_connect (getWidget(), "row-collapsed", G_CALLBACK (row_collapsed_cb), this);
		g_signal_connect (getWidget(), "row-expanded", G_CALLBACK (row_expanded_cb), this);
	}

	virtual bool _recursiveSelection() { return recursiveSelection(); }

	void addNode (YItem *item, GtkTreeIter *parent)
	{
		GtkTreeIter iter;
		addRow (item, &iter, parent);
		setRowText (&iter, 0, item->iconName(), 1, item->label(), this);
		if (item->selected()) {
			if (hasMultiSelection())
				setRowMark (&iter, 2, item->selected());
			else
				focusItem (item, true);
		}
		if (((YTreeItem *) item)->isOpen())
			expand (&iter);
		for (YItemConstIterator it = item->childrenBegin();
		     it != item->childrenEnd(); it++)
			addNode (*it, &iter);
	}


	void expand (GtkTreeIter *iter)
	{
		GtkTreePath *path = gtk_tree_model_get_path (getModel(), iter);
		gtk_tree_view_expand_row (getView(), path, FALSE);
		gtk_tree_path_free (path);
	}
#if 0
	bool isReallyOpen (YTreeItem *item)  // are parents open as well?
	{
		for (YTreeItem *i = item; i; i = i->parent())
			if (!i->isOpen())
				return false;
		return true;
	}
#endif

	// YTree

	virtual void rebuildTree()
	{
		blockSelected();

		doDeleteAllItems();
		for (YItemConstIterator it = YTree::itemsBegin(); it != YTree::itemsEnd(); it++)
			addNode (*it, NULL);

		int depth = getTreeDepth();
		gtk_tree_view_set_show_expanders (getView(), depth > 1);
		gtk_tree_view_set_enable_tree_lines (getView(), depth > 3);

		// for whatever reason, we need to expand nodes only after the model
		// is fully initialized
		struct inner {
			static gboolean foreach_sync_open (
				GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer _pThis)
			{
				YGTree *pThis = (YGTree *) _pThis;
				YTreeItem *item = (YTreeItem *) pThis->getYItem (iter);
				if (item->isOpen())
					gtk_tree_view_expand_row (pThis->getView(), path, FALSE);
				return FALSE;
			}
		};

		g_signal_handlers_block_by_func (getWidget(), (gpointer) row_expanded_cb, this);
		gtk_tree_model_foreach (getModel(), inner::foreach_sync_open, this);
		g_signal_handlers_unblock_by_func (getWidget(), (gpointer) row_expanded_cb, this);

		syncCount();
	}

	virtual YTreeItem *currentItem()
	{ return (YTreeItem *) getFocusItem(); }

	void _markItem (YItem *item, bool select, bool recursive) {
		GtkTreeIter iter;
		getTreeIter (item, &iter);
		setRowMark (&iter, 2, select);

		if (recursive) {
			YTreeItem *_item = (YTreeItem *) item;
			for (YItemConstIterator it = _item->childrenBegin();
				 it != _item->childrenEnd(); it++)
				_markItem (*it, select, true);
		}
	}

	// YGSelectionStore

	void doAddItem (YItem *item) {}  // rebuild will be called anyway

	void doSelectItem (YItem *item, bool select)
	{
		if (hasMultiSelection()) {
			_markItem (item, select, recursiveSelection());
			syncCount();
		}
		else
			focusItem (item, select);
	}

	void doDeselectAllItems()
	{
		if (hasMultiSelection()) {
			unmarkAll();
			syncCount();
		}
		else
			unfocusAllItems();
	}

	// callbacks

	void reportRowOpen (GtkTreeIter *iter, bool open)
	{
		YTreeItem *item = static_cast <YTreeItem *> (getYItem (iter));
		item->setOpen (open);
	}

	static void row_collapsed_cb (GtkTreeView *view, GtkTreeIter *iter,
	                              GtkTreePath *path, YGTree *pThis)
	{ pThis->reportRowOpen (iter, false); }

	static void row_expanded_cb (GtkTreeView *view, GtkTreeIter *iter,
	                             GtkTreePath *path, YGTree *pThis)
	{ pThis->reportRowOpen (iter, true); }

#if 0
		// we do a bit of a work-around here to mimic -qt behavior... A node can
		// be initialized as open, yet its parent, or some grand-parent, be closed.
		// We thus honor the open state when its parent gets open.
		YTreeItem *item = static_cast <YTreeItem *> (pThis->getYItem (iter));
		for (YItemConstIterator it = item->childrenBegin();
		     it != item->childrenEnd(); it++) {
			const YTreeItem *child = static_cast <YTreeItem *> (*it);
			if (child->isOpen()) {
				GtkTreeIter iter;
				if (pThis->getIter (child, &iter))
					pThis->expand (&iter);
			}
		}
#endif

	// NOTE copy of Qt one
	void activate()
        {
            // send an activation event for this widget
            if ( notify() )
                YGUI::ui()->sendEvent( new YWidgetEvent( this,YEvent::Activated ) );
        }


	YGLABEL_WIDGET_IMPL (YTree)
	YGSELECTION_WIDGET_IMPL (YTree)
};

YTree *YGWidgetFactory::createTree (YWidget *parent, const std::string &label, bool multiselection, bool recursiveSelection)
{ return new YGTree (parent, label, multiselection, recursiveSelection); }

