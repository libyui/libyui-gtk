//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

/* YGtkWizard widget */
// check the header file for information about this widget

#include "ygtkwizard.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <string.h>
#include "ygtkrichtext.h"
#include "ygtksteps.h"

// YGUtils bridge
extern void ygutils_setWidgetFont (GtkWidget *widget, PangoWeight weight,
                                   double scale);

/** YGtkHelpDialog **/

/* XPM */
static const char *search_icon_xpm [] = {
	"20 14 2 1",
	" 	c None",
	".	c #DEDEDE",
	" ....         ...   ",
	" ...          ...   ",
	"...            ...  ",
	"...            ...  ",
	"...            ...  ",
	"...            ...  ",
	" ...          ...   ",
	" ....        ....   ",
	"  ....      .....   ",
	"  ......  .......   ",
	"   ...............  ",
	"     .............. ",
	"        ..    ......",
	"               ....."};

static void ygtk_help_dialog_class_init (YGtkHelpDialogClass *klass);
static void ygtk_help_dialog_init       (YGtkHelpDialog      *dialog);
static void ygtk_help_dialog_realize    (GtkWidget       *widget);
static gboolean ygtk_help_dialog_expose_event (GtkWidget      *widget,
                                               GdkEventExpose *event);

static void close_button_clicked_cb (GtkButton *button, YGtkHelpDialog *dialog);
static void search_entry_modified_cb (GtkEditable *editable, YGtkHelpDialog *dialog);
static gboolean help_dialog_key_pressed_cb (GtkWidget *widget, GdkEventKey *event,
                                            YGtkHelpDialog *dialog);
static gboolean search_entry_expose_cb (GtkWidget *widget, GdkEventExpose *event);

G_DEFINE_TYPE (YGtkHelpDialog, ygtk_help_dialog, GTK_TYPE_WINDOW)

void ygtk_help_dialog_class_init (YGtkHelpDialogClass *klass)
{
	ygtk_help_dialog_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->expose_event = ygtk_help_dialog_expose_event;
	widget_class->realize = ygtk_help_dialog_realize;
}

void ygtk_help_dialog_init (YGtkHelpDialog *dialog)
{
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
	gtk_window_set_type_hint (GTK_WINDOW (dialog), GDK_WINDOW_TYPE_HINT_DIALOG);

	GtkStockItem help_item;
	gtk_stock_lookup (GTK_STOCK_HELP, &help_item);

	gtk_window_set_title (GTK_WINDOW (dialog), "Help");
	gtk_window_set_default_size (GTK_WINDOW (dialog), 400, 350);

	// title
	dialog->title_box = gtk_hbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (dialog->title_box), 6);
	dialog->title_image = gtk_image_new_from_stock (GTK_STOCK_HELP,
	                                   GTK_ICON_SIZE_LARGE_TOOLBAR);
	dialog->title_label = gtk_label_new ("Help");
	gtk_widget_modify_fg (dialog->title_label, GTK_STATE_NORMAL,
	                      &dialog->title_label->style->fg [GTK_STATE_SELECTED]);
	ygutils_setWidgetFont (dialog->title_label, PANGO_WEIGHT_ULTRABOLD,
	                       PANGO_SCALE_LARGE);
	gtk_box_pack_start (GTK_BOX (dialog->title_box), dialog->title_image,
	                    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (dialog->title_box), dialog->title_label,
	                    FALSE, FALSE, 0);

	// help text
	dialog->help_box = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy  (GTK_SCROLLED_WINDOW (dialog->help_box),
	                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (dialog->help_box),
	                                     GTK_SHADOW_IN);
	dialog->help_text = ygtk_richtext_new();
	gtk_container_add (GTK_CONTAINER (dialog->help_box), dialog->help_text);

	// bottom part (search entry + close button)
	GtkWidget *bottom_box = gtk_hbox_new (FALSE, 0);
	GtkWidget *bottom_separator = gtk_label_new (NULL);
	dialog->search_entry = gtk_entry_new();
	dialog->close_button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	gtk_box_pack_start (GTK_BOX (bottom_box), dialog->search_entry, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (bottom_box), bottom_separator, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (bottom_box), dialog->close_button, FALSE, FALSE, 0);

	g_signal_connect (G_OBJECT (dialog->search_entry), "expose-event",
	                  G_CALLBACK (search_entry_expose_cb), dialog);
	g_signal_connect (G_OBJECT (dialog->search_entry), "changed",
	                  G_CALLBACK (search_entry_modified_cb), dialog);
	g_signal_connect (G_OBJECT (dialog->close_button), "clicked",
	                  G_CALLBACK (close_button_clicked_cb), dialog);

	gtk_widget_show_all (dialog->close_button);

	// glue it
	dialog->vbox = gtk_vbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (dialog->vbox), dialog->title_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (dialog->vbox), dialog->help_box, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (dialog->vbox), bottom_box, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (dialog), dialog->vbox);
	gtk_widget_show_all (dialog->vbox);

	g_signal_connect (G_OBJECT (dialog), "key-press-event",
	                  G_CALLBACK (help_dialog_key_pressed_cb), dialog);
}

void ygtk_help_dialog_realize (GtkWidget *widget)
{
printf ("help dialog realized\n");
	GTK_WIDGET_CLASS (ygtk_help_dialog_parent_class)->realize (widget);
	YGtkHelpDialog *dialog = YGTK_HELP_DIALOG (widget);

printf ("setting help text background\n");
	// set help text background
	gtk_widget_realize (dialog->help_text);
	ygtk_richtext_set_background (YGTK_RICHTEXT (dialog->help_text),
	                              THEMEDIR "/wizard/help-background.png");

	// set search entry background icon
#if 0
	gtk_widget_realize (dialog->search_entry);
	GdkWindow *window = dialog->search_entry->window;
	GdkPixmap *pixmap = gdk_pixmap_create_from_xpm_d (GDK_DRAWABLE (window),
	                        NULL, NULL, search_icon_xpm);
	gdk_window_set_back_pixmap (window, pixmap, FALSE);

/*

	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_xpm_data (search_icon_xpm);
	if (pixbuf) {
		GdkPixmap *pixmap;
		gdk_pixbuf_render_pixmap_and_mask_for_colormap (pixbuf,
			gdk_drawable_get_colormap (GDK_DRAWABLE (window)), &pixmap, NULL, 0);
		g_object_unref (G_OBJECT (pixbuf));
		gdk_window_set_back_pixmap (window, pixmap, FALSE);
	}
*/
#endif
	// set close as default widget
	GTK_WIDGET_SET_FLAGS (dialog->close_button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (dialog->close_button);
	gtk_widget_grab_focus (dialog->close_button);
}

GtkWidget *ygtk_help_dialog_new (GtkWindow *parent)
{
printf ("creating help dialog\n");
	GtkWidget *dialog = g_object_new (YGTK_TYPE_HELP_DIALOG, NULL);
	if (parent)
{
printf ("help dialog - parent is set\n");
		gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
}
	return dialog;
}

void ygtk_help_dialog_set_text (YGtkHelpDialog *dialog, const char *text)
{
	gtk_editable_delete_text (GTK_EDITABLE (dialog->search_entry), 0, -1);
	ygtk_richtext_set_text (YGTK_RICHTEXT (dialog->help_text), text, FALSE, FALSE);
}

gboolean ygtk_help_dialog_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
//printf ("expose\n");
	YGtkHelpDialog *dialog = YGTK_HELP_DIALOG (widget);

	// draw little gradient on title
	int x, y, w, h;
	x = dialog->title_box->allocation.x;
	y = dialog->title_box->allocation.y;
	w = dialog->title_box->allocation.width;
	h = dialog->title_box->allocation.height;
	if (x + w >= event->area.x && x <= event->area.x + event->area.width &&
	    y + h >= event->area.y && x <= event->area.y + event->area.height) {
		GdkColor *clr = &widget->style->bg [GTK_STATE_SELECTED];
		double r = clr->red / 65535.0;
		double g = clr->green / 65535.0;
		double b = clr->blue / 65535.0;

		cairo_t *cr = gdk_cairo_create (widget->window);
		cairo_pattern_t *pattern = cairo_pattern_create_linear (0, 0, w, 0);
		cairo_pattern_add_color_stop_rgba (pattern, 0, r, g, b, 1.0);
		cairo_pattern_add_color_stop_rgba (pattern, 1, r, g, b, 0.40);

		cairo_rectangle (cr, x, y, w, h);
		cairo_set_source (cr, pattern);
		cairo_fill (cr);

		cairo_pattern_destroy (pattern);
		cairo_destroy (cr);
	}

	gtk_container_propagate_expose (GTK_CONTAINER (dialog), dialog->vbox, event);
	return FALSE;
}

gboolean search_entry_expose_cb (GtkWidget *widget, GdkEventExpose *event)
{
#if 0
printf ("expose\n");
	//gdk_window_set_back_pixmap (widget->window, NULL, FALSE);

	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_xpm_data (search_icon_xpm);
if (!pixbuf)
printf ("pixbuf null!\n");

	int x, y, w, h;
	w = 20; h = 14;
	x = widget->allocation.x + widget->allocation.width - w;
	y = widget->allocation.y;
	cairo_t *cr = gdk_cairo_create (widget->window);
	gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
	cairo_rectangle (cr, x, y, w, h);
	cairo_fill (cr);

	g_object_unref (G_OBJECT (pixbuf));
	cairo_destroy (cr);
#endif
	return FALSE;
}

void close_button_clicked_cb (GtkButton *button, YGtkHelpDialog *dialog)
{
	gtk_widget_hide (GTK_WIDGET (dialog));
}

gboolean help_dialog_key_pressed_cb (GtkWidget *widget, GdkEventKey *event,
                                     YGtkHelpDialog *dialog)
{
printf ("help dialog - key pressed\n");
printf ("event->state: %d\n", event->state);
printf ("event->keyval: %d\n", event->keyval);
printf ("GDK_Escape: %d\n", GDK_Escape);
	if (event->keyval == GDK_Escape) {
printf ("esc pressed!\n");
		gtk_widget_hide (widget);
		return TRUE;
	}

	if (event->keyval == GDK_F3 || (GTK_WIDGET_HAS_FOCUS (dialog->search_entry) &&
	    event->keyval == GDK_Return)) {
		const gchar *text = gtk_entry_get_text (GTK_ENTRY (dialog->search_entry));
		ygtk_richtext_forward_mark (YGTK_RICHTEXT (dialog->help_text), text);
		return TRUE;
	}

	return FALSE;
}

void search_entry_modified_cb (GtkEditable *editable, YGtkHelpDialog *dialog)
{
	gchar *key = gtk_editable_get_chars (editable, 0, -1);
printf ("searching for %s\n", key);
	GdkColor background_clr = { 0, 255 << 8, 255 << 8, 255 << 8 };  // red, if not found
	if (!ygtk_richtext_mark_text (YGTK_RICHTEXT (dialog->help_text), key)) {
		background_clr.green = background_clr.blue = 0;
		gdk_beep();
	}
	gtk_widget_modify_base (dialog->search_entry, GTK_STATE_NORMAL, &background_clr);
	ygtk_richtext_forward_mark (YGTK_RICHTEXT (dialog->help_text), key);
	g_free (key);
}

/** YGtkWizard **/

static gint get_header_padding (GtkWidget *widget)
{
#if GTK_CHECK_VERSION(2,10,0)
	gint padding;
	gtk_widget_style_get (widget, "header-padding", &padding, NULL);
	return padding;
#else
	return 6;
#endif
}

static gint get_content_padding (GtkWidget *widget)
{
#if GTK_CHECK_VERSION(2,10,0)
	gint padding;
	gtk_widget_style_get (widget, "content-padding", &padding, NULL);
	return padding;
#else
	return 1;
#endif
}

static void ygtk_wizard_class_init (YGtkWizardClass *klass);
static void ygtk_wizard_init       (YGtkWizard      *wizard);
static void ygtk_wizard_destroy    (GtkObject       *object);
static void ygtk_wizard_realize    (GtkWidget       *widget);
static void ygtk_wizard_internal_size_request  (GtkWidget      *widget,
                                                GtkRequisition *requisition);
static void ygtk_wizard_size_allocate (GtkWidget      *widget,
                                       GtkAllocation  *allocation);
static gboolean ygtk_wizard_expose_event (GtkWidget *widget, GdkEventExpose *event);
static void ygtk_wizard_forall (GtkContainer *container, gboolean include_internals,
                                GtkCallback   callback,  gpointer callback_data);

// callbacks
static void button_clicked_cb (GtkButton *button, YGtkWizard *wizard);
static void help_button_clicked_cb (GtkButton *button, YGtkWizard *wizard);
static void selected_menu_item_cb (GtkMenuItem *item, const char* id);
static void tree_item_selected_cb (GtkTreeView *tree_view, YGtkWizard *wizard);
static void destroy_string (gpointer data)
{
	gchar *string = data;
	g_free (string);
}
static void destroy_tree_path (gpointer data)
{
	GtkTreePath *path = data;
	gtk_tree_path_free (path);
}
static void free_data (gpointer data, GClosure *closure)
{ g_free (data); }

// signals
static guint action_triggered_signal;
// marshal
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

G_DEFINE_TYPE (YGtkWizard, ygtk_wizard, GTK_TYPE_BIN)

static void ygtk_wizard_class_init (YGtkWizardClass *klass)
{
	ygtk_wizard_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	//** Get expose, so we can draw line border
	widget_class->expose_event = ygtk_wizard_expose_event;
	widget_class->realize = ygtk_wizard_realize;
	widget_class->size_request  = ygtk_wizard_internal_size_request;
	widget_class->size_allocate = ygtk_wizard_size_allocate;

	GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);
	container_class->forall = ygtk_wizard_forall;

	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
	gtkobject_class->destroy = ygtk_wizard_destroy;

	action_triggered_signal = g_signal_new ("action-triggered",
		G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (YGtkWizardClass, action_triggered),
		NULL, NULL, ygtk_marshal_VOID__POINTER_INT, G_TYPE_NONE,
		2, G_TYPE_POINTER, G_TYPE_INT);
}

static void ygtk_wizard_init (YGtkWizard *wizard)
{
	wizard->menu_ids = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                          destroy_string, NULL);
	wizard->tree_ids = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                          destroy_string, destroy_tree_path);
	wizard->steps_ids = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                           destroy_string, NULL);

	gtk_container_set_border_width (GTK_CONTAINER (wizard), 6);

	//** Title
printf ("creating the title\n");
	wizard->m_title = gtk_hbox_new (FALSE, 0);

	wizard->m_title_image = gtk_image_new();
	wizard->m_title_label = gtk_label_new("");

	// setup label look
	gtk_widget_modify_fg (wizard->m_title_label, GTK_STATE_NORMAL,
	                      &wizard->m_title_label->style->fg [GTK_STATE_SELECTED]);
	// set a strong font to the heading label
	ygutils_setWidgetFont (wizard->m_title_label, PANGO_WEIGHT_ULTRABOLD,
	                       PANGO_SCALE_XX_LARGE);

	gtk_box_pack_start (GTK_BOX (wizard->m_title), wizard->m_title_label,
	                    FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (wizard->m_title), wizard->m_title_image,
	                  FALSE, FALSE, 0);

	gtk_widget_set_parent (wizard->m_title, GTK_WIDGET (wizard));
	gtk_widget_show_all (wizard->m_title);

	//** Adding the bottom buttons
printf ("creating the bottom buttons\n");
	wizard->m_next_button  = gtk_button_new_with_mnemonic ("");
	wizard->m_back_button  = gtk_button_new_with_mnemonic ("");
	wizard->m_abort_button = gtk_button_new_with_mnemonic ("");
	wizard->m_help_button  = gtk_button_new_from_stock (GTK_STOCK_HELP);
	wizard->m_release_notes_button = gtk_button_new_with_mnemonic ("");

	// we need to protect this button against insensitive in some cases
	// this is a flag to enable that
	ygtk_wizard_protect_next_button (wizard, FALSE);

	g_signal_connect (G_OBJECT (wizard->m_back_button), "clicked",
	                  G_CALLBACK (button_clicked_cb), wizard);
	g_signal_connect (G_OBJECT (wizard->m_abort_button), "clicked",
	                  G_CALLBACK (button_clicked_cb), wizard);
	g_signal_connect (G_OBJECT (wizard->m_next_button), "clicked",
	                  G_CALLBACK (button_clicked_cb), wizard);
	g_signal_connect (G_OBJECT (wizard->m_help_button), "clicked",
	                  G_CALLBACK (help_button_clicked_cb), wizard);
	g_signal_connect (G_OBJECT (wizard->m_release_notes_button), "clicked",
	                  G_CALLBACK (button_clicked_cb), wizard);

	wizard->m_buttons = gtk_hbox_new (FALSE, 12);

	GtkWidget *spacer = gtk_label_new(NULL);
	gtk_widget_show (spacer);

	gtk_box_pack_start (GTK_BOX (wizard->m_buttons), wizard->m_help_button, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (wizard->m_buttons), wizard->m_release_notes_button,
	                    FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (wizard->m_buttons), spacer, TRUE, TRUE, 0);  // a spacer
	gtk_box_pack_end (GTK_BOX (wizard->m_buttons), wizard->m_next_button, FALSE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (wizard->m_buttons), wizard->m_back_button, FALSE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (wizard->m_buttons), wizard->m_abort_button, FALSE, TRUE, 0);

	gtk_widget_set_parent (wizard->m_buttons, GTK_WIDGET (wizard));
	gtk_widget_show (wizard->m_buttons);

	// make buttons all having the same size
	GtkSizeGroup *buttons_group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
	gtk_size_group_add_widget (buttons_group, wizard->m_help_button);
	gtk_size_group_add_widget (buttons_group, wizard->m_release_notes_button);
	gtk_size_group_add_widget (buttons_group, wizard->m_next_button);
	gtk_size_group_add_widget (buttons_group, wizard->m_back_button);
	gtk_size_group_add_widget (buttons_group, wizard->m_abort_button);
	g_object_unref (G_OBJECT (buttons_group));  // it is already ref by widgets

	//** The menu and the navigation widget will be created when requested.
	//** Help dialog will be build on realize so we can give it a parent window.
}

void ygtk_wizard_realize (GtkWidget *widget)
{
	YGtkWizard *wizard = YGTK_WIZARD (widget);

printf ("trying to get the window parent of widget\n");
	GtkWidget *parent = widget;
	while (parent && !GTK_IS_WINDOW (parent)) {
printf ("getting parent\n");
		parent = parent->parent;
}
	if (parent) {
printf ("found window!\n");
		wizard->m_help_dialog = ygtk_help_dialog_new (GTK_WINDOW (parent));
}
	else
		wizard->m_help_dialog = ygtk_help_dialog_new (NULL);

	GTK_WIDGET_CLASS (ygtk_wizard_parent_class)->realize (widget);

	GTK_WIDGET_SET_FLAGS (wizard->m_next_button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (wizard->m_next_button);
	gtk_widget_grab_focus (wizard->m_next_button);
}

static void ygtk_wizard_destroy (GtkObject *object)
{
printf ("destroy wizard\n");
	YGtkWizard *wizard = YGTK_WIZARD (object);
	g_hash_table_destroy (wizard->menu_ids);
	g_hash_table_destroy (wizard->tree_ids);
	g_hash_table_destroy (wizard->steps_ids);

	gtk_widget_destroy (wizard->m_title);
	gtk_widget_destroy (wizard->m_buttons);
	if (wizard->m_menu)
		gtk_widget_destroy (wizard->m_menu);
	if (wizard->m_navigation)
		gtk_widget_destroy (wizard->m_navigation);

	if (wizard->m_help_dialog)
		gtk_widget_destroy (wizard->m_help_dialog);

printf ("padding destroy\n");
	GTK_OBJECT_CLASS (ygtk_wizard_parent_class)->destroy (object);
printf ("done destroying\n");
}

GtkWidget *ygtk_wizard_new()
{
	return g_object_new (YGTK_TYPE_WIZARD, NULL);
}

void ygtk_wizard_enable_steps (YGtkWizard *wizard)
{
	if (wizard->m_navigation) {
		g_error ("YGtkWizard: a tree or steps widgets have already been enabled.");
		return;
	}
	wizard->m_navigation_widget = ygtk_steps_new();
	wizard->m_navigation = wizard->m_navigation_widget;
	gtk_widget_modify_text (wizard->m_navigation_widget, GTK_STATE_NORMAL,
	                        &wizard->m_navigation_widget->style->fg [GTK_STATE_SELECTED]);

	gtk_widget_set_parent (wizard->m_navigation, GTK_WIDGET (wizard));
	gtk_widget_show_all (wizard->m_navigation);
}

void ygtk_wizard_enable_tree (YGtkWizard *wizard)
{
	if (wizard->m_navigation) {
		g_error ("YGtkWizard: a tree or steps widgets have already been enabled.");
		return;
	}

	wizard->m_navigation_widget = gtk_tree_view_new_with_model
		(GTK_TREE_MODEL (gtk_tree_store_new (1, G_TYPE_STRING)));
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (wizard->m_navigation_widget),
		0, "(no title)", gtk_cell_renderer_text_new(), "text", 0, NULL);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (wizard->m_navigation_widget), FALSE);
	g_signal_connect (G_OBJECT (wizard->m_navigation_widget), "cursor-changed",
	                  G_CALLBACK (tree_item_selected_cb), wizard);

	wizard->m_navigation = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (wizard->m_navigation),
	                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (wizard->m_navigation),
	                                     GTK_SHADOW_IN);

	gtk_container_add (GTK_CONTAINER (wizard->m_navigation), wizard->m_navigation_widget);
	gtk_widget_set_size_request (wizard->m_navigation, 180, -1);

	gtk_widget_set_parent (wizard->m_navigation, GTK_WIDGET (wizard));
	gtk_widget_show_all (wizard->m_navigation);
}

void ygtk_wizard_set_child (YGtkWizard *wizard, GtkWidget *new_child)
{
printf ("set child\n");
	GtkWidget *child = GTK_BIN (wizard)->child;
	if (child)
		gtk_container_remove (GTK_CONTAINER (wizard), child);
	if (new_child)
		gtk_container_add (GTK_CONTAINER (wizard), new_child);
}

static gboolean hash_table_clear_cb (gpointer key, gpointer value, gpointer data)
{ return TRUE; }
static void hash_table_clear (GHashTable *hash_table)
{ g_hash_table_foreach_remove (hash_table, hash_table_clear_cb, NULL); }

#define enable_widget(enable, widget) \
	(enable ? gtk_widget_show (widget) : gtk_widget_hide (widget))

/* Commands */

void ygtk_wizard_set_help_text (YGtkWizard *wizard, const char *text)
{
	ygtk_help_dialog_set_text (YGTK_HELP_DIALOG (wizard->m_help_dialog), text);
	enable_widget (strlen (text), wizard->m_help_button);
}

gboolean ygtk_wizard_add_tree_item (YGtkWizard *wizard, const char *parent_id,
                                    const char *text, const char *id)
{
	GtkTreeModel *model = gtk_tree_view_get_model
	                          (GTK_TREE_VIEW (wizard->m_navigation_widget));
	GtkTreeIter iter;

	if (!parent_id || !*parent_id)
		gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
	else {
		GtkTreePath *path = g_hash_table_lookup (wizard->tree_ids, parent_id);
		if (path == NULL)
			return FALSE;
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
	GtkTreeView *tree = GTK_TREE_VIEW (wizard->m_navigation_widget);
	gtk_tree_store_clear (GTK_TREE_STORE (gtk_tree_view_get_model (tree)));
	hash_table_clear (wizard->tree_ids);
}

gboolean ygtk_wizard_select_tree_item (YGtkWizard *wizard, const char *id)
{
	GtkTreePath *path = g_hash_table_lookup (wizard->tree_ids, id);
	if (path == NULL)
		return FALSE;

	g_signal_handlers_block_by_func (wizard->m_navigation_widget,
	                                 (gpointer) tree_item_selected_cb, wizard);

	gtk_tree_view_expand_to_path (GTK_TREE_VIEW (wizard->m_navigation_widget), path);
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (wizard->m_navigation_widget), path,
	                          NULL, FALSE);

	g_signal_handlers_unblock_by_func (wizard->m_navigation_widget,
	                                   (gpointer) tree_item_selected_cb, wizard);
	return TRUE;
}

void ygtk_wizard_set_header_text (YGtkWizard *wizard, GtkWindow *window,
                                  const char *text)
{
	gtk_label_set_text (GTK_LABEL (wizard->m_title_label), text);
	if (window) {
		char *title = g_strdup_printf ("YaST: %s", text);
		gtk_window_set_title (window, title);
		g_free (title);
	}
}

gboolean ygtk_wizard_set_header_icon (YGtkWizard *wizard, GtkWindow *window,
                                      const char *icon)
{
	GError *error = 0;
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (icon, &error);
	if (!pixbuf)
		return FALSE;

	gtk_image_set_from_pixbuf (GTK_IMAGE (wizard->m_title_image), pixbuf);
	if (window)
		gtk_window_set_icon (window, pixbuf);
	g_object_unref (G_OBJECT (pixbuf));
	return TRUE;
}

void ygtk_wizard_set_abort_button_label (YGtkWizard *wizard, const char *text)
{
	gtk_button_set_label (GTK_BUTTON (wizard->m_abort_button), text);
	enable_widget (text[0], wizard->m_abort_button);
}

void ygtk_wizard_set_back_button_label (YGtkWizard *wizard, const char *text)
{
	gtk_button_set_label (GTK_BUTTON (wizard->m_back_button), text);
	enable_widget (text[0], wizard->m_back_button);
}

void ygtk_wizard_set_next_button_label (YGtkWizard *wizard, const char *text)
{
	gtk_button_set_label (GTK_BUTTON (wizard->m_next_button), text);
	enable_widget (text[0], wizard->m_next_button);
}

void ygtk_wizard_set_back_button_id (YGtkWizard *wizard, gpointer id,
                                     GDestroyNotify destroy_cb)
{
	g_object_set_data_full (G_OBJECT (wizard->m_back_button), "id", id, destroy_cb);
}

void ygtk_wizard_set_next_button_id (YGtkWizard *wizard, gpointer id,
                                     GDestroyNotify destroy_cb)
{
	g_object_set_data_full (G_OBJECT (wizard->m_next_button), "id", id, destroy_cb);
}

void ygtk_wizard_set_abort_button_id (YGtkWizard *wizard, gpointer id,
                                      GDestroyNotify destroy_cb)
{
	g_object_set_data_full (G_OBJECT (wizard->m_abort_button), "id", id, destroy_cb);
}

void ygtk_wizard_set_release_notes_button_label (YGtkWizard *wizard,
                                     const gchar *text, gpointer id,
                                     GDestroyNotify destroy_cb)
{
	gtk_button_set_label (GTK_BUTTON (wizard->m_release_notes_button), text);
	g_object_set_data_full (G_OBJECT (wizard->m_release_notes_button), "id",
	                        id, destroy_cb);
	gtk_widget_show (wizard->m_release_notes_button);
}

void ygtk_wizard_show_release_notes_button (YGtkWizard *wizard, gboolean enable)
{
	enable_widget (enable, wizard->m_release_notes_button);
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

void ygtk_wizard_add_menu (YGtkWizard *wizard, const char *text,
                           const char *id)
{
	if (!wizard->m_menu) {
		wizard->m_menu = gtk_menu_bar_new();
		gtk_widget_set_parent (wizard->m_menu, GTK_WIDGET (wizard));
	}

	GtkWidget *entry = gtk_menu_item_new_with_mnemonic (text);
	gtk_menu_shell_append (GTK_MENU_SHELL (wizard->m_menu), entry);

	GtkWidget *submenu = gtk_menu_new();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (entry), submenu);

	g_hash_table_insert (wizard->menu_ids, g_strdup (id), submenu);
	gtk_widget_show_all (wizard->m_menu);
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
	                       free_data, 0);
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

void ygtk_wizard_add_step_header (YGtkWizard *wizard, const char *text)
{
	ygtk_steps_append_heading (YGTK_STEPS (wizard->m_navigation_widget), text);
}

void ygtk_wizard_add_step (YGtkWizard *wizard, const char* text, const char *id)
{
	guint step_nb = ygtk_steps_append (YGTK_STEPS (wizard->m_navigation_widget), text);
	g_hash_table_insert (wizard->steps_ids, g_strdup (id), GINT_TO_POINTER (step_nb));
}

gboolean ygtk_wizard_set_current_step (YGtkWizard *wizard, const char *id)
{
	gpointer step_nb = g_hash_table_lookup (wizard->steps_ids, id);
	if (!step_nb)
		return FALSE;
	ygtk_steps_set_current (YGTK_STEPS (wizard->m_navigation_widget),
	                        GPOINTER_TO_INT (step_nb));
	return TRUE;
}

void ygtk_wizard_clear_steps (YGtkWizard *wizard)
{
	ygtk_steps_clear (YGTK_STEPS (wizard->m_navigation_widget));
	hash_table_clear (wizard->steps_ids);
}

static const gchar *found_key;
static void hask_lookup_tree_path_value (gpointer _key, gpointer _value,
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
	GtkTreeViewColumn *column;
	gtk_tree_view_get_cursor (GTK_TREE_VIEW (wizard->m_navigation_widget),
	                          &path, &column);
	if (path == NULL || column == NULL)
		return NULL;

	/* A more elegant solution would be using a crossed hash table, but there
	   is none out of box, so we'll just iterate the hash table. */
	found_key = 0;
	g_hash_table_foreach (wizard->tree_ids, hask_lookup_tree_path_value, path);
	return found_key;
}

void ygtk_wizard_set_sensitive (YGtkWizard *wizard, gboolean sensitive)
{
	// FIXME: check if this chains through
	gtk_widget_set_sensitive (GTK_WIDGET (wizard), sensitive);

	if (ygtk_wizard_is_next_button_protected (wizard))
		gtk_widget_set_sensitive (wizard->m_next_button, TRUE);
}

void ygtk_wizard_size_request (YGtkWizard *wizard, GtkRequisition *requisition,
                               gboolean around_child)
{
printf ("getting size request...\n");
	requisition->width = requisition->height = 0;

	gint border = GTK_CONTAINER (wizard)->border_width;
	gint header_padding = get_header_padding (GTK_WIDGET (wizard));
	gint content_padding = get_content_padding (GTK_WIDGET (wizard));
	GtkRequisition req;  // temp usage

	if (wizard->m_menu) {
		gtk_widget_size_request (wizard->m_menu, &req);
		if (!around_child)
			requisition->width = req.width;
		requisition->height += req.height;
	}

	// body height
	{
		// title
		GtkRequisition title_req;
		gtk_widget_size_request (wizard->m_title, &title_req);
		if (!around_child)
			requisition->width = MAX (title_req.width + header_padding*2, requisition->width);
		requisition->height += title_req.height + header_padding*2;

		// content
		int content_height = 0;
		GtkWidget *child = GTK_BIN (wizard)->child;
		if (child && !around_child) {
			gtk_widget_size_request (child, &req);
			requisition->width = MAX (req.width + content_padding*2, requisition->width);
			content_height += req.height;
		}

		// navigation
		int navigation_height = 0;
		if (wizard->m_navigation) {
			gtk_widget_size_request (wizard->m_navigation, &req);
			requisition->width += req.width + content_padding*2;
			if (!around_child)
				navigation_height += req.height;
		}

		// body result
		requisition->width += border*2 + content_padding*2;
		requisition->height += MAX (content_height, navigation_height) + border*2 +
		                            content_padding*2;
	}

	// buttons
	gtk_widget_size_request (wizard->m_buttons, &req);
	if (!around_child)
		requisition->width = MAX (req.width, requisition->width);
	requisition->height += req.height + border;

printf ("wizard size request (%s): %d x %d\n", around_child ? "around child" : "normal", requisition->width, requisition->height);
}

//** internal stuff

#define PRINT_GTK_ALLOCATION(label, alloc) \
	printf ("allocating " label " - x: %d, y: %d, width: %d, height: %d\n", \
		alloc.x, alloc.y, alloc.width, alloc.height);

void ygtk_wizard_internal_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	ygtk_wizard_size_request (YGTK_WIZARD (widget), requisition, FALSE);
}

void ygtk_wizard_size_allocate (GtkWidget     *widget,
                                GtkAllocation *allocation)
{

printf ("size allocate\n");
	YGtkWizard *wizard = YGTK_WIZARD (widget);

	gint border = GTK_CONTAINER (wizard)->border_width;
	gint header_padding = get_header_padding (widget);
	gint content_padding = get_content_padding (widget);

	GtkRequisition req;  // temp usage
	GtkAllocation menu_alloc, title_alloc, navigation_alloc, buttons_alloc, child_alloc;

	// menu
	menu_alloc.x = allocation->x;
	menu_alloc.y = allocation->y;
	menu_alloc.width = allocation->width;
	if (wizard->m_menu) {
		gtk_widget_get_child_requisition (wizard->m_menu, &req);
		menu_alloc.height = req.height;
	} else
		menu_alloc.height = 0;

	// buttons
	gtk_widget_get_child_requisition (wizard->m_buttons, &req);
	buttons_alloc.x = allocation->x + border;
	buttons_alloc.y = (allocation->y + allocation->height) - req.height - border;
	buttons_alloc.width = allocation->width - border*2;
	buttons_alloc.height = req.height;

	// navigation X
	navigation_alloc.x = border + content_padding;
	if (wizard->m_navigation) {
		gtk_widget_get_child_requisition (wizard->m_navigation, &req);
		navigation_alloc.width = req.width;
	} else
		navigation_alloc.width = 0;

	// title
	gtk_widget_get_child_requisition (wizard->m_title, &req);
	if (wizard->m_navigation)
		title_alloc.x = navigation_alloc.x + navigation_alloc.width +
		                content_padding + header_padding;
	else
		title_alloc.x = border + header_padding;
	title_alloc.y = menu_alloc.y + menu_alloc.height + border + header_padding;
	title_alloc.width = allocation->width - title_alloc.x - border - header_padding;
	title_alloc.height = req.height;

	// navigation Y
	navigation_alloc.y = title_alloc.y + title_alloc.height + header_padding + content_padding;
	navigation_alloc.height = buttons_alloc.y - navigation_alloc.y - header_padding
	                          - content_padding;

	// child (aka content aka containee)
	GtkWidget *child = GTK_BIN (wizard)->child;
	if (child) {
		if (wizard->m_navigation)
			child_alloc.x = navigation_alloc.x + navigation_alloc.width + content_padding*2;
		else
			child_alloc.x = border + content_padding;
		child_alloc.y = title_alloc.y + title_alloc.height + header_padding + content_padding;
		child_alloc.width = allocation->width - child_alloc.x - border - content_padding;
		child_alloc.height = allocation->height - child_alloc.y - buttons_alloc.height
		                     - content_padding;
	}

	gtk_widget_size_allocate (wizard->m_title, &title_alloc);
PRINT_GTK_ALLOCATION ("title", title_alloc)
	gtk_widget_size_allocate (wizard->m_buttons, &buttons_alloc);
PRINT_GTK_ALLOCATION ("buttons", buttons_alloc)
	if (wizard->m_menu)
{
		gtk_widget_size_allocate (wizard->m_menu, &menu_alloc);
PRINT_GTK_ALLOCATION ("menu", menu_alloc)
}
	if (wizard->m_navigation)
{
		gtk_widget_size_allocate (wizard->m_navigation, &navigation_alloc);
PRINT_GTK_ALLOCATION ("navigation", navigation_alloc)
}
	if (child)
{
		gtk_widget_size_allocate (child, &child_alloc);
PRINT_GTK_ALLOCATION ("child", child_alloc)
}

	GTK_WIDGET_CLASS (ygtk_wizard_parent_class)->size_allocate (widget, allocation);
}

gboolean ygtk_wizard_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
//printf ("expose\n");
	YGtkWizard *wizard = YGTK_WIZARD (widget);

	gint border = GTK_CONTAINER (wizard)->border_width;
	gint header_padding = get_header_padding (widget);
	gint content_padding = get_content_padding (widget);

	// paint a colored box
	cairo_t *cr = gdk_cairo_create (widget->window);
	int x, y, w, h;

	// (outer rectangle)
	x = border;
	y = border;
	if (wizard->m_menu)
		y += wizard->m_menu->allocation.height;
	w = widget->allocation.width - border*2;
	h = wizard->m_buttons->allocation.y - y - border;

//printf ("outer rectangle: %d, %d x %d, %d\n", x, y, w, h);
	gdk_cairo_set_source_color (cr, &widget->style->bg [GTK_STATE_SELECTED]);
	cairo_rectangle (cr, x, y, w, h);
  cairo_fill (cr);

	// (inner rectangle)
	x += content_padding;
	w -= content_padding*2;
	y += wizard->m_title->allocation.height + header_padding*2 + content_padding;
	h -= wizard->m_title->allocation.height + header_padding*2 + content_padding*2;
	if (wizard->m_navigation) {
		int navigation_w = wizard->m_navigation->allocation.width;
		x += navigation_w + content_padding;
		w -= navigation_w + content_padding;
	}

//printf ("inner rectangle: %d, %d x %d, %d\n", x, y, w, h);
	gdk_cairo_set_source_color (cr, &widget->style->bg [GTK_STATE_NORMAL]);
	cairo_rectangle (cr, x, y, w, h);
  cairo_fill (cr);

  cairo_destroy (cr);

	// propagate expose
	GtkContainer *container = GTK_CONTAINER (wizard);
	if (wizard->m_menu)
		gtk_container_propagate_expose (container, wizard->m_menu, event);
	gtk_container_propagate_expose (container, wizard->m_title, event);
	if (wizard->m_navigation)
		gtk_container_propagate_expose (container, wizard->m_navigation, event);
	gtk_container_propagate_expose (container, wizard->m_buttons, event);
	if (GTK_BIN (container)->child)
		gtk_container_propagate_expose (container, GTK_BIN (container)->child, event);

	return FALSE;
}

void ygtk_wizard_forall (GtkContainer *container, gboolean include_internals,
                         GtkCallback   callback,  gpointer callback_data)
{
	YGtkWizard *wizard = YGTK_WIZARD (container);
	if (include_internals) {
		(*callback) (wizard->m_title, callback_data);
		(*callback) (wizard->m_buttons, callback_data);
		if (wizard->m_menu)
			(*callback) (wizard->m_menu, callback_data);
		if (wizard->m_navigation)
			(*callback) (wizard->m_navigation, callback_data);
	}

	GtkWidget *containee = GTK_BIN (container)->child;
	if (containee)
		(*callback) (containee, callback_data);
}

void button_clicked_cb (GtkButton *button, YGtkWizard *wizard)
{
	printf ("button pressed\n");
	gpointer id = g_object_get_data (G_OBJECT (button), "id");
	g_signal_emit (wizard, action_triggered_signal, 0, id, G_TYPE_POINTER);

	if (GTK_WIDGET (button) == wizard->m_release_notes_button) {
		printf ("release notes pressed\n");
		help_button_clicked_cb (button, wizard);
	}
}

void help_button_clicked_cb (GtkButton *button, YGtkWizard *wizard)
{
	gtk_widget_show (wizard->m_help_dialog);
}

void selected_menu_item_cb (GtkMenuItem *item, const char *id)
{
	YGtkWizard *wizard = g_object_get_data (G_OBJECT (item), "wizard");
	g_signal_emit (wizard, action_triggered_signal, 0, id, G_TYPE_STRING);
}

void tree_item_selected_cb (GtkTreeView *tree_view, YGtkWizard *wizard)
{
	const gchar *id = ygtk_wizard_get_tree_selection (wizard);
	if (id)
		g_signal_emit (wizard, action_triggered_signal, 0, id, G_TYPE_STRING);
}
