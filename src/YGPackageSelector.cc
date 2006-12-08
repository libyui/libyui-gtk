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
typedef zypp::ResObject*          ZyppObjectPtr;
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
static bool mark_selectable (ZyppSelectablePtr selectable,
    bool install /*or remove*/, ZyppObjectPtr object = NULL)
{
	zypp::ui::Status status = selectable->status();

	if (install && !acceptLicense (selectable))
		return false;

	if (install) {
		if (status == zypp::ui::S_Del)
			status = zypp::ui::S_KeepInstalled;
		else {
			if (object && !selectable->setCandidate (object)) {
				y2warning ("YGPackageSelector: package version not available?!");
				return false;
			}
			if (status == zypp::ui::S_KeepInstalled)
				status = zypp::ui::S_Update;
			else
				status = zypp::ui::S_Install;
		}
	}
	else /*remove*/ {
		if (status == zypp::ui::S_NoInst)
			;  // do nothing
		if (status == zypp::ui::S_Install)
			status = zypp::ui::S_NoInst;
		else
			status = zypp::ui::S_Del;
	}

	return selectable->set_status (status);
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

#define INFORMATION_HEIGHT 120

// Expander widget with the package information
class PackageInformation
{
	GtkWidget *m_widget;
	// Information text
	GtkWidget *m_description_text, *m_filelist_text, *m_history_text;
	GtkWidget *m_scrolled_window;

public:
	PackageInformation (const char *expander_text, bool only_description)
	{
		// TODO: set white color on expanders
		GtkWidget *padding_widget, *vbox, *view_port;
		m_widget = gtk_expander_new (expander_text);

		padding_widget = gtk_hbox_new (FALSE, 0);

		view_port = gtk_viewport_new (NULL, NULL);
		gtk_viewport_set_shadow_type (GTK_VIEWPORT (view_port), GTK_SHADOW_NONE);
		m_scrolled_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (m_scrolled_window),
		                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_add_with_viewport
			(GTK_SCROLLED_WINDOW (m_scrolled_window), view_port);

		vbox = gtk_vbox_new (FALSE, 0);
		gtk_container_add (GTK_CONTAINER (view_port), vbox);
		gtk_box_pack_start (GTK_BOX (padding_widget), m_scrolled_window,
		                    TRUE, TRUE, 15);
		gtk_container_add (GTK_CONTAINER (m_widget), padding_widget);

		m_description_text = ygtk_richtext_new ();
		ygtk_richtext_set_text (YGTK_RICHTEXT (m_description_text),
		                        "<i>(no package selected)</i>", FALSE, TRUE);
		gtk_box_pack_start (GTK_BOX (vbox), m_description_text, FALSE, FALSE, 0);

		if (only_description) {
			m_filelist_text = NULL;
			m_history_text = NULL;
		}
		else {
			GtkWidget *filelist_expander, *history_expander;
//			GdkColor white = { 0, 255 << 8, 255 << 8, 255 << 8 };

			filelist_expander = gtk_expander_new ("File listing");
			m_filelist_text = ygtk_richtext_new ();
			gtk_container_add (GTK_CONTAINER (filelist_expander), m_filelist_text);
			gtk_box_pack_start (GTK_BOX (vbox), filelist_expander, FALSE, FALSE, 0);

			history_expander = gtk_expander_new ("Packaging History");
			m_history_text = ygtk_richtext_new ();
			gtk_container_add (GTK_CONTAINER (history_expander), m_history_text);
			gtk_box_pack_start (GTK_BOX (vbox), history_expander, FALSE, FALSE, 0);

//			g_signal_connect (G_OBJECT (vbox), "size-request",
//			                  G_CALLBACK (size_changed_cb), m_scrolled_window);
		}

		gtk_widget_set_size_request (m_scrolled_window, -1, INFORMATION_HEIGHT);
	}

	GtkWidget *getWidget()
	{ return m_widget; }

	void setPackage (ZyppSelectablePtr selectable, ZyppObject object)
	{
		string description = "<i>(no package selected)</i>", file_list, history;
		if (selectable) {
			description = "<p><b>" + selectable->name() + "</b> - "
			              + object->summary() + "</p>"
//FIXME: break authors nicely.
			 + "<p>" + object->description() + "</p>";

			if (m_filelist_text && m_history_text) {
				ZyppPackage package = tryCastToZyppPkg (object);

				const std::list <string> &filenames = package->filenames();
				for (std::list <string>::const_iterator it = filenames.begin();
				     it != filenames.end(); it++)
					file_list += (*it) + "\n";

				const std::list <zypp::ChangelogEntry> &changelog = package->changelog();
				for (std::list <zypp::ChangelogEntry>::const_iterator it = changelog.begin();
				     it != changelog.end(); it++)
					history += "<p><i>" + it->date().asString() + "</i> "
					           "<b>" + it->author() + "</b></p>"
					           "<p>" + it->text() + "</p>";
			}
		}

		ygtk_richtext_set_text (YGTK_RICHTEXT (m_description_text),
		                        description.c_str(), FALSE, TRUE);
		if (m_filelist_text && m_history_text) {
			ygtk_richtext_set_text (YGTK_RICHTEXT (m_filelist_text),
			                        file_list.c_str(), TRUE, TRUE);
			ygtk_richtext_set_text (YGTK_RICHTEXT (m_history_text),
			                        history.c_str(), FALSE, TRUE);
		}

//		size_changed_cb (0, 0, m_scrolled_window);
	}

	// This forces GtkScrolledWindow to set "new" scrollbars to reflect
	// a new box size (due to expanders being activated)
	// FIXME: ain't working all that well...
/*
	static void size_changed_cb (GtkWidget *widget, GtkRequisition *requisition,
	                             GtkWidget *view)
	{
		gtk_scrolled_window_set_vadjustment (GTK_SCROLLED_WINDOW (view), NULL);
	}
*/
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

	// Model with the packages list
	GtkTreeStore *m_packages_store;

	// The GtkTreeView widgets
	GtkWidget *m_installed_view, *m_available_view;

	// Package information widget
	PackageInformation *m_information_widget;

	// Search gizmos
	GtkWidget *m_search_entry;
	guint search_timeout_id;
	GtkWidget *m_categories_view_check;

public:
	PackageSelector (bool enablePatterns = true)
	{
		IMPL
		m_widget = gtk_vbox_new (FALSE, 0);

		GtkWidget *packages_hbox = gtk_hbox_new (FALSE, 0);

		GtkWidget *available_box, *installed_box;
		// Create the packages model
		// models' columns: available package name (string), installed package name
		// (string), has available package (boolean), has installed package (boolean)
		// selectable object (pointer), available resolvable object (pointer),
		// installed resolvable object (pointer) = 7
		m_packages_store = gtk_tree_store_new (7, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_POINTER, G_TYPE_POINTER,
			G_TYPE_POINTER);

		available_box = createListWidget ("<b>Available Software:</b>",
		                                  "gtk-cdrom", m_available_view,
		                                  is_package_available);
		installed_box = createListWidget ("<b>Installed Software:</b>",
		                                  "computer", m_installed_view,
		                                  is_package_installed);
		g_object_unref (G_OBJECT (m_packages_store));

		GtkWidget *selection_buttons_vbox, *install_button, *remove_button;
		selection_buttons_vbox = gtk_vbox_new (FALSE, 0);

		install_button = createArrowButton ("_install", GTK_ARROW_RIGHT);
		remove_button = createArrowButton ("_remove", GTK_ARROW_LEFT);

		gtk_box_pack_start (GTK_BOX (selection_buttons_vbox), install_button,
		                                                      TRUE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (selection_buttons_vbox), remove_button,
		                                                      TRUE, FALSE, 0);
		gtk_container_set_border_width (GTK_CONTAINER (selection_buttons_vbox), 6);

		gtk_box_pack_start (GTK_BOX (packages_hbox), available_box, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (packages_hbox), selection_buttons_vbox,
		                                             FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (packages_hbox), installed_box, TRUE, TRUE, 0);

		GtkWidget *categories_hbox;
		categories_hbox = gtk_hbox_new (FALSE, 0);
		m_categories_view_check = gtk_check_button_new_with_mnemonic ("View in categories");
		gtk_box_pack_start (GTK_BOX (categories_hbox), m_categories_view_check,
		                    TRUE, TRUE, 0);

		GtkWidget *search_hbox, *search_label;
		search_hbox = gtk_hbox_new (FALSE, 0);
		search_label = gtk_label_new_with_mnemonic ("_Search:");
		m_search_entry = gtk_entry_new();
		gtk_label_set_mnemonic_widget (GTK_LABEL (search_label), m_search_entry);
		gtk_box_pack_start (GTK_BOX (search_hbox), search_label, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (search_hbox), m_search_entry, TRUE, TRUE, 4);
		search_timeout_id = 0;
		g_signal_connect (G_OBJECT (m_search_entry), "changed",
		                  G_CALLBACK (search_request_cb), this);

		m_information_widget = new PackageInformation ("Package Information", false);
		GtkWidget *pkg_info_widget = m_information_widget->getWidget();

		gtk_box_pack_start (GTK_BOX (m_widget), packages_hbox, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_widget), categories_hbox, FALSE, FALSE, 12);
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
		g_signal_connect (G_OBJECT (m_categories_view_check), "toggled",
		                  G_CALLBACK (toggle_packages_view_cb), this);

		g_signal_connect (G_OBJECT (m_installed_view), "cursor-changed",
		                  G_CALLBACK (package_clicked_cb), this);
		g_signal_connect (G_OBJECT (m_available_view), "cursor-changed",
		                  G_CALLBACK (package_clicked_cb), this);
	}

	// create widgets functions to cut down on code
	GtkWidget *createListWidget (const char *header, const char *icon,
		GtkWidget *&list, GtkTreeModelFilterVisibleFunc visible_packages)
	{
		GtkWidget *vbox, *header_hbox, *image, *label, *scrolled_window;

		vbox = gtk_vbox_new (FALSE, 0);

		scrolled_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_shadow_type
			(GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		list = gtk_tree_view_new();
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list), FALSE);
		gtk_container_add (GTK_CONTAINER (scrolled_window), list);

		header_hbox = gtk_hbox_new (FALSE, 0);
		image = gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_BUTTON);
		label = gtk_label_new_with_mnemonic (header);
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
		gtk_label_set_mnemonic_widget (GTK_LABEL (label), list);
		gtk_box_pack_start (GTK_BOX (header_hbox), image, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (header_hbox), label, FALSE, FALSE, 4);

		gtk_box_pack_start (GTK_BOX (vbox), header_hbox, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 4);

		// Create a model for the GtkTreeView. This will be a filter that gets
		// m_packages_store installed over it so it only shows the appropriate
		// packages (installed or available).
		GtkTreeModel *filter;
		GtkTreeViewColumn *column;
		filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (m_packages_store), NULL);
		gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter),
		                                        visible_packages, this, NULL);
		column = gtk_tree_view_column_new_with_attributes (header,
			gtk_cell_renderer_text_new(), "text", 0, NULL);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

		gtk_tree_view_set_model (GTK_TREE_VIEW (list), filter);
		g_object_unref (G_OBJECT (filter));

		return vbox;
	}

	GtkWidget *createArrowButton (const char *label, GtkArrowType arrow_type)
	{
		GtkWidget *button, *box = gtk_hbox_new (FALSE, 0);
		GtkWidget *arrow = gtk_arrow_new (arrow_type, GTK_SHADOW_OUT);
		gtk_container_add (GTK_CONTAINER (box), arrow);
		gtk_container_add (GTK_CONTAINER (box), gtk_label_new_with_mnemonic (label));
		gtk_box_set_child_packing (GTK_BOX (box), arrow, FALSE, TRUE, 0,
			arrow_type == GTK_ARROW_LEFT ? GTK_PACK_START : GTK_PACK_END);

		button = gtk_button_new();
		gtk_container_add (GTK_CONTAINER (button), box);
		return button;
	}

	void loadPackagesListAsPlain()
	{
		gtk_tree_store_clear (m_packages_store);

		GtkTreeIter iter;
		for (ZyppPool::const_iterator it = zyppPool().byKindBegin <zypp::Package>();
		     it != zyppPool().byKindEnd <zypp::Package>(); it++)
		{
			ZyppSelectable selectable = *it;
			gtk_tree_store_append (m_packages_store, &iter, NULL);
			loadPackageRow (&iter, selectable);
		}
	}

	void loadPackagesListByCategory()
	{
		gtk_tree_store_clear (m_packages_store);

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
							gtk_tree_model_get_iter (GTK_TREE_MODEL (m_packages_store),
							                         &parent_iter, path);
							gtk_tree_store_append (m_packages_store, &iter, &parent_iter);
						}
						else  // create at root
							gtk_tree_store_append (m_packages_store, &iter, NULL);

						gtk_tree_store_set (m_packages_store, &iter,
							0, node.c_str(), 1, node.c_str(), 2, TRUE, 3, TRUE,
							4, NULL, 5, NULL, 6, NULL, -1);

						path = gtk_tree_model_get_path (
							GTK_TREE_MODEL (m_packages_store), &iter);
						tree [node] = path;
					}
					else // exists
						path = it->second;
				}
			}

			if (!path)  // package has no group
				continue;

			gtk_tree_model_get_iter (GTK_TREE_MODEL (m_packages_store),
			                         &parent_iter, path);
			gtk_tree_store_append (m_packages_store, &iter, &parent_iter);
			loadPackageRow (&iter, selectable);
		}

		// free GtkTreePaths
		for (map <string, GtkTreePath*>::iterator it = tree.begin();
		     it != tree.end(); it++)
			gtk_tree_path_free (it->second);
	}

	void loadPackageRow (GtkTreeIter *iter, ZyppSelectable selectable)
	{
		ZyppObject available, install;
		zypp::ui::Status status = selectable->status();
		switch (status) {
			case zypp::ui::S_Install:
			case zypp::ui::S_Update:
				available = NULL;
				install = selectable->candidateObj();
				break;
			case zypp::ui::S_Del:
				available = selectable->installedObj();
				install = NULL;
				break;
			default:
				available = selectable->candidateObj();
				install = selectable->installedObj();
				if (available && install &&
				    available->edition() == install->edition())
					available = NULL;
				break;
		}

		string availableName = selectable->name(),
		       installedName = selectable->name();
		if (available && install) {
			availableName += " (" + available->edition().version() + ")";
			installedName += " (" + install->edition().version() + ")";
		}
		else if (status == zypp::ui::S_Update)
			installedName += " (" + install->edition().version() + ")";

		gtk_tree_store_set (m_packages_store, iter,
			0, availableName.c_str(), 1, installedName.c_str(),
			2, available != 0, 3, install != 0,
			4, get_pointer (selectable), 5, get_pointer (available),
			6, get_pointer (install), -1);
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
printf ("search!\n");
		PackageSelector *pThis = (PackageSelector *) data;
		pThis->search_timeout_id = 0;

		// force a re-filter
		GtkTreeModel *available_model =
			gtk_tree_view_get_model (GTK_TREE_VIEW (pThis->m_available_view));
		GtkTreeModel *installed_model =
			gtk_tree_view_get_model (GTK_TREE_VIEW (pThis->m_installed_view));
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (available_model));
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (installed_model));

		string search = gtk_entry_get_text (GTK_ENTRY (pThis->m_search_entry));
		if (!search.empty())
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (pThis->m_categories_view_check), FALSE);
		return FALSE;
	}

	static gboolean is_package_installed (GtkTreeModel *model, GtkTreeIter *iter,
	                                      gpointer data)
	{
		gboolean visible;
		gtk_tree_model_get (model, iter, 3, &visible, -1);
		if (visible)
			visible = show_package (model, iter, (PackageSelector *) data);
		return visible;
	}

	static gboolean is_package_available (GtkTreeModel *model, GtkTreeIter *iter,
	                                      gpointer data)
	{
		gboolean visible;
		gtk_tree_model_get (model, iter, 2, &visible, -1);
		if (visible)
			visible = show_package (model, iter, (PackageSelector *) data);
		return visible;
	}

	// common stuff for is_package_installed and is_package_available
	// used for search and to not show categories with no children
	static gboolean show_package (GtkTreeModel *model, GtkTreeIter *iter,
	                              PackageSelector *pThis)
	{
		ZyppSelectablePtr selectable = 0;
		gtk_tree_model_get (model, iter, 4, &selectable, -1);
		if (!selectable)
			return true;  // group
#if 0
// TODO: don't show empty nodes
// A bit harder than I thouht -- we need to go iterate
// through all children and see if they don't have
// packages either -- consider caching this.
		if (!selectable)  // this means it's a category
			if (!gtk_tree_model_iter_has_child (model, iter))
				return FALSE;
#endif

		// search
		gboolean visible = TRUE;
		string search = gtk_entry_get_text (GTK_ENTRY (pThis->m_search_entry));
		if (!search.empty()) {
			if (!YGUtils::contains (selectable->name(), search)) {
				// check also description and rpm provides
				ZyppObject object = selectable->theObj();
				if (!YGUtils::contains (object->summary(), search) &&
				    !YGUtils::contains (object->description(), search)) {
					// test rpm-provides
					const zypp::CapSet &capSet = object->dep (zypp::Dep::PROVIDES);
					for (zypp::CapSet::const_iterator it = capSet.begin();
					     it != capSet.end(); it++)
						if ((visible = YGUtils::contains (it->asString(), search)) == TRUE)
							break;
				}
			}
		}
		return visible;
	}

	// callbacks
	static void toggle_packages_view_cb  (GtkToggleButton *button,
	                                      PackageSelector *pThis)
	{
		IMPL
		if (gtk_toggle_button_get_active (button))
			pThis->loadPackagesListByCategory();
		else
			pThis->loadPackagesListAsPlain();
	}

	static bool getSelectedPackage (GtkTreeView *tree_view,
		ZyppSelectablePtr *package_sel, ZyppObjectPtr *available_obj = 0,
		ZyppObjectPtr *installed_obj = 0, GtkTreePath **_path = 0)
	{
		IMPL
		GtkTreePath *path = 0;
		gtk_tree_view_get_cursor (tree_view, &path, NULL);
		if (path) {
			GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
			GtkTreeIter iter;
			gtk_tree_model_get_iter (model, &iter, path);
			gtk_tree_model_get (model, &iter, 4, package_sel, -1);
			if (available_obj)
				gtk_tree_model_get (model, &iter, 5, available_obj, -1);
			if (installed_obj)
				gtk_tree_model_get (model, &iter, 6, installed_obj, -1);

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
		ZyppObjectPtr obj;
		GtkTreePath *path;
		if (!getSelectedPackage (view, &sel, &obj, NULL, &path))
			return;  // no package selected

		if (!sel || !mark_selectable (sel, installed, obj))
			return;

		// update model
		GtkTreeIter iter;
		gtk_tree_model_get_iter (GTK_TREE_MODEL (m_packages_store), &iter, path);
		loadPackageRow (&iter, sel);
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

	static void package_clicked_cb (GtkTreeView *tree_view, PackageSelector *pThis)
	{
		IMPL
		ZyppSelectablePtr sel;
		ZyppObjectPtr avail_obj, inst_obj;
		getSelectedPackage (tree_view, &sel, &avail_obj, &inst_obj);
		ZyppObjectPtr obj = (GTK_WIDGET (tree_view) == pThis->m_available_view)
		                    ? avail_obj : inst_obj;
		pThis->m_information_widget->setPackage (sel, obj);

		// set the other view as un-selected
		if (tree_view == GTK_TREE_VIEW (pThis->m_installed_view))
			gtk_tree_selection_unselect_all (
				gtk_tree_view_get_selection (GTK_TREE_VIEW (pThis->m_available_view)));
		else
			gtk_tree_selection_unselect_all (
				gtk_tree_view_get_selection (GTK_TREE_VIEW (pThis->m_installed_view)));
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
