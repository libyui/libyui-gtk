/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* YGtkPkgDetailView, selectable info-box */
// check the header file for information about this widget

/*
  Textdomain "gtk"
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

static std::string onlyInstalledMsg() {
	std::string s ("<i>");
	s += _("Information only available for installed packages.");
	s += "</i>";
	return s;
}

static const char *keywordOpenTag = "<keyword>", *keywordCloseTag = "</keyword>";

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
				std::string label (YGUtils::mapKBAccel ("&Open"));
				if (getuid() == 0) {
					const char *username = getenv ("USERNAME");
					if (!username || !(*username))
						username = "root";
					label += " ("; label += _("as");
					label += " "; label += username; label += ")";
				}
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
	GtkWidget *versions_box;
	std::list <Ypp::Version> versions;

	VersionExpander()
	: DetailExpander (_("Versions"), false)
	{
		versions_box = gtk_vbox_new (FALSE, 2);

		// draw border
		GtkWidget *frame = gtk_frame_new (NULL);
		gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
		gtk_container_add (GTK_CONTAINER (frame), versions_box);
		setChild (frame);
	}

	GtkWidget *addVersion (Ypp::Selectable &sel, Ypp::Version version, GtkWidget *group)
	{
		GtkWidget *button = gtk_toggle_button_new();
		gtk_container_add (GTK_CONTAINER (button), gtk_image_new());

		GtkWidget *label = gtk_label_new ("");
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0);

		GtkWidget *hbox = gtk_hbox_new (FALSE, 6);
		gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
		gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
		gtk_widget_show_all (hbox);

		g_object_set_data (G_OBJECT (button), "nb", GINT_TO_POINTER (versions.size()));
		updateVersionWidget (hbox, sel, version);
		g_signal_connect (G_OBJECT (button), "toggled",
		                  G_CALLBACK (button_clicked_cb), this);

		GtkWidget *widget = hbox;
		if ((versions.size() % 2) == 1)
			g_signal_connect (G_OBJECT (widget), "draw",
			                  G_CALLBACK (draw_gray_cb), NULL);
		versions.push_back (version);
		gtk_box_pack_start (GTK_BOX (versions_box), widget, FALSE, TRUE, 0);
		return widget;
	}

	void updateVersionLabel (GtkWidget *label, Ypp::Selectable &sel, Ypp::Version &version)
	{
		std::string repo; bool modified;
		if (version.isInstalled()) {
			repo = _("Installed");
			modified = sel.toRemove();
		}
		else {
			repo = version.repository().name();
			modified = sel.toInstall();
		}
		modified = modified && version.toModify();

		std::string number (version.number()), arch (version.arch());
		char *tooltip = g_strdup_printf ("%s <small>(%s)</small>\n<small>%s</small>",
			number.c_str(), arch.c_str(), repo.c_str());
		number = YGUtils::truncate (number, 20, 0);
		char *str = g_strdup_printf ("%s%s <small>(%s)</small>\n<small>%s</small>%s",
			modified ? "<i>" : "", number.c_str(), arch.c_str(), repo.c_str(),
			modified ? "</i>" : "");

		gtk_label_set_markup (GTK_LABEL (label), str);
		if (number.size() > 20)
			gtk_widget_set_tooltip_markup (label, tooltip);
		g_free (tooltip); g_free (str);
	}

	void updateVersionButton (GtkWidget *button, Ypp::Selectable &sel, Ypp::Version &version)
	{
		const char *stock, *tooltip;
		bool modified, can_modify = true;
		if (version.isInstalled()) {
			tooltip = _("Remove");
			stock = GTK_STOCK_DELETE;
			can_modify = sel.canRemove();
			modified = sel.toRemove();
		}
		else {
			if (sel.hasInstalledVersion()) {
				Ypp::Version installed = sel.installed();
				if (installed < version) {
					tooltip = _("Upgrade");
					stock = GTK_STOCK_GO_UP;
				}
				else if (installed > version) {
					tooltip = _("Downgrade");
					stock = GTK_STOCK_GO_DOWN;
				}
				else {
					tooltip = _("Re-install");
					stock = GTK_STOCK_REFRESH;
				}
			}
			else {
				tooltip = _("Install");
				stock = GTK_STOCK_SAVE;
			}
			modified = sel.toInstall();
		}
		modified = modified && version.toModify();
		if (modified)
			tooltip = _("Undo");

		g_signal_handlers_block_by_func (button, (gpointer) button_clicked_cb, this);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), modified);
		g_signal_handlers_unblock_by_func (button, (gpointer) button_clicked_cb, this);
		can_modify ? gtk_widget_show (button) : gtk_widget_hide (button);
		gtk_widget_set_tooltip_text (button, tooltip);
		gtk_image_set_from_stock (GTK_IMAGE (gtk_bin_get_child (GTK_BIN(button))),
			stock, GTK_ICON_SIZE_BUTTON);
	}

	void updateVersionWidget (GtkWidget *widget, Ypp::Selectable &sel, Ypp::Version &version)
	{
		GList *children = gtk_container_get_children (GTK_CONTAINER (widget));
		GtkWidget *label = (GtkWidget *) children->data;
		GtkWidget *button = (GtkWidget *) children->next->data;
		g_list_free (children);

		updateVersionLabel (label, sel, version);
		updateVersionButton (button, sel, version);
	}

	void updateMultiselectionButton (GtkWidget *button)
	{
		Ypp::ListProps props (list);

		const char *stock, *tooltip;
		bool modified, hide = false;
		if (props.isInstalled()) {
			tooltip = _("Remove");
			stock = GTK_STOCK_DELETE;
		}
		else if (props.isNotInstalled()) {
			tooltip = _("Install");
			stock = GTK_STOCK_SAVE;
		}
		else
			hide = true;
		modified = props.toModify();
		if (modified)
			tooltip = _("Undo");

		g_signal_handlers_block_by_func (button, (gpointer) button_clicked_cb, this);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), modified);
		g_signal_handlers_unblock_by_func (button, (gpointer) button_clicked_cb, this);
		hide ? gtk_widget_hide (button) : gtk_widget_show (button);
		gtk_widget_set_tooltip_text (button, tooltip);
		gtk_image_set_from_stock (GTK_IMAGE (gtk_bin_get_child (GTK_BIN(button))),
			stock, GTK_ICON_SIZE_BUTTON);
	}

	void clearVersions()
	{
		GList *children = gtk_container_get_children (GTK_CONTAINER (versions_box));
		for (GList *i = children; i; i = i->next)
			gtk_container_remove (GTK_CONTAINER (versions_box), (GtkWidget *) i->data);
		g_list_free (children);
		versions.clear();
	}

	Ypp::Version &getVersion (GtkToggleButton *button)
	{
		int nb = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "nb"));
		std::list <Ypp::Version>::iterator it = versions.begin();
		for (int i = 0; i < nb; i++) it++;
		return *it;
	}

	static void button_clicked_cb (GtkToggleButton *button, VersionExpander *pThis)
	{
		BusyOp op;
		if (gtk_toggle_button_get_active (button)) {  // was un-pressed
			if (pThis->list.size() > 1) {
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
			else {
				Ypp::Selectable sel = pThis->list.get (0);
				Ypp::Version &version = pThis->getVersion (button);

				if (sel.toModify())
					sel.undo();
				if (version.isInstalled())
					sel.remove();
				else {
					sel.setCandidate (version);
					sel.install();
				}
			}
		}
		else  // button was pressed
			pThis->list.undo();
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
		}
		else {
			GtkWidget *button = gtk_toggle_button_new();
			gtk_container_add (GTK_CONTAINER (button), gtk_image_new());

			GtkWidget *label = gtk_label_new (_("Several selected"));
			GtkWidget *hbox = gtk_hbox_new (FALSE, 6);
			gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (versions_box), hbox, FALSE, TRUE, 0);
			gtk_widget_show_all (hbox);

			updateMultiselectionButton (button);
			g_signal_connect (G_OBJECT (button), "toggled",
			                  G_CALLBACK (button_clicked_cb), this);
		}

		gtk_widget_set_sensitive (versions_box, !props.isLocked());
	}

	virtual void showRefreshList (Ypp::List list)
	{
		// update radios
		GList *children = gtk_container_get_children (GTK_CONTAINER (versions_box));
		if (list.size() == 1) {
			Ypp::Selectable &sel = list.get (0);
			std::list <Ypp::Version>::iterator it = versions.begin();
			for (GList *i = children; i; i = i->next, it++)
				updateVersionWidget ((GtkWidget *) i->data, sel, *it);
		}
		else {
			GtkWidget *hbox = (GtkWidget *) children->data;
			GList *l = gtk_container_get_children (GTK_CONTAINER (hbox));
			GtkWidget *button = (GtkWidget *) l->next->data;
			g_list_free (l);
			updateMultiselectionButton (button);
		}
		g_list_free (children);
	}

	static gboolean draw_gray_cb (GtkWidget *widget, cairo_t *cr)
	{
		int w = gtk_widget_get_allocated_width(widget);
		int h = gtk_widget_get_allocated_height(widget);

		cairo_rectangle (cr, 0, 0, w, h);
		// use alpha to cope with styles who might not have a white background
		cairo_set_source_rgba (cr, 0, 0, 0, .060);
		cairo_fill (cr);
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
			Ypp::Selectable sel = list.get (0);
			Ypp::Package pkg (sel);

			int support = pkg.support();

			std::string label ("<b>" + std::string (_("Supportability:")) + "</b> ");
			label += Ypp::Package::supportSummary (support);
			gtk_expander_set_label (GTK_EXPANDER (expander), label.c_str());
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (text),
				Ypp::Package::supportDescription (support).c_str());

			if (support == 0)
				gtk_widget_hide(expander);
			else
				gtk_widget_show (expander);
		}
		else
			gtk_widget_hide (expander);
	}
};

struct DependenciesExpander : public DetailExpander {
	GtkWidget *grid;

	DependenciesExpander()
	: DetailExpander (_("Dependencies"), false)
	{
		grid = gtk_grid_new();
		gtk_widget_set_hexpand (grid, TRUE);
		gtk_widget_set_vexpand (grid, FALSE);
		setChild (grid);
	}

	void clear()
	{
		GList *children = gtk_container_get_children (GTK_CONTAINER (grid));
		for (GList *i = children; i; i = i->next)
			gtk_container_remove (GTK_CONTAINER (grid), (GtkWidget *) i->data);
		g_list_free (children);
	}

	void addLine (const std::string &col1, const std::string &col2,
	              const std::string &col3, int dep)
	{
//		if (dep >= 0)
//			gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new(), FALSE, TRUE, 0);

		GtkWidget *cols[3];
		for(int i = 0; i < 3; i++) {
			cols[i] = ygtk_rich_text_new();
			gtk_widget_set_size_request(cols[i], 100, -1);
		}

		ygtk_rich_text_set_text (YGTK_RICH_TEXT (cols[0]), ("<i>" + col1 + "</i>").c_str());
		ygtk_rich_text_set_text (YGTK_RICH_TEXT (cols[1]), col2.c_str());
		ygtk_rich_text_set_text (YGTK_RICH_TEXT (cols[2]), col3.c_str());

		for(int i = 1; i < 3; i++) {
			if (dep == 0)
				g_signal_connect (G_OBJECT (cols[i]), "link-clicked",
					              G_CALLBACK (requires_link_cb), NULL);
			else if (dep == 2)
				g_signal_connect (G_OBJECT (cols[i]), "link-clicked",
					              G_CALLBACK (provides_link_cb), NULL);
		}

		GList *children = gtk_container_get_children(GTK_CONTAINER(grid));
		guint top = g_list_length (children) / 3;
		g_list_free (children);

		for(int i = 0; i < 3; i++)
			gtk_grid_attach (GTK_GRID (grid), cols[i], i, top, 1, 1);
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
		std::string i ("<i>"), _i ("</i>"), b ("<b>"), _b ("</b>");
		std::string installed_str (i + b + _("Installed Version:") + _b + _i);
		std::string candidate_str (i + b + _("Available Version:") + _b + _i);
		bool hasInstalled = sel.hasInstalledVersion(),
			hasCandidate = sel.hasCandidateVersion();
		if (hasInstalled)
			installed_str += "<br>" + sel.installed().number();
		if (hasCandidate)
			candidate_str += "<br>" + sel.candidate().number();
		addLine ("", installed_str, candidate_str, -1);
		for (int dep = 0; dep < VersionDependencies::total(); dep++) {
			std::string inst, cand;
			if (hasInstalled)
				inst = VersionDependencies (sel.installed()).getText (dep);
			if (hasCandidate)
				cand = VersionDependencies (sel.candidate()).getText (dep);
			if (!inst.empty() || !cand.empty())
				addLine (VersionDependencies::getLabel (dep), inst, cand, dep);
		}
		gtk_widget_show_all (grid);
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
				case zypp::Dep::PROVIDES_e: return "Provides:";
				case zypp::Dep::PREREQUIRES_e: return "Pre-requires:";
				case zypp::Dep::REQUIRES_e: return "Requires:";
				case zypp::Dep::CONFLICTS_e: return "Conflicts:";
				case zypp::Dep::OBSOLETES_e: return "Obsoletes:";
				case zypp::Dep::RECOMMENDS_e: return "Recommends:";
				case zypp::Dep::SUGGESTS_e: return "Suggests:";
				case zypp::Dep::ENHANCES_e: return "Enhances:";
				case zypp::Dep::SUPPLEMENTS_e: return "Supplements:";
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
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (text), onlyInstalledMsg().c_str());
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
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (text), onlyInstalledMsg().c_str());
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
		if (str.empty()) {
			str = "<i>"; str += _("Attribute not-specified."); str += "</i>";
		}
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

		GdkColor *color = &gtk_widget_get_style(detail_description)->base [GTK_STATE_NORMAL];
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
                GtkAllocation alloc;
                gtk_widget_get_allocation(scroll, &alloc);

		int height = alloc.height;
		gdouble increment;
		switch (step)  {
			case GTK_MOVEMENT_DISPLAY_LINES:
				increment = height / 10.0;
				break;
			case GTK_MOVEMENT_PAGES:
				increment = height * 0.9;
				break;
			case GTK_MOVEMENT_DISPLAY_LINE_ENDS:
                                increment = gtk_adjustment_get_upper(adj) - gtk_adjustment_get_lower(adj);
				break;
			default:
				increment = 0.0;
				break;
		}

		gdouble value = gtk_adjustment_get_value(adj) + (count * increment);
		value = MIN (value, gtk_adjustment_get_upper(adj) - gtk_adjustment_get_page_size(adj));
		value = MAX (value, gtk_adjustment_get_lower(adj));
		if (value != gtk_adjustment_get_value(adj))
			gtk_adjustment_set_value (adj, value);
	}

	static void fix_keys (GtkWidget *widget, void *_scroll)
	{
		GtkWidget *scroll = (GtkWidget *) _scroll;
		if (GTK_IS_TEXT_VIEW (widget))
			g_signal_connect (G_OBJECT (widget), "move-cursor",
			                  G_CALLBACK (move_cursor_cb), scroll);
		else if (GTK_IS_CONTAINER (widget))
			gtk_container_foreach (GTK_CONTAINER (widget), fix_keys, _scroll);
	}

	static void scroll_realize_cb (GtkWidget *widget, Impl *pThis)
	{ fix_keys (widget, widget); }
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

