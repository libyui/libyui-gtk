/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* YGtkPkgDetailView, selectable info-box */
// check the header file for information about this widget

/*
  Textdomain "yast2-gtk"
 */

#include "YGi18n.h"
#include "config.h"
#include "YGDialog.h"
#include "YGUtils.h"
#include "ygtkpkgdetailview.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "YGPackageSelector.h"
#include "ygtkrichtext.h"
#include "ygtkpkgsearchentry.h"
#include "ygtkpkglistview.h"
#include <YStringTree.h>

#define BROWSER_BIN "/usr/bin/firefox"
#define GNOME_OPEN_BIN "/usr/bin/gnome-open"

static const char *onlyInstalledMsg =
	_("<i>Information only available for installed packages.</i>");

#if 0
static const char *keywordOpenTag = "<font color=\"#000000\" bgcolor=\"#ffff00\">";
static const char *keywordCloseTag = "</font>";
#else
static const char *keywordOpenTag = "<keyword>", *keywordCloseTag = "</keyword>";
#endif

struct BusyOp {
	BusyOp() {
		YGDialog::currentDialog()->busyCursor();
		while (g_main_context_iteration (NULL, FALSE)) ;
	}
	~BusyOp() {
		YGDialog::currentDialog()->normalCursor();
	}
};

struct DetailWidget {
	virtual ~DetailWidget() {}
	virtual GtkWidget *getWidget() = 0;
	virtual void refreshList (Ypp::List list) = 0;
	virtual void setList (Ypp::List list) = 0;
};

struct DetailName : public DetailWidget {
	GtkWidget *hbox, *icon, *text;

	DetailName()
	{
		text = ygtk_rich_text_new();
		icon = gtk_image_new();

		hbox = gtk_hbox_new (FALSE, 2);
		gtk_box_pack_start (GTK_BOX (hbox), text, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);
	}

	virtual GtkWidget *getWidget()
	{ return hbox; }

	virtual void refreshList (Ypp::List list)
	{
		std::string str;
		str.reserve (1024);
		if (list.size() > 0) {
			Ypp::ListProps props (list);
			str = "<p bgcolor=\"";
			str += props.toModify() ? "#f4f4b7" : "#ededed";
			str += "\"><font color=\"#000000\">";
			if (list.size() == 1) {
				Ypp::Selectable &sel = list.get (0);
				if (gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL)
					str += sel.summary() + " - " + "<b>" + sel.name() + "</b>";
				else
					str += "<b>" + sel.name() + "</b> - " + sel.summary();
			}
			else {
				str += "<b>";
				str += _("Several selected");
				str += "</b>";
			}
			str += "</font></p>";
		}
		ygtk_rich_text_set_text (YGTK_RICH_TEXT (text), str.c_str());
	}

	virtual void setList (Ypp::List list)
	{
		gtk_widget_hide (icon);
		if (list.size() == 1) {
			Ypp::Selectable &sel = list.get (0);

			gtk_image_clear (GTK_IMAGE (icon));
			GtkIconTheme *icons = gtk_icon_theme_get_default();
			GdkPixbuf *pixbuf = gtk_icon_theme_load_icon (icons,
				sel.name().c_str(), 32, GtkIconLookupFlags (0), NULL);
			if (pixbuf) {
				gtk_image_set_from_pixbuf (GTK_IMAGE (icon), pixbuf);
				g_object_unref (G_OBJECT (pixbuf));
				gtk_widget_show (icon);
			}
		}

		refreshList (list);
	}
};

struct DetailDescription : public DetailWidget {
	GtkWidget *text, *popup;
	std::string link_str;

	DetailDescription() : popup (NULL)
	{
		text = ygtk_rich_text_new();
		g_signal_connect (G_OBJECT (text), "link-clicked",
		                  G_CALLBACK (link_clicked_cb), this);
	}

	virtual ~DetailDescription()
	{ if (popup) gtk_widget_destroy (popup); }

	virtual GtkWidget *getWidget()
	{ return text; }

	virtual void refreshList (Ypp::List list)
	{ setList (list); }

	virtual void setList (Ypp::List list)
	{
		std::string str;
		str.reserve (2048);
		if (list.size() == 1) {
			Ypp::Selectable &sel = list.get (0);
			str = sel.description (true);

			YGtkPkgSearchEntry *search = YGPackageSelector::get()->getSearchEntry();
			if (search->getAttribute() == Ypp::PoolQuery::DESCRIPTION) {
				std::list <std::string> keywords;
				keywords = YGPackageSelector::get()->getSearchEntry()->getText();
				highlightMarkup (str, keywords, keywordOpenTag, keywordCloseTag,
					strlen (keywordOpenTag), strlen (keywordCloseTag));
			}

			if (sel.type() == Ypp::Selectable::PACKAGE) {
				Ypp::Package pkg (sel);
				std::string url (pkg.url());
				if (!url.empty()) {
					str += "<p><i>"; str += _("Web site:");
					str += "</i> <a href=\""; str += url; str += "\">";
					str += url; str += "</a></p>";
				}
				if (pkg.isCandidatePatch()) {
					Ypp::Selectable _patch = pkg.getCandidatePatch();
					Ypp::Patch patch (_patch);
					str += "<p><i>"; str += _("Patch issued:");
					str += "</i> "; str += _patch.summary();
					str += " <b>("; str += Ypp::Patch::prioritySummary (patch.priority());
					str += ")</b>"; str += "</p>";

				}
			}
		}
		else {
			if (list.size() > 0) {
				str += "<ul>";
				for (int i = 0; i < list.size(); i++)
					str += "<li>" + list.get (i).name() + "</li>";
				str += "</ul>";
			}
		}
		ygtk_rich_text_set_text (YGTK_RICH_TEXT (text), str.c_str());
	}

	static void link_clicked_cb (YGtkRichText *text, const gchar *link, DetailDescription *pThis)
	{
		if (!pThis->popup) {
			GtkWidget *menu = pThis->popup = gtk_menu_new();
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
		gtk_menu_popup (GTK_MENU (pThis->popup), NULL, NULL, NULL, NULL, 
		                0, gtk_get_current_event_time());
		pThis->link_str = link;
	}

	static void open_link_cb (GtkMenuItem *item, DetailDescription *pThis)
	{
		const std::string &link = pThis->link_str;
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
		(void) system (command.c_str());
	}

	static void copy_link_cb (GtkMenuItem *item, DetailDescription *pThis)
	{
		const std::string &link = pThis->link_str;
		GtkClipboard *clipboard =
			gtk_widget_get_clipboard (pThis->text, GDK_SELECTION_CLIPBOARD);
		gtk_clipboard_set_text (clipboard, link.c_str(), -1);
	}
};

struct DetailExpander : public DetailWidget {
	GtkWidget *expander;
	Ypp::List list;
	bool dirty;

	DetailExpander (const std::string &label, bool default_expanded)
	: list (0), dirty (false)
	{
		std::string str = "<b>" + label + "</b>";
		expander = gtk_expander_new (str.c_str());
		gtk_expander_set_use_markup (GTK_EXPANDER (expander), TRUE);
		gtk_expander_set_expanded (GTK_EXPANDER (expander), default_expanded);
		g_signal_connect_after (G_OBJECT (expander), "notify::expanded",
			                    G_CALLBACK (expanded_cb), this);
	}

	virtual GtkWidget *getWidget() { return expander; }

	void updateList()
	{
		if (dirty)
			if (gtk_expander_get_expanded (GTK_EXPANDER (expander)))
				if (visible()) {
					showList (list);
					dirty = false;
				}
	}

	bool visible()
	{
		if (onlySingleList())
			return list.size() == 1;
		return list.size() > 0;
	}

	virtual void setList (Ypp::List list)
	{
		this->list = list;
		dirty = true;
		visible() ? gtk_widget_show (expander) : gtk_widget_hide (expander);
		updateList();
	}

	virtual void refreshList (Ypp::List list)
	{
		if (gtk_expander_get_expanded (GTK_EXPANDER (expander)))
			showRefreshList (list);
		else
			dirty = true;
	}

	static void expanded_cb (GObject *object, GParamSpec *param_spec, DetailExpander *pThis)
	{ pThis->updateList(); }

	void setChild (GtkWidget *child)
	{ gtk_container_add (GTK_CONTAINER (expander), child); }

	// use this method to only request data when/if necessary; so to speed up expander children
	virtual void showList (Ypp::List list) = 0;
	virtual void showRefreshList (Ypp::List list) = 0;
	virtual bool onlySingleList() = 0;  // show only when list.size()==1
};

struct DetailsExpander : public DetailExpander {
	GtkWidget *text;

	DetailsExpander()
	: DetailExpander (_("Details"), true)
	{
		text = ygtk_rich_text_new();
		gtk_widget_set_size_request (text, 160, -1);
		setChild (text);
	}

	virtual bool onlySingleList() { return true; }

	virtual void showList (Ypp::List list)
	{
		Ypp::Selectable sel = list.get (0);
		ZyppSelectable zsel = sel.zyppSel();
		ZyppResObject zobj = zsel->theObj();
		ZyppPackage zpkg = castZyppPackage (zobj);

		std::string str;
		std::string b ("<i>"), _b ("</i> "), i (""), _i(""), br ("<br/>");
		str.reserve (2048);
		str += b + _("Size:") + _b + i + zobj->installSize().asString() + _i;
		str += br + b + _("License:") + _b + i + zpkg->license() + _i;
		if (zsel->hasInstalledObj())
			str += br + b + ("Installed at:") + _b + i +
				zsel->installedObj()->installtime().form ("%x") + _i;
		if (zsel->hasCandidateObj())
			str += br + b + ("Latest build:") + _b + i +
				zsel->candidateObj()->buildtime().form ("%x") + _i;
		ygtk_rich_text_set_text (YGTK_RICH_TEXT (text), str.c_str());
	}

	virtual void showRefreshList (Ypp::List list) {}
};

typedef zypp::ResPoolProxy::const_iterator	ZyppPoolIterator;
inline ZyppPoolIterator zyppSrcPkgBegin()
{ return zyppPool().byKindBegin<zypp::SrcPackage>();	}
inline ZyppPoolIterator zyppSrcPkgEnd()
{ return zyppPool().byKindEnd<zypp::SrcPackage>();	}

struct VersionExpander : public DetailExpander {
	GtkWidget *box, *versions_box, *button, *undo_button;
	std::list <Ypp::Version> versions;

	VersionExpander()
	: DetailExpander (_("Versions"), false)
	{
		button = gtk_button_new_with_label ("");
		g_signal_connect (G_OBJECT (button), "clicked",
		                  G_CALLBACK (button_clicked_cb), this);

		GtkSettings *settings = gtk_settings_get_default();
		gboolean show_button_images;
		g_object_get (settings, "gtk-button-images", &show_button_images, NULL);
		if (show_button_images) {
			undo_button = gtk_button_new_with_label ("");
			gtk_button_set_image (GTK_BUTTON (undo_button),
				gtk_image_new_from_stock (GTK_STOCK_UNDO, GTK_ICON_SIZE_BUTTON));
			gtk_widget_set_tooltip_text (undo_button, _("Undo"));
		}
		else
			undo_button = gtk_button_new_with_label (_("Undo"));
		g_signal_connect (G_OBJECT (undo_button), "clicked",
		                  G_CALLBACK (undo_clicked_cb), this);

		versions_box = gtk_vbox_new (FALSE, 2);

		GtkWidget *button_box = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_end (GTK_BOX (button_box), undo_button, FALSE, TRUE, 0);
		gtk_box_pack_end (GTK_BOX (button_box), button, FALSE, TRUE, 0);

		box = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (box), versions_box, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (box), button_box, FALSE, TRUE, 0);
		setChild (box);
	}

	void clearVersions()
	{
		GList *children = gtk_container_get_children (GTK_CONTAINER (versions_box));
		for (GList *i = children; i; i = i->next)
			gtk_container_remove (GTK_CONTAINER (versions_box), (GtkWidget *) i->data);
		g_list_free (children);
		versions.clear();
	}

	GtkWidget *addVersion (Ypp::Selectable &sel, Ypp::Version version, GtkWidget *group)
	{
		GtkWidget *radio = gtk_radio_button_new_with_label_from_widget
			(GTK_RADIO_BUTTON (group), "");
		updateRadio (radio, sel, version);

		gtk_box_pack_start (GTK_BOX (versions_box), radio, FALSE, TRUE, 0);
		if (version.toModify())
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);
		g_signal_connect (G_OBJECT (radio), "toggled",
			              G_CALLBACK (version_toggled_cb), this);

		if ((versions.size() % 2) == 1)
			g_signal_connect (G_OBJECT (radio), "expose-event",
			                  G_CALLBACK (draw_gray_cb), NULL);
		versions.push_back (version);
		return radio;
	}

	void updateRadio (GtkWidget *radio, Ypp::Selectable &sel, Ypp::Version &version)
	{
		bool modified;
		std::string repo;
		if (version.isInstalled()) {
			repo = _("Installed");
			modified = sel.toRemove();
		}
		else {
			repo = version.repository().name();
			modified = sel.toInstall();
		}
		modified = modified && version.toModify();

		std::string number (version.number());
		std::string arch (version.arch());
		char *tooltip = g_strdup_printf ("%s <small>(%s)</small>\n<small>%s</small>",
			number.c_str(), arch.c_str(), repo.c_str());
		number = YGUtils::truncate (number, 20, 0);
		char *label = g_strdup_printf ("%s%s <small>(%s)</small>\n<small>%s</small>%s",
			modified ? "<i>" : "", number.c_str(), arch.c_str(), repo.c_str(),
			modified ? "</i>" : "");

		gtk_label_set_markup (GTK_LABEL (GTK_BIN (radio)->child), label);
		if (number.size() > 20)
			gtk_widget_set_tooltip_markup (radio, tooltip);
		g_free (tooltip);
		g_free (label);
	}

	Ypp::Version &getSelected()
	{
		std::list <Ypp::Version>::iterator it = versions.begin();
		GList *children = gtk_container_get_children (GTK_CONTAINER (versions_box));
		for (GList *i = children; i; i = i->next, it++)
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (i->data)))
				break;
		g_list_free (children);
		return *it;
	}

	virtual bool onlySingleList() { return false; }

	virtual void showList (Ypp::List list)
	{
		Ypp::ListProps props (list);
		clearVersions();

		if (list.size() == 1) {
			Ypp::Selectable sel = list.get (0);
			GtkWidget *radio = 0;
			for (int i = 0; i < sel.totalVersions(); i++)
				radio = addVersion (sel, sel.version (i), radio);
			gtk_widget_show_all (versions_box);
		}

		// is locked
		if (props.isLocked() || props.isUnlocked())
			gtk_widget_set_sensitive (button, !props.isLocked());
		else
			gtk_widget_set_sensitive (button, TRUE);

		updateButton();
	}

	virtual void showRefreshList (Ypp::List list)
	{
		updateButton();

		// update radios
		if (list.size() == 1) {
			Ypp::Selectable &sel = list.get (0);
			std::list <Ypp::Version>::iterator it = versions.begin();
			GList *children = gtk_container_get_children (GTK_CONTAINER (versions_box));
			for (GList *i = children; i; i = i->next, it++)
				updateRadio ((GtkWidget *) i->data, sel, *it);
			g_list_free (children);
		}
	}

	void updateButton()
	{
		const char *label = 0, *stock = 0;
		bool modified = false, can_modify = true, can_never_modify = false;
		if (list.size() == 1) {
			Ypp::Selectable sel = list.get (0);
			Ypp::Version &version = getSelected();

			if (version.isInstalled()) {
				label = _("Remove");
				stock = GTK_STOCK_DELETE;
				can_never_modify = !sel.canRemove();
				modified = sel.toRemove();
			}
			else {
				if (sel.hasInstalledVersion()) {
					Ypp::Version installed = sel.installed();
					if (installed < version) {
						label = _("Upgrade");
						stock = GTK_STOCK_GO_UP;
					}
					else if (installed > version) {
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
				modified = sel.toInstall();
			}
			can_modify = !sel.isLocked();
			modified = modified && version.toModify();
		}
		else {
			Ypp::ListProps props (list);
			modified = props.toModify();
			if (props.hasUpgrade()) {
				label = _("Upgrade");
				stock = GTK_STOCK_GO_UP;
			}
			else if (props.isInstalled()) {
				label = _("Remove");
				stock = GTK_STOCK_DELETE;
				can_never_modify = !props.canRemove();
			}
			else if (props.isNotInstalled()) {
				label = _("Install");
				stock = GTK_STOCK_SAVE;
			}
			else if (props.toModify()) {
				label = _("Undo");
				stock = GTK_STOCK_UNDO;
				modified = false;  // don't show another undo button
			}
			can_modify = props.isUnlocked();
		}

		if (label) {
			gtk_button_set_label (GTK_BUTTON (button), label);
			GtkWidget *image = gtk_image_new_from_stock (
				stock, GTK_ICON_SIZE_BUTTON);
			gtk_button_set_image (GTK_BUTTON (button), image);
			gtk_widget_show (button);
		}
		else
			gtk_widget_hide (button);
		gtk_widget_set_sensitive (button, !modified && can_modify);
		if (can_modify)
			gtk_widget_set_tooltip_text (button, NULL);
		else
			gtk_widget_set_tooltip_text (button, _("Package is locked"));
		!can_never_modify ? gtk_widget_show (button) : gtk_widget_hide (button);
		modified ? gtk_widget_show (undo_button) : gtk_widget_hide (undo_button);
	}

	static void version_toggled_cb (GtkToggleButton *button, VersionExpander *pThis)
	{
		if (gtk_toggle_button_get_active (button)) {
			Ypp::Selectable sel = pThis->list.get (0);
			Ypp::Version &version = pThis->getSelected();

			// enclosing this within startTransactions() so the button is updated
			// before signaling any change, for a snappy feel
			Ypp::startTransactions();
			if (!sel.toInstall())
				sel.setCandidate (version);

			pThis->updateButton();
			while (g_main_context_iteration (NULL, FALSE)) ;
			Ypp::finishTransactions();
		}
	}

	static void button_clicked_cb (GtkButton *button, VersionExpander *pThis)
	{
		BusyOp op;
		if (pThis->list.size() == 1) {
			Ypp::Selectable sel = pThis->list.get (0);
			Ypp::Version &version = pThis->getSelected();

			if (version.isInstalled())
				sel.remove();
			else {
				sel.setCandidate (version);
				sel.install();
			}
		}
		else {
			Ypp::ListProps props (pThis->list);
			if (props.hasUpgrade())
				pThis->list.install();
			else if (props.isInstalled())
				pThis->list.remove();
			else if (props.isNotInstalled())
				pThis->list.install();
			else if (props.toModify())
				pThis->list.undo();
		}
	}

	static void undo_clicked_cb (GtkButton *button, VersionExpander *pThis)
	{ BusyOp op; pThis->list.undo(); }

	static gboolean sel_modified_idle_cb (gpointer data)
	{ Ypp::runSolver(); return FALSE; }

	static gboolean draw_gray_cb (GtkWidget *widget, GdkEventExpose *event)
	{
		GtkAllocation *alloc = &widget->allocation;
		int x = alloc->x, y = alloc->y, w = alloc->width, h = alloc->height;

		cairo_t *cr = gdk_cairo_create (widget->window);
		cairo_rectangle (cr, x, y, w, h);
		// use alpha to cope with styles who might not have a white background
		cairo_set_source_rgba (cr, 0, 0, 0, .060);
		cairo_fill (cr);
		cairo_destroy (cr);
		return FALSE;
	}
};

struct SupportExpander : public DetailWidget {
	// not derived from DetailExpander as this one behaves a little differently
	GtkWidget *text, *expander;

	SupportExpander()
	{
		text = ygtk_rich_text_new();
		expander = gtk_expander_new ("");
		gtk_expander_set_use_markup (GTK_EXPANDER (expander), TRUE);
		gtk_container_add (GTK_CONTAINER (expander), text);
	}

	virtual GtkWidget *getWidget() { return expander; }

	virtual void refreshList (Ypp::List list) {}

	virtual void setList (Ypp::List list)
	{
		if (list.size() == 1) {
			gtk_widget_show (expander);

			Ypp::Selectable sel = list.get (0);
			Ypp::Package pkg (sel);

			std::string label ("<b>" + std::string (_("Supportability:")) + "</b> ");
			label += Ypp::Package::supportSummary (pkg.support());
			gtk_expander_set_label (GTK_EXPANDER (expander), label.c_str());

			ygtk_rich_text_set_text (YGTK_RICH_TEXT (text),
				Ypp::Package::supportDescription (pkg.support()).c_str());
		}
		else
			gtk_widget_hide (expander);
	}
};

struct DependenciesExpander : public DetailExpander {
	GtkWidget *vbox;

	DependenciesExpander()
	: DetailExpander (_("Dependencies"), false)
	{
		vbox = gtk_vbox_new (FALSE, 6);
		setChild (vbox);
	}

	void clear()
	{
		GList *children = gtk_container_get_children (GTK_CONTAINER (vbox));
		for (GList *i = children; i; i = i->next)
			gtk_container_remove (GTK_CONTAINER (vbox), (GtkWidget *) i->data);
		g_list_free (children);
	}

	void addLine (const std::string &col1, const std::string &col2,
	              const std::string &col3, int dep)
	{
//		if (dep >= 0)
//			gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new(), FALSE, TRUE, 0);

		GtkWidget *hbox = gtk_hbox_new (FALSE, 6);
		GtkWidget *col;
		col = ygtk_rich_text_new();
		gtk_widget_set_size_request (col, 100, -1);
		ygtk_rich_text_set_text (YGTK_RICH_TEXT (col),
			("<i>" + col1 + "</i>").c_str());
		gtk_box_pack_start (GTK_BOX (hbox), col, FALSE, TRUE, 0);
		// by settings both the two following "col" text widgets with the same
		// fixed width, and expanded=True, we ensure all rows have the same size
		col = ygtk_rich_text_new();
		gtk_widget_set_size_request (col, 80, -1);
		ygtk_rich_text_set_text (YGTK_RICH_TEXT (col), col2.c_str());
		if (dep == 0)
			g_signal_connect (G_OBJECT (col), "link-clicked",
			                  G_CALLBACK (requires_link_cb), NULL);
		else if (dep == 2)
			g_signal_connect (G_OBJECT (col), "link-clicked",
			                  G_CALLBACK (provides_link_cb), NULL);
		gtk_box_pack_start (GTK_BOX (hbox), col, TRUE, TRUE, 0);
		col = ygtk_rich_text_new();
		gtk_widget_set_size_request (col, 80, -1);
		ygtk_rich_text_set_text (YGTK_RICH_TEXT (col), col3.c_str());
		if (dep == 0)
			g_signal_connect (G_OBJECT (col), "link-clicked",
			                  G_CALLBACK (requires_link_cb), NULL);
		else if (dep == 2)
			g_signal_connect (G_OBJECT (col), "link-clicked",
			                  G_CALLBACK (provides_link_cb), NULL);
		gtk_box_pack_start (GTK_BOX (hbox), col, TRUE, TRUE, 0);

		gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	}

	static void requires_link_cb (GtkWidget *widget, const gchar *link)
	{ YGPackageSelector::get()->searchFor (Ypp::PoolQuery::REQUIRES, link); }

	static void provides_link_cb (GtkWidget *widget, const gchar *link)
	{ YGPackageSelector::get()->searchFor (Ypp::PoolQuery::PROVIDES, link); }


	virtual bool onlySingleList() { return true; }

	virtual void showList (Ypp::List list)
	{
		Ypp::Selectable sel = list.get (0);

		clear();
		std::string installed_str (_("<i>Installed Version</i>"));
		std::string candidate_str (_("<i>Available Version</i>"));
		if (sel.hasInstalledVersion())
			installed_str += "<br>" + sel.installed().number();
		if (sel.hasCandidateVersion())
			candidate_str += "<br>" + sel.candidate().number();
		addLine ("", installed_str, candidate_str, -1);
		for (int dep = 0; dep < VersionDependencies::total(); dep++) {
			std::string inst, cand;
			if (sel.hasInstalledVersion())
				inst = VersionDependencies (sel.installed()).getText (dep);
			if (sel.hasCandidateVersion())
				cand = VersionDependencies (sel.candidate()).getText (dep);
			if (!inst.empty() || !cand.empty())
				addLine (VersionDependencies::getLabel (dep), inst, cand, dep);
		}
		gtk_widget_show_all (vbox);
	}

	virtual void showRefreshList (Ypp::List list)
	{ showList (list); }

	struct VersionDependencies {
		VersionDependencies (Ypp::Version version)
		: m_version (version) {}

		static int total()
		{ return ((int) zypp::Dep::SUPPLEMENTS_e) + 1; }

		static const char *getLabel (int dep)
		{
			switch ((zypp::Dep::for_use_in_switch) dep) {
				case zypp::Dep::PROVIDES_e: return _("Provides:");
				case zypp::Dep::PREREQUIRES_e: return _("Pre-requires:");
				case zypp::Dep::REQUIRES_e: return _("Requires:");
				case zypp::Dep::CONFLICTS_e: return _("Conflicts:");
				case zypp::Dep::OBSOLETES_e: return _("Obsoletes:");
				case zypp::Dep::RECOMMENDS_e: return _("Recommends:");
				case zypp::Dep::SUGGESTS_e: return _("Suggests:");
				case zypp::Dep::ENHANCES_e: return _("Enhances:");
				case zypp::Dep::SUPPLEMENTS_e: return _("Supplements:");
			}
			return 0;
		}

		// zypp::Dep(zypp::Dep::for_use_in_switch) construtor is private
		static zypp::Dep getDep (int dep)
		{
			switch (dep) {
				case 0: return zypp::Dep::PROVIDES;
				case 1: return zypp::Dep::PREREQUIRES;
				case 2: return zypp::Dep::REQUIRES;
				case 3: return zypp::Dep::CONFLICTS;
				case 4: return zypp::Dep::OBSOLETES;
				case 5: return zypp::Dep::RECOMMENDS;
				case 6: return zypp::Dep::SUGGESTS;
				case 7: return zypp::Dep::ENHANCES;
				case 8: return zypp::Dep::SUPPLEMENTS;
			}
			return zypp::Dep::PROVIDES;
		}


		std::string getText (int dep)
		{
			std::string keyword;
			YGtkPkgSearchEntry *search = YGPackageSelector::get()->getSearchEntry();
			if ((dep == 0 && search->getAttribute() == Ypp::PoolQuery::PROVIDES) ||
			    (dep == 2 && search->getAttribute() == Ypp::PoolQuery::REQUIRES))
				keyword = search->getTextStr();

			ZyppResObject zobj = m_version.zyppObj();
			zypp::Capabilities caps = zobj->dep (getDep (dep));
			std::string ret;
			ret.reserve (4092);
			for (zypp::Capabilities::const_iterator it = caps.begin();
			     it != caps.end(); it++) {
				if (!ret.empty())
					ret += ", ";

				std::string str (it->asString());
				bool highlight = (str == keyword);


				if (dep == 0 || dep == 2) {
					std::string::size_type i;
					i = MIN (str.find (' '), str.find ('('));

					std::string name (str, 0, i);
					ret += "<a href=\"" + name + "\">" + YGUtils::escapeMarkup (name) + "</a>";
					if (i != std::string::npos) {
						std::string rest (str, i);
						ret += YGUtils::escapeMarkup (rest);
					}
				}
				else
					ret += YGUtils::escapeMarkup (str);

				if (highlight)
					ret += keywordCloseTag;
			}
			return ret;
		}

		Ypp::Version m_version;
	};
};

struct FilelistExpander : public DetailExpander {
	GtkWidget *text;

	FilelistExpander()
	: DetailExpander (_("File list"), false)
	{
		text = ygtk_rich_text_new();
		g_signal_connect (G_OBJECT (text), "link-clicked",
				          G_CALLBACK (dirname_pressed_cb), this);
		setChild (text);
	}

	virtual bool onlySingleList() { return true; }

	virtual void showList (Ypp::List list)
	{
		Ypp::Selectable sel = list.get (0);
		if (sel.isInstalled())
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (text), filelist (sel).c_str());
		else
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (text), onlyInstalledMsg);
	}

	virtual void showRefreshList (Ypp::List list) {}

	static void dirname_pressed_cb (GtkWidget *text, const gchar *link, FilelistExpander *pThis)
	{
		gchar *cmd = g_strdup_printf (GNOME_OPEN_BIN " %s &", link);
		(void) system (cmd);
		g_free (cmd);
	}

	static std::string filelist (Ypp::Selectable &sel)
	{
		std::string text;
		text.reserve (4096);

		ZyppResObject zobj = sel.installed().zyppObj();
		ZyppPackage zpkg = castZyppPackage (zobj);
		if (zpkg) {
			YStringTree tree ("");

			zypp::Package::FileList files = zpkg->filelist();
			for (zypp::Package::FileList::iterator it = files.begin();
				 it != files.end(); it++)
				tree.addBranch (*it, '/');

			struct inner {
				static std::string path (YStringTreeItem *item)
				{
					std::string str;
					str.reserve (1024);
					for (YStringTreeItem *i = item->parent(); i->parent(); i = i->parent()) {
						std::string val (i->value().orig());
						std::string prefix ("/");
						str = prefix + val + str;
					}
					return str;
				}

				static void traverse (YStringTreeItem *item, std::string &text)
				{
					if (!item) return;

					// traverse nodes first
					bool has_leaves = false;
					for (YStringTreeItem *i = item; i; i = i->next()) {
						YStringTreeItem *child = i->firstChild();
						if (child)
							traverse (child, text);
						else
							has_leaves = true;
					}

					// traverse leaves now
					if (has_leaves) {
						std::string dirname (path (item));
						text += "<a href=\"" + dirname + "\">" + dirname + "</a>";
						text += "<blockquote>";

						std::string keyword;
						YGtkPkgSearchEntry *search = YGPackageSelector::get()->getSearchEntry();
						if (search->getAttribute() == Ypp::PoolQuery::FILELIST)
							keyword = search->getTextStr();
						if (!keyword.empty() && keyword[0] == '/') {
							std::string _dirname (keyword, 0, keyword.find_last_of ('/'));
							if (dirname == _dirname)
								keyword = std::string (keyword, _dirname.size()+1);
							else
								keyword.clear();
						}

						for (YStringTreeItem *i = item; i; i = i->next())
							if (!i->firstChild()) {
								if (i != item)  // not first
									text += ", ";
								std::string basename (i->value().orig());

								bool highlight = (basename == keyword);
								if (highlight)
									text += keywordOpenTag;
								text += basename;
								if (highlight)
									text += keywordCloseTag;
							 }

						text += "</blockquote>";
					}
				}
			};

			inner::traverse (tree.root(), text);
		}
		return text;
	}
};

struct ChangelogExpander : public DetailExpander {
	GtkWidget *text;

	ChangelogExpander()
	: DetailExpander (_("Changelog"), false)
	{
		text = ygtk_rich_text_new();
		setChild (text);
	}

	virtual bool onlySingleList() { return true; }

	virtual void showList (Ypp::List list)
	{
		Ypp::Selectable sel = list.get (0);
		if (sel.isInstalled())
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (text), changelog (sel).c_str());
		else
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (text), onlyInstalledMsg);
	}

	virtual void showRefreshList (Ypp::List list) {}

	static std::string changelog (Ypp::Selectable &sel)
	{
		std::string text;
		text.reserve (32768);
		text += "<p><i>";
		text += _("Changelog applies only to the installed version.");
		text += "</i></p>";
		ZyppResObject zobj = sel.installed().zyppObj();
		ZyppPackage zpkg = castZyppPackage (zobj);
		if (zpkg) {
			const std::list <zypp::ChangelogEntry> &logs = zpkg->changelog();
			for (std::list <zypp::ChangelogEntry>::const_iterator it = logs.begin();
				 it != logs.end(); it++) {
				std::string date (it->date().form ("%d %B %Y")), author (it->author()),
					        changes (it->text());
				author = YGUtils::escapeMarkup (author);
				changes = YGUtils::escapeMarkup (changes);
				YGUtils::replace (changes, "\n", 1, "<br>");
				if (author.compare (0, 2, "- ", 2) == 0)  // zypp returns a lot of author strings as
					author.erase (0, 2);                  // "- author". wtf?
				text += "<i>" + date + " (" + author + "):</i><br><blockquote>" + changes + "</blockquote>";
			}
		}
		return text;
	}
};

struct AuthorsExpander : public DetailExpander {
	GtkWidget *text;

	AuthorsExpander()
	: DetailExpander (_("Authors"), false)
	{
		text = ygtk_rich_text_new();
		setChild (text);
	}

	virtual bool onlySingleList() { return true; }

	virtual void showList (Ypp::List list)
	{
		Ypp::Selectable sel = list.get (0);
		std::string str (authors (sel));
		if (str.empty())
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (text), _("<i>Not specified.</i>"));
		else
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (text), str.c_str());
	}

	virtual void showRefreshList (Ypp::List list) {}

	static std::string authors (Ypp::Selectable &sel)
	{
		std::string text;
		ZyppResObject object = sel.zyppSel()->theObj();
		ZyppPackage package = castZyppPackage (object);
		if (package) {
			std::string packager = package->packager(), vendor = package->vendor(), authors;
			packager = YGUtils::escapeMarkup (packager);
			vendor = YGUtils::escapeMarkup (vendor);
			const std::list <std::string> &authorsList = package->authors();
			for (std::list <std::string>::const_iterator it = authorsList.begin();
				 it != authorsList.end(); it++) {
				std::string author (*it);
				author = YGUtils::escapeMarkup (author);
				if (!authors.empty())
					authors += "<br>";
				authors += author;
			}
			// look for Authors line in description
			std::string description = package->description();
			std::string::size_type i = description.find ("\nAuthors:\n-----", 0);
			if (i != std::string::npos) {
				i += sizeof ("\nAuthors:\n----");
				i = description.find ("\n", i);
				if (i != std::string::npos)
					i++;
			}
			else {
				i = description.find ("\nAuthor:", 0);
				if (i == std::string::npos) {
					i = description.find ("\nAuthors:", 0);
					if (i != std::string::npos)
						i++;
				}
				if (i != std::string::npos)
					i += sizeof ("\nAuthor:");
			}
			if (i != std::string::npos) {
				std::string str = description.substr (i);
				str = YGUtils::escapeMarkup (str);
				YGUtils::replace (str, "\n", 1, "<br>");
				authors += str;
			}

			if (!authors.empty()) {
				text = "<i>"; text += _("Developed by:"); text += "</i>";
				text += ("<blockquote>" + authors) + "</blockquote>";
				if (!packager.empty() || !vendor.empty()) {
					text += "<i>"; text += _("Packaged by:"); text += "</i>";
					text += "<blockquote>";
					if (!packager.empty()) text += packager + " ";
					if (!vendor.empty()) text += "(" + vendor + ")";
					text += "</blockquote>";
				}
			}
		}
		return text;
	}
};

struct ContentsExpander : public DetailExpander {
	YGtkPkgListView *view;

	ContentsExpander()
	: DetailExpander (_("Applies to"), false)
	{
		view = new YGtkPkgListView (true, Ypp::List::NAME_SORT, false, false);
		view->addCheckColumn (INSTALLED_CHECK_PROP);
		view->addTextColumn (_("Name"), NAME_SUMMARY_PROP, true, -1);
		view->addTextColumn (_("Version"), VERSION_PROP, true, 125);
		view->addTextColumn (_("Size"), SIZE_PROP, false, 85);
		view->addTextColumn (_("Repository"), REPOSITORY_PROP, false, 180);
		view->addTextColumn (_("Supportability"), SUPPORT_PROP, false, 120);
		gtk_widget_set_size_request (view->getWidget(), -1, 150);

		setChild (view->getWidget());
	}

	virtual ~ContentsExpander()
	{ delete view; }

	virtual bool onlySingleList() { return true; }

	virtual void showList (Ypp::List list)
	{
		Ypp::Selectable sel = list.get (0);
		Ypp::Collection col (sel);

		Ypp::PoolQuery query (Ypp::Selectable::PACKAGE);
		query.addCriteria (new Ypp::FromCollectionMatch (col));
		view->setQuery (query);
	}

	virtual void showRefreshList (Ypp::List list) {}
};

struct YGtkPkgDetailView::Impl : public Ypp::SelListener
{
std::list <DetailWidget *> m_widgets;
GtkWidget *m_scroll;
Ypp::List m_list;

	Impl() : m_list (0)
	{
		DetailWidget *widget;

		GtkWidget *side_vbox = gtk_vbox_new (FALSE, 0);

		if (!YGPackageSelector::get()->onlineUpdateMode()) {
			widget = new DetailsExpander();
			m_widgets.push_back (widget);
			gtk_box_pack_start (GTK_BOX (side_vbox), widget->getWidget(), FALSE, TRUE, 0);
		}
		widget = new VersionExpander();
		m_widgets.push_back (widget);
		gtk_box_pack_start (GTK_BOX (side_vbox), widget->getWidget(), FALSE, TRUE, 0);

		GtkWidget *main_vbox = gtk_vbox_new (FALSE, 0);

		widget = new DetailName();
		m_widgets.push_back (widget);
		gtk_box_pack_start (GTK_BOX (main_vbox), widget->getWidget(), FALSE, TRUE, 0);

		widget = new DetailDescription();
		m_widgets.push_back (widget);
		gtk_box_pack_start (GTK_BOX (main_vbox), widget->getWidget(), FALSE, TRUE, 0);
		GtkWidget *detail_description = widget->getWidget();

		if (YGPackageSelector::get()->onlineUpdateMode()) {
			widget = new ContentsExpander();
			m_widgets.push_back (widget);
			gtk_box_pack_start (GTK_BOX (main_vbox), widget->getWidget(), FALSE, TRUE, 0);
		}
		else {
			widget = new FilelistExpander();
			m_widgets.push_back (widget);
			gtk_box_pack_start (GTK_BOX (main_vbox), widget->getWidget(), FALSE, TRUE, 0);
			widget = new ChangelogExpander();
			m_widgets.push_back (widget);
			gtk_box_pack_start (GTK_BOX (main_vbox), widget->getWidget(), FALSE, TRUE, 0);
			widget = new AuthorsExpander();
			m_widgets.push_back (widget);
			gtk_box_pack_start (GTK_BOX (main_vbox), widget->getWidget(), FALSE, TRUE, 0);
			widget = new DependenciesExpander();
			m_widgets.push_back (widget);
			gtk_box_pack_start (GTK_BOX (main_vbox), widget->getWidget(), FALSE, TRUE, 0);
			widget = new SupportExpander();
			m_widgets.push_back (widget);
			gtk_box_pack_start (GTK_BOX (main_vbox), widget->getWidget(), FALSE, TRUE, 0);
		}

		GtkWidget *hbox = gtk_hbox_new (FALSE, 2);
		gtk_box_pack_start (GTK_BOX (hbox), main_vbox, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), side_vbox, FALSE, TRUE, 0);

		GtkWidget *child = gtk_event_box_new();
		gtk_container_add (GTK_CONTAINER (child), hbox);

		GdkColor *color = &detail_description->style->base [GTK_STATE_NORMAL];
		gtk_widget_modify_bg (child, GTK_STATE_NORMAL, color);

		m_scroll = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (m_scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (m_scroll), child);

		gtk_widget_show_all (m_scroll);
		g_signal_connect (G_OBJECT (m_scroll), "realize",
		                  G_CALLBACK (scroll_realize_cb), this);
		Ypp::addSelListener (this);
	}

	~Impl()
	{
		for (std::list <DetailWidget *>::iterator it = m_widgets.begin();
		     it != m_widgets.end(); it++)
			delete *it;
		Ypp::removeSelListener (this);
	}

	void refreshList (Ypp::List list)
	{
		for (std::list <DetailWidget *>::iterator it = m_widgets.begin();
		     it != m_widgets.end(); it++)
			(*it)->refreshList (list);
	}

	void setList (Ypp::List list)
	{
		for (std::list <DetailWidget *>::iterator it = m_widgets.begin();
		     it != m_widgets.end(); it++)
			(*it)->setList (list);

		m_list = list;
		scrollTop();
	}

	virtual void selectableModified()
	{ refreshList (m_list); }

	void scrollTop()
	{
		GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW (m_scroll);
		GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment (scroll);
		YGUtils::scrollWidget (vadj, true);
	}

	// fix cursor keys support
	static void move_cursor_cb (GtkTextView *view, GtkMovementStep step, gint count,
	                            gboolean extend_selection, GtkWidget *scroll)
	{
		GtkScrolledWindow *_scroll = GTK_SCROLLED_WINDOW (scroll);
		GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment (_scroll);
		int height = scroll->allocation.height;
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

	static void fix_keybindings (GtkWidget *scroll, GtkWidget *widget)
	{
		if (GTK_IS_TEXT_VIEW (widget))
			g_signal_connect (G_OBJECT (widget), "move-cursor",
			                  G_CALLBACK (move_cursor_cb), scroll);
		else if (GTK_IS_CONTAINER (widget)) {
			GList *children = gtk_container_get_children (GTK_CONTAINER (widget));
			for (GList *i = children; i; i = i->next)
				fix_keybindings (scroll, (GtkWidget *) i->data);
			g_list_free (children);
		}
	}

	static void scroll_realize_cb (GtkWidget *widget, Impl *pThis)
	{ fix_keybindings (widget, widget); }
};

YGtkPkgDetailView::YGtkPkgDetailView()
: impl (new Impl()) {}

YGtkPkgDetailView::~YGtkPkgDetailView()
{ delete impl; }

GtkWidget *YGtkPkgDetailView::getWidget()
{ return impl->m_scroll; }

void YGtkPkgDetailView::setSelectable (Ypp::Selectable &sel)
{
	Ypp::List list (1);
	list.append (sel);
	impl->setList (list);
}

void YGtkPkgDetailView::setList (Ypp::List list)
{ impl->setList (list); }

