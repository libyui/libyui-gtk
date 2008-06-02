/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkWizard widget */
// check the header file for information about this widget

#include <config.h>
#include "ygtkwizard.h"
#include <atk/atk.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "ygtkhtmlwrap.h"
#include "ygtksteps.h"
#include "ygtkfindentry.h"

#define BUTTONS_SPACING 12
#define BORDER           4
#define CHILD_BORDER     6

// YGUtils bridge
extern void ygutils_setWidgetFont (GtkWidget *widget, PangoWeight weight,
                                   double scale);
extern void ygutils_setStockIcon (GtkWidget *button, const char *ycp_str);

static void set_label_header_cb (GtkWidget *label, GtkStyle *prev_style, gpointer psize)
{
	if (prev_style) return;
	double size = psize == 0 ? PANGO_SCALE_LARGE : PANGO_SCALE_XX_LARGE;
	gtk_widget_modify_fg (label, GTK_STATE_NORMAL, &label->style->fg [GTK_STATE_SELECTED]);
	ygutils_setWidgetFont (label, PANGO_WEIGHT_ULTRABOLD, size);
}
static void set_label_header (GtkWidget *label, int size)
{
	g_signal_connect (G_OBJECT (label), "style-set",
	                  G_CALLBACK (set_label_header_cb), GINT_TO_POINTER (size));
}

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


	gtk_window_set_title (GTK_WINDOW (dialog), "Help");
	gtk_window_set_default_size (GTK_WINDOW (dialog), 400, 350);

	// title
	GtkStockItem help_item;
	gtk_stock_lookup (GTK_STOCK_HELP, &help_item);

	dialog->title_box = gtk_hbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (dialog->title_box), 6);
	dialog->title_image = gtk_image_new_from_stock (GTK_STOCK_HELP,
	                                   GTK_ICON_SIZE_LARGE_TOOLBAR);
	dialog->title_label = gtk_label_new ("Help");
	set_label_header (dialog->title_label, 0);
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
	gtk_box_pack_start (GTK_BOX (dialog->vbox), dialog->title_box, FALSE, FALSE, 0);
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

#if 0
	// set help text background
	gtk_widget_realize (dialog->help_text);
	ygtk_html_wrap_set_background (dialog->help_text, THEMEDIR "/wizard/help-background.png");
#endif

	// set close as default widget
	gtk_widget_grab_default (dialog->close_button);
	gtk_widget_grab_focus (dialog->close_button);
}

static gboolean ygtk_help_dialog_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	YGtkHelpDialog *dialog = YGTK_HELP_DIALOG (widget);

	// draw rect on title
	int x, y, w, h;
	x = dialog->title_box->allocation.x;
	y = dialog->title_box->allocation.y;
	w = dialog->title_box->allocation.width;
	h = dialog->title_box->allocation.height;
	if (x + w >= event->area.x && x <= event->area.x + event->area.width &&
	    y + h >= event->area.y && x <= event->area.y + event->area.height) {
		cairo_t *cr = gdk_cairo_create (widget->window);
		gdk_cairo_set_source_color (cr, &widget->style->bg [GTK_STATE_SELECTED]);
		cairo_rectangle (cr, x, y, w, h);
		cairo_fill (cr);

		cairo_destroy (cr);
	}

	gtk_container_propagate_expose (GTK_CONTAINER (dialog), dialog->vbox, event);
	return FALSE;
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
	widget_class->expose_event = ygtk_help_dialog_expose_event;
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

//** YGtkWizard

static gint get_header_padding (GtkWidget *widget)
{
#if 0
// TODO: read GtkAssistance style_rc attributes
	gint padding;
	gtk_widget_style_get (widget, "header-padding", &padding, NULL);
	return padding;
#else
	return 6;
#endif
}

static gint get_content_padding (GtkWidget *widget)
{
#if 0
	gint padding;
	gtk_widget_style_get (widget, "content-padding", &padding, NULL);
	return padding;
#else
	return 1;
#endif
}

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

static void ygtk_wizard_popup_help (YGtkWizard *wizard)
{
	if (!wizard->m_help_dialog) {
		GtkWindow *window = (GtkWindow *) gtk_widget_get_ancestor (
		                        GTK_WIDGET (wizard), GTK_TYPE_WINDOW);
		wizard->m_help_dialog = ygtk_help_dialog_new (window);
		ygtk_help_dialog_set_text (YGTK_HELP_DIALOG (wizard->m_help_dialog),
		                           wizard->m_help);
	}
	gtk_window_present (GTK_WINDOW (wizard->m_help_dialog));
}

static void help_button_clicked_cb (GtkWidget *button, YGtkWizard *wizard)
{ ygtk_wizard_popup_help (wizard); }

G_DEFINE_TYPE (YGtkWizard, ygtk_wizard, GTK_TYPE_BIN)

static void ygtk_wizard_init (YGtkWizard *wizard)
{
	wizard->menu_ids = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                          g_free, NULL);
	wizard->tree_ids = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                          g_free, destroy_tree_path);
	wizard->steps_ids = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                           g_free, NULL);

	gtk_container_set_border_width (GTK_CONTAINER (wizard), BORDER);
	wizard->child_border_width = CHILD_BORDER;

	//** Title
	wizard->m_title = gtk_hbox_new (FALSE, 0);

	wizard->m_title_image = gtk_image_new();
	wizard->m_title_label = gtk_label_new("");
	gtk_label_set_ellipsize (GTK_LABEL (wizard->m_title_label), PANGO_ELLIPSIZE_END);
	gtk_misc_set_alignment (GTK_MISC (wizard->m_title_label), 0, 0.5);
	set_label_header (wizard->m_title_label, 1);

	gtk_box_pack_start (GTK_BOX (wizard->m_title), wizard->m_title_label,
	                    TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (wizard->m_title), wizard->m_title_image,
	                  FALSE, FALSE, 0);

	gtk_widget_set_parent (wizard->m_title, GTK_WIDGET (wizard));
	gtk_widget_show_all (wizard->m_title);

	//** Adding the bottom buttons
	wizard->m_next_button  = button_new (wizard);
	wizard->m_back_button  = button_new (wizard);
	wizard->m_abort_button = button_new (wizard);
	wizard->m_release_notes_button = button_new (wizard);

	wizard->m_help_button  = gtk_button_new_from_stock (GTK_STOCK_HELP);
	GTK_WIDGET_SET_FLAGS (wizard->m_help_button, GTK_CAN_DEFAULT);
	g_signal_connect (G_OBJECT (wizard->m_help_button), "clicked",
	                  G_CALLBACK (help_button_clicked_cb), wizard);

	// we need to protect this button against insensitive in some cases
	// this is a flag to enable that
	ygtk_wizard_protect_next_button (wizard, FALSE);

	wizard->m_buttons = gtk_hbox_new (FALSE, 12);

	gtk_box_pack_start (GTK_BOX (wizard->m_buttons), wizard->m_help_button, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (wizard->m_buttons), wizard->m_release_notes_button,
	                    FALSE, TRUE, 0);

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
	g_object_unref (G_OBJECT (buttons_group)); 

	//** The menu and the navigation widgets will be created when requested.
	//** Help dialog will be build on realize so we can give it a parent window.
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

/* We must unparent these widgets from the wizard as they would try
   to use gtk_container_remove() on it. We ref them since we still
   want to call destroy on them so they children die. */
#define DESTROY_WIDGET(widget)                \
	if (widget) {                             \
		gtk_widget_unparent (widget);         \
		widget = NULL;                        \
	}
	DESTROY_WIDGET (wizard->m_title)
	DESTROY_WIDGET (wizard->m_buttons)
	DESTROY_WIDGET (wizard->m_menu)
	DESTROY_WIDGET (wizard->m_steps)
	DESTROY_WIDGET (wizard->m_title)
#undef DESTROY_WIDGET

	if (wizard->m_help_dialog) {
		gtk_widget_destroy (wizard->m_help_dialog);
		wizard->m_help_dialog = NULL;
	}
}

GtkWidget *ygtk_wizard_new (void)
{
	return g_object_new (YGTK_TYPE_WIZARD, NULL);
}

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

void ygtk_wizard_enable_steps (YGtkWizard *wizard)
{
	g_return_if_fail (wizard->m_steps == NULL);

	wizard->m_steps = ygtk_steps_new();
	gtk_widget_modify_text (wizard->m_steps, GTK_STATE_NORMAL,
	                        &wizard->m_steps->style->fg [GTK_STATE_SELECTED]);

	gtk_widget_set_parent (wizard->m_steps, GTK_WIDGET (wizard));
	gtk_widget_show (wizard->m_steps);
	gtk_widget_queue_resize (GTK_WIDGET (wizard));
}

void ygtk_wizard_enable_tree (YGtkWizard *wizard)
{
	g_return_if_fail (wizard->m_tree == NULL);

	wizard->m_tree_view = gtk_tree_view_new_with_model
		(GTK_TREE_MODEL (gtk_tree_store_new (1, G_TYPE_STRING)));
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (wizard->m_tree_view),
		0, "", gtk_cell_renderer_text_new(), "text", 0, NULL);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (wizard->m_tree_view), FALSE);
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (
		GTK_TREE_VIEW (wizard->m_tree_view)), GTK_SELECTION_BROWSE);

	g_signal_connect (G_OBJECT (wizard->m_tree_view), "cursor-changed",
	                  G_CALLBACK (tree_item_selected_cb), wizard);

	wizard->m_tree = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (wizard->m_tree),
	                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (wizard->m_tree),
	                                     GTK_SHADOW_IN);

	gtk_container_add (GTK_CONTAINER (wizard->m_tree), wizard->m_tree_view);
	gtk_widget_set_size_request (wizard->m_tree, 180, -1);

	GtkWidget *child = wizard->m_child, *pane;
	pane = gtk_hpaned_new();
	gtk_paned_pack1 (GTK_PANED (pane), wizard->m_tree, TRUE, TRUE);
	gtk_widget_show_all (pane);
	ygtk_wizard_set_child (wizard, pane);
	wizard->m_pane = pane;
	if (child)
		gtk_container_add (GTK_CONTAINER (wizard), child);
	gtk_paned_set_position (GTK_PANED (pane), 180);
}

static void ygtk_wizard_add_child (GtkContainer *container, GtkWidget *child)
{
	YGtkWizard *wizard = YGTK_WIZARD (container);
	wizard->m_child = child;
	if (wizard->m_pane)
		gtk_paned_pack2 (GTK_PANED (wizard->m_pane), child, TRUE, FALSE);
	else
		GTK_CONTAINER_CLASS (ygtk_wizard_parent_class)->add (container, child);
}

static void ygtk_wizard_remove_child (GtkContainer *container, GtkWidget *child)
{
	YGtkWizard *wizard = YGTK_WIZARD (container);
	wizard->m_child = NULL;
	if (wizard->m_pane && child != wizard->m_pane)
		gtk_container_remove (GTK_CONTAINER (wizard->m_pane), child);
	else
		GTK_CONTAINER_CLASS (ygtk_wizard_parent_class)->remove (container, child);
}

void ygtk_wizard_set_child (YGtkWizard *wizard, GtkWidget *new_child)
{
	if (wizard->m_child)
		gtk_container_remove (GTK_CONTAINER (wizard), wizard->m_child);
	if (new_child)
		gtk_container_add (GTK_CONTAINER (wizard), new_child);
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
	ENABLE_WIDGET_STR (text, wizard->m_help_button);
}

gboolean ygtk_wizard_add_tree_item (YGtkWizard *wizard, const char *parent_id,
                                    const char *text, const char *id)
{
	GtkTreeModel *model = gtk_tree_view_get_model
	                          (GTK_TREE_VIEW (wizard->m_tree_view));
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
		const gchar *_title = gtk_label_get_text (GTK_LABEL (wizard->m_title_label));
		char *title;
		if (*_title == '\0')
			title = g_strdup ("YaST");
		else
			title = g_strdup_printf ("%s - YaST", _title);
		GdkPixbuf *pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (wizard->m_title_image));

		gtk_window_set_title (GTK_WINDOW (window), title);
		gtk_window_set_icon (GTK_WINDOW (window), pixbuf);

		g_free (title);
	}
}

void ygtk_wizard_set_header_text (YGtkWizard *wizard, const char *text)
{
	gtk_label_set_text (GTK_LABEL (wizard->m_title_label), text);
	sync_window_title (wizard);
}

gboolean ygtk_wizard_set_header_icon (YGtkWizard *wizard, const char *icon)
{
	GError *error = 0;
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (icon, &error);
	if (!pixbuf)
		return FALSE;
	gtk_image_set_from_pixbuf (GTK_IMAGE (wizard->m_title_image), pixbuf);
	g_object_unref (G_OBJECT (pixbuf));
	sync_window_title (wizard);
	return TRUE;
}

static void ygtk_wizard_map (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (ygtk_wizard_parent_class)->map (widget);
	sync_window_title (YGTK_WIZARD (widget));
}

void ygtk_wizard_set_abort_button_label (YGtkWizard *wizard, const char *text)
{
	gtk_button_set_label (GTK_BUTTON (wizard->m_abort_button), text);
	ENABLE_WIDGET_STR (text, wizard->m_abort_button);
	ygutils_setStockIcon (wizard->m_abort_button, text);
}

void ygtk_wizard_set_back_button_label (YGtkWizard *wizard, const char *text)
{
	gtk_button_set_label (GTK_BUTTON (wizard->m_back_button), text);
	ENABLE_WIDGET_STR (text, wizard->m_back_button);
	ygutils_setStockIcon (wizard->m_back_button, text);
}

void ygtk_wizard_set_next_button_label (YGtkWizard *wizard, const char *text)
{
	gtk_button_set_label (GTK_BUTTON (wizard->m_next_button), text);
	ENABLE_WIDGET_STR (text, wizard->m_next_button);
	ygutils_setStockIcon (wizard->m_next_button, text);
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

void ygtk_wizard_set_release_notes_button_label (YGtkWizard *wizard, const gchar *text)
{
	gtk_button_set_label (GTK_BUTTON (wizard->m_release_notes_button), text);
	gtk_widget_show (wizard->m_release_notes_button);
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
		gtk_widget_set_parent (wizard->m_menu, GTK_WIDGET (wizard));
		gtk_widget_queue_draw (GTK_WIDGET (wizard));
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

static void ygtk_wizard_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	YGtkWizard *wizard = YGTK_WIZARD (widget);

	gint border = GTK_CONTAINER (wizard)->border_width;
	gint child_border = wizard->child_border_width;
	gint header_padding = get_header_padding (GTK_WIDGET (wizard));
	gint content_padding = get_content_padding (GTK_WIDGET (wizard));
	GtkRequisition req;  // temp usage

	requisition->width = 0;
	requisition->height = border * 2;

	if (wizard->m_menu) {
		gtk_widget_size_request (wizard->m_menu, &req);
		requisition->width = MAX (requisition->width, req.width);
		requisition->height += req.height;
	}

	// title
	gtk_widget_size_request (wizard->m_title, &req);
	req.width += header_padding * 2 + border*2;
	req.height += header_padding * 2;

	requisition->width = MAX (MIN (req.width, 40), requisition->width);
	requisition->height += req.height;

	// body
	{
		GtkRequisition nav_req, child_req;

		if (wizard->m_steps) {
			gtk_widget_size_request (wizard->m_steps, &nav_req);
			nav_req.width += content_padding * 2;
			nav_req.height += content_padding * 2;
		}
		else
			nav_req.width = nav_req.height = 0;

		GtkWidget *child = GTK_BIN (wizard)->child;
		if (child && GTK_WIDGET_VISIBLE (child))
			gtk_widget_size_request (child, &child_req);
		else
			child_req.width = child_req.height = 0;
		child_req.width += content_padding * 2 + child_border*2;
		child_req.height += content_padding * 2 + child_border*2;

		req.width = nav_req.width + child_req.width + border*2;
		req.height = MAX (nav_req.height, child_req.height);

		requisition->width = MAX (requisition->width, req.width);
		requisition->height += req.height;
	}

	// buttons
	gtk_widget_size_request (wizard->m_buttons, &req);
	req.width += border*2;
	req.height += BUTTONS_SPACING + border;

	requisition->width = MAX (requisition->width, req.width);
	requisition->height += req.height;
}

// helper -- applies padding to a GtkAllocation
static inline void apply_allocation_padding (GtkAllocation *alloc, gint padding)
{
	alloc->x += padding;
	alloc->y += padding;
	alloc->width -= padding * 2;
	alloc->height -= padding * 2;
}

static void ygtk_wizard_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	YGtkWizard *wizard = YGTK_WIZARD (widget);

	gint border = GTK_CONTAINER (wizard)->border_width;
	gint header_padding = get_header_padding (widget);
	gint content_padding = get_content_padding (widget);
	GtkRequisition req;  // temp usage
	// we use "areas" because we need to do some tweaking when doing the actual
	// allocation and this makes it easier to place the other widgets.
	GtkAllocation menu_area, title_area, nav_area, child_area, buttons_area;

	// menu
	menu_area.x = allocation->x;
	menu_area.y = allocation->y;
	if (wizard->m_menu) {
		gtk_widget_get_child_requisition (wizard->m_menu, &req);
		menu_area.width = allocation->width;
		menu_area.height = req.height;
	}
	else
		menu_area.width = menu_area.height = 0;

	// title
	gtk_widget_get_child_requisition (wizard->m_title, &req);
	title_area.x = allocation->x + border;
	title_area.y = menu_area.y + menu_area.height + border;
	title_area.width = allocation->width - border*2;
	title_area.height = req.height + header_padding*2;

	// buttons
	gtk_widget_get_child_requisition (wizard->m_buttons, &req);
	buttons_area.x = title_area.x;
	buttons_area.y = (allocation->y + allocation->height) - req.height - border;
	buttons_area.width = title_area.width;
	buttons_area.height = req.height;

	// child (aka content aka containee)
	child_area.x = title_area.x;
	child_area.y = title_area.y + title_area.height;
	child_area.width = title_area.width;
	child_area.height = allocation->height - menu_area.height - title_area.height -
                      buttons_area.height - border*2 - BUTTONS_SPACING;

	// navigation pane
	nav_area.x = child_area.x;
	nav_area.y = child_area.y;
	nav_area.height = child_area.height;
	if (wizard->m_steps) {
		gtk_widget_get_child_requisition (wizard->m_steps, &req);
		nav_area.width = req.width + content_padding*2;

		child_area.x += nav_area.width;
		child_area.width -= nav_area.width;
		/*  // align to child
		title_area.x += nav_area.width;
		title_area.width -= nav_area.width;*/
	}
	else
		nav_area.width = 0;

	// Actual allocations
	// menu
	if (wizard->m_menu)
		gtk_widget_size_allocate (wizard->m_menu, &menu_area);

	// title
	apply_allocation_padding (&title_area, header_padding);
	gtk_widget_size_allocate (wizard->m_title, &title_area);

	// child
	GtkWidget *child = GTK_BIN (wizard)->child;
	if (child && GTK_WIDGET_VISIBLE (child)) {
		apply_allocation_padding (&child_area, content_padding + wizard->child_border_width);
		gtk_widget_size_allocate (child, &child_area);
	}

	// navigation pane
	if (wizard->m_steps) {
		apply_allocation_padding (&nav_area, content_padding);
		gtk_widget_size_allocate (wizard->m_steps, &nav_area);
	}

	// buttons
	gtk_widget_size_allocate (wizard->m_buttons, &buttons_area);

	GTK_WIDGET_CLASS (ygtk_wizard_parent_class)->size_allocate (widget, allocation);
}

static gboolean ygtk_wizard_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	if (!GTK_WIDGET_DRAWABLE (widget))
		return FALSE;
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
	w = widget->allocation.width - border*2;
	h = wizard->m_buttons->allocation.y - border - BUTTONS_SPACING;
	if (wizard->m_menu) {
		y += wizard->m_menu->allocation.height;
		h -= wizard->m_menu->allocation.height;
	}

	gdk_cairo_set_source_color (cr, &widget->style->bg [GTK_STATE_SELECTED]);
	cairo_rectangle (cr, x, y, w, h);
	cairo_fill (cr);

	// (inner rectangle)
	x += content_padding;
	w -= content_padding*2;
	y += wizard->m_title->allocation.height + header_padding*2 + content_padding;
	h -= wizard->m_title->allocation.height + header_padding*2 + content_padding*2;
	if (wizard->m_steps) {
		int steps_w = wizard->m_steps->allocation.width;
		x += steps_w + content_padding;
		w -= steps_w + content_padding;
	}

	gdk_cairo_set_source_color (cr, &widget->style->bg [GTK_STATE_NORMAL]);
	cairo_rectangle (cr, x, y, w, h);
	cairo_fill (cr);

	cairo_destroy (cr);

	// propagate expose
	GtkContainer *container = GTK_CONTAINER (wizard);
	if (wizard->m_menu)
		gtk_container_propagate_expose (container, wizard->m_menu, event);
	gtk_container_propagate_expose (container, wizard->m_title, event);
	if (wizard->m_steps)
		gtk_container_propagate_expose (container, wizard->m_steps, event);
	gtk_container_propagate_expose (container, wizard->m_buttons, event);
	if (GTK_BIN (container)->child)
		gtk_container_propagate_expose (container, GTK_BIN (container)->child, event);
	return TRUE;
}

static void ygtk_wizard_forall (GtkContainer *container, gboolean include_internals,
                                GtkCallback   callback,  gpointer callback_data)
{
	YGtkWizard *wizard = YGTK_WIZARD (container);
	if (include_internals) {
		(*callback) (wizard->m_title, callback_data);
		(*callback) (wizard->m_buttons, callback_data);
		if (wizard->m_menu)
			(*callback) (wizard->m_menu, callback_data);
		if (wizard->m_steps)
			(*callback) (wizard->m_steps, callback_data);
	}

	GtkWidget *containee = GTK_BIN (container)->child;
	if (containee)
		(*callback) (containee, callback_data);
}

//** Accessibility hooks

static gint ygtk_wizard_accessible_get_n_children (AtkObject *accessible)
{
	GtkWidget *widget = GTK_ACCESSIBLE (accessible)->widget;
	if (!widget)
		return 0;
	YGtkWizard *wizard = YGTK_WIZARD (widget);
	int nb = 1 /*content*/ + 5 /* buttons */;
	if (wizard->m_tree_view)
		nb++;  /* side navigator */
	return nb;
}

static AtkObject *ygtk_wizard_accessible_ref_child (AtkObject *accessible, gint index)
{
	GtkWidget *widget = GTK_ACCESSIBLE (accessible)->widget;
	if (!widget)
		return NULL;
	YGtkWizard *wizard = YGTK_WIZARD (widget);
	GtkWidget *child = 0;
	if (wizard->m_tree_view) {
		if (index == 0)
			child = wizard->m_tree_view;
		index--;
	}
	switch (index) {
		case 0:
			child = wizard->m_child;
			break;
		case 1:
			child = wizard->m_help_button;
			break;
		case 2:
			child = wizard->m_abort_button;
			break;
		case 3:
			child = wizard->m_back_button;
			break;
		case 4:
			child = wizard->m_next_button;
			break;
		case 5:
			if (GTK_WIDGET_VISIBLE (wizard->m_release_notes_button))
				child = wizard->m_release_notes_button;
			break;
		default:
			break;
	}

	if (child) {
		AtkObject *obj = gtk_widget_get_accessible (child);
		return g_object_ref (obj);
	}
	return NULL;
}

static void ygtk_wizard_accessible_class_init (AtkObjectClass *class)
{
	class->get_n_children = ygtk_wizard_accessible_get_n_children;
	class->ref_child = ygtk_wizard_accessible_ref_child;
}

static GType ygtk_wizard_accessible_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GType derived_type = g_type_parent (YGTK_TYPE_WIZARD);
		AtkObjectFactory *factory;
		GType derived_atk_type;
		GTypeQuery query;
		factory = atk_registry_get_factory (atk_get_default_registry(), derived_type);
		derived_atk_type = atk_object_factory_get_accessible_type (factory);
		g_type_query (derived_atk_type, &query);
		type = g_type_register_static_simple (derived_atk_type, 
			"YGtkWizardAccessible",  query.class_size,
			(GClassInitFunc) ygtk_wizard_accessible_class_init, query.instance_size,
			NULL, 0);
	}
	return type;
}

static AtkObject *ygtk_wizard_accessible_new (GObject *obj)
{
	g_return_val_if_fail (YGTK_IS_WIZARD (obj), NULL);
	AtkObject *accessible = g_object_new (ygtk_wizard_accessible_get_type (), NULL); 
	atk_object_initialize (accessible, obj);
	return accessible;
}

static GType ygtk_wizard_accessible_factory_get_accessible_type (void)
{
	return ygtk_wizard_accessible_get_type();
}

static AtkObject*ygtk_wizard_accessible_factory_create_accessible (GObject *obj)
{
  return ygtk_wizard_accessible_new (obj);
}

static void ygtk_wizard_accessible_factory_class_init (AtkObjectFactoryClass *class)
{
  class->create_accessible = ygtk_wizard_accessible_factory_create_accessible;
  class->get_accessible_type = ygtk_wizard_accessible_factory_get_accessible_type;
}

static GType ygtk_wizard_accessible_factory_get_type (void)
{
	static GType type = 0;
	if (!type)  {
		type = g_type_register_static_simple (ATK_TYPE_OBJECT_FACTORY, 
			"YGtkWizardAccessibleFactory", sizeof (AtkObjectFactoryClass),
			(GClassInitFunc) ygtk_wizard_accessible_factory_class_init,
			sizeof (AtkObjectFactory), NULL, 0);
	}
	return type;
}

static AtkObject *ygtk_wizard_get_accessible (GtkWidget *widget)
{
	static gboolean first_time = TRUE;
	if (first_time)  {
		GType derived_type = g_type_parent (YGTK_TYPE_WIZARD);
		AtkRegistry *registry = atk_get_default_registry();
		AtkObjectFactory *factory;
		GType derived_atk_type; 
		factory = atk_registry_get_factory (registry, derived_type);
		derived_atk_type = atk_object_factory_get_accessible_type (factory);
		if (g_type_is_a (derived_atk_type, GTK_TYPE_ACCESSIBLE)) {
			atk_registry_set_factory_type (registry, YGTK_TYPE_WIZARD,
				ygtk_wizard_accessible_factory_get_type());
		}
		first_time = FALSE;
	}
	return GTK_WIDGET_CLASS (ygtk_wizard_parent_class)->get_accessible (widget);
}

static void ygtk_wizard_class_init (YGtkWizardClass *klass)
{
	ygtk_wizard_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	//** Get expose, so we can draw line border
	widget_class->expose_event = ygtk_wizard_expose_event;
	widget_class->realize = ygtk_wizard_realize;
	widget_class->size_request  = ygtk_wizard_size_request;
	widget_class->size_allocate = ygtk_wizard_size_allocate;
	widget_class->get_accessible = ygtk_wizard_get_accessible;
	widget_class->map = ygtk_wizard_map;

	GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);
	container_class->forall = ygtk_wizard_forall;
	container_class->add    = ygtk_wizard_add_child;
	container_class->remove = ygtk_wizard_remove_child;

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

