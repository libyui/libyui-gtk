/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* Textdomain "yast2-gtk" */
/* YGtkPkgStatusBar, a status-bar */
// check the header file for information about this widget

#include "YGi18n.h"
#include "config.h"
#include "YGUI.h"
#include "ygtkpkgstatusbar.h"
#include "YGPackageSelector.h"
#include "YGUtils.h"
#include "ygtkpkglistview.h"
#include "ygtkpkgundolist.h"
#include "yzyppwrapper.h"
#include <gtk/gtk.h>

static void small_button_style_set_cb (GtkWidget *button, GtkStyle *prev_style)
{  // based on gedit
	int width, height;
	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (button),
		GTK_ICON_SIZE_MENU, &width, &height);
	gtk_widget_set_size_request (button, width+2, height+2);
}

static void setup_small_widget_style()
{
	static bool first_time = true;
	if (first_time) {
		first_time = false;
		gtk_rc_parse_string (
			"style \"small-widget-style\"\n"
			"{\n"
			"  GtkWidget::focus-padding = 0\n"
			"  GtkWidget::focus-line-width = 0\n"
			"  xthickness = 0\n"
			"  ythickness = 0\n"
			"}\n"
			"widget \"*.small-widget\" style \"small-widget-style\"");
	}
}

static GtkWidget *create_small_button (const char *stock_icon, const char *tooltip)
{
	setup_small_widget_style();

	GtkWidget *button = gtk_button_new();
	gtk_widget_set_name (button, "small-widget");
	gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text (button, tooltip);
	GtkWidget *image = gtk_image_new_from_stock (stock_icon, GTK_ICON_SIZE_MENU);
	gtk_button_set_image (GTK_BUTTON (button), image);
	g_signal_connect (G_OBJECT (button), "style-set",
	                  G_CALLBACK (small_button_style_set_cb), NULL);
	return button;
}

struct LastChange {
	GtkWidget *hbox, *icon, *text, *undo_button;

	GtkWidget *getWidget() { return hbox; }

	LastChange()
	{
		icon = gtk_image_new();
		gtk_widget_set_size_request (icon, 16, 16);
		text = gtk_label_new ("");
		gtk_misc_set_alignment (GTK_MISC (text), 0, .5);
		gtk_label_set_ellipsize (GTK_LABEL (text), PANGO_ELLIPSIZE_END);
		undo_button = create_small_button (GTK_STOCK_UNDO, _("Undo"));
		g_signal_connect (G_OBJECT (undo_button), "clicked",
		                  G_CALLBACK (undo_clicked_cb), this);
		gchar *str = g_strdup_printf ("(<a href=\"more\">%s</a>)", _("more"));
		GtkWidget *more = gtk_label_new (str);
		g_free (str);
		gtk_label_set_use_markup (GTK_LABEL (more), TRUE);
		gtk_label_set_track_visited_links (GTK_LABEL (more), FALSE);
		g_signal_connect (G_OBJECT (more), "activate-link",
		                  G_CALLBACK (more_link_cb), this);

		hbox = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), text, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), undo_button, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), more, FALSE, TRUE, 0);
	}

	void undoChanged (YGtkPkgUndoList *list)
	{
		int auto_count;
		Ypp::Selectable *sel = list->front (&auto_count);
		if (sel) {
			const char *stock = getStatusStockIcon (*sel);
			GdkPixbuf *pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(),
				stock, 16, GtkIconLookupFlags (0), NULL);
			GdkPixbuf *_pixbuf = YGUtils::setGray (pixbuf);
			gtk_image_set_from_pixbuf (GTK_IMAGE (icon), _pixbuf);
			g_object_unref (G_OBJECT (pixbuf));
			g_object_unref (G_OBJECT (_pixbuf));

			const char *action;
			if (sel->toInstall()) {
				action = _("install");
				if (sel->type() == Ypp::Selectable::PACKAGE) {
					if (sel->isInstalled()) {
						Ypp::Version candidate = sel->candidate(), installed = sel->installed();
						if (candidate > installed)
							action = _("upgrade");
						else if (candidate < installed)
							action = _("downgrade");
						else
							action = _("re-install");
					}
				}
			}
			else if (sel->toRemove())
				action = _("remove");
			else
				action = _("modify");  // generic for locked and so on

			const char *format;
			if (auto_count > 1)
				format = _("<b>%s</b> %s, plus %d dependencies");
			else if (auto_count == 1)
				format = _("<b>%s</b> %s, plus 1 dependency");
			else
				format = "<b>%s</b> %s";
			gchar *str = g_strdup_printf (format, action, sel->name().c_str(), auto_count);
			gtk_label_set_text (GTK_LABEL (text), str);
			gtk_widget_set_sensitive (text, TRUE);
			gtk_widget_set_sensitive (undo_button, TRUE);
			g_free (str);
		}
		else {
			gtk_image_clear (GTK_IMAGE (icon));
			gtk_label_set_text (GTK_LABEL (text), _("<i>No changes to perform</i>"));
			gtk_widget_set_sensitive (text, FALSE);
			gtk_widget_set_sensitive (undo_button, FALSE);
		}
		gtk_label_set_use_markup (GTK_LABEL (text), TRUE);
	}

	static gboolean more_link_cb (GtkLabel *label, gchar *uri, LastChange *pThis)
	{ YGPackageSelector::get()->popupChanges(); return TRUE; }

	static void undo_clicked_cb (GtkButton *button, LastChange *pThis)
	{
		Ypp::Selectable *sel = YGPackageSelector::get()->undoList()->front (NULL);
		if (sel) sel->undo();
	}
};

struct StatChange {
	GtkWidget *hbox, *to_install, *to_upgrade, *to_remove;

	GtkWidget *getWidget() { return hbox; }

	StatChange()
	{
		to_install = gtk_label_new ("");
		gtk_label_set_selectable (GTK_LABEL (to_install), TRUE);
		to_upgrade = gtk_label_new ("");
		gtk_label_set_selectable (GTK_LABEL (to_upgrade), TRUE);
		to_remove = gtk_label_new ("");
		gtk_label_set_selectable (GTK_LABEL (to_remove), TRUE);

		hbox = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (_("To install:")), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), to_install, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), gtk_vseparator_new(), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (_("To upgrade:")), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), to_upgrade, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), gtk_vseparator_new(), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (_("To remove:")), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), to_remove, FALSE, TRUE, 0);
	}

	void undoChanged (YGtkPkgUndoList *undo)
	{
		int toInstall = 0, toUpgrade = 0, toRemove = 0;
		Ypp::List list (undo->getList());
		for (int i = 0; i < list.size(); i++) {
			Ypp::Selectable &sel = list.get (i);
			if (sel.toInstall()) {
				if (sel.isInstalled())
					toUpgrade++;
				else
					toInstall++;
			}
			else if (sel.toRemove())
				toInstall++;
		}

		label_set_int (to_install, toInstall);
		label_set_int (to_upgrade, toUpgrade);
		label_set_int (to_remove, toRemove);
	}

	static void label_set_int (GtkWidget *label, int value)
	{
		gchar *str = g_strdup_printf ("%d", value);
		gtk_label_set_text (GTK_LABEL (label), str);
		g_free (str);
	}
};

typedef zypp::DiskUsageCounter::MountPoint    ZyppDu;
typedef zypp::DiskUsageCounter::MountPointSet ZyppDuSet;

static std::vector <std::string> getPartitionList()
{
	ZyppDuSet diskUsage = zypp::getZYpp()->diskUsage();
	std::vector <std::string> partitions;
	partitions.reserve (diskUsage.size());
	for (ZyppDuSet::iterator it = diskUsage.begin(); it != diskUsage.end(); it++) {
		const ZyppDu &point = *it;
		if (!point.readonly)
			partitions.push_back (point.dir);
	}
	std::sort (partitions.begin(), partitions.end());
	return partitions;
}

static const ZyppDu getPartition (const std::string &mount_point)
{
	ZyppDuSet diskUsage = zypp::getZYpp()->diskUsage();
	for (ZyppDuSet::iterator it = diskUsage.begin(); it != diskUsage.end(); it++) {
		const ZyppDu &point = *it;
		if (mount_point == point.dir)
			return point;
	}
	return *zypp::getZYpp()->diskUsage().begin();  // error
}

#define MIN_FREE_MB_WARN	400
#define MIN_PERCENT_WARN	90

struct DiskChange {
	GtkWidget *hbox, *combo, *text;

	GtkWidget *getWidget() { return hbox; }

	DiskChange (bool small)
	{
		GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
		std::vector <std::string> partitions = getPartitionList();
		int active = -1;
		for (unsigned int i = 0; i < partitions.size(); i++) {
			const std::string &part = partitions[i];
			if (part == "/usr" || part == "/usr/")
				active = i;
			else if (active == -1 && part == "/")
				active = i;

			GtkTreeIter iter;
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, part.c_str(), -1);
		}

		combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
		g_object_unref (G_OBJECT (store));
		setup_small_widget_style();
		gtk_widget_set_name (combo, "small-widget");
		gtk_combo_box_set_focus_on_click (GTK_COMBO_BOX (combo), FALSE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		if (small)
			g_object_set (G_OBJECT (renderer), "scale", .70, NULL);
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer, "text", 0, NULL);
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo), active);
		g_signal_connect (G_OBJECT (combo), "changed",
		                  G_CALLBACK (combo_changed_cb), this);

		text = gtk_label_new ("");
		YGUtils::setWidgetFont (text, PANGO_STYLE_ITALIC,
			PANGO_WEIGHT_NORMAL, PANGO_SCALE_MEDIUM);
		gtk_label_set_selectable (GTK_LABEL (text), TRUE);

		hbox = gtk_hbox_new (FALSE, 4);
		gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (_("Size available:")), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), text, FALSE, TRUE, 0);
	}

	void undoChanged (YGtkPkgUndoList *undo)
	{
		GtkTreeIter iter;
		gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);
		GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
		gchar *mount_point;
		gtk_tree_model_get (model, &iter, 0, &mount_point, -1);
		const ZyppDu part = getPartition (mount_point);
		g_free (mount_point);

		int percent = part.total_size ? ((100 * part.pkg_size) / part.total_size) : 0;
		long long free = (part.total_size - part.pkg_size) * 1024 * 1024;

		const char *format = "%s";
		if (percent > MIN_PERCENT_WARN && free < MIN_FREE_MB_WARN)
			format = "<span foreground=\"red\"><b>%s<b></span>";
		char *str = g_strdup_printf (format, part.freeAfterCommit().asString().c_str());
		gtk_label_set_markup (GTK_LABEL (text), str);
		g_free (str);
	}

	static void combo_changed_cb (GtkComboBox *combo, DiskChange *pThis)
	{ pThis->undoChanged (YGPackageSelector::get()->undoList()); }
};

struct YGtkPkgStatusBar::Impl : public YGtkPkgUndoList::Listener {
	GtkWidget *box;
	LastChange *last;
	StatChange *stat;
	DiskChange *disk;

	GtkWidget *getWidget() { return box; }

	Impl (YGtkPkgUndoList *undo, bool real_bar)
	{
		last = new LastChange();
		stat = new StatChange();
		disk = new DiskChange (real_bar);

		GtkWidget *hbox = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (hbox), last->getWidget(), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), gtk_vseparator_new(), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), stat->getWidget(), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), gtk_vseparator_new(), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), disk->getWidget(), FALSE, TRUE, 0);

		if (real_bar) {
			GtkWidget *status = gtk_statusbar_new();
			gtk_box_pack_start (GTK_BOX (hbox), status, FALSE, TRUE, 0);
			gtk_widget_set_size_request (status, 25, -1);
			gtk_widget_set_size_request (last->getWidget(), -1, 0);
			gtk_widget_set_size_request (stat->getWidget(), -1, 0);
			gtk_widget_set_size_request (disk->getWidget(), -1, 0);

			box = gtk_vbox_new (FALSE, 1);
			gtk_box_pack_start (GTK_BOX (box), gtk_hseparator_new(), FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (box), hbox, TRUE, TRUE, 0);
		}
		else
			box = hbox;
		gtk_widget_show_all (box);

		undoChanged (undo);
		undo->addListener (this);
	}

	~Impl()
	{
		delete last;
		delete stat;
		delete disk;
		YGPackageSelector::get()->undoList()->removeListener (this);
	}

	virtual void undoChanged (YGtkPkgUndoList *list)
	{
		last->undoChanged (list);
		stat->undoChanged (list);
		disk->undoChanged (list);
	}
};

YGtkPkgStatusBar::YGtkPkgStatusBar (YGtkPkgUndoList *undo, bool real_bar)
: impl (new Impl (undo, real_bar))
{}

YGtkPkgStatusBar::~YGtkPkgStatusBar()
{ delete impl; }

GtkWidget *YGtkPkgStatusBar::getWidget()
{ return impl->getWidget(); }

