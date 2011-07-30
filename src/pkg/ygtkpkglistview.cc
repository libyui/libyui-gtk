/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* YGtkPkgListView, Zypp GtkTreeView implementation */
// check the header file for information about this widget

/*
  Textdomain "gtk"
 */

#define YUILogComponent "gtk"
#include "config.h"
#include "YGi18n.h"
#include "YGUtils.h"
#include "YGUI.h"
#include "YGPackageSelector.h"
#include "ygtkpkglistview.h"
#include "ygtktreeview.h"
#include "ygtktreemodel.h"
#include "ygtkcellrenderertext.h"
#include "ygtkcellrendererbutton.h"
#include "ygtkcellrenderersidebutton.h"
#include <gtk/gtk.h>
#include <string.h>

#define GRAY_COLOR "#727272"

//** Model

enum ImplProperty {
	// booleans
	HAS_UPGRADE_PROP = TOTAL_PROPS, TO_UPGRADE_PROP, CAN_TOGGLE_INSTALL_PROP,
	MANUAL_MODIFY_PROP, IS_LOCKED_PROP,
	// integer
	XPAD_PROP,
	// string
	FOREGROUND_PROP, BACKGROUND_PROP, REPOSITORY_STOCK_PROP,
	ACTION_ICON_PROP,
	// pointer
	PTR_PROP,
	TOTAL_IMPL_PROPS
};

static GType _columnType (int col)
{
	switch (col) {
		case NAME_PROP: case ACTION_NAME_PROP: case NAME_SUMMARY_PROP:
		case VERSION_PROP: case SINGLE_VERSION_PROP: case REPOSITORY_PROP:
		case SUPPORT_PROP: case SIZE_PROP: case STATUS_ICON_PROP:
		case ACTION_BUTTON_PROP: case ACTION_ICON_PROP: case FOREGROUND_PROP:
		case BACKGROUND_PROP: case REPOSITORY_STOCK_PROP:
			return G_TYPE_STRING;
		case INSTALLED_CHECK_PROP:
		case HAS_UPGRADE_PROP: case TO_UPGRADE_PROP: case CAN_TOGGLE_INSTALL_PROP:
		case MANUAL_MODIFY_PROP: case IS_LOCKED_PROP:
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
	// we pass GtkTreeView to the model, so we can test for selected rows
	// and modify text markup appropriely (trashing the data-view model, heh)

	YGtkZyppModel (Ypp::List list) : m_list (list.clone())
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
			case ACTION_NAME_PROP: {
				std::string str ("<b>"), name (sel.name());
				str.reserve (name.size() + 35);
				str += getStatusAction (&sel);
				str += "</b> ";
				str += name;
				g_value_set_string (value, str.c_str());
				break;
			}
			case NAME_SUMMARY_PROP: {
				std::string name (sel.name()), summary (sel.summary()), str;
				summary = YGUtils::escapeMarkup (summary);
				highlightMarkupSpan (name, m_keywords);
				highlightMarkupSpan (summary, m_keywords);
				str.reserve (name.size() + summary.size() + 64);
				str = name;
				if (!summary.empty()) {
					str += "\n";
					str += "<span color=\"" GRAY_COLOR "\">";
					str += "<small>" + summary + "</small>";
					str += "</span>";
				}
				g_value_set_string (value, str.c_str());
				break;
			}
			case REPOSITORY_PROP: {
				std::string str;
				if (sel.hasCandidateVersion()) {
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
				if (sel.hasCandidateVersion()) {
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
				if (sel.type() == Ypp::Selectable::PACKAGE) {
					Ypp::Package pkg (sel);
					std::string str (Ypp::Package::supportSummary (pkg.support()));
					g_value_set_string (value, str.c_str());
				}
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
				bool hasCandidate = sel.hasCandidateVersion();
				bool patch = false;
				if (hasCandidate) {
					Ypp::Version candidate = sel.candidate();
					if (sel.isInstalled()) {
						Ypp::Version installed = sel.installed();
						if (candidate > installed)
							cmp = 1;
						if (candidate < installed)
							cmp = -1;
					}
				}
				if (cmp > 0) {
					str += "<span color=\"blue\">";

					if (sel.type() == Ypp::Selectable::PACKAGE) {
						Ypp::Package pkg (sel);
						patch = pkg.isCandidatePatch();
					}
				}
				else if (cmp < 0)
					str += "<span color=\"red\">";

				if (cmp == 0) {
					if (sel.isInstalled()) {
						if (!hasCandidate)  // red for orphan too
							str += "<span color=\"red\">";
						str += sel.installed().number();
						if (!hasCandidate)
							str += "</span>";
					}
					else
						str += sel.candidate().number();
				}
				else {
					str += sel.candidate().number();
					if (patch)
					{ str += " <small>"; str += _("patch"); str += "</small>"; }
					str += "</span>\n<small>";
					str += sel.installed().number();
					str += "</small>";
				}
				g_value_set_string (value, str.c_str());
				break;
			}
			case SINGLE_VERSION_PROP: {
				std::string str;
				str.reserve (128);
				if (sel.hasCandidateVersion() && !sel.toRemove())
					str = sel.candidate().number();
				else
					str = sel.installed().number();
				g_value_set_string (value, str.c_str());
				break;
			}
			case INSTALLED_CHECK_PROP: {
				bool installed;  // whether it is installed or will be at apply
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
			case CAN_TOGGLE_INSTALL_PROP:
				g_value_set_boolean (value, !sel.isInstalled() || sel.canRemove());
				break;
			case MANUAL_MODIFY_PROP:
				g_value_set_boolean (value, sel.toModify() && !sel.toModifyAuto());
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
			case FOREGROUND_PROP: {
				const char *color = 0;
				if (sel.toModifyAuto())
					color = "#6f6f6f";
				g_value_set_string (value, color);
				break;
			}
			case XPAD_PROP: {
				int xpad = sel.toModifyAuto() ? 20 : 0;
				g_value_set_int (value, xpad);
				break;
			}
			case STATUS_ICON_PROP:
				g_value_set_string (value, getStatusStockIcon (sel));
				break;
			case ACTION_ICON_PROP: {
				const char *stock;
				if (sel.toModify())
					stock = GTK_STOCK_UNDO;
				else if (sel.isInstalled())
					stock = GTK_STOCK_REMOVE;
				else
					stock = GTK_STOCK_ADD;
				g_value_set_string (value, stock);
				break;
			}
			case ACTION_BUTTON_PROP: {
				const char *text;
				if (sel.toModify())
					text = _("Undo");
				else if (sel.isInstalled())
					text = _("Remove");
				else
					text = _("Install");
				g_value_set_string (value, text);
				break;
			}
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
	bool indentAuto, colorModified;

	Impl (bool descriptiveTooltip, int default_sort_attrb, bool indentAuto, bool colorModified)
	: listener (NULL), list (0), descriptiveTooltip (descriptiveTooltip),
	  sort_attrb (default_sort_attrb), ascendent (true), userModified (false),
	  indentAuto (indentAuto), colorModified (colorModified) {}

	void setList (Ypp::List _list, int _attrb, bool _ascendent, bool userSorted, const std::list <std::string> &keywords)
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

		GtkTreeModel *model = ygtk_tree_model_new (new YGtkZyppModel (list));
		gtk_tree_view_set_model (GTK_TREE_VIEW (view), model);
		g_object_unref (G_OBJECT (model));
		setHighlight (keywords);

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
		if (m_keywords.empty() && keywords.empty()) return;
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
		static void appendItem (GtkWidget *menu, const char *_label,
			const char *tooltip, const char *stock, bool sensitive,
			void (& callback) (GtkMenuItem *item, YGtkPkgListView *pThis), YGtkPkgListView *pThis)
		{
			GtkWidget *item;
			std::string label;
			if (_label)
				label = YGUtils::mapKBAccel (_label);
			if (stock) {
				if (_label) {
					item = gtk_image_menu_item_new_with_mnemonic (label.c_str());
					GtkWidget *image = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_MENU);
					gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
				}
				else
					item = gtk_image_menu_item_new_from_stock (stock, NULL);
			}
			else
				item = gtk_menu_item_new_with_mnemonic (label.c_str());
			if (tooltip)
				gtk_widget_set_tooltip_markup (item, tooltip);
			gtk_widget_set_sensitive (item, sensitive);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (callback), pThis);
		}
		static void install_cb (GtkMenuItem *item, YGtkPkgListView *pThis)
		{ pThis->getSelected().install(); }
		static void reinstall_cb (GtkMenuItem *item, YGtkPkgListView *pThis)
		{ reinstall (pThis->getSelected()); }
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

		static bool canReinstall (Ypp::List list)
		{
			for(int i = 0; i < list.size(); i++) {
				Ypp::Selectable &sel = list.get(i);
				if (sel.hasInstalledVersion()) {
					Ypp::Version installedVersion = sel.installed();
					int j;
					for (j = 0; j < sel.totalVersions(); j++) {
						Ypp::Version version = sel.version (j);
						if (!version.isInstalled() && version == installedVersion)
							break;
					}
					if(j == sel.totalVersions())
						return false;
				}
				else
					return false;
			}
			return true;
		}
		static void reinstall (Ypp::List list)
		{
			for(int i = 0; i < list.size(); i++) {
				Ypp::Selectable &sel = list.get(i);
				Ypp::Version installedVersion = sel.installed();
				for (int j = 0; j < sel.totalVersions(); j++) {
					Ypp::Version version = sel.version (j);
					if (!version.isInstalled() && version == installedVersion) {
						sel.setCandidate (version);
						sel.install();
						break;
					}
				}
			}
		}
	};

	GtkWidget *menu = gtk_menu_new();
	Ypp::List list = pThis->getSelected();
	Ypp::Selectable::Type type = Ypp::Selectable::PACKAGE;
	if (list.size() > 0)
		type = list.get(0).type();

	if (!outreach) {
		Ypp::ListProps props (list);

		bool canLock = props.canLock(), unlocked = props.isUnlocked();
		bool modified = props.toModify();
		bool locked = !unlocked && canLock;
		if (props.isNotInstalled() && !modified)
			inner::appendItem (menu, _("&Install"), 0, GTK_STOCK_SAVE,
				!locked, inner::install_cb, pThis);
		if (props.hasUpgrade() && !modified)
			inner::appendItem (menu, _("&Upgrade"), 0, GTK_STOCK_GO_UP,
				!locked, inner::install_cb, pThis);
		if (type == Ypp::Selectable::PACKAGE && inner::canReinstall(list) && !modified)
			inner::appendItem (menu, _("&Re-install"), 0, GTK_STOCK_REFRESH,
				!locked, inner::reinstall_cb, pThis);
		if (props.isInstalled() && !modified)
			inner::appendItem (menu, _("&Remove"), 0, GTK_STOCK_DELETE,
				!locked && props.canRemove(), inner::remove_cb, pThis);
		if (modified)
			inner::appendItem (menu, _("&Undo"), 0, GTK_STOCK_UNDO,
				true, inner::undo_cb, pThis);
		if (canLock) {
			static const char *lock_tooltip =
				"<b>Package lock:</b> prevents the package status from being modified by "
				"the dependencies resolver.";
			if (props.isLocked())
				inner::appendItem (menu, _("&Unlock"), _(lock_tooltip),
					GTK_STOCK_DIALOG_AUTHENTICATION, true, inner::unlock_cb, pThis);
			if (unlocked)
				inner::appendItem (menu, _("&Lock"), _(lock_tooltip),
					GTK_STOCK_DIALOG_AUTHENTICATION, !modified,
					inner::lock_cb, pThis);
		}
	}

	if (type == Ypp::Selectable::PACKAGE || type == Ypp::Selectable::PATCH) {
		GList *items = gtk_container_get_children (GTK_CONTAINER (menu));
		g_list_free (items);

		if (items != NULL)  /* add separator if there are other items */
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());
		inner::appendItem (menu, NULL, NULL, GTK_STOCK_SELECT_ALL,
		                   true, inner::select_all_cb, pThis);
		ygtk_tree_view_append_show_columns_item (YGTK_TREE_VIEW (pThis->impl->view), menu);
	}
	ygtk_tree_view_popup_menu (YGTK_TREE_VIEW (pThis->impl->view), menu);
}

static void selection_changed_cb (GtkTreeSelection *selection, YGtkPkgListView *pThis)
{
	if (gtk_widget_get_realized (pThis->impl->view) && pThis->impl->listener)
		pThis->impl->listener->selectionChanged();
}

static void row_activated_cb (GtkTreeView *view, GtkTreePath *path,
	GtkTreeViewColumn *column, YGtkPkgListView *pThis)
{
	YGUI::ui()->busyCursor();
	if (YGPackageSelector::get()->yield()) return;

	GtkTreeModel *model = gtk_tree_view_get_model (view);
	Ypp::Selectable *sel = ygtk_zypp_model_get_sel (model, path);
	if (sel->toModify())
		sel->undo();
	else if (sel->isInstalled())
		sel->remove();
	else
		sel->install();

	YGUI::ui()->normalCursor();
}

static void check_toggled_cb (GtkCellRendererToggle *renderer, gchar *path_str,
	YGtkPkgListView *pThis)
{
	YGUI::ui()->busyCursor();
	if (YGPackageSelector::get()->yield()) return;

	GtkTreeView *view = GTK_TREE_VIEW (pThis->impl->view);
	GtkTreeModel *model = gtk_tree_view_get_model (view);
	Ypp::Selectable *sel = ygtk_zypp_model_get_sel (model, path_str);

	gboolean active = gtk_cell_renderer_toggle_get_active (renderer);
	if (sel->toModify())
		sel->undo();
	else
		active ? sel->remove() : sel->install();

	YGUI::ui()->normalCursor();
}

static void upgrade_toggled_cb (YGtkCellRendererButton *renderer, gchar *path_str,
	YGtkPkgListView *pThis)
{
	YGUI::ui()->busyCursor();
	if (YGPackageSelector::get()->yield()) return;

	GtkTreeView *view = GTK_TREE_VIEW (pThis->impl->view);
	GtkTreeModel *model = gtk_tree_view_get_model (view);
	Ypp::Selectable *sel = ygtk_zypp_model_get_sel (model, path_str);
	sel->toInstall() ? sel->undo() : sel->install();

	YGUI::ui()->normalCursor();
}

static void undo_toggled_cb (YGtkCellRendererButton *renderer, gchar *path_str,
	YGtkPkgListView *pThis)
{
	GtkTreeView *view = GTK_TREE_VIEW (pThis->impl->view);
	GtkTreeModel *model = gtk_tree_view_get_model (view);
	Ypp::Selectable *sel = ygtk_zypp_model_get_sel (model, path_str);
	sel->undo();
}

static void action_button_toggled_cb (YGtkCellRendererButton *renderer, gchar *path_str,
	YGtkPkgListView *pThis)
{
	GtkTreeView *view = GTK_TREE_VIEW (pThis->impl->view);
	GtkTreeModel *model = gtk_tree_view_get_model (view);
	Ypp::Selectable *sel = ygtk_zypp_model_get_sel (model, path_str);
	if (sel->toModify())
		sel->undo();
	else if (sel->isInstalled())
		sel->remove();
	else
		sel->install();
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
		if (column == ygtk_tree_view_get_column (YGTK_TREE_VIEW (view), 0)) {
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
	pThis->impl->setList (pThis->impl->list, attrb, ascendent, true, pThis->impl->m_keywords);
}

static void set_sort_column (YGtkPkgListView *pThis, GtkTreeViewColumn *column, int property)
{
	int attrb = -1;
	switch (property) {
		case INSTALLED_CHECK_PROP: attrb = Ypp::List::IS_INSTALLED_SORT; break;
		case NAME_PROP: case ACTION_NAME_PROP: case NAME_SUMMARY_PROP:
			attrb = Ypp::List::NAME_SORT; break;
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

YGtkPkgListView::YGtkPkgListView (bool descriptiveTooltip, int default_sort, bool indentAuto, bool colorModified, bool variableHeight)
: impl (new Impl (descriptiveTooltip, default_sort, indentAuto, colorModified))
{
	impl->scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (impl->scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (impl->scroll),
		GTK_SHADOW_IN);

	GtkTreeView *view = GTK_TREE_VIEW (impl->view = ygtk_tree_view_new (_("No matches.")));
	if (!variableHeight)
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

void YGtkPkgListView::setList (Ypp::List list)
{
	std::list <std::string> keywords;
	impl->setList (list, impl->sort_attrb, impl->ascendent, false, keywords);
}

void YGtkPkgListView::setHighlight (const std::list <std::string> &keywords)
{
	impl->setHighlight (keywords);

	int index = keywords.size() == 1 ? impl->list.find (keywords.front()) : -1;
	if (index != -1) {
		GtkTreeView *view = GTK_TREE_VIEW (impl->view);
		GtkTreePath *path = gtk_tree_path_new_from_indices (index, -1);
		gtk_tree_view_scroll_to_cell (view, path, NULL, TRUE, .5, 0);

		GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
		gtk_tree_selection_select_path (selection, path);
		gtk_tree_path_free (path);
	}
	else if (gtk_widget_get_realized (impl->view))
		gtk_tree_view_scroll_to_point (GTK_TREE_VIEW (impl->view), -1, 0);
}

void YGtkPkgListView::addTextColumn (const char *header, int property, bool visible, int size)
{
	GtkTreeView *view = GTK_TREE_VIEW (impl->view);
	if (header)
		gtk_tree_view_set_headers_visible (view, TRUE);
	GtkTreeViewColumn *column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title (column, header);

	GtkCellRenderer *renderer;
	if (property == REPOSITORY_PROP) {
		renderer = gtk_cell_renderer_pixbuf_new();
		if (gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL)
			gtk_tree_view_column_pack_end (column, renderer, FALSE);
		else
			gtk_tree_view_column_pack_start (column, renderer, FALSE);
		gtk_tree_view_column_set_attributes (column, renderer,
			"icon-name", REPOSITORY_STOCK_PROP, NULL);
	}

	if (property == VERSION_PROP) {
		renderer = ygtk_cell_renderer_side_button_new();
		g_object_set (G_OBJECT (renderer), "stock-id", GTK_STOCK_GO_UP, NULL);
		g_signal_connect (G_OBJECT (renderer), "toggled",
				          G_CALLBACK (upgrade_toggled_cb), this);
	}
	else
		renderer = ygtk_cell_renderer_text_new();

	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
		"markup", property, "sensitive", IS_LOCKED_PROP, NULL);

	if (impl->colorModified)
		gtk_tree_view_column_add_attribute (column, renderer,
			"cell-background", BACKGROUND_PROP);
	if (impl->indentAuto) {
		gtk_tree_view_column_add_attribute (column, renderer, "xpad", XPAD_PROP);
		gtk_tree_view_column_add_attribute (column, renderer, "foreground", FOREGROUND_PROP);
	}

	PangoEllipsizeMode ellipsize = PANGO_ELLIPSIZE_END;
	if (size >= 0 && property != NAME_SUMMARY_PROP)
		ellipsize = PANGO_ELLIPSIZE_MIDDLE;
	g_object_set (G_OBJECT (renderer), "ellipsize", ellipsize, NULL);

	if (property == VERSION_PROP) {
		gtk_tree_view_column_add_attribute (column, renderer,
			"button-visible", HAS_UPGRADE_PROP);
		gtk_tree_view_column_add_attribute (column, renderer,
			"active", TO_UPGRADE_PROP);
	}

	if (size != -1)  // on several columns
		gtk_tree_view_set_rules_hint (view, TRUE);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_resizable (column, TRUE);
	if (size >= 0)
		gtk_tree_view_column_set_fixed_width (column, size);
	else
		gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_visible (column, visible);
	set_sort_column (this, column, property);
	ygtk_tree_view_append_column (YGTK_TREE_VIEW (view), column);
}

void YGtkPkgListView::addCheckColumn (int property)
{
	GtkTreeView *view = GTK_TREE_VIEW (impl->view);
	GtkCellRenderer *renderer = gtk_cell_renderer_toggle_new();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (NULL,
		renderer, "active", property, "sensitive", IS_LOCKED_PROP, NULL);
	if (impl->colorModified)
		gtk_tree_view_column_add_attribute (column, renderer,
			"cell-background", BACKGROUND_PROP);
	if (property == INSTALLED_CHECK_PROP)
		gtk_tree_view_column_add_attribute (column, renderer,
			"activatable", CAN_TOGGLE_INSTALL_PROP);
	g_signal_connect (G_OBJECT (renderer), "toggled",
	                  G_CALLBACK (check_toggled_cb), this);

	// it seems like GtkCellRendererToggle has no width at start, so fixed doesn't work
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 25);
	set_sort_column (this, column, property);
	gtk_tree_view_append_column (view, column);
}

void YGtkPkgListView::addImageColumn (const char *header, int property, bool onlyManualModified)
{
	GtkTreeView *view = GTK_TREE_VIEW (impl->view);
	GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (
		header, renderer, "icon-name", property, NULL);
	if (impl->colorModified)
		gtk_tree_view_column_add_attribute (column, renderer,
			"cell-background", BACKGROUND_PROP);
	if (onlyManualModified)
		gtk_tree_view_column_add_attribute (column, renderer,
			"visible", MANUAL_MODIFY_PROP);

	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	int height = MAX (32, YGUtils::getCharsHeight (impl->view, 1));
	gtk_cell_renderer_set_fixed_size (renderer, -1, height);
	gtk_tree_view_column_set_fixed_width (column, 38);
	gtk_tree_view_append_column (view, column);
}

void YGtkPkgListView::addButtonColumn (const char *header, int property)
{
	GtkCellRenderer *renderer = ygtk_cell_renderer_button_new();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (
		header, renderer, "sensitive", IS_LOCKED_PROP, NULL);
	if (impl->colorModified)
		gtk_tree_view_column_add_attribute (column, renderer,
			"cell-background", BACKGROUND_PROP);

	gboolean show_icon;
	g_object_get (G_OBJECT (gtk_settings_get_default()), "gtk-button-images", &show_icon, NULL);

	const char *text;
	if (property == UNDO_BUTTON_PROP) {  // static property (always "Undo")
		text = _("Undo");
		g_object_set (G_OBJECT (renderer), "text", text, NULL);
		if (show_icon)
			g_object_set (G_OBJECT (renderer), "stock-id", GTK_STOCK_UNDO, NULL);
		gtk_tree_view_column_add_attribute (column, renderer,
			"visible", MANUAL_MODIFY_PROP);
		g_signal_connect (G_OBJECT (renderer), "toggled",
				          G_CALLBACK (undo_toggled_cb), this);
	}
	else {
		text = "xxxxxxxxxx";
		gtk_tree_view_column_add_attribute (column, renderer,
			"text", property);
		if (show_icon)
			gtk_tree_view_column_add_attribute (column, renderer,
				"stock-id", ACTION_ICON_PROP);
		g_signal_connect (G_OBJECT (renderer), "toggled",
				          G_CALLBACK (action_button_toggled_cb), this);
	}

	PangoRectangle rect;
	int width = 0;
	PangoLayout *layout = gtk_widget_create_pango_layout (impl->view,
		strlen (header) > strlen (text) ? header : text);
	pango_layout_get_pixel_extents (layout, NULL, &rect);
	width = MAX (width, rect.width);
	g_object_unref (G_OBJECT (layout));
	width += 18;
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

void YGtkPkgListView::setListener (YGtkPkgListView::Listener *listener)
{ impl->listener = listener; }

Ypp::List YGtkPkgListView::getList()
{ return impl->list; }

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

const char *getStatusAction (Ypp::Selectable *sel)
{
	const char *action = 0;
	if (sel->toInstall()) {
		action = _("install");
		if (sel->type() == Ypp::Selectable::PACKAGE) {
			if (sel->isInstalled()) {
				Ypp::Version candidate = sel->candidate(), installed = sel->installed();
				if (candidate > installed)
					action = _("upgrade");
				else if (candidate < installed)
					action = _("downgrade");
				else
					action = _("re-install");
			}
		}
	}
	else if (sel->toRemove())
		action = _("remove");
	else //if (sel->toModify())
		action = _("modify");  // generic for locked and so on
	return action;
}

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
		if (sel.hasUpgrade()) {
			text += " ";
			text += _("(upgrade available)");
		}
	}
	else
		text = _("Not installed");
	if (sel.toModifyAuto()) {
		text += "\n<i>";
		text += _("status changed by the dependencies resolver");
		text += "</i>";
	}
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
	url = repo.isSystem() ? _("Installed packages") : repo.url();
	str.reserve (name.size() + url.size() + 64);
	str = name + "\n";
	str += "<span color=\"" GRAY_COLOR "\">";
	str += "<small>" + url + "</small>";
	str += "</span>";
	return str;
}

const char *getRepositoryStockIcon (const std::string &url)
{
	if (url.empty())
		return GTK_STOCK_MISSING_IMAGE;
	if (url.compare (0, 2, "cd", 2) == 0 || url.compare (0, 3, "dvd", 3) == 0)
		return GTK_STOCK_CDROM;
	if (url.compare (0, 3, "iso", 3) == 0)
		return GTK_STOCK_FILE;
	if (url.find ("KDE") != std::string::npos)
		return "pattern-kde";
	if (url.find ("GNOME") != std::string::npos)
		return "pattern-gnome";
	if (url.find ("update") != std::string::npos)
		return "yast-update";
	if (url.find ("home") != std::string::npos)
		return "yast-users";
	return GTK_STOCK_NETWORK;
}

const char *getRepositoryStockIcon (Ypp::Repository &repo)
{
	if (repo.isSystem())
		return "yast-host";
	return getRepositoryStockIcon (repo.url());
}

void highlightMarkup (std::string &text, const std::list <std::string> &keywords,
                      const char *openTag, const char *closeTag, int openTagLen, int closeTagLen)
{
	if (keywords.empty()) return;
	text.reserve ((openTagLen + closeTagLen + 2) * 6);
	const char *i = text.c_str();
	while (*i) {
		std::list <std::string>::const_iterator it;
		for (it = keywords.begin(); it != keywords.end(); it++) {
			const std::string &keyword = *it;
			int len = keyword.size();
			if (strncasecmp (i, keyword.c_str(), len) == 0) {
				int pos = i - text.c_str();
				text.insert (pos+len, closeTag);
				text.insert (pos, openTag);
				i = text.c_str() + pos + len + openTagLen + closeTagLen - 2;
				break;
			}
		}
		if (it == keywords.end())
			i++;
	}
}

void highlightMarkupSpan (std::string &text, const std::list <std::string> &keywords)
{
	static const char openTag[] = "<span fgcolor=\"#000000\" bgcolor=\"#ffff00\">";
	static const char closeTag[] = "</span>";
	highlightMarkup (text, keywords, openTag, closeTag, sizeof (openTag), sizeof (closeTag));
}

