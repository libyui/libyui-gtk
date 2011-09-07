/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* YGtkPkgHistoryDialog, dialog */
// check the header file for information about this widget

/*
  Textdomain "gtk"
 */

#include "YGi18n.h"
#include "config.h"
#include "ygtkpkghistorydialog.h"
#include "YGDialog.h"
#include "YGUtils.h"
#include "YGPackageSelector.h"
#include "ygtkpkglistview.h"
#include <gtk/gtk.h>
#include "ygtktreeview.h"
#include "ygtkcellrenderertext.h"

#include <zypp/parser/HistoryLogReader.h>
#define FILENAME "/var/log/zypp/history"

static std::string reqbyTreatment (const std::string &reqby)
{
	if (reqby.empty())
		return _("automatic");
	if (reqby.compare (0, 4, "root", 4) == 0) {
		std::string str (_("user:"));
		str += " root";
		return str;
	}
	return reqby;
}

struct Handler
{
	virtual void date (const std::string &str, bool first) = 0;
	virtual void item (const std::string &action, const std::string &name,
		const std::string &description, const std::string &repositoryName,
		const std::string &repositoryUrl, const std::string &reqby, bool autoReq) = 0;
};

struct LogListHandler
{
	GtkListStore *store;

	enum Column { ICON_COLUMN, NAME_COLUMN, VERSION_URL_COLUMN, REPOSITORY_COLUMN,
		REQBY_COLUMN, REPOSITORY_ICON_COLUMN, REPOSITORY_URL_COLUMN, SHORTCUT_COLUMN,
		COLOR_COLUMN, XPAD_COLUMN, TOTAL_COLUMNS };

	LogListHandler()
	{
		store = gtk_list_store_new (TOTAL_COLUMNS, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
	}

	~LogListHandler()
	{ g_object_unref (G_OBJECT (store)); }

	GtkTreeModel *getModel()
	{ return GTK_TREE_MODEL (store); }

	void addRow (GtkTreeIter *iter)
	{
		gtk_list_store_append (store, iter);
		gtk_list_store_set (store, iter, ICON_COLUMN, NULL, NAME_COLUMN, NULL,
			VERSION_URL_COLUMN, NULL, REPOSITORY_COLUMN, NULL, REQBY_COLUMN, NULL,
			REPOSITORY_ICON_COLUMN, NULL, REPOSITORY_URL_COLUMN, NULL,
			SHORTCUT_COLUMN, NULL, COLOR_COLUMN, NULL, XPAD_COLUMN, 0, -1);
	}

	int date (const std::string &str, bool first)
	{
		GtkTreeIter iter;
		if (!first) addRow (&iter);  // white space
		std::string _date = std::string ("<b>\u26ab ") + str + "</b>";
		addRow (&iter);
		gtk_list_store_set (store, &iter, NAME_COLUMN, _date.c_str(), -1);

		GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
		int row = gtk_tree_path_get_indices (path)[0];
		gtk_tree_path_free (path);
		return row;
	}

	void item (const std::string &action, const std::string &name,
		const std::string &description, const std::string &repositoryName,
		const std::string &repositoryUrl, const std::string &reqby, bool autoReq)
	{
		GtkTreeIter iter;
		const char *icon = 0;
		std::string shortcut (name);
		if (action == _("install"))
			icon = GTK_STOCK_ADD;
		else if (action == _("upgrade"))
			icon = GTK_STOCK_GO_UP;
		else if (action == _("remove"))
			icon = GTK_STOCK_REMOVE;
		else if (action == _("downgrade"))
			icon = GTK_STOCK_GO_DOWN;
		else if (action == _("re-install"))
			icon = GTK_STOCK_REFRESH;
		else {
			icon = getRepositoryStockIcon (name);
			shortcut = "_repo";
		}

		std::string _name;
		_name.reserve (action.size() + name.size() + 64);
		_name = "<b>";
		_name += action + "</b> " + name;
		int xpad = 0; // autoReq ? 25 : 0;

		const char *repo_icon = 0, *color = 0;
		bool is_patch = false;
		if (action == _("upgrade") && !repositoryUrl.empty()) {
			// if 'upgrade' and from '*update*' server then mark as patch
			repo_icon = getRepositoryStockIcon (repositoryUrl);
			if (repositoryUrl.find ("update") != std::string::npos) {
				//color = "red";
				std::string tag;
				tag.reserve (64);
				tag = "<small><span color=\"#999999\">"; tag += _("patch"); tag += "</span></small>";
				_name += "   "; _name += tag;
				is_patch = true;
			}
		}
		if (autoReq && !is_patch) {  // dependency
			std::string tag;
			tag.reserve (64);
			tag = "<small><span color=\"#999999\">"; tag += _("auto"); tag += "</span></small>";
			_name += "   "; _name += tag;
		}

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, ICON_COLUMN, icon, NAME_COLUMN, _name.c_str(),
			VERSION_URL_COLUMN, description.c_str(), REPOSITORY_COLUMN, repositoryName.c_str(),
			REPOSITORY_ICON_COLUMN, repo_icon, REPOSITORY_URL_COLUMN, repositoryUrl.c_str(),
			REQBY_COLUMN, reqby.c_str(), SHORTCUT_COLUMN, shortcut.c_str(),
			COLOR_COLUMN, color, XPAD_COLUMN, xpad, -1);
	}
};

struct DateListHandler
{
	GtkListStore *store;

	enum Column { TEXT_COLUMN, LOG_ROW_COLUMN, TOTAL_COLUMNS };

	DateListHandler()
	{ store = gtk_list_store_new (TOTAL_COLUMNS, G_TYPE_STRING, G_TYPE_INT); }

	~DateListHandler()
	{ g_object_unref (G_OBJECT (store)); }

	GtkTreeModel *getModel()
	{ return GTK_TREE_MODEL (store); }

	void date (const std::string &str, bool first, int log_row)
	{
		GtkTreeIter iter;
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, TEXT_COLUMN, str.c_str(),
			LOG_ROW_COLUMN, log_row, -1);
	}
};

struct ListHandler : public Handler
{
	LogListHandler *log_handler;
	DateListHandler *date_handler;

	ListHandler()
	{
		log_handler = new LogListHandler();
		date_handler = new DateListHandler();
	}

	~ListHandler()
	{ delete log_handler; delete date_handler; }

	virtual void date (const std::string &str, bool first)
	{
		int row = log_handler->date (str, first);
		date_handler->date (str, first, row);
	}

	virtual void item (const std::string &a, const std::string &n,
	                    const std::string &d, const std::string &rn,
	                    const std::string &ru, const std::string &rb, bool ar)
	{ log_handler->item (a, n, d, rn, ru, rb, ar); }
};

struct FileHandler : public Handler
{
	FILE *file;

	FileHandler (const char *filename)
	{ file = fopen (filename, "w"); 	}

	~FileHandler()
	{ fclose (file); }

	void addSpace()
	{ (void) fwrite ("\n", sizeof (char), 1, file); }

	virtual void date (const std::string &str, bool first)
	{
		if (!first)
			addSpace();
		(void) fwrite (str.c_str(), sizeof (char), str.size(), file);
		addSpace(); addSpace();
	}

	virtual void item (const std::string &action, const std::string &name,
		const std::string &description, const std::string &repositoryName,
		const std::string &repositoryUrl, const std::string &reqby, bool autoReq)
	{
		std::string str;
		str.reserve (action.size() + name.size() + description.size() + 4);
		str = std::string ("\t") + action + " " + name + " " + description + "\n";
		(void) fwrite (str.c_str(), sizeof (char), str.size(), file);
	}
};

struct ZyppHistoryParser
{
	Handler *handler;
	std::string _date;
	std::map <std::string, zypp::Edition> installed;

	ZyppHistoryParser (Handler *handler)
	: handler (handler)
	{
		zypp::parser::HistoryLogReader parser (FILENAME, boost::ref (*this));
		try {
			parser.readAll();
		}
		catch (const zypp::Exception &ex) {
			yuiWarning () << "Error: Could not load log file" << FILENAME << ": "
				<< ex.asUserHistory() << std::endl;
		}
	}

	bool operator() (const zypp::HistoryItem::Ptr &item)
	{
		std::string date (item->date.form ("%d %B %Y"));
		if (_date != date) {
			handler->date (date, _date.empty());
			_date = date;
		}

		std::string action, name, descrpt, repoName, repoUrl, reqby, t;
		bool autoreq = false;
		switch (item->action.toEnum()) {
			case zypp::HistoryActionID::NONE_e:
				break;
			case zypp::HistoryActionID::INSTALL_e: {
				zypp::HistoryItemInstall *_item =
					static_cast <zypp::HistoryItemInstall *> (item.get());
				name = _item->name;
				descrpt = _item->edition.version();
				Ypp::getRepositoryFromAlias (_item->repoalias, repoName, repoUrl);
				reqby = _item->reqby; autoreq = reqby.empty();
				reqby = reqbyTreatment (reqby);
				zypp::Edition edition = _item->edition;
				std::map <std::string, zypp::Edition>::iterator it;
				it = installed.find (name);
				if (it == installed.end())
					action = _("install");
				else {
					zypp::Edition prev_edition = it->second;
					if (edition > prev_edition)
						action = _("upgrade");
					else if (edition < prev_edition)
						action = _("downgrade");
					else // (edition == prev_edition)
						action = _("re-install");
				}
				installed[name] = edition;
				break;
			}
			case zypp::HistoryActionID::REMOVE_e: {
				zypp::HistoryItemRemove *_item =
					static_cast <zypp::HistoryItemRemove *> (item.get());
				action = _("remove");
				name = _item->name;
				descrpt = _item->edition.version();
				reqby = _item->reqby; autoreq = reqby.empty();
				reqby = reqbyTreatment (reqby);
				std::map <std::string, zypp::Edition>::iterator it;
				it = installed.find (name);
				if (it != installed.end())
					installed.erase (it);
				break;
			}
			case zypp::HistoryActionID::REPO_ADD_e: {
				zypp::HistoryItemRepoAdd *_item =
					static_cast <zypp::HistoryItemRepoAdd *> (item.get());
				action = _("add repository");
				Ypp::getRepositoryFromAlias (_item->alias, name, t);
				descrpt = _item->url.asString();
				break;
			}
			case zypp::HistoryActionID::REPO_REMOVE_e: {
				zypp::HistoryItemRepoRemove *_item =
					static_cast <zypp::HistoryItemRepoRemove *> (item.get());
				action = _("remove repository");
				name = _item->alias;
				break;
			}
			case zypp::HistoryActionID::REPO_CHANGE_ALIAS_e: {
				zypp::HistoryItemRepoAliasChange *_item =
					static_cast <zypp::HistoryItemRepoAliasChange *> (item.get());
				action = _("change repository alias");
				name = _item->oldalias + " -> " + _item->newalias;
				break;
			}
			case zypp::HistoryActionID::REPO_CHANGE_URL_e: {
				zypp::HistoryItemRepoUrlChange *_item =
					static_cast <zypp::HistoryItemRepoUrlChange *> (item.get());
				action = _("change repository url");
				Ypp::getRepositoryFromAlias (_item->alias, name, t);
				descrpt = _item->newurl.asString();
				break;
			}
		}
		if (!action.empty())
			handler->item (action, name, descrpt, repoName, repoUrl, reqby, autoreq);
		return true;
	}
};

static void goto_clicked (GtkTreeView *log_view)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (log_view);
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		char *shortcut;
		gtk_tree_model_get (model, &iter, LogListHandler::SHORTCUT_COLUMN, &shortcut, -1);
		if (!strcmp (shortcut, "_repo"))
			YGPackageSelector::get()->showRepoManager();
		else
			YGPackageSelector::get()->searchFor (Ypp::PoolQuery::NAME, shortcut);
		g_free (shortcut);

		GtkWidget *dialog = gtk_widget_get_toplevel (GTK_WIDGET (log_view));
		gtk_widget_hide (dialog);
	}
}

static void log_selection_changed_cb (GtkTreeSelection *selection, GtkDialog *dialog)
{
	bool selected = gtk_tree_selection_count_selected_rows (selection) > 0;
	gtk_dialog_set_response_sensitive (dialog, 1, selected);
}

static void log_row_activated_cb (GtkTreeView *view, GtkTreePath *path,
	GtkTreeViewColumn *column)
{ goto_clicked (view); }

static gboolean log_can_select_cb (GtkTreeSelection *selection, GtkTreeModel *model,
	GtkTreePath *path, gboolean path_currently_selected, gpointer data)
{
	GtkTreeIter iter;
	gtk_tree_model_get_iter (model, &iter, path);
	gchar *shortcut;
	gtk_tree_model_get (model, &iter, LogListHandler::SHORTCUT_COLUMN, &shortcut, -1);
	bool can_select = shortcut != NULL;
	if (shortcut) g_free (shortcut);
	return can_select;
}

static void date_selection_changed_cb (GtkTreeSelection *selection, GtkTreeView *log_view)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		int log_row;
		gtk_tree_model_get (model, &iter, 1, &log_row, -1);

		GtkTreePath *path = gtk_tree_path_new_from_indices (log_row, -1);
		gtk_tree_view_scroll_to_cell (log_view, path, NULL, TRUE, 0, 0);
		gtk_tree_path_free (path);
	}
}

static void save_to_file (GtkWindow *parent)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new (_("Save to"), parent,
		GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), TRUE);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		FileHandler handler (filename);
		ZyppHistoryParser parser (&handler);
		g_free (filename);
	}
	gtk_widget_destroy (dialog);
}

static void response_cb (GtkDialog *dialog, gint response, GtkTreeView *log_view)
{
	switch (response) {
		case 1: goto_clicked (log_view); break;
		case 2: save_to_file (GTK_WINDOW (dialog)); break;
		default: gtk_widget_hide (GTK_WIDGET (dialog)); break;
	}
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
		gtk_tree_path_free (path);

		GtkTreeViewColumn *column;
		int bx, by;
		gtk_tree_view_convert_widget_to_bin_window_coords (
			view, x, y, &bx, &by);
		gtk_tree_view_get_path_at_pos (
			view, x, y, NULL, &column, NULL, NULL);

		std::string text;
		text.reserve (254);
		const char *icon = 0;

		if (column == ygtk_tree_view_get_column (YGTK_TREE_VIEW (view), 2)) {  // repository
			char *name, *url;
			gtk_tree_model_get (model, &iter, LogListHandler::REPOSITORY_COLUMN, &name,
				LogListHandler::REPOSITORY_URL_COLUMN, &url, -1);
			if (name && *name) {
				text = name;
				if (url && *url) {
					text += "\n<small>"; text += url; text += "</small>";
					icon = getRepositoryStockIcon (url);
				}
				if (url)
					g_free (url);
			}
			if (name)
				g_free (name);
		}

		if (!text.empty()) {
			gtk_tooltip_set_markup (tooltip, text.c_str());
			gtk_tooltip_set_icon_from_icon_name (tooltip,
				icon, GTK_ICON_SIZE_BUTTON);
			return TRUE;
		}
	}
	return FALSE;
}

static void goto_activate_cb (GtkMenuItem *item, GtkTreeView *view)
{ goto_clicked (view); }

static void right_click_cb (YGtkTreeView *view, gboolean outreach)
{
	GtkWidget *menu = gtk_menu_new();
	if (!outreach) {
		GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
		bool selected = gtk_tree_selection_get_selected (selection, NULL, NULL);

		GtkWidget *item = gtk_image_menu_item_new_from_stock (GTK_STOCK_JUMP_TO, NULL);
		gtk_widget_set_sensitive (item, selected);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (goto_activate_cb), view);
	}

	ygtk_tree_view_append_show_columns_item (view, menu);
	ygtk_tree_view_popup_menu (view, menu);
}

static gboolean read_logs_idle_cb (void *data)
{
	GtkWidget **views = (GtkWidget **) data;
	GtkWidget *dialog = views[0];
	GtkTreeView *log_view = GTK_TREE_VIEW (views[1]);
	GtkTreeView *date_view = GTK_TREE_VIEW (views[2]);

	ListHandler handler;
	ZyppHistoryParser parser (&handler);

	gtk_tree_view_set_model (date_view, handler.date_handler->getModel());
	gtk_tree_view_set_model (log_view, handler.log_handler->getModel());

	GtkTreeSelection *selection = gtk_tree_view_get_selection (date_view);
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter_first (
	    gtk_tree_view_get_model (date_view), &iter))
		gtk_tree_selection_select_iter (selection, &iter);

	gdk_window_set_cursor (gtk_widget_get_window(GTK_WIDGET(dialog)), NULL);
	return FALSE;
}

YGtkPkgHistoryDialog::YGtkPkgHistoryDialog()
{
	GtkCellRenderer *renderer, *pix_renderer;
	GtkTreeViewColumn *column;

	GtkWidget *log_view = ygtk_tree_view_new (_("No entries."));
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (log_view), LogListHandler::SHORTCUT_COLUMN);
	gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (log_view), TRUE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (log_view), TRUE);
	gtk_widget_set_has_tooltip (log_view, TRUE);
	g_signal_connect (G_OBJECT (log_view), "query-tooltip",
	                  G_CALLBACK (query_tooltip_cb), this);
	g_signal_connect (G_OBJECT (log_view), "right-click",
	                  G_CALLBACK (right_click_cb), this);

	bool reverse = gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title (column, _("Action"));
	gtk_tree_view_column_set_spacing (column, 6);

	pix_renderer = gtk_cell_renderer_pixbuf_new();
	if (!reverse)
		gtk_tree_view_column_pack_start (column, pix_renderer, FALSE);

	renderer = ygtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
		"markup", LogListHandler::NAME_COLUMN, "xpad", LogListHandler::XPAD_COLUMN,
		"foreground", LogListHandler::COLOR_COLUMN, NULL);
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);

	if (reverse)
		gtk_tree_view_column_pack_start (column, pix_renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, pix_renderer,
		"icon-name", LogListHandler::ICON_COLUMN, NULL);

	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_expand (column, TRUE);
	ygtk_tree_view_append_column (YGTK_TREE_VIEW (log_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (_("Version / URL"), renderer,
		"text", LogListHandler::VERSION_URL_COLUMN, "xpad", LogListHandler::XPAD_COLUMN,
		"foreground", LogListHandler::COLOR_COLUMN, NULL);
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 120);
	ygtk_tree_view_append_column (YGTK_TREE_VIEW (log_view), column);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title (column, _("Repository"));
	gtk_tree_view_column_set_spacing (column, 2);

	pix_renderer = gtk_cell_renderer_pixbuf_new();
	if (!reverse)
		gtk_tree_view_column_pack_start (column, pix_renderer, FALSE);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
		"text", LogListHandler::REPOSITORY_COLUMN, NULL);

	if (reverse)
		gtk_tree_view_column_pack_start (column, pix_renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, pix_renderer,
		"icon-name", LogListHandler::REPOSITORY_ICON_COLUMN, NULL);

	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 140);
	gtk_tree_view_column_set_visible (column, FALSE);
	ygtk_tree_view_append_column (YGTK_TREE_VIEW (log_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (_("Requested by"), renderer,
		"text", LogListHandler::REQBY_COLUMN, NULL);
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 100);
	gtk_tree_view_column_set_visible (column, FALSE);
	ygtk_tree_view_append_column (YGTK_TREE_VIEW (log_view), column);

	GtkWidget *log_scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (log_scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (
		GTK_SCROLLED_WINDOW (log_scroll), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (log_scroll), log_view);

	GtkWidget *date_view = gtk_tree_view_new();
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (date_view), DateListHandler::TEXT_COLUMN);
	gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (date_view), TRUE);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (_("Date"), renderer,
		"text", DateListHandler::TEXT_COLUMN, NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column (GTK_TREE_VIEW (date_view), column);

	GtkWidget *date_scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (date_scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (
		GTK_SCROLLED_WINDOW (date_scroll), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (date_scroll), date_view);

	GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
		GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_OTHER, GTK_BUTTONS_NONE,
		_("Show History (%s)"), FILENAME);
	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_JUMP_TO, 1);
	gtk_dialog_add_button (GTK_DIALOG (dialog), _("Save to File"), 2);
	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), 1, FALSE);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
	gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 650, 600);
	g_signal_connect (G_OBJECT (dialog), "response",
	                  G_CALLBACK (response_cb), log_view);
	g_signal_connect (G_OBJECT (dialog), "delete-event",
	                  G_CALLBACK (gtk_true), log_view);

	GtkWidget *hpaned = gtk_hpaned_new();
	gtk_paned_pack1 (GTK_PANED (hpaned), date_scroll, FALSE, FALSE);
	gtk_paned_pack2 (GTK_PANED (hpaned), log_scroll, TRUE, FALSE);
	YGUtils::setPaneRelPosition (hpaned, .30);
	gtk_widget_set_vexpand (hpaned, TRUE);
	GtkContainer *content = GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog)));
	gtk_container_add (content, hpaned);

	gtk_widget_show_all (dialog);
	m_dialog = dialog;

	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (log_view));
	g_signal_connect (G_OBJECT (selection), "changed",
	                  G_CALLBACK (log_selection_changed_cb), dialog);
	g_signal_connect (G_OBJECT (log_view), "row-activated",
	                  G_CALLBACK (log_row_activated_cb), NULL);
	gtk_tree_selection_set_select_function (selection, log_can_select_cb, NULL, NULL);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (date_view));
	g_signal_connect (G_OBJECT (selection), "changed",
	                  G_CALLBACK (date_selection_changed_cb), log_view);

	GdkCursor *cursor = gdk_cursor_new (GDK_WATCH);
	gdk_window_set_cursor (gtk_widget_get_window(GTK_WIDGET(dialog)), cursor);
	gdk_cursor_unref (cursor);

	GtkWidget **views = g_new (GtkWidget *, 3);
	views[0] = dialog; views[1] = log_view; views[2] = date_view;
	g_idle_add_full (G_PRIORITY_LOW, read_logs_idle_cb, views, g_free);
}

YGtkPkgHistoryDialog::~YGtkPkgHistoryDialog()
{ gtk_widget_destroy (m_dialog); }

void YGtkPkgHistoryDialog::popup()
{ gtk_window_present (GTK_WINDOW (m_dialog)); }

