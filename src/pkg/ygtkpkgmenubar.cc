/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* YGtkPkgMenuBar, menu bar */
// check the header file for information about this widget

/*
  Textdomain "yast2-gtk"
 */

#include "YGi18n.h"
#include "config.h"
#include "YGDialog.h"
#include "YGUtils.h"
#include "ygtkpkgmenubar.h"
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>
#include "yzyppwrapper.h"
#include "YGPackageSelector.h"

// utilities

static GtkWidget *append_menu_item (GtkWidget *menu, const char *text,
	const char *stock, GCallback callback, gpointer callback_data)
{
	GtkWidget *item;
	if (stock && text) {
		GtkWidget *icon = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_MENU);
		item = gtk_image_menu_item_new_with_label (text);
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), icon);
	}
	else if (text)
		item = gtk_menu_item_new_with_label (text);
	else if (stock)
		item = gtk_image_menu_item_new_from_stock (stock, NULL);
	else
		item = gtk_separator_menu_item_new();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	if (callback)
		g_signal_connect (G_OBJECT (item), "activate", callback, callback_data);
	return item;
}

static GtkWidget *append_check_menu_item (GtkWidget *menu, const char *text,
	bool checked, GCallback callback, gpointer callback_data)
{
	GtkWidget *item = gtk_check_menu_item_new_with_label (text);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), checked);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	if (callback)
		g_signal_connect (G_OBJECT (item), "toggled", callback, callback_data);
	return item;
}

static void errorMsg (const std::string &message)
{
	GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
		GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
		_("Error"));
	gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
		"%s", message.c_str());
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

// callback implementations
// code from yast2-qt

#include <zypp/SysContent.h>
#include <zypp/ui/Status.h>
#include <zypp/ui/Selectable.h>
#include <zypp/ResPoolProxy.h>
#include <zypp/ZYppFactory.h>
#include <fstream>

using zypp::ui::S_Protected;
using zypp::ui::S_Taboo;
using zypp::ui::S_Del;
using zypp::ui::S_Update;
using zypp::ui::S_Install;
using zypp::ui::S_AutoDel;
using zypp::ui::S_AutoUpdate;
using zypp::ui::S_AutoInstall;
using zypp::ui::S_KeepInstalled;
using zypp::ui::S_NoInst;
typedef zypp::ui::Selectable::Ptr		ZyppSel;
typedef zypp::ui::Status			ZyppStatus;
typedef zypp::ResPoolProxy::const_iterator	ZyppPoolIterator;
typedef zypp::ResPoolProxy			ZyppPool;
//inline ZyppPool		zyppPool()		{ return zypp::getZYpp()->poolProxy();	}
template<class T> ZyppPoolIterator zyppBegin()	{ return zyppPool().byKindBegin<T>();	}
template<class T> ZyppPoolIterator zyppEnd()	{ return zyppPool().byKindEnd<T>();	}
inline ZyppPoolIterator zyppPkgBegin()		{ return zyppBegin<zypp::Package>();	}
inline ZyppPoolIterator zyppPkgEnd()		{ return zyppEnd<zypp::Package>();	}
inline ZyppPoolIterator zyppPatternsBegin()	{ return zyppBegin<zypp::Pattern>();	}
inline ZyppPoolIterator zyppPatternsEnd()	{ return zyppEnd<zypp::Pattern>();	}

#define DEFAULT_EXPORT_FILE_NAME "user-packages.xml"

static void
importSelectable( ZyppSel		selectable,
				     bool		isWanted,
				     const char * 	kind )
{
    ZyppStatus oldStatus = selectable->status();
    ZyppStatus newStatus = oldStatus;

    if ( isWanted )
    {
	//
	// Make sure this selectable does not get installed
	//

	switch ( oldStatus )
	{
	    case S_Install:
	    case S_AutoInstall:
	    case S_KeepInstalled:
	    case S_Protected:
	    case S_Update:
	    case S_AutoUpdate:
		newStatus = oldStatus;
		break;

	    case S_Del:
	    case S_AutoDel:
		newStatus = S_KeepInstalled;
		yuiDebug() << "Keeping " << kind << " " << selectable->name() << endl;
		break;

	    case S_NoInst:
	    case S_Taboo:

		if ( selectable->hasCandidateObj() )
		{
		    newStatus = S_Install;
		    yuiDebug() << "Adding " << kind << " " <<  selectable->name() << endl;
		}
		else
		{
		    yuiDebug() << "Can't add " << kind << " " << selectable->name()
			       << ": No candidate" << endl;
		}
		break;
	}
    }
    else // ! isWanted
    {
	//
	// Make sure this selectable does not get installed
	//

	switch ( oldStatus )
	{
	    case S_Install:
	    case S_AutoInstall:
	    case S_KeepInstalled:
	    case S_Protected:
	    case S_Update:
	    case S_AutoUpdate:
		newStatus = S_Del;
		yuiDebug() << "Deleting " << kind << " " << selectable->name() << endl;
		break;

	    case S_Del:
	    case S_AutoDel:
	    case S_NoInst:
	    case S_Taboo:
		newStatus = oldStatus;
		break;
	}
    }

    if ( oldStatus != newStatus )
	selectable->setStatus( newStatus );
}

static void import_file_cb (GtkMenuItem *item)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new (_("Load Package List"),
		YGDialog::currentWindow(), GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name (filter, "*.xml");
	gtk_file_filter_add_pattern (filter, "*.xml");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name (filter, "*");
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), DEFAULT_EXPORT_FILE_NAME);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), TRUE);

	int ret = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    if ( ret == GTK_RESPONSE_ACCEPT )
    {
	char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	yuiMilestone() << "Importing package list from " << filename << endl;

	try
	{
	    std::ifstream importFile( filename );
	    zypp::syscontent::Reader reader( importFile );

	    //
	    // Put reader contents into maps
	    //

	    typedef zypp::syscontent::Reader::Entry	ZyppReaderEntry;
	    typedef std::pair<string, ZyppReaderEntry>	ImportMapPair;

	    map<string, ZyppReaderEntry> importPkg;
	    map<string, ZyppReaderEntry> importPatterns;

	    for ( zypp::syscontent::Reader::const_iterator it = reader.begin();
		  it != reader.end();
		  ++ it )
	    {
		string kind = it->kind();

		if      ( kind == "package" ) 	importPkg.insert     ( ImportMapPair( it->name(), *it ) );
		else if ( kind == "pattern" )	importPatterns.insert( ImportMapPair( it->name(), *it ) );
	    }

	    yuiDebug() << "Found "        << importPkg.size()
		       <<" packages and " << importPatterns.size()
		       << " patterns in " << filename
		       << endl;


	    //
	    // Set status of all patterns and packages according to import map
	    //

	    for ( ZyppPoolIterator it = zyppPatternsBegin();
		  it != zyppPatternsEnd();
		  ++it )
	    {
		ZyppSel selectable = *it;
		importSelectable( *it, importPatterns.find( selectable->name() ) != importPatterns.end(), "pattern" );
	    }

	    for ( ZyppPoolIterator it = zyppPkgBegin();
		  it != zyppPkgEnd();
		  ++it )
	    {
		ZyppSel selectable = *it;
		importSelectable( *it, importPkg.find( selectable->name() ) != importPkg.end(), "package" );
	    }

		YGPackageSelector::get()->popupChanges();
	}
	catch ( const zypp::Exception & exception )
	{
	    yuiWarning() << "Error reading package list from " << filename << endl;
	    char *text = g_strdup_printf (_("Error loading package list from '%s'"), filename);
		errorMsg (text);
		g_free (text);
	}

	g_free (filename);
	Ypp::runSolver();
    }


	gtk_widget_destroy (dialog);
}

static void export_file_cb (GtkMenuItem *item)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new (_("Save Package List"),
		YGDialog::currentWindow(), GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name (filter, "*.xml");
	gtk_file_filter_add_pattern (filter, "*.xml");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name (filter, "*");
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), DEFAULT_EXPORT_FILE_NAME);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), TRUE);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

	int ret = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

	if (ret == GTK_RESPONSE_ACCEPT) {
	char *filename;
	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

	zypp::syscontent::Writer writer;
	const zypp::ResPool & pool = zypp::getZYpp()->pool();

	// The ZYPP obfuscated C++ contest proudly presents:

	for_each( pool.begin(), pool.end(),
		  boost::bind( &zypp::syscontent::Writer::addIf,
			       boost::ref( writer ),
			       _1 ) );
	// Yuck. What a mess.
	//
	// Does anybody seriously believe this kind of thing is easier to read,
	// let alone use? Get real. This is an argument in favour of all C++
	// haters. And it's one that is really hard to counter.
	//
	// -sh 2006-12-13

	try
	{
	    std::ofstream exportFile( filename );
	    exportFile.exceptions( std::ios_base::badbit | std::ios_base::failbit );
	    exportFile << writer;

	    yuiMilestone() << "Package list exported to " << filename << endl;
	}
	catch ( std::exception & exception )
	{
	    yuiWarning() << "Error exporting package list to " << filename << endl;

	    // The export might have left over a partially written file.
	    // Try to delete it. Don't care if it doesn't exist and unlink() fails.
	    g_remove (filename);

	    // Post error popup
	    char *text = g_strdup_printf (_("Error exporting package list to '%s'"), filename);
		errorMsg (text);
		g_free (text);
	}

	g_free (filename);
   }
}

static void create_solver_testcase_cb (GtkMenuItem *item)
{
	const char *dirname = "/var/log/YaST2/solverTestcase";
	std::string msg = _("Use this to generate extensive logs to help tracking down "
	                  "bugs in the dependency resolver.\nThe logs will be stored in "
	                  "directory: ");
	msg += dirname;

	GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
		GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK_CANCEL,
		"%s", _("Create Dependency Resolver Test Case"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", msg.c_str());
	int ret = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	if (ret == GTK_RESPONSE_OK) {
		bool success;
		yuiMilestone() << "Generating solver test case START" << endl;
		success = zypp::getZYpp()->resolver()->createSolverTestcase (dirname);
		yuiMilestone() << "Generating solver test case END" << endl;

	    if (success) {
			GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
				GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION,
				GTK_BUTTONS_YES_NO, "%s", _("Success"));
			msg = _("Dependency resolver test case written to");
			msg += " <tt>";
			msg += dirname;
			msg += "</tt>\n";
			msg += _("Prepare <tt>y2logs.tgz tar</tt> archive to attach to Bugzilla?");
			gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
                                                                                "%s", msg.c_str());
			ret = gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
			if (ret == GTK_RESPONSE_YES)
				YGUI::ui()->askSaveLogs();
	    }
	    else {
	    	msg = _("Failed to create dependency resolver test case.\n"
				"Please check disk space and permissions for");
			msg += " <tt>";
			msg += dirname;
			msg += "</tt>";
			errorMsg (msg.c_str());
	    }
	}
}

static void repoManager()
{
    yuiMilestone() << "Closing PackageSelector with \"RepoManager\"" << endl;
    YGUI::ui()->sendEvent( new YMenuEvent( "repo_mgr" ) );
}

static void onlineUpdateConfiguration()
{
    yuiMilestone() << "Closing PackageSelector with \"OnlineUpdateConfiguration\"" << endl;
    YGUI::ui()->sendEvent( new YMenuEvent( "online_update_configuration" ) );
}

static void webpinSearch()
{
    yuiMilestone() << "Closing PackageSelector with \"webpin\"" << endl;
    YGUI::ui()->sendEvent( new YMenuEvent( "webpin" ) );
}

static void manualResolvePackageDependencies()
{ Ypp::runSolver (true); }

static void auto_check_cb (GtkCheckMenuItem *item)
{ Ypp::setEnableSolver (gtk_check_menu_item_get_active (item)); }

static void show_devel_pkgs_cb (GtkCheckMenuItem *item)
{
	bool exclude = !gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item));
	YGPackageSelector::get()->filterPkgSuffix ("-devel", exclude);
}

static void show_debug_pkgs_cb (GtkCheckMenuItem *item)
{
	bool exclude = !gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item));
	YGPackageSelector::get()->filterPkgSuffix ("-debuginfo", exclude);
	YGPackageSelector::get()->filterPkgSuffix ("-debugsource", exclude);
}

static void system_verification_mode_cb (GtkCheckMenuItem *item)
{
	bool on = gtk_check_menu_item_get_active (item);
	zypp::getZYpp()->resolver()->setSystemVerification (on);
}

static void installSubPkgs (std::string suffix)
{
    // Find all matching packages and put them into a QMap

    std::map<std::string, ZyppSel> subPkgs;

    for ( ZyppPoolIterator it = zyppPkgBegin(); it != zyppPkgEnd(); ++it )
    {
	std::string name = (*it)->name();

	if (YGUtils::endsWith (name, suffix))
	{
	    subPkgs[ name ] = *it;
	    yuiDebug() << "Found subpackage: " << name << endl;
	}
    }


    // Now go through all packages and look if there is a corresponding subpackage in the QMap

    for ( ZyppPoolIterator it = zyppPkgBegin(); it != zyppPkgEnd(); ++it )
    {
	std::string name = (*it)->name();

    std::string subPkgName( name + suffix );
	if ( subPkgs.find( subPkgName ) != subPkgs.end() )
	{
	    ZyppSel subPkg = subPkgs[ subPkgName ];

	    switch ( (*it)->status() )
	    {
		case S_AutoDel:
		case S_NoInst:
		case S_Protected:
		case S_Taboo:
		case S_Del:
		    // Don't install the subpackage
		    yuiMilestone() << "Ignoring unwanted subpackage " << subPkgName << endl;
		    break;

		case S_AutoInstall:
		case S_Install:
		case S_KeepInstalled:

		    // Install the subpackage, but don't try to update it

		    if ( ! subPkg->installedObj() )
		    {
			subPkg->setStatus( S_Install );
			yuiMilestone() << "Installing subpackage " << subPkgName << endl;
		    }
		    break;


		case S_Update:
		case S_AutoUpdate:

		    // Install or update the subpackage

		    if ( ! subPkg->installedObj() )
		    {
			subPkg->setStatus( S_Install );
			yuiMilestone() << "Installing subpackage " << subPkgName << endl;
		    }
		    else
		    {
			subPkg->setStatus( S_Update );
			yuiMilestone() << "Updating subpackage " << subPkgName << endl;
		    }
		    break;

		    // Intentionally omitting 'default' branch so the compiler can
		    // catch unhandled enum states
	    }
	}
    }

	Ypp::runSolver();
	YGPackageSelector::get()->popupChanges();
}

static void install_all_devel_pkgs_cb()
{ installSubPkgs ("-devel"); }

static void install_all_debug_info_pkgs_cb()
{ installSubPkgs ("-debuginfo"); }

static void install_all_debug_source_pkgs_cb()
{ installSubPkgs ("-debugsource"); }

static void show_pkg_changes_cb()
{ YGPackageSelector::get()->popupChanges(); }


#include "ygtkpkghistorydialog.h"

static void show_log_changes_cb()
{ popupHistoryDialog(); }

static void reset_ignored_dependency_conflicts_cb()
{ zypp::getZYpp()->resolver()->undo(); }

#include "ygtkpkgproductdialog.h"

static void show_products_cb()
{
	YGtkPkgProductDialog dialog;
	dialog.popup();
}

static void accept_item_cb (GtkMenuItem *item, YGPackageSelector *selector)
{ selector->apply(); }

static void reject_item_cb (GtkMenuItem *item, YGPackageSelector *selector)
{ selector->cancel(); }

YGtkPkgMenuBar::YGtkPkgMenuBar()
{
	YGPackageSelector *selector = YGPackageSelector::get();
	m_menu = gtk_menu_bar_new();

	GtkWidget *menu_bar = m_menu, *item, *submenu;
	item = append_menu_item (menu_bar, _("File"), NULL, NULL, NULL);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), (submenu = gtk_menu_new()));
		append_menu_item (submenu, _("Import..."), NULL,
			G_CALLBACK (import_file_cb), this);
		append_menu_item (submenu, _("Export..."), NULL,
			G_CALLBACK (export_file_cb), this);
		append_menu_item (submenu, NULL, NULL, NULL, NULL);
		append_menu_item (submenu, NULL, GTK_STOCK_APPLY, G_CALLBACK (accept_item_cb), selector);
		append_menu_item (submenu, NULL, GTK_STOCK_QUIT, G_CALLBACK (reject_item_cb), selector);
	if (selector->repoMgrEnabled()) {
		item = append_menu_item (menu_bar, _("Configuration"), NULL, NULL, NULL);
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), (submenu = gtk_menu_new()));
			append_menu_item (submenu, _("Repositories..."), NULL,
				G_CALLBACK (repoManager), this);
			append_menu_item (submenu, _("Online Update..."), NULL,
				G_CALLBACK (onlineUpdateConfiguration), this);
			append_menu_item (submenu, _("Search Packages on Web..."), NULL,
				G_CALLBACK (webpinSearch), this);
	}
	item = append_menu_item (menu_bar, _("Dependencies"), NULL, NULL, NULL);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), (submenu = gtk_menu_new()));
		append_menu_item (submenu, _("Check Now"), NULL,
			G_CALLBACK (manualResolvePackageDependencies), this);
		append_check_menu_item (submenu, _("Autocheck"), Ypp::isSolverEnabled(),
			G_CALLBACK (auto_check_cb), this);
	item = append_menu_item (menu_bar, _("Options"), NULL, NULL, NULL);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), (submenu = gtk_menu_new()));
		// Translators: don't translate the "-devel"
		append_check_menu_item (submenu, _("Show -devel packages"),
			true, G_CALLBACK (show_devel_pkgs_cb), this);
		// Translators: don't translate the "-debuginfo/-debugsource" part
		append_check_menu_item (submenu, _("Show -debuginfo/-debugsource Packages"),
			true, G_CALLBACK (show_debug_pkgs_cb), this);
		append_check_menu_item (submenu, _("System Verification Mode"),
			zypp::getZYpp()->resolver()->systemVerification(),
			G_CALLBACK (system_verification_mode_cb), this);
	item = append_menu_item (menu_bar, _("Extras"), NULL, NULL, NULL);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), (submenu = gtk_menu_new()));
		append_menu_item (submenu, _("Show Products"), NULL,
			G_CALLBACK (show_products_cb), this);
		append_menu_item (submenu, _("Show Package Changes"), NULL,
			G_CALLBACK (show_pkg_changes_cb), this);
		if (!selector->onlineUpdateMode())
			append_menu_item (submenu, _("Show History of Changes"), NULL,
				G_CALLBACK (show_log_changes_cb), this);
		append_menu_item (submenu, NULL, NULL, NULL, NULL);
		append_menu_item (submenu, _("Install All Matching -devel Packages"), NULL,
			G_CALLBACK (install_all_devel_pkgs_cb), this);
		append_menu_item (submenu, _("Install All Matching -debug-info Packages"), NULL,
			G_CALLBACK (install_all_debug_info_pkgs_cb), this);
		append_menu_item (submenu, _("Install All Matching -debug-source Packages"), NULL,
			G_CALLBACK (install_all_debug_source_pkgs_cb), this);
		append_menu_item (submenu, NULL, NULL, NULL, NULL);
		append_menu_item (submenu, _("Generate Dependency Resolver Test Case"), NULL,
			G_CALLBACK (create_solver_testcase_cb), this);
		append_menu_item (submenu, _("Reset Ignored Dependency Conflicts"), NULL,
			G_CALLBACK (reset_ignored_dependency_conflicts_cb), this);

	gtk_widget_show_all (m_menu);
}

