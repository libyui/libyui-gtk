//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YGWidget.h"
#include "YPackageSelector.h"
#include "ygtkrichtext.h"
#include "ygtkwizard.h"
#include "ygtkcellrendererarrow.h"
#include "ygtkratiobox.h"
#include "ygtkfindentry.h"

//#define DISABLE_PACKAGE_SELECTOR

#ifndef DISABLE_PACKAGE_SELECTOR
#include <zypp/ZYppFactory.h>
#include <zypp/ResObject.h>
#include <zypp/ResPoolProxy.h>
#include <zypp/ui/Selectable.h>
#include <zypp/Patch.h>
#include <zypp/Selection.h>
#include <zypp/Pattern.h>
#include <zypp/Language.h>

// some typedefs and functions to short Zypp names
typedef zypp::ResPoolProxy ZyppPool;
inline ZyppPool zyppPool() { return zypp::getZYpp()->poolProxy(); }
typedef zypp::ui::Selectable::Ptr ZyppSelectable;
typedef zypp::ui::Selectable*     ZyppSelectablePtr;
typedef zypp::ResObject::constPtr ZyppObject;
typedef zypp::Package::constPtr   ZyppPackage;
typedef zypp::Patch::constPtr     ZyppPatch;
typedef zypp::Selection::constPtr ZyppSelection;
typedef zypp::Pattern::constPtr   ZyppPattern;
typedef zypp::Language::constPtr  ZyppLanguage;
inline ZyppPackage tryCastToZyppPkg (ZyppObject obj)
	{ return zypp::dynamic_pointer_cast <const zypp::Package> (obj); }
inline ZyppPatch tryCastToZyppPatch (ZyppObject obj)
	{ return zypp::dynamic_pointer_cast <const zypp::Patch> (obj); }
inline ZyppSelection tryCastToZyppSelection (ZyppObject obj)
	{ return zypp::dynamic_pointer_cast <const zypp::Selection> (obj); }
inline ZyppPattern tryCastToZyppPattern (ZyppObject obj)
	{ return zypp::dynamic_pointer_cast <const zypp::Pattern> (obj); }
inline ZyppLanguage tryCastToZyppLanguage (ZyppObject obj)
	{ return zypp::dynamic_pointer_cast <const zypp::Language> (obj); }

// Computer icon comes from Tango, but may not be installed, or not initialized.
#include "computer.xpm"

static bool acceptLicense (ZyppSelectablePtr sel)
{
	if (sel->hasLicenceConfirmed())
		return true;

	ZyppObject object = sel->candidateObj();
	ZyppPackage package = tryCastToZyppPkg (object);
	if (!package)
		return true;

	const string &license = package->licenseToConfirm();
	if (license.empty()) {
		sel->setLicenceConfirmed (true);
		return true;
	}

	GtkWidget *dialog = gtk_dialog_new_with_buttons ("Accept License?",
		YGUI::ui()->currentWindow(), GTK_DIALOG_MODAL,
		"_Reject", GTK_RESPONSE_REJECT, "_Accept", GTK_RESPONSE_ACCEPT, NULL);

	GtkWidget *license_view, *license_window;
	GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);
	gtk_text_buffer_set_text (buffer, license.c_str(), -1);
	license_view = gtk_text_view_new_with_buffer (buffer);

	license_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (license_window),
	                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type
		(GTK_SCROLLED_WINDOW (license_window), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (license_window), license_view);

	GtkBox *vbox = GTK_BOX (GTK_DIALOG(dialog)->vbox);
	gtk_box_pack_start (vbox, license_view,  FALSE, FALSE, 6);

	gtk_window_set_default_size (GTK_WINDOW (dialog), 300, 400);
	gtk_widget_show_all (dialog);

	bool confirmed = gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT;
	gtk_widget_destroy (dialog);

	sel->setLicenceConfirmed (confirmed);
	return confirmed;
}

// install / remove convinience wrapper. You only need to specify an object
// if you want that particular version to be installed.
// returns true on sucess
static bool mark_selectable (ZyppSelectablePtr sel, bool install /*or remove*/)
{
	zypp::ui::Status status = sel->status();

	if (install) {
		if (!sel->hasCandidateObj()) {
			y2warning ("YGPackageSelector: no package on repository to install");
			return false;
		}
		if (!acceptLicense (sel))
			return false;
		if (status == zypp::ui::S_Del && sel->hasInstalledObj() &&
		    sel->candidateObj()->edition() == sel->installedObj()->edition())
			status = zypp::ui::S_KeepInstalled;
		else {
			if (status == zypp::ui::S_KeepInstalled)
				status = zypp::ui::S_Update;
			else
				status = zypp::ui::S_Install;
		}
	}
	else /*remove*/ {
		if (status == zypp::ui::S_NoInst)
			;  // do nothing
		else if (status == zypp::ui::S_Install)
			status = zypp::ui::S_NoInst;
		else
			status = zypp::ui::S_Del;
	}

// if debug
	const char *name = sel->name().c_str();
	switch (status) {
		case zypp::ui::S_KeepInstalled:
			fprintf (stderr, "keep %s installed\n", name);
			break;
		case zypp::ui::S_Update:
			fprintf (stderr, "update %s\n", name);
			break;
		case zypp::ui::S_Install:
			fprintf (stderr, "install %s\n", name);
			break;
		case zypp::ui::S_Del:
			fprintf (stderr, "remove %s\n", name);
			break;
		case zypp::ui::S_NoInst:
			fprintf (stderr, "don't install %s\n", name);
			break;
		default:
			fprintf (stderr, "error: unknown action: should not happen\n");
			break;
	}

	return sel->set_status (status);
}

// tries to solve possible dependencies problems and asks the user for
// manual resolution if needed (there is not much space for interface
// design here)
static bool solveProblems()
{
	IMPL
	zypp::Resolver_Ptr resolver = zypp::getZYpp()->resolver();
	if (resolver->resolvePool())
		return true;
	zypp::ResolverProblemList problems = resolver->problems();
	if (problems.empty())
		return true;

	GtkWidget *dialog = gtk_dialog_new_with_buttons ("Resolve Problems",
		YGUI::ui()->currentWindow(), GTK_DIALOG_MODAL,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, "C_onfirm", GTK_RESPONSE_ACCEPT, NULL);

	GtkWidget *problems_view;
	problems_view = gtk_tree_view_new();

	GtkWidget *problems_window;
	problems_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (problems_window),
	                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type
		(GTK_SCROLLED_WINDOW (problems_window), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (problems_window), problems_view);

	GtkBox *vbox = GTK_BOX (GTK_DIALOG(dialog)->vbox);
	gtk_box_pack_start (vbox, problems_window,  TRUE, TRUE, 0);

	// create model
	// the vairous columns are: radio button active (boolean), problem
	// or solution description (string), is radio button (boolean: always
	// true), show radio button (boolean; false for problems)
	GtkTreeStore *problems_store = gtk_tree_store_new (4,
		G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
	// install view
	GtkTreeViewColumn *column;
	GtkCellRenderer *radio_renderer = gtk_cell_renderer_toggle_new();
	column = gtk_tree_view_column_new_with_attributes ("",
		radio_renderer, "active", 0, "radio", 2, "visible", 3, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (problems_view), column);
	g_signal_connect (G_OBJECT (radio_renderer), "toggled",
	                  G_CALLBACK (YGUtils::tree_view_radio_toggle_cb),
	                  GTK_TREE_MODEL (problems_store));

	column = gtk_tree_view_column_new_with_attributes ("Problems",
		gtk_cell_renderer_text_new(), "text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (problems_view), column);

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (problems_view), FALSE);
	gtk_tree_view_set_model (GTK_TREE_VIEW (problems_view),
	                         GTK_TREE_MODEL (problems_store));
	g_object_unref (G_OBJECT (problems_store));
	// disable selections
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (
		GTK_TREE_VIEW (problems_view)), GTK_SELECTION_NONE);;

	// construct model
	GtkTreeIter piter, siter;
	map <GtkTreePath *, zypp::ProblemSolution *> users_actions;
	for (zypp::ResolverProblemList::iterator it = problems.begin();
	     it != problems.end(); it++) {
		zypp::ProblemSolutionList solutions = (*it)->solutions();

		gtk_tree_store_append (problems_store, &piter, NULL);
		gtk_tree_store_set (problems_store, &piter, 0, FALSE,
			1, (*it)->description().c_str(), 2, TRUE, 3, FALSE, -1);

		for (zypp::ProblemSolutionList::iterator jt = solutions.begin();
		     jt != solutions.end(); jt++) {
			gtk_tree_store_append (problems_store, &siter, &piter);
			gtk_tree_store_set (problems_store, &siter, 0, FALSE,
				1, (*jt)->description().c_str(), 2, TRUE, 3, TRUE, -1);
			users_actions [gtk_tree_model_get_path (GTK_TREE_MODEL (
				problems_store), &siter)] = get_pointer (*jt);
		}
	}

	gtk_window_set_default_size (GTK_WINDOW (dialog), 300, 400);
	gtk_widget_show_all (dialog);

	bool confirmed = gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT;

	if (confirmed) {
		// apply user solutions
		zypp::ProblemSolutionList userChoices;

		GtkTreeIter iter;
		for (map <GtkTreePath *, zypp::ProblemSolution *>::iterator it
		     = users_actions.begin(); it != users_actions.end(); it++) {
			gtk_tree_model_get_iter (GTK_TREE_MODEL (problems_store),
			                         &iter, it->first);
			gboolean state = FALSE;
			gtk_tree_model_get (GTK_TREE_MODEL (problems_store), &iter,
			                    0, &state, -1);
			if (state)
				userChoices.push_back (it->second);

			gtk_tree_path_free (it->first);
		}
		resolver->applySolutions (userChoices);
	}

	gtk_widget_destroy (dialog);
	if (confirmed)
		// repeate the all process as things may not yet be solved
		return solveProblems();
	return false;
}

#define INFORMATION_HEIGHT 140

// FIXME: do we really need both YGtkRichText and GtkTextView here

// Expander widget with the package information
class PackageInformation
{
	GtkWidget *m_widget;
	// Information text
	GtkWidget *m_about_text, *m_authors_text, *m_filelist_text, *m_history_text;

public:
	PackageInformation (const char *expander_text, bool only_description)
	{
		m_widget = gtk_expander_new (expander_text);
		gtk_widget_set_sensitive (m_widget, FALSE);

		if (only_description) {
			GtkWidget *about_win = gtk_scrolled_window_new (NULL, NULL);
			gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (about_win),
			                                GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
			m_about_text = gtk_text_view_new();
			gtk_container_add (GTK_CONTAINER (about_win), m_about_text);
			gtk_container_add (GTK_CONTAINER (m_widget), about_win);
			m_authors_text = m_filelist_text = m_history_text = NULL;
		}
		else {
			GtkWidget *notebook = gtk_notebook_new();
			gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_BOTTOM);
			gtk_container_add (GTK_CONTAINER (m_widget), notebook);

			m_about_text = add_text_tab (notebook, "Description");
			m_filelist_text = add_text_tab (notebook, "File List");
			m_history_text = add_text_tab (notebook, "History");
			m_authors_text = add_text_tab (notebook, "Authors");
		}
		gtk_widget_set_size_request (gtk_bin_get_child (GTK_BIN (m_widget)),
		                             -1, INFORMATION_HEIGHT);
	}

	GtkWidget *getWidget()
	{ return m_widget; }

	void setPackage (ZyppSelectablePtr selectable, ZyppObject object)
	{
		if (!selectable || !object) {
			if (m_about_text)
				set_text (m_about_text, "");
			if (m_filelist_text)
				set_text (m_filelist_text, "");
			if (m_history_text)
				set_text (m_history_text, "");
			if (m_authors_text)
				set_text (m_authors_text, "");
			gtk_widget_set_sensitive (m_widget, FALSE);
			return;
		}

		gtk_widget_set_sensitive (m_widget, TRUE);

		if (m_about_text) {
			string description = object->description();//YGUtils::escape_markup (object->description());
			set_text (m_about_text, description);
			// TODO: cut "Authors:" and following "-----" line to some authors string
		}
		ZyppPackage package = tryCastToZyppPkg (object);
		if (m_filelist_text) {
			string filelist;
			const std::list <string> &filenames = package->filenames();
			for (std::list <string>::const_iterator it = filenames.begin();
			     it != filenames.end(); it++)
				filelist += (*it) + "\n";
			set_text (m_filelist_text, filelist);
		}
		if (m_history_text) {
			string history;
			const std::list <zypp::ChangelogEntry> &changelog = package->changelog();
			for (std::list <zypp::ChangelogEntry>::const_iterator it = changelog.begin();
			     it != changelog.end(); it++)
				history += "<p><i>" + it->date().asString() + "</i> "
				           "<b>" + it->author() + "</b><br>" +
				           it->text() + "</p>";
			set_text (m_history_text, history);
		}
	}

	// auxiliaries to cut down on code
	static GtkWidget *add_text_tab (GtkWidget *notebook, const char *label)
	{
		GtkWidget *widget, *scroll_win;
		scroll_win = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll_win),
		                                GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
		widget = ygtk_richtext_new();
		gtk_container_add (GTK_CONTAINER (scroll_win), widget);

		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scroll_win,
		                          gtk_label_new (label));
		return widget;
	}

	static void set_text (GtkWidget *widget, const string &text)
	{
		ygtk_richtext_set_text (YGTK_RICHTEXT (widget), text.c_str(), FALSE, TRUE);
	}
};

// YOU patch selector
class YGPatchSelector : public YPackageSelector, public YGWidget
{
	GtkWidget *m_patches_view;
	GtkTreeModel *m_patches_model;  // GtkListStore type

	// Package information widget
	PackageInformation *m_information_widget;

public:
	YGPatchSelector (const YWidgetOpt &opt, YGWidget *parent)
		: YPackageSelector (opt),
		  YGWidget (this, parent, true, YGTK_TYPE_WIZARD, NULL)
	{
		setBorder (0);
		GtkWidget *main_vbox = gtk_vbox_new (FALSE, 0);

		YGtkWizard *wizard = YGTK_WIZARD (getWidget());
		ygtk_wizard_set_child (YGTK_WIZARD (getWidget()), main_vbox);
		GtkWindow *window = YGUI::ui()->currentWindow();
		ygtk_wizard_set_header_text (wizard, window, "Patch Selector");
		ygtk_wizard_set_header_icon (wizard, window,
			THEMEDIR "/icons/32x32/apps/yast-software.png");
		ygtk_wizard_set_help_text (wizard,
			"For information on a given patch, just press it and as well as the "
			"Package Information expander to make those informations visible.<br>"
			"To install a patch you just need to press the check button next to it "
			"and then the button Install when you are done."
		);
		ygtk_wizard_set_next_button_label (wizard, "_Install");
		ygtk_wizard_set_next_button_id (wizard, g_strdup ("install"), g_free);
		ygtk_wizard_set_abort_button_label (wizard, "_Cancel");
		ygtk_wizard_set_abort_button_id (wizard, g_strdup ("cancel"), g_free);
		g_signal_connect (G_OBJECT (getWidget()), "action-triggered",
		                  G_CALLBACK (wizard_action_cb), this);

		m_patches_view = gtk_tree_view_new();
		GtkWidget *patches_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_shadow_type
			(GTK_SCROLLED_WINDOW (patches_window), GTK_SHADOW_IN);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (patches_window),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_container_add (GTK_CONTAINER (patches_window), m_patches_view);

		m_information_widget = new PackageInformation ("Patch Information", true);
		GtkWidget *pkg_info_widget = m_information_widget->getWidget();

		gtk_box_pack_start (GTK_BOX (main_vbox), patches_window, TRUE, TRUE, 6);
		gtk_box_pack_start (GTK_BOX (main_vbox), pkg_info_widget, FALSE, FALSE, 6);
		gtk_widget_show_all (main_vbox);

		// Create a model for the patches lists
		// models' columns: selected radio button state (boolean),
		// package name (string), patch priority (string), selectable object (pointer)
		m_patches_model = GTK_TREE_MODEL (gtk_list_store_new
			(4, G_TYPE_BOOLEAN,  G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER));

		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer = gtk_cell_renderer_toggle_new();
		column = gtk_tree_view_column_new_with_attributes ("", renderer,
		                                                   "active", 0, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (m_patches_view), column);
		g_signal_connect (G_OBJECT (renderer), "toggled",
		                  G_CALLBACK (patch_toggled_cb), this);

		column = gtk_tree_view_column_new_with_attributes ("Priority",
			gtk_cell_renderer_text_new(), "text", 1, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (m_patches_view), column);

		column = gtk_tree_view_column_new_with_attributes ("Name",
			gtk_cell_renderer_text_new(), "text", 2, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (m_patches_view), column);

		for (ZyppPool::const_iterator it = zyppPool().byKindBegin <zypp::Patch>();
		     it != zyppPool().byKindEnd <zypp::Patch>(); it++) {
			ZyppSelectable selectable = *it;
			ZyppPatch patch = tryCastToZyppPatch (selectable->theObj());

			if (patch && !selectable->hasInstalledObj() &&
			    selectable->hasCandidateObj() /*don't show installed ones*/) {
				// select them all
				selectable->set_status (zypp::ui::S_Install);

				GtkTreeIter iter;
				GtkListStore *store = GTK_LIST_STORE (m_patches_model);
				gtk_list_store_append (store, &iter);
				gtk_list_store_set (store, &iter, 0, selectable->toInstall(),
					1, patch->category().c_str(), 2, selectable->name().c_str(),
					3, get_pointer (selectable), -1);
			}
		}

		gtk_tree_view_set_model (GTK_TREE_VIEW (m_patches_view), m_patches_model);

		// TODO: maybe we should sort using severity, not using string.
		YGUtils::tree_view_set_sortable (GTK_TREE_VIEW (m_patches_view));
		// sort severity by default
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (m_patches_model), 1,
		                                      GTK_SORT_ASCENDING);
		column = gtk_tree_view_get_column (GTK_TREE_VIEW (m_patches_view), 1);
		gtk_tree_view_column_set_sort_order (column, GTK_SORT_ASCENDING);
		gtk_tree_view_column_set_sort_indicator (column, TRUE);
		gtk_tree_view_set_search_column (GTK_TREE_VIEW (m_patches_view), 2);

		g_signal_connect (G_OBJECT (m_patches_view), "cursor-changed",
		                  G_CALLBACK (patch_clicked_cb), this);
	}

	virtual ~YGPatchSelector()
	{
		IMPL
		g_object_unref (m_patches_model);
		delete m_information_widget;
	}

	static ZyppSelectablePtr selectedPatch (GtkTreeView *tree_view)
	{
		IMPL
		GtkTreePath *path;
		GtkTreeViewColumn *column;
		gtk_tree_view_get_cursor (tree_view, &path, &column);
		if (path) {
			GtkTreeModel *model = gtk_tree_view_get_model (tree_view);

			ZyppSelectablePtr selectable = 0;
			GtkTreeIter iter;
			gtk_tree_model_get_iter (model, &iter, path);
			gtk_tree_model_get (model, &iter, 3, &selectable, -1);

			gtk_tree_path_free (path);
			return selectable;
		}
		return NULL;
	}

	// callbacks
	static void patch_clicked_cb (GtkTreeView *tree_view, YGPatchSelector *pThis)
	{
		IMPL
		ZyppSelectablePtr sel = selectedPatch (tree_view);
		pThis->m_information_widget->setPackage (sel, sel->theObj());
	}

	static void wizard_action_cb (YGtkWizard *wizard, gpointer id,
	                              gint id_type, YGPatchSelector *pThis)
	{
		const gchar *action = (gchar *) id;
		if (!strcmp (action, "install")) {
			y2milestone ("Closing PackageSelector with 'accept'");
			if (solveProblems())
				YGUI::ui()->sendEvent (new YMenuEvent (YCPSymbol ("accept")));
		}
		else if (!strcmp (action, "cancel")) {
			y2milestone ("Closing PackageSelector with 'cancel'");
			YGUI::ui()->sendEvent (new YCancelEvent());
		}
	}

	static void patch_toggled_cb (GtkCellRendererToggle *renderer,
                                gchar *path_str, YGPatchSelector *pThis)
	{
		IMPL
		GtkTreeModel *model = pThis->m_patches_model;

		// Toggle the box
		GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
		gint *column = (gint*) g_object_get_data (G_OBJECT (renderer), "column");
		GtkTreeIter iter;
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_path_free (path);

		gboolean state;
		ZyppSelectablePtr selectable = 0;
		gtk_tree_model_get (model, &iter, 0, &state, 3, &selectable, -1);

		state = !state;
//printf ("setting patch %s for %d\n", selectable->name().c_str(), state);

		if (mark_selectable (selectable, state))
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, state, -1);
	}

	YGWIDGET_IMPL_COMMON
};

// Pattern selector
class PatternSelector
{
	GtkWidget *m_widget, *m_buttons;

public:
	PatternSelector()
	{
		IMPL
		m_widget = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (getWidget()),
		                                GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

		GtkWidget *view_port = gtk_viewport_new (NULL, NULL);
		gtk_viewport_set_shadow_type (GTK_VIEWPORT (view_port), GTK_SHADOW_NONE);
		gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (m_widget),
		                                       view_port);

		m_buttons = gtk_vbox_new (FALSE, 0);
		gtk_container_add (GTK_CONTAINER (view_port), m_buttons);

		// before starting to actually create the interface widgets, we need
		// to store everything to get everything in the right order
		map <string, vector <ZyppSelectablePtr> > categories;
		// split into categories
		for (ZyppPool::const_iterator it = zyppPool().byKindBegin <zypp::Selection>();
		     it != zyppPool().byKindEnd <zypp::Selection>(); it++) {
			ZyppSelectable selectable = *it;
			ZyppObject object = selectable->theObj();
			ZyppSelection selection = tryCastToZyppSelection (object);
			if (selection && selection->visible())
				categories [selection->category()].push_back (get_pointer (selectable));
		}
		// order them
		for (map <string, vector <ZyppSelectablePtr> >::iterator it = categories.begin();
		     it != categories.end(); it++) {
			vector <ZyppSelectablePtr> &patterns = it->second;
			for (unsigned int i = 0; i < patterns.size(); i++) {
				ZyppSelection sel_i = tryCastToZyppSelection (patterns[i]->theObj());
				for (unsigned int j = 0; j < patterns.size(); j++) {
					ZyppSelection sel_j = tryCastToZyppSelection (patterns[j]->theObj());
					if (strcmp (sel_i->order().c_str(), sel_j->order().c_str()) == 1) {
						ZyppSelectablePtr t =  patterns[j];
						patterns[j] = patterns[i];
						patterns[i] = t;
					}
				}
			}
		}
		// create interface
		// adding patterns
		for (map <string, vector <ZyppSelectablePtr> >::iterator it = categories.begin();
		     it != categories.end(); it++) {
			const vector <ZyppSelectablePtr> &patterns = it->second;

			GtkWidget *label = gtk_label_new (
				("<big><b>" + it->first + "</b></big>").c_str());
			gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
			gtk_box_pack_start (GTK_BOX (m_buttons), label, FALSE, FALSE, 6);

			for (unsigned int i = 0; i < patterns.size(); i++) {
				ZyppSelectablePtr selectable = patterns[i];
				ZyppObject object = selectable->theObj();
				ZyppSelection selection = tryCastToZyppSelection (object);
				ZyppPattern pattern = tryCastToZyppPattern (object);

				if (selection && selection->visible()) {
					GtkWidget *hbox, *button, *image = 0;
					button = gtk_check_button_new();
					g_object_set_data (G_OBJECT (button), "selectable", selectable);

					hbox = gtk_hbox_new (FALSE, 0);
					label = gtk_label_new (
						("<b>" + object->summary() + "</b>\n" + object->description()).c_str());
					gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
					gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

					string icon = pattern ? pattern->icon().asString() : "";
					if (icon.empty()) {
						// it looks like there is rarely an icon present, so we'll hard-
						// code a couple of cases
						string name = selectable->name();
						if (selection->isBase())
							icon = "system";
						else if (YGUtils::contains (name, "gnome"))
							icon = "gnome";
						else if (YGUtils::contains (name, "kde"))
							icon = "kde";
						else if (YGUtils::contains (name, "games"))
							icon = "yast-joystick";
						else if (YGUtils::contains (name, "x11"))
							icon = "yast-x11";

						if (!icon.empty())
							icon = THEMEDIR "/icons/32x32/apps/" + icon + ".png";
					}

					if (g_file_test (icon.c_str(), G_FILE_TEST_EXISTS))
						image = gtk_image_new_from_file (icon.c_str());

					if (image)
						gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
					gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 6);
					gtk_container_add (GTK_CONTAINER (button), hbox);

					bool is_installed = selectable->status() == zypp::ui::S_KeepInstalled;
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), is_installed);
					if (selection->isBase() && is_installed)
						gtk_widget_set_sensitive (button, FALSE);

					gtk_box_pack_start (GTK_BOX (m_buttons), button, FALSE, FALSE, 6);
				}
			}
		}
		// adding languages
		{
			GtkWidget *hbox, *image, *button, *label;

			hbox = gtk_hbox_new (FALSE, 0);
			label = gtk_label_new ("<big><b>Languages</b></big>");
			gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
			if (g_file_test (THEMEDIR "/icons/32x32/apps/yast-yast-language.png",
			                 G_FILE_TEST_EXISTS)) {
				image = gtk_image_new_from_file (THEMEDIR
				            "/icons/32x32/apps/yast-yast-language.png");
				gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 6);
			}
			gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (m_buttons), hbox, FALSE, FALSE, 6);

			for (ZyppPool::const_iterator it = zyppPool().byKindBegin <zypp::Language>();
			     it != zyppPool().byKindEnd <zypp::Language>(); it++) {
				ZyppSelectable selectable = *it;
				ZyppObject object = selectable->theObj();
				ZyppLanguage language = tryCastToZyppLanguage (object);

				if (language) {
					button = gtk_check_button_new();
					g_object_set_data (G_OBJECT (button), "selectable",
					                   get_pointer (selectable));

					label = gtk_label_new (object->description().c_str());
					gtk_container_add (GTK_CONTAINER (button), label);

					bool is_installed = selectable->status() == zypp::ui::S_KeepInstalled;
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), is_installed);

					gtk_box_pack_start (GTK_BOX (m_buttons), button, FALSE, FALSE, 6);
				}
			}
		}
		gtk_widget_show_all (m_widget);
	}

	GtkWidget *getWidget()
	{ return m_widget; }

	// Applies the pattern and returns the packages to install and remove
	// via arguments references
	void apply (set <string> &to_install, set <string> &to_remove)
	{
		IMPL
		GList *buttons = gtk_container_get_children (GTK_CONTAINER (m_buttons));
		GList *i;
		for (i = g_list_first (buttons); i; i = i->next) {
			if (!GTK_IS_TOGGLE_BUTTON (i->data))
				continue;
			GtkToggleButton *button = GTK_TOGGLE_BUTTON (i->data);
			ZyppSelectablePtr selectable = (ZyppSelectablePtr)
				g_object_get_data (G_OBJECT (button), "selectable");
			if (!selectable)
				continue;

			ZyppSelection pattern = tryCastToZyppSelection (selectable->theObj());
			ZyppLanguage lang = tryCastToZyppLanguage (selectable->theObj());

			set <string> *packages;
			if (gtk_toggle_button_get_active (button)) {
				if (!selectable->set_status (zypp::ui::S_Install))
					continue;
				packages = &to_install;
			}
			else {
				if (!selectable->set_status (zypp::ui::S_Del))
					continue;
				packages = &to_remove;
			}

			if (pattern) {
				const set <string> &_packages = pattern->install_packages();
				packages->insert (_packages.begin(), _packages.end());
			}
			else {
				for (ZyppPool::const_iterator it =
				          zyppPool().byKindBegin <zypp::Package>();
				      it != zyppPool().byKindEnd <zypp::Package>(); it++) {
					ZyppSelectable selectable = *it;
					ZyppObject object = selectable->theObj();

					const zypp::CapSet &capSet = object->dep (zypp::Dep::FRESHENS);
					for (zypp::CapSet::const_iterator it = capSet.begin();
					     it != capSet.end(); it++) {
						if (it->asString() == lang->name()) {
							packages->insert (selectable->name());
						}
					}
				}
			}
		}
		g_list_free (buttons);
	}
};

// Package selector's widget
class PackageSelector
{
friend class YGPackageSelector;

	GtkWidget *m_widget;

	// The GtkTreeView widgets
	GtkWidget *m_installed_view, *m_available_view;
	// Packages backend model
	GtkTreeModel *m_packages_model;

	// Package information widget
	PackageInformation *m_information_widget;

	// Search gizmos
	GtkWidget *m_search_entry, *m_plain_view;
	guint search_timeout_id;
	bool name_opt, summary_opt, descr_opt, provides_opt, requires_opt;
	bool m_searching;
	list <string> m_search_queries;

	// Interface tweak
	GtkWidget *m_install_label, *m_remove_label;

public:
	PackageSelector (bool enablePatterns = true)
	{
		IMPL
		m_widget = gtk_vbox_new (FALSE, 0);

		GtkWidget *packages_hbox = gtk_hbox_new (FALSE, 0);

		GtkWidget *available_box, *installed_box;
		/* All views share a similar model structure. There is a common model for
		   all packages, where filters are installed upon to split it on two, each
		   installed on the tree view. The common model follows the following spec:
		   0 - selectable object (pointer), 1 - installed name (string), 2 - available
		   name (string), 3 - is installed (boolean), 4 - is available (boolean),
		   5 - can be upgraded (boolean), 6 - can be downgraded (boolean) = 7

		   Models are created at each view mode change (and the other freed). This
		   allows for more than one model type be used and is also better for speed,
		   since the filter and tree view then installed upon don't have to keep
		   syncing at every item change. */
		installed_box = createListWidget ("<b>Installed Software:</b>",
		                                  "computer", computer_xpm, m_installed_view,
		                                  1, false);
		available_box = createListWidget ("<b>Available Software:</b>",
		                                  "gtk-cdrom", NULL, m_available_view,
		                                  2, true);

		GtkWidget *buttons_minsize, *selection_buttons_vbox, *install_button,
		          *remove_button;
		selection_buttons_vbox = gtk_vbox_new (FALSE, 0);
		// to avoid re-labeling glitches, let it only grow
		buttons_minsize = ygtk_min_size_new (0, 0);
		gtk_container_add (GTK_CONTAINER (buttons_minsize), selection_buttons_vbox);
		ygtk_min_size_set_only_expand (YGTK_MIN_SIZE (buttons_minsize), TRUE);

		install_button = createArrowButton ("_install", GTK_ARROW_RIGHT, &m_install_label);
		remove_button = createArrowButton ("_remove", GTK_ARROW_LEFT, &m_remove_label);

		gtk_box_pack_start (GTK_BOX (selection_buttons_vbox), install_button,
		                                                      TRUE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (selection_buttons_vbox), remove_button,
		                                                      TRUE, FALSE, 0);
		gtk_container_set_border_width (GTK_CONTAINER (selection_buttons_vbox), 6);

		gtk_box_pack_start (GTK_BOX (packages_hbox), available_box, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (packages_hbox), buttons_minsize,
		                                             FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (packages_hbox), installed_box, TRUE, TRUE, 0);

		GtkWidget *view_box, *view_label, *view_categories, *view_patterns;
		view_label = gtk_label_new ("View packages:");
		m_plain_view = gtk_radio_button_new_with_mnemonic (NULL, "as _plain list");
		view_categories = gtk_radio_button_new_with_mnemonic_from_widget
		                      (GTK_RADIO_BUTTON (m_plain_view), "in _categories");
		view_patterns = gtk_radio_button_new_with_mnemonic_from_widget
		                      (GTK_RADIO_BUTTON (m_plain_view), "in _patterns");
		view_box = gtk_hbox_new (FALSE, 0);
		gtk_box_pack_start (GTK_BOX (view_box), view_label, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (view_box), m_plain_view, FALSE, FALSE, 4);
		gtk_box_pack_start (GTK_BOX (view_box), view_categories, FALSE, FALSE, 4);
		gtk_box_pack_start (GTK_BOX (view_box), view_patterns, FALSE, FALSE, 4);
		g_signal_connect (G_OBJECT (m_plain_view), "toggled",
		                  G_CALLBACK (view_plain_mode_cb), this);
		g_signal_connect (G_OBJECT (view_categories), "toggled",
		                  G_CALLBACK (view_categories_mode_cb), this);
		g_signal_connect (G_OBJECT (view_patterns), "toggled",
		                  G_CALLBACK (view_patterns_mode_cb), this);

		// default search fields
		name_opt = summary_opt = descr_opt = provides_opt = true;
		requires_opt = false;

		GtkWidget *search_hbox, *search_label;
		search_hbox = gtk_hbox_new (FALSE, 0);
		search_label = gtk_label_new_with_mnemonic ("_Search:");
		m_search_entry = ygtk_find_entry_new();
		gtk_label_set_mnemonic_widget (GTK_LABEL (search_label), m_search_entry);
		gtk_box_pack_start (GTK_BOX (search_hbox), search_label, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (search_hbox), m_search_entry, TRUE, TRUE, 4);
		search_timeout_id = 0;
		g_signal_connect (G_OBJECT (m_search_entry), "activate",  // when Enter
		                  G_CALLBACK (search_request_cb), this);  // is pressed
		g_signal_connect (G_OBJECT (m_search_entry), "changed",
		                  G_CALLBACK (search_request_cb), this);
		ygtk_find_entry_attach_menu (YGTK_FIND_ENTRY (m_search_entry),
		                             create_search_menu());

		m_information_widget = new PackageInformation ("Package Information", false);
		GtkWidget *pkg_info_widget = m_information_widget->getWidget();

		gtk_box_pack_start (GTK_BOX (m_widget), packages_hbox, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_widget), view_box, FALSE, FALSE, 12);
		gtk_box_pack_start (GTK_BOX (m_widget), search_hbox, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (m_widget), pkg_info_widget, FALSE, FALSE, 12);
		gtk_widget_show_all (m_widget);

		g_object_ref (G_OBJECT (m_widget));
		gtk_object_sink (GTK_OBJECT (m_widget));

		// interface sugar
		g_signal_connect (G_OBJECT (install_button), "clicked",
		                  G_CALLBACK (install_button_clicked_cb), this);
		g_signal_connect (G_OBJECT (remove_button), "clicked",
		                  G_CALLBACK (remove_button_clicked_cb), this);

		g_signal_connect (G_OBJECT (m_installed_view), "cursor-changed",
		                  G_CALLBACK (package_clicked_cb), this);
		g_signal_connect (G_OBJECT (m_available_view), "cursor-changed",
		                  G_CALLBACK (package_clicked_cb), this);

		// signal plain view
		g_signal_emit_by_name (G_OBJECT (m_plain_view), "toggled", this);
	}

	// create widgets functions to cut down on code
	GtkWidget *createListWidget (const char *header, const char *stock_icon,
		const char **xpm_icon,  GtkWidget *&list, int package_name_col,
		bool has_version_col)
	{
		GtkWidget *vbox, *header_hbox, *image, *label, *scrolled_window;

		vbox = gtk_vbox_new (FALSE, 0);
		scrolled_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_shadow_type
			(GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
		                                GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		list = gtk_tree_view_new();
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list), FALSE);
		gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list), TRUE);
		gtk_container_add (GTK_CONTAINER (scrolled_window), list);

		header_hbox = gtk_hbox_new (FALSE, 0);
		image = NULL;
		if (stock_icon)
			image = gtk_image_new_from_icon_name (stock_icon, GTK_ICON_SIZE_BUTTON);
		if ((!stock_icon || gtk_image_get_storage_type (GTK_IMAGE (image)) == GTK_IMAGE_EMPTY)
		    && xpm_icon) {
			GdkPixbuf *pixbuf = gdk_pixbuf_new_from_xpm_data (xpm_icon);
			image = gtk_image_new_from_pixbuf (pixbuf);
			g_object_unref (G_OBJECT (pixbuf));
		}
		label = gtk_label_new_with_mnemonic (header);
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
		gtk_label_set_mnemonic_widget (GTK_LABEL (label), list);
		if (image)
			gtk_box_pack_start (GTK_BOX (header_hbox), image, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (header_hbox), label, FALSE, FALSE, 4);

		gtk_box_pack_start (GTK_BOX (vbox), header_hbox, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 4);

		// The treeview model will be built according to the view... Columns
		// and renderers are common anyway...
		GtkCellRenderer *text_renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column;
		int wrap_width = 250;
		column = gtk_tree_view_column_new_with_attributes (NULL,
			text_renderer, "markup", package_name_col, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
		// versions combo column
		if (has_version_col) {
			GtkCellRenderer *arrow_renderer = ygtk_cell_renderer_arrow_new();
			column = gtk_tree_view_column_new_with_attributes (NULL,
				arrow_renderer, "can-go-up", 5, "can-go-down", 6, NULL);
//			gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
			gtk_tree_view_column_set_expand (column, FALSE);
			gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
			g_signal_connect (G_OBJECT (arrow_renderer), "pressed",
			                  G_CALLBACK (change_available_version_cb), this);

			int w;
			gtk_cell_renderer_get_size (GTK_CELL_RENDERER (arrow_renderer), list,
			                            NULL, NULL, NULL, &w, NULL);
			wrap_width -= w;
		}
		g_object_set (G_OBJECT (text_renderer), "wrap-width", wrap_width,
			"wrap-mode", PANGO_WRAP_WORD_CHAR, NULL);
		GObject *vadjustment = G_OBJECT (gtk_scrolled_window_get_vadjustment (
		                           GTK_SCROLLED_WINDOW (scrolled_window)));
		g_object_set_data (vadjustment, "view", list);
		g_signal_connect (vadjustment, "value-changed",
		                  G_CALLBACK (view_scrolled_cb), this);
		return vbox;
	}

	GtkWidget *createArrowButton (const char *label, GtkArrowType arrow_type,
	                              GtkWidget **label_widget)
	{
		GtkWidget *button, *box = gtk_hbox_new (FALSE, 0);
		GtkWidget *arrow = gtk_arrow_new (arrow_type, GTK_SHADOW_OUT);
		gtk_container_add (GTK_CONTAINER (box), arrow);
		*label_widget = gtk_label_new_with_mnemonic (label);
		gtk_container_add (GTK_CONTAINER (box), *label_widget);
		gtk_box_set_child_packing (GTK_BOX (box), arrow, FALSE, TRUE, 0,
			arrow_type == GTK_ARROW_LEFT ? GTK_PACK_START : GTK_PACK_END);

		button = gtk_button_new();
		gtk_container_add (GTK_CONTAINER (button), box);
		return button;
	}

	static GtkTreeModel *loadPackagesListAsPlain()
	{
		GtkListStore *store = gtk_list_store_new (7, G_TYPE_POINTER, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
		GtkTreeModel *model = GTK_TREE_MODEL (store);

		GtkTreeIter iter;
		for (ZyppPool::const_iterator it = zyppPool().byKindBegin <zypp::Package>();
		     it != zyppPool().byKindEnd <zypp::Package>(); it++)
		{
			ZyppSelectable selectable = *it;
			gtk_list_store_append (store, &iter);
			loadPackageRow (model, &iter, selectable);
		}
		return model;
	}

	static GtkTreeModel *loadPackagesListByCategory()
	{
		GtkTreeStore *store = gtk_tree_store_new (7, G_TYPE_POINTER, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
		GtkTreeModel *model = GTK_TREE_MODEL (store);

		// we need to create the categories tree as we iterate packages
		map <string, GtkTreePath*> tree;

		GtkTreeIter iter, parent_iter;
		for (ZyppPool::const_iterator it = zyppPool().byKindBegin <zypp::Package>();
		     it != zyppPool().byKindEnd <zypp::Package>(); it++)
		{
			ZyppSelectable selectable = *it;
			ZyppObject object = selectable->theObj();

			GtkTreePath *path = NULL;
			ZyppPackage package = tryCastToZyppPkg (object);
			if (package) {
				// group is a string like "Productivity/Networking/Email/Utilities"
				string group = package->group();

				// We will now retrieve the hierarchy of the package's group and
				// make sure a path has already been created on all nodes, as well
				// as get last node's GtkTreePath
				list <string> hierarchy = YGUtils::splitString (group, '/');
				while (!hierarchy.empty()) {
					string node = hierarchy.front();
					hierarchy.pop_front();

					map <string, GtkTreePath*>::iterator it = tree.find (node);
					if (it == tree.end()) {
						if (path) {
							gtk_tree_model_get_iter (GTK_TREE_MODEL (store),
							                         &parent_iter, path);
							gtk_tree_store_append (store, &iter, &parent_iter);
						}
						else  // create at root
							gtk_tree_store_append (store, &iter, NULL);

						string name = "<b>" + node + "</b>";
						gtk_tree_store_set (store, &iter,
							0, NULL, 1, name.c_str(), 2, name.c_str(), 3, TRUE,
							4, TRUE, 5, FALSE, 6, FALSE, -1);

						path = gtk_tree_model_get_path (model, &iter);
						tree [node] = path;
					}
					else // exists
						path = it->second;
				}
			}

			if (!path)  // package has no group
				continue;

			gtk_tree_model_get_iter (model, &parent_iter, path);
			gtk_tree_store_append (store, &iter, &parent_iter);
			loadPackageRow (model, &iter, selectable);
		}

		// free GtkTreePaths
		for (map <string, GtkTreePath*>::iterator it = tree.begin();
		     it != tree.end(); it++)
			gtk_tree_path_free (it->second);
		return model;
	}

	static void induceObjects (ZyppSelectable selectable, ZyppObject &install_obj,
	                           ZyppObject &available_obj)
	{
		available_obj = (install_obj = NULL);
		if (!selectable)
			return;
		switch (selectable->status()) {
			case zypp::ui::S_Install:
			case zypp::ui::S_Update:
				install_obj = selectable->candidateObj();
				break;
			case zypp::ui::S_Del:
				available_obj = selectable->installedObj();
				break;
			default:
				available_obj = selectable->candidateObj();
				install_obj = selectable->installedObj();
				if (available_obj && install_obj && selectable->availableObjs() == 1 &&
				    available_obj->edition() == install_obj->edition())
					available_obj = NULL;
				break;
		}
	}

	/* Must be called for the main model. */
	static void loadPackageRow (GtkTreeModel *model, GtkTreeIter *iter,
	                            ZyppSelectable selectable)
	{
		ZyppObject install_obj, available_obj;
		induceObjects (selectable, install_obj, available_obj);

		string availableName, installedName;
		availableName = (installedName = YGUtils::escape_markup (selectable->name()));
		if (available_obj)
			availableName += " (" + available_obj->edition().version() + ")\n" +
			                 "<small>" + YGUtils::escape_markup (available_obj->summary()) +
			                 "</small>";
		if (install_obj)
			installedName += " (" + install_obj->edition().version() + ")\n" +
			                 "<small>" + YGUtils::escape_markup (install_obj->summary()) +
			                 "</small>";

		bool has_downgrade = false, has_upgrade = false;
		if (available_obj) {
			for (zypp::ui::Selectable::available_iterator it = selectable->availableBegin();
			     it != selectable->availableEnd(); it++) {
				int res = zypp::Edition::compare ((*it)->edition(), available_obj->edition());
				if (res < 0)
					has_downgrade = true;
				else if (res > 0)
					has_upgrade = true;
			}
		}

		// oh, would be nice to have a common set for tree models...
		if (GTK_IS_LIST_STORE (model))
			gtk_list_store_set (GTK_LIST_STORE (model), iter,
				0, get_pointer (selectable), 1, installedName.c_str(),
				2, availableName.c_str(), 3, install_obj != 0, 4, available_obj != 0,
				5, has_upgrade, 6, has_downgrade, -1);
		else /*if (GTK_IS_TREE_STORE (model))*/
			gtk_tree_store_set (GTK_TREE_STORE (model), iter,
				0, get_pointer (selectable), 1, installedName.c_str(),
				2, availableName.c_str(), 3, install_obj != 0, 4, available_obj != 0,
				5, has_upgrade, 6, has_downgrade, -1);
	}

	virtual ~PackageSelector()
	{
		IMPL
		if (search_timeout_id)
			g_source_remove (search_timeout_id);

		delete m_information_widget;
		gtk_widget_destroy (m_widget);
		g_object_unref (G_OBJECT (m_widget));
	}

	GtkWidget *getWidget()
	{ return m_widget; }

	void load_packages_view (GtkTreeModel *(* build_model) (void))
	{
		GtkTreeModel *model = (m_packages_model = build_model());

		GtkTreeModel *installed_model, *available_model;
		installed_model = gtk_tree_model_filter_new (model, NULL);
		available_model = gtk_tree_model_filter_new (model, NULL);
		g_object_unref (G_OBJECT (model));

		gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (
			installed_model), is_package_installed, this, NULL);
		gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (
			available_model), is_package_available, this, NULL);

		gtk_tree_view_set_model (GTK_TREE_VIEW (m_installed_view), installed_model);
		gtk_tree_view_set_model (GTK_TREE_VIEW (m_available_view), available_model);
		g_object_unref (G_OBJECT (installed_model));
		g_object_unref (G_OBJECT (available_model));

		YGUtils::tree_model_set_col_sortable (GTK_TREE_SORTABLE (model), 1);
		YGUtils::tree_model_set_col_sortable (GTK_TREE_SORTABLE (model), 2);
	}

	static void view_plain_mode_cb  (GtkToggleButton *button,
	                                 PackageSelector *pThis)
	{
		if (!gtk_toggle_button_get_active (button)) return;
		pThis->load_packages_view (&loadPackagesListAsPlain);
	}

	static void view_categories_mode_cb  (GtkToggleButton *button,
	                                      PackageSelector *pThis)
	{
		if (!gtk_toggle_button_get_active (button)) return;
		pThis->clear_search_entry (true);
		pThis->load_packages_view (&loadPackagesListByCategory);
	}

	static void view_patterns_mode_cb  (GtkToggleButton *button,
	                                    PackageSelector *pThis)
	{
		if (!gtk_toggle_button_get_active (button)) return;
		pThis->clear_search_entry (true);
//		pThis->load_packages_view (&loadPackagesListByPattern);  // TODO
	}

	void clear_search_entry (bool dont_load)
	{
		if (dont_load)
			g_signal_handlers_block_by_func (m_search_entry,
			             (gpointer) search_request_cb, this);
		gtk_entry_set_text (GTK_ENTRY (m_search_entry), "");
		if (dont_load)
			g_signal_handlers_unblock_by_func (m_search_entry,
			               (gpointer) search_request_cb, this);
	}

	static void search_activate_cb (GtkEntry *entry, PackageSelector *pThis)
	{
		IMPL
		if (pThis->search_timeout_id)
			g_source_remove (pThis->search_timeout_id);
		search_cb (pThis);
	}

	static void search_request_cb (GtkEditable *editable, PackageSelector *pThis)
	{
		IMPL
		// we'll make a delay for the actual search to wait for the user
		// to finish writting
		if (pThis->search_timeout_id)
			g_source_remove (pThis->search_timeout_id);
		pThis->search_timeout_id = g_timeout_add (1000, search_cb, pThis);
	}

	static gboolean search_cb (gpointer data)
	{
		IMPL
		PackageSelector *pThis = (PackageSelector *) data;
		pThis->search_timeout_id = 0;

		const gchar *query = gtk_entry_get_text (GTK_ENTRY (pThis->m_search_entry));
		bool plain_view = gtk_toggle_button_get_active (
		                      GTK_TOGGLE_BUTTON (pThis->m_plain_view));
		if (!plain_view)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pThis->m_plain_view), TRUE);
		pThis->m_searching = strlen (query) > 0;
		pThis->m_search_queries = YGUtils::splitString (query, ' ');

		// force a re-filter
		if (plain_view) {
			GtkTreeModel *available_model =
				gtk_tree_view_get_model (GTK_TREE_VIEW (pThis->m_available_view));
			GtkTreeModel *installed_model =
				gtk_tree_view_get_model (GTK_TREE_VIEW (pThis->m_installed_view));
			gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (available_model));
			gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (installed_model));
		}
		return FALSE;
	}

	GtkMenu *create_search_menu()
	{
		GtkWidget *menu = gtk_menu_new();
		append_option_item (menu, "Name", &name_opt);
		append_option_item (menu, "Summary", &summary_opt);
		append_option_item (menu, "Description", &descr_opt);
		append_option_item (menu, "RPM Provides", &provides_opt);
		append_option_item (menu, "RPM Requires", &requires_opt);
		gtk_widget_show_all (menu);
		return GTK_MENU (menu);
	}

	static void append_option_item (GtkWidget *menu, const char *label, bool *option)
	{
		GtkWidget *item = gtk_check_menu_item_new_with_label (label);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), *option);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		g_signal_connect (G_OBJECT (item), "toggled",
		                  G_CALLBACK (search_option_change_cb), option);
	}

	static void search_option_change_cb (GtkCheckMenuItem *item, bool *option)
	{ *option = item->active; }

	static gboolean is_package_installed (GtkTreeModel *model, GtkTreeIter *iter,
	                                      gpointer data)
	{
		PackageSelector *pThis = (PackageSelector *) data;
		gboolean visible;
		gtk_tree_model_get (model, iter, 3, &visible, -1);
		if (visible && pThis->m_searching) {
			ZyppSelectablePtr selectable;
			gtk_tree_model_get (model, iter, 0, &selectable, -1);
			visible = pThis->does_package_match (selectable);
		}
		return visible;
	}

	static gboolean is_package_available (GtkTreeModel *model, GtkTreeIter *iter,
	                                      gpointer data)
	{
		PackageSelector *pThis = (PackageSelector *) data;
		gboolean visible;
		gtk_tree_model_get (model, iter, 4, &visible, -1);
		if (visible && pThis->m_searching) {
			ZyppSelectablePtr selectable;
			gtk_tree_model_get (model, iter, 0, &selectable, -1);
			visible = pThis->does_package_match (selectable);
		}
		return visible;
	}

	gboolean does_package_match (ZyppSelectablePtr sel)
	{
		// TODO: add google syntax
		if (m_search_queries.empty())
			return true;
		ZyppObject obj = sel->theObj();
		for (list <string>::iterator it = m_search_queries.begin();
		     it != m_search_queries.end(); it++) {
			const string &str = *it;
			if (name_opt && YGUtils::contains (sel->name(), str))
				return TRUE;
			if (summary_opt && YGUtils::contains (obj->summary(), str))
				return TRUE;
			if (descr_opt && YGUtils::contains (obj->description(), str))
				return TRUE;
			if (provides_opt) {
				const zypp::CapSet &capSet = obj->dep (zypp::Dep::PROVIDES);
				for (zypp::CapSet::const_iterator it = capSet.begin();
				     it != capSet.end(); it++)
					if (YGUtils::contains (it->asString(), str))
						return TRUE;
			}
			if (requires_opt) {
				const zypp::CapSet &capSet = obj->dep (zypp::Dep::REQUIRES);
				for (zypp::CapSet::const_iterator it = capSet.begin();
				     it != capSet.end(); it++)
					if (YGUtils::contains (it->asString(), str))
						return TRUE;
			}
		}
		return FALSE;
	}

	// callbacks
	static bool getSelectedPackage (GtkTreeView *tree_view,
		ZyppSelectablePtr *package_sel, GtkTreePath **_path = 0)
	{
		IMPL
		GtkTreePath *path = 0;
		gtk_tree_view_get_cursor (tree_view, &path, NULL);
		if (path) {
			GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
			GtkTreeIter iter;
			gtk_tree_model_get_iter (model, &iter, path);
			gtk_tree_model_get (model, &iter, 0, package_sel, -1);
			if (_path)
				*_path = gtk_tree_model_filter_convert_path_to_child_path
				             (GTK_TREE_MODEL_FILTER (model), path);
			gtk_tree_path_free (path);
			return true;
		}
		return false;
	}

	void markSelectedPackage (GtkTreeView *view, bool available, bool installed)
	{
		IMPL
		ZyppSelectablePtr sel;
		GtkTreePath *path = NULL;
		if (getSelectedPackage (view, &sel, &path)) { // if some package selected
			if (sel && mark_selectable (sel, installed)) {
				// update model
				GtkTreeIter iter;
				gtk_tree_model_get_iter (m_packages_model, &iter, path);
				loadPackageRow (m_packages_model, &iter, sel);
			}
		}
		if (path)
			gtk_tree_path_free (path);
	}

	static void install_button_clicked_cb (GtkButton *button, PackageSelector *pThis)
	{
		IMPL
		pThis->markSelectedPackage (GTK_TREE_VIEW (pThis->m_available_view), false, true);
		gtk_widget_grab_focus (pThis->m_available_view);
	}

	static void remove_button_clicked_cb (GtkButton *button, PackageSelector *pThis)
	{
		IMPL
		pThis->markSelectedPackage (GTK_TREE_VIEW (pThis->m_installed_view), true, false);
		gtk_widget_grab_focus (pThis->m_installed_view);
	}

	static bool sync_tree_views_scroll (GtkTreeView *current_view, GtkTreeView *other_view,
	                                    GtkTreePath *current_path, bool select_it)
	{
		// TODO: should we check for installable/available objects here?

		// converts the path from one model to the other
		GtkTreePath *_path, *other_path;
		_path = gtk_tree_model_filter_convert_path_to_child_path (
			GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (current_view)), current_path);
		if (!_path)
			return false;
		other_path = gtk_tree_model_filter_convert_child_path_to_path (
			GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (other_view)), _path);
		gtk_tree_path_free (_path);
		if (!other_path)
			return false;

		GdkRectangle cell_rect, visible_rect;
		gtk_tree_view_get_cell_area (other_view, other_path, NULL, &cell_rect);
		gtk_tree_view_get_visible_rect (other_view, &visible_rect);
		int y = visible_rect.y + cell_rect.y;

		gtk_tree_view_get_cell_area (current_view, current_path, NULL, &cell_rect);
		y -= cell_rect.y;  // offset

		gtk_tree_view_expand_to_path (other_view, other_path);
		YGUtils::tree_view_smooth_scroll_to_point (other_view, 0, y);
		if (select_it)
			gtk_tree_selection_select_path (gtk_tree_view_get_selection (other_view), other_path);

		gtk_tree_path_free (other_path);
		return true;
	}

	static void package_clicked_cb (GtkTreeView *tree_view, PackageSelector *pThis)
	{
		IMPL
		ZyppSelectablePtr sel = 0;
		getSelectedPackage (tree_view, &sel);

		ZyppObject install_obj, available_obj;
		induceObjects (sel, install_obj, available_obj);

		bool install_view = tree_view == GTK_TREE_VIEW (pThis->m_installed_view);
		GtkTreeView *other_view = GTK_TREE_VIEW (install_view ?
			pThis->m_available_view : pThis->m_installed_view);

		// select and scroll to the package in the other view
		// (if it is listed there.)
		GtkTreePath *path = 0;
		gtk_tree_view_get_cursor (tree_view, &path, NULL);
 
		if (path && (
		    (install_obj && available_obj) || (!install_obj && !available_obj)
		    /* TODO: group flag would obsolete this last check */))
			sync_tree_views_scroll (tree_view, other_view, path, true);
		else  // set the other view as un-selected
			gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (other_view));

		if (path)
			gtk_tree_path_free (path);

		// show package information
		ZyppObject &obj = install_view ? install_obj : available_obj;
		pThis->m_information_widget->setPackage (sel, obj);

		// change the install/remove button label to upgrade, or etc
		GtkLabel *install_label = GTK_LABEL (pThis->m_install_label),
		         *remove_label = GTK_LABEL (pThis->m_remove_label);
		// remove label
		if (sel && sel->toInstall())
			gtk_label_set_text (remove_label, "_undo");
		else
			gtk_label_set_text (remove_label, "_remove");
		// install label
		if (sel && sel->toDelete())
			gtk_label_set_text (install_label, "_undo");
		else if (!install_obj)
			gtk_label_set_text (install_label, "_install");
		else if (available_obj) {
			if (install_obj->edition() < available_obj->edition())
				gtk_label_set_text (install_label, "_upgrade");
			else
				gtk_label_set_text (install_label, "_downgrade");
		}
		gtk_label_set_use_underline (install_label, TRUE);
		gtk_label_set_use_underline (remove_label, TRUE);
	}

	static void change_available_version_cb (YGtkCellRendererArrow *renderer,
		guint arrow_type, const gchar *_path, PackageSelector *pThis)
	{
		GtkTreeModel *model = gtk_tree_view_get_model (
			GTK_TREE_VIEW (pThis->m_available_view));
		GtkTreePath *path = gtk_tree_path_new_from_string (_path);
		GtkTreeIter it;
		gtk_tree_model_get_iter (model, &it, path);
		gtk_tree_path_free (path);

		GtkTreeIter iter;
		gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (model),
			&iter, &it);
		model = pThis->m_packages_model;

		ZyppSelectablePtr selectable;
		gtk_tree_model_get (model, &iter, 0, &selectable, -1);

		ZyppObject candidate = selectable->candidateObj();
		for (zypp::ui::Selectable::available_iterator it = selectable->availableBegin();
		     it != selectable->availableEnd(); it++) {
			int cmp = zypp::Edition::compare ((*it)->edition(), candidate->edition());
			if (arrow_type == GTK_ARROW_UP) {	// upgrade
				if (cmp > 0)
					candidate = *it;
			}
			else /*if (arrow_type == GTK_ARROW_DOWN)*/ {	// downgrade
				if (cmp < 0)
					candidate = *it;
			}
		}
		if (!selectable->setCandidate (candidate))
			y2warning ("Error: Could not %sgrade\n", arrow_type == GTK_ARROW_UP ? "up" : "down");
		pThis->loadPackageRow (model, &iter, selectable);
	}

	static gboolean view_scrolled_cb (GtkAdjustment *adj, PackageSelector *pThis)
	{
		// TODO: do some syncing scrolling
#if 0
		GtkTreeView *current_view = GTK_TREE_VIEW (
			g_object_get_data (G_OBJECT (adj), "view"));
		GtkTreeView *other_view = GTK_TREE_VIEW (
			GTK_WIDGET (current_view) == pThis->m_available_view ?
			pThis->m_installed_view : pThis->m_available_view);

		GtkTreePath *first_path;
		if (gtk_tree_view_get_visible_range (current_view, &first_path, NULL)) {
			return FALSE;

			sync_tree_views_scroll (current_view, other_view, first_path, false);
			gtk_tree_path_free (first_path);
		}
#endif
		return FALSE;
	}
};

// The ordinary package selector that displays all packages
class YGPackageSelector : public YPackageSelector, public YGWidget
{
	PackageSelector *m_package_selector;

	PatternSelector *m_pattern_selector;
	GtkWidget *m_patterns_widget;

public:
	YGPackageSelector (const YWidgetOpt &opt, YGWidget *parent)
		: YPackageSelector (opt),
		  YGWidget (this, parent, true, YGTK_TYPE_WIZARD, NULL)
	{
		setBorder (0);

		YGtkWizard *wizard = YGTK_WIZARD (getWidget());
		ygtk_wizard_set_header_icon (wizard, YGUI::ui()->currentWindow(),
			THEMEDIR "/icons/32x32/apps/yast-software.png");
		ygtk_wizard_set_next_button_label (wizard, "_Accept");
		ygtk_wizard_set_next_button_id (wizard, g_strdup ("accept"), g_free);
		g_signal_connect (G_OBJECT (getWidget()), "action-triggered",
		                  G_CALLBACK (wizard_action_cb), this);

		m_package_selector = new PackageSelector (opt.searchMode.value());
		m_pattern_selector = 0;

		if (opt.searchMode.value()) {
			m_package_selector->loadPackagesListAsPlain();
			setPackagesMode (false);
		}
		else {
			// display a pattern selector that will then toggle to
			// the package selector if the user wants to (on Customize...)
			m_pattern_selector = new PatternSelector();

			GtkWidget *buttons_box;
			m_patterns_widget = gtk_vbox_new (FALSE, 0);
			buttons_box = gtk_hbutton_box_new();
			gtk_box_pack_start (GTK_BOX (m_patterns_widget), m_pattern_selector->getWidget(),
			                    TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (m_patterns_widget), buttons_box,
			                    FALSE, FALSE, 8);
			gtk_widget_show_all (m_patterns_widget);

			g_object_ref (G_OBJECT (m_patterns_widget));
			gtk_object_sink (GTK_OBJECT (m_patterns_widget));

			setPatternsMode();
		}
	}

	virtual ~YGPackageSelector()
	{
		if (m_pattern_selector) {
			gtk_widget_destroy (m_patterns_widget);
			g_object_unref (G_OBJECT (m_patterns_widget));
			delete m_pattern_selector;
		}
		delete m_package_selector;
	}

protected:
	void setPackagesMode (bool use_back_button)
	{
		IMPL
		YGtkWizard *wizard = YGTK_WIZARD (getWidget());
		ygtk_wizard_set_header_text (wizard, YGUI::ui()->currentWindow(),
		                             "Package Selector");
		ygtk_wizard_set_help_text (wizard,
			"Two pools are presented; one with the available software, the other "
			"with the installed one. To install software you choose a package "
			"from the install pool and press Install. Similar method for removal "
			"of software. When you are done press the Accept button.<br>"
			"Information on a given package is displayed on the Package Information "
			"expander at the bottom which may be enlarged.<br>"
			"A categories view of the software is possible, as well as searching "
			"for a given package."
		);

		if (use_back_button) {
			ygtk_wizard_set_back_button_label (wizard, "_Back");
			ygtk_wizard_set_back_button_id (wizard, g_strdup ("back"), g_free);
			ygtk_wizard_set_abort_button_label (wizard, "");
		}
		else {
			ygtk_wizard_set_abort_button_label (wizard, "_Cancel");
			ygtk_wizard_set_abort_button_id (wizard, g_strdup ("cancel"), g_free);
			ygtk_wizard_set_back_button_label (wizard, "");
		}

		ygtk_wizard_set_child (wizard, m_package_selector->getWidget());
	}

	void setPatternsMode()
	{
		IMPL
		YGtkWizard *wizard = YGTK_WIZARD (getWidget());
		ygtk_wizard_set_header_text (wizard, YGUI::ui()->currentWindow(),
		                             "Patterns Selector");
		ygtk_wizard_set_help_text (wizard,
			"Patterns are bundles of software that you may install by pressing them "
			"and then clicking Accept.<br>"
			"If you are an experienced user, you may press Customize to choose "
			"from individual packages."
		);

		ygtk_wizard_set_back_button_label (wizard, "_Customize...");
		ygtk_wizard_set_back_button_id (wizard, g_strdup ("customize"), g_free);
		ygtk_wizard_set_abort_button_label (wizard, "_Cancel");
		ygtk_wizard_set_abort_button_id (wizard, g_strdup ("cancel"), g_free);

		ygtk_wizard_set_child (wizard, m_patterns_widget);
	}

	static void wizard_action_cb (YGtkWizard *wizard, gpointer id,
	                              gint id_type, YGPackageSelector *pThis)
	{
		IMPL
		const gchar *action = (gchar *) id;

		// patterns main window specific
		if (!strcmp (action, "customize")) {
			// apply patterns
			pThis->m_package_selector->loadPackagesListAsPlain();
			pThis->setPackagesMode (true);
		}
		else if (!strcmp (action, "back"))
			pThis->setPatternsMode();
		// generic
		else if (!strcmp (action, "accept")) {
			if (zyppPool().diffState <zypp::Package> ()) {
				// in case of changes, check problems, ask for confirmation and JFDI.
				if (pThis->confirmChanges() && solveProblems()) {
					y2milestone ("Closing PackageSelector with 'accept'");
					YGUI::ui()->sendEvent (new YMenuEvent (YCPSymbol ("accept")));
				}
			}
			else
				YGUI::ui()->sendEvent (new YCancelEvent());
		}
		else if (!strcmp (action, "cancel")) {
			y2milestone ("Closing PackageSelector with 'cancel'");
			YGUI::ui()->sendEvent (new YCancelEvent());
		}
	}

	// helper function for confirmChanges
	static bool isInstalled (const string &package)
	{
		for (ZyppPool::const_iterator it = zyppPool().byKindBegin <zypp::Package>();
		      it != zyppPool().byKindEnd <zypp::Package>(); it++) {
			if ((*it)->hasInstalledObj() || (*it)->toInstall()) {
				if (package == (*it)->name())
					return true;
				ZyppObject obj = (*it)->theObj();
				const zypp::CapSet &caps = obj->dep (zypp::Dep::PROVIDES);
				for (zypp::CapSet::const_iterator jt = caps.begin();
				      jt != caps.end(); jt++)
					if (package == jt->asString())
						return true;
			}
		}
		return false; // doesn't even exist
	}

	bool confirmChanges()
	{
		IMPL
		if (zyppPool().empty <zypp::Package> ())
			return false;

		GtkWidget *dialog = gtk_dialog_new_with_buttons ("Changes Summary",
			YGUI::ui()->currentWindow(),
			GtkDialogFlags (GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, "C_onfirm", GTK_RESPONSE_ACCEPT, NULL);

		GtkWidget *install_label, *remove_label, *install_view, *remove_view;
		install_label = gtk_label_new ("To install:");
		remove_label  = gtk_label_new ("To remove:");
		install_view  = gtk_tree_view_new();
		remove_view   = gtk_tree_view_new();

		GtkWidget *install_window, *remove_window;
		install_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (install_window),
		                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type
			(GTK_SCROLLED_WINDOW (install_window), GTK_SHADOW_IN);
		gtk_container_add (GTK_CONTAINER (install_window), install_view);
		remove_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (remove_window),
		                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type
			(GTK_SCROLLED_WINDOW (remove_window), GTK_SHADOW_IN);
		gtk_container_add (GTK_CONTAINER (remove_window), remove_view);

		GtkBox *vbox = GTK_BOX (GTK_DIALOG(dialog)->vbox);
		gtk_box_pack_start (vbox, install_label,  FALSE, FALSE, 6);
		gtk_box_pack_start (vbox, install_window, TRUE, TRUE, 6);
		gtk_box_pack_start (vbox, remove_label,  FALSE, FALSE, 6);
		gtk_box_pack_start (vbox, remove_window, TRUE, TRUE, 6);

		// create model
		{
			GtkTreeStore *install_store = gtk_tree_store_new (1, G_TYPE_STRING);
			GtkTreeStore *remove_store = gtk_tree_store_new (1, G_TYPE_STRING);

			// install view
			GtkTreeViewColumn *column;
			column = gtk_tree_view_column_new_with_attributes ("Install packages",
				gtk_cell_renderer_text_new(), "text", 0, NULL);
			gtk_tree_view_append_column (GTK_TREE_VIEW (install_view), column);

			gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (install_view), FALSE);
			gtk_tree_view_set_model (GTK_TREE_VIEW (install_view),
			                         GTK_TREE_MODEL (install_store));
			g_object_unref (G_OBJECT (install_store));
			gtk_tree_selection_set_mode (gtk_tree_view_get_selection (
				GTK_TREE_VIEW (install_view)), GTK_SELECTION_NONE);;

			// remove view
			column = gtk_tree_view_column_new_with_attributes ("Remove packages",
				gtk_cell_renderer_text_new(), "text", 0, NULL);
			gtk_tree_view_append_column (GTK_TREE_VIEW (remove_view), column);

			gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (remove_view), FALSE);
			gtk_tree_view_set_model (GTK_TREE_VIEW (remove_view),
			                         GTK_TREE_MODEL (remove_store));
			g_object_unref (G_OBJECT (remove_store));
			gtk_tree_selection_set_mode (gtk_tree_view_get_selection (
				GTK_TREE_VIEW (remove_view)), GTK_SELECTION_NONE);;

			// construct model
			for (ZyppPool::const_iterator it = zyppPool().byKindBegin <zypp::Package>();
			     it != zyppPool().byKindEnd <zypp::Package>(); it++)
			{
				ZyppSelectable selectable = *it;
				if (!selectable)
					continue;

				GtkTreeStore *store;
				if (selectable->toInstall())
					store = install_store;
				else if (selectable->toDelete())
					store = remove_store;
				else continue;

				GtkTreeIter iter;
				gtk_tree_store_append (store, &iter, NULL);
				gtk_tree_store_set (store, &iter, 0, selectable->name().c_str(), -1);

				// show dependencies -- we just ask Zypp for dependencies in the form
				// of a string (which is the only way to do it anyway).
				if (store == install_store) {
						GtkTreeIter dep_iter;
						ZyppObject object = selectable->theObj();
						const zypp::CapSet &capSet = object->dep (zypp::Dep::REQUIRES);
						for (zypp::CapSet::const_iterator it = capSet.begin();
						     it != capSet.end(); it++) {
							// don't show if it is already installed
							if (isInstalled (it->asString()))
								continue;

							gtk_tree_store_append (store, &dep_iter, &iter);
							gtk_tree_store_set (store, &dep_iter,
							                    0, it->asString().c_str(), -1);
						}
					}

				// show packages that require this -- we will need to iterate through
				// all the packages... (not very accurate)
				else {
					GtkTreeIter req_iter;

					for (ZyppPool::const_iterator it =
				         zyppPool().byKindBegin <zypp::Package>();
					     it != zyppPool().byKindEnd <zypp::Package>(); it++) {
						// only show if it is installed
						if ((*it)->hasInstalledObj() || (*it)->toInstall()) {
							ZyppSelectable dep_selectable = *it;
							ZyppObject dep_object = dep_selectable->theObj();

							const zypp::CapSet &capSet = dep_object->dep (zypp::Dep::REQUIRES);
							for (zypp::CapSet::const_iterator it = capSet.begin();
							     it != capSet.end(); it++) {
								if (it->asString() == selectable->name()) {
									gtk_tree_store_append (store, &req_iter, &iter);
									gtk_tree_store_set (store, &req_iter,
									                    0, dep_selectable->name().c_str(), -1);
									break;
								}
							}
						}
					}
				}
			}
		}

		gtk_window_set_default_size (GTK_WINDOW (dialog), 300, 400);
		gtk_widget_show_all (dialog);

		bool confirmed = gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT;
		gtk_widget_destroy (dialog);

		return confirmed;
	}

	YGWIDGET_IMPL_COMMON
};
#endif /*DISABLE_PACKAGE_SELECTOR*/

YWidget *
YGUI::createPackageSelector (YWidget *parent, YWidgetOpt &opt,
                             const YCPString &floppyDevice)
{
	// TODO: floppyDevice really needed?
#ifndef DISABLE_PACKAGE_SELECTOR
	if (opt.youMode.value())
		return new YGPatchSelector (opt, YGWidget::get (parent));
	return new YGPackageSelector (opt, YGWidget::get (parent));
#else
	return NULL;
#endif
}

YWidget *
YGUI::createPkgSpecial (YWidget *parent, YWidgetOpt &opt,
                        const YCPString &subwidget)
{
	// same as Qt's
	// TODO: check what this is supposed to do (something for ncurses maybe?)
	y2error ("The GTK+ UI does not support PkgSpecial subwidgets!");
	return 0;
}
