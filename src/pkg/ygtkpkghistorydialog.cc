/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* Textdomain "yast2-gtk" */
/* YGtkPkgMenuBar, menu bar */
// check the header file for information about this widget

#include "YGi18n.h"
#include "config.h"
#include "ygtkpkghistorydialog.h"
#include "YGDialog.h"
#include "YGPackageSelector.h"
#include "ygtkpkglistview.h"
#include "ygtkcellrenderertextpixbuf.h"
#include <gtk/gtk.h>
#include "ygtktreeview.h"

#include <zypp/parser/HistoryLogReader.h>
#define FILENAME "/var/log/zypp/history"

static void getRepositoryInfo (const std::string &alias, std::string &name, std::string &url)
{
	static std::map <std::string, zypp::RepoInfo> repos;
	if (repos.empty()) {
		zypp::RepoManager manager;
		std::list <zypp::RepoInfo> known_repos = manager.knownRepositories();
		for (std::list <zypp::RepoInfo>::const_iterator it = known_repos.begin();
			 it != known_repos.end(); it++)
			repos[it->alias()] = *it;
	}

	std::map <std::string, zypp::RepoInfo>::iterator it = repos.find (alias);
	if (it != repos.end()) {
		zypp::RepoInfo *repo = &it->second;
		name = repo->name();
		url = repo->url().asString();
	}
	else
		name = alias;  // return alias if repo not currently setup-ed
}

static std::string reqbyTreatment (const std::string &reqby)
{
	if (reqby.empty())
		return _("automatic");
	if (reqby.compare (0, 4, "root", 4) == 0)
		return _("user: root");
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
		std::string shortcut = name;
		if (action == _("install"))
			icon = GTK_STOCK_ADD;
		else if (action == _("upgrade"))
			icon = GTK_STOCK_GO_UP;
		else if (action == _("remove"))
			icon = GTK_STOCK_REMOVE;
		else {
			icon = getRepositoryStockIcon (name);
			shortcut = "_repo";
		}

		std::string _name;
		_name.reserve (action.size() + name.size() + 10);
		_name = std::string ("<b>") + action + "</b> " + name;
		int xpad = 0; // autoReq ? 25 : 0;

		const char *repo_icon = 0, *color = 0;
		if (!repositoryUrl.empty()) {
			repo_icon = getRepositoryStockIcon (repositoryUrl);
			if (repositoryUrl.find ("update") != std::string::npos) {
				//color = "red";
				_name += _("  <i>(patch)</i>");
			}
		}
		if (autoReq)
			_name += _("  <i>(auto)</i>");

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
	{ fwrite ("\n", sizeof (char), 1, file); }

	virtual void date (const std::string &str, bool first)
	{
		if (!first)
			addSpace();
		fwrite (str.c_str(), sizeof (char), str.size(), file);
		addSpace(); addSpace();
	}

	virtual void item (const std::string &action, const std::string &name,
		const std::string &description, const std::string &repositoryName,
		const std::string &repositoryUrl, const std::string &reqby, bool autoReq)
	{
		std::string str;
		str.reserve (action.size() + name.size() + description.size() + 4);
		str = std::string ("\t") + action + " " + name + " " + description + "\n";
		fwrite (str.c_str(), sizeof (char), str.size(), file);
	}
};

struct ZyppHistoryParser
{
	Handler *handler;
	std::string _date;
	std::map <std::string, int> installed;

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
				getRepositoryInfo (_item->repoalias, repoName, repoUrl);
				reqby = _item->reqby; autoreq = reqby.empty(); reqby = reqbyTreatment (reqby);
				if (installed.find (name) != installed.end())
					action = _("upgrade");
				else {
					action = _("install");
					installed[name] = 0;
				}
				break;
			}
			case zypp::HistoryActionID::REMOVE_e: {
				zypp::HistoryItemRemove *_item =
					static_cast <zypp::HistoryItemRemove *> (item.get());
				action = _("remove");
				name = _item->name;
				descrpt = _item->edition.version();
				reqby = _item->reqby; autoreq = reqby.empty(); reqby = reqbyTreatment (reqby);
				installed.erase (installed.find (name));
				break;
			}
			case zypp::HistoryActionID::REPO_ADD_e: {
				zypp::HistoryItemRepoAdd *_item =
					static_cast <zypp::HistoryItemRepoAdd *> (item.get());
				action = _("add repository");
				getRepositoryInfo (_item->alias, name, t);
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
				getRepositoryInfo (_item->alias, name, t);
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
	}

	GtkWidget *dialog = gtk_widget_get_toplevel (GTK_WIDGET (log_view));
	gtk_widget_destroy (dialog);
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
	GtkWidget *dialog = gtk_file_chooser_dialog_new (_("Save logs to"), parent,
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
		default: gtk_widget_destroy (GTK_WIDGET (dialog)); break;
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

		int col;
		for (col = 0; gtk_tree_view_get_column (view, col); col++)
			if (column == gtk_tree_view_get_column (view, col))
				break;
/*
		char *color;
		gtk_tree_model_get (model, &iter, LogListHandler::COLOR_COLUMN, &color, -1);
		bool has_color = color != NULL;
		g_free (color);
*/
		switch (col) {
			case LogListHandler::REPOSITORY_COLUMN: {
				char *name, *url;
				gtk_tree_model_get (model, &iter,
					LogListHandler::REPOSITORY_COLUMN, &name,
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
				break;
			}
			case LogListHandler::NAME_COLUMN:
			case LogListHandler::VERSION_URL_COLUMN:
			case LogListHandler::REQBY_COLUMN: {
				char *str;
				gtk_tree_model_get (model, &iter, col, &str, -1);
				if (str && *str) {
					text = str;
/*					if (has_color && col != LogListHandler::REQBY_COLUMN)
						text += _(" (patch)");*/
				}
				if (str)
					g_free (str);
				break;
			}
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

static void right_click_cb (YGtkTreeView *view, gboolean outreach)
{
	GtkWidget *menu = ygtk_tree_view_append_show_columns_item (view, NULL);
	ygtk_tree_view_popup_menu (view, menu);
}

YGtkPkgHistoryDialog::YGtkPkgHistoryDialog()
{
	ListHandler handler;
	ZyppHistoryParser parser (&handler);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	GtkWidget *log_view = ygtk_tree_view_new();
	gtk_tree_view_set_model (GTK_TREE_VIEW (log_view), handler.log_handler->getModel());
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (log_view), LogListHandler::SHORTCUT_COLUMN);
	gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (log_view), TRUE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (log_view), TRUE);
	gtk_widget_set_has_tooltip (log_view, TRUE);
	g_signal_connect (G_OBJECT (log_view), "query-tooltip",
	                  G_CALLBACK (query_tooltip_cb), this);
	g_signal_connect (G_OBJECT (log_view), "right-click",
	                  G_CALLBACK (right_click_cb), this);

	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes (NULL, renderer,
		"icon-name", LogListHandler::ICON_COLUMN, NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (column, 38);
	ygtk_tree_view_append_column (YGTK_TREE_VIEW (log_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (_("Action"), renderer,
		"markup", LogListHandler::NAME_COLUMN, "xpad", LogListHandler::XPAD_COLUMN,
		"foreground", LogListHandler::COLOR_COLUMN, NULL);
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
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
	gtk_tree_view_column_set_fixed_width (column, 100);
	ygtk_tree_view_append_column (YGTK_TREE_VIEW (log_view), column);

	renderer = ygtk_cell_renderer_text_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes (_("Repository"), renderer,
		"text", LogListHandler::REPOSITORY_COLUMN,
		"icon-name", LogListHandler::REPOSITORY_ICON_COLUMN, NULL);
	g_object_set (G_OBJECT (renderer), "size", 16, NULL);
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

	GtkWidget *date_view = gtk_tree_view_new_with_model (handler.date_handler->getModel());
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

	std::string title (_("History of Changes"));
	title += " ("; title += FILENAME; title += ")";
	GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
		GtkDialogFlags (0), GTK_MESSAGE_OTHER, GTK_BUTTONS_NONE, title.c_str());
	gtk_dialog_add_button (GTK_DIALOG (dialog), _("Save to File"), 2);
	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_JUMP_TO, 1);
	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), 1, FALSE);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
	gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 650, 600);
	g_signal_connect (G_OBJECT (dialog), "response",
	                  G_CALLBACK (response_cb), log_view);

	GtkWidget *hpaned = gtk_hpaned_new();
	gtk_paned_pack1 (GTK_PANED (hpaned), date_scroll, FALSE, FALSE);
	gtk_paned_pack2 (GTK_PANED (hpaned), log_scroll, TRUE, FALSE);
	gtk_paned_set_position (GTK_PANED (hpaned), 180);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hpaned);

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
}

YGtkPkgHistoryDialog::~YGtkPkgHistoryDialog() {}

void YGtkPkgHistoryDialog::popup()
{ gtk_widget_show (m_dialog); }

