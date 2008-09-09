/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkWizard widget */
// check the header file for information about this widget

/*
  Textdomain "yast2-gtk"
 */

#include <config.h>
#include "ygtkwizard.h"
#include <atk/atk.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "ygtkhtmlwrap.h"
#include "ygtksteps.h"
#include "ygtkfindentry.h"
#include "ygtklinklabel.h"
#define YGI18N_C
#include "YGi18n.h"

// YGUtils bridge
extern void ygutils_setWidgetFont (GtkWidget *widget, PangoWeight weight,
                                    double scale);
extern gboolean ygutils_setStockIcon (const char *label, GtkWidget *button);

//** YGtkHelpDialog

G_DEFINE_TYPE (YGtkHelpDialog, ygtk_help_dialog, GTK_TYPE_WINDOW)

// callbacks
static void ygtk_help_dialog_find_next (YGtkHelpDialog *dialog)
{
	const gchar *text = gtk_entry_get_text (GTK_ENTRY (dialog->search_entry));
	ygtk_html_wrap_search_next (dialog->help_text, text);
}

static void search_entry_modified_cb (GtkEditable *editable, YGtkHelpDialog *dialog)
{
	gchar *key = gtk_editable_get_chars (editable, 0, -1);
	if (!ygtk_html_wrap_search (dialog->help_text, key)) {
		GdkColor red = { 0, 255 << 8, 102 << 8, 102 << 8 },
		         white = { 0, 255 << 8, 255 << 8, 255 << 8 };
		gtk_widget_modify_base (dialog->search_entry, GTK_STATE_NORMAL, &red);
		gtk_widget_modify_text (dialog->search_entry, GTK_STATE_NORMAL, &white);
		gtk_widget_error_bell (GTK_WIDGET (dialog));
	}
	else {
		gtk_widget_modify_base (dialog->search_entry, GTK_STATE_NORMAL, NULL);
		gtk_widget_modify_text (dialog->search_entry, GTK_STATE_NORMAL, NULL);
	}
	g_free (key);
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
	gtk_window_set_default_size (GTK_WINDOW (dialog), 400, 350);

	// help text
	dialog->help_box = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy  (GTK_SCROLLED_WINDOW (dialog->help_box),
	                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (dialog->help_box),
	                                     GTK_SHADOW_IN);
	dialog->help_text = ygtk_html_wrap_new();
	gtk_container_add (GTK_CONTAINER (dialog->help_box), dialog->help_text);

	// bottom part (search entry + close button)
	GtkWidget *bottom_box;
	bottom_box = gtk_hbox_new (FALSE, 0);
	dialog->search_entry = ygtk_find_entry_new();
	gtk_widget_set_size_request (dialog->search_entry, 140, -1);
	dialog->close_button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	GTK_WIDGET_SET_FLAGS (dialog->close_button, GTK_CAN_DEFAULT);

	gtk_box_pack_start (GTK_BOX (bottom_box), dialog->search_entry, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (bottom_box), dialog->close_button, FALSE, FALSE, 0);

	g_signal_connect (G_OBJECT (dialog->search_entry), "changed",
	                  G_CALLBACK (search_entry_modified_cb), dialog);
	g_signal_connect (G_OBJECT (dialog->search_entry), "activate",
	                  G_CALLBACK (search_entry_activated_cb), dialog);

	// glue it
	dialog->vbox = gtk_vbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (dialog->vbox), dialog->help_box, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (dialog->vbox), bottom_box, FALSE, FALSE, 0);
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
	gtk_widget_grab_focus (dialog->close_button);
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
	gtk_binding_entry_add_signal (binding_set, GDK_F3, 0, "find_next", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_Escape, 0, "close", 0);
}

//** Header

typedef struct _YGtkWizardHeader
{
	GtkEventBox box;
	// members:
	GtkWidget *title, *description, *icon, *description_more;
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

	header->title = gtk_label_new (NULL);
	gtk_label_set_ellipsize (GTK_LABEL (header->title), PANGO_ELLIPSIZE_END);
	gtk_misc_set_alignment (GTK_MISC (header->title), 0, 0.5);
	ygutils_setWidgetFont (header->title, PANGO_WEIGHT_BOLD, PANGO_SCALE_XX_LARGE);

	header->description = ygtk_link_label_new ("", _("more"));
	g_signal_connect (G_OBJECT (header->description), "link-clicked",
	                  G_CALLBACK (description_link_clicked_cb), header);

	header->icon = gtk_image_new();

	GtkWidget *text_box = gtk_vbox_new (FALSE, 1);
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

GtkWidget *ygtk_wizard_header_new()
{ return g_object_new (YGTK_TYPE_WIZARD_HEADER, NULL); }

static void ygtk_wizard_header_class_init (YGtkWizardHeaderClass *klass)
{
	more_clicked_signal = g_signal_new ("more-clicked",
		G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (YGtkWizardHeaderClass, more_clicked), NULL, NULL,
		g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void ygtk_wizard_header_set_title (YGtkWizardHeader *header, const gchar *text)
{ gtk_label_set_text (GTK_LABEL (header->title), text); }

static void ygtk_wizard_header_set_description (YGtkWizardHeader *header, const gchar *text)
{
	GString *str = g_string_new ("");
	int i;
	gboolean copy_word = FALSE;
	for (i = 0; text[i]; i++) {
		if (text[i] == '<') {
			int a = i;
			for (; text[i]; i++)
				if (text[i] == '>')
					break;
			if (!strncasecmp (text+a, "<h", 2) || !strncasecmp (text+a, "<big>", 5) ||
		        (!str->len && !strncasecmp (text+a, "<b>", 3))) {
				for (i++; text[i]; i++) {
					if (text[i] == '<')
						a = i;
					if (text[i] == '>') {
						if (!strncasecmp (text+a, "</h", 3) || !strncasecmp (text+a, "</big>", 6) ||
						    !strncasecmp (text+a, "</b>", 4))
							break;
					}
				}
			}
		}
		else if (g_ascii_isspace (text[i])) {
			if (copy_word)
				g_string_append_c (str, ' ');
			copy_word = FALSE;
		}
		else {
			copy_word = TRUE;
			if (text[i] == '&') {
				if (!strncasecmp (text+i, "&product;", 9)) {
					g_string_append (str, "Linux");
					i += 8;
				}
			}
			else {
				g_string_append_c (str, text[i]);
				if (text[i] == '.') {
					if (g_ascii_isspace (text[i+1]) || text[i+1] == '<') {
						i++;
						break;
					}
				}
			}
		}
	}
	gboolean cut = FALSE;
	for (; text[i]; i++) {
		if (!g_ascii_isspace (text[i])) {
			cut = TRUE;
			break;
		}
	}
	gchar *desc = g_string_free (str, FALSE);
	ygtk_link_label_set_text (YGTK_LINK_LABEL (header->description), desc, NULL, cut);
	g_free (desc);
}

static void ygtk_wizard_header_set_icon (YGtkWizardHeader *header, GdkPixbuf *pixbuf)
{ gtk_image_set_from_pixbuf (GTK_IMAGE (header->icon), pixbuf); }

static const gchar *ygtk_wizard_header_get_title (YGtkWizardHeader *header)
{ return gtk_label_get_text (GTK_LABEL (header->title)); }
static GdkPixbuf *ygtk_wizard_header_get_icon (YGtkWizardHeader *header)
{ return gtk_image_get_pixbuf (GTK_IMAGE (header->icon)); }

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
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (button_clicked_cb), wizard);
	return button;
}

static GtkWidget *create_help_button()
{
	GtkWidget *button, *image;
	image = gtk_image_new_from_stock (GTK_STOCK_HELP, GTK_ICON_SIZE_BUTTON);
	button = gtk_toggle_button_new();
	gtk_button_set_label (GTK_BUTTON (button), _("Help"));
	gtk_button_set_image (GTK_BUTTON (button), image);
	gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
	return button;
}

static void ygtk_wizard_popup_help (YGtkWizard *wizard);
static void help_button_toggled_cb (GtkToggleButton *button, YGtkWizard *wizard)
{
	if (gtk_toggle_button_get_active (button))
		ygtk_wizard_popup_help (wizard);
	else if (wizard->m_help_dialog)
		gtk_widget_hide (wizard->m_help_dialog);
}
static void help_button_silent_set_active (YGtkWizard *wizard, gboolean active)
{
	GtkToggleButton *button = GTK_TOGGLE_BUTTON (wizard->m_help_button);
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
	if (!wizard->m_help_dialog) {
		GtkWindow *window = (GtkWindow *) gtk_widget_get_ancestor (
		                        GTK_WIDGET (wizard), GTK_TYPE_WINDOW);
		wizard->m_help_dialog = ygtk_help_dialog_new (window);
		ygtk_help_dialog_set_text (YGTK_HELP_DIALOG (wizard->m_help_dialog),
		                           wizard->m_help);
		g_signal_connect (G_OBJECT (wizard->m_help_dialog), "unmap",
		                  G_CALLBACK (help_dialog_unmap_cb), wizard);
	}
	help_button_silent_set_active (wizard, TRUE);
	gtk_window_present (GTK_WINDOW (wizard->m_help_dialog));
}

static void more_clicked_cb (YGtkWizardHeader *header, YGtkWizard *wizard)
{ ygtk_wizard_popup_help (wizard); }

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
	wizard->m_next_button  = button_new (wizard);
	wizard->m_back_button  = button_new (wizard);
	wizard->m_abort_button = button_new (wizard);
	wizard->m_release_notes_button = button_new (wizard);
	wizard->m_help_button = create_help_button();
	g_signal_connect (G_OBJECT (wizard->m_help_button), "toggled",
	                  G_CALLBACK (help_button_toggled_cb), wizard);

	// we need to protect this button against insensitive in some cases
	// this is a flag to enable that
	ygtk_wizard_protect_next_button (wizard, FALSE);

	wizard->m_buttons = gtk_hbox_new (FALSE, 12);
	gtk_widget_show (wizard->m_buttons);
	gtk_box_pack_start (GTK_BOX (wizard->m_buttons), wizard->m_help_button, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (wizard->m_buttons), wizard->m_release_notes_button,
	                    FALSE, TRUE, 0);

	gtk_box_pack_end (GTK_BOX (wizard->m_buttons), wizard->m_next_button, FALSE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (wizard->m_buttons), wizard->m_back_button, FALSE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (wizard->m_buttons), wizard->m_abort_button, FALSE, TRUE, 0);

	// make buttons all having the same size
	GtkSizeGroup *buttons_group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
	gtk_size_group_add_widget (buttons_group, wizard->m_help_button);
	gtk_size_group_add_widget (buttons_group, wizard->m_release_notes_button);
	gtk_size_group_add_widget (buttons_group, wizard->m_next_button);
	gtk_size_group_add_widget (buttons_group, wizard->m_back_button);
	gtk_size_group_add_widget (buttons_group, wizard->m_abort_button);
	g_object_unref (G_OBJECT (buttons_group)); 


	//** The menu and the navigation widgets will be created when requested.
	// space for them
	wizard->m_menu_box = gtk_event_box_new();

	wizard->m_pane = gtk_hpaned_new();
	gtk_widget_show (wizard->m_pane);
	wizard->m_contents_box = gtk_hbox_new (FALSE, 6);
	GtkWidget *contents_align = gtk_alignment_new (0, 0, 1, 1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (contents_align), 6, 12, 0, 0);
	gtk_container_add (GTK_CONTAINER (contents_align), wizard->m_pane);
	gtk_box_pack_start (GTK_BOX (wizard->m_contents_box), contents_align, TRUE, TRUE, 0);
	gtk_widget_show_all (wizard->m_contents_box);

//	gtk_box_set_spacing (GTK_BOX (wizard), 6);
	gtk_box_pack_start (GTK_BOX (wizard), wizard->m_menu_box, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (wizard), wizard->m_title, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (wizard), wizard->m_contents_box, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (wizard), wizard->m_buttons, FALSE, TRUE, 6);
}

static void ygtk_wizard_realize (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (ygtk_wizard_parent_class)->realize (widget);
	YGtkWizard *wizard = YGTK_WIZARD (widget);
	gtk_widget_grab_default (wizard->m_next_button);
	gtk_widget_grab_focus (wizard->m_next_button);
}

static void destroy_hash (GHashTable **hash)
{
	if (*hash)
		g_hash_table_destroy (*hash);
	*hash = NULL;
}

static void ygtk_wizard_destroy (GtkObject *object)
{
	GTK_OBJECT_CLASS (ygtk_wizard_parent_class)->destroy (object);

	YGtkWizard *wizard = YGTK_WIZARD (object);
	destroy_hash (&wizard->menu_ids);
	destroy_hash (&wizard->tree_ids);
	destroy_hash (&wizard->steps_ids);

	if (wizard->m_help) {
		g_free (wizard->m_help);
		wizard->m_help = NULL;
	}
	if (wizard->m_help_dialog) {
		gtk_widget_destroy (wizard->m_help_dialog);
		wizard->m_help_dialog = NULL;
	}
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

void ygtk_wizard_set_child (YGtkWizard *wizard, GtkWidget *widget)
{
	wizard->m_child = widget;
	gtk_paned_pack2 (GTK_PANED (wizard->m_pane), widget, TRUE, TRUE);
}

static gboolean ygtk_wizard_set_information_expose_cb (GtkWidget *widget, GdkEventExpose *event,
                                                        GtkAllocation *alloc)
{
	cairo_t *cr = gdk_cairo_create (widget->window);
	int x = alloc->x, y = alloc->y, w = alloc->width, h = alloc->height;
	cairo_pattern_t *pattern = cairo_pattern_create_linear (x, y, x, y+h);
	cairo_pattern_add_color_stop_rgba (pattern, 0, 1, 1, 1, 1);
	cairo_pattern_add_color_stop_rgba (pattern, 1, 1, 1, 1, 0);
	cairo_set_source (cr, pattern);
	cairo_rectangle (cr, x, y, w, h);
	cairo_fill (cr);
	cairo_pattern_destroy (pattern);
	cairo_destroy (cr);
	return FALSE;
}

void ygtk_wizard_set_information_expose_hook (GtkWidget *widget, GtkAllocation *alloc)
{
	g_signal_connect (G_OBJECT (widget), "expose-event",
	                  G_CALLBACK (ygtk_wizard_set_information_expose_cb), alloc);
}

void ygtk_wizard_set_information_widget (YGtkWizard *wizard, GtkWidget *widget)
{
	gtk_box_pack_start (GTK_BOX (wizard->m_contents_box), widget, FALSE, TRUE, 0);
}

void ygtk_wizard_set_control_widget (YGtkWizard *wizard, GtkWidget *widget)
{
	gtk_paned_pack1 (GTK_PANED (wizard->m_pane), widget, FALSE, TRUE);
}

void ygtk_wizard_enable_steps (YGtkWizard *wizard)
{
	g_return_if_fail (wizard->m_steps == NULL);
	wizard->m_steps = ygtk_steps_new();
	gtk_widget_show (wizard->m_steps);
	ygtk_wizard_set_information_widget (wizard, wizard->m_steps);
	ygtk_wizard_set_information_expose_hook (wizard->m_steps, &wizard->m_steps->allocation);
}

void ygtk_wizard_enable_tree (YGtkWizard *wizard)
{
	g_return_if_fail (wizard->m_tree_view == NULL);

	wizard->m_tree_view = gtk_tree_view_new_with_model
		(GTK_TREE_MODEL (gtk_tree_store_new (1, G_TYPE_STRING)));
	GtkTreeView *view = GTK_TREE_VIEW (wizard->m_tree_view);
	gtk_tree_view_insert_column_with_attributes (view,
		0, "", gtk_cell_renderer_text_new(), "text", 0, NULL);
	gtk_tree_view_set_headers_visible (view, FALSE);
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (view), GTK_SELECTION_BROWSE);
	g_signal_connect (G_OBJECT (wizard->m_tree_view), "cursor-changed",
	                  G_CALLBACK (tree_item_selected_cb), wizard);
	// start by assuming it will be list, and set expanders when a tree is built
	gtk_tree_view_set_show_expanders (view, FALSE);

	GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
	                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
	                                     GTK_SHADOW_IN);

	gtk_container_add (GTK_CONTAINER (scroll), wizard->m_tree_view);
	gtk_widget_show_all (scroll);

	ygtk_wizard_set_control_widget (wizard, scroll);
	gtk_paned_set_position (GTK_PANED (wizard->m_pane), 180);
}

#define ENABLE_WIDGET(enable, widget) \
	(enable ? gtk_widget_show (widget) : gtk_widget_hide (widget))
#define ENABLE_WIDGET_STR(str, widget) \
	(str && str[0] ? gtk_widget_show (widget) : gtk_widget_hide (widget))

/* Commands */

void ygtk_wizard_set_help_text (YGtkWizard *wizard, const gchar *text)
{
	if (wizard->m_help)
		g_free (wizard->m_help);
	wizard->m_help = g_strdup (text);
	if (wizard->m_help_dialog)
		ygtk_help_dialog_set_text (YGTK_HELP_DIALOG (wizard->m_help_dialog), text);
	ygtk_wizard_header_set_description (YGTK_WIZARD_HEADER (wizard->m_title), text);
	ENABLE_WIDGET_STR (text, wizard->m_help_button);
}

gboolean ygtk_wizard_add_tree_item (YGtkWizard *wizard, const char *parent_id,
                                    const char *text, const char *id)
{
	GtkTreeView *view = GTK_TREE_VIEW (wizard->m_tree_view);
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

/* this is g_hash_table_remove_all in new glib */
static gboolean hash_table_clear_cb (gpointer key, gpointer value, gpointer data)
{
	return TRUE;
}
static void yg_hash_table_remove_all (GHashTable *hash_table)
{
	g_hash_table_foreach_remove (hash_table, hash_table_clear_cb, NULL);
}

void ygtk_wizard_clear_tree (YGtkWizard *wizard)
{
	GtkTreeView *tree = GTK_TREE_VIEW (wizard->m_tree_view);
	gtk_tree_store_clear (GTK_TREE_STORE (gtk_tree_view_get_model (tree)));
	yg_hash_table_remove_all (wizard->tree_ids);
}

gboolean ygtk_wizard_select_tree_item (YGtkWizard *wizard, const char *id)
{
	GtkTreePath *path = g_hash_table_lookup (wizard->tree_ids, id);
	if (path == NULL)
		return FALSE;

	g_signal_handlers_block_by_func (wizard->m_tree_view,
	                                 (gpointer) tree_item_selected_cb, wizard);

	GtkWidget *widget = wizard->m_tree_view;
	gtk_tree_view_expand_to_path (GTK_TREE_VIEW (widget), path);
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (widget), path,
	                          NULL, FALSE);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (widget), path, NULL,
	                              TRUE, 0.5, 0.5);

	g_signal_handlers_unblock_by_func (wizard->m_tree_view,
	                                   (gpointer) tree_item_selected_cb, wizard);
	return TRUE;
}

static void sync_window_title (YGtkWizard *wizard)
{
	GtkWidget *window = gtk_widget_get_toplevel (GTK_WIDGET (wizard));
	if (GTK_WIDGET_TOPLEVEL (window)) {
		const gchar *_title = ygtk_wizard_header_get_title (
			YGTK_WIZARD_HEADER (wizard->m_title));
		char *title;
		if (*_title == '\0')
			title = g_strdup ("YaST");
		else
			title = g_strdup_printf ("%s - YaST", _title);
		GdkPixbuf *pixbuf = ygtk_wizard_header_get_icon (
			YGTK_WIZARD_HEADER (wizard->m_title));

		gtk_window_set_title (GTK_WINDOW (window), title);
		gtk_window_set_icon (GTK_WINDOW (window), pixbuf);
		g_free (title);
	}
}

void ygtk_wizard_set_header_text (YGtkWizard *wizard, const char *text)
{
	ygtk_wizard_header_set_title (YGTK_WIZARD_HEADER (wizard->m_title), text);
	sync_window_title (wizard);
}

gboolean ygtk_wizard_set_header_icon (YGtkWizard *wizard, const char *icon)
{
	GError *error = 0;
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (icon, &error);
	if (!pixbuf)
		return FALSE;
	ygtk_wizard_header_set_icon (YGTK_WIZARD_HEADER (wizard->m_title), pixbuf);
	g_object_unref (G_OBJECT (pixbuf));
	sync_window_title (wizard);
	return TRUE;
}

static void set_stock_icon (GtkWidget *button, const gchar *label,
                             const gchar *stock_fallback)
{
	if (!ygutils_setStockIcon (label, button) && stock_fallback) {
		GtkWidget *image = gtk_image_new_from_stock (stock_fallback, GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image (GTK_BUTTON (button), image);
	}
}

void ygtk_wizard_set_abort_button_label (YGtkWizard *wizard, const char *text)
{
	gtk_button_set_label (GTK_BUTTON (wizard->m_abort_button), text);
	ENABLE_WIDGET_STR (text, wizard->m_abort_button);
	set_stock_icon (wizard->m_abort_button, text, GTK_STOCK_CANCEL);
}

void ygtk_wizard_set_back_button_label (YGtkWizard *wizard, const char *text)
{
	gtk_button_set_label (GTK_BUTTON (wizard->m_back_button), text);
	ENABLE_WIDGET_STR (text, wizard->m_back_button);
	set_stock_icon (wizard->m_back_button, text, NULL);
}

void ygtk_wizard_set_next_button_label (YGtkWizard *wizard, const char *text)
{
	gtk_button_set_label (GTK_BUTTON (wizard->m_next_button), text);
	ENABLE_WIDGET_STR (text, wizard->m_next_button);
	set_stock_icon (wizard->m_next_button, text, GTK_STOCK_APPLY);
}

void ygtk_wizard_set_release_notes_button_label (YGtkWizard *wizard, const gchar *text)
{
	gtk_button_set_label (GTK_BUTTON (wizard->m_release_notes_button), text);
	ENABLE_WIDGET_STR (text, wizard->m_release_notes_button);
}

void ygtk_wizard_set_back_button_ptr_id (YGtkWizard *wizard, gpointer id)
{
	g_object_set_data (G_OBJECT (wizard->m_back_button), "ptr-id", id);
}

void ygtk_wizard_set_next_button_ptr_id (YGtkWizard *wizard, gpointer id)
{
	g_object_set_data (G_OBJECT (wizard->m_next_button), "ptr-id", id);
}

void ygtk_wizard_set_abort_button_ptr_id (YGtkWizard *wizard, gpointer id)
{
	g_object_set_data (G_OBJECT (wizard->m_abort_button), "ptr-id", id);
}

void ygtk_wizard_set_release_notes_button_ptr_id (YGtkWizard *wizard, gpointer id)
{
	g_object_set_data (G_OBJECT (wizard->m_release_notes_button), "ptr-id", id);
}

void ygtk_wizard_set_back_button_str_id (YGtkWizard *wizard, const char *id)
{
	g_object_set_data_full (G_OBJECT (wizard->m_back_button), "str-id",
	                        g_strdup (id), g_free);
}

void ygtk_wizard_set_next_button_str_id (YGtkWizard *wizard, const char *id)
{
	g_object_set_data_full (G_OBJECT (wizard->m_next_button), "str-id",
	                        g_strdup (id), g_free);
}

void ygtk_wizard_set_abort_button_str_id (YGtkWizard *wizard, const char *id)
{
	g_object_set_data_full (G_OBJECT (wizard->m_abort_button), "str-id",
	                        g_strdup (id), g_free);
}

void ygtk_wizard_set_release_notes_button_str_id (YGtkWizard *wizard, const char *id)
{
	g_object_set_data_full (G_OBJECT (wizard->m_release_notes_button), "str-id",
	                        g_strdup (id), g_free);
}

void ygtk_wizard_show_release_notes_button (YGtkWizard *wizard, gboolean enable)
{
	ENABLE_WIDGET (enable, wizard->m_release_notes_button);
}

void ygtk_wizard_enable_back_button (YGtkWizard *wizard, gboolean enable)
{
	gtk_widget_set_sensitive (GTK_WIDGET (wizard->m_back_button), enable);
}

void ygtk_wizard_enable_next_button (YGtkWizard *wizard, gboolean enable)
{
	if (enable || !ygtk_wizard_is_next_button_protected (wizard))
		gtk_widget_set_sensitive (GTK_WIDGET (wizard->m_next_button), enable);
}

void ygtk_wizard_enable_abort_button (YGtkWizard *wizard, gboolean enable)
{
	gtk_widget_set_sensitive (GTK_WIDGET (wizard->m_abort_button), enable);
}

void ygtk_wizard_enable_release_notes_button (YGtkWizard *wizard, gboolean enable)
{
	gtk_widget_set_sensitive (GTK_WIDGET (wizard->m_release_notes_button), enable);
}

gboolean ygtk_wizard_is_next_button_protected (YGtkWizard *wizard)
{
	return GPOINTER_TO_INT (g_object_get_data (
	           G_OBJECT (wizard->m_next_button), "protect"));
}

void ygtk_wizard_protect_next_button (YGtkWizard *wizard, gboolean protect)
{
	g_object_set_data (G_OBJECT (wizard->m_abort_button), "protect",
	                   GINT_TO_POINTER (protect));
}

void ygtk_wizard_focus_next_button (YGtkWizard *wizard)
{
	gtk_widget_grab_focus (wizard->m_next_button);
}

void ygtk_wizard_focus_back_button (YGtkWizard *wizard)
{
	gtk_widget_grab_focus (wizard->m_back_button);
}

void ygtk_wizard_set_extra_button (YGtkWizard *wizard, GtkWidget *widget)
{
	gtk_box_pack_start (GTK_BOX (wizard->m_buttons), widget, FALSE, TRUE, 0);
}

void ygtk_wizard_add_menu (YGtkWizard *wizard, const char *text,
                           const char *id)
{
	if (!wizard->m_menu) {
		wizard->m_menu = gtk_menu_bar_new();
		gtk_container_add (GTK_CONTAINER (wizard->m_menu_box), wizard->m_menu);
		// we probably want to hide the title, so the menu is more visible
		gtk_widget_hide (wizard->m_title);
	}

	GtkWidget *entry = gtk_menu_item_new_with_mnemonic (text);
	gtk_menu_shell_append (GTK_MENU_SHELL (wizard->m_menu), entry);

	GtkWidget *submenu = gtk_menu_new();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (entry), submenu);

	g_hash_table_insert (wizard->menu_ids, g_strdup (id), submenu);
	gtk_widget_show_all (wizard->m_menu_box);
}

gboolean ygtk_wizard_add_menu_entry (YGtkWizard *wizard, const char *parent_id,
                                     const char *text, const char *id)
{
	GtkWidget *parent = g_hash_table_lookup (wizard->menu_ids, parent_id);
	if (!parent)
		return FALSE;

	GtkWidget *entry = gtk_menu_item_new_with_mnemonic (text);
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
	gtk_menu_shell_append (GTK_MENU_SHELL (parent), entry);

	GtkWidget *submenu = gtk_menu_new();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (entry), submenu);

	g_hash_table_insert (wizard->menu_ids, g_strdup (id), submenu);
	gtk_widget_show_all (entry);
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
	if (!wizard->m_menu)
		return;
	yg_hash_table_remove_all (wizard->menu_ids);
	GList *children = gtk_container_get_children (GTK_CONTAINER (wizard->m_menu)), *i;
	for (i = children; i; i = i->next) {
		GtkWidget *child = (GtkWidget *) i->data;
		gtk_container_remove (GTK_CONTAINER (wizard->m_menu), child);
	}
}

void ygtk_wizard_add_step_header (YGtkWizard *wizard, const char *text)
{
	g_return_if_fail (wizard->m_steps != NULL);
	ygtk_steps_append_heading (YGTK_STEPS (wizard->m_steps), text);
}

void ygtk_wizard_add_step (YGtkWizard *wizard, const char* text, const char *id)
{
	guint step_nb;
	g_return_if_fail (wizard->m_steps != NULL);
	step_nb = ygtk_steps_append (YGTK_STEPS (wizard->m_steps), text);
	g_hash_table_insert (wizard->steps_ids, g_strdup (id), GINT_TO_POINTER (step_nb));
}

gboolean ygtk_wizard_set_current_step (YGtkWizard *wizard, const char *id)
{
	gpointer step_nb = g_hash_table_lookup (wizard->steps_ids, id);
	if (!step_nb)
		return FALSE;
	ygtk_steps_set_current (YGTK_STEPS (wizard->m_steps), GPOINTER_TO_INT (step_nb));
	return TRUE;
}

void ygtk_wizard_clear_steps (YGtkWizard *wizard)
{
	ygtk_steps_clear (YGTK_STEPS (wizard->m_steps));
	yg_hash_table_remove_all (wizard->steps_ids);
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
	gtk_tree_view_get_cursor (GTK_TREE_VIEW (wizard->m_tree_view),
	                          &path, NULL);
	if (path == NULL)
		return NULL;

	/* A more elegant solution would be using a crossed hash table, but there
	   is none out of box, so we'll just iterate the hash table. */
	found_key = 0;
	g_hash_table_foreach (wizard->tree_ids, hash_lookup_tree_path_value, path);

	gtk_tree_path_free (path);
	return found_key;
}

void ygtk_wizard_set_sensitive (YGtkWizard *wizard, gboolean sensitive)
{
	// TODO: check if this chains through
	gtk_widget_set_sensitive (GTK_WIDGET (wizard), sensitive);

	if (ygtk_wizard_is_next_button_protected (wizard))
		gtk_widget_set_sensitive (wizard->m_next_button, TRUE);
}

static void ygtk_wizard_class_init (YGtkWizardClass *klass)
{
	ygtk_wizard_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->realize = ygtk_wizard_realize;

	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
	gtkobject_class->destroy = ygtk_wizard_destroy;

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
	gtk_binding_entry_add_signal (binding_set, GDK_F1, 0, "popup_help", 0);
}

