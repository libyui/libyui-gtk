/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* YGtkPkgPatternList, pattern and language list implementations */
// check the header file for information about this widget

/*
  Textdomain "gtk"
 */

#include "YGi18n.h"
#include "config.h"
#include "YGUtils.h"
#include "ygtkpkgpatternview.h"
#include "ygtktreeview.h"
#include "ygtkcellrenderertext.h"
#include <gtk/gtk.h>

#define GRAY_COLOR "#727272"

enum Column {
	HAS_CHECK_COLUMN, CHECK_COLUMN, HAS_ICON_COLUMN, ICON_COLUMN,
	TEXT_COLUMN, ORDER_COLUMN, POINTER_COLUMN, TOTAL_COLUMNS
};

struct YGtkPkgPatternView::Impl : public Ypp::SelListener {
	GtkWidget *scroll, *view;

	Impl()
	{ Ypp::addSelListener (this); }
	~Impl()
	{
		Ypp::removeSelListener (this);
		if (!g_object_get_data (G_OBJECT (getModel()), "patterns"))
			gtk_tree_model_foreach (getModel(), free_foreach_cb, this);
	}

	GtkTreeModel *getModel()
	{ return gtk_tree_view_get_model (GTK_TREE_VIEW (view)); }

	static gboolean free_foreach_cb (GtkTreeModel *model,
		GtkTreePath *path, GtkTreeIter *iter, gpointer data)
	{
		gpointer _data;
		gtk_tree_model_get (model, iter, POINTER_COLUMN, &_data, -1);
		g_free (_data);
		return FALSE;
	}

	static gboolean update_foreach_cb (GtkTreeModel *model,
		GtkTreePath *path, GtkTreeIter *iter, gpointer data)
	{
		gpointer _data;
		gtk_tree_model_get (model, iter, POINTER_COLUMN, &_data, -1);
		if (_data) {
			ZyppSelectablePtr zsel = (ZyppSelectablePtr) _data;
			Ypp::Selectable sel (zsel);

			GtkTreeStore *store = GTK_TREE_STORE (model);
			gtk_tree_store_set (store, iter, CHECK_COLUMN,
				sel.isInstalled() || sel.toInstall(), -1);
		}
		return FALSE;
	}

	virtual void selectableModified()
	{ gtk_tree_model_foreach (getModel(), update_foreach_cb, this); }
};

static Ypp::Selectable get_iter_selectable (GtkTreeModel *model, GtkTreeIter *iter)
{
	if (g_object_get_data (G_OBJECT (model), "patterns")) {
		ZyppSelectablePtr zsel;
		gtk_tree_model_get (model, iter, POINTER_COLUMN, &zsel, -1);
		return Ypp::Selectable (zsel);
	}
	else {
		gchar *code;
		gtk_tree_model_get (model, iter, POINTER_COLUMN, &code, -1);
		zypp::Locale locale (code);
		g_free (code);
		return Ypp::Selectable (locale);
	}
}

static void set_row (
	GtkTreeStore *store, GtkTreeIter *iter, Ypp::Selectable &sel, ZyppPattern pattern)
{
	std::string order;
	GdkPixbuf *pixbuf = 0;
	gpointer ptr;
	if (pattern) {
		std::string filename (pattern->icon().asString());
		if (filename == zypp::Pathname("yast-system").asString() || filename.empty())
			filename = "pattern-generic";
		if (filename.compare (0, 2, "./", 2) == 0)
			filename.erase (0, 2);
		filename = zypp::str::form ("%s/icons/%dx%d/apps/%s.png",
			THEMEDIR, 32, 32, filename.c_str());
		pixbuf = YGUtils::loadPixbuf (filename);
/*
		if (pixbuf && !sel.isInstalled()) {
			GdkPixbuf *_pixbuf = pixbuf;
			pixbuf = YGUtils::setOpacity (_pixbuf, 50, true);
			g_object_unref (_pixbuf);
		}
*/
		order = pattern->order();
		ptr = get_pointer (sel.zyppSel());
	}
	else
		ptr = g_strdup (sel.zyppLocale().code().c_str());

	gtk_tree_store_set (store, iter, HAS_CHECK_COLUMN, TRUE,
		CHECK_COLUMN, sel.isInstalled() || sel.toInstall(), HAS_ICON_COLUMN, TRUE,
		ICON_COLUMN, pixbuf, TEXT_COLUMN, (sel.name() + '\n').c_str(),
		ORDER_COLUMN, order.c_str(),
		POINTER_COLUMN, ptr, -1);

	if (pixbuf)
		g_object_unref (G_OBJECT (pixbuf));
}

static gboolean set_rows_cb (GtkTreeModel *model,
	GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	gpointer _data;
	gtk_tree_model_get (model, iter, POINTER_COLUMN, &_data, -1);
	if (!_data) return FALSE;

	Ypp::Selectable sel (get_iter_selectable (model, iter));

	int installed, total;
	Ypp::Collection (sel).stats (&installed, &total);
	gchar *text = g_strdup_printf ("%s\n<span color=\"" GRAY_COLOR
		"\"><small>Installed %d of %d</small></span>",
		sel.name().c_str(), installed, total);

	GtkTreeStore *store = GTK_TREE_STORE (model);
	gtk_tree_store_set (store, iter, TEXT_COLUMN, text, -1);
	g_free (text);
	return FALSE;
}

static gboolean set_rows_idle (gpointer data)
{
	gtk_tree_model_foreach (GTK_TREE_MODEL (data), set_rows_cb, NULL);
	return FALSE;
}

static bool find_name (GtkTreeStore *store, GtkTreeIter *parent, GtkTreeIter *iter, const std::string &name)
{
	GtkTreeModel *model = GTK_TREE_MODEL (store);
	bool done = false;
	if (gtk_tree_model_iter_children (model, iter, parent))
		do {
			gchar *value;
			gtk_tree_model_get (model, iter, TEXT_COLUMN, &value, -1);
			if (name == value)
				done = true;
			g_free (value);
			if (done)
				return true;
		} while (gtk_tree_model_iter_next (model, iter));
	return false;
}

static bool find_place (GtkTreeStore *store, GtkTreeIter *parent, GtkTreeIter *iter, const std::string &order)
{
	GtkTreeModel *model = GTK_TREE_MODEL (store);
	bool done = false;
	if (gtk_tree_model_iter_children (model, iter, parent))
		do {
			gchar *value;
			gtk_tree_model_get (model, iter, ORDER_COLUMN, &value, -1);
			if (order < value)
				done = true;
			g_free (value);
			if (done) return true;
		} while (gtk_tree_model_iter_next (model, iter));
	return false;
}

static void insert_node (GtkTreeStore *store, GtkTreeIter *parent, GtkTreeIter *iter, const std::string &order)
{
	if (find_place (store, parent, iter, order)) {
		GtkTreeIter sibling = *iter;
		gtk_tree_store_insert_before (store, iter, parent, &sibling);
	}
	else
		gtk_tree_store_append (store, iter, parent);
}

static void insert_pattern (
	GtkTreeStore *store, GtkTreeIter *parent, Ypp::Selectable &sel, ZyppPattern pattern)
{
	GtkTreeIter iter;
	insert_node (store, parent, &iter, pattern->order());
	set_row (store, &iter, sel, pattern);
}

static void insert_category (GtkTreeStore *store, Ypp::Selectable &sel, ZyppPattern pattern)
{
	GtkTreeIter iter;
	std::string text ("<big><b>" + pattern->category() + "</b></big>");

	if (!find_name (store, NULL, &iter, text)) {
		insert_node (store, NULL, &iter, pattern->order());
		gtk_tree_store_set (store, &iter, HAS_CHECK_COLUMN, FALSE,
			CHECK_COLUMN, FALSE, HAS_ICON_COLUMN, FALSE, ICON_COLUMN, NULL,
			TEXT_COLUMN, text.c_str(),
			ORDER_COLUMN, pattern->order().c_str(), POINTER_COLUMN, NULL, -1);
	}
	insert_pattern (store, &iter, sel, pattern);
}

static void insert_language (GtkTreeStore *store, Ypp::Selectable &sel)
{
	std::string name (sel.name());
	bool done = false;
	GtkTreeModel *model = GTK_TREE_MODEL (store);
	GtkTreeIter iter;
	if (gtk_tree_model_iter_children (model, &iter, NULL))
		do {
			gchar *value;
			gtk_tree_model_get (model, &iter, TEXT_COLUMN, &value, -1);
			if (g_utf8_collate (name.c_str(), value) < 0)
				done = true;
			g_free (value);
			if (done) break;
		} while (gtk_tree_model_iter_next (model, &iter));

	if (done) {
		GtkTreeIter sibling = iter;
		gtk_tree_store_insert_before (store, &iter, NULL, &sibling);
	}
	else
		gtk_tree_store_append (store, &iter, NULL);

	set_row (store, &iter, sel, NULL);
}

static void install_cb (GtkWidget *widget, ZyppSelectablePtr zsel)
{ Ypp::Selectable (zsel).install(); }
static void undo_cb (GtkWidget *widget, ZyppSelectablePtr zsel)
{ Ypp::Selectable (zsel).undo(); }
static void remove_cb (GtkWidget *widget, ZyppSelectablePtr zsel)
{ Ypp::Selectable (zsel).remove(); }

static GtkWidget *menu_item_append (GtkWidget *menu, const char *_label, const char *stock, bool sensitive)
{
	std::string label;
	if (_label)
		label = YGUtils::mapKBAccel (_label);
	GtkWidget *item;
	if (_label) {
		item = gtk_image_menu_item_new_with_mnemonic (label.c_str());
		if (stock) {
			GtkWidget *icon = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_MENU);
			gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), icon);
		}
	}
	else
		item = gtk_image_menu_item_new_from_stock (stock, NULL);
	gtk_widget_set_sensitive (item, sensitive);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	return item;
}

static void right_click_cb (YGtkTreeView *yview, gboolean outreach, YGtkPkgPatternView *pThis)
{
	if (!outreach) {
		GtkTreeView *view = GTK_TREE_VIEW (yview);
		GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
		GtkTreeModel *model;
		GtkTreeIter iter;
		if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
			ZyppSelectablePtr zsel;
			gtk_tree_model_get (model, &iter, POINTER_COLUMN, &zsel, -1);
			Ypp::Selectable sel (zsel);

			GtkWidget *menu = gtk_menu_new(), *item;
			if (sel.toModify()) {
				item = menu_item_append (menu, NULL, GTK_STOCK_UNDO, true);
				g_signal_connect (G_OBJECT (item), "activate",
							      G_CALLBACK (undo_cb), zsel);
			}
			else {
				if (sel.isInstalled()) {
					if (sel.canRemove()) {
						item = menu_item_append (menu, _("&Remove"), GTK_STOCK_REMOVE, true);
						g_signal_connect (G_OBJECT (item), "activate",
										  G_CALLBACK (remove_cb), zsel);
					}
					else
						menu_item_append (menu, _("Remove: cannot remove patterns"), NULL, false);
				}
				else {
					item = menu_item_append (menu, _("&Install"), GTK_STOCK_ADD, true);
					g_signal_connect (G_OBJECT (item), "activate",
									  G_CALLBACK (install_cb), zsel);
				}
			}

			ygtk_tree_view_popup_menu (yview, menu);
		}
	}
}

static void toggle_iter (GtkTreeModel *model, GtkTreeIter *iter)
{
	Ypp::Selectable sel (get_iter_selectable (model, iter));
	sel.toModify() ? sel.undo() : sel.install();
}

static void row_activated_cb (GtkTreeView *view,
	GtkTreePath *path, GtkTreeViewColumn *column, YGtkPkgPatternView *pThis)
{
	GtkTreeModel *model = pThis->impl->getModel();
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter (model, &iter, path))
		toggle_iter (model, &iter);
}

static void toggled_cb (GtkCellRendererToggle *renderer, gchar *path, YGtkPkgPatternView *pThis)
{
	GtkTreeModel *model = pThis->impl->getModel();
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter_from_string (model, &iter, path))
		toggle_iter (model, &iter);
}

static gboolean can_tree_select_cb (GtkTreeSelection *selection,
	GtkTreeModel *model, GtkTreePath *path, gboolean path_selected, gpointer data)
{
	GtkTreeIter iter;
	gtk_tree_model_get_iter (model, &iter, path);
	gpointer _data;
	gtk_tree_model_get (model, &iter, POINTER_COLUMN, &_data, -1);
	return _data != NULL;
}

static void selection_changed_cb (GtkTreeSelection *selection, YGtkPkgPatternView *pThis)
{ pThis->notify(); }

static gboolean query_tooltip_cb (GtkWidget *widget, gint x, gint y,
	gboolean keyboard_mode, GtkTooltip *tooltip, YGtkPkgPatternView *pThis)
{
	GtkTreeView *view = GTK_TREE_VIEW (widget);
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	if (gtk_tree_view_get_tooltip_context (view,
	        &x, &y, keyboard_mode, &model, &path, &iter)) {
		gtk_tree_view_set_tooltip_row (view, tooltip, path);
		gtk_tree_path_free (path);

		GtkTreeViewColumn *column;
		int bx, by;
		gtk_tree_view_convert_widget_to_bin_window_coords (
			view, x, y, &bx, &by);
		gtk_tree_view_get_path_at_pos (
			view, x, y, NULL, &column, NULL, NULL);

		ZyppSelectablePtr zsel;
		gtk_tree_model_get (model, &iter, POINTER_COLUMN, &zsel, -1);
		if (zsel) {
			Ypp::Selectable sel (zsel);

			if (column == ygtk_tree_view_get_column (YGTK_TREE_VIEW (view), 0)) {  // check-marks
				if (sel.isInstalled())
					gtk_tooltip_set_text (tooltip,
						_("Installed: cannot remove a pattern.\n\n"
						"You must manually remove the individual packages you no "
						"longer want to keep."));
				else
					gtk_tooltip_set_text (tooltip, _("Not installed"));
			}
			else {
				std::string text (YGUtils::escapeMarkup (sel.description (false)));
				gtk_tooltip_set_text (tooltip, text.c_str());
			}
			return TRUE;
		}
	}
	return FALSE;
}

YGtkPkgPatternView::YGtkPkgPatternView (Ypp::Selectable::Type type)
: impl (new Impl())
{
	assert (type == Ypp::Selectable::LANGUAGE || type == Ypp::Selectable::PATTERN);

	GtkTreeStore *store = gtk_tree_store_new (TOTAL_COLUMNS, G_TYPE_BOOLEAN,
		G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, GDK_TYPE_PIXBUF, G_TYPE_STRING,
		G_TYPE_STRING, G_TYPE_POINTER);

	if (type == Ypp::Selectable::LANGUAGE) {
		Ypp::LangQuery query;
		Ypp::List list (query);
		list.sort (Ypp::List::NAME_SORT, true);
		for (int i = 0; i < list.size(); i++)
			insert_language (store, list.get(i));
	}
	else {
		// HACK: in opensuse 11.2, zypp crashes when re-iterating patterns
#if 1  // zypp::PoolQuery is crashing on me when using Patterns
		Ypp::PoolQuery query (type);
		while (query.hasNext()) {
			Ypp::Selectable sel = query.next();
			ZyppPattern pattern = castZyppPattern (sel.zyppSel()->theObj());
			if (pattern->userVisible())
				insert_category (store, sel, pattern);
		}
/*		static Ypp::List list (0);
		if (list.size() == 0) {
			Ypp::PoolQuery query (type);
			list = Ypp::List (query);
		}
		for (int i = 0; i < list.size(); i++) {
			Ypp::Selectable &sel = list.get (i);
			ZyppPattern pattern = castZyppPattern (sel.zyppSel()->theObj());
			if (pattern->userVisible())
				insert_category (store, sel, pattern);
		}*/
#else
		for (zypp::ResPoolProxy::const_iterator it =
			zypp::getZYpp()->poolProxy().byKindBegin <zypp::Pattern>();
			it != zypp::getZYpp()->poolProxy().byKindEnd <zypp::Pattern>();
			it++) {
			Ypp::Selectable sel (*it);
			ZyppPattern pattern = castZyppPattern (sel.zyppSel()->theObj());
			if (pattern->userVisible())
				insert_category (store, sel, pattern);
		}
#endif
		g_object_set_data (G_OBJECT (store), "patterns", GINT_TO_POINTER (1));
	}

	impl->view = ygtk_tree_view_new (NULL);
	GtkTreeView *view = GTK_TREE_VIEW (impl->view);
	gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));
	g_object_unref (G_OBJECT (store));
	gtk_tree_view_set_headers_visible (view, FALSE);
	gtk_tree_view_set_search_column (view, TEXT_COLUMN);
	gtk_tree_view_expand_all (view);
	gtk_tree_view_set_show_expanders (view, FALSE);
	gtk_widget_set_has_tooltip (impl->view, TRUE);
	g_signal_connect (G_OBJECT (view), "query-tooltip",
	                  G_CALLBACK (query_tooltip_cb), this);
	g_signal_connect (G_OBJECT (view), "right-click",
		              G_CALLBACK (right_click_cb), this);
	g_signal_connect (G_OBJECT (view), "row-activated",
	                  G_CALLBACK (row_activated_cb), this);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	renderer = gtk_cell_renderer_toggle_new();
	column = gtk_tree_view_column_new_with_attributes (
		NULL, renderer, "active", CHECK_COLUMN, "visible", HAS_CHECK_COLUMN, NULL);
	g_signal_connect (G_OBJECT (renderer), "toggled",
	                  G_CALLBACK (toggled_cb), this);
	ygtk_tree_view_append_column (YGTK_TREE_VIEW (view), column);

	bool reverse = gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title (column, NULL);
	gtk_tree_view_column_set_spacing (column, 4);

	GtkCellRenderer *pix_renderer = gtk_cell_renderer_pixbuf_new();
	if (!reverse)
		gtk_tree_view_column_pack_start (column, pix_renderer, FALSE);

	renderer = ygtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
		"markup", TEXT_COLUMN, NULL);
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);

	if (reverse)
		gtk_tree_view_column_pack_start (column, pix_renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, pix_renderer,
		"pixbuf", ICON_COLUMN, "visible", HAS_ICON_COLUMN, NULL);

	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_expand (column, TRUE);
	ygtk_tree_view_append_column (YGTK_TREE_VIEW (view), column);

	GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
	gtk_tree_selection_set_select_function (selection, can_tree_select_cb, NULL, NULL);
	g_signal_connect (G_OBJECT (selection), "changed",
	                  G_CALLBACK (selection_changed_cb), this);
	clearSelection();

	impl->scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (impl->scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (
		GTK_SCROLLED_WINDOW (impl->scroll), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (impl->scroll), impl->view);
	gtk_widget_show_all (impl->scroll);

	g_idle_add_full (G_PRIORITY_LOW, set_rows_idle, store, NULL);
}

YGtkPkgPatternView::~YGtkPkgPatternView()
{ delete impl; }

GtkWidget *YGtkPkgPatternView::getWidget()
{ return impl->scroll; }

void YGtkPkgPatternView::clearSelection()
{
	GtkTreeView *view = GTK_TREE_VIEW (impl->view);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
	g_signal_handlers_block_by_func (selection, (gpointer) selection_changed_cb, this);
	gtk_tree_selection_unselect_all (selection);
	g_signal_handlers_unblock_by_func (selection, (gpointer) selection_changed_cb, this);
}

bool YGtkPkgPatternView::writeQuery (Ypp::PoolQuery &query)
{
	GtkTreeView *view = GTK_TREE_VIEW (impl->view);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		Ypp::Selectable sel (get_iter_selectable (model, &iter));
		Ypp::Collection collection (sel);
		query.addCriteria (new Ypp::FromCollectionMatch (collection));
		return true;
	}
	return false;
}

bool isPatternsPoolEmpty()
{ return zyppPool().empty <zypp::Pattern>(); }

