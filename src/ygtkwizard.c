/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkWizard widget */
// check the header file for information about this widget

/*
  Textdomain "gtk"
 */

#include <Libyui_config.h>
#include "ygtkwizard.h"
#include <atk/atk.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "ygtkhtmlwrap.h"
#include "ygtksteps.h"
#include "ygtklinklabel.h"
#define YGI18N_C
#include "YGi18n.h"

// YGUtils bridge
extern char *ygutils_mapKBAccel (const char *src);
extern void ygutils_setWidgetFont (GtkWidget *widget, PangoStyle style,
                                   PangoWeight weight, double scale);
extern void ygutils_setPaneRelPosition (GtkWidget *paned, gdouble rel);
extern const char *ygutils_mapStockIcon (const char *label);
extern const char *ygutils_setStockIcon (GtkWidget *button, const char *label,
                                      const char *fallbackIcon);
extern GdkPixbuf *ygutils_setOpacity (const GdkPixbuf *src, int opacity, gboolean alpha);
extern void ygdialog_setTitle (const gchar *title, gboolean sticky);
extern gchar *ygutils_headerize_help (const char *help_text, gboolean *cut);

//** YGtkHelpDialog

G_DEFINE_TYPE (YGtkHelpDialog, ygtk_help_dialog, GTK_TYPE_WINDOW)

// callbacks
static void ygtk_help_dialog_find_next (YGtkHelpDialog *dialog)
{
	const gchar *text = gtk_entry_get_text (GTK_ENTRY (dialog->search_entry));
	ygtk_html_wrap_search_next (dialog->help_text, text);
}

static void search_entry_changed_cb (GtkEditable *editable, YGtkHelpDialog *dialog)
{
	static GdkColor red = { 0, 255 << 8, 102 << 8, 102 << 8 };
	static GdkColor white = { 0, 0xffff, 0xffff, 0xffff };
	static GdkColor yellow = { 0, 0xf7f7, 0xf7f7, 0xbdbd };
	static GdkColor black = { 0, 0, 0, 0 };

	GtkWidget *widget = GTK_WIDGET (editable);
	GtkEntry *entry = GTK_ENTRY (editable);
	const gchar *text = gtk_entry_get_text (entry);
	gboolean found = ygtk_html_wrap_search (dialog->help_text, text);

	if (found && *text) {
		gtk_widget_modify_base (widget, GTK_STATE_NORMAL, &yellow);
		gtk_widget_modify_text (widget, GTK_STATE_NORMAL, &black);
	}
	else if (found) {  // revert
		gtk_widget_modify_base (widget, GTK_STATE_NORMAL, NULL);
		gtk_widget_modify_text (widget, GTK_STATE_NORMAL, NULL);
	}
	else {
		gtk_widget_modify_base (widget, GTK_STATE_NORMAL, &red);
		gtk_widget_modify_text (widget, GTK_STATE_NORMAL, &white);
		gtk_widget_error_bell (widget);
	}

	gboolean showIcon = *text;  // show clear icon if text
	if (showIcon != gtk_entry_get_icon_activatable (entry, GTK_ENTRY_ICON_SECONDARY)) {
		gtk_entry_set_icon_activatable (entry,
			GTK_ENTRY_ICON_SECONDARY, showIcon);
		gtk_entry_set_icon_from_stock (entry,
			GTK_ENTRY_ICON_SECONDARY, showIcon ? GTK_STOCK_CLEAR : NULL);
		if (showIcon)
			gtk_entry_set_icon_tooltip_text (entry,
				GTK_ENTRY_ICON_SECONDARY, _("Clear"));
	}
}

static void search_entry_icon_press_cb (GtkEntry *entry, GtkEntryIconPosition pos,
                                         GdkEvent *event, YGtkHelpDialog *dialog)
{
	if (pos == GTK_ENTRY_ICON_PRIMARY)
		gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
	else
		gtk_entry_set_text (entry, "");
	gtk_widget_grab_focus (GTK_WIDGET (entry));
}

static void search_entry_activated_cb (GtkEntry *entry, YGtkHelpDialog *dialog)
{ ygtk_help_dialog_find_next (dialog); }

static void close_button_clicked_cb (GtkButton *button, YGtkHelpDialog *dialog)
{ gtk_widget_hide (GTK_WIDGET (dialog)); }

static void ygtk_help_dialog_init (YGtkHelpDialog *dialog)
{
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
	gtk_window_set_type_hint (GTK_WINDOW (dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_title (GTK_WINDOW (dialog), _("Help"));
	GdkPixbuf *icon = gtk_widget_render_icon (
		GTK_WIDGET (dialog), GTK_STOCK_HELP, GTK_ICON_SIZE_MENU, NULL);
	gtk_window_set_icon (GTK_WINDOW (dialog), icon);
	g_object_unref (G_OBJECT (icon));
	gtk_window_set_default_size (GTK_WINDOW (dialog), 500, 450);

	// help text
	dialog->help_box = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy  (GTK_SCROLLED_WINDOW (dialog->help_box),
	                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (dialog->help_box),
	                                     GTK_SHADOW_IN);
	dialog->help_text = ygtk_html_wrap_new();
	gtk_container_add (GTK_CONTAINER (dialog->help_box), dialog->help_text);

#if 0  // show a nice background image
	GtkIconTheme *theme = gtk_icon_theme_get_default();
	GtkIconInfo *info = gtk_icon_theme_lookup_icon (theme, HELP_IMG_BG, 192, 0);
	if (info) {
		GdkPixbuf *pixbuf = gtk_icon_info_load_icon (info, NULL);
		if (pixbuf) {
			const gchar *filename = gtk_icon_info_get_filename (info);
			GdkPixbuf *transparent = ygutils_setOpacity (pixbuf, 60, FALSE);
			ygtk_html_wrap_set_background (dialog->help_text, transparent, filename);
			g_object_unref (pixbuf);
			g_object_unref (transparent);
		}
		gtk_icon_info_free (info);
	}
#endif

	// bottom part (search entry + close button)
	dialog->search_entry = gtk_entry_new();
	gtk_widget_set_size_request (dialog->search_entry, 140, -1);
	gtk_entry_set_icon_from_stock (GTK_ENTRY (dialog->search_entry),
		GTK_ENTRY_ICON_PRIMARY, GTK_STOCK_FIND);
	gtk_entry_set_icon_activatable (GTK_ENTRY (dialog->search_entry),
		GTK_ENTRY_ICON_PRIMARY, TRUE);
	g_signal_connect (G_OBJECT (dialog->search_entry), "icon-press",
	                  G_CALLBACK (search_entry_icon_press_cb), dialog);
	g_signal_connect (G_OBJECT (dialog->search_entry), "changed",
	                  G_CALLBACK (search_entry_changed_cb), dialog);
	g_signal_connect (G_OBJECT (dialog->search_entry), "activate",
	                  G_CALLBACK (search_entry_activated_cb), dialog);

	dialog->close_button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
        gtk_widget_set_can_default(dialog->close_button, TRUE);

	GtkWidget *close_box = gtk_hbutton_box_new();
	gtk_container_add (GTK_CONTAINER (close_box), dialog->close_button);

	char *label_str = ygutils_mapKBAccel (_("&Find:"));
	GtkWidget *bottom_box, *label = gtk_label_new_with_mnemonic (label_str);
	g_free (label_str);
	gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), dialog->search_entry);

	bottom_box = gtk_hbox_new (FALSE, 2);
	gtk_box_pack_start (GTK_BOX (bottom_box), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (bottom_box), dialog->search_entry, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (bottom_box), close_box, FALSE, FALSE, 0);

#ifdef SET_HELP_HISTORY
	dialog->history_combo = gtk_combo_box_new_text();
	GList *cells = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (dialog->history_combo));
	g_object_set (G_OBJECT (cells->data), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	g_list_free (cells);
#endif

	// glue it
	dialog->vbox = gtk_vbox_new (FALSE, 6);
#ifdef SET_HELP_HISTORY
	GtkWidget *hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), gtk_image_new_from_stock (GTK_STOCK_HELP, GTK_ICON_SIZE_BUTTON), FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), dialog->history_combo, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (dialog->vbox), hbox, FALSE, TRUE, 0);
#endif
	gtk_box_pack_start (GTK_BOX (dialog->vbox), dialog->help_box, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (dialog->vbox), bottom_box, FALSE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (dialog), dialog->vbox);
	gtk_widget_show_all (dialog->vbox);

	g_signal_connect (G_OBJECT (dialog->close_button), "clicked",
	                  G_CALLBACK (close_button_clicked_cb), dialog);
	g_signal_connect (G_OBJECT (dialog), "delete-event",
	                  G_CALLBACK (gtk_widget_hide_on_delete), NULL);
}

static void ygtk_help_dialog_realize (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (ygtk_help_dialog_parent_class)->realize (widget);
	YGtkHelpDialog *dialog = YGTK_HELP_DIALOG (widget);

	// set close as default widget
	gtk_widget_grab_default (dialog->close_button);
}

static void ygtk_help_dialog_close (YGtkHelpDialog *dialog)
{ gtk_widget_hide (GTK_WIDGET (dialog)); }

GtkWidget *ygtk_help_dialog_new (GtkWindow *parent)
{
	GtkWidget *dialog = g_object_new (YGTK_TYPE_HELP_DIALOG, NULL);
	if (parent)
		gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
	return dialog;
}

void ygtk_help_dialog_set_text (YGtkHelpDialog *dialog, const gchar *text)
{
	gtk_editable_delete_text (GTK_EDITABLE (dialog->search_entry), 0, -1);
	ygtk_html_wrap_set_text (dialog->help_text, text, FALSE);
	ygtk_html_wrap_scroll (dialog->help_text, TRUE);
}

static void ygtk_help_dialog_class_init (YGtkHelpDialogClass *klass)
{
	klass->find_next = ygtk_help_dialog_find_next;
	klass->close = ygtk_help_dialog_close;

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->realize = ygtk_help_dialog_realize;

	// key bindings (F3 for next word, Esc to close the window)
	g_signal_new ("find_next", G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)),
	              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
	              G_STRUCT_OFFSET (YGtkHelpDialogClass, find_next),
	              NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	g_signal_new ("close", G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)),
	              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
	              G_STRUCT_OFFSET (YGtkHelpDialogClass, close),
	              NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	GtkBindingSet *binding_set = gtk_binding_set_by_class (klass);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_F3, 0, "find_next", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Escape, 0, "close", 0);
}

#ifdef SET_HELP_HISTORY
typedef struct TitleTextPair {
	gchar *title, *text;
} TitleTextPair;
#endif

YGtkHelpText *ygtk_help_text_new (void)
{ return g_new0 (YGtkHelpText, 1); }

void ygtk_help_text_destroy (YGtkHelpText *help)
{
#ifdef SET_HELP_HISTORY
	if (help->history) {
		GList *i;
		for (i = help->history; i; i = i->next) {
			TitleTextPair *pair = i->data;
			g_free (pair->title);
			g_free (pair->text);
			g_free (pair);
		}
		g_list_free (help->history);
		help->history = 0;
	}
#else
	if (help->text) {
		g_free (help->text);
		help->text = 0;
	}
#endif
	if (help->dialog) {
		gtk_widget_destroy (help->dialog);
		help->dialog = 0;
	}
}

#ifdef SET_HELP_HISTORY
static gint compare_links (gconstpointer pa, gconstpointer pb)
{
	const TitleTextPair *a = pa, *b = pb;
	return strcmp (a->text, b->text);
}
#endif

void ygtk_help_text_set (YGtkHelpText *help, const gchar *title, const gchar *text)
{
	if (!*text) return;
#ifdef SET_HELP_HISTORY
	TitleTextPair *pair = g_new (TitleTextPair, 1);
	if (title)
		pair->title = g_strdup (title);
	else {
		gboolean in_tag = FALSE;
		GString *str = g_string_new ("");
		const gchar *i;
		for (i = text; *i; i++) {
			if (*i == '<')
				in_tag = TRUE;
			else if (*i == '>')
				in_tag = FALSE;
			else if (*i == '\n') {
				if (str->len)
					break;
			}
			else if (!in_tag)
				str = g_string_append_c (str, *i);
		}
		pair->title = g_string_free (str, FALSE);
	}
	pair->text = g_strdup (text);

	GList *i = g_list_find_custom (help->history, pair, (GCompareFunc) compare_links);
	if (i) {
		TitleTextPair *p = i->data;
		g_free (p->text);
		g_free (p->title);
		g_free (p);
		help->history = g_list_delete_link (help->history, i);
	}
	help->history = g_list_prepend (help->history, pair);
#else
	if (help->text)
		g_free (help->text);
	help->text = g_strdup (text);
#endif
	if (help->dialog)
		ygtk_help_text_sync (help, NULL);
}

const gchar *ygtk_help_text_get (YGtkHelpText *help, gint n)
{
#ifdef SET_HELP_HISTORY
	TitleTextPair *pair = g_list_nth_data (help->history, n);
	if (pair)
		return pair->text;
	return NULL;
#else
	return help->text;
#endif
}

#ifdef SET_HELP_HISTORY
static void history_changed_cb (GtkComboBox *combo, YGtkHelpText *text)
{
	YGtkHelpDialog *dialog = YGTK_HELP_DIALOG (text->dialog);
	gint active = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->history_combo));
	ygtk_help_dialog_set_text (dialog, ygtk_help_text_get (text, active));
}
#endif

void ygtk_help_text_sync (YGtkHelpText *help, GtkWidget *widget)
{
	YGtkHelpDialog *dialog;
	if (!help->dialog) {
		if (!widget)
			return;
#ifdef SET_HELP_HISTORY
		dialog = YGTK_HELP_DIALOG (widget);
		g_signal_connect (G_OBJECT (dialog->history_combo), "changed",
		                  G_CALLBACK (history_changed_cb), help);
#endif
		help->dialog = widget;
	}
	dialog = YGTK_HELP_DIALOG (help->dialog);
	ygtk_help_dialog_set_text (dialog, ygtk_help_text_get (help, 0));

#ifdef SET_HELP_HISTORY
	g_signal_handlers_block_by_func (dialog->history_combo, history_changed_cb, help);
	GtkListStore *store = GTK_LIST_STORE (gtk_combo_box_get_model (
		GTK_COMBO_BOX (dialog->history_combo)));
	gtk_list_store_clear (store);
	GList *i;
	for (i = help->history; i; i = i->next) {
		TitleTextPair *pair = i->data;
		gtk_combo_box_append_text (GTK_COMBO_BOX (dialog->history_combo), pair->title);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->history_combo), 0);
	g_signal_handlers_unblock_by_func (dialog->history_combo, history_changed_cb, help);
#endif
}

//** Header

typedef struct _YGtkWizardHeader
{
	GtkEventBox box;
	// members:
	GtkWidget *title, *description, *icon, *description_more;
	gint press_x, press_y;
} YGtkWizardHeader;

typedef struct _YGtkWizardHeaderClass
{
	GtkEventBoxClass parent_class;
	// signals:
	void (*more_clicked) (YGtkWizardHeader *header);
} YGtkWizardHeaderClass;

static guint more_clicked_signal;

#define YGTK_TYPE_WIZARD_HEADER            (ygtk_wizard_header_get_type ())
#define YGTK_WIZARD_HEADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                            YGTK_TYPE_WIZARD_HEADER, YGtkWizardHeader))
#define YGTK_WIZARD_HEADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                            YGTK_TYPE_WIZARD_HEADER, YGtkWizardHeaderClass))
#define YGTK_IS_WIZARD_HEADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                            YGTK_TYPE_WIZARD_HEADER))
#define YGTK_IS_WIZARD_HEADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                            YGTK_TYPE_WIZARD_HEADER))
#define YGTK_WIZARD_HEADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                            YGTK_TYPE_WIZARD_HEADER, YGtkWizardHeaderClass))

static GtkWidget *ygtk_wizard_header_new (void);
static GType ygtk_wizard_header_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (YGtkWizardHeader, ygtk_wizard_header, GTK_TYPE_EVENT_BOX)

static void description_link_clicked_cb (YGtkLinkLabel *label, YGtkWizardHeader *header)
{
	g_signal_emit (header, more_clicked_signal, 0, NULL);
}

static void ygtk_wizard_header_init (YGtkWizardHeader *header)
{
	GdkColor white = { 0, 0xffff, 0xffff, 0xffff };
	gtk_widget_modify_bg (GTK_WIDGET (header), GTK_STATE_NORMAL, &white);

	header->title = gtk_label_new ("");
	gtk_label_set_ellipsize (GTK_LABEL (header->title), PANGO_ELLIPSIZE_END);
	gtk_misc_set_alignment (GTK_MISC (header->title), 0, 0.5);
	ygutils_setWidgetFont (header->title, PANGO_STYLE_NORMAL, PANGO_WEIGHT_BOLD,
	                       PANGO_SCALE_X_LARGE);
	GdkColor black = { 0, 0, 0, 0 };  // set text to black cause of some style colors
	gtk_widget_modify_fg (header->title, GTK_STATE_NORMAL, &black);

	header->description = ygtk_link_label_new ("", _("more"));
	g_signal_connect (G_OBJECT (header->description), "link-clicked",
	                  G_CALLBACK (description_link_clicked_cb), header);
	gtk_widget_modify_fg (header->description, GTK_STATE_NORMAL, &black);

	header->icon = gtk_image_new();

	GtkWidget *text_box = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (text_box), header->title, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (text_box), header->description, FALSE, TRUE, 0);

	GtkWidget *title_box = gtk_hbox_new (FALSE, 10);
	gtk_box_pack_start (GTK_BOX (title_box), header->icon, FALSE, TRUE, 4);
	gtk_box_pack_start (GTK_BOX (title_box), text_box, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (title_box), 6);

	GtkWidget *box = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), title_box, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (box), gtk_hseparator_new(), FALSE, TRUE, 0);
	gtk_widget_show_all (box);
	gtk_container_add (GTK_CONTAINER (header), box);
}

static gboolean ygtk_wizard_header_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	if (event->button == 1) {
		GdkCursor *cursor = gdk_cursor_new (GDK_FLEUR);
		gdk_window_set_cursor (event->window, cursor);
		gdk_cursor_unref (cursor);

		YGtkWizardHeader *header = YGTK_WIZARD_HEADER (widget);
		header->press_x = event->x;
		header->press_y = event->y;
	}
	return TRUE;
}

static gboolean ygtk_wizard_header_button_release_event (GtkWidget *widget, GdkEventButton *event)
{
	if (event->button == 1)
		gdk_window_set_cursor (event->window, NULL);
	return TRUE;
}

static gboolean ygtk_wizard_header_motion_notify_event (GtkWidget *widget, GdkEventMotion *event)
{
	if (event->state & GDK_BUTTON1_MASK) {
		YGtkWizardHeader *header = YGTK_WIZARD_HEADER (widget);
		gint root_x, root_y, pointer_x, pointer_y;
		gdk_window_get_root_origin (event->window, &root_x, &root_y);
		gdk_window_get_pointer (event->window, &pointer_x, &pointer_y, NULL);

		gint x = pointer_x + root_x - header->press_x;
		gint y = pointer_y + root_y - header->press_y;

		GtkWidget *top_window = gtk_widget_get_toplevel (widget);
		gtk_window_move (GTK_WINDOW (top_window), x, y);
	}
	return TRUE;
}

GtkWidget *ygtk_wizard_header_new()
{ return g_object_new (YGTK_TYPE_WIZARD_HEADER, NULL); }

static void ygtk_wizard_header_class_init (YGtkWizardHeaderClass *klass)
{
	ygtk_wizard_header_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->button_press_event = ygtk_wizard_header_button_press_event;
	widget_class->button_release_event = ygtk_wizard_header_button_release_event;
	widget_class->motion_notify_event = ygtk_wizard_header_motion_notify_event;

	more_clicked_signal = g_signal_new ("more-clicked",
		G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (YGtkWizardHeaderClass, more_clicked), NULL, NULL,
		g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void ygtk_wizard_header_set_title (YGtkWizardHeader *header, const gchar *text)
{ gtk_label_set_text (GTK_LABEL (header->title), text); }

static const gchar *ygtk_wizard_header_get_title (YGtkWizardHeader *header)
{ return gtk_label_get_text (GTK_LABEL (header->title)); }

static void ygtk_wizard_header_set_description (YGtkWizardHeader *header, const gchar *text)
{
	gboolean cut = FALSE;
	gchar *desc = ygutils_headerize_help (text, &cut);
	ygtk_link_label_set_text (YGTK_LINK_LABEL (header->description), desc, NULL, cut);
	g_free (desc);
}

static void ygtk_wizard_header_set_icon (YGtkWizardHeader *header, GdkPixbuf *pixbuf)
{ gtk_image_set_from_pixbuf (GTK_IMAGE (header->icon), pixbuf); }

//** YGtkWizard

// callbacks
static void destroy_tree_path (gpointer data)
{
	GtkTreePath *path = data;
	gtk_tree_path_free (path);
}

// signals
static guint action_triggered_signal;

static void ygtk_marshal_VOID__POINTER_INT (GClosure *closure,
	GValue *return_value, guint n_param_values, const GValue *param_values,
	gpointer invocation_hint, gpointer marshal_data)
{
	typedef void (*GMarshalFunc_VOID__POINTER_INT) (gpointer data1, gpointer arg_1,
	                                                gint arg_2, gpointer data2);
	register GMarshalFunc_VOID__POINTER_INT callback;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;

	g_return_if_fail (n_param_values == 3);

	if (G_CCLOSURE_SWAP_DATA (closure)) {
		data1 = closure->data;
		data2 = g_value_peek_pointer (param_values + 0);
	}
	else {
		data1 = g_value_peek_pointer (param_values + 0);
		data2 = closure->data;
	}
	callback = (GMarshalFunc_VOID__POINTER_INT)
	               (marshal_data ? marshal_data : cc->callback);

	callback (data1, g_value_get_pointer (param_values + 1),
	                 g_value_get_int (param_values + 2), data2);
}

static void button_clicked_cb (GtkButton *button, YGtkWizard *wizard)
{
	gpointer id;
	id = g_object_get_data (G_OBJECT (button), "ptr-id");
	if (id)
		g_signal_emit (wizard, action_triggered_signal, 0, id, G_TYPE_POINTER);
	id = g_object_get_data (G_OBJECT (button), "str-id");
	if (id)
		g_signal_emit (wizard, action_triggered_signal, 0, id, G_TYPE_STRING);
}

static GtkWidget *button_new (YGtkWizard *wizard)
{
	GtkWidget *button = gtk_button_new_with_mnemonic ("");
        gtk_widget_set_can_default(button, TRUE);
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (button_clicked_cb), wizard);
	return button;
}

static GtkWidget *create_help_button()
{
	GtkWidget *button, *image;
	button = gtk_toggle_button_new();
	gtk_button_set_label (GTK_BUTTON (button), _("Help"));
	gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
	image = gtk_image_new_from_stock (GTK_STOCK_HELP, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image (GTK_BUTTON (button), image);
	return button;
}

static void ygtk_wizard_popup_help (YGtkWizard *wizard);
static void help_button_toggled_cb (GtkToggleButton *button, YGtkWizard *wizard)
{
	if (gtk_toggle_button_get_active (button))
		ygtk_wizard_popup_help (wizard);
	else if (wizard->m_help->dialog)
		gtk_widget_hide (wizard->m_help->dialog);
}
static void help_button_silent_set_active (YGtkWizard *wizard, gboolean active)
{
	if (!wizard->help_button) return;  // unmap may be issued at destroy
	GtkToggleButton *button = GTK_TOGGLE_BUTTON (wizard->help_button);
	g_signal_handlers_block_by_func (button,
		(gpointer) help_button_toggled_cb, wizard);
	gtk_toggle_button_set_active (button, active);
	g_signal_handlers_unblock_by_func (button,
		(gpointer) help_button_toggled_cb, wizard);
}
static void help_dialog_unmap_cb (GtkWidget *dialog, YGtkWizard *wizard)
{ help_button_silent_set_active (wizard, FALSE); }

static void ygtk_wizard_popup_help (YGtkWizard *wizard)
{
	if (!wizard->m_help->dialog) {
		GtkWindow *window = (GtkWindow *) gtk_widget_get_ancestor (
		                        GTK_WIDGET (wizard), GTK_TYPE_WINDOW);
		GtkWidget *dialog = ygtk_help_dialog_new (window);
		g_signal_connect (G_OBJECT (dialog), "unmap",
		                  G_CALLBACK (help_dialog_unmap_cb), wizard);
		ygtk_help_text_sync (wizard->m_help, dialog);
	}
	help_button_silent_set_active (wizard, TRUE);
	gtk_window_present (GTK_WINDOW (wizard->m_help->dialog));
}

static void more_clicked_cb (YGtkWizardHeader *header, YGtkWizard *wizard)
{ ygtk_wizard_popup_help (wizard); }

/* We must dishonor the size group if the space doesn't afford it. */

static void buttons_size_allocate_cb (GtkWidget *box, GtkAllocation *alloc,
                                       GtkSizeGroup *group)
{
	GSList *buttons = gtk_size_group_get_widgets (group), *i;
	int max_width = 0, total = 0;
	for (i = buttons; i; i = i->next) {
		if (!gtk_widget_get_visible (i->data))
			continue;
		GtkRequisition req;
		gtk_widget_get_child_requisition ((GtkWidget *) i->data, &req);
		max_width = MAX (max_width, req.width);
		total++;
	}
	int spacing = gtk_box_get_spacing (GTK_BOX (box));
	int width = max_width*total + (total ? spacing*(total-1) : 0);
	GtkSizeGroupMode new_mode = width > alloc->width ?
		GTK_SIZE_GROUP_VERTICAL : GTK_SIZE_GROUP_BOTH;
	if (gtk_size_group_get_mode (group) != new_mode)
		gtk_size_group_set_mode (group, new_mode);
}

G_DEFINE_TYPE (YGtkWizard, ygtk_wizard, GTK_TYPE_VBOX)

static void ygtk_wizard_init (YGtkWizard *wizard)
{
	wizard->menu_ids = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                          g_free, NULL);
	wizard->tree_ids = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                          g_free, destroy_tree_path);
	wizard->steps_ids = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                           g_free, NULL);

	//** Title
	wizard->m_title = ygtk_wizard_header_new();
	g_signal_connect (G_OBJECT (wizard->m_title), "more-clicked",
	                  G_CALLBACK (more_clicked_cb), wizard);
	gtk_widget_show_all (wizard->m_title);

	//** Adding the bottom buttons
	wizard->next_button  = button_new (wizard);
	wizard->back_button  = button_new (wizard);
	wizard->abort_button = button_new (wizard);
	wizard->release_notes_button = button_new (wizard);
	wizard->help_button = create_help_button();
	g_signal_connect (G_OBJECT (wizard->help_button), "toggled",
	                  G_CALLBACK (help_button_toggled_cb), wizard);

	wizard->m_buttons = gtk_hbutton_box_new();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (wizard->m_buttons), GTK_BUTTONBOX_END);
	gtk_box_set_spacing (GTK_BOX (wizard->m_buttons), 6);
	gtk_widget_show (wizard->m_buttons);
	gtk_box_pack_start (GTK_BOX (wizard->m_buttons), wizard->help_button, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (wizard->m_buttons), wizard->release_notes_button,
	                    FALSE, TRUE, 0);
	gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (wizard->m_buttons), wizard->help_button, TRUE);
	gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (wizard->m_buttons), wizard->release_notes_button, TRUE);

	gtk_box_pack_end (GTK_BOX (wizard->m_buttons), wizard->abort_button, FALSE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (wizard->m_buttons), wizard->back_button, FALSE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (wizard->m_buttons), wizard->next_button, FALSE, TRUE, 0);

	// make buttons all having the same size
	GtkSizeGroup *buttons_group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
	gtk_size_group_add_widget (buttons_group, wizard->help_button);
	gtk_size_group_add_widget (buttons_group, wizard->release_notes_button);
	gtk_size_group_add_widget (buttons_group, wizard->next_button);
	gtk_size_group_add_widget (buttons_group, wizard->back_button);
	gtk_size_group_add_widget (buttons_group, wizard->abort_button);
	g_object_unref (G_OBJECT (buttons_group));
	gtk_widget_set_size_request (wizard->m_buttons, 0, -1);
	g_signal_connect_after (G_OBJECT (wizard->m_buttons), "size-allocate",
	                        G_CALLBACK (buttons_size_allocate_cb), buttons_group);
	wizard->m_default_button = NULL;

	//** The menu and the navigation widgets will be created when requested.
	// space for them
	wizard->m_menu_box = gtk_event_box_new();
	wizard->m_info_box = gtk_event_box_new();
	wizard->m_status_box = gtk_event_box_new();

	wizard->m_pane = gtk_hpaned_new();
	gtk_widget_show (wizard->m_pane);

	wizard->m_contents_box = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (wizard->m_contents_box), wizard->m_info_box, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (wizard->m_contents_box), wizard->m_pane, TRUE, TRUE, 0);
	gtk_widget_show (wizard->m_contents_box);

	GtkWidget *vbox;
	vbox = gtk_vbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);  // content's border
	gtk_box_pack_start (GTK_BOX (vbox), wizard->m_contents_box, TRUE, TRUE, 0);
#if 0
	GtkWidget *hsep = gtk_hseparator_new();
	gtk_box_pack_start (GTK_BOX (vbox), hsep, FALSE, TRUE, 0);
	gtk_widget_show (hsep);
#endif
	gtk_box_pack_start (GTK_BOX (vbox), wizard->m_buttons, FALSE, TRUE, 0);
	gtk_widget_show (vbox);

	gtk_box_pack_start (GTK_BOX (wizard), wizard->m_menu_box, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (wizard), wizard->m_title, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (wizard), vbox, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (wizard), wizard->m_status_box, FALSE, TRUE, 0);
}

static void ygtk_wizard_realize (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (ygtk_wizard_parent_class)->realize (widget);
	YGtkWizard *wizard = YGTK_WIZARD (widget);
	if (wizard->m_default_button) {
		GtkWidget *window = gtk_widget_get_toplevel (widget);
		if (GTK_IS_WINDOW (window))
			if (!gtk_window_get_default_widget (GTK_WINDOW (window)))
				gtk_widget_grab_default (wizard->m_default_button);
	}
}

static void ygtk_wizard_map (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (ygtk_wizard_parent_class)->map (widget);
	// since wizards can swap the window, we need to update the title on map
	YGtkWizard *wizard = YGTK_WIZARD (widget);
	YGtkWizardHeader *header = YGTK_WIZARD_HEADER (wizard->m_title);
	const gchar *title = gtk_label_get_text (GTK_LABEL (header->title));
	ygdialog_setTitle (title, FALSE);
}

static gboolean clear_hash_cb (gpointer key, gpointer value, gpointer data)
{ return TRUE; }
static void clear_hash (GHashTable *hash_table)
{
	g_hash_table_foreach_remove (hash_table, clear_hash_cb, NULL);
}
static void destroy_hash (GHashTable **hash, gboolean is_tree)
{
	if (*hash)
		g_hash_table_destroy (*hash);
	*hash = NULL;
}

static void ygtk_wizard_finalize (GObject *object)
{
	YGtkWizard *wizard = YGTK_WIZARD (object);
	wizard->help_button = NULL;  // dialog unmap will try to access this
	destroy_hash (&wizard->menu_ids, FALSE);
	destroy_hash (&wizard->tree_ids, TRUE);
	destroy_hash (&wizard->steps_ids, FALSE);
	if (wizard->m_help) {
		ygtk_help_text_destroy (wizard->m_help);
		wizard->m_help = NULL;
	}
	G_OBJECT_CLASS (ygtk_wizard_parent_class)->finalize (object);
}

GtkWidget *ygtk_wizard_new (void)
{ return g_object_new (YGTK_TYPE_WIZARD, NULL); }

static void selected_menu_item_cb (GtkMenuItem *item, const char *id)
{
	YGtkWizard *wizard = g_object_get_data (G_OBJECT (item), "wizard");
	g_signal_emit (wizard, action_triggered_signal, 0, id, G_TYPE_STRING);
}

static void tree_item_selected_cb (GtkTreeView *tree_view, YGtkWizard *wizard)
{
	const gchar *id = ygtk_wizard_get_tree_selection (wizard);
	if (id)
		g_signal_emit (wizard, action_triggered_signal, 0, id, G_TYPE_STRING);
}

void ygtk_wizard_set_child (YGtkWizard *wizard, GtkWidget *child)
{
	if (wizard->m_child)
		gtk_container_remove (GTK_CONTAINER (wizard->m_pane), wizard->m_child);
	wizard->m_child = child;
	if (child)
		gtk_paned_pack2 (GTK_PANED (wizard->m_pane), child, TRUE, TRUE);
}

void ygtk_wizard_set_information_widget (YGtkWizard *wizard, GtkWidget *widget)
{
	GtkWidget *hbox = gtk_hbox_new (FALSE, 2), *sep = gtk_vseparator_new();
	gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), sep, FALSE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (wizard->m_info_box), hbox);
	gtk_widget_show_all (wizard->m_info_box);
}

void ygtk_wizard_set_control_widget (YGtkWizard *wizard, GtkWidget *widget)
{
	gtk_paned_pack1 (GTK_PANED (wizard->m_pane), widget, FALSE, TRUE);
}

void ygtk_wizard_enable_steps (YGtkWizard *wizard)
{
	g_return_if_fail (wizard->steps == NULL);
	wizard->steps = ygtk_steps_new();
	ygtk_wizard_set_information_widget (wizard, wizard->steps);
}

void ygtk_wizard_enable_tree (YGtkWizard *wizard)
{
	g_return_if_fail (wizard->tree_view == NULL);

	wizard->tree_view = gtk_tree_view_new_with_model
		(GTK_TREE_MODEL (gtk_tree_store_new (1, G_TYPE_STRING)));
	GtkTreeView *view = GTK_TREE_VIEW (wizard->tree_view);
	gtk_tree_view_insert_column_with_attributes (view,
		0, "", gtk_cell_renderer_text_new(), "text", 0, NULL);
	gtk_tree_view_set_headers_visible (view, FALSE);
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (view), GTK_SELECTION_BROWSE);
	g_signal_connect (G_OBJECT (wizard->tree_view), "cursor-changed",
	                  G_CALLBACK (tree_item_selected_cb), wizard);
	// start by assuming it will be list, and set expanders when a tree is built
	gtk_tree_view_set_show_expanders (view, FALSE);

	GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
	                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
	                                     GTK_SHADOW_IN);

	gtk_container_add (GTK_CONTAINER (scroll), wizard->tree_view);
	gtk_widget_show_all (scroll);

	ygtk_wizard_set_control_widget (wizard, scroll);
	ygutils_setPaneRelPosition (wizard->m_pane, .30);
}

#define ENABLE_WIDGET(enable, widget) \
	(enable ? gtk_widget_show (widget) : gtk_widget_hide (widget))
#define ENABLE_WIDGET_STR(str, widget) \
	(str && str[0] ? gtk_widget_show (widget) : gtk_widget_hide (widget))

/* Commands */

void ygtk_wizard_set_help_text (YGtkWizard *wizard, const gchar *text)
{
	if (!wizard->m_help)
		wizard->m_help = ygtk_help_text_new();
	const gchar *title = ygtk_wizard_header_get_title (YGTK_WIZARD_HEADER (wizard->m_title));
	ygtk_help_text_set (wizard->m_help, title, text);
/*	helpful for building out test.cc
	fprintf (stderr, "Help text:\n%s\n", text); */
	ygtk_wizard_header_set_description (YGTK_WIZARD_HEADER (wizard->m_title), text);
	ENABLE_WIDGET_STR (text, wizard->help_button);
}

gboolean ygtk_wizard_add_tree_item (YGtkWizard *wizard, const char *parent_id,
                                    const char *text, const char *id)
{
	GtkTreeView *view = GTK_TREE_VIEW (wizard->tree_view);
	GtkTreeModel *model = gtk_tree_view_get_model (view);
	GtkTreeIter iter;

	if (!parent_id || !*parent_id)
		gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
	else {
		GtkTreePath *path = g_hash_table_lookup (wizard->tree_ids, parent_id);
		if (path == NULL)
			return FALSE;
		gtk_tree_view_set_show_expanders (view, TRUE);  // has children
		GtkTreeIter parent_iter;
		gtk_tree_model_get_iter (model, &parent_iter, path);
		gtk_tree_store_append (GTK_TREE_STORE (model), &iter, &parent_iter);
	}

	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 0, text, -1);
	g_hash_table_insert (wizard->tree_ids, g_strdup (id),
	                     gtk_tree_model_get_path (model, &iter));
	return TRUE;
}

void ygtk_wizard_clear_tree (YGtkWizard *wizard)
{
	GtkTreeView *tree = GTK_TREE_VIEW (wizard->tree_view);
	gtk_tree_store_clear (GTK_TREE_STORE (gtk_tree_view_get_model (tree)));
	clear_hash (wizard->tree_ids);
}

gboolean ygtk_wizard_select_tree_item (YGtkWizard *wizard, const char *id)
{
	GtkTreePath *path = g_hash_table_lookup (wizard->tree_ids, id);
	if (path == NULL)
		return FALSE;

	g_signal_handlers_block_by_func (wizard->tree_view,
	                                 (gpointer) tree_item_selected_cb, wizard);

	GtkWidget *widget = wizard->tree_view;
	gtk_tree_view_expand_to_path (GTK_TREE_VIEW (widget), path);
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (widget), path,
	                          NULL, FALSE);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (widget), path, NULL,
	                              TRUE, 0.5, 0.5);

	g_signal_handlers_unblock_by_func (wizard->tree_view,
	                                   (gpointer) tree_item_selected_cb, wizard);
	return TRUE;
}

void ygtk_wizard_set_header_text (YGtkWizard *wizard, const char *text)
{
	if (*text)
		ygtk_wizard_header_set_title (YGTK_WIZARD_HEADER (wizard->m_title), text);
}

gboolean ygtk_wizard_set_header_icon (YGtkWizard *wizard, const char *icon)
{
	GError *error = 0;
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (icon, &error);
	if (!pixbuf)
		return FALSE;
	ygtk_wizard_header_set_icon (YGTK_WIZARD_HEADER (wizard->m_title), pixbuf);
	g_object_unref (G_OBJECT (pixbuf));
	return TRUE;
}

void ygtk_wizard_set_button_label (YGtkWizard *wizard, GtkWidget *button,
                                   const char *_label, const char *stock)
{
	const char *label = _label ? _label : "";
	gtk_button_set_label (GTK_BUTTON (button), label);
	ENABLE_WIDGET_STR (label, button);
	if (button == wizard->abort_button)
		stock = GTK_STOCK_CANCEL;
	else if (button == wizard->release_notes_button)
		stock = GTK_STOCK_EDIT;

	const char *_stock = ygutils_setStockIcon (button, label, stock);
	g_object_set_data (G_OBJECT (button), "icon-fallback", _stock ? 0 : GINT_TO_POINTER (1));
}

void ygtk_wizard_set_button_str_id (YGtkWizard *wizard, GtkWidget *button, const char *id)
{
	g_object_set_data_full (G_OBJECT (button), "str-id", g_strdup (id), g_free);
}

void ygtk_wizard_set_button_ptr_id (YGtkWizard *wizard, GtkWidget *button, gpointer id)
{
	g_object_set_data (G_OBJECT (button), "ptr-id", id);
}

void ygtk_wizard_set_default_button (YGtkWizard *wizard, GtkWidget *button)
{ wizard->m_default_button = button; }

void ygtk_wizard_enable_button (YGtkWizard *wizard, GtkWidget *button, gboolean enable)
{
	gtk_widget_set_sensitive (button, enable);
}

void ygtk_wizard_set_extra_button (YGtkWizard *wizard, GtkWidget *widget)
{
	gtk_box_pack_start (GTK_BOX (wizard->m_buttons), widget, FALSE, TRUE, 0);
}

void ygtk_wizard_add_menu (YGtkWizard *wizard, const char *text,
                           const char *id)
{
	if (!wizard->menu) {
		wizard->menu = gtk_menu_bar_new();
		ygtk_wizard_set_custom_menubar (wizard, wizard->menu, TRUE);
		gtk_widget_show (wizard->menu);
	}

	GtkWidget *entry = gtk_menu_item_new_with_mnemonic (text);
	GtkWidget *submenu = gtk_menu_new();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (entry), submenu);
	gtk_menu_shell_append (GTK_MENU_SHELL (wizard->menu), entry);
	gtk_widget_show_all (entry);

	g_hash_table_insert (wizard->menu_ids, g_strdup (id), submenu);
}

gboolean ygtk_wizard_add_menu_entry (YGtkWizard *wizard, const char *parent_id,
                                     const char *text, const char *id)
{
	GtkWidget *parent = g_hash_table_lookup (wizard->menu_ids, parent_id);
	if (!parent)
		return FALSE;

	GtkWidget *entry;
	const char *icon = ygutils_mapStockIcon (text);
	if (icon) {
		GtkWidget *image = gtk_image_new_from_stock (icon, GTK_ICON_SIZE_MENU);
		entry = gtk_image_menu_item_new_with_mnemonic (text);
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (entry), image);
	}
	else
		entry = gtk_menu_item_new_with_mnemonic (text);
	gtk_menu_shell_append (GTK_MENU_SHELL (parent), entry);
	gtk_widget_show (entry);

	// we need to get YGtkWizard to send signal
	g_object_set_data (G_OBJECT (entry), "wizard", wizard);
	g_signal_connect_data (G_OBJECT (entry), "activate",
	                       G_CALLBACK (selected_menu_item_cb), g_strdup (id),
	                       (GClosureNotify) g_free, 0);
	return TRUE;
}

gboolean ygtk_wizard_add_sub_menu (YGtkWizard *wizard, const char *parent_id,
                                   const char *text, const char *id)
{
	GtkWidget *parent = g_hash_table_lookup (wizard->menu_ids, parent_id);
	if (!parent)
		return FALSE;

	GtkWidget *entry = gtk_menu_item_new_with_mnemonic (text);
	GtkWidget *submenu = gtk_menu_new();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (entry), submenu);
	gtk_menu_shell_append (GTK_MENU_SHELL (parent), entry);
	gtk_widget_show_all (entry);

	g_hash_table_insert (wizard->menu_ids, g_strdup (id), submenu);
	return TRUE;
}

gboolean ygtk_wizard_add_menu_separator (YGtkWizard *wizard, const char *parent_id)
{
	GtkWidget *parent = g_hash_table_lookup (wizard->menu_ids, parent_id);
	if (!parent)
		return FALSE;

	GtkWidget *separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append (GTK_MENU_SHELL (parent), separator);
	gtk_widget_show (separator);
	return TRUE;
}

void ygtk_wizard_clear_menu (YGtkWizard *wizard)
{
	if (!wizard->menu)
		return;
	clear_hash (wizard->menu_ids);
	GList *children = gtk_container_get_children (GTK_CONTAINER (wizard->menu)), *i;
	for (i = children; i; i = i->next) {
		GtkWidget *child = (GtkWidget *) i->data;
		gtk_container_remove (GTK_CONTAINER (wizard->menu), child);
	}
}

void ygtk_wizard_set_custom_menubar (YGtkWizard *wizard, GtkWidget *menu_bar, gboolean hide_header)
{
	gtk_container_add (GTK_CONTAINER (wizard->m_menu_box), menu_bar);
	gtk_widget_show (wizard->m_menu_box);
	// we probably want to hide the title, so the menu is more visible
	if (hide_header)
		gtk_widget_hide (wizard->m_title);
}

void ygtk_wizard_set_status_bar (YGtkWizard *wizard, GtkWidget *status_bar)
{
	gtk_container_add (GTK_CONTAINER (wizard->m_status_box), status_bar);
	gtk_widget_show (wizard->m_status_box);
}

void ygtk_wizard_add_step_header (YGtkWizard *wizard, const char *text)
{
	g_return_if_fail (wizard->steps != NULL);
	ygtk_steps_append_heading (YGTK_STEPS (wizard->steps), text);
}

void ygtk_wizard_add_step (YGtkWizard *wizard, const char *text, const char *id)
{
	g_return_if_fail (wizard->steps != NULL);
	YGtkSteps *steps = YGTK_STEPS (wizard->steps);

	// append may be called for the same step a few times to mean that we
	// should consider it several steps, but present it only once
	gint step_nb, last_n = ygtk_steps_total (steps)-1;
	const gchar *last = ygtk_steps_get_nth_label (steps, last_n);
	if (last && !strcmp (last, text))
		step_nb = last_n;
	else
		step_nb = ygtk_steps_append (steps, text);
	g_hash_table_insert (wizard->steps_ids, g_strdup (id), GINT_TO_POINTER (step_nb));
}

gboolean ygtk_wizard_set_current_step (YGtkWizard *wizard, const char *id)
{
	if (*id) {
#if 0
		gpointer step_nb = g_hash_table_lookup (wizard->steps_ids, id);
		if (!step_nb)
			return FALSE;
		ygtk_steps_set_current (YGTK_STEPS (wizard->steps), GPOINTER_TO_INT (step_nb));
#else
		// can't use ordinary lookup because it returns '0' if not found
		// which is a valid step_nb
		gpointer step_nb, foo;
		if (!g_hash_table_lookup_extended (wizard->steps_ids, id, &foo, &step_nb))
			return FALSE;
		ygtk_steps_set_current (YGTK_STEPS (wizard->steps), GPOINTER_TO_INT (step_nb));
#endif
	}
	else
		ygtk_steps_set_current (YGTK_STEPS (wizard->steps), -1);
	return TRUE;
}

void ygtk_wizard_clear_steps (YGtkWizard *wizard)
{
	ygtk_steps_clear (YGTK_STEPS (wizard->steps));
	clear_hash (wizard->steps_ids);
}

static const gchar *found_key;
static void hash_lookup_tree_path_value (gpointer _key, gpointer _value,
                                         gpointer user_data)
{
	gchar *key = _key;
	GtkTreePath *value = _value;
	GtkTreePath *needle = user_data;

	if (gtk_tree_path_compare (value, needle) == 0)
		found_key = key;
}

const gchar *ygtk_wizard_get_tree_selection (YGtkWizard *wizard)
{
	GtkTreePath *path;
	gtk_tree_view_get_cursor (GTK_TREE_VIEW (wizard->tree_view), &path, NULL);
	if (path == NULL) return NULL;

	/* A more elegant solution would be using a crossed hash table, but there
	   is none out of box, so we'll just iterate the hash table. */
	found_key = 0;
	g_hash_table_foreach (wizard->tree_ids, hash_lookup_tree_path_value, path);

	gtk_tree_path_free (path);
	return found_key;
}

static void ygtk_wizard_class_init (YGtkWizardClass *klass)
{
	ygtk_wizard_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->realize = ygtk_wizard_realize;
	widget_class->map = ygtk_wizard_map;

	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = ygtk_wizard_finalize;

	action_triggered_signal = g_signal_new ("action-triggered",
		G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (YGtkWizardClass, action_triggered),
		NULL, NULL, ygtk_marshal_VOID__POINTER_INT, G_TYPE_NONE,
		2, G_TYPE_POINTER, G_TYPE_INT);

	// on F1, popup the help box
	klass->popup_help = ygtk_wizard_popup_help;
	g_signal_new ("popup_help", G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)),
	              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
	              G_STRUCT_OFFSET (YGtkWizardClass, popup_help),
	              NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	GtkBindingSet *binding_set = gtk_binding_set_by_class (klass);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_F1, 0, "popup_help", 0);
}

