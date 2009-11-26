/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkDiskView widget */
// check the header file for information about this widget

/*
  Textdomain "yast2-gtk"
 */
#define YUILogComponent "gtk"
#include "config.h"
#include "ygtkdiskview.h"
#include "YGi18n.h"
#include "YGUtils.h"
#include "YGDialog.h"
#include "ygtkmenubutton.h"

static GtkWidget *DiskList_new (GtkTreeModel *model, bool framed)
{
	bool foreign_model = model == NULL;
	if (model == NULL) {
		GtkListStore *store = gtk_list_store_new (6,
			// 0 - mount point, 1 - usage percent, 2 - usage string,
			// (highlight warns) 3 - font weight, 4 - font color string,
			// 5 - delta description
			G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING,
			G_TYPE_STRING);
		model = GTK_TREE_MODEL (store);
	}

	GtkWidget *view = gtk_tree_view_new_with_model (model), *scroll = view;
	if (foreign_model)
		g_object_unref (G_OBJECT (model));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)),
		                         GTK_SELECTION_NONE);

	GtkTreeViewColumn *column;
	column = gtk_tree_view_column_new_with_attributes (_("Mount Point"),
		gtk_cell_renderer_text_new(), "text", 0, "weight", 3, "foreground", 4, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
	column = gtk_tree_view_column_new_with_attributes (_("Usage"),
		gtk_cell_renderer_progress_new(), "value", 1, "text", 2, NULL);
	gtk_tree_view_column_set_min_width (column, 180);  // SIZE_REQUEST
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	g_object_set (G_OBJECT (renderer), "alignment", PANGO_ALIGN_RIGHT,
	              "style", PANGO_STYLE_ITALIC, NULL);
	column = gtk_tree_view_column_new_with_attributes ("Delta",
		renderer, "text", 5, NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

	if (framed) {
		scroll = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
			                                 GTK_SHADOW_IN);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				                        GTK_POLICY_NEVER, GTK_POLICY_NEVER);
		gtk_container_add (GTK_CONTAINER (scroll), view);
	}
	else
		g_object_set (view, "can-focus", FALSE, NULL);
	gtk_widget_show_all (scroll);
	return scroll;
}

static void DiskList_setup (GtkWidget *widget, int i, const std::string &path,
	int usage_percent, int delta_percent, const std::string &description,
	const std::string &changed, bool is_full)
{
	GtkTreeView *view = GTK_TREE_VIEW (widget);
	GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (view));
	if (i == 0)
		gtk_list_store_clear (store);
	GtkTreeIter iter;
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, path.c_str(), 1, usage_percent,
		                2, description.c_str(), 3, PANGO_WEIGHT_NORMAL, 4, NULL,
		                5, changed.c_str(), -1);
	if (is_full)
		gtk_list_store_set (store, &iter, 3, PANGO_WEIGHT_BOLD, 4, "red", -1);
}

GtkWidget *DiskView::getWidget()
{ return m_button; }

DiskView::DiskView ()
: m_hasWarn (false)
{
	m_button = ygtk_menu_button_new();
	gtk_widget_set_tooltip_text (m_button, _("Disk usage"));
	gtk_container_add (GTK_CONTAINER (m_button), gtk_image_new_from_pixbuf (NULL));

	m_view = DiskList_new (NULL, false);
	ygtk_menu_button_set_popup_align (YGTK_MENU_BUTTON (m_button), m_view, 0.0, 1.0);

	m_diskPixbuf = YGUtils::loadPixbuf (std::string (DATADIR) + "/harddisk.png");
	m_diskFullPixbuf = YGUtils::loadPixbuf (std::string (DATADIR) + "/harddisk-full.png");

	Ypp::get()->getDisk()->addListener (this);
	updateDisk();
}

DiskView::~DiskView()
{
	g_object_unref (m_diskPixbuf);
	g_object_unref (m_diskFullPixbuf);
}

void DiskView::updateDisk()
{
	#define MIN_PERCENT_WARN 90
	#define MIN_FREE_MB_WARN (500*1024)
	bool warn = false;
	Ypp::Disk *disk = Ypp::get()->getDisk();
	for (int i = 0; disk->getPartition (i); i++) {
		const Ypp::Disk::Partition *part = disk->getPartition (i);

		int usage_percent = (part->used * 100) / (part->total + 1);
		int delta_percent = (part->delta * 100) / (part->total + 1);

		bool full = usage_percent > MIN_PERCENT_WARN &&
		            (part->total - part->used) < MIN_FREE_MB_WARN;
		if (full) warn = true;
		std::string description (part->used_str + _(" of ") + part->total_str);
		std::string delta_str;
		if (part->delta) {
			delta_str = (part->delta > 0 ? "(+" : "(");
			delta_str += part->delta_str + ")";
		}
		DiskList_setup (m_view, i, part->path, usage_percent, delta_percent,
		                description, delta_str, full);
	}
	GdkPixbuf *pixbuf = m_diskPixbuf;
	if (warn) {
		pixbuf = m_diskFullPixbuf;
		popupWarning();
	}
	gtk_image_set_from_pixbuf (GTK_IMAGE (GTK_BIN (m_button)->child), pixbuf);
}

void DiskView::popupWarning()
{
	if (m_hasWarn) return;
	m_hasWarn = true;
	if (!GTK_WIDGET_REALIZED (getWidget())) return;

	GtkWidget *dialog, *view;
	dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
		GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING,
		GTK_BUTTONS_OK, "%s", _("Disk Almost Full !"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s",
		_("One of the partitions is reaching its limit of capacity. You may "
		"have to remove packages if you wish to install some."));

	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (m_view));
	view = DiskList_new (model, true);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), view);
	g_signal_connect (G_OBJECT (dialog), "response",
	                  G_CALLBACK (gtk_widget_destroy), this);
	gtk_widget_show (dialog);
}

