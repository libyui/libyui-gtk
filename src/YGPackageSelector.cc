/* Yast-GTK */
#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YGWidget.h"
#include "YPackageSelector.h"
#include "ygtkrichtext.h"
#include "ygtkwizard.h"

#ifndef DISABLE_PACKAGE_SELECTOR
#include <zypp/ZYppFactory.h>
#include <zypp/ResObject.h>
#include <zypp/ResPoolProxy.h>
#include <zypp/ui/Selectable.h>
#include <zypp/Patch.h>
#include <zypp/Selection.h>
#include <zypp/Pattern.h>

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
inline ZyppPackage tryCastToZyppPkg (ZyppObject obj)
	{ return zypp::dynamic_pointer_cast <const zypp::Package> (obj); }
inline ZyppPatch tryCastToZyppPatch (ZyppObject obj)
	{ return zypp::dynamic_pointer_cast <const zypp::Patch> (obj); }
inline ZyppSelection tryCastToZyppSelection (ZyppObject obj)
	{ return zypp::dynamic_pointer_cast <const zypp::Selection> (obj); }
inline ZyppPattern tryCastToZyppPattern (ZyppObject obj)
	{ return zypp::dynamic_pointer_cast <const zypp::Pattern> (obj); }

static bool acceptLicense (ZyppSelectablePtr sel)
{
// TODO. This should be done, but results in some linkage problems here.
// It seems there is some mismatch between the headers and the library. :/
	return true;
#if 0
	ZyppObject object = sel->candidateObj();
	ZyppPackage package = tryCastToZyppPkg (object);
	if (!package)
		return true;

	const string &license = package->licenseToConfirm();
	if (license.empty())
		return true;

	GtkWidget *dialog = gtk_dialog_new_with_buttons ("Confirm License",
		GTK_WINDOW (YGUI::ui()->currentGtkDialog()), GTK_DIALOG_MODAL,
		"C_onfirm", 1, GTK_STOCK_CANCEL, 0, NULL);

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

	bool confirmed = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	sel->setLicenceConfirmed (confirmed);
	return confirmed;
#endif
}

// install / remove ZyppSelectables convinience wrapper
// returns true on sucess
static bool mark_selectable (ZyppSelectablePtr selectable,
                             bool install /*or remove*/)
{
	zypp::ui::Status status = selectable->status();

	if (install && !acceptLicense (selectable))
		return false;

	if (install) {
		if (status == zypp::ui::S_KeepInstalled)
			status = zypp::ui::S_Update;
		if (status == zypp::ui::S_Del)
			status = zypp::ui::S_KeepInstalled;
		else
			status = zypp::ui::S_Install;
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

#define INFORMATION_HEIGHT 120

// Expander widget with the package information
class PackageInformation
{
	GtkWidget *m_widget;
	// Information text
	GtkWidget *m_description_text, *m_filelist_text, *m_history_text;
	GtkWidget *m_scrolled_window;

public:
	PackageInformation (bool only_description)
	{
		// TODO: set white color on expanders
		GtkWidget *padding_widget, *vbox, *view_port;
		m_widget = gtk_expander_new ("Package Information");

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

			g_signal_connect (G_OBJECT (vbox), "size-request",
			                  G_CALLBACK (size_changed_cb), m_scrolled_window);
		}

		gtk_widget_set_size_request (m_scrolled_window, -1, INFORMATION_HEIGHT);
	}

	GtkWidget *getWidget()
	{ return m_widget; }

	void setPackage (ZyppSelectablePtr selectable)
	{
		string description = "<i>(no package selected)</i>", file_list, history;
		if (selectable) {
			ZyppObject object = selectable->theObj();

			description = "<p><b>" + selectable->name() + "</b> - "
			              + object->summary() + "</p>"
//FIXME
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
		                        description.c_str(), FALSE, FALSE);
		if (m_filelist_text)
			ygtk_richtext_set_text (YGTK_RICHTEXT (m_filelist_text),
			                        file_list.c_str(), TRUE, TRUE);
		if (m_history_text)
			ygtk_richtext_set_text (YGTK_RICHTEXT (m_history_text),
			                        history.c_str(), FALSE, TRUE);

		size_changed_cb (0, 0, m_scrolled_window);
	}

	// This forces GtkScrolledWindow to set "new" scrollbars to reflect
	// a new box size (due to expanders being activated)
	static void size_changed_cb (GtkWidget *widget, GtkRequisition *requisition,
	                             GtkWidget *view)
	{
		gtk_scrolled_window_set_vadjustment (GTK_SCROLLED_WINDOW (view), NULL);
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
		ygtk_wizard_set_header_text (wizard, "Patch Selector");
		ygtk_wizard_set_header_icon (wizard, NULL,
			THEMEDIR "/icons/32x32/apps/yast-software.png");
		ygtk_wizard_set_help_text (wizard,
			"For information on a given patch, just press it and as well as the"
			"Package Information expander to make those informations visible.<br>"
			"To install a patch you just need to press the check button next to it"
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

		m_information_widget = new PackageInformation (true);
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
		                  G_CALLBACK (YGUtils::tree_view_toggle_cb), m_patches_model);

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

			if (patch) {
				GtkTreeIter iter;
				GtkListStore *store = GTK_LIST_STORE (m_patches_model);
				gtk_list_store_append (store, &iter);
				gtk_list_store_set (store, &iter, 0, FALSE,
					1, patch->category().c_str(), 2, selectable->name().c_str(),
					3, get_pointer (selectable), -1);
			}
		}

		gtk_tree_view_set_model (GTK_TREE_VIEW (m_patches_view), m_patches_model);

		g_signal_connect (G_OBJECT (m_patches_view), "cursor-changed",
		                  G_CALLBACK (package_clicked_cb), this);
	}

	virtual ~YGPatchSelector()
	{
		IMPL
		delete m_information_widget;
	}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_KEYBOARD_FOCUS

	static ZyppSelectablePtr selectedPackage (GtkTreeView *tree_view)
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
	static void package_clicked_cb (GtkTreeView *tree_view, YGPatchSelector *pThis)
	{
		IMPL
		ZyppSelectablePtr sel = selectedPackage (tree_view);
		pThis->m_information_widget->setPackage (sel);
	}

	static void wizard_action_cb (YGtkWizard *wizard, gpointer id,
	                              gint id_type, YGPatchSelector *pThis)
	{
		const gchar *action = (gchar *) id;
		if (!strcmp (action, "install")) {
			y2milestone ("Closing PackageSelector with 'accept'");
			YGUI::ui()->sendEvent (new YMenuEvent (YCPSymbol ("accept")));
		}
		else if (!strcmp (action, "cancel")) {
			y2milestone ("Closing PackageSelector with 'cancel'");
			YGUI::ui()->sendEvent (new YCancelEvent());
		}
	}
};

// Pattern selector
class PatternSelector
{
	GtkWidget *m_widget;
//	list < pair <GtkWidget *, 

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

		GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
		gtk_container_add (GTK_CONTAINER (view_port), vbox);

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
		for (map <string, vector <ZyppSelectablePtr> >::iterator it = categories.begin();
		     it != categories.end(); it++) {
			const vector <ZyppSelectablePtr> &patterns = it->second;

			GtkWidget *label = gtk_label_new (
				("<big><b>" + it->first + "</b></big>").c_str());
			gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
			gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 6);

			for (unsigned int i = 0; i < patterns.size(); i++) {
				ZyppSelectablePtr selectable = patterns[i];
				ZyppObject object = selectable->theObj();
				ZyppSelection selection = tryCastToZyppSelection (object);
				ZyppPattern pattern = tryCastToZyppPattern (object);

				if (selection && selection->visible()) {
					GtkWidget *hbox, *button, *image = 0;

					hbox = gtk_hbox_new (FALSE, 0);
					button = gtk_check_button_new ();
					g_signal_connect (G_OBJECT (button), "toggled",
					                  G_CALLBACK (selection_toggled_cb), selectable);

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
							icon = THEMEDIR "/icons/48x48/apps/" + icon + ".png";
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

					gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 6);
				}
			}
		}
		gtk_widget_show_all (m_widget);
	}

	GtkWidget *getWidget()
	{ return m_widget; }

	static void selection_toggled_cb (GtkToggleButton *button,
	                                  ZyppSelectablePtr selectable)
	{
		IMPL
		mark_selectable (selectable, gtk_toggle_button_get_active (button));
	}

#if 0
	set <string> get_packages_to (bool install)
	{
		IMPL
		zypp::ui::Status status = install ? zypp::ui::S_KeepInstalled
		                                  : zypp::ui::S_NoInst;

		set <string> packages;
		GList *buttons = gtk_container_get_children (GTK_CONTAINER (m_buttons));
		GList *i;
		for (i = g_list_first (buttons); i; i = i->next) {
			GtkToggleButton *button = GTK_TOGGLE_BUTTON (i->data);
			ZyppSelectablePtr selectable;
			selectable = (ZyppSelectablePtr) g_object_get_data
			                                     (G_OBJECT (button), "selectable");
			if (gtk_toggle_button_get_active (button) && selectable->status() != status) {
				selectable->set_status (status);
				const set <string> &_packages =
					(tryCastToZyppSelection (selectable->theObj()))->install_packages();
				packages.insert (_packages.begin(), _packages.end());
			}
		}

		g_list_free (buttons);
		return packages;
	}
#endif
};

// Package selector's widget
class PackageSelector
{
friend class YGPackageSelector;

	GtkWidget *m_widget;

	// Models with the actual information (plan and category views)
	GtkListStore *m_packages_list;
	GtkTreeStore *m_packages_tree;

	// Models with the filtered information
	// (filtered by search or because they were moved to the other pool.)
	GtkTreeModel *m_installed_list, *m_available_list;
	GtkTreeModel *m_installed_tree, *m_available_tree;

	// The GtkTreeView widgets
	GtkWidget *m_installed_view, *m_available_view;

	// Package information widget
	PackageInformation *m_information_widget;

	// Have a button for patterns selection
	GtkWidget *m_patterns_button;

	// Search gizmos
	GtkWidget *m_search_entry;
	guint search_timeout_id;
	GtkWidget *m_categories_view_check;

public:
	PackageSelector (bool show_patterns_button)
	{
		m_widget = gtk_vbox_new (FALSE, 0);

		GtkWidget *packages_hbox = gtk_hbox_new (FALSE, 0);

		GtkWidget *available_box, *installed_box;
		available_box = createListWidget ("<b>Available Software:</b>",
		                                  "gtk-cdrom", m_available_view);
		installed_box = createListWidget ("<b>Installed Software:</b>",
		                                  "computer", m_installed_view);

		GtkWidget *selection_buttons_vbox, *install_button, *remove_button;
		selection_buttons_vbox = gtk_vbox_new (FALSE, 0);
		install_button = gtk_button_new_with_mnemonic ("_install >");
		remove_button = gtk_button_new_with_mnemonic ("< _remove");
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
		if (show_patterns_button) {
			m_patterns_button = gtk_button_new_with_mnemonic ("_Patterns...");
			gtk_box_pack_end (GTK_BOX (categories_hbox), m_patterns_button,
			                  FALSE, FALSE, 4);
		}

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

		m_information_widget = new PackageInformation (false);
		GtkWidget *pkg_info_widget = m_information_widget->getWidget();

		gtk_box_pack_start (GTK_BOX (m_widget), packages_hbox, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_widget), categories_hbox, FALSE, FALSE, 6);
		gtk_box_pack_start (GTK_BOX (m_widget), search_hbox, FALSE, FALSE, 2);
		gtk_box_pack_start (GTK_BOX (m_widget), pkg_info_widget, FALSE, FALSE, 6);
		gtk_widget_show_all (m_widget);

		// Create a model for the package lists
		// models' columns: package name (string), selectable object (pointer),
		// is available package (boolean), is_installed_package (boolean),
		// the correspondent row of the other view (GtkTreePath*)
		m_packages_list = gtk_list_store_new (5, G_TYPE_STRING,  G_TYPE_POINTER,
		                         G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_POINTER);
		m_packages_tree = gtk_tree_store_new (5, G_TYPE_STRING,  G_TYPE_POINTER,
		                         G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_POINTER);

		// we need to create the categories tree as we iterate packages
		map <string, GtkTreePath*> tree;

		GtkTreeIter tree_iter, list_iter, iter_t;
		for (ZyppPool::const_iterator it = zyppPool().byKindBegin <zypp::Package>();
		     it != zyppPool().byKindEnd <zypp::Package>(); it++)
		{
			ZyppSelectable selectable = *it;
			ZyppObject object = selectable->theObj();

			GtkTreePath *tree_path = NULL;
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
						// doesn't exist -- create it
						if (!tree_path)  // root
							gtk_tree_store_append (m_packages_tree, &iter_t, NULL);
						else {
							gtk_tree_model_get_iter (GTK_TREE_MODEL (m_packages_tree),
							                         &tree_iter, tree_path);
							gtk_tree_store_append (m_packages_tree, &iter_t, &tree_iter);
						}
						gtk_tree_store_set (m_packages_tree, &iter_t,
							0, node.c_str(), 1, NULL, 2, TRUE, 3, TRUE, -1);

						tree_path = gtk_tree_model_get_path (GTK_TREE_MODEL (m_packages_tree),
						                                     &iter_t);
						tree [node] = tree_path;
					}
					else // exists
						tree_path = it->second;
				}
			}

			string name = selectable->name();
			if (selectable->hasBothObjects())
				name += " (" + object->edition().version() + ")";

			gtk_list_store_append (m_packages_list, &list_iter);
			gtk_list_store_set (m_packages_list, &list_iter,
				0, name.c_str(), 1, get_pointer (selectable),
				2, selectable->hasCandidateObj(), 3, selectable->hasInstalledObj(),
				4, gtk_tree_path_copy (tree_path), -1);
			GtkTreePath* list_path = gtk_tree_model_get_path
			                             (GTK_TREE_MODEL (m_packages_list), &list_iter);

			if (!tree_path) {
				g_warning ("YGPackageSelector: Package with no group."
				           "Something went wrong.");
				continue;
			}

			gtk_tree_model_get_iter (GTK_TREE_MODEL (m_packages_tree),
			                         &tree_iter, tree_path);
			gtk_tree_store_append (m_packages_tree, &iter_t, &tree_iter);

			gtk_tree_store_set (m_packages_tree, &iter_t,
				0, selectable->name().c_str(), 1, get_pointer (selectable),
				2, selectable->hasCandidateObj(), 3, selectable->hasInstalledObj(),
				4, list_path, -1);
		}

		// free GtkTreePaths
		for (map <string, GtkTreePath*>::iterator it = tree.begin();
		     it != tree.end(); it++)
			gtk_tree_path_free (it->second);

		GtkTreeViewColumn *column;
		column = gtk_tree_view_column_new_with_attributes ("Available",
			gtk_cell_renderer_text_new(), "text", 0, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (m_available_view), column);

		column = gtk_tree_view_column_new_with_attributes ("Installed",
			gtk_cell_renderer_text_new(), "text", 0, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (m_installed_view), column);

		// user should use the search entry
		gtk_tree_view_set_enable_search (GTK_TREE_VIEW (m_installed_view), FALSE);
		gtk_tree_view_set_enable_search (GTK_TREE_VIEW (m_available_view), FALSE);

		m_installed_list = gtk_tree_model_filter_new
		                       (GTK_TREE_MODEL (m_packages_list), NULL);
		m_available_list = gtk_tree_model_filter_new
		                       (GTK_TREE_MODEL (m_packages_list), NULL);
		m_installed_tree = gtk_tree_model_filter_new
		                       (GTK_TREE_MODEL (m_packages_tree), NULL);
		m_available_tree = gtk_tree_model_filter_new
		                       (GTK_TREE_MODEL (m_packages_tree), NULL);
		gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (m_installed_list),
		                                        is_package_installed, this, NULL);
		gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (m_available_list),
		                                        is_package_available, this, NULL);
		gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (m_installed_tree),
		                                        is_package_installed, this, NULL);
		gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (m_available_tree),
		                                        is_package_available, this, NULL);

		setPlainView();

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

	GtkWidget *createListWidget (const char *header, const char *icon,
	                             GtkWidget *&list)
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

		return vbox;
	}

	void free_tree_paths (GtkTreeModel *model, GtkTreeIter *iter)
	{
		if (gtk_tree_model_iter_has_child (model, iter)) {
			GtkTreeIter child;
			if (gtk_tree_model_iter_children (model, &child, iter))
				free_tree_paths (model, &child);
		}

		gpointer tree_path;
		gtk_tree_model_get (model, iter, 4, &tree_path, -1);
		gtk_tree_path_free ((GtkTreePath *) tree_path);

		if (gtk_tree_model_iter_next (model, iter))
			free_tree_paths (model, iter);
	}

	virtual ~PackageSelector()
	{
		IMPL
		if (search_timeout_id)
			g_source_remove (search_timeout_id);

		delete m_information_widget;

		// clean the GtkTreePath stored on the list and tree
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (m_packages_list), &iter))
			free_tree_paths (GTK_TREE_MODEL (m_packages_list), &iter);
		if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (m_packages_tree), &iter))
			free_tree_paths (GTK_TREE_MODEL (m_packages_tree), &iter);

		// removing all our models
		g_object_unref (G_OBJECT (m_packages_list));
		g_object_unref (G_OBJECT (m_packages_tree));
		g_object_unref (G_OBJECT (m_installed_list));
		g_object_unref (G_OBJECT (m_available_list));
		g_object_unref (G_OBJECT (m_installed_tree));
		g_object_unref (G_OBJECT (m_available_tree));
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
		pThis->search_timeout_id = g_timeout_add (2000, search_cb, pThis);
	}

	static gboolean search_cb (gpointer data)
	{
		IMPL
		PackageSelector *pThis = (PackageSelector *) data;
		string search = gtk_entry_get_text (GTK_ENTRY (pThis->m_search_entry));
		pThis->search_timeout_id = 0;

		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (pThis->m_installed_list));
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (pThis->m_available_list));

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pThis->m_categories_view_check),
		                              FALSE);
		return FALSE;
	}

	static gboolean is_package_installed (GtkTreeModel *model, GtkTreeIter *iter,
	                                      gpointer data)
	{
		IMPL
		gboolean visible;
		gtk_tree_model_get (model, iter, 3, &visible, -1);
		if (visible)
			visible = show_package (model, iter, (PackageSelector *) data);
		return visible;
	}

	static gboolean is_package_available (GtkTreeModel *model, GtkTreeIter *iter,
	                                      gpointer data)
	{
		IMPL
		gboolean visible;
		gtk_tree_model_get (model, iter, 2, &visible, -1);
		if (visible)
			visible = show_package (model, iter, (PackageSelector *) data);
		return visible;
	}

	// common stuff for is_package_installed and is_package_available (ie. search)
	static gboolean show_package (GtkTreeModel *model, GtkTreeIter *iter,
	                              PackageSelector *pThis)
	{
		IMPL
		gboolean visible = TRUE;
		if (GTK_IS_LIST_STORE (model)) {
			string search = gtk_entry_get_text (GTK_ENTRY (pThis->m_search_entry));
			if (!search.empty()) {
				ZyppSelectablePtr selectable = 0;
				gtk_tree_model_get (model, iter, 1, &selectable, -1);

				if (!YGUtils::contains (selectable->name(), search)) {
					// check also description and rpm provides
					ZyppObject object = selectable->theObj();
					if (!YGUtils::contains (object->description(), search)) {
						// test rpm-provides
						const zypp::CapSet &capSet = object->dep (zypp::Dep::PROVIDES);
						for (zypp::CapSet::const_iterator it = capSet.begin();
						     it != capSet.end(); it++)
							if ((visible = YGUtils::contains (it->index(), search)) == TRUE)
								break;
					}
				}
			}
		}
		return visible;
	}

	// set eiter plain or categories view
	void setPlainView()
	{
		gtk_tree_view_set_model(GTK_TREE_VIEW (m_available_view), m_available_list);
		gtk_tree_view_set_model(GTK_TREE_VIEW (m_installed_view), m_installed_list);
	}
	void setTreeView()
	{
		gtk_tree_view_set_model(GTK_TREE_VIEW (m_available_view), m_available_tree);
		gtk_tree_view_set_model(GTK_TREE_VIEW (m_installed_view), m_installed_tree);
		gtk_entry_set_text (GTK_ENTRY (m_search_entry), "");
	}

	// callbacks
	static void toggle_packages_view_cb  (GtkToggleButton *button,
	                                      PackageSelector *pThis)
	{
		IMPL
		if (gtk_toggle_button_get_active (button))
			pThis->setTreeView();
		else
			pThis->setPlainView();
	}

	static ZyppSelectablePtr selectedPackage (GtkTreeView *tree_view,
	                                          GtkTreePath **list_path = 0,
	                                          GtkTreePath **tree_path = 0)
	{
		if (list_path) *list_path = 0;
		if (tree_path) *tree_path = 0;
		GtkTreePath *path = 0;
		GtkTreeViewColumn *column;
		gtk_tree_view_get_cursor (tree_view, &path, &column);
		if (path) {
			GtkTreeModelFilter *model_filter =
				GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (tree_view));
			GtkTreeModel *model = gtk_tree_model_filter_get_model (model_filter);
			GtkTreePath *actual_path =
				gtk_tree_model_filter_convert_path_to_child_path (model_filter, path);

			ZyppSelectablePtr selectable = 0;
			GtkTreeIter iter;
			gtk_tree_model_get_iter (model, &iter, actual_path);
			gtk_tree_model_get (model, &iter, 1, &selectable, -1);

			if (list_path && tree_path) {
				if (GTK_IS_LIST_STORE (model)) {
					gtk_tree_model_get (model, &iter, 4, tree_path, -1);
					*tree_path = gtk_tree_path_copy (*tree_path);
					*list_path = actual_path;
				}
				else { //if (GTK_IS_TREE_STORE (model))
					gtk_tree_model_get (model, &iter, 4, list_path, -1);
					*list_path = gtk_tree_path_copy (*list_path);
					*tree_path = actual_path;
				}
			}
			else
				gtk_tree_path_free (actual_path);
			gtk_tree_path_free (path);
			return selectable;
		}
		return NULL;
	}

	void setSelectedPackage (GtkTreeView *view, bool available, bool installed)
	{
		IMPL
		GtkTreeIter iter;
		GtkTreePath *list_path, *tree_path;
		ZyppSelectablePtr sel = selectedPackage (view, &list_path, &tree_path);

		if (!sel || !mark_selectable (sel, installed))
			goto set_selected_cleanups;

		gtk_tree_model_get_iter (GTK_TREE_MODEL (m_packages_list), &iter, list_path);
		gtk_list_store_set (m_packages_list, &iter, 2, available, 3, installed, -1);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (m_packages_tree), &iter, tree_path);
		gtk_tree_store_set (m_packages_tree, &iter, 2, available, 3, installed, -1);

		set_selected_cleanups:
			if (list_path) gtk_tree_path_free (list_path);
			if (tree_path) gtk_tree_path_free (tree_path);
	}

	static void install_button_clicked_cb (GtkButton *button, PackageSelector *pThis)
	{
		IMPL
		pThis->setSelectedPackage (GTK_TREE_VIEW (pThis->m_available_view), false, true);
	}

	static void remove_button_clicked_cb (GtkButton *button, PackageSelector *pThis)
	{
		IMPL
		pThis->setSelectedPackage (GTK_TREE_VIEW (pThis->m_installed_view), true, false);
	}

	static void package_clicked_cb (GtkTreeView *tree_view, PackageSelector *pThis)
	{
		IMPL
		ZyppSelectablePtr sel = selectedPackage (tree_view);
		pThis->m_information_widget->setPackage (sel);
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
		ygtk_wizard_set_header_icon (wizard, NULL,
			THEMEDIR "/icons/32x32/apps/yast-software.png");
		ygtk_wizard_set_next_button_label (wizard, "_Accept");
		ygtk_wizard_set_next_button_id (wizard, g_strdup ("accept"), g_free);
		ygtk_wizard_set_abort_button_label (wizard, "_Cancel");
		ygtk_wizard_set_abort_button_id (wizard, g_strdup ("cancel"), g_free);
		g_signal_connect (G_OBJECT (getWidget()), "action-triggered",
		                  G_CALLBACK (wizard_action_cb), this);

		m_pattern_selector = new PatternSelector();
		m_package_selector = new PackageSelector (opt.searchMode.value());

		if (opt.searchMode.value()) {
			set_packages_help();
			ygtk_wizard_set_child (wizard, m_package_selector->getWidget());

			// Patterns window
			m_patterns_widget = gtk_window_new (GTK_WINDOW_TOPLEVEL);
			GtkWidget *window = m_patterns_widget;
			gtk_window_set_transient_for
				(GTK_WINDOW (window), GTK_WINDOW (YGUI::ui()->currentGtkDialog()));
			gtk_window_set_title (GTK_WINDOW (window), "Patterns Selector");
			gtk_window_set_default_size (GTK_WINDOW (window), -1, 400);

			GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
			GtkWidget *buttons_box = gtk_hbutton_box_new();
			GtkWidget *ok_button = gtk_button_new_from_stock (GTK_STOCK_OK);
			gtk_box_pack_end (GTK_BOX (buttons_box), ok_button, FALSE, FALSE, 0);
			g_signal_connect (G_OBJECT (ok_button), "clicked",
			                  G_CALLBACK (patterns_apply_cb), this);

			gtk_box_pack_start (GTK_BOX (vbox), m_pattern_selector->getWidget(),
			                    TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (vbox), buttons_box, FALSE, FALSE, 8);

			gtk_container_add (GTK_CONTAINER (window), vbox);
			gtk_container_set_border_width (GTK_CONTAINER (window), 6);
			gtk_widget_show_all (vbox);

			g_signal_connect (G_OBJECT (m_package_selector->m_patterns_button),
			                  "clicked", G_CALLBACK (show_patterns_cb), this);
			g_signal_connect (G_OBJECT (window), "delete_event",
			                  G_CALLBACK (close_patterns_window_cb), this);
		}
		else {
			ygtk_wizard_set_header_text (wizard, "Patterns Selector");
			ygtk_wizard_set_help_text (wizard,
				"Patterns are bundles of software that you may install by pressing them"
				"and then clicking Accept.<br>"
				"If you are an experienced user, you may press Customize to choose"
				"from individual packages."
			);
			ygtk_wizard_set_back_button_label (wizard, "_Customize...");
			ygtk_wizard_set_back_button_id (wizard, g_strdup ("customize"), g_free);

			// start by displaying a pattern selector that will then toggle to
			// the package selector if the user wants to (on Customize...)
			GtkWidget *buttons_box;
			m_patterns_widget = gtk_vbox_new (FALSE, 0);
			buttons_box = gtk_hbutton_box_new();

			gtk_box_pack_start (GTK_BOX (m_patterns_widget), m_pattern_selector->getWidget(),
			                    TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (m_patterns_widget), buttons_box,
			                    FALSE, FALSE, 8);
			gtk_widget_show_all (m_patterns_widget);

			ygtk_wizard_set_child (wizard, m_patterns_widget);
		}
	}

	virtual ~YGPackageSelector()
	{
		gtk_widget_destroy (m_patterns_widget);
		g_object_unref (G_OBJECT (m_patterns_widget));
		delete m_package_selector;
		delete m_pattern_selector;
	}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_KEYBOARD_FOCUS

protected:
	void set_packages_help()
	{
		YGtkWizard *wizard = YGTK_WIZARD (getWidget());
		ygtk_wizard_set_header_text (wizard, "Package Selector");
		ygtk_wizard_set_help_text (wizard,
			"Two pools are presented; one with the available software, the other"
			"with the installed one. To install software you choose a package"
			"from the install pool and press Install. Similar method for removal"
			"of software. When you are done press the Accept button.<br>"
			"Information on a given package is displayed on the Package Information"
			"expander at the bottom which may be enlarged.<br>"
			"A categories view of the software is possible, as well as searching"
			"for a given package."
		);
		ygtk_wizard_set_back_button_label (wizard, "");
	}

	static void show_patterns_cb (GtkButton *button, YGPackageSelector *pThis)
	{
		IMPL
		gtk_widget_show (pThis->m_patterns_widget);
	}

	static gboolean close_patterns_window_cb (GtkWidget *widget, GdkEvent *event,
	                                          YGPackageSelector *pThis)
	{
		gtk_widget_hide (widget);
		return TRUE;
	}

	static void patterns_apply_cb (GtkButton *button, YGPackageSelector *pThis)
	{
		IMPL
		if (button)
			gtk_widget_hide (pThis->m_patterns_widget);

		// TODO: currently patterns should be being installed, but when a pattern
		// is selected, its packages should be displayed in the packages list
#if 0
		set <string> packages;
		// to remove
		packages = pThis->m_pattern_selector->get_packages_to (false);
		for (set <string>::iterator it = packages.begin(); it != packages.end(); it++)
			pThis->markPackage (*it, false);
		// to install
		packages = pThis->m_pattern_selector->get_packages_to (true);
		for (set <string>::iterator it = packages.begin(); it != packages.end(); it++)
			pThis->markPackage (*it, true);
#endif
	}

	bool confirmChanges()
	{
		IMPL
		GtkWidget *dialog = gtk_dialog_new_with_buttons ("Changes Summary",
			GTK_WINDOW (YGUI::ui()->currentGtkDialog()), GTK_DIALOG_MODAL,
			"C_onfirm", 1, GTK_STOCK_CANCEL, 0, NULL);

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

			// remove view
			column = gtk_tree_view_column_new_with_attributes ("Remove packages",
				gtk_cell_renderer_text_new(), "text", 0, NULL);
			gtk_tree_view_append_column (GTK_TREE_VIEW (remove_view), column);

			gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (remove_view), FALSE);
			gtk_tree_view_set_model (GTK_TREE_VIEW (remove_view),
			                         GTK_TREE_MODEL (remove_store));

			// construct model
			GtkTreeModel *packages_model = GTK_TREE_MODEL (
			                                   m_package_selector->m_packages_list);
			GtkTreeIter iter, packages_iter;
			if (gtk_tree_model_get_iter_first (packages_model, &packages_iter))
				do {
					ZyppSelectablePtr selectable = 0;
					gtk_tree_model_get (packages_model, &packages_iter, 1, &selectable, -1);

					if (selectable) {
						GtkTreeStore *store;
						if (selectable->toInstall())
							store = install_store;
						else if (selectable->toDelete())
							store = remove_store;
						else
							continue;

						gtk_tree_store_append (store, &iter, NULL);
						gtk_tree_store_set (store, &iter,
							                  0, selectable->name().c_str(), -1);
// TODO: put dependencies here! (for now, it goes as a list)




					}
				} while (gtk_tree_model_iter_next (packages_model, &packages_iter));
			}

		gtk_window_set_default_size (GTK_WINDOW (dialog), 300, 400);
		gtk_widget_show_all (dialog);

		bool confirmed = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		return confirmed;
	}
#if 0
	bool licensesAccepted()
	{
		y2milestone ("Showing all pending license agreements");

		GtkTreeModel *model = m_package_selector->m_packages_list;
		GtkTreeIter iter;
		gtk_tree_model_get_iter_first (model, &iter);

		do {
			ZyppSelectablePtr selectable = 0;
			gtk_tree_model_get (model, &iter, 3, &selectable, -1);

			if (selectable && selectable->toInstall()) {
				ZyppObject object = sel->candidateObj()
				string license = object->licenseToConfirm();
				if (!license.empty()) {
					if (!confirmLicense (license))
						return false;
				}
				selectable->setLicenceConfirmed (true);
			}
		} while (gtk_tree_model_iter_next (model, &iter));

		return true;
	}
#endif
	bool solveProblems()
	{
		IMPL
		zypp::Resolver_Ptr resolver = zypp::getZYpp()->resolver();
		zypp::ResolverProblemList problems = resolver->problems();
		if (problems.empty())
			return true;

		GtkWidget *dialog = gtk_dialog_new_with_buttons ("Resolve Problems",
			GTK_WINDOW (YGUI::ui()->currentGtkDialog()), GTK_DIALOG_MODAL,
			"C_onfirm", 1, GTK_STOCK_CANCEL, 0, NULL);

		GtkWidget *problems_view;
		problems_view  = gtk_tree_view_new();

		GtkWidget *problems_window;
		problems_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (problems_window),
		                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type
			(GTK_SCROLLED_WINDOW (problems_window), GTK_SHADOW_IN);
		gtk_container_add (GTK_CONTAINER (problems_window), problems_view);

		GtkBox *vbox = GTK_BOX (GTK_DIALOG(dialog)->vbox);
		gtk_box_pack_start (vbox, problems_window,  FALSE, FALSE, 0);

		// create model
		// the vairous columns are: radio button active (boolean), problem
		// or solution description (string), is radio button (boolean: always
		// true), show radio button (boolean; false for problems)
		GtkListStore *problems_store = gtk_list_store_new (4,
			G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
		// install view
		GtkTreeViewColumn *column;
		GtkCellRenderer *radio_renderer = gtk_cell_renderer_toggle_new();
		column = gtk_tree_view_column_new_with_attributes ("",
			radio_renderer, "active", 0, "radio", 2, NULL);
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

		// construct model
		GtkTreeIter iter;
		map <GtkTreePath *, zypp::ProblemSolution *> users_actions;
		for (zypp::ResolverProblemList::iterator it = problems.begin();
		     it != problems.end(); it++) {
			zypp::ProblemSolutionList solutions = (*it)->solutions();

			gtk_list_store_append (problems_store, &iter);
			gtk_list_store_set (problems_store, &iter, 0, FALSE,
				1, ((*it)->description() + "\n" + (*it)->details()).c_str(),
				2, TRUE, 3, FALSE, -1);

			for (zypp::ProblemSolutionList::iterator jt = solutions.begin();
			     jt != solutions.end(); jt++) {
				gtk_list_store_append (problems_store, &iter);
				gtk_list_store_set (problems_store, &iter, 0, FALSE,
					1, ((*jt)->description() + "\n" + (*jt)->details()).c_str(),
					2, TRUE, 3, TRUE, -1);
				users_actions [gtk_tree_model_get_path (GTK_TREE_MODEL (
					problems_store), &iter)] = get_pointer (*jt);
			}
		}

		gtk_window_set_default_size (GTK_WINDOW (dialog), 300, 400);
		gtk_widget_show_all (dialog);

		bool confirmed = gtk_dialog_run (GTK_DIALOG (dialog));

		if (confirmed) {
			// apply user solutions
			zypp::ProblemSolutionList userChoices;

			for (map <GtkTreePath *, zypp::ProblemSolution *>::iterator it
			     = users_actions.begin(); it != users_actions.end(); it++) {
				gtk_tree_model_get_iter (GTK_TREE_MODEL (problems_store),
				                         &iter, it->first);
				gboolean state = FALSE;
				gtk_tree_model_get (GTK_TREE_MODEL (problems_store), &iter,
				                    0, &state, -1);
				if (state) {
					userChoices.push_back (it->second);
					resolver->applySolutions (userChoices);
				}
				gtk_tree_path_free (it->first);
			}
		}

		gtk_widget_destroy (dialog);
		if (confirmed)
			// repeate the all process just in case
			return solveProblems();
		return false;
	}

	static void wizard_action_cb (YGtkWizard *wizard, gpointer id,
	                              gint id_type, YGPackageSelector *pThis)
	{
		IMPL
		const gchar *action = (gchar *) id;
		if (!strcmp (action, "customize")) {
			YGtkWizard *wizard = YGTK_WIZARD (pThis->getWidget());

			ygtk_wizard_unset_child (wizard);
			patterns_apply_cb (NULL, pThis);

			ygtk_wizard_set_child (wizard, pThis->m_package_selector->getWidget());
			pThis->set_packages_help();
		}
		else if (!strcmp (action, "accept")) {
			if (zyppPool().diffState <zypp::Package> () ||
			    zyppPool().diffState <zypp::Pattern> () ||
			    zyppPool().diffState <zypp::Selection>()) {
				// in case of changes, check problems, ask for confirmation and JFDI.
				if (pThis->confirmChanges() && pThis->solveProblems()) {
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
