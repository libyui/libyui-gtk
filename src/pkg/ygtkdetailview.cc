/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkZyppView, Zypp GtkTreeView implementation */
// check the header file for information about this widget

/*
  Textdomain "yast2-gtk"
 */
#define YUILogComponent "gtk"
#include "config.h"
#include "ygtkdetailview.h"
#include "ygtkpackageview.h"
#include "ygtkrichtext.h"
#include "YGi18n.h"
#include "YGUtils.h"
#include "YGDialog.h"
#include <stdlib.h>
#include <string.h>
#include <list>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

//** Utilities

#define BROWSER_BIN "/usr/bin/firefox"
#define GNOME_OPEN_BIN "/usr/bin/gnome-open"

static const char *onlyInstalledMsg =
	_("<i>Information only available for installed packages.</i>");

//** Detail & control

struct Versions {
	GtkWidget *m_box, *m_versions_box, *m_button, *m_undo_button;
	Ypp::PkgList m_packages;

	GtkWidget *getWidget() { return m_box; }

	Versions()
	{
		m_versions_box = gtk_vbox_new (FALSE, 2);
		m_button = gtk_button_new_with_label ("");
		g_signal_connect (G_OBJECT (m_button), "clicked", G_CALLBACK (button_clicked_cb), this);
		GtkSettings *settings = gtk_settings_get_default();
		gboolean show_button_images;
		g_object_get (settings, "gtk-button-images", &show_button_images, NULL);
		if (show_button_images) {
			m_undo_button = gtk_button_new_with_label ("");
			gtk_button_set_image (GTK_BUTTON (m_undo_button),
				gtk_image_new_from_stock (GTK_STOCK_UNDO, GTK_ICON_SIZE_BUTTON));
			gtk_widget_set_tooltip_text (m_undo_button, _("Undo"));
		}
		else
			m_undo_button = gtk_button_new_with_label (_("Undo"));
		g_signal_connect (G_OBJECT (m_undo_button), "clicked", G_CALLBACK (undo_clicked_cb), this);
		GtkWidget *button_box = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (button_box), gtk_label_new(""), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (button_box), m_button, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (button_box), m_undo_button, FALSE, TRUE, 0);

		m_box = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (m_box), m_versions_box, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_box), button_box, FALSE, TRUE, 0);
		g_object_set_data (G_OBJECT (getWidget()), "class", this);
	}

	void setPackages (Ypp::PkgList packages)
	{
		m_packages = packages;

		GList *children = gtk_container_get_children (GTK_CONTAINER (m_versions_box));
		for (GList *i = children; i; i = i->next)
			gtk_container_remove (GTK_CONTAINER (m_versions_box), (GtkWidget *) i->data);
		g_list_free (children);

		if (packages.size() == 0) {
			gtk_widget_hide (m_box);
			return;
		}
		gtk_widget_show (m_box);
		if (packages.size() == 1) {
			Ypp::Package *package = packages.get (0);

			int children = 0;
			GtkWidget *radio = 0;
			const Ypp::Package::Version *version;
			if ((version = package->getInstalledVersion())) {
				std::string _version = YGUtils::truncate (version->number, 20, 0);

				bool italic = package->toRemove();
				char *text = g_strdup_printf ("%s%s <small>(%s)</small>\n%s%s",
					italic ? "<i>" : "",
					_version.c_str(), version->arch.c_str(), _("Installed"),
					italic ? "</i>" : "");
				radio = gtk_radio_button_new_with_label (NULL, text);
				gtk_label_set_use_markup (GTK_LABEL (GTK_BIN (radio)->child), TRUE);
				if (version->number.size() > 20) {
					char *text = g_strdup_printf ("%s <small>(%s)</small>\n%s",
						version->number.c_str(), version->arch.c_str(), _("Installed"));
					gtk_widget_set_tooltip_markup (radio, text);
					g_free (text);
				}
				gtk_box_pack_start (GTK_BOX (m_versions_box), radio, FALSE, TRUE, 0);
				g_free (text);
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);
				g_object_set_data (G_OBJECT (radio), "version", (void *) version);
				g_signal_connect (G_OBJECT (radio), "toggled",
					              G_CALLBACK (version_toggled_cb), this);
				children++;
			}
			bool activeSet = package->toRemove();
			const Ypp::Package::Version *toInstall = 0;
			if (!package->toInstall (&toInstall))
				toInstall = 0;
			const Ypp::Repository *favoriteRepo = Ypp::get()->favoriteRepository();
			for (int i = 0; (version = package->getAvailableVersion (i)); i++) {
				std::string _version = YGUtils::truncate (version->number, 20, 0);
				std::string repo;
				if (version->repo)
					repo = YGUtils::truncate (version->repo->name, 22, 0);

				bool italic = toInstall == version;
				char *text = g_strdup_printf ("%s%s <small>(%s)</small>\n%s%s",
					italic ? "<i>" : "",
					_version.c_str(), version->arch.c_str(), repo.c_str(),
					italic ? "</i>" : "");
				radio = gtk_radio_button_new_with_label_from_widget (
					GTK_RADIO_BUTTON (radio), text);
				gtk_label_set_use_markup (GTK_LABEL (GTK_BIN (radio)->child), TRUE);
				g_free (text);
				if (version->number.size() > 20 ||
					(version->repo && version->repo->name.size() > 22)) {
					char *text = g_strdup_printf ("%s <small>(%s)</small>\n%s",
						version->number.c_str(), version->arch.c_str(),
						version->repo ? version->repo->name.c_str() : "");
					gtk_widget_set_tooltip_markup (radio, text);
					g_free (text);
				}
				gtk_box_pack_start (GTK_BOX (m_versions_box), radio, FALSE, TRUE, 0);
				if (!activeSet) {
					if (toInstall == version) {
						gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);
						activeSet = true;
					}
					else if (i == 0 && (!package->isInstalled() || version->cmp > 0))
						gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);
					else if (version->repo == favoriteRepo) {
						gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);
						favoriteRepo = 0;  // select only the 1st hit
					}
				}
				g_object_set_data (G_OBJECT (radio), "version", (void *) version);
				g_signal_connect (G_OBJECT (radio), "toggled",
					              G_CALLBACK (version_toggled_cb), this);
				if ((children % 2) == 1)
					g_signal_connect (G_OBJECT (radio), "expose-event", G_CALLBACK (draw_gray_cb), this);
				children++;
			}
			gtk_widget_show_all (m_versions_box);
		}

		// is locked
		if (packages.isLocked() || packages.isUnlocked())
			gtk_widget_set_sensitive (m_button, !packages.isLocked());
		else
			gtk_widget_set_sensitive (m_button, TRUE);
		syncButton();
	}

private:
	static gboolean draw_gray_cb (GtkWidget *widget, GdkEventExpose *event, Versions *pThis)
	{
		GtkAllocation *alloc = &widget->allocation;
		int x = alloc->x, y = alloc->y, w = alloc->width, h = alloc->height;

		cairo_t *cr = gdk_cairo_create (widget->window);
		cairo_rectangle (cr, x, y, w, h);
		cairo_set_source_rgb (cr, 245/255.0, 245/255.0, 245/255.0);
		cairo_fill (cr);
		cairo_destroy (cr);
		return FALSE;
	}

	const Ypp::Package::Version *getVersion()
	{
		GtkWidget *radio = 0;
		GList *children = gtk_container_get_children (GTK_CONTAINER (m_versions_box));
		for (GList *i = children; i; i = i->next)
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (i->data))) {
				radio = (GtkWidget *) i->data;
				break;
			}
		g_list_free (children);
		if (radio)
			return (Ypp::Package::Version *) g_object_get_data (G_OBJECT (radio), "version");
		return NULL;
	}

	void syncButton()
	{
		const char *label = 0, *stock = 0;
		bool modified = false;
		if (m_packages.size() == 1) {
			Ypp::Package *package = m_packages.get (0);
			const Ypp::Package::Version *version = getVersion();
			const Ypp::Package::Version *installed = package->getInstalledVersion();
			if (installed == version) {
				label = _("Remove");
				stock = GTK_STOCK_DELETE;
				if (package->toRemove())
					modified = true;
			}
			else {
				if (installed) {
					if (version->cmp > 0) {
						label = _("Upgrade");
						stock = GTK_STOCK_GO_UP;
					}
					else if (version->cmp < 0) {
						label = _("Downgrade");
						stock = GTK_STOCK_GO_DOWN;
					}
					else {
						label = _("Re-install");
						stock = GTK_STOCK_REFRESH;
					}
				}
				else {
					label = _("Install");
					stock = GTK_STOCK_SAVE;
				}

				const Ypp::Package::Version *toInstall = 0;
				if (!package->toInstall (&toInstall))
					toInstall = 0;
				if (toInstall == version)
					modified = true;
			}
		}
		else {
			if (m_packages.toModify())
				modified = true;
			if (m_packages.hasUpgrade()) {
				label = _("Upgrade");
				stock = GTK_STOCK_GO_UP;
			}
			else if (m_packages.isInstalled()) {
				label = _("Remove");
				stock = GTK_STOCK_DELETE;
			}
			else if (m_packages.isNotInstalled()) {
				label = _("Install");
				stock = GTK_STOCK_SAVE;
			}
			else if (m_packages.toModify()) {
				label = _("Undo");
				stock = GTK_STOCK_UNDO;
				modified = false;
			}
		}
		if (label) {
			gtk_button_set_label (GTK_BUTTON (m_button), label);
			GtkWidget *image = gtk_image_new_from_stock (
				stock, GTK_ICON_SIZE_BUTTON);
			gtk_button_set_image (GTK_BUTTON (m_button), image);
			gtk_widget_show (m_button);
		}
		else
			gtk_widget_hide (m_button);
		gtk_widget_set_sensitive (m_button, !modified);
		modified ? gtk_widget_show (m_undo_button) : gtk_widget_hide (m_undo_button);
	}

	static void version_toggled_cb (GtkToggleButton *radio, Versions *pThis)
	{ if (gtk_toggle_button_get_active (radio)) pThis->syncButton(); }

	static void button_clicked_cb (GtkButton *button, Versions *pThis)
	{
		if (pThis->m_packages.size() == 1) {
			Ypp::Package *package = pThis->m_packages.get (0);
			const Ypp::Package::Version *version = pThis->getVersion();
			const Ypp::Package::Version *installed = package->getInstalledVersion();
			if (installed == version)
				pThis->m_packages.remove();
			else
				package->install (version);
		}
		else {
			if (pThis->m_packages.hasUpgrade())
				pThis->m_packages.install();
			else if (pThis->m_packages.isInstalled())
				pThis->m_packages.remove();
			else if (pThis->m_packages.isNotInstalled())
				pThis->m_packages.install();
			else if (pThis->m_packages.toModify())
				pThis->m_packages.undo();
		}
	}

	static void undo_clicked_cb (GtkButton *button, Versions *pThis)
	{
		pThis->m_packages.undo();
	}
};

class YGtkDetailView::Impl : public Ypp::PkgList::Listener
{
GtkWidget *m_scroll, *m_icon, *m_icon_frame, *m_name, *m_description,
	*m_filelist, *m_changelog, *m_authors, *m_support, *m_dependencies,
	*m_contents, *m_details, *m_versions_exp;
GtkWidget *m_link_popup;
std::string m_link_str;
Versions *m_versions;
bool m_verMode;
Ypp::PkgList m_packages;  // we keep a copy to test against modified...
bool m_onlineUpdate;

public:
	Impl (GtkWidget *scroll, bool onlineUpdate)
	: m_verMode (false), m_onlineUpdate (onlineUpdate)
	{
		m_scroll = scroll;
		m_versions = new Versions();
		GtkWidget *hbox = gtk_hbox_new (FALSE, 2);
		setupScrollAsWhite (scroll, hbox);

		GtkWidget *side_box = gtk_vbox_new (FALSE, 0);
		m_versions_exp = appendExpander (side_box, _("Versions"), m_versions->getWidget(), versions_cb);
		m_details = ygtk_rich_text_new();
		m_details = appendExpander (side_box, _("Details"), m_details, details_cb);
		gtk_widget_set_size_request (m_details, 140, -1);
		gtk_expander_set_expanded (GTK_EXPANDER (m_details), TRUE);

		GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
		m_name = ygtk_rich_text_new();
		GtkWidget *header_box = gtk_hbox_new (FALSE, 2);
		gtk_box_pack_start (GTK_BOX (header_box), m_name, TRUE, TRUE, 0);
		m_icon_frame = m_icon = gtk_image_new();
		gtk_box_pack_start (GTK_BOX (header_box), m_icon, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), header_box, FALSE, TRUE, 0);

		m_description = ygtk_rich_text_new();
		g_signal_connect (G_OBJECT (m_description), "link-clicked",
		                  G_CALLBACK (link_clicked_cb), this);
		gtk_box_pack_start (GTK_BOX (vbox), m_description, FALSE, TRUE, 0);

		m_filelist = m_changelog = m_authors = m_support = m_dependencies = m_contents = 0;
		if (!onlineUpdate) {
			m_filelist = ygtk_rich_text_new();
			g_signal_connect (G_OBJECT (m_filelist), "link-clicked",
					          G_CALLBACK (dirname_pressed_cb), this);
			m_support = ygtk_rich_text_new();
			m_dependencies = gtk_hbox_new (TRUE, 0);
			gtk_box_pack_start (GTK_BOX (m_dependencies), ygtk_rich_text_new(), TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (m_dependencies), ygtk_rich_text_new(), TRUE, TRUE, 0);

			m_filelist = appendExpander (vbox, _("File List"), m_filelist, filelist_cb);
			m_changelog = appendExpander (vbox, _("Changelog"), ygtk_rich_text_new(), changelog_cb);
			m_authors = appendExpander (vbox, _("Authors"), ygtk_rich_text_new(), authors_cb);
			m_dependencies = appendExpander (vbox, _("Dependencies"), m_dependencies, dependencies_cb);
			m_support = appendExpander (vbox, "", m_support, NULL);
		}
		else {
			YGtkPackageView *contents;
			m_contents = GTK_WIDGET (contents = ygtk_package_view_new (TRUE));
			gtk_widget_set_size_request (m_contents, -1, 150);
			m_contents = appendExpander (vbox, _("Applies to"), m_contents, contents_cb);
		}
		gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), side_box, FALSE, TRUE, 0);
		g_signal_connect (G_OBJECT (scroll), "hierarchy-changed",
		                  G_CALLBACK (hierarchy_changed_cb), this);
		g_signal_connect (G_OBJECT (scroll), "realize",
		                  G_CALLBACK (realize_cb), this);

		// PkgList::addListeners() isn't currently working for ordinary PkgList
		// other than getPackages(), thus this hack.
		if (!m_onlineUpdate)
			Ypp::get()->getPackages (Ypp::Package::PACKAGE_TYPE).addListener (this);
		gtk_widget_show_all (hbox);
	}

	~Impl()
	{
		delete m_versions;
		if (!m_onlineUpdate)
			Ypp::get()->getPackages (Ypp::Package::PACKAGE_TYPE).removeListener (this);
	}

	void refreshPackages (Ypp::PkgList packages)
	{
		std::string name;
		name.reserve (1024);
		if (packages.size() > 0) {
			name = "<p bgcolor=\"";
			name += packages.toModify() ? "#f4f4b7" : "#ededed";
			name += "\">";
			if (packages.size() == 1) {
				Ypp::Package *package = packages.get (0);
				name += "<b>" + package->name() + "</b> - " + package->summary();
			}
			else {
				name += "<b>";
				name += _("Several selected");
				name += "</b>";
			}
			name += "</p>";
		}
		ygtk_rich_text_set_text (YGTK_RICH_TEXT (m_name), name.c_str());
		ygtk_expander_update (m_versions_exp);
	}

	void setPackages (Ypp::PkgList packages)
	{
		m_packages = packages;

		gtk_widget_hide (m_icon_frame);
		Ypp::Package *package = 0;
		if (packages.size() == 1) {
			package = packages.get (0);
			std::string description (package->description (HTML_MARKUP));
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (m_description), description.c_str());
			if (m_support) {
				std::string label ("<b>" + std::string (_("Supportability:")) + "</b> ");
				label += package->support();
				gtk_expander_set_label (GTK_EXPANDER (m_support), label.c_str());
				ygtk_rich_text_set_text (YGTK_RICH_TEXT (GTK_BIN (m_support)->child),
					package->supportText (HTML_MARKUP).c_str());
				gtk_widget_show (m_support);
			}
			gtk_image_clear (GTK_IMAGE (m_icon));
			GtkIconTheme *icons = gtk_icon_theme_get_default();
			GdkPixbuf *pixbuf = gtk_icon_theme_load_icon (icons,
				package->name().c_str(), 32, GtkIconLookupFlags (0), NULL);
			if (pixbuf) {
				gtk_image_set_from_pixbuf (GTK_IMAGE (m_icon), pixbuf);
				g_object_unref (G_OBJECT (pixbuf));
				gtk_widget_show (m_icon_frame);
			}
		}
		else {
			std::string description;
			if (packages.size() > 0) {
				description += "<ul>";
				for (int i = 0; packages.get (i); i++)
					description += "<li>" + packages.get (i)->name() + "</li>";
				description += "</ul>";
			}
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (m_description), description.c_str());
			if (m_support)
				gtk_widget_hide (m_support);
		}

		ygtk_expander_update (m_filelist);
		ygtk_expander_update (m_changelog);
		ygtk_expander_update (m_authors);
		ygtk_expander_update (m_dependencies);
		ygtk_expander_update (m_contents);
		ygtk_expander_update (m_details);

		refreshPackages (packages);
		scrollTop();
	}

	void setPackage (Ypp::Package *package)
	{
		Ypp::PkgList packages;
		if (package)
			packages.append (package);
		setPackages (packages);
	}

	void setVerticalMode (bool verMode)
	{
		if (verMode != m_verMode) {
			m_verMode = verMode;
		}
	}

private:
	virtual void entryChanged (const Ypp::PkgList list, int index, Ypp::Package *package)
	{
		if (m_packages.contains (package)) {
			m_packages.refreshProps();
			refreshPackages (m_packages);
		}
	}
	// won't happen:
	virtual void entryInserted (const Ypp::PkgList list, int index, Ypp::Package *package) {}
	virtual void entryDeleted  (const Ypp::PkgList list, int index, Ypp::Package *package) {}

	void scrollTop()
	{
		GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW (m_scroll);
		GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment (scroll);
		YGUtils::scrollWidget (vadj, true);
	}

	// utilities:
	static void setupScrollAsWhite (GtkWidget *scroll, GtkWidget *box)
	{
		struct inner {
			static gboolean expose_cb (GtkWidget *widget, GdkEventExpose *event)
			{
				cairo_t *cr = gdk_cairo_create (widget->window);
				GdkColor color = { 0, 255 << 8, 255 << 8, 255 << 8 };
				gdk_cairo_set_source_color (cr, &color);
				cairo_rectangle (cr, event->area.x, event->area.y,
						         event->area.width, event->area.height);
				cairo_fill (cr);
				cairo_destroy (cr);
				return FALSE;
			}
		};

		g_signal_connect (G_OBJECT (box), "expose-event",
			              G_CALLBACK (inner::expose_cb), NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroll), box);
	}

	typedef bool (*UpdateCb) (GtkWidget *widget, Ypp::PkgList list);
	GtkWidget *appendExpander (GtkWidget *box, const char *label, GtkWidget *child, UpdateCb update_cb)
	{
		std::string str = std::string ("<b>") + label + "</b>";
		GtkWidget *expander = gtk_expander_new (str.c_str());
		gtk_expander_set_use_markup (GTK_EXPANDER (expander), TRUE);
		gtk_container_add (GTK_CONTAINER (expander), child);
		gtk_box_pack_start (GTK_BOX (box), expander, FALSE, TRUE, 0);
		if (update_cb) {
			g_object_set_data (G_OBJECT (expander), "update-cb", (gpointer) update_cb);
			g_signal_connect_after (G_OBJECT (expander), "notify::expanded",
				                    G_CALLBACK (ygtk_expander_expanded_cb), this);
		}
		return expander;
	}

	// callbacks
	static void link_clicked_cb (YGtkRichText *text, const gchar *link, Impl *pThis)
	{
		if (!pThis->m_link_popup) {
			GtkWidget *menu = pThis->m_link_popup = gtk_menu_new();
			gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (text), NULL);

			GtkWidget *item;
			if (g_file_test (BROWSER_BIN, G_FILE_TEST_IS_EXECUTABLE)) {
				std::string label;
				if (getuid() == 0) {
					const char *username = getenv ("USERNAME");
					if (!username || !(*username))
						username = "root";
					label = _("_Open (as ");
					label += username;
					label += ")";
				}
				else
					label = _("_Open");
				item = gtk_image_menu_item_new_with_mnemonic (label.c_str());
				gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
					gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
				gtk_widget_show (item);
				g_signal_connect (G_OBJECT (item), "activate",
					              G_CALLBACK (open_link_cb), pThis);
				gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			}
			item = gtk_image_menu_item_new_from_stock (GTK_STOCK_COPY, NULL);
			gtk_widget_show (item);
			g_signal_connect (G_OBJECT (item), "activate",
				              G_CALLBACK (copy_link_cb), pThis);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		}
		gtk_menu_popup (GTK_MENU (pThis->m_link_popup), NULL, NULL, NULL, NULL, 
		                0, gtk_get_current_event_time());
		pThis->m_link_str = link;
	}

	static void open_link_cb (GtkMenuItem *item, Impl *pThis)
	{
		const std::string &link = pThis->m_link_str;
		std::string command;
		command.reserve (256);

		const char *username = 0;
		if (getuid() == 0) {
			username = getenv ("USERNAME");
			if (username && !(*username))
				username = 0;
		}

		if (username) {
			command += "gnomesu -u ";
			command += username;
			command += " -c \"" BROWSER_BIN " --new-window ";
			command += link;
			command += "\"";
		}
		else {
			command += BROWSER_BIN " --new-window ";
			command += link;
		}
		command += " &";
		system (command.c_str());
	}

	static void copy_link_cb (GtkMenuItem *item, Impl *pThis)
	{
		const std::string &link = pThis->m_link_str;
		GtkClipboard *clipboard =
			gtk_widget_get_clipboard (pThis->m_description, GDK_SELECTION_CLIPBOARD);
		gtk_clipboard_set_text (clipboard, link.c_str(), -1);
	}

	static void dirname_pressed_cb (GtkWidget *text, const gchar *link, Impl *pThis)
	{
		gchar *cmd = g_strdup_printf (GNOME_OPEN_BIN " %s &", link);
		system (cmd);
		g_free (cmd);
	}

	static void hierarchy_changed_cb (GtkWidget *widget, GtkWidget *old, Impl *pThis)
	{
		GtkWidget *parent = gtk_widget_get_parent (widget);
		if (parent) {
			bool vertical = GTK_IS_HPANED (parent) || GTK_IS_HBOX (parent);
			pThis->setVerticalMode (vertical);
		}
	}

	static bool versions_cb (GtkWidget *widget, Ypp::PkgList list)
	{
		Versions *versions = (Versions *) g_object_get_data (G_OBJECT (widget), "class");
		versions->setPackages (list);
		return true;
	}

	static bool details_cb (GtkWidget *widget, Ypp::PkgList list)
	{
		if (list.size() != 1) return false;
		Ypp::Package *pkg = list.get (0);
		std::string text;
		std::string b ("<b>"), _b ("</b> "), br ("<br/>");
		text.reserve (2048);
		text += b + _("Size:") + _b + pkg->size();
		text += br + b + _("License:") + _b + pkg->license();
		std::string installed (pkg->installedDate()), candidate (pkg->candidateDate());
		if (!installed.empty())
			text += br + b + _("Installed at:") + _b + installed;
		if (!candidate.empty())
			text += br + b + _("Latest build:") + _b + candidate;
		ygtk_rich_text_set_text (YGTK_RICH_TEXT (widget), text.c_str());
		return true;
	}

	static bool filelist_cb (GtkWidget *widget, Ypp::PkgList list)
	{
		if (list.size() != 1) return false;
		std::string str (list.get(0)->filelist (HTML_MARKUP));
		if (str.empty())
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (widget), onlyInstalledMsg);
		else
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (widget), str.c_str());
		return true;
	}

	static bool changelog_cb (GtkWidget *widget, Ypp::PkgList list)
	{
		if (list.size() != 1) return false;
		std::string str (list.get(0)->changelog());
		if (str.empty())
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (widget), onlyInstalledMsg);
		else
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (widget), str.c_str());
		return true;
	}

	static bool authors_cb (GtkWidget *widget, Ypp::PkgList list)
	{
		if (list.size() != 1) return false;
		std::string str (list.get(0)->authors (HTML_MARKUP));
		if (str.empty())
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (widget),
				_("<i>Not specified.</i>"));
		else
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (widget), str.c_str());
		return true;
	}

	static bool dependencies_cb (GtkWidget *widget, Ypp::PkgList list)
	{
		if (list.size() != 1) return false;
		Ypp::Package *pkg = list.get (0);
		GList *children = gtk_container_get_children (GTK_CONTAINER (widget));
		int n = 0;
		for (GList *i = children; i; i = i->next, n++) {
			std::string str, label, text;
			switch (n) {
				case 0:
					label = _("Requires:");
					text = pkg->requires (HTML_MARKUP);
					break;
				case 1:
					label = _("Provides:");
					text = pkg->provides (HTML_MARKUP);
					break;
			}
			YGUtils::replace (text, "\n", 1, "<br>");
			str.reserve (1024);
			str = label;
			str += "<br><blockquote>";
			str += text;
			str += "</blockquote>";
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (i->data), str.c_str());
		}
		g_list_free (children);
		return true;
	}

	static bool contents_cb (GtkWidget *widget, Ypp::PkgList list)
	{
		if (list.size() != 1) return false;
		YGtkPackageView *contents = YGTK_PACKAGE_VIEW (widget);
		Ypp::QueryBase *query = new Ypp::QueryCollection (list.get (0));
		contents->setList (Ypp::PkgQuery (Ypp::Package::PACKAGE_TYPE, query), 0);
		return true;
	}

	// fix cursor keys support
	static void move_cursor_cb (GtkTextView *view, GtkMovementStep step, gint count,
	                            gboolean extend_selection, Impl *pThis)
	{
		GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW (pThis->m_scroll);
		GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment (scroll);
		int height = pThis->m_scroll->allocation.height;
		gdouble increment;
		switch (step)  {
			case GTK_MOVEMENT_DISPLAY_LINES:
				increment = height / 10.0;
				break;
			case GTK_MOVEMENT_PAGES:
				increment = height * 0.9;
				break;
			case GTK_MOVEMENT_DISPLAY_LINE_ENDS:
				increment = adj->upper - adj->lower;
				break;
			default:
				increment = 0.0;
				break;
		}

		gdouble value = adj->value + (count * increment);
		value = MIN (value, adj->upper - adj->page_size);
		value = MAX (value, adj->lower);
		if (value != adj->value)
			gtk_adjustment_set_value (adj, value);
	}

	static inline void fix_keybindings (Impl *pThis, GtkWidget *widget)
	{
		g_signal_connect (G_OBJECT (widget), "move-cursor",
		                  G_CALLBACK (move_cursor_cb), pThis);
	}

	static inline void fix_expander_keybindings (Impl *pThis, GtkWidget *widget)
	{
		if (widget)
			fix_keybindings (pThis, gtk_bin_get_child (GTK_BIN (widget)));
	}

	static void realize_cb (GtkWidget *widget, Impl *pThis)
	{
		fix_keybindings (pThis, pThis->m_description);
		fix_keybindings (pThis, pThis->m_filelist);
		fix_keybindings (pThis, pThis->m_changelog);
		fix_keybindings (pThis, pThis->m_authors);
		fix_keybindings (pThis, pThis->m_support);
		fix_keybindings (pThis, pThis->m_dependencies);
	}

	// make the widget request data when/if necessary; speeds up expander children
	void ygtk_expander_update (GtkWidget *expander)
	{
		if (!expander) return;
		if (m_packages.size() == 1)
			gtk_widget_show (expander);
		else {
			gtk_widget_hide (expander);
			return;
		}
		UpdateCb update_cb = (UpdateCb) g_object_get_data (G_OBJECT (expander), "update-cb");
		GtkWidget *child = gtk_bin_get_child (GTK_BIN (expander));
		if (gtk_expander_get_expanded (GTK_EXPANDER (expander)))
			update_cb (child, m_packages);
	}

	static void ygtk_expander_expanded_cb (GObject *object, GParamSpec *param_spec, Impl *pThis)
	{ pThis->ygtk_expander_update (GTK_WIDGET (object)); }
};

G_DEFINE_TYPE (YGtkDetailView, ygtk_detail_view, GTK_TYPE_SCROLLED_WINDOW)

static void ygtk_detail_view_init (YGtkDetailView *view)
{}

GtkWidget *ygtk_detail_view_new (gboolean onlineUpdate)
{
	YGtkDetailView *view = (YGtkDetailView *) g_object_new (YGTK_TYPE_DETAIL_VIEW, NULL);
	view->impl = new YGtkDetailView::Impl (GTK_WIDGET (view), onlineUpdate);
	return GTK_WIDGET (view);
}

static void ygtk_detail_view_finalize (GObject *object)
{
	G_OBJECT_CLASS (ygtk_detail_view_parent_class)->finalize (object);
	YGtkDetailView *view = YGTK_DETAIL_VIEW (object);
	delete view->impl;
	view->impl = NULL;
}

static void ygtk_detail_view_class_init (YGtkDetailViewClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = ygtk_detail_view_finalize;
}

void YGtkDetailView::setPackages (Ypp::PkgList packages)
{ impl->setPackages (packages); }

void YGtkDetailView::setPackage (Ypp::Package *package)
{ impl->setPackage (package); }

