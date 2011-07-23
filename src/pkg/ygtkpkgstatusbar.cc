/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* YGtkPkgStatusBar, a status-bar */
// check the header file for information about this widget

/*
  Textdomain "gtk"
 */

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

struct LastChange {
	GtkWidget *hbox, *icon, *text, *undo_button;

	GtkWidget *getWidget() { return hbox; }

	LastChange()
	{
		icon = gtk_image_new();
		gtk_widget_set_size_request (icon, 16, 16);

		text = gtk_label_new ("");
		gtk_misc_set_alignment (GTK_MISC (text), 0, .5);
		undo_button = gtk_button_new_from_stock (GTK_STOCK_UNDO);
		YGUtils::shrinkWidget (undo_button);
		g_signal_connect (G_OBJECT (undo_button), "clicked",
		                  G_CALLBACK (undo_clicked_cb), this);
		gchar *str = g_strdup_printf ("(<a href=\"more\">%s</a>)", _("view all changes"));
		GtkWidget *more = gtk_label_new (str);
		g_free (str);
		gtk_label_set_use_markup (GTK_LABEL (more), TRUE);
		gtk_label_set_track_visited_links (GTK_LABEL (more), FALSE);
		g_signal_connect (G_OBJECT (more), "activate-link",
		                  G_CALLBACK (more_link_cb), this);

		hbox = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), text, FALSE, TRUE, 0);
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
			//GdkPixbuf *_pixbuf = YGUtils::setGray (pixbuf);
			gtk_image_set_from_pixbuf (GTK_IMAGE (icon), pixbuf);
			g_object_unref (G_OBJECT (pixbuf));

			const char *action = getStatusAction (sel);
			gchar *str;
			if (sel->toModifyAuto())
				str = g_strdup_printf (_("<b>%s</b> %d preselected packages"), action, auto_count);
			else {
				if (auto_count == 0)
					str = g_strdup_printf ("<b>%s</b> %s", action, sel->name().c_str());
				else {
					char *deps;
					if (auto_count == 1)
						deps = g_strdup (_("plus 1 dependency"));
					else
						deps = g_strdup_printf (_("plus %d dependencies"), auto_count);
					str = g_strdup_printf ("<b>%s</b> %s, %s", action, sel->name().c_str(), deps);
					g_free (deps);
				}
			}

			gtk_label_set_markup (GTK_LABEL (text), str);
			gtk_label_set_attributes (GTK_LABEL (text), NULL);
			gtk_widget_set_sensitive (undo_button, TRUE);
			g_free (str);
		}
		else {
			gtk_image_clear (GTK_IMAGE (icon));
			gtk_label_set_text (GTK_LABEL (text), _("No changes to perform"));

			PangoAttrList *attrs = pango_attr_list_new();
			pango_attr_list_insert (attrs, pango_attr_foreground_new (110<<8, 110<<8, 110<<8));
			pango_attr_list_insert (attrs, pango_attr_style_new (PANGO_STYLE_ITALIC));
			gtk_label_set_attributes (GTK_LABEL (text), attrs);
			pango_attr_list_unref (attrs);

			gtk_widget_set_sensitive (undo_button, FALSE);
		}
		set_ellipsize (text);
	}

	static gboolean more_link_cb (GtkLabel *label, gchar *uri, LastChange *pThis)
	{ YGPackageSelector::get()->popupChanges(); return TRUE; }

	static void undo_clicked_cb (GtkButton *button, LastChange *pThis)
	{
		Ypp::Selectable *sel = YGPackageSelector::get()->undoList()->front (NULL);
		if (sel) sel->undo();
	}

	void set_ellipsize (GtkWidget *label)
	{
		GtkWidget *hbox = gtk_widget_get_parent (this->hbox);
		if (gtk_widget_get_realized (hbox)) {
			gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_NONE);

			GtkRequisition req;
			gtk_widget_size_request (hbox, &req);
			GtkWidget *window = gtk_widget_get_toplevel (hbox);
                        GtkAllocation allocation;
                        gtk_widget_get_allocation(window, &allocation);

			bool ellipsize = req.width > allocation.width - 10;

			PangoEllipsizeMode mode = ellipsize ? PANGO_ELLIPSIZE_MIDDLE : PANGO_ELLIPSIZE_NONE;
			gtk_label_set_ellipsize (GTK_LABEL (label), mode);
			gtk_box_set_child_packing (GTK_BOX (this->hbox), label, ellipsize, TRUE, 0, GTK_PACK_START);
		}
	}
};

#if 0
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
#endif

#define MIN_FREE_MB_WARN	400
#define MIN_PERCENT_WARN	90

struct DiskChange {
	GtkWidget *hbox, *combo, *text;

	GtkWidget *getWidget() { return hbox; }

	DiskChange()
	{
		GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
		std::vector <std::string> partitions = Ypp::getPartitionList();
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
		YGUtils::shrinkWidget (combo);
		gtk_widget_set_name (combo, "small-widget");
		gtk_combo_box_set_focus_on_click (GTK_COMBO_BOX (combo), FALSE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
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
		gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (_("Space available:")), FALSE, TRUE, 0);
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
		const ZyppDu part = Ypp::getPartition (mount_point);
		g_free (mount_point);

		int percent = part.total_size ? ((100 * part.pkg_size) / part.total_size) : 0;
		long long free = (part.total_size - part.pkg_size) * 1024 * 1024;

		const char *format = "%s";
		if (percent > MIN_PERCENT_WARN && free < MIN_FREE_MB_WARN)
			format = "<b><span foreground=\"red\"><b>%s<b></span></b>";
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
	DiskChange *disk;

	GtkWidget *getWidget() { return box; }

	Impl (YGtkPkgUndoList *undo)
	{
		last = new LastChange();
		disk = new DiskChange();

		GtkWidget *hbox = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (hbox), last->getWidget(), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), disk->getWidget(), FALSE, TRUE, 0);
		box = hbox;
		gtk_widget_show_all (box);

		undoChanged (undo);
		undo->addListener (this);
	}

	~Impl()
	{
		delete last;
		delete disk;
		YGPackageSelector::get()->undoList()->removeListener (this);
	}

	virtual void undoChanged (YGtkPkgUndoList *list)
	{
		last->undoChanged (list);
		disk->undoChanged (list);
	}
};

YGtkPkgStatusBar::YGtkPkgStatusBar (YGtkPkgUndoList *undo)
: impl (new Impl (undo))
{}

YGtkPkgStatusBar::~YGtkPkgStatusBar()
{ delete impl; }

GtkWidget *YGtkPkgStatusBar::getWidget()
{ return impl->getWidget(); }

