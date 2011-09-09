/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/*
  Textdomain "gtk"
 */

#include "YGi18n.h"
#include "config.h"
#include <string.h>
#include "YGUtils.h"
#include "YGDialog.h"
#include "YGUI.h"
#include "YGPackageSelector.h"
#include "yzyppwrapper.h"
#include "ygtkwizard.h"
#include "ygtkhtmlwrap.h"
#include "ygtkpkglistview.h"
#include "ygtkpkgsearchentry.h"
#include "ygtkpkgfilterview.h"
#include "ygtkpkgrpmgroupsview.h"
#include "ygtkpkgquerycombo.h"
#include "ygtkpkgpatternview.h"
#include "ygtkpkglanguageview.h"
#include "ygtkpkgundolist.h"
#include "ygtkpkgmenubar.h"
#include "ygtkpkgstatusbar.h"
#include "ygtkpkgdetailview.h"

#include "ygtkpkghistorydialog.h"
#ifdef HAS_VESTIGIAL_DIALOG
#include "ygtkpkgvestigialdialog.h"
#endif

//#define USE_LIST_BUTTONS

YGPackageSelector *YGPackageSelector::singleton = 0;

struct YGPackageSelector::Impl :
	public Ypp::Interface, YGtkPkgListView::Listener, YGtkPkgQueryWidget::Listener,
		YGtkPkgQueryCombo::Factory
{
GtkWidget *m_widget, *m_toolbox;
YGtkPkgListView *m_list;
YGtkPkgQueryCombo *m_combo;
YGtkPkgSearchEntry *m_entry;
std::list <YGtkPkgQueryWidget *> m_queryWidgets;
YGtkPkgUndoList *m_undo;
YGtkPkgStatusBar *m_status;
YGtkPkgMenuBar *m_menu;
std::list <std::string> m_filterSuffices;
GtkWidget *m_overview;
YGtkPkgDetailView *m_details;
guint m_refresh_id;
Ypp::List m_refresh_list;
bool has_patterns_mode;

struct SuffixFilter : public Ypp::Match {
	SuffixFilter (Impl *pThis) : pThis (pThis) {}

	virtual bool match (Ypp::Selectable &sel)
	{
		if (!pThis->m_filterSuffices.empty()) {
			std::string name (sel.name());
			for (std::list <std::string>::iterator it = pThis->m_filterSuffices.begin();
				 it != pThis->m_filterSuffices.end(); it++)
				if (YGUtils::endsWith (name, *it))
					return false;
		}
		return true;
	}

	Impl *pThis;
};

	GtkWidget *getWidget() { return m_widget; }

	GtkWidget *createMainArea()
	{
		bool onlineUpdate = YGPackageSelector::get()->onlineUpdateMode();

		m_entry = new YGtkPkgSearchEntry();
		m_queryWidgets.push_back (m_entry);

		m_list = new YGtkPkgListView (false, Ypp::List::NAME_SORT, false, true);
#ifdef USE_LIST_BUTTONS
		m_list->addImageColumn (NULL, STATUS_ICON_PROP);
#else
		m_list->addCheckColumn (INSTALLED_CHECK_PROP);
#endif
		m_list->addTextColumn (_("Name"), NAME_SUMMARY_PROP, true, -1);
		m_list->addTextColumn (_("Version"), VERSION_PROP, true, 125);
		if (!onlineUpdate)
			m_list->addTextColumn (_("Size"), SIZE_PROP, false, 85);
#ifdef USE_LIST_BUTTONS
		m_list->addButtonColumn (_("Action"), ACTION_BUTTON_PROP);
#endif
		m_list->addTextColumn (_("Repository"), REPOSITORY_PROP, false, 180);
		if (!onlineUpdate)
			m_list->addTextColumn (_("Supportability"), SUPPORT_PROP, false, 120);
		m_list->setListener (this);
		m_entry->setActivateWidget (m_list->getView());

		GtkWidget *hbox = gtk_hbox_new (FALSE, 2);
		GtkWidget *label = gtk_label_new_with_mnemonic (_("Package _listing:"));
		gtk_label_set_mnemonic_widget (GTK_LABEL (label), m_list->getView());
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), gtk_event_box_new(), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), m_entry->getWidget(), FALSE, TRUE, 0);
		m_toolbox = gtk_hbox_new (FALSE, 6);

		GtkWidget *overview = ygtk_html_wrap_new();
		if (onlineUpdate)
			ygtk_html_wrap_set_text (overview,
				"<h1><img src=\"gtk-dialog-info\" />Overview</h1>"
				"<p>Press a patch in the list to see more information about it.</p>"
				"<p>To install a patch, just click on its \"checkbox\".</p>", FALSE);
		else
			ygtk_html_wrap_set_text (overview,
				"<h1><img src=\"gtk-dialog-info\" />Overview</h1>"
				"<p>Browse packages using the groups list on the left.</p>"
				"<p>Press a package in the list to see more information about it.</p>"
				"<p>To install or remove a package, just click on its \"checkbox\".</p>", FALSE);

		m_overview = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (m_overview),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (m_overview),
			GTK_SHADOW_IN);
		gtk_container_add (GTK_CONTAINER (m_overview), overview);
		GtkWidget *text = gtk_event_box_new();
		gtk_container_add (GTK_CONTAINER (text), m_overview);

		GtkWidget *list_vbox = gtk_vbox_new (FALSE, 4);
		gtk_box_pack_start (GTK_BOX (list_vbox), m_list->getWidget(), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (list_vbox), m_toolbox, FALSE, TRUE, 0);

		GtkWidget *vpaned = gtk_vpaned_new();
		gtk_paned_pack1 (GTK_PANED (vpaned), list_vbox, TRUE, FALSE);
		gtk_paned_pack2 (GTK_PANED (vpaned), text, FALSE, TRUE);
		YGUtils::setPaneRelPosition (vpaned, .65);

		GtkWidget *_vbox = gtk_vbox_new (FALSE, 0);
		gtk_box_pack_start (GTK_BOX (_vbox), hbox, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (_vbox), vpaned, TRUE, TRUE, 0);
		return _vbox;
	}

	GtkWidget *createSidebar()
	{
		m_combo = new YGtkPkgQueryCombo (this);
		if (YGPackageSelector::get()->onlineUpdateMode()) {
			m_combo->add (_("Priorities"));
			m_combo->add (_("Repositories"));

			int active = YGPackageSelector::get()->repoMode() ? 1 : 0;
			m_combo->setActive (active);
		}
		else {
			m_combo->add (_("Groups"));
			m_combo->add (_("RPM Groups"));
			m_combo->add (_("Repositories"));
			m_combo->add (_("Support"));
			m_combo->add ("");
			has_patterns_mode = !isPatternsPoolEmpty();
			if (has_patterns_mode)
				m_combo->add (_("Patterns"));
			m_combo->add (_("Languages"));

			int active = 5;
			if (YGPackageSelector::get()->repoMode())
				active = 2;
			else if (YGPackageSelector::get()->searchMode())
				active = 0;
			m_combo->setActive (active);
		}
		m_queryWidgets.push_back (m_combo);

		YGtkPkgFilterView *status = new YGtkPkgFilterView (new YGtkPkgStatusModel());
		if (YGPackageSelector::get()->updateMode())
			status->select (3);
		m_queryWidgets.push_back (status);

		GtkWidget *vpaned = gtk_vpaned_new();
		gtk_paned_pack1 (GTK_PANED (vpaned), m_combo->getWidget(), TRUE, FALSE);
		gtk_paned_pack2 (GTK_PANED (vpaned), status->getWidget(), FALSE, FALSE);
		YGUtils::setPaneRelPosition (vpaned, .80);
		return vpaned;
	}

	Impl() : m_menu (NULL), m_details (NULL), m_refresh_id (0), m_refresh_list (0)
	{
		Ypp::init();
		m_undo = new YGtkPkgUndoList();
		m_status = 0;

		GtkWidget *hpaned = gtk_hpaned_new();
		gtk_paned_pack1 (GTK_PANED (hpaned), createSidebar(), FALSE, TRUE);
		gtk_paned_pack2 (GTK_PANED (hpaned), createMainArea(), TRUE, FALSE);
		YGUtils::setPaneRelPosition (hpaned, .28);

		m_widget = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (m_widget), hpaned, TRUE, TRUE, 0);
		if (!YGPackageSelector::get()->onlineUpdateMode()) {
			m_status = new YGtkPkgStatusBar (m_undo);
			gtk_box_pack_start (GTK_BOX (m_widget), m_status->getWidget(), FALSE, TRUE, 0);
		}
		gtk_widget_show_all (m_widget);
		gtk_widget_hide (m_toolbox);

		for (std::list <YGtkPkgQueryWidget *>::iterator it = m_queryWidgets.begin();
		     it != m_queryWidgets.end(); it++)
			(*it)->setListener (this);
	}

	~Impl()
	{
		if (m_refresh_id)
			g_source_remove (m_refresh_id);
		for (std::list <YGtkPkgQueryWidget *>::iterator it = m_queryWidgets.begin();
		     it != m_queryWidgets.end(); it++)
			delete *it;
		delete m_list;
		delete m_menu;
		delete m_status;
		delete m_details;
		delete m_undo;
		Ypp::finish();
	}

	void clearQueryWidgets()
	{
		for (std::list <YGtkPkgQueryWidget *>::iterator it = m_queryWidgets.begin();
		     it != m_queryWidgets.end(); it++)
			(*it)->clearSelection();
	}

	// Ypp::Interface

	static bool acceptText (Ypp::Selectable &selectable, const std::string &title,
		const std::string &open, const std::string &text)
	{
		GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
			(GtkDialogFlags) 0, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
			"%s %s", selectable.name().c_str(), title.c_str());
		if (!open.empty())
			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
				"%s", open.c_str());
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

		GtkWidget *view = ygtk_html_wrap_new(), *scroll;
		ygtk_html_wrap_set_text (view, text.c_str(), FALSE);

		scroll = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				                        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type
			(GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_IN);
		gtk_container_add (GTK_CONTAINER (scroll), view);

		GtkBox *content = GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG(dialog)));
		gtk_box_pack_start (content, scroll, TRUE, TRUE, 6);

		gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
		gtk_window_set_default_size (GTK_WINDOW (dialog), 550, 500);
		gtk_widget_show_all (dialog);

		gint ret = gtk_dialog_run (GTK_DIALOG (dialog));
		bool confirmed = (ret == GTK_RESPONSE_YES);
		gtk_widget_destroy (dialog);
		return confirmed;
	}

	virtual bool showLicense (Ypp::Selectable &sel, const std::string &license)
	{
		return acceptText (sel, _("License Agreement"),
			_("Do you accept the terms of this license?"), license);
	}

	virtual bool showMessage (Ypp::Selectable &sel, const std::string &message)
	{ return acceptText (sel, _("Warning Message"), _("Install anyway?"), message); }

	virtual bool resolveProblems (const std::list <Ypp::Problem *> &problems)
	{
		// we can't use ordinary radio buttons, as gtk+ enforces that in a group
		// one must be selected...
		#define DETAILS_PAD  25
		enum ColumnAlias {
			SHOW_TOGGLE_COL, ACTIVE_TOGGLE_COL, TEXT_COL, WEIGHT_TEXT_COL,
			TEXT_PAD_COL, APPLY_PTR_COL, TOTAL_COLS
		};

		struct inner {
			static void solution_toggled (GtkTreeModel *model, GtkTreePath *path)
			{
				GtkTreeStore *store = GTK_TREE_STORE (model);
				GtkTreeIter iter, parent;

				gboolean enabled;
				bool *apply;
				gtk_tree_model_get_iter (model, &iter, path);
				gtk_tree_model_get (model, &iter, ACTIVE_TOGGLE_COL, &enabled,
					                APPLY_PTR_COL, &apply, -1);
				if (!apply)
					return;

				// disable all the other radios on the group, setting current
				gtk_tree_model_get_iter (model, &iter, path);
				if (gtk_tree_model_iter_parent (model, &parent, &iter)) {
					gtk_tree_model_iter_children (model, &iter, &parent);
					do {
						gtk_tree_store_set (store, &iter, ACTIVE_TOGGLE_COL, FALSE, -1);
						bool *apply;
						gtk_tree_model_get (model, &iter, APPLY_PTR_COL, &apply, -1);
						if (apply) *apply = false;
					} while (gtk_tree_model_iter_next (model, &iter));
				}

				enabled = !enabled;
				*apply = enabled;
				gtk_tree_model_get_iter (model, &iter, path);
				gtk_tree_store_set (store, &iter, ACTIVE_TOGGLE_COL, enabled, -1);
			}
			static void cursor_changed_cb (GtkTreeView *view, GtkTreeModel *model)
			{
				GtkTreePath *path;
				gtk_tree_view_get_cursor (view, &path, NULL);
				solution_toggled (model, path);
				gtk_tree_path_free (path);
			}
		};

		// model
		GtkTreeStore *store = gtk_tree_store_new (TOTAL_COLS,
			G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_INT,
			G_TYPE_INT, G_TYPE_POINTER);
		for (std::list <Ypp::Problem *>::const_iterator it = problems.begin();
			 it != problems.end(); it++) {
			GtkTreeIter problem_iter;
			gtk_tree_store_append (store, &problem_iter, NULL);
			gtk_tree_store_set (store, &problem_iter, SHOW_TOGGLE_COL, FALSE,
				TEXT_COL, (*it)->description.c_str(), WEIGHT_TEXT_COL, PANGO_WEIGHT_BOLD, -1);
			if (!(*it)->details.empty()) {
				GtkTreeIter details_iter;
				gtk_tree_store_append (store, &details_iter, &problem_iter);
				gtk_tree_store_set (store, &details_iter, SHOW_TOGGLE_COL, FALSE,
					TEXT_COL, (*it)->details.c_str(), TEXT_PAD_COL, DETAILS_PAD, -1);
			}

			for (int i = 0; (*it)->getSolution (i); i++) {
				Ypp::Problem::Solution *solution = (*it)->getSolution (i);
				GtkTreeIter solution_iter;
				gtk_tree_store_append (store, &solution_iter, &problem_iter);
				gtk_tree_store_set (store, &solution_iter, SHOW_TOGGLE_COL, TRUE,
					WEIGHT_TEXT_COL, PANGO_WEIGHT_NORMAL,
					ACTIVE_TOGGLE_COL, FALSE, TEXT_COL, solution->description.c_str(),
					APPLY_PTR_COL, &solution->apply, -1);
				if (!solution->details.empty()) {
					gtk_tree_store_append (store, &solution_iter, &problem_iter);
					gtk_tree_store_set (store, &solution_iter, SHOW_TOGGLE_COL, FALSE,
						WEIGHT_TEXT_COL, PANGO_WEIGHT_NORMAL,
						TEXT_COL, solution->details.c_str(), TEXT_PAD_COL, DETAILS_PAD, -1);
				}
			}
		}

		// interface
		GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
			GtkDialogFlags (0), GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, "%s",
			_("There are some conflicts on the transaction that must be solved manually."));
		gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_APPLY, GTK_RESPONSE_APPLY, NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_APPLY);

		GtkWidget *view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
		g_object_unref (G_OBJECT (store));
		gtk_tree_selection_set_mode (gtk_tree_view_get_selection (
			GTK_TREE_VIEW (view)), GTK_SELECTION_NONE);
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
		gtk_tree_view_set_search_column (GTK_TREE_VIEW (view), TEXT_COL);

		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;
		column = gtk_tree_view_column_new();

		renderer = gtk_cell_renderer_toggle_new();
		gtk_tree_view_column_pack_start (column, renderer, FALSE);
		gtk_tree_view_column_set_attributes (column, renderer,
			"visible", SHOW_TOGGLE_COL, "active", ACTIVE_TOGGLE_COL, NULL);
		gtk_cell_renderer_toggle_set_radio (
			GTK_CELL_RENDERER_TOGGLE (renderer), TRUE);
		// we should not connect the actual toggle button, as we toggle on row press
		g_signal_connect (G_OBJECT (view), "cursor-changed",
			G_CALLBACK (inner::cursor_changed_cb), store);

		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start (column, renderer, TRUE);
		gtk_tree_view_column_set_attributes (column, renderer,
			"text", TEXT_COL, "weight", WEIGHT_TEXT_COL, "xpad", TEXT_PAD_COL, NULL);
/*		g_object_set (G_OBJECT (renderer), "wrap-width", 400,
			"wrap-mode", PANGO_WRAP_WORD, NULL);*/

		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
		gtk_tree_view_expand_all (GTK_TREE_VIEW (view));
		gtk_widget_set_has_tooltip (view, TRUE);

		GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
			GTK_SHADOW_IN);
		gtk_container_add (GTK_CONTAINER (scroll), view);

		GtkBox *content = GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG(dialog)));
		gtk_box_pack_start (content, scroll, TRUE, TRUE, 6);

		gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
		gtk_window_set_default_size (GTK_WINDOW (dialog), -1, 500);
		gtk_widget_show_all (dialog);

		bool apply = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_APPLY);
		gtk_widget_destroy (dialog);
		return apply;
	}

	// YGtkPkgListView::Listener

	virtual void selectionChanged()
	{
		Ypp::List selected = m_list->getSelected();
		if (selected.size() == 0) return;

		if (m_overview) {
			GtkWidget *parent = gtk_widget_get_parent (m_overview);
			gtk_container_remove (GTK_CONTAINER (parent), m_overview);
			m_overview = NULL;

			m_details = new YGtkPkgDetailView();
			gtk_container_add (GTK_CONTAINER (parent), m_details->getWidget());
		}

		m_details->setList (selected);
	}

	// YGtkPkgQueryWidget::Listener

	void refreshQueryWidget (YGtkPkgQueryWidget *widget)
	{
		Ypp::Selectable::Type type = Ypp::Selectable::PACKAGE;
		if (YGPackageSelector::get()->onlineUpdateMode())
			type = Ypp::Selectable::PATCH;
		Ypp::PoolQuery query (type);
		for (std::list <YGtkPkgQueryWidget *>::iterator it = m_queryWidgets.begin();
		     it != m_queryWidgets.end(); it++) {
			if (*it == widget)
				continue;
			(*it)->writeQuery (query);
		}
		query.addCriteria (new SuffixFilter (this));

		Ypp::List list (query);
		widget->updateList (list);
	}

	void refreshFilters (Ypp::List list)
	{
		for (std::list <YGtkPkgQueryWidget *>::iterator it = m_queryWidgets.begin();
		     it != m_queryWidgets.end(); it++) {
			if ((*it)->begsUpdate()) {
				if (YGPackageSelector::get()->yield()) return;

				if ((*it)->modified)
					refreshQueryWidget (*it);
				else
					(*it)->updateList (list);
			}
		}
	}

	void refreshToolbox()
	{
		// only present one toolbox widget as they may be quite large
		GtkWidget *toolbox = 0;
		for (std::list <YGtkPkgQueryWidget *>::iterator it = m_queryWidgets.begin();
		     it != m_queryWidgets.end(); it++)
			if ((toolbox = (*it)->createToolbox()))
				break;

		GList *children = gtk_container_get_children (GTK_CONTAINER (m_toolbox));
		for (GList *i = children; i; i = i->next)
			gtk_container_remove (GTK_CONTAINER (m_toolbox), (GtkWidget *) i->data);
		g_list_free (children);

		if (toolbox) {
			gtk_box_pack_end (GTK_BOX (m_toolbox), toolbox, FALSE, TRUE, 0);
			gtk_widget_show (m_toolbox);
		}
		else
			gtk_widget_hide (m_toolbox);
	}

	static gboolean refresh_filters_timeout_cb (gpointer data)
	{
		YGUI::ui()->busyCursor();
		if (YGPackageSelector::get()->yield()) return FALSE;

		Impl *pThis = (Impl *) data;
		pThis->refreshToolbox();
		pThis->refreshFilters (pThis->m_refresh_list);
		pThis->m_refresh_id = 0;

		YGUI::ui()->normalCursor();
		return FALSE;
	}

	virtual void refreshQuery()
	{
		if (m_refresh_id) {
			g_source_remove (m_refresh_id);
			m_refresh_id = 0;
		}

		YGUI::ui()->busyCursor();
		if (YGPackageSelector::get()->yield()) return;

		std::list <std::string> keywords;
		if (m_entry->getAttribute() == Ypp::PoolQuery::NAME)
			keywords = m_entry->getText();

		Ypp::Selectable::Type type = Ypp::Selectable::PACKAGE;
		if (YGPackageSelector::get()->onlineUpdateMode())
			type = Ypp::Selectable::PATCH;
		Ypp::PoolQuery query (type);
		for (std::list <YGtkPkgQueryWidget *>::iterator it = m_queryWidgets.begin();
			 it != m_queryWidgets.end(); it++)
			(*it)->modified = (*it)->writeQuery (query);
		query.addCriteria (new SuffixFilter (this));

		Ypp::List list (query);
		m_list->setList (list);
		m_list->setHighlight (keywords);

		YGUI::ui()->normalCursor();

		m_refresh_list = list;
		int wait = 800;
		if (keywords.empty())
			wait = 500;
		if (list.size() == 0)
			wait = 200;
		m_refresh_id = g_timeout_add_full (G_PRIORITY_LOW, wait, refresh_filters_timeout_cb, this, NULL);
	}

	// YGtkPkgQueryCombo callback

	virtual YGtkPkgQueryWidget *createQueryWidget (YGtkPkgQueryCombo *combo, int index)
	{
		Ypp::Busy busy (0);
		YGtkPkgFilterModel *model = 0;
		if (YGPackageSelector::get()->onlineUpdateMode()) {
			switch (index) {
				case 0: model = new YGtkPkgPriorityModel(); break;
				case 1: model = new YGtkPkgRepositoryModel(); break;
			}
		}
		else {
			switch (index) {
				case 0: model = new YGtkPkgPKGroupModel(); break;
				case 1: return new YGtkPkgRpmGroupsView();
				case 2: model = new YGtkPkgRepositoryModel(); break;
				case 3: model = new YGtkPkgSupportModel(); break;
				case 5:
					if (has_patterns_mode)
						return new YGtkPkgPatternView (Ypp::Selectable::PATTERN);
				case 6: return new YGtkPkgLanguageView();
			}
		}
		return new YGtkPkgFilterView (model);
	}

	// YGPackageSelector complementary methods

	static bool confirmCancel()
	{
		GtkWidget *dialog;
		dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
			 GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
			 "%s", _("Changes not saved!"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s",
			_("Quit anyway?"));
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
				                GTK_STOCK_QUIT, GTK_RESPONSE_YES, NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

		bool yes = gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES;
		gtk_widget_destroy (dialog);
		return yes;
	}

	bool confirmUnsupported()
	{
		struct UnsupportedMatch : public Ypp::Match {
			virtual bool match (Ypp::Selectable &sel)
			{ return Ypp::Package (sel).support() <= 1; }
		};

		Ypp::PoolQuery query (Ypp::Selectable::PACKAGE);
		query.addCriteria (new Ypp::StatusMatch (Ypp::StatusMatch::TO_MODIFY));
		query.addCriteria (new UnsupportedMatch());
		Ypp::List list (query);
		if(list.size() == 0)
			return true;

		GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
			GtkDialogFlags (0), GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
			_("Unsupported packages"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
			_("Please realize that the following software is either unsupported "
			"or requires an additional customer contract for support."));
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
			GTK_STOCK_APPLY, GTK_RESPONSE_YES, NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
		gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
		gtk_window_set_default_size (GTK_WINDOW (dialog), 600, 500);

		YGtkPkgListView view (true, Ypp::List::NAME_SORT, false, true);
		view.addCheckColumn (INSTALLED_CHECK_PROP);
		view.addTextColumn (_("Name"), NAME_SUMMARY_PROP, true, -1);
		view.addTextColumn (_("Supportability"), SUPPORT_PROP, true, 140);
		view.addTextColumn (_("Version"), VERSION_PROP, true, 110);
		view.addTextColumn (_("Repository"), REPOSITORY_PROP, false, 160);
		view.setListener (this);
		view.setList (list);

		GtkBox *content = GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG(dialog)));
		gtk_box_pack_start (content, view.getWidget(), TRUE, TRUE, 6);
		gtk_widget_show_all (dialog);
		int ret = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return ret == GTK_RESPONSE_YES;
	}
};

extern const char *pkg_help[], *patch_help[];

static bool confirm_cb (void *pThis)
{
	if (Ypp::isModified())
		return YGPackageSelector::Impl::confirmCancel();
	return true;
}

static void wizard_action_cb (YGtkWizard *wizard, gpointer id,
                              gint id_type, YGPackageSelector *pThis)
{
	const gchar *action = (gchar *) id;
	yuiMilestone() << "Closing PackageSelector with '" << action << "'\n";
	if (!strcmp (action, "accept"))
		pThis->apply();
	else if (!strcmp (action, "cancel"))
		pThis->cancel();
}

YGPackageSelector::YGPackageSelector (YWidget *parent, long mode)
: YPackageSelector (NULL, mode), YGWidget (this, parent, YGTK_TYPE_WIZARD, NULL),
m_historyDialog (NULL)
#ifdef HAS_VESTIGIAL_DIALOG
, m_vestigialDialog (NULL)
#endif
{
	singleton = this;
	setBorder (0);
	YGDialog *dialog = YGDialog::currentDialog();
	dialog->setCloseCallback (confirm_cb, this);

	const char *icon, *title, **help;
	if (onlineUpdateMode()) {
		icon = THEMEDIR "/icons/22x22/apps/yast-online_update.png";
		title = _("Online Update");
		help = patch_help;
	}
	else {
		icon = THEMEDIR "/icons/22x22/apps/yast-software.png";
		title = _("Software Manager");
		help = pkg_help;
	}

	YGtkWizard *wizard = YGTK_WIZARD (getWidget());
	ygtk_wizard_set_header_text (wizard, title);
	ygtk_wizard_set_header_icon (wizard, icon);
	dialog->setIcon (icon);
	ygtk_wizard_set_help_text (wizard, _("Please wait..."));

	std::string cancel_str (YGUtils::mapKBAccel ("&Cancel"));
	std::string apply_str (YGUtils::mapKBAccel ("&Apply"));
	ygtk_wizard_set_button_label (wizard,  wizard->abort_button, _(cancel_str.c_str()), GTK_STOCK_CANCEL);
	ygtk_wizard_set_button_str_id (wizard, wizard->abort_button, "cancel");
	ygtk_wizard_set_button_label (wizard,  wizard->back_button, NULL, NULL);
	ygtk_wizard_set_button_label (wizard,  wizard->next_button, _(apply_str.c_str()), GTK_STOCK_APPLY);
	ygtk_wizard_set_button_str_id (wizard, wizard->next_button, "accept");
	g_signal_connect (G_OBJECT (wizard), "action-triggered",
	                  G_CALLBACK (wizard_action_cb), this);

	impl = new Impl();  // can take a little
	ygtk_wizard_set_child (wizard, impl->getWidget());
	impl->m_menu = new YGtkPkgMenuBar();
	ygtk_wizard_set_custom_menubar (wizard, impl->m_menu->getWidget(), FALSE);
	//** TEMP: work-around global-menubar-applet: see bug 595560
	gtk_widget_show_all (impl->m_menu->getWidget());

	std::string str;
	str.reserve (6820);
	for (int i = 0; help[i]; i++)
		str.append (help [i]);
	ygtk_wizard_set_help_text (wizard, str.c_str());
	dialog->setTitle (title);

	Ypp::setInterface (impl);
	Ypp::runSolver();  // check dependencies at start
	impl->refreshQuery();

	if (summaryMode()) popupChanges();
}

YGPackageSelector::~YGPackageSelector()
{
	delete m_historyDialog;
#ifdef HAS_VESTIGIAL_DIALOG
	delete m_vestigialDialog;
#endif
	delete impl;
	singleton = 0;
}

void YGPackageSelector::cancel()
{
	if (Ypp::isModified())
		if (!Impl::confirmCancel())
			return;
	YGUI::ui()->sendEvent (new YCancelEvent());
}

void YGPackageSelector::apply()
{
	if (!Ypp::runSolver()) return;  // final dependencies check
	if (onlineUpdateMode())
		if (!Ypp::showPendingLicenses (Ypp::Selectable::PATCH)) return;
	if (!Ypp::showPendingLicenses (Ypp::Selectable::PACKAGE)) return;

	if (Ypp::isModified()) {  // confirm
		if (!onlineUpdateMode() && confirmUnsupported()) {
			if (!impl->confirmUnsupported())
				return;
		}
		if (!impl->m_undo->popupDialog (true))
			return;
	}

	YGUI::ui()->sendEvent (new YMenuEvent ("accept"));
}

void YGPackageSelector::showFilterWidget (const char *filter)
{
	int index = -1;
	if (!strcmp (filter, "patterns"))
		index = 5;
	assert (index != -1);
	impl->m_combo->setActive (index);
}

void YGPackageSelector::searchFor (Ypp::PoolQuery::StringAttribute attrb, const std::string &text)
{
	impl->clearQueryWidgets();
	impl->m_entry->setText (attrb, text);
	impl->refreshQuery();
}

void YGPackageSelector::popupChanges()
{ impl->m_undo->popupDialog (false); }

void YGPackageSelector::filterPkgSuffix (const std::string &suffix, bool enable)
{
	impl->m_filterSuffices.remove (suffix);
	if (enable)
		impl->m_filterSuffices.push_back (suffix);
	if (Ypp::getInterface() == impl)  // inited ?
		impl->refreshQuery();
}

void YGPackageSelector::showRepoManager()
{ YGUI::ui()->sendEvent (new YMenuEvent ("repo_mgr")); }

void YGPackageSelector::showHistoryDialog()
{
	if (!m_historyDialog)
		m_historyDialog = new YGtkPkgHistoryDialog();
	m_historyDialog->popup();
}

#ifdef HAS_VESTIGIAL_DIALOG
void YGPackageSelector::showVestigialDialog()
{
	if (!m_vestigialDialog)
		m_vestigialDialog = new YGtkPkgVestigialDialog();
	m_vestigialDialog->popup();
}
#endif

YGtkPkgUndoList *YGPackageSelector::undoList()
{ return impl->m_undo; }

YGtkPkgSearchEntry *YGPackageSelector::getSearchEntry()
{ return impl->m_entry; }

bool YGPackageSelector::yield()
{
	static int _id = 0;
	int id = ++_id;
	while (g_main_context_iteration (NULL, FALSE)) ;
	return id != _id;
}

#include "pkg/YGPackageSelectorPluginImpl.h"

YPackageSelector *
YGPackageSelectorPluginImpl::createPackageSelector (YWidget *parent, long modeFlags)
{
	modeFlags |= YPkg_SearchMode;
	return new YGPackageSelector (parent, modeFlags);
}

YWidget *
YGPackageSelectorPluginImpl::createPatternSelector (YWidget *parent, long modeFlags)
{
	modeFlags &= YPkg_SearchMode;
	return new YGPackageSelector (parent, modeFlags);
}

YWidget *
YGPackageSelectorPluginImpl::createSimplePatchSelector (YWidget *parent, long modeFlags)
{
	modeFlags |= YPkg_OnlineUpdateMode;
	return new YGPackageSelector (parent, modeFlags);
}

