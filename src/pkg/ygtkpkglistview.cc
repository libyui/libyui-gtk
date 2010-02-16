/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* Textdomain "yast2-gtk" */
/* YGtkPkgListView, Zypp GtkTreeView implementation */
// check the header file for information about this widget

#define YUILogComponent "gtk"
#include "config.h"
#include "YGi18n.h"
#include "YGUtils.h"
#include "ygtkpkglistview.h"
#include "ygtktreeview.h"
#include "ygtktreemodel.h"
#include "ygtkcellrenderertextpixbuf.h"
#include "ygtkcellrendererbutton.h"
#include "ygtkcellrenderersidebutton.h"
#include <gtk/gtk.h>
#include <string.h>

//** Model

enum ImplProperty {
	// booleans
	HAS_UPGRADE_PROP = TOTAL_PROPS, TO_UPGRADE_PROP, TO_MODIFY_PROP, IS_LOCKED_PROP,
	// integer
	XPAD_PROP,
	// string
	VERSION_FOREGROUND_PROP, BACKGROUND_PROP, REPOSITORY_STOCK_PROP,
	// pointer
	PTR_PROP,
	TOTAL_IMPL_PROPS
};

static GType _columnType (int col)
{
	switch (col) {
		case NAME_PROP: case NAME_SUMMARY_PROP: case VERSION_PROP:
		case REPOSITORY_PROP: case SUPPORT_PROP: case SIZE_PROP:
		case VERSION_FOREGROUND_PROP: case BACKGROUND_PROP: case REPOSITORY_STOCK_PROP:
		case STATUS_ICON_PROP:
			return G_TYPE_STRING;
		case INSTALLED_CHECK_PROP:
		case HAS_UPGRADE_PROP: case TO_UPGRADE_PROP: case TO_MODIFY_PROP:
		case IS_LOCKED_PROP:
			return G_TYPE_BOOLEAN;
		case XPAD_PROP:
			return G_TYPE_INT;
		case PTR_PROP:
			return G_TYPE_POINTER;
	}
	return 0;
}

struct YGtkZyppModel : public YGtkTreeModel, Ypp::SelListener
{
	YGtkZyppModel (Ypp::List list, const std::list <std::string> &keywords)
	: m_list (list), m_keywords (keywords)
	{ addSelListener (this); }

	~YGtkZyppModel()
	{ removeSelListener (this); }

	void setHighlight (std::list <std::string> keywords)
	{ m_keywords = keywords; }

protected:
	Ypp::List m_list;
	std::list <std::string> m_keywords;

	virtual int rowsNb() { return m_list.size(); }
	virtual int columnsNb() const { return TOTAL_IMPL_PROPS; }
	virtual bool showEmptyEntry() const { return false; }

	virtual GType columnType (int col) const
	{ return _columnType (col); }

	virtual void getValue (int row, int col, GValue *value)
	{
		Ypp::Selectable &sel = m_list.get (row);
		switch (col) {
			case NAME_PROP: {
				std::string str (sel.name());
				highlightMarkupSpan (str, m_keywords);
				g_value_set_string (value, str.c_str());
				break;
			}
			case NAME_SUMMARY_PROP: {
				std::string str, name (sel.name()), summary (sel.summary());
				str.reserve (name.size() + summary.size() + 64);
				str = name;
				if (!summary.empty()) {
					summary = YGUtils::escapeMarkup (summary);
					str += "\n<small>" + summary + "</small>";
				}
				highlightMarkupSpan (str, m_keywords);
				g_value_set_string (value, str.c_str());
				break;
			}
			case REPOSITORY_PROP: {
				std::string str;
				if (sel.availableSize() > 0) {
					Ypp::Repository repo (sel.candidate().repository());
					str = getRepositoryLabel (repo);
				}
#if 0
				else {
					Ypp::Repository repo (sel.installed().repository());
					str = getRepositoryLabel (repo);
				}
#endif
				g_value_set_string (value, str.c_str());
				break;
			}
			case REPOSITORY_STOCK_PROP: {
				const char *stock = 0;
				if (sel.availableSize() > 0) {
					Ypp::Repository repo (sel.candidate().repository());
					stock = getRepositoryStockIcon (repo);
				}
#if 0
				else {
					Ypp::Repository repo (sel.installed().repository());
					stock = getRepositoryStockIcon (repo);
				}
#endif
				g_value_set_string (value, stock);
				break;
			}
			case SUPPORT_PROP: {
				Ypp::Package pkg (sel);
				std::string str (Ypp::Package::supportSummary (pkg.support()));
				g_value_set_string (value, str.c_str());
				break;
			}
			case SIZE_PROP: {
				Size_t size = sel.anyVersion().size();
				g_value_set_string (value, size.asString().c_str());
				break;
			}
			case VERSION_PROP: {
				std::string str;
				str.reserve (128);
				int cmp = 0;
				bool hasAvailable = sel.availableSize() > 0;
				if (hasAvailable) {
					Ypp::Version candidate = sel.candidate();
					if (sel.isInstalled()) {
						Ypp::Version installed = sel.installed();
						if (candidate > installed)
							cmp = 1;
						if (candidate < installed)
							cmp = -1;
					}
				}
				if (cmp > 0)
					str += "<span color=\"blue\">";
				else if (cmp < 0)
					str += "<span color=\"red\">";

				if (cmp == 0) {
					if (sel.isInstalled()) {
						if (!hasAvailable)  // red for orphan too
							str += "<span color=\"red\">";
						str += sel.installed().number();
						if (!hasAvailable)
							str += "</span>";
					}
					else
						str += sel.candidate().number();
				}
				else {
					str += sel.candidate().number();
					str += "</span>\n<small>";
					str += sel.installed().number();
					str += "</small>";
				}
fprintf (stderr, "version: %s\n", str.c_str());
				g_value_set_string (value, str.c_str());
				break;
			}
			case INSTALLED_CHECK_PROP: {  // as it is, will it be installed at apply?
				bool installed;
				if (sel.toInstall())
					installed = true;
				else if (sel.toRemove())
					installed = false;
				else
					installed = sel.isInstalled();
				g_value_set_boolean (value, installed);
				break;
			}
			case HAS_UPGRADE_PROP:
				g_value_set_boolean (value, sel.hasUpgrade());
				break;
			case TO_UPGRADE_PROP:
				g_value_set_boolean (value, sel.hasUpgrade() && sel.toInstall());
				break;
			case TO_MODIFY_PROP:
				g_value_set_boolean (value, sel.toModify());
				break;
			case IS_LOCKED_PROP:
				g_value_set_boolean (value, !sel.isLocked());
				break;
			case BACKGROUND_PROP: {
				const char *color = 0;
				if (sel.toModify())
					color = "#f4f4b7";
				g_value_set_string (value, color);
				break;
			}
			case XPAD_PROP: {
				int xpad = sel.toModifyAuto() ? 15 : 0;
				g_value_set_int (value, xpad);
				break;
			}
			case STATUS_ICON_PROP:
				g_value_set_string (value, getStatusStockIcon (sel));
				break;
			case PTR_PROP:
				g_value_set_pointer (value, (void *) &sel);
				break;
		}
	}

	virtual void selectableModified()
	{
		for (int i = 0; i < rowsNb(); i++)
			listener->rowChanged (i);
	}
};

static GtkTreeModel *ygtk_zypp_model_new (Ypp::List list, const std::list <std::string> &keywords)
{ return ygtk_tree_model_new (new YGtkZyppModel (list, keywords)); }

static Ypp::Selectable *ygtk_zypp_model_get_sel (GtkTreeModel *model, gchar *path_str)
{
	GtkTreeIter iter;
	gtk_tree_model_get_iter_from_string (model, &iter, path_str);
	Ypp::Selectable *sel;
	gtk_tree_model_get (model, &iter, PTR_PROP, &sel, -1);
	return sel;
}

static Ypp::Selectable *ygtk_zypp_model_get_sel (GtkTreeModel *model, GtkTreePath *path)
{
	GtkTreeIter iter;
	gtk_tree_model_get_iter (model, &iter, path);
	Ypp::Selectable *sel;
	gtk_tree_model_get (model, &iter, PTR_PROP, &sel, -1);
	return sel;
}

//** View

struct YGtkPkgListView::Impl {
	GtkWidget *scroll, *view;
	YGtkPkgListView::Listener *listener;
	Ypp::List list;
	bool descriptiveTooltip;
	int sort_attrb, ascendent : 2;
	bool userModified;
	std::list <std::string> m_keywords;

	Impl (bool descriptiveTooltip, int default_sort_attrb) : listener (NULL), list (0),
		descriptiveTooltip (descriptiveTooltip), sort_attrb (default_sort_attrb),
		ascendent (true), userModified (false) {}

	void setList (Ypp::List _list, int _attrb, bool _ascendent, bool userSorted)
	{
		if (userSorted) userModified = true;
		if (_list != list || sort_attrb != _attrb || ascendent != _ascendent) {
			if (_attrb != -1) {
				if (_list == list && sort_attrb == _attrb)
					_list.reverse();
				else
					_list.sort ((Ypp::List::SortAttribute) _attrb, _ascendent);
			}
			list = _list;
			sort_attrb = _attrb;
			ascendent = _ascendent;
		}

		GtkTreeModel *model = ygtk_zypp_model_new (list.clone(), m_keywords);
		gtk_tree_view_set_model (GTK_TREE_VIEW (view), model);
		g_object_unref (G_OBJECT (model));
		setHighlight (m_keywords);

		if (userModified) {
			GList *columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (view));
			for (GList *i = columns; i; i = i->next) {
				GtkTreeViewColumn *column = (GtkTreeViewColumn *) i->data;
				bool v = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (column), "attrb")) == _attrb;
				gtk_tree_view_column_set_sort_indicator (column, v);
				if (v)
					gtk_tree_view_column_set_sort_order (column,
						ascendent ? GTK_SORT_ASCENDING : GTK_SORT_DESCENDING);
			}
			g_list_free (columns);
		}

		// search_column is one prop (among others) that gets reset on new model
		gtk_tree_view_set_search_column (GTK_TREE_VIEW (view), NAME_PROP);
	}

	void setHighlight (const std::list <std::string> &keywords)
	{
		if (m_keywords.empty() && keywords.empty())
			return;
		m_keywords = keywords;
		GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
		YGtkZyppModel *zmodel = (YGtkZyppModel *) ygtk_tree_model_get_model (model);
		zmodel->setHighlight (keywords);
		gtk_widget_queue_draw (view);
	}
};

static void right_click_cb (YGtkTreeView *view, gboolean outreach, YGtkPkgListView *pThis)
{
	struct inner {
		static void appendItem (GtkWidget *menu, const char *label,
			const char *tooltip, const char *stock, bool sensitive,
			void (& callback) (GtkMenuItem *item, YGtkPkgListView *pThis), YGtkPkgListView *pThis)
		{
			GtkWidget *item;
			if (stock) {
				if (label) {
					item = gtk_image_menu_item_new_with_mnemonic (label);
					GtkWidget *image = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_MENU);
					gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
				}
				else
					item = gtk_image_menu_item_new_from_stock (stock, NULL);
			}
			else
				item = gtk_menu_item_new_with_mnemonic (label);
			if (tooltip)
				gtk_widget_set_tooltip_markup (item, tooltip);
			if (!sensitive)
				gtk_widget_set_sensitive (item, FALSE);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (callback), pThis);
		}
		static void install_cb (GtkMenuItem *item, YGtkPkgListView *pThis)
		{ pThis->getSelected().install(); }
		static void remove_cb (GtkMenuItem *item, YGtkPkgListView *pThis)
		{ pThis->getSelected().remove(); }
		static void undo_cb (GtkMenuItem *item, YGtkPkgListView *pThis)
		{ pThis->getSelected().undo(); }
		static void lock_cb (GtkMenuItem *item, YGtkPkgListView *pThis)
		{ pThis->getSelected().lock (true); }
		static void unlock_cb (GtkMenuItem *item, YGtkPkgListView *pThis)
		{ pThis->getSelected().lock (false); }
		static void select_all_cb (GtkMenuItem *item, YGtkPkgListView *pThis)
		{ pThis->selectAll(); }
		static void show_column_cb (GtkCheckMenuItem *item, YGtkPkgListView *pThis)
		{
			int col = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item), "column"));
			GtkTreeViewColumn *column = gtk_tree_view_get_column (
				GTK_TREE_VIEW (pThis->impl->view), col);
			bool visible = gtk_check_menu_item_get_active (item);
			gtk_tree_view_column_set_visible (column, visible);
		}
	};

	GtkWidget *menu = gtk_menu_new();
	bool empty = true;
	Ypp::List list = pThis->getSelected();
	if (!outreach) {
		Ypp::ListProps props (list);

		bool canLock = props.canLock(), unlocked = props.isUnlocked();
		bool locked = !unlocked && canLock;
		if (props.isNotInstalled())
			inner::appendItem (menu, _("_Install"), 0, GTK_STOCK_SAVE,
				!locked, inner::install_cb, pThis), empty = false;
		if (props.hasUpgrade())
			inner::appendItem (menu, _("_Upgrade"), 0, GTK_STOCK_GO_UP,
				!locked, inner::install_cb, pThis), empty = false;
		if (props.isInstalled() && props.canRemove())
			inner::appendItem (menu, _("_Remove"), 0, GTK_STOCK_DELETE,
				!locked, inner::remove_cb, pThis), empty = false;
		if (props.toModify())
			inner::appendItem (menu, _("_Undo"), 0, GTK_STOCK_UNDO,
				true, inner::undo_cb, pThis), empty = false;
		if (canLock) {
			static const char *lock_tooltip =
				"<b>Package lock:</b> prevents the package status from being modified by "
				"the solver (that is, it won't honour dependencies or collections ties.)";
			if (props.isLocked())
				inner::appendItem (menu, _("_Unlock"), _(lock_tooltip),
					GTK_STOCK_DIALOG_AUTHENTICATION, true, inner::unlock_cb, pThis), empty = false;
			if (unlocked)
				inner::appendItem (menu, _("_Lock"), _(lock_tooltip),
					GTK_STOCK_DIALOG_AUTHENTICATION, true, inner::lock_cb, pThis), empty = false;
		}
	}

	Ypp::Selectable::Type type = Ypp::Selectable::PACKAGE;
	if (list.size())
		type = list.get(0).type();
	if (type == Ypp::Selectable::PACKAGE || type == Ypp::Selectable::PATCH) {
		if (!empty)
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());
		inner::appendItem (menu, NULL, NULL, GTK_STOCK_SELECT_ALL,
		                   true, inner::select_all_cb, pThis);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());
		GtkWidget *item = gtk_menu_item_new_with_mnemonic (_("_Show column"));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		GtkWidget *submenu = gtk_menu_new();
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
		GList *columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (pThis->impl->view));
		int n = 0;
		for (GList *i = columns; i; i = i->next, n++) {
			GtkTreeViewColumn *column = (GtkTreeViewColumn *) i->data;
			const gchar *header = gtk_tree_view_column_get_title (column);
			if (header) {
				GtkWidget *item = gtk_check_menu_item_new_with_label (header);
				g_object_set_data (G_OBJECT (item), "column", GINT_TO_POINTER (n));
				gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item);
				gboolean visible = gtk_tree_view_column_get_visible (column);
				gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), visible);
				g_signal_connect (G_OBJECT (item), "toggled",
					G_CALLBACK (inner::show_column_cb), pThis);
			}
		}
		g_list_free (columns);
	}
	ygtk_tree_view_popup_menu (YGTK_TREE_VIEW (pThis->impl->view), menu);
}

static void selection_changed_cb (GtkTreeSelection *selection, YGtkPkgListView *pThis)
{
	if (GTK_WIDGET_REALIZED (pThis->impl->view) && pThis->impl->listener)
		pThis->impl->listener->selectionChanged();
}

static void row_activated_cb (GtkTreeView *view, GtkTreePath *path,
	GtkTreeViewColumn *column, YGtkPkgListView *pThis)
{
	GtkTreeModel *model = gtk_tree_view_get_model (view);
	Ypp::Selectable *sel = ygtk_zypp_model_get_sel (model, path);
	if (sel->toModify())
		sel->undo();
	else if (sel->isInstalled())
		sel->remove();
	else
		sel->install();
}

static void check_toggled_cb (GtkCellRendererToggle *renderer, gchar *path_str,
	YGtkPkgListView *pThis)
{
	GtkTreeView *view = GTK_TREE_VIEW (pThis->impl->view);
	GtkTreeModel *model = gtk_tree_view_get_model (view);
	Ypp::Selectable *sel = ygtk_zypp_model_get_sel (model, path_str);

	gboolean active = gtk_cell_renderer_toggle_get_active (renderer);
	active ? sel->remove() : sel->install();
}

static void upgrade_toggled_cb (YGtkCellRendererSideButton *renderer, gchar *path_str,
	YGtkPkgListView *pThis)
{
	GtkTreeView *view = GTK_TREE_VIEW (pThis->impl->view);
	GtkTreeModel *model = gtk_tree_view_get_model (view);
	Ypp::Selectable *sel = ygtk_zypp_model_get_sel (model, path_str);
	sel->toInstall() ? sel->undo() : sel->install();
}

static void undo_toggled_cb (YGtkCellRendererButton *renderer, gchar *path_str,
	YGtkPkgListView *pThis)
{
	GtkTreeView *view = GTK_TREE_VIEW (pThis->impl->view);
	GtkTreeModel *model = gtk_tree_view_get_model (view);
	Ypp::Selectable *sel = ygtk_zypp_model_get_sel (model, path_str);
	sel->undo();
}

static gboolean query_tooltip_cb (GtkWidget *widget, gint x, gint y,
	gboolean keyboard_mode, GtkTooltip *tooltip, YGtkPkgListView *pThis)
{
	GtkTreeView *view = GTK_TREE_VIEW (widget);
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	if (gtk_tree_view_get_tooltip_context (view,
	        &x, &y, keyboard_mode, &model, &path, &iter)) {
		gtk_tree_view_set_tooltip_row (view, tooltip, path);

		Ypp::Selectable *sel = ygtk_zypp_model_get_sel (model, path);
		gtk_tree_path_free (path);
		std::string text;
		text.reserve (256);
		const char *icon = 0;

		GtkTreeViewColumn *column;
		int bx, by;
		gtk_tree_view_convert_widget_to_bin_window_coords (
			view, x, y, &bx, &by);
		gtk_tree_view_get_path_at_pos (
			view, x, y, NULL, &column, NULL, NULL);

		GtkIconSize icon_size = GTK_ICON_SIZE_MENU;
		if (column == gtk_tree_view_get_column (view, 0)) {
			text = getStatusSummary (*sel);
			icon = getStatusStockIcon (*sel);
		}
		else if (pThis->impl->descriptiveTooltip) {
			text = std::string ("<b>") + sel->name() + "</b>\n\n";
			text += sel->description (false);
			switch (sel->type()) {
				case Ypp::Selectable::PATTERN: {
					ZyppPattern pattern = castZyppPattern (sel->zyppSel()->theObj());
					icon = pattern->icon().asString().c_str();
					if (!(*icon))
						icon = "pattern-generic";
					break;
				}
				case Ypp::Selectable::PACKAGE:
					icon = getStatusStockIcon (*sel);
					break;
				default: break;
			}
			icon_size = GTK_ICON_SIZE_DIALOG;
		}

		if (text.empty())
			return FALSE;
		gtk_tooltip_set_markup (tooltip, text.c_str());
		if (icon)
			gtk_tooltip_set_icon_from_icon_name (tooltip, icon, icon_size);
		return TRUE;
	}
	return FALSE;
}

static void column_clicked_cb (GtkTreeViewColumn *column, YGtkPkgListView *pThis)
{
	int attrb = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (column), "attrb"));
	bool ascendent = true;
	if (gtk_tree_view_column_get_sort_indicator (column))
		ascendent = gtk_tree_view_column_get_sort_order (column) == GTK_SORT_DESCENDING;
	pThis->impl->setList (pThis->impl->list, attrb, ascendent, true);
}

static void set_sort_column (YGtkPkgListView *pThis, GtkTreeViewColumn *column, int property)
{
	int attrb = -1;
	switch (property) {
		case INSTALLED_CHECK_PROP: attrb = Ypp::List::IS_INSTALLED_SORT; break;
		case NAME_PROP: case NAME_SUMMARY_PROP: attrb = Ypp::List::NAME_SORT; break;
		case REPOSITORY_PROP: attrb = Ypp::List::REPOSITORY_SORT; break;
		case SUPPORT_PROP: attrb = Ypp::List::SUPPORT_SORT; break;
		case SIZE_PROP: attrb = Ypp::List::SIZE_SORT; break;
	}

	gtk_tree_view_column_set_clickable (column, true);
	g_object_set_data (G_OBJECT (column), "attrb", GINT_TO_POINTER (attrb));
	if (attrb != -1)
		g_signal_connect (G_OBJECT (column), "clicked",
			              G_CALLBACK (column_clicked_cb), pThis);
}

YGtkPkgListView::YGtkPkgListView (bool descriptiveTooltip, int default_sort)
: impl (new Impl (descriptiveTooltip, default_sort))
{
	impl->scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (impl->scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (impl->scroll),
		GTK_SHADOW_IN);

	GtkTreeView *view = GTK_TREE_VIEW (impl->view = ygtk_tree_view_new());
	gtk_tree_view_set_fixed_height_mode (view, TRUE);
	gtk_tree_view_set_headers_visible (view, FALSE);

	GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	g_signal_connect (G_OBJECT (selection), "changed",
	                  G_CALLBACK (selection_changed_cb), this);

	g_signal_connect (G_OBJECT (view), "row-activated",
	                  G_CALLBACK (row_activated_cb), this);
	g_signal_connect (G_OBJECT (view), "right-click",
		              G_CALLBACK (right_click_cb), this);
	gtk_widget_set_has_tooltip (impl->view, TRUE);
	g_signal_connect (G_OBJECT (view), "query-tooltip",
	                  G_CALLBACK (query_tooltip_cb), this);

	gtk_container_add (GTK_CONTAINER (impl->scroll), impl->view);
	gtk_widget_show_all (impl->scroll);
}

YGtkPkgListView::~YGtkPkgListView()
{ delete impl; }

GtkWidget *YGtkPkgListView::getWidget()
{ return impl->scroll; }

GtkWidget *YGtkPkgListView::getView()
{ return impl->view; }

void YGtkPkgListView::setQuery (Ypp::Query &query)
{ setList (Ypp::List (query)); }

void YGtkPkgListView::setList (Ypp::List query)
{
	Ypp::List list (query);
	impl->setList (list, impl->sort_attrb, impl->ascendent, false);
}

void YGtkPkgListView::setHighlight (const std::list <std::string> &keywords)
{
	int index = keywords.size() == 1 ? impl->list.find (keywords.front()) : -1;
	if (index != -1) {
		GtkTreeView *view = GTK_TREE_VIEW (impl->view);
		GtkTreePath *path = gtk_tree_path_new_from_indices (index, -1);
		gtk_tree_view_scroll_to_cell (view, path, NULL, TRUE, .5, 0);

		GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
		gtk_tree_selection_select_path (selection, path);
		gtk_tree_path_free (path);
	}
	else if (GTK_WIDGET_REALIZED (impl->view))
		gtk_tree_view_scroll_to_point (GTK_TREE_VIEW (impl->view), -1, 0);

	impl->setHighlight (keywords);
}

void YGtkPkgListView::addTextColumn (const char *header, int property, bool visible, int size, bool identAuto)
{
	GtkTreeView *view = GTK_TREE_VIEW (impl->view);
	if (header)
		gtk_tree_view_set_headers_visible (view, TRUE);
	GtkCellRenderer *renderer;
	if (property == VERSION_PROP) {
		renderer = ygtk_cell_renderer_side_button_new();
		g_object_set (G_OBJECT (renderer), "stock-id", GTK_STOCK_GO_UP, NULL);
		g_signal_connect (G_OBJECT (renderer), "toggled",
				          G_CALLBACK (upgrade_toggled_cb), this);
	}
	else if (property == REPOSITORY_PROP)
		renderer = ygtk_cell_renderer_text_pixbuf_new();
	else
		renderer = gtk_cell_renderer_text_new();
	PangoEllipsizeMode ellipsize = PANGO_ELLIPSIZE_END;
	if (size >= 0 && property != NAME_SUMMARY_PROP)
		ellipsize = PANGO_ELLIPSIZE_MIDDLE;
	g_object_set (G_OBJECT (renderer), "ellipsize", ellipsize, NULL);

	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (
		header, renderer, "markup", property, "sensitive", IS_LOCKED_PROP,
		"cell-background", BACKGROUND_PROP, NULL);
	if (property == VERSION_PROP) {
		gtk_tree_view_column_add_attribute (column, renderer,
			"button-visible", HAS_UPGRADE_PROP);
		gtk_tree_view_column_add_attribute (column, renderer,
			"active", TO_UPGRADE_PROP);
	}
	else if (property == REPOSITORY_PROP)
		gtk_tree_view_column_add_attribute (column, renderer,
			"stock-id", REPOSITORY_STOCK_PROP);

	if (size != -1)  // on several columns
		gtk_tree_view_set_rules_hint (view, TRUE);
	if (identAuto)
		gtk_tree_view_column_add_attribute (column, renderer, "xpad", XPAD_PROP);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_resizable (column, TRUE);
	if (size >= 0)
		gtk_tree_view_column_set_fixed_width (column, size);
	else
		gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_visible (column, visible);
	set_sort_column (this, column, property);
	gtk_tree_view_append_column (view, column);
}

void YGtkPkgListView::addCheckColumn (int property)
{
	GtkTreeView *view = GTK_TREE_VIEW (impl->view);
	GtkCellRenderer *renderer = gtk_cell_renderer_toggle_new();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (NULL,
		renderer, "active", property, "sensitive", IS_LOCKED_PROP,
		"cell-background", BACKGROUND_PROP, NULL);
	g_signal_connect (G_OBJECT (renderer), "toggled",
	                  G_CALLBACK (check_toggled_cb), this);

	// it seems like GtkCellRendererToggle has no width at start, so fixed doesn't work
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 25);
	set_sort_column (this, column, property);
	gtk_tree_view_append_column (view, column);
}

void YGtkPkgListView::addUndoButtonColumn (const char *header)
{
	GtkCellRenderer *renderer = ygtk_cell_renderer_button_new();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (
		header, renderer, "sensitive", IS_LOCKED_PROP,
		"cell-background", BACKGROUND_PROP,
		"visible", TO_MODIFY_PROP, NULL);
	const char *text = _("Undo");
	g_object_set (G_OBJECT (renderer), "text", text, NULL);
	gboolean show_icon;
	g_object_get (G_OBJECT (gtk_settings_get_default()), "gtk-button-images", &show_icon, NULL);
	if (show_icon)
		g_object_set (G_OBJECT (renderer), "stock-id", GTK_STOCK_UNDO, NULL);
	g_signal_connect (G_OBJECT (renderer), "toggled",
		              G_CALLBACK (undo_toggled_cb), this);

	PangoRectangle rect;
	int width = 0;
	PangoLayout *layout = gtk_widget_create_pango_layout (impl->view, text);
	pango_layout_get_pixel_extents (layout, NULL, &rect);
	width = MAX (width, rect.width);
	g_object_unref (G_OBJECT (layout));
	width += 16;
	if (show_icon) {
		int icon_width;
		gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (impl->view),
			GTK_ICON_SIZE_MENU, &icon_width, NULL);
		width += icon_width;
	}

	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, width);
	gtk_tree_view_append_column (GTK_TREE_VIEW (impl->view), column);
}

void YGtkPkgListView::addImageColumn (const char *header, int property)
{
	GtkTreeView *view = GTK_TREE_VIEW (impl->view);
	GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (
		header, renderer, "icon-name", property, "cell-background", BACKGROUND_PROP, NULL);

	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	int height = MAX (32, YGUtils::getCharsHeight (impl->view, 1));
	gtk_cell_renderer_set_fixed_size (renderer, -1, height);
	gtk_tree_view_column_set_fixed_width (column, 38);
	gtk_tree_view_append_column (view, column);
}

void YGtkPkgListView::setListener (YGtkPkgListView::Listener *listener)
{ impl->listener = listener; }

Ypp::List YGtkPkgListView::getSelected()
{
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (impl->view));
	GList *rows = gtk_tree_selection_get_selected_rows (selection, &model);

	Ypp::List list (g_list_length (rows));
	for (GList *i = rows; i; i = i->next) {
		GtkTreeIter iter;
		GtkTreePath *path = (GtkTreePath *) i->data;
		gtk_tree_model_get_iter (model, &iter, path);

		Ypp::Selectable *sel;
		gtk_tree_model_get (model, &iter, PTR_PROP, &sel, -1);
		gtk_tree_path_free (path);
		list.append (*sel);
	}
	g_list_free (rows);
	return list;
}

void YGtkPkgListView::selectAll()
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (impl->view));
	gtk_tree_selection_select_all (selection);
}

// utilities

std::string getStatusSummary (Ypp::Selectable &sel)
{
	std::string text;
	if (sel.isLocked())
		text = _("locked: right-click to unlock");
	else if (sel.toInstall()) {
		Ypp::Version candidate = sel.candidate();
		text = _("To install") + std::string (" ") + candidate.number();
	}
	else if (sel.toRemove())
		text = _("To remove");
	else if (sel.isInstalled()) {
		text = _("Installed");
		if (sel.hasUpgrade())
			text += _(" (upgrade available)");
	}
	else
		text = _("Not installed");
	if (sel.toModifyAuto())
		text += _("\n<i>status changed by the dependency solver</i>");
	return text;
}

const char *getStatusStockIcon (Ypp::Selectable &sel)
{
	const char *icon;
	if (sel.isLocked())
		icon = GTK_STOCK_DIALOG_AUTHENTICATION;
	else if (sel.toInstall()) {
		icon = GTK_STOCK_ADD;
		if (sel.type() == Ypp::Selectable::PACKAGE) {
			Ypp::Version candidate = sel.candidate();
			if (sel.isInstalled()) {
				Ypp::Version installed = sel.installed();
				if (candidate > installed)
					icon = GTK_STOCK_GO_UP;
				else if (candidate < installed)
					icon = GTK_STOCK_GO_DOWN;
				else // if (candidate == installed)
					icon = GTK_STOCK_REFRESH;
			}
		}
	}
	else if (sel.toRemove())
		icon = GTK_STOCK_REMOVE;
	else if (sel.isInstalled())
		icon = GTK_STOCK_HARDDISK;
	else
		icon = "package";
	return icon;
}

std::string getRepositoryLabel (Ypp::Repository &repo)
{
	std::string name (repo.name()), url, str;
	url = repo.isSystem() ? _("Local database") : repo.url();
	str.reserve (name.size() + url.size() + 32);
	str = name + "\n<small>" + url + "</small>";
	return str;
}

const char *getRepositoryStockIcon (Ypp::Repository &repo)
{
	std::string url (repo.url());
	const char *icon;
	if (repo.isSystem())
		icon = "yast-host";
	else if (url.empty())
		icon = GTK_STOCK_MISSING_IMAGE;
	else if (url.compare (0, 2, "cd", 2) == 0 || url.compare (0, 3, "dvd", 3) == 0)
		icon = GTK_STOCK_CDROM;
	else if (url.compare (0, 3, "iso", 3) == 0)
		icon = GTK_STOCK_FILE;
	else if (url.find ("KDE") != std::string::npos)
		icon = "pattern-kde";
	else if (url.find ("GNOME") != std::string::npos)
		icon = "pattern-gnome";
	else if (url.find ("update") != std::string::npos)
		icon = "yast-update";
	else if (url.find ("home") != std::string::npos)
		icon = "yast-users";
	else
		icon = GTK_STOCK_NETWORK;
	return icon;
}

void highlightMarkup (std::string &text, const std::list <std::string> &keywords,
                      const char *openTag, const char *closeTag, int openTagLen, int closeTagLen)
{
	if (keywords.empty()) return;
	for (std::list <std::string>::const_iterator it = keywords.begin();
	     it != keywords.end(); it++) {
		const std::string &keyword = *it;
		const char *c = text.c_str();
		while ((c = strcasestr (c, keyword.c_str()))) {
			int pos = c - text.c_str(), len = keyword.size();
			text.insert (pos+len, closeTag);
			text.insert (pos, openTag);
			c = text.c_str() + pos + len + openTagLen + closeTagLen - 2;
		}
	}
}

void highlightMarkupSpan (std::string &text, const std::list <std::string> &keywords)
{
	static const char openTag[] = "<span fgcolor=\"#000000\" bgcolor=\"#ffff00\">";
	static const char closeTag[] = "</span>";
	highlightMarkup (text, keywords, openTag, closeTag, sizeof (openTag), sizeof (closeTag));
}

