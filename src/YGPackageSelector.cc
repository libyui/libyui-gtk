/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/*
  Textdomain	"yast2-gtk"
*/

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YGi18n.h"
#include "YGDialog.h"
#include "YPackageSelector.h"
#include "ygtkhtmlwrap.h"
#include "ygtkwizard.h"
#include "ygtkcellrendererarrow.h"
#include "ygtkratiobox.h"
#include "ygtkfindentry.h"

#include <map>

//#define DISABLE_PACKAGE_SELECTOR

#ifndef DISABLE_PACKAGE_SELECTOR
#include <zypp/ZYppFactory.h>
#include <zypp/ResObject.h>
#include <zypp/ResPoolProxy.h>
#include <zypp/ui/Selectable.h>
#include <zypp/Patch.h>
#include <zypp/Selection.h>
#include <zypp/Package.h>
#include <zypp/Pattern.h>
#include <zypp/Language.h>
#include <zypp/Product.h>
#ifdef PRE_ZYPP_3
#include <zypp/Source.h>
#include <zypp/SourceManager.h>
#else
#include <zypp/Repository.h>
#include <zypp/RepoManager.h>
#endif

/* We should consider linking to libgnome and use gnome_url_show(url) here,
   or at least do some path finding. */
#define FILEMANAGER_EXEC "/usr/bin/nautilus"
inline bool FILEMANAGER_PRESENT()
{ return g_file_test (FILEMANAGER_EXEC, G_FILE_TEST_IS_EXECUTABLE); }
inline void FILEMANAGER_LAUNCH (const char *path)
{ system ((string (FILEMANAGER_EXEC) + " " + path + " &").c_str()); }

// some typedefs and functions to short Zypp names
typedef zypp::ResPoolProxy ZyppPool;
inline ZyppPool zyppPool() { return zypp::getZYpp()->poolProxy(); }
typedef zypp::ui::Selectable::Ptr ZyppSelectable;
typedef zypp::ui::Selectable*     ZyppSelectablePtr;
typedef zypp::ResObject::constPtr ZyppObject;
typedef zypp::Package::constPtr   ZyppPackage;
typedef zypp::Patch::constPtr     ZyppPatch;
typedef zypp::Pattern::constPtr   ZyppPattern;
typedef zypp::Language::constPtr  ZyppLanguage;
inline ZyppPackage tryCastToZyppPkg (ZyppObject obj)
	{ return zypp::dynamic_pointer_cast <const zypp::Package> (obj); }
inline ZyppPatch tryCastToZyppPatch (ZyppObject obj)
	{ return zypp::dynamic_pointer_cast <const zypp::Patch> (obj); }
inline ZyppPattern tryCastToZyppPattern (ZyppObject obj)
	{ return zypp::dynamic_pointer_cast <const zypp::Pattern> (obj); }
inline ZyppLanguage tryCastToZyppLanguage (ZyppObject obj)
	{ return zypp::dynamic_pointer_cast <const zypp::Language> (obj); }

static zypp::Text
fastGetSummary (ZyppObject obj)
{
    static std::map<std::string, zypp::Text> name_to_summary;
    std::string &summary = name_to_summary[ obj->name() ];
    if (summary.length() <= 0)
        summary = obj->summary();
    return summary;
}

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

	GtkWidget *dialog = gtk_dialog_new_with_buttons (_("License Agreement"),
		YGUI::ui()->currentWindow(), GTK_DIALOG_MODAL,
		_("_Reject"), GTK_RESPONSE_REJECT, _("_Accept"), GTK_RESPONSE_ACCEPT, NULL);

	GtkWidget *license_view, *license_window;

    license_view = ygtk_html_wrap_new();
    ygtk_html_wrap_set_text (license_view, license.c_str());

	license_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (license_window),
	                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type
		(GTK_SCROLLED_WINDOW (license_window), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (license_window), license_view);

	GtkBox *vbox = GTK_BOX (GTK_DIALOG(dialog)->vbox);
	gtk_box_pack_start (vbox, license_window, TRUE, TRUE, 6);

    const char *print_text = _("If you would like to print this license, check the EULA.txt file on the first media");
    GtkLabel *print_label = GTK_LABEL (gtk_label_new (print_text));
    gtk_box_pack_start (vbox, GTK_WIDGET (print_label), FALSE, TRUE, 2);

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
static bool mark_selectable (ZyppSelectablePtr selectable, bool install /*or remove*/)
{
	zypp::ui::Status status = selectable->status();

	if (install) {
		if (!selectable->hasCandidateObj()) {
			y2warning ("YGPackageSelector: no package on repository to install");
			return false;
		}
		if (!acceptLicense (selectable))
			return false;
		if (status == zypp::ui::S_Del)
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
		else if (status == zypp::ui::S_Update)
			status = zypp::ui::S_KeepInstalled;
		else
			status = zypp::ui::S_Del;
	}

#if 0	// debug
	const char *name = selectable->name().c_str();
	switch (status) {
		case zypp::ui::S_KeepInstalled:
			y2milestone ("keep %s installed\n", name);
			break;
		case zypp::ui::S_Update:
			y2milestone ("update %s\n", name);
			break;
		case zypp::ui::S_Install:
			y2milestone ("install %s\n", name);
			break;
		case zypp::ui::S_Del:
			y2milestone ("remove %s\n", name);
			break;
		case zypp::ui::S_NoInst:
			y2milestone ("don't install %s\n", name);
			break;
		default:
			y2milestone ("error: unknown action: should not happen\n");
			break;
	}
#endif

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

	GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Resolve Problems"),
		YGUI::ui()->currentWindow(), GTK_DIALOG_MODAL,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, _("C_onfirm"), GTK_RESPONSE_ACCEPT, NULL);

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

	column = gtk_tree_view_column_new_with_attributes (_("Problems"),
		gtk_cell_renderer_text_new(), "text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (problems_view), column);

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (problems_view), FALSE);
	gtk_tree_view_set_model (GTK_TREE_VIEW (problems_view),
	                         GTK_TREE_MODEL (problems_store));
	g_object_unref (G_OBJECT (problems_store));

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

	// set tree properties
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (
		GTK_TREE_VIEW (problems_view)), GTK_SELECTION_NONE);
	gtk_tree_view_expand_all (GTK_TREE_VIEW (problems_view));

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

#ifdef PRE_ZYPP_3
static string getSourceName (zypp::Source_Ref source)
{  // based on yast-qt's singleProduct()
	if (!source.enabled())
		// can't get name if disabled; strangely, source.resolvables().begin()
		// hangs
		return "";
	string ret;
	bool found = false;
	for (zypp::ResStore::iterator it = source.resolvables().begin();
	     it != source.resolvables().end(); it++) {
		zypp::Product::Ptr product;
		product = zypp::dynamic_pointer_cast <zypp::Product> (*it);
		if (product) {
			if (found) {
				y2milestone ("Multiple products on installation source %s",
				             source.alias().c_str());
				return "";
			}
			found = true;
			ret = fastGetSummary (product);
		}
	}

	if (!found)
		y2milestone ("No product on installation source %s",
		             source.alias().c_str());
	return ret;
}
#endif

#define PACKAGE_INFO_HEIGHT  140
#define ADVANCED_INFO_HEIGHT  80

// Expander widget with the package information
class PackageInformation
{
	GtkWidget *m_widget, *m_notebook;
	// Information text
	GtkWidget *m_about_text, *m_authors_text, *m_filelist_text, *m_changelog_text;
	bool m_use_filemanager;

public:
	PackageInformation (const char *expander_text, bool only_description)
	{
		m_widget = gtk_expander_new (expander_text);
		gtk_widget_set_sensitive (m_widget, FALSE);

		if (only_description) {
			GtkWidget *about_win = gtk_scrolled_window_new (NULL, NULL);
			gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (about_win),
			                                GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
			m_about_text = ygtk_html_wrap_new();
			gtk_container_add (GTK_CONTAINER (about_win), m_about_text);
			gtk_container_add (GTK_CONTAINER (m_widget), about_win);
			m_authors_text = m_filelist_text = m_changelog_text = NULL;
			m_notebook = NULL;
		}
		else {
			m_notebook = gtk_notebook_new();
			gtk_notebook_set_tab_pos (GTK_NOTEBOOK (m_notebook), GTK_POS_BOTTOM);
			gtk_container_add (GTK_CONTAINER (m_widget), m_notebook);

			m_about_text = add_text_tab (m_notebook, _("Description"));
			m_filelist_text = add_text_tab (m_notebook, _("File List"));
			if ((m_use_filemanager = FILEMANAGER_PRESENT()))
				ygtk_html_wrap_connect_link_clicked (m_filelist_text,
					G_CALLBACK (dir_pressed_cb), NULL);
			m_changelog_text = add_text_tab (m_notebook, _("Change Log"));
			m_authors_text = add_text_tab (m_notebook, _("Authors"));
		}
		gtk_widget_set_size_request (gtk_bin_get_child (GTK_BIN (m_widget)),
		                             -1, PACKAGE_INFO_HEIGHT);
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
			if (m_changelog_text)
				set_text (m_changelog_text, "");
			if (m_authors_text)
				set_text (m_authors_text, "");
			gtk_expander_set_expanded (GTK_EXPANDER (m_widget), FALSE);
			gtk_widget_set_sensitive (m_widget, FALSE);
			return;
		}

		/* Keep notebook on first page; but only when not visible to not
		   annoy the user. */
		if (m_notebook && !gtk_expander_get_expanded (GTK_EXPANDER (m_widget)))
			gtk_notebook_set_current_page (GTK_NOTEBOOK (m_notebook), 0);
		gtk_widget_set_sensitive (m_widget, TRUE);

		ZyppPackage package = tryCastToZyppPkg (object);
		if (m_about_text) {
			string description = YGUtils::escape_markup (object->description(), true);

			// cut authors, since they have their own section
			string::size_type pos = description.find ("Authors:");
			if (pos != string::npos)
				description.erase (pos, string::npos);

			string str;
			if (package) {
				description += "<br>";
				str = package->url();
				if (!str.empty())
					description += _("Website: ") + str + "<br>";
				str = package->license();
				if (!str.empty())
					description += _("License: ") + str + "<br>";
				description += _("Size: ") + object->size().asString() + "b<br>";
			}

#ifdef PRE_ZYPP_3
			zypp::Source_Ref source = object->source();
			str = getSourceName (source);
			if (str.empty())
				str = source.url().asString();
			else
				str = str + " (" + source.url().asString() + ")";
#else
			zypp::Repository repo = object->repository();
			str = repo.info().name();
#endif
			if (!str.empty())
				description += _("Repository: ") + str;

			set_text (m_about_text, description);
		}
		if (package) {
			if (m_filelist_text) {
				string filelist;
				const std::list <string> &filenames = package->filenames();
				for (std::list <string>::const_iterator it = filenames.begin();
				     it != filenames.end(); it++) {
					string file (*it);
					// don't show created dirs
					if (g_file_test (file.c_str(), G_FILE_TEST_IS_DIR))
						continue;
					// set the path as a link
					if (m_use_filemanager) {
						string::size_type pos = file.find_last_of ('/');
						if (pos != string::npos) {  // should be always true
							string dirname (file, 0, pos+1);
							string basename (file, pos+1, string::npos);
							file = "<a href=" + dirname + ">" + dirname + "</a>" + basename;
						}
					}
					filelist += file + "<br>";
				}
				set_text (m_filelist_text, filelist);
			}
			if (m_changelog_text) {
				string text;
				const std::list <zypp::ChangelogEntry> &changelog = package->changelog();
				for (std::list <zypp::ChangelogEntry>::const_iterator it = changelog.begin();
				     it != changelog.end(); it++) {
					string t = "<blockquote>" + YGUtils::escape_markup (it->text(), true) +
					              "</blockquote>";
					text += "<p>" + it->date().asString() + ""
					        + YGUtils::escape_markup (it->author()) + ":<br>"
					        + t + "</p>";
				}
				set_text (m_changelog_text, text);
			}
			if (m_authors_text) {
				string packager (YGUtils::escape_markup (package->packager())), authors;

				const std::list <string> &authors_list = package->authors();
				if (!authors_list.empty()) {
					for (std::list <string>::const_iterator it = authors_list.begin();
					     it != authors_list.end(); it++)
						authors += *it;
				}
				else {
					/* authors() should be the proper way to get authors, but it seems to
					   be rarely used, instead packagers list them on the description. */
					string description (object->description());
					string::size_type pos = description.find ("Authors:");
					if (pos != string::npos) {
						pos = description.find_first_not_of (
							'-', pos + sizeof ("Authors:") + 1);
						authors = string (description, pos, string::npos);
						authors = YGUtils::escape_markup (authors, true);
					}
				}

				string text;
				if (!packager.empty())
					text = _("Packaged by:") + ("<br><blockquote>" + packager) +
					       "</blockquote>";
				if (!authors.empty()) {
					if (!packager.empty())
						text += "<br><br>";
					text += _("Developed by:") + ("<br><blockquote>" + authors) +
					        "</blockquote>";
				}
				set_text (m_authors_text, text);
			}
		}
		else {
			if (m_filelist_text)
				set_text (m_filelist_text, "");
			if (m_changelog_text)
				set_text (m_changelog_text, "");
			if (m_authors_text)
				set_text (m_authors_text, "");
		}
	}

private:
	// auxiliaries to cut down on code
	static GtkWidget *add_text_tab (GtkWidget *notebook, const char *label)
	{
		GtkWidget *widget, *scroll_win;
		scroll_win = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll_win),
		                                GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
		widget = ygtk_html_wrap_new();
		gtk_container_add (GTK_CONTAINER (scroll_win), widget);

		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scroll_win,
		                          gtk_label_new (label));
		return widget;
	}

	static void set_text (GtkWidget *widget, const string &text)
	{
		const char *str = _("<i>(not available)</i>");
		if (!text.empty())
			str = text.c_str();
		ygtk_html_wrap_set_text (widget, str);
		ygtk_html_wrap_scroll (widget, TRUE);  // scroll to the start
	}

	/* When a directory is pressed on the file list. */
	static void dir_pressed_cb (GtkWidget *text, const gchar *link)
	{ FILEMANAGER_LAUNCH (link); }
};

// Table widget with package sources
	/* I don't like this, since it differs from the rest of the code, but to
	   avoid declarations... */
	struct SourcesTableListener
	{
		virtual ~SourcesTableListener() {}
		virtual void sources_changed_cb() = 0;
	};
class SourcesTable
{
	GtkWidget *m_widget;
	GtkTreeModel *m_model;
	SourcesTableListener *m_listener;

public:
	SourcesTable (SourcesTableListener *listener)
		: m_listener (listener)
	{
		/* 0 - source enabled?, 1  - source name (estimation), 2 - source URL,
		   3 - source id; what they call of alias. */
		GtkListStore *store = gtk_list_store_new (4, G_TYPE_BOOLEAN, G_TYPE_STRING,
		                                             G_TYPE_STRING, G_TYPE_STRING);
		m_model = GTK_TREE_MODEL (store);

#ifdef PRE_ZYPP_3
		zypp::SourceManager_Ptr manager = zypp::SourceManager::sourceManager();
		for (zypp::SourceManager::Source_const_iterator it = manager->Source_begin();
		     it != manager->Source_end(); it++) {
			zypp::Source_Ref src = *it;
			if (src && src.enabled()) {
				GtkTreeIter iter;
				gtk_list_store_append (store, &iter);
				gtk_list_store_set (store, &iter, 0, src.enabled(),
					1, getSourceName (src).c_str(), 2, src.url().asString().c_str(),
					3, src.alias().c_str(), -1);
			}
		}
#else
		zypp::RepoManager manager;
		std::list <zypp::RepoInfo> repos = manager.knownRepositories();
		for (std::list <zypp::RepoInfo>::iterator it = repos.begin();
		     it != repos.end(); it++) {
			if (it->enabled()) {
				GtkTreeIter iter;
				gtk_list_store_append (store, &iter);
				gtk_list_store_set (store, &iter, 0, bool(it->enabled()),
					1, it->name().c_str(), 2, it->alias().c_str(), -1);
			}
		}
#endif

		GtkWidget *view = gtk_tree_view_new_with_model (m_model);
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
		gtk_tree_view_set_enable_search (GTK_TREE_VIEW (view), FALSE);
		gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)),
		                             GTK_SELECTION_NONE);
		g_object_unref (G_OBJECT (m_model));  // tree view will take care of it

		// prepare the view
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;

		renderer = gtk_cell_renderer_toggle_new();
		column = gtk_tree_view_column_new_with_attributes ("",
		             renderer, "active", 0, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
		g_signal_connect (G_OBJECT (renderer), "toggled",
		                  G_CALLBACK (source_toggled_cb), this);

		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes ("",
		             renderer, "text", 1, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

#ifdef PRE_ZYPP_3
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes ("",
		             renderer, "text", 2, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
#endif

		m_widget = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (m_widget),
		                                GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_widget_set_size_request (m_widget, -1, ADVANCED_INFO_HEIGHT);
		gtk_container_add (GTK_CONTAINER (m_widget), view);
	}

	GtkWidget *getWidget()
	{ return m_widget; }

private:
	static void source_toggled_cb (GtkCellRendererToggle *renderer,
	                               gchar *path_str, SourcesTable *pThis)
	{
		IMPL
		GtkTreeModel *model = pThis->m_model;
		GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
		GtkTreeIter iter;
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_path_free (path);

#ifdef PRE_ZYPP_3
		gchar *alias;
		gtk_tree_model_get (model, &iter, 3, &alias, -1);

		zypp::SourceManager_Ptr manager = zypp::SourceManager::sourceManager();
		zypp::Source_Ref source = manager->findSource (alias);
		g_free (alias);

		if (gtk_cell_renderer_toggle_get_active (renderer))
			source.disable();
		else
			source.enable();
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		                    0, source.enabled(), -1);
#else
		gchar *alias;
		gtk_tree_model_get (model, &iter, 2, &alias, -1);

		zypp::RepoManager manager;
		zypp::RepoInfo repo = manager.getRepositoryInfo (alias);
		g_free (alias);

		bool enable = !gtk_cell_renderer_toggle_get_active (renderer);
		repo.setEnabled (enable);

		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		                    0, bool (repo.enabled()), -1);
#endif

		pThis->m_listener->sources_changed_cb();
	}
};

// Table widget to monitor disk partitions space-usage
#define MIN_FREE_MB_WARN (80*1024)

class DiskTable
{
	GtkWidget *m_widget;
	GtkTreeModel *m_model;
	bool m_has_warned;  // only warn once

	// evil long names
	typedef zypp::DiskUsageCounter::MountPoint    ZyppDu;
	typedef zypp::DiskUsageCounter::MountPointSet ZyppDuSet;
	typedef zypp::DiskUsageCounter::MountPointSet::iterator ZyppDuSetIterator;

public:
	DiskTable()
	{
		m_has_warned = false;
		if (zypp::getZYpp()->diskUsage().empty())
			zypp::getZYpp()->setPartitions (
			    zypp::DiskUsageCounter::detectMountPoints());

		/* 0 - mount point (also used as id), 1 - percentage of disk usage
		  ([0, 100]), 2 - disk usage detail (i.e. "200Mb of 1Gb") */
		GtkListStore *store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_INT,
		                                             G_TYPE_STRING);
		m_model = GTK_TREE_MODEL (store);
		update();

		GtkWidget *view = gtk_tree_view_new_with_model (m_model);
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
		gtk_tree_view_set_enable_search (GTK_TREE_VIEW (view), FALSE);
		gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)),
		                             GTK_SELECTION_NONE);
		g_object_unref (G_OBJECT (m_model));  // tree view will take care of it

		// prepare the view
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;

		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes (_("Mount Point"),
		             renderer, "text", 0, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

		renderer = gtk_cell_renderer_progress_new();
		column = gtk_tree_view_column_new_with_attributes (_("Usage"),
		             renderer, "value", 1, "text", 2, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

		m_widget = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (m_widget),
		                                GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_widget_set_size_request (m_widget, -1, ADVANCED_INFO_HEIGHT);
		gtk_container_add (GTK_CONTAINER (m_widget), view);
	}

	GtkWidget *getWidget()
	{ return m_widget; }

	static string sizeToString (long long size)
	{
		zypp::ByteCount count (size, zypp::ByteCount::K);
		return count.asString();
	}

	void update()
	{
		GtkListStore *store = GTK_LIST_STORE (m_model);
		gtk_list_store_clear (store);

		bool warning = false;
		ZyppDuSet diskUsage = zypp::getZYpp()->diskUsage();
		for (ZyppDuSetIterator it = diskUsage.begin(); it != diskUsage.end(); it++) {
			const ZyppDu &partition = *it;
			if (!partition.readonly) {
				// partition fields: dir, used_size, total_size (size on Kb)
				GtkTreeIter iter;
				gtk_list_store_append (store, &iter);

				long usage = (partition.pkg_size * 100) / (partition.total_size + 1);
				string usage_str = sizeToString (partition.pkg_size) + " (of " +
				                   sizeToString (partition.total_size) + ")";
				gtk_list_store_set (store, &iter, 0, partition.dir.c_str(),
					1, usage, 2, usage_str.c_str(), -1);

				warning = warning ||
					(partition.total_size > 1024 &&
                     partition.total_size - partition.pkg_size < MIN_FREE_MB_WARN);
			}
		}
		if (warning)
			warn();
	}

	void warn()
	{
		if (m_has_warned)
			return;
		m_has_warned = true;

		GtkWidget *dialog, *view, *scroll_view;
		dialog = gtk_message_dialog_new_with_markup (YGUI::ui()->currentWindow(),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING,
			GTK_BUTTONS_OK, _("<b>Disk Almost Full !</b>\n\n"
			"One of the partitions is reaching its limit of capacity. You may "
			"have to remove packages if you wish to install some."));

		view = gtk_tree_view_new_with_model (m_model);
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
		gtk_tree_view_set_enable_search (GTK_TREE_VIEW (view), FALSE);
		gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)),
		                             GTK_SELECTION_NONE);

		GtkTreeViewColumn *column;
		column = gtk_tree_view_column_new_with_attributes (_("Mount Point"),
		             gtk_cell_renderer_text_new(), "text", 0, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
		column = gtk_tree_view_column_new_with_attributes (_("Usage"),
		             gtk_cell_renderer_progress_new(),
		             "value", 1, "text", 2, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

		scroll_view = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll_view),
		                                GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_widget_set_size_request (scroll_view, -1, 80);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll_view),
		                                     GTK_SHADOW_OUT);
		gtk_container_add (GTK_CONTAINER (scroll_view), view);
		gtk_widget_show_all (scroll_view);
		gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), scroll_view);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
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
		ygtk_wizard_set_header_text (wizard, window, _("Patch Selector"));
		ygtk_wizard_set_header_icon (wizard, window,
			THEMEDIR "/icons/32x32/apps/yast-software.png");
		ygtk_wizard_set_help_text (wizard,
			_("For information on a given patch, just press it and as well as the "
			"Package Information expander to make those informations visible.<br>"
			"To install a patch you just need to press the check button next to it "
			"and then the button Install when you are done.")
		);
		ygtk_wizard_set_next_button_label (wizard, _("_Install"));
		ygtk_wizard_set_next_button_id (wizard, g_strdup ("install"), g_free);
		ygtk_wizard_set_abort_button_label (wizard, _("_Cancel"));
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

		m_information_widget = new PackageInformation (_("Patch Information"), true);
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

		column = gtk_tree_view_column_new_with_attributes (_("Priority"),
			gtk_cell_renderer_text_new(), "text", 1, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (m_patches_view), column);

		column = gtk_tree_view_column_new_with_attributes (_("Name"),
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

		// TODO: severity should be sort by severity, not alphabetically
		YGUtils::tree_view_set_sortable (GTK_TREE_VIEW (m_patches_view), 1);

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
		if (mark_selectable (selectable, state))
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, state, -1);
	}

	YGWIDGET_IMPL_COMMON
};

// Package selector's widget
class PackageSelector : public SourcesTableListener
{
friend class YGPackageSelector;

	GtkWidget *m_widget;

	// The GtkTreeView widgets
	GtkWidget *m_installed_view, *m_available_view;
	// Packages backend model
	GtkTreeModel *m_packages_model;

	// Package information widget
	PackageInformation *m_information_widget;
	SourcesTable *m_sources_table;
	DiskTable *m_disk_table;

	// Search gizmos
	GtkWidget *m_search_entry, *m_plain_view;
	guint search_timeout_id;
	bool name_opt, summary_opt, descr_opt, provides_opt, requires_opt;
	list <string> m_search_queries;

	// Interface tweak
	GtkWidget *m_install_label, *m_remove_label;

public:
	PackageSelector (bool patterns_mode)
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
		   5 - can be upgraded (boolean), 6 - can be downgraded (boolean),
		   7 - show up/downgrade control (boolean), 8 - has children,
		   9 - font style; italic for modified (integer) = 10

		   Models are created at each view mode change (and the other freed). This
		   allows for more than one model type be used and is also better for speed,
		   since the filter and tree view then installed upon don't have to keep
		   syncing at every item change. */
		installed_box = createListWidget (_("<b>Installed Software:</b>"),
		                                  "computer", computer_xpm, m_installed_view,
		                                  1, false);
		available_box = createListWidget (_("<b>Available Software:</b>"),
		                                  "gtk-cdrom", NULL, m_available_view,
		                                  2, true);

		/* FIXME: it seems (due to markup or something) search, this won't work.
		   Not really a issue as we provide searching, but...
		gtk_tree_view_set_search_column (GTK_TREE_VIEW (m_installed_view), 0);
		gtk_tree_view_set_search_column (GTK_TREE_VIEW (m_available_view), 0); */
		gtk_tree_view_set_enable_search (GTK_TREE_VIEW (m_installed_view), FALSE);
		gtk_tree_view_set_enable_search (GTK_TREE_VIEW (m_available_view), FALSE);

		gtk_tree_selection_set_mode (gtk_tree_view_get_selection (
			GTK_TREE_VIEW (m_installed_view)), GTK_SELECTION_MULTIPLE);
		gtk_tree_selection_set_mode (gtk_tree_view_get_selection (
			GTK_TREE_VIEW (m_available_view)), GTK_SELECTION_MULTIPLE);

		GtkWidget *buttons_minsize, *selection_buttons_vbox, *install_button,
		          *remove_button;
		selection_buttons_vbox = gtk_vbox_new (TRUE, 80);
		// to avoid re-labeling glitches, let it only grow
		buttons_minsize = ygtk_min_size_new (0, 0);
		gtk_container_add (GTK_CONTAINER (buttons_minsize), selection_buttons_vbox);
		ygtk_min_size_set_only_expand (YGTK_MIN_SIZE (buttons_minsize), TRUE);

		install_button = createArrowButton (_("_install"), GTK_ARROW_RIGHT, &m_install_label);
		remove_button = createArrowButton (_("_remove"), GTK_ARROW_LEFT, &m_remove_label);

		GtkWidget *install_align = gtk_alignment_new (0, 1, 1, 0);
		gtk_container_add (GTK_CONTAINER (install_align), install_button);
		GtkWidget *remove_align = gtk_alignment_new (0, 0, 1, 0);
		gtk_container_add (GTK_CONTAINER (remove_align), remove_button);

		gtk_box_pack_start (GTK_BOX (selection_buttons_vbox), install_align,
		                    TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (selection_buttons_vbox), remove_align,
		                    TRUE, TRUE, 0);
		gtk_container_set_border_width (GTK_CONTAINER (selection_buttons_vbox), 6);

		gtk_box_pack_start (GTK_BOX (packages_hbox), available_box, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (packages_hbox), buttons_minsize,
		                                             FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (packages_hbox), installed_box, TRUE, TRUE, 0);

		GtkWidget *view_box, *view_label, *view_categories, *view_patterns,
		          *view_languages;
		view_label = gtk_label_new (_("View Packages:"));
		gtk_label_set_use_markup (GTK_LABEL (view_label), TRUE);
		gtk_misc_set_alignment (GTK_MISC (view_label), 0, 0.5);
		m_plain_view = gtk_radio_button_new_with_mnemonic (NULL, _("as _plain list"));
		view_categories = gtk_radio_button_new_with_mnemonic_from_widget
		                      (GTK_RADIO_BUTTON (m_plain_view), _("in _categories"));
		view_patterns = gtk_radio_button_new_with_mnemonic_from_widget
		                      (GTK_RADIO_BUTTON (m_plain_view), _("in _patterns"));
		view_languages = gtk_radio_button_new_with_mnemonic_from_widget
		                      (GTK_RADIO_BUTTON (m_plain_view), _("in _languages"));
		view_box = gtk_hbox_new (FALSE, 0);
		gtk_box_pack_start (GTK_BOX (view_box), view_label, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (view_box), m_plain_view, FALSE, TRUE, 4);
		gtk_box_pack_start (GTK_BOX (view_box), view_categories, FALSE, TRUE, 4);
		gtk_box_pack_start (GTK_BOX (view_box), view_patterns, FALSE, TRUE, 4);
		gtk_box_pack_start (GTK_BOX (view_box), view_languages, FALSE, TRUE, 4);
		g_signal_connect (G_OBJECT (m_plain_view), "toggled",
		                  G_CALLBACK (view_plain_mode_cb), this);
		g_signal_connect (G_OBJECT (view_categories), "toggled",
		                  G_CALLBACK (view_categories_mode_cb), this);
		g_signal_connect (G_OBJECT (view_patterns), "toggled",
		                  G_CALLBACK (view_patterns_mode_cb), this);
		g_signal_connect (G_OBJECT (view_languages), "toggled",
		                  G_CALLBACK (view_languages_mode_cb), this);

		// default search fields
		name_opt = summary_opt = provides_opt = true;
		requires_opt = descr_opt = false;

		GtkWidget *search_hbox, *search_label;
		search_hbox = gtk_hbox_new (FALSE, 0);
		search_label = gtk_label_new_with_mnemonic (_("_Search:"));
		gtk_label_set_use_markup (GTK_LABEL (search_label), TRUE);
		gtk_misc_set_alignment (GTK_MISC (search_label), 0, 0.5);
		m_search_entry = ygtk_find_entry_new();
		gtk_label_set_mnemonic_widget (GTK_LABEL (search_label), m_search_entry);
		gtk_box_pack_start (GTK_BOX (search_hbox), search_label, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (search_hbox), m_search_entry, TRUE, TRUE, 4);
		search_timeout_id = 0;
		g_signal_connect (G_OBJECT (m_search_entry), "activate",  // when Enter
		                  G_CALLBACK (search_activate_cb), this);  // is pressed
		g_signal_connect_after (G_OBJECT (m_search_entry), "changed",
		                        G_CALLBACK (search_request_cb), this);
		ygtk_find_entry_attach_menu (YGTK_FIND_ENTRY (m_search_entry),
		                             create_search_menu());

		/* we want "View Packages" to be aligned to "Search"; we could use a
		   GtkTable for that, but a GtkSizeGroup does the job with less hassle. */
		GtkSizeGroup *align_labels = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
		gtk_size_group_add_widget (align_labels, view_label);
		gtk_size_group_add_widget (align_labels, search_label);
		g_object_unref (G_OBJECT (align_labels));

		m_information_widget = new PackageInformation (_("Package Information"), false);
		GtkWidget *pkg_info_widget = m_information_widget->getWidget();

		m_sources_table = new SourcesTable (this);
		m_disk_table = new DiskTable();

		GtkWidget *advanced_expander = gtk_expander_new (_("Advanced"));
		GtkWidget *advanced_notebook = gtk_notebook_new();
		GtkWidget *sources_vbox = gtk_vbox_new (FALSE, 4);
		GtkWidget *sources_label = gtk_label_new (
			_("<i>Use the Installation Source tool to manage the sources.</i>"));
		gtk_misc_set_alignment (GTK_MISC (sources_label), 1, 0);
		gtk_label_set_use_markup (GTK_LABEL (sources_label), TRUE);
		gtk_box_pack_start (GTK_BOX (sources_vbox), m_sources_table->getWidget(),
		                    TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (sources_vbox), sources_label, TRUE, TRUE, 0);
		gtk_notebook_set_tab_pos (GTK_NOTEBOOK (advanced_notebook), GTK_POS_BOTTOM);
		gtk_notebook_append_page (GTK_NOTEBOOK (advanced_notebook),
			sources_vbox, gtk_label_new (_("Repositories")));
		gtk_notebook_append_page (GTK_NOTEBOOK (advanced_notebook),
			m_disk_table->getWidget(), gtk_label_new (_("Disk Usage")));
		gtk_container_add (GTK_CONTAINER (advanced_expander), advanced_notebook);

		gtk_box_pack_start (GTK_BOX (m_widget), packages_hbox, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_widget), view_box, FALSE, FALSE, 12);
		gtk_box_pack_start (GTK_BOX (m_widget), search_hbox, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (m_widget), pkg_info_widget, FALSE, FALSE, 12);
		gtk_box_pack_start (GTK_BOX (m_widget), advanced_expander, FALSE, FALSE, 0);
		gtk_widget_show_all (m_widget);

		g_object_ref (G_OBJECT (m_widget));
		gtk_object_sink (GTK_OBJECT (m_widget));

		// interface sugar
		g_signal_connect (G_OBJECT (install_button), "clicked",
		                  G_CALLBACK (install_button_clicked_cb), this);
		g_signal_connect (G_OBJECT (remove_button), "clicked",
		                  G_CALLBACK (remove_button_clicked_cb), this);

		g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (
		                            m_installed_view))), "changed",
		                  G_CALLBACK (package_clicked_cb), this);
		g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (
		                            m_available_view))), "changed",
		                  G_CALLBACK (package_clicked_cb), this);

		// signal for default view
		if (patterns_mode)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (view_patterns), TRUE);
		else
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
		                                GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
		list = gtk_tree_view_new();
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list), FALSE);
		gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list), TRUE);
		gtk_tree_selection_set_select_function (gtk_tree_view_get_selection (
			GTK_TREE_VIEW (list)), dont_select_groups_cb, this, NULL);
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
		g_object_set (G_OBJECT (text_renderer),
		              "ellipsize", PANGO_ELLIPSIZE_END, NULL);
		GtkTreeViewColumn *column;
		column = gtk_tree_view_column_new_with_attributes (_("Packages"),
			text_renderer, "markup", package_name_col, "style", 9, NULL);
		gtk_tree_view_column_set_expand (column, TRUE);
		gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
		// versions combo column
		if (has_version_col) {
			GtkCellRenderer *arrow_renderer = ygtk_cell_renderer_arrow_new();
			column = gtk_tree_view_column_new_with_attributes (NULL,
				arrow_renderer, "can-go-up", 5, "can-go-down", 6, "visible", 7, NULL);
			gtk_tree_view_column_set_expand (column, FALSE);
			gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
			g_signal_connect (G_OBJECT (arrow_renderer), "pressed",
			                  G_CALLBACK (change_available_version_cb), this);
		}
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

	// Dynamic views support
	void load_packages_view (GtkTreeModel *(* build_model) (GtkProgressBar *))
	{
		GtkWidget *dialog, *label, *vbox, *progress = 0;
		dialog = gtk_dialog_new();
		gtk_window_set_title (GTK_WINDOW (dialog), "");
		gtk_window_set_transient_for (GTK_WINDOW (dialog),
		                              YGUI::ui()->currentWindow());
		gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
		gtk_window_set_default_size (GTK_WINDOW (dialog), 100, -1);
		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
		gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
		vbox = GTK_DIALOG (dialog)->vbox;
//		gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
		label = gtk_label_new (_("Loading packages list..."));
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
		gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 4);
		progress = gtk_progress_bar_new();
		gtk_box_pack_start (GTK_BOX (vbox), progress, TRUE, FALSE, 4);
		gtk_widget_show_all (dialog);

		gtk_tree_view_set_model (GTK_TREE_VIEW (m_installed_view), NULL);
		gtk_tree_view_set_model (GTK_TREE_VIEW (m_available_view), NULL);
		m_information_widget->setPackage (NULL, NULL);

		GtkProgressBar *progress_bar = GTK_PROGRESS_BAR (progress);
		// build it
		GtkTreeModel *model = (m_packages_model = build_model (progress_bar));

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

		// use the "available" name to do the sorting
		GtkTreeSortable *sortable = GTK_TREE_SORTABLE (model);
		gtk_tree_sortable_set_sort_func (sortable, 2, YGUtils::sort_compare_cb,
		                                 GINT_TO_POINTER (2), NULL);
		gtk_tree_sortable_set_sort_column_id (sortable, 2, GTK_SORT_ASCENDING);

		gtk_widget_destroy (dialog);
	}

	// macros to handle progress bar
	#define SET_PROGRESS(_steps, _jump) int steps = _steps, step = 0, jump = _jump;
	#define PROGRESS()                                          \
		if (progress && ((step++) % jump == 0)) {               \
			gdouble fraction = steps > 0 ? ((gdouble) step) / steps : 0; \
			gtk_progress_bar_set_fraction (progress, fraction); \
			while (gtk_events_pending()) gtk_main_iteration(); }

	static GtkTreeModel *loadPackagesListAsPlain (GtkProgressBar *progress)
	{
		GtkListStore *store = gtk_list_store_new (10, G_TYPE_POINTER, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
			G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_INT);
		GtkTreeModel *model = GTK_TREE_MODEL (store);
		g_object_set_data (G_OBJECT (model), "detail-package", GINT_TO_POINTER (1));

		SET_PROGRESS (zyppPool().size <zypp::Package>(), 200)
		GtkTreeIter iter;
		for (ZyppPool::const_iterator it = zyppPool().byKindBegin <zypp::Package>();
		     it != zyppPool().byKindEnd <zypp::Package>(); it++)
		{
			ZyppSelectable selectable = *it;
			gtk_list_store_append (store, &iter);
			loadPackageRow (model, &iter, selectable);
			PROGRESS()
		}
		return model;
	}

	static GtkTreeModel *loadPackagesListByCategory (GtkProgressBar *progress)
	{
		GtkTreeStore *store = gtk_tree_store_new (10, G_TYPE_POINTER, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
			G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_INT);
		GtkTreeModel *model = GTK_TREE_MODEL (store);
		g_object_set_data (G_OBJECT (model), "detail-package", GINT_TO_POINTER (1));

		// we need to create the categories tree as we iterate packages
		map <string, GtkTreePath *> tree;

		SET_PROGRESS (zyppPool().size <zypp::Package>(), 80)
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

						string name = "<big><b>" + node + "</b></big>";
						gtk_tree_store_set (store, &iter,
							0, NULL, 1, name.c_str(), 2, name.c_str(), 3, TRUE,
							4, TRUE, 5, FALSE, 6, FALSE, 7, FALSE, 8, TRUE, -1);

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
			PROGRESS()
		}

		// free GtkTreePaths
		for (map <string, GtkTreePath*>::iterator it = tree.begin();
		     it != tree.end(); it++)
			gtk_tree_path_free (it->second);
		return model;
	}

	/* NOTE: Getting the packages under a pattern, language, etc is done
	   differently, so code re-use would be limited. Probably doesn't worth
	   the trouble and complexity of doing so. */

	static GtkTreeModel *loadPackagesListByPattern (GtkProgressBar *progress)
	{
		GtkTreeStore *store = gtk_tree_store_new (10, G_TYPE_POINTER, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
			G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_INT);
		GtkTreeModel *model = GTK_TREE_MODEL (store);
		g_object_set_data (G_OBJECT (model), "detail-package", GINT_TO_POINTER (0));

		// we need to create a categories tree for the patterns
		map <string, GtkTreeIter> tree;

		SET_PROGRESS (zyppPool().size <zypp::Pattern>(), 5)
		for (ZyppPool::const_iterator it = zyppPool().byKindBegin <zypp::Pattern>();
		     it != zyppPool().byKindEnd <zypp::Pattern>(); it++) {
			ZyppSelectable selectable = *it;
			ZyppObject object = selectable->theObj();
			ZyppPattern pattern = tryCastToZyppPattern (object);
			if (pattern && pattern->userVisible()) {
				GtkTreeIter category_iter, pattern_iter, package_iter;
				string category = pattern->category();
				map <string, GtkTreeIter>::iterator cat_it = tree.find (category);
				if (cat_it == tree.end()) {
					string name = "<big><b>" + category + "</b></big>";
					gtk_tree_store_append (store, &category_iter, NULL);
					gtk_tree_store_set (store, &category_iter,
						0, NULL, 1, name.c_str(), 2, name.c_str(), 3, TRUE,
						4, TRUE, 5, FALSE, 6, FALSE, 7, FALSE, 8, TRUE, -1);
					tree [category] = category_iter;
				}
				else
					category_iter = cat_it->second;

				string name = "<b>" + fastGetSummary (object) + "</b>";
				gtk_tree_store_append (store, &pattern_iter, &category_iter);
				loadPackageRow (model, &pattern_iter, selectable);

/*				gtk_tree_store_set (store, &pattern_iter,
					0, get_pointer (selectable), 1, name.c_str(), 2, name.c_str(),
					3, TRUE, 4, TRUE, 5, FALSE, 6, FALSE, 7, FALSE, 8, TRUE, -1);*/

				// adding children packages
				const set <string> &packages = pattern->install_packages();
				for (ZyppPool::const_iterator pt2 =
				         zyppPool().byKindBegin <zypp::Package>();
				     pt2 != zyppPool().byKindEnd <zypp::Package>(); pt2++) {
					ZyppSelectable sel = *pt2;
					for (set <string>::iterator pt1 = packages.begin();
					     pt1 != packages.end(); pt1++) {
							if (sel->name() == *pt1) {
								gtk_tree_store_append (store, &package_iter, &pattern_iter);
								loadPackageRow (model, &package_iter, sel);
							}
					}
				}
			}
			PROGRESS()
		}
		return model;
	}

	static GtkTreeModel *loadPackagesListByLanguage (GtkProgressBar *progress)
	{
		GtkTreeStore *store = gtk_tree_store_new (10, G_TYPE_POINTER, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
			G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_INT);
		GtkTreeModel *model = GTK_TREE_MODEL (store);
		g_object_set_data (G_OBJECT (model), "detail-package", GINT_TO_POINTER (0));

		SET_PROGRESS (zyppPool().size <zypp::Language>(), 20)
		for (ZyppPool::const_iterator it = zyppPool().byKindBegin <zypp::Language>();
		     it != zyppPool().byKindEnd <zypp::Language>(); it++) {
			ZyppSelectable langsel = *it;
			ZyppObject langobj = langsel->theObj();
			ZyppLanguage lang = tryCastToZyppLanguage (langobj);
			if (lang) {
				string langname = langsel->name();
				if (langname.empty() || langname == "C")
					continue;

				string descrpt = "<b>" + langobj->description() + "</b>";
				GtkTreeIter parent_iter, iter;
				gtk_tree_store_append (store, &parent_iter, NULL);
				loadPackageRow (model, &parent_iter, langsel);
/*				gtk_tree_store_set (store, &parent_iter,
					0, get_pointer (langsel), 1, descrpt.c_str(), 2, descrpt.c_str(),
					3, TRUE, 4, TRUE, 5, FALSE, 6, FALSE, 7, FALSE, 8, TRUE, -1);*/

				for (ZyppPool::const_iterator it =
				          zyppPool().byKindBegin <zypp::Package>();
				      it != zyppPool().byKindEnd <zypp::Package>(); it++) {
					ZyppSelectable pkgsel = *it;
					ZyppObject pkgobj = pkgsel->theObj();

					const zypp::CapSet &capSet = pkgobj->dep (zypp::Dep::FRESHENS);
					for (zypp::CapSet::const_iterator it = capSet.begin();
					     it != capSet.end(); it++) {
						if (it->index() == lang->name()) {
							gtk_tree_store_append (store, &iter, &parent_iter);
							loadPackageRow (model, &iter, pkgsel);
						}
					}
				}
			}
			PROGRESS()
		}
		return model;
	}
	#undef SET_PROGRESS
	#undef PROGRESS

	static void induceObjects (ZyppSelectable selectable, ZyppObject &install_obj,
	                           ZyppObject &available_obj, bool *has_upgrade,
	                           bool *has_downgrade)
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
				break;
		}

		// zypp keeps on the pool objects whose sources we disabled, so we may
		// need to calculate the candidate object here.
#ifdef PRE_ZYPP_3
		if (available_obj != NULL && !available_obj->source().enabled()) {
#else
        // beware lurking tribool requires bool cast here.
		if (available_obj != NULL && !(bool)(available_obj->repository().info().enabled())) {
#endif
			available_obj = NULL;
			for (zypp::ui::Selectable::available_iterator it = selectable->availableBegin();
			     it != selectable->availableEnd(); it++) {
#ifdef PRE_ZYPP_3
				if (!(*it)->source().enabled())
#else
				if (!(*it)->repository().info().enabled())
#endif
					;
				else if (!available_obj)
					available_obj = *it;
				else if (zypp::Edition::compare ((*it)->edition(),
				                                 available_obj->edition()) > 0)
					available_obj = *it;
			}
		}

		bool has_up = false, has_down = false;
		if (available_obj != NULL) {
			for (zypp::ui::Selectable::available_iterator it = selectable->availableBegin();
			     it != selectable->availableEnd(); it++) {
#ifdef PRE_ZYPP_3
				if (!(*it)->source().enabled())
#else
				if (!(*it)->repository().info().enabled())
#endif
					continue;
				int res = zypp::Edition::compare ((*it)->edition(),
				                                  available_obj->edition());
				if (res < 0)
					has_down = true;
				else if (res > 0)
					has_up = true;
			}
		}

		if (has_upgrade)
			*has_upgrade = has_up;
		if (has_downgrade)
			*has_downgrade = has_down;
	}

	/* Must be called for the main model. */
	static void loadPackageRow (GtkTreeModel *model, GtkTreeIter *iter,
	                            ZyppSelectable selectable)
	{
		ZyppObject install_obj, available_obj;
		bool has_upgrade, has_downgrade;
		induceObjects (selectable, install_obj, available_obj,
		               &has_upgrade, &has_downgrade);

		bool detailed = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (model),
		                                                    "detail-package"));
		string availableName, installedName, name (YGUtils::escape_markup (selectable->name()));
		if (available_obj) {
			if (detailed)
				availableName = "<b>" + name + "</b> (" + available_obj->edition().version() +
				                ")\n<small>" + YGUtils::escape_markup (fastGetSummary (available_obj)) +
				                "</small>";
			else {
				ZyppPattern pattern = tryCastToZyppPattern (available_obj);
				ZyppLanguage lang = tryCastToZyppLanguage (available_obj);
				if (pattern)
					availableName = "<b>" + fastGetSummary (available_obj) + "</b>";
				else if (lang)
					availableName = "<b>" + available_obj->description() + "</b>";
				else
					availableName = name + " (" + available_obj->edition().version() + ")";
			}
		}
		if (install_obj) {
			if (detailed)
				installedName = "<b>" + name + "</b> (" + install_obj->edition().version() +
				                ")\n<small>" + YGUtils::escape_markup (fastGetSummary (install_obj)) +
				                "</small>";
			else {
				ZyppPattern pattern = tryCastToZyppPattern (install_obj);
				ZyppLanguage lang = tryCastToZyppLanguage (install_obj);
				if (pattern)
					installedName = "<b>" + fastGetSummary (install_obj) + "</b>";
				else if (lang)
					installedName = "<b>" + install_obj->description() + "</b>";
				else
					installedName = name + " (" + install_obj->edition().version() + ")";
			}
		}

		PangoStyle style = PANGO_STYLE_NORMAL;
		if (selectable->toModify())
			style = PANGO_STYLE_ITALIC;

		// oh, would be nice to have a common set for tree models...
		if (GTK_IS_LIST_STORE (model))
			gtk_list_store_set (GTK_LIST_STORE (model), iter,
				0, get_pointer (selectable), 1, installedName.c_str(),
				2, availableName.c_str(), 3, install_obj != 0, 4, available_obj != 0,
				5, has_upgrade, 6, has_downgrade, 7, detailed, 8, FALSE, 9, style, -1);
		else /*if (GTK_IS_TREE_STORE (model))*/
			gtk_tree_store_set (GTK_TREE_STORE (model), iter,
				0, get_pointer (selectable), 1, installedName.c_str(),
				2, availableName.c_str(), 3, install_obj != 0, 4, available_obj != 0,
				5, has_upgrade, 6, has_downgrade, 7, detailed, 8, FALSE, 9, style, -1);
/*		y2milestone ("set %s: %d - %d\n", selectable->name().c_str(),
		             available_obj != 0, install_obj != 0);*/
	}

	virtual ~PackageSelector()
	{
		IMPL
		if (search_timeout_id)
			g_source_remove (search_timeout_id);

		delete m_information_widget;
		delete m_sources_table;
		delete m_disk_table;
		gtk_widget_destroy (m_widget);
		g_object_unref (G_OBJECT (m_widget));
	}

	GtkWidget *getWidget()
	{ return m_widget; }

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
		pThis->load_packages_view (&loadPackagesListByPattern);
	}

	static void view_languages_mode_cb  (GtkToggleButton *button,
	                                     PackageSelector *pThis)
	{
		if (!gtk_toggle_button_get_active (button)) return;
		pThis->clear_search_entry (true);
		pThis->load_packages_view (&loadPackagesListByLanguage);
	}

	void sources_changed_cb()
	{
		// reload view; FIXME: re-load the current view, not necessarly
		// the plain list; hacky anyway
		if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (m_plain_view)))
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (m_plain_view), TRUE);
		else
			g_signal_emit_by_name (G_OBJECT (m_plain_view), "toggled", this);
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
		pThis->search_timeout_id = g_timeout_add (500, search_cb, pThis);
	}

    struct FindClosure {
        bool found;
        GtkTreeView *view;
        PackageSelector *pThis;
    };
    static gboolean find_exact_match (GtkTreeModel *model, GtkTreePath *path,
                                      GtkTreeIter *iter, gpointer data)
    {
        FindClosure *cl = (FindClosure *) data;
        
        ZyppSelectablePtr sel = NULL;

		gtk_tree_model_get (model, iter, 0, &sel, -1);

        if (!sel)
            return FALSE;

        if (sel->name() == *cl->pThis->m_search_queries.begin()) {
            cl->found = true;
            gtk_tree_selection_select_iter
                (gtk_tree_view_get_selection (cl->view), iter);
            package_clicked_cb (gtk_tree_view_get_selection (cl->view), cl->pThis);
            return TRUE;
        }
        return FALSE;
    }

    void highlight_exact_matches ()
    {
		if (m_search_queries.empty() || !name_opt)
			return;

        FindClosure cl;
        cl.found = false;
        cl.pThis = this;
        cl.view = GTK_TREE_VIEW (m_installed_view);

        gtk_tree_model_foreach (gtk_tree_view_get_model (cl.view),
                                find_exact_match, &cl);

        if (!cl.found) {
            cl.view = GTK_TREE_VIEW (m_available_view);
            gtk_tree_model_foreach (gtk_tree_view_get_model (cl.view),
                                    find_exact_match, &cl);
        }
    }

	static gboolean search_cb (gpointer data)
	{
		IMPL

        // This is potentially very slow ...
#ifdef IMPL_DEBUG
        fprintf (stderr, "search start...\n");
#endif
		PackageSelector *pThis = (PackageSelector *) data;
		pThis->search_timeout_id = 0;

		const gchar *query = gtk_entry_get_text (GTK_ENTRY (pThis->m_search_entry));
		bool plain_view = gtk_toggle_button_get_active (
		                      GTK_TOGGLE_BUTTON (pThis->m_plain_view));
		if (!plain_view)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pThis->m_plain_view), TRUE);
		pThis->m_search_queries = YGUtils::splitString (query, ' ');

		// just re-filter
		GtkTreeModel *available_model =
			gtk_tree_view_get_model (GTK_TREE_VIEW (pThis->m_available_view));
		GtkTreeModel *installed_model =
			gtk_tree_view_get_model (GTK_TREE_VIEW (pThis->m_installed_view));
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (available_model));
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (installed_model));

        pThis->highlight_exact_matches ();

#ifdef IMPL_DEBUG
        fprintf (stderr, "search done...\n");
#endif
		return FALSE;
	}

	GtkMenu *create_search_menu()
	{
		GtkWidget *menu = gtk_menu_new();
		append_option_item (menu, _("Name"), &name_opt);
		append_option_item (menu, _("Summary"), &summary_opt);
		append_option_item (menu, _("Description"), &descr_opt);
		append_option_item (menu, _("RPM Provides"), &provides_opt);
		append_option_item (menu, _("RPM Requires"), &requires_opt);
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

	inline static gboolean is_package_visible (GtkTreeModel *model, GtkTreeIter *iter,
	                                           PackageSelector *pThis, int visible_col)
	{
		gboolean visible, has_children;
        ZyppSelectablePtr selectable;

		gtk_tree_model_get (model, iter, visible_col, &visible, 8, &has_children,
                            0, &selectable, -1);

//        fprintf (stderr, "is visible '%s'\n", selectable ? selectable->name().c_str() : "<noname>");
		if (has_children)
			visible = TRUE;
		else if (visible && !pThis->m_search_queries.empty())
			visible = pThis->does_package_match (selectable);

		return visible;
	}

	static gboolean is_package_installed (GtkTreeModel *model, GtkTreeIter *iter,
	                                      gpointer data)
	{ return is_package_visible (model, iter, (PackageSelector *) data, 3); }

	static gboolean is_package_available (GtkTreeModel *model, GtkTreeIter *iter,
	                                      gpointer data)
	{ return is_package_visible (model, iter, (PackageSelector *) data, 4); }

    bool does_package_match_one (ZyppSelectablePtr sel, string key)
    {
		ZyppObject obj = NULL;

        if (name_opt && YGUtils::contains (sel->name(), key))
            return TRUE;
        
        if (summary_opt || descr_opt || provides_opt || requires_opt)
            obj = sel->theObj();

        if (summary_opt && YGUtils::contains (fastGetSummary (obj), key))
            return TRUE;
        if (descr_opt && YGUtils::contains (obj->description(), key))
            return TRUE;
        if (provides_opt) {
            const zypp::CapSet &capSet = obj->dep (zypp::Dep::PROVIDES);
            for (zypp::CapSet::const_iterator it = capSet.begin();
                 it != capSet.end(); it++)
					if (YGUtils::contains (it->asString(), key))
						return TRUE;
        }
        if (requires_opt) {
            const zypp::CapSet &capSet = obj->dep (zypp::Dep::REQUIRES);
            for (zypp::CapSet::const_iterator it = capSet.begin();
                 it != capSet.end(); it++)
                if (YGUtils::contains (it->asString(), key))
                    return TRUE;
        }
        return FALSE;
    }

	gboolean does_package_match (ZyppSelectablePtr sel)
	{
		if (m_search_queries.empty())
			return true;

        bool and_results = true; // vs. or them ...

        bool result = and_results;
		for (list <string>::iterator it = m_search_queries.begin();
		     it != m_search_queries.end(); it++) {

            // TODO: googlify more ...
            if (*it == "OR") {
                and_results = false;
                continue;
            }
            if (*it == "AND") {
                and_results = true;
                continue;
            }
            bool match = does_package_match_one (sel, *it);
            result = and_results ? result && match : result || match;
		}
		return result;
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

    // For SLED10 / older gtk+'s ...
    static bool compat_gtk_tree_model_filter_convert_child_iter_to_iter (GtkTreeModelFilter *filter,
                                                                         GtkTreeIter        *filter_iter,
                                                                         GtkTreeIter        *child_iter)
    {
#if GTK_CHECK_VERSION(2,10,0)
        return gtk_tree_model_filter_convert_child_iter_to_iter (filter, filter_iter, child_iter);
#else // cut/paste from gtk+ HEAD...
        gboolean ret;
        GtkTreePath *child_path, *path;

        memset (filter_iter, 0, sizeof (GtkTreeIter));

        GtkTreeModel *child_model;
        g_object_get (G_OBJECT (filter), "child-model", &child_model, NULL);
        child_path = gtk_tree_model_get_path (child_model, child_iter);
        g_return_val_if_fail (child_path != NULL, FALSE);
        
        path = gtk_tree_model_filter_convert_child_path_to_path (filter,
                                                                 child_path);
        gtk_tree_path_free (child_path);
        
        if (!path)
            return FALSE;

        ret = gtk_tree_model_get_iter (GTK_TREE_MODEL (filter), filter_iter, path);
        gtk_tree_path_free (path);

        return ret;
#endif
    }

	static bool sync_tree_views_scroll (GtkTreeView *current_view, GtkTreeView *other_view,
	                                    GtkTreePath *current_path, bool select_it)
	{
		/* What we do here is to scroll the other view to the correspondent
		   package position. If the package isn't present in that view, we
		   iterate it so we get the closest package (with respect to alphabetic
		   sorting). */

		// converts the path from one model to the other
		GtkTreePath *_path, *other_path;
		_path = gtk_tree_model_filter_convert_path_to_child_path (
			GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (current_view)),
			current_path);
		if (!_path)
			return false;

		GtkTreeModel *base_model = gtk_tree_model_filter_get_model (
			GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (current_view)));

		GtkTreeIter iter, other_iter;
		gtk_tree_model_get_iter (base_model, &iter, _path);
		gtk_tree_path_free (_path);

        int timeout = 0;
        // Try to find a a similar item in the other view to synchronise with
		while (!compat_gtk_tree_model_filter_convert_child_iter_to_iter (
			GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (other_view)),
			&other_iter, &iter))
		{
			if (!gtk_tree_model_iter_next (base_model, &iter))
				return false;
            // This turns into N^3 very quickly if we search too hard
            if (timeout++ > 10)
                return false;
			select_it = false;  // not the same package -- dont select it then
		}

		other_path = gtk_tree_model_get_path (gtk_tree_view_get_model (other_view),
		                                      &other_iter);

		GdkRectangle cell_rect, visible_rect;
		gtk_tree_view_get_cell_area (other_view, other_path, NULL, &cell_rect);
		gtk_tree_view_get_visible_rect (other_view, &visible_rect);
		int y = visible_rect.y + cell_rect.y;

		gtk_tree_view_get_cell_area (current_view, current_path, NULL, &cell_rect);
		y -= cell_rect.y;  // offset

		gtk_tree_view_expand_to_path (other_view, other_path);
		YGUtils::tree_view_smooth_scroll_to_point (other_view, 0, y);
		if (select_it)
			gtk_tree_selection_select_path (gtk_tree_view_get_selection (other_view),
			                                other_path);
		gtk_tree_path_free (other_path);
		return true;
	}

	static void package_clicked_cb (GtkTreeSelection *selection, PackageSelector *pThis)
	{
		IMPL
		int selected_rows = gtk_tree_selection_count_selected_rows (selection);
		if (selected_rows == 0)
			return;

		static bool safeguard = false;
		if (safeguard)
			return;
		safeguard = true;

		GtkTreeView *tree_view = gtk_tree_selection_get_tree_view (selection);
		bool install_view = tree_view == GTK_TREE_VIEW (pThis->m_installed_view);
		GtkTreeView *other_view = GTK_TREE_VIEW (install_view ?
			pThis->m_available_view : pThis->m_installed_view);

		// unselect the other view
		gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (other_view));

		// select and scroll to the package in the other view (if it's listed there)
		GtkTreePath *path = 0;
		gtk_tree_view_get_cursor (tree_view, &path, NULL);
		if (path) {
			// function can be called to set some package visible
			gtk_tree_view_scroll_to_cell (tree_view, path, NULL, FALSE, 0, 0);

			sync_tree_views_scroll (tree_view, other_view, path, selected_rows == 1);
			gtk_tree_path_free (path);
		}

		// show package information
		ZyppSelectablePtr sel = 0;
		getSelectedPackage (tree_view, &sel);

		ZyppObject install_obj, available_obj;
		induceObjects (sel, install_obj, available_obj, NULL, NULL);

		ZyppObject &obj = install_view ? install_obj : available_obj;
		pThis->m_information_widget->setPackage (sel, obj);

		// change the install/remove button label to upgrade, or etc
		GtkLabel *install_label = GTK_LABEL (pThis->m_install_label),
		         *remove_label = GTK_LABEL (pThis->m_remove_label);
		if (selected_rows > 1) {
			gtk_label_set_text (install_label, _("_install"));
			gtk_label_set_text (remove_label, _("_remove"));
		}
		else {  // personalize
			// remove label
			if (sel && sel->toInstall())
				gtk_label_set_text (remove_label, _("_undo"));
			else
				gtk_label_set_text (remove_label, _("_remove"));
			// install label
			if (sel && sel->toDelete())
				gtk_label_set_text (install_label, _("_undo"));
			else if (!install_obj)
				gtk_label_set_text (install_label, _("_install"));
			else if (available_obj) {
				int res = zypp::Edition::compare (install_obj->edition(),
				                                  available_obj->edition());
				if (res < 0)
					gtk_label_set_text (install_label, _("_upgrade"));
				else if (res > 0)
					gtk_label_set_text (install_label, _("_downgrade"));
				else
					gtk_label_set_text (install_label, _("re-_install"));
			}
		}
		gtk_label_set_use_underline (install_label, TRUE);
		gtk_label_set_use_underline (remove_label, TRUE);

		safeguard = false;
	}

	// install/remove/update/... selected packages
	void markSelectedPackage (GtkTreeView *view, bool available, bool installed)
	{
		IMPL
		/* Since the filter model gets changed, get paths for the base model and
		   then do our stuff. */
		GList *selected = gtk_tree_selection_get_selected_rows (
		                      gtk_tree_view_get_selection (view), NULL);

		int selected_len = g_list_length (selected);
		GtkTreeIter iters [selected_len];

		GtkTreeModel *filter_model = gtk_tree_view_get_model (view);
		GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER (filter_model);
		GtkTreeModel *model = gtk_tree_model_filter_get_model (filter);

		int i = 0;
		for (GList *it = selected; it; it = it->next, i++) {
			GtkTreePath *path = (GtkTreePath *) it->data;

			GtkTreeIter filter_iter, iter;
			gtk_tree_model_get_iter (filter_model, &filter_iter, path);
			gtk_tree_path_free (path);

			gtk_tree_model_filter_convert_iter_to_child_iter (filter, &iter, &filter_iter);
			iters[i] = iter;
		}
		g_list_free (selected);
		for (i = 0; i < selected_len; i++) {
			GtkTreeIter *iter = &iters[i];
			ZyppSelectablePtr sel = 0;
			gtk_tree_model_get (model, iter, 0, &sel, -1);
			if (sel && mark_selectable (sel, installed) /* install/remove */) {
				loadPackageRow (model, iter, sel);  // update model

				// mark also children (eg. for patterns and languages)
				GtkTreeIter child;
				if (gtk_tree_model_iter_children (model, &child, iter)) {
					do {
						gtk_tree_model_get (model, &child, 0, &sel, -1);
						if (sel && mark_selectable (sel, installed))
							loadPackageRow (model, &child, sel);
					} while (gtk_tree_model_iter_next (model, &child));

				}
			}
		}

		m_disk_table->update();

		// select path, so the buttons get updated and all (hacky)
		GtkTreeView *other_view = GTK_TREE_VIEW (
			GTK_WIDGET (view) == m_installed_view ? m_available_view : m_installed_view);
		package_clicked_cb (gtk_tree_view_get_selection (other_view), this);
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

	static gboolean dont_select_groups_cb (GtkTreeSelection *selection, GtkTreeModel *model,
	                                       GtkTreePath *path, gboolean selected, gpointer data)
	{
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter (model, &iter, path)) {
			ZyppSelectablePtr selectable;
			gtk_tree_model_get (model, &iter, 0, &selectable, -1);
			return selectable != NULL;
		}
		return FALSE;
	}
};

// The ordinary package selector that displays all packages
class YGPackageSelector : public YPackageSelector, public YGWidget
{
	PackageSelector *m_package_selector;

    static bool confirm_cb (void *pThis)
    {
        return ((YGPackageSelector *)pThis)->checkDelete();
    }

    bool checkDelete()
    {
        GtkWidget *dialog;

        bool changed =
            zyppPool().diffState<zypp::Package  >()	||
            zyppPool().diffState<zypp::Pattern  >()	||
            zyppPool().diffState<zypp::Selection>() ||
            zyppPool().diffState<zypp::Language >() ||
            zyppPool().diffState<zypp::Patch    >();

        if (!changed)
            return true;

		dialog = gtk_message_dialog_new_with_markup
            (YGUI::ui()->currentWindow(),
			 GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING,
             GTK_BUTTONS_OK_CANCEL,
             _("<b>Abandon all changes ?</b>"));
        gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_CANCEL);
        bool ok = gtk_dialog_run (GTK_DIALOG (dialog)) ==
                                   GTK_RESPONSE_OK;
		gtk_widget_destroy (dialog);
        return ok;
    }

public:
	YGPackageSelector (const YWidgetOpt &opt, YGWidget *parent)
		: YPackageSelector (opt),
		  YGWidget (this, parent, true, YGTK_TYPE_WIZARD, NULL)
	{
		setBorder (0);

        YGDialog *dialog = YGUI::ui()->currentYGDialog();

		GtkWindow *window = GTK_WINDOW (dialog->getWindow());
		gtk_window_resize (window, 680, 580);

        dialog->setCloseCallback (confirm_cb, this);

		YGtkWizard *wizard = YGTK_WIZARD (getWidget());

		ygtk_wizard_set_header_icon (wizard, window,
			THEMEDIR "/icons/32x32/apps/yast-software.png");
		ygtk_wizard_set_header_text (wizard, YGUI::ui()->currentWindow(),
		                             "Package Selector");
		ygtk_wizard_set_help_text (wizard,
			_("Two pools are presented; one with the available software, the other "
			"with the installed one. To install software you choose a package "
			"from the install pool and press Install. Similar method for removal "
			"of software. When you are done press the Accept button.<br>"
			"Information on a given package is displayed on the Package Information "
			"expander at the bottom which may be enlarged.<br>"
			"A categories view of the software is possible, as well as searching "
			"for a given package.")
		);

		ygtk_wizard_set_abort_button_label (wizard, _("_Cancel"));
		ygtk_wizard_set_abort_button_id (wizard, g_strdup ("cancel"), g_free);
		ygtk_wizard_set_back_button_label (wizard, "");
		ygtk_wizard_set_next_button_label (wizard, _("_Accept"));
		ygtk_wizard_set_next_button_id (wizard, g_strdup ("accept"), g_free);
		g_signal_connect (G_OBJECT (getWidget()), "action-triggered",
		                  G_CALLBACK (wizard_action_cb), this);

		m_package_selector = new PackageSelector (!opt.searchMode.value());
		gtk_container_add (GTK_CONTAINER (wizard), m_package_selector->getWidget());
	}

	virtual ~YGPackageSelector()
	{
		delete m_package_selector;
	}

protected:
	static void wizard_action_cb (YGtkWizard *wizard, gpointer id,
	                              gint id_type, YGPackageSelector *pThis)
	{
		IMPL
		const gchar *action = (gchar *) id;

		if (!strcmp (action, "accept")) {
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
            if (pThis->checkDelete())
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

		GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Changes Summary"),
			YGUI::ui()->currentWindow(),
			GtkDialogFlags (GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, _("C_onfirm"), GTK_RESPONSE_ACCEPT, NULL);

		GtkWidget *install_label, *remove_label, *install_view, *remove_view;
		install_label = gtk_label_new (_("To install:"));
		remove_label  = gtk_label_new (_("To remove:"));
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
			column = gtk_tree_view_column_new_with_attributes (_("Install packages"),
				gtk_cell_renderer_text_new(), "text", 0, NULL);
			gtk_tree_view_append_column (GTK_TREE_VIEW (install_view), column);

			gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (install_view), FALSE);
			gtk_tree_view_set_model (GTK_TREE_VIEW (install_view),
			                         GTK_TREE_MODEL (install_store));
			g_object_unref (G_OBJECT (install_store));
			gtk_tree_selection_set_mode (gtk_tree_view_get_selection (
				GTK_TREE_VIEW (install_view)), GTK_SELECTION_NONE);;

			// remove view
			column = gtk_tree_view_column_new_with_attributes (_("Remove packages"),
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
	else
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
	y2error ("The GTK+ UI does not support PkgSpecial subwidgets!");
	return 0;
}
