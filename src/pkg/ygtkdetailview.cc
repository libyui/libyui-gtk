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

#define BROWSER_PATH "/usr/bin/firefox"
#define GNOME_OPEN_PATH "/usr/bin/gnome-open"
inline bool CAN_OPEN_URL()
{ return g_file_test (BROWSER_PATH, G_FILE_TEST_IS_EXECUTABLE); }
inline bool CAN_OPEN_DIRNAME()
{ return g_file_test (GNOME_OPEN_PATH, G_FILE_TEST_IS_EXECUTABLE); }

void run (const std::string &cmd, bool as_user)
{
	std::string prepend, append;
	if (as_user && getuid() == 0) {
		char *username = getenv ("USERNAME");
		if (username && *username && strcmp (username, "root") != 0) {
			prepend.reserve (64);
			prepend = "gnomesu -u ";
			prepend += username;
			prepend += " -c \"";
			append = "\"";
		}
	}
	system (((prepend + cmd + append) + " &").c_str());
}

void OPEN_URL (const char *uri)
{
	std::string command;
	command.reserve (256);
	command = BROWSER_PATH " --new-window ";
	command += uri;
	run (command, true);
}
void OPEN_DIRNAME (const char *uri)
{
	std::string command;
	command.reserve (256);
	command = GNOME_OPEN_PATH " ";
	command += uri;
	run (command, false);
}

static const char *onlyInstalledMsg =
	_("<i>Information only available for installed packages.</i>");

//** Detail & control

class YGtkDetailView::Impl
{
private:
	struct Versions : public Ypp::PkgList::Listener {
		GtkWidget *m_box, *m_versions_box, *m_button, *m_undo_button;
		Ypp::PkgList m_packages;  // we keep a copy to test against modified...
		bool m_onlineUpdate;

		GtkWidget *getWidget() { return m_box; }

		Versions (bool onlineUpdate) : m_onlineUpdate (onlineUpdate)
		{
			GtkWidget *label = gtk_label_new (_("Versions:"));
			YGUtils::setWidgetFont (label, PANGO_STYLE_NORMAL, PANGO_WEIGHT_BOLD, PANGO_SCALE_MEDIUM);
			gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

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
			gtk_box_pack_start (GTK_BOX (m_box), label, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (m_box), m_versions_box, TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (m_box), button_box, FALSE, TRUE, 0);

			// PkgList::addListeners() isn't currently working for ordinary PkgList
			// other than getPackages(), thus this hack.
			if (!m_onlineUpdate)
				Ypp::get()->getPackages (Ypp::Package::PACKAGE_TYPE).addListener (this);
		}

		~Versions()
		{
			if (!m_onlineUpdate)
				Ypp::get()->getPackages (Ypp::Package::PACKAGE_TYPE).removeListener (this);
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
			if (packages.locked() || packages.unlocked())
				gtk_widget_set_sensitive (m_button, !packages.locked());
			else
				gtk_widget_set_sensitive (m_button, TRUE);
			syncButton();
		}

	private:
		virtual void entryChanged (const Ypp::PkgList list, int index, Ypp::Package *package)
		{
			if (m_packages.contains (package))
				setPackages (m_packages);  // refresh !
		}
		// won't happen:
		virtual void entryInserted (const Ypp::PkgList list, int index, Ypp::Package *package) {}
		virtual void entryDeleted  (const Ypp::PkgList list, int index, Ypp::Package *package) {}

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
				if (m_packages.modified())
					modified = true;
				if (m_packages.upgradable()) {
					label = _("Upgrade");
					stock = GTK_STOCK_GO_UP;
				}
				else if (m_packages.installed()) {
					label = _("Remove");
					stock = GTK_STOCK_DELETE;
				}
				else if (m_packages.notInstalled()) {
					label = _("Install");
					stock = GTK_STOCK_SAVE;
				}
				else if (m_packages.modified()) {
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
				if (pThis->m_packages.upgradable())
					pThis->m_packages.install();
				else if (pThis->m_packages.installed())
					pThis->m_packages.remove();
				else if (pThis->m_packages.notInstalled())
					pThis->m_packages.install();
				else if (pThis->m_packages.modified())
					pThis->m_packages.undo();
			}
		}

		static void undo_clicked_cb (GtkButton *button, Versions *pThis)
		{
			pThis->m_packages.undo();
		}
	};

GtkWidget *m_scroll, *m_icon, *m_icon_frame, *m_description, *m_filelist, *m_changelog,
          *m_authors, *m_support, *m_requires, *m_provides, *m_contents;
Versions *m_versions;
bool m_verMode;

public:
	Impl (GtkWidget *scroll, bool onlineUpdate)
	: m_verMode (false)
	{
		m_scroll = scroll;
		m_versions = new Versions (onlineUpdate);
		GtkWidget *hbox = gtk_hbox_new (FALSE, 2);
		setupScrollAsWhite (scroll, hbox);

		GtkWidget *versions_box = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (versions_box),
			createIconWidget (&m_icon, &m_icon_frame), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (versions_box), m_versions->getWidget(), FALSE, TRUE, 0);

		GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
		m_description = ygtk_rich_text_new();
		g_signal_connect (G_OBJECT (m_description), "link-clicked",
		                  G_CALLBACK (link_pressed_cb), this);
		gtk_box_pack_start (GTK_BOX (vbox), m_description, FALSE, TRUE, 0);

		m_filelist = m_changelog = m_authors = m_support = m_requires = m_provides = m_contents = 0;
		if (!onlineUpdate) {
			m_filelist = ygtk_rich_text_new();
			if (CAN_OPEN_DIRNAME())
				g_signal_connect (G_OBJECT (m_filelist), "link-clicked",
						          G_CALLBACK (dirname_pressed_cb), this);
			m_changelog = ygtk_rich_text_new();
			m_authors = ygtk_rich_text_new();
			m_support = ygtk_rich_text_new();
			GtkWidget *dependencies_box = gtk_hbox_new (TRUE, 0);
			m_requires = ygtk_rich_text_new();
			m_provides = ygtk_rich_text_new();
			gtk_box_pack_start (GTK_BOX (dependencies_box), m_requires, TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (dependencies_box), m_provides, TRUE, TRUE, 0);

			appendExpander (vbox, _("File List"), m_filelist);
			appendExpander (vbox, _("Changelog"), m_changelog);
			appendExpander (vbox, _("Authors"), m_authors);
			appendExpander (vbox, _("Dependencies"), dependencies_box);
			appendExpander (vbox, "", m_support);
		}
		else {
			YGtkPackageView *contents;
			m_contents = GTK_WIDGET (contents = ygtk_package_view_new (TRUE));
			gtk_widget_set_size_request (m_contents, -1, 150);
			appendExpander (vbox, _("Applies to"), m_contents);
		}
		gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), versions_box, FALSE, TRUE, 0);
		gtk_widget_show_all (hbox);
		g_signal_connect (G_OBJECT (scroll), "hierarchy-changed",
		                  G_CALLBACK (hierarchy_changed_cb), this);
		g_signal_connect (G_OBJECT (scroll), "realize",
		                  G_CALLBACK (realize_cb), this);
	}

	~Impl()
	{ delete m_versions; }

	void setPackages (Ypp::PkgList packages)
	{
		gtk_widget_hide (m_icon_frame);
		Ypp::Package *package = 0;
		if (packages.size() == 1) {
			package = packages.get (0);
			std::string description = "<b>" + package->name() + "</b><br>";
			description += package->description (HTML_MARKUP);
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (m_description), description.c_str());
			if (m_support) {
				std::string label ("<b>" + std::string (_("Supportability:")) + "</b> ");
				label += package->support();
				gtk_expander_set_label (GTK_EXPANDER (m_support->parent), label.c_str());
				ygtk_rich_text_set_text (YGTK_RICH_TEXT (m_support),
					package->supportText (HTML_MARKUP).c_str());
				gtk_widget_show (m_support->parent);
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
			setPassiveUpdate (m_filelist, filelist_cb, package);
			setPassiveUpdate (m_changelog, changelog_cb, package);
			setPassiveUpdate (m_authors, authors_cb, package);
			setPassiveUpdate (m_requires, requires_cb, package);
			setPassiveUpdate (m_provides, provides_cb, package);
			setPassiveUpdate (m_contents, contents_cb, package);
		}
		else {
			std::string description;
			if (packages.size() > 0) {
				description = _("Selected:");
				description += "<ul>";
				for (int i = 0; packages.get (i); i++)
					description += "<li>" + packages.get (i)->name() + "</li>";
				description += "</ul>";
			}
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (m_description), description.c_str());
			if (m_support)
				gtk_widget_hide (m_support->parent);
			setPassiveUpdate (m_filelist, NULL, NULL);
			setPassiveUpdate (m_changelog, NULL, NULL);
			setPassiveUpdate (m_authors, NULL, NULL);
			setPassiveUpdate (m_requires, NULL, NULL);
			setPassiveUpdate (m_provides, NULL, NULL);
			setPassiveUpdate (m_contents, NULL, NULL);
		}

		m_versions->setPackages (packages);
		scrollTop();
	}

	void setPackage (Ypp::Package *package)
	{
		Ypp::PkgList packages;
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
	void scrollTop()
	{
		GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW (m_scroll);
		GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment (scroll);
		YGUtils::scrollWidget (vadj, true);
	}

	// utilities:
	static GtkWidget *createIconWidget (GtkWidget **icon_widget, GtkWidget **icon_frame)
	{
		*icon_widget = gtk_image_new();

		GtkWidget *box, *align, *frame;
		box = gtk_event_box_new();
		gtk_container_add (GTK_CONTAINER (box), *icon_widget);
		gtk_container_set_border_width (GTK_CONTAINER (box), 2);

		frame = gtk_frame_new (NULL);
		gtk_container_add (GTK_CONTAINER (frame), box);

		align = gtk_alignment_new (0, 0, 0, 0);
		gtk_container_add (GTK_CONTAINER (align), frame);
		gtk_container_set_border_width (GTK_CONTAINER (align), 6);
		*icon_frame = align;
		return align;
	}

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

	static GtkWidget *appendExpander (GtkWidget *box, const char *label, GtkWidget *child)
	{
		std::string str = std::string ("<b>") + label + "</b>";
		GtkWidget *expander = gtk_expander_new (str.c_str());
		gtk_expander_set_use_markup (GTK_EXPANDER (expander), TRUE);
		gtk_container_add (GTK_CONTAINER (expander), child);
		gtk_box_pack_start (GTK_BOX (box), expander, FALSE, TRUE, 0);
		return expander;
	}

	// callbacks
	static void link_pressed_cb (GtkWidget *text, const gchar *link, Impl *pThis)
	{
		if (!strncmp (link, "pkg://", 6)) {
			const gchar *pkg_name = link + 6;
			Ypp::Package *pkg = Ypp::get()->getPackages (Ypp::Package::PACKAGE_TYPE).find (pkg_name);
			if (pkg)
				pThis->setPackage (pkg);
			else {
				GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
					GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK, _("Package '%s' was not found."), pkg_name);
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
			}
		}
		else
			OPEN_URL (link);
	}

	static void dirname_pressed_cb (GtkWidget *text, const gchar *link, Impl *pThis)
	{ OPEN_DIRNAME (link); }

	static void hierarchy_changed_cb (GtkWidget *widget, GtkWidget *old, Impl *pThis)
	{
		GtkWidget *parent = gtk_widget_get_parent (widget);
		if (parent) {
			bool vertical = GTK_IS_HPANED (parent) || GTK_IS_HBOX (parent);
			pThis->setVerticalMode (vertical);
		}
	}

	static void filelist_cb (GtkWidget *widget, gpointer pkg)
	{
		std::string str (((Ypp::Package *) pkg)->filelist (HTML_MARKUP));
		if (str.empty())
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (widget), onlyInstalledMsg);
		else
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (widget), str.c_str());
	}

	static void changelog_cb (GtkWidget *widget, gpointer pkg)
	{
		std::string str (((Ypp::Package *) pkg)->changelog());
		if (str.empty())
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (widget), onlyInstalledMsg);
		else
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (widget), str.c_str());
	}

	static void authors_cb (GtkWidget *widget, gpointer pkg)
	{
		std::string str (((Ypp::Package *) pkg)->authors (HTML_MARKUP));
		ygtk_rich_text_set_text (YGTK_RICH_TEXT (widget), str.c_str());
	}

	static void requires_cb (GtkWidget *widget, gpointer pkg)
	{
		std::string str = _("Requires:");
		str += "<br><blockquote>";
		str += ((Ypp::Package *) pkg)->requires (HTML_MARKUP);
		YGUtils::replace (str, "\n", 1, "<br>");
		str += "</blockquote>";
		ygtk_rich_text_set_text (YGTK_RICH_TEXT (widget), str.c_str());
	}

	static void provides_cb (GtkWidget *widget, gpointer pkg)
	{
		std::string str = _("Provides:");
		str += "<br><blockquote>";
		str += ((Ypp::Package *) pkg)->provides (HTML_MARKUP);
		YGUtils::replace (str, "\n", 1, "<br>");
		str += "</blockquote>";
		ygtk_rich_text_set_text (YGTK_RICH_TEXT (widget), str.c_str());
	}

	static void contents_cb (GtkWidget *widget, gpointer pkg)
	{
		YGtkPackageView *contents = YGTK_PACKAGE_VIEW (widget);
		Ypp::QueryBase *query = new Ypp::QueryCollection ((Ypp::Package *) pkg);
		contents->setList (Ypp::PkgQuery (Ypp::Package::PACKAGE_TYPE, query), 0);
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
		if (widget)
			g_signal_connect (G_OBJECT (widget), "move-cursor",
			                  G_CALLBACK (move_cursor_cb), pThis);
	}

	static void realize_cb (GtkWidget *widget, Impl *pThis)
	{
		fix_keybindings (pThis, pThis->m_description);
		fix_keybindings (pThis, pThis->m_filelist);
		fix_keybindings (pThis, pThis->m_changelog);
		fix_keybindings (pThis, pThis->m_authors);
		fix_keybindings (pThis, pThis->m_support);
		fix_keybindings (pThis, pThis->m_requires);
		fix_keybindings (pThis, pThis->m_provides);
	}

	// make the widget request data when/if necessary; speeds up expander children
	typedef void (*UpdateCb) (GtkWidget *widget, gpointer data);

	static void ygtk_widget_passive_update (GtkWidget *widget, UpdateCb update_cb, gpointer data)
	{
		g_object_set_data (G_OBJECT (widget), "update-cb", (gpointer) update_cb);
		g_object_set_data (G_OBJECT (widget), "user-data", data);
		if (GTK_WIDGET_MAPPED (widget) || !update_cb)
			ygtk_widget_passive_update_cb (widget);
		else
			g_signal_connect (G_OBJECT (widget), "map",
				              G_CALLBACK (ygtk_widget_passive_update_cb), NULL);
	}

	static void ygtk_widget_passive_update_cb (GtkWidget *widget)
	{
		g_signal_handlers_disconnect_by_func (widget, (gpointer) ygtk_widget_passive_update_cb, NULL);
		UpdateCb update_cb = (UpdateCb) g_object_get_data (G_OBJECT (widget), "update-cb");
		gpointer data = g_object_get_data (G_OBJECT (widget), "user-data");
		if (update_cb)  // could be set to null to disable
			update_cb (widget, data);
	}

	static void setPassiveUpdate (GtkWidget *widget, UpdateCb update_cb, Ypp::Package *pkg)
	{  // convinience method
		if (widget) {
			ygtk_widget_passive_update (widget, update_cb, (gpointer) pkg);
			GtkWidget *expander = gtk_widget_get_ancestor (widget, GTK_TYPE_EXPANDER);
			pkg ? gtk_widget_show (expander) : gtk_widget_hide (expander);
		}
	}
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

