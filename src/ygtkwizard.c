//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

/* YGtkWizard widget */
// check the header file for information about this widget

#include "ygtkwizard.h"
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkimage.h>
#include <gtk/gtktreemodel.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkmenubar.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkseparatormenuitem.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtkhpaned.h>
#include <gtk/gtkframe.h>
#include <glib-object.h>
#include "ygtkrichtext.h"
#include "ygtksteps.h"

#define CONTENT_PADDING 15
#define TITLE_HEIGHT    45
#define MAIN_BORDER      6
#define HELP_BOX_CHARS_WIDTH 30

// YGUtils is C++, so this is the bridge
int ygutils_getCharsWidth (GtkWidget *widget, int chars_nb);
int ygutils_getCharsHeight (GtkWidget *widget, int chars_nb);
void ygutils_setWidgetFont (GtkWidget *widget, PangoWeight weight, double scale);

static void ygtk_wizard_class_init (YGtkWizardClass *klass);
static void ygtk_wizard_init       (YGtkWizard      *wizard);
static void ygtk_wizard_finalize   (GObject         *object);
static void ygtk_wizard_realize (GtkWidget *widget);

static gboolean expose_event (GtkWidget *widget, GdkEventExpose *event);

static void button_clicked_cb (GtkButton *button, YGtkWizard *wizard);
static void selected_menu_item_cb (GtkMenuItem *item, const char* id);
static void tree_item_selected_cb (GtkTreeView *tree_view, YGtkWizard *wizard);
static void gfree_data_cb (gpointer data, GClosure *closure);

static guint action_triggered_signal;

static GtkEventBoxClass *parent_class = NULL;

GType ygtk_wizard_get_type()
{
	static GType type = 0;
	if (!type) {
		static const GTypeInfo info = {
			sizeof (YGtkWizardClass),
			NULL, NULL, (GClassInitFunc) ygtk_wizard_class_init, NULL, NULL,
			sizeof (YGtkWizard), 0, (GInstanceInitFunc) ygtk_wizard_init, NULL
		};

		type = g_type_register_static (GTK_TYPE_EVENT_BOX, "YGtkWizard",
		                               &info, (GTypeFlags) 0);
	}
	return type;
}

static void destroy_strings (gpointer data)
{
	gchar *string = data;
	g_free (string);
}

static void destroy_tree_path (gpointer data)
{
	GtkTreePath *path = data;
	gtk_tree_path_free (path);
}

static void ygtk_marshal_VOID__POINTER_INT (GClosure     *closure,
                                            GValue       *return_value,
                                            guint         n_param_values,
                                            const GValue *param_values,
                                            gpointer      invocation_hint,
                                            gpointer      marshal_data)
{
	typedef void (*GMarshalFunc_VOID__POINTER_INT) (gpointer     data1,
	                                                gpointer     arg_1,
	                                                gint         arg_2,
	                                                gpointer     data2);
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

static void ygtk_wizard_class_init (YGtkWizardClass *klass)
{
	parent_class = (GtkEventBoxClass*) g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	//** Get expose, so we can draw line border
	widget_class->expose_event = expose_event;
	widget_class->realize = ygtk_wizard_realize;

	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = ygtk_wizard_finalize;

	action_triggered_signal = g_signal_new ("action-triggered",
	                       G_TYPE_FROM_CLASS (gobject_class),
	                       G_SIGNAL_RUN_LAST,
	                       G_STRUCT_OFFSET (YGtkWizardClass, action_triggered),
	                       NULL, NULL,
	                       ygtk_marshal_VOID__POINTER_INT,
	                       G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_INT);
}

static void ygtk_wizard_init (YGtkWizard *wizard)
{
	wizard->menu_ids = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                          destroy_strings, NULL);
	wizard->tree_ids = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                          destroy_strings, destroy_tree_path);
	wizard->steps_ids = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                           destroy_strings, NULL);

	// Layout widgets
	GtkWidget *main_vbox;

	//** Menu bar
	wizard->m_menu = gtk_menu_bar_new();

	//** Title
	wizard->m_title_hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_set_size_request (wizard->m_title_hbox, -1, TITLE_HEIGHT);

	wizard->m_title_image = gtk_image_new();
	wizard->m_title_label = gtk_label_new("");

	// setup label look
	gtk_widget_modify_fg (wizard->m_title_label, GTK_STATE_NORMAL,
	                      &wizard->m_title_label->style->fg[GTK_STATE_SELECTED]);

	// set a strong font to the heading label
	ygutils_setWidgetFont (wizard->m_title_label, PANGO_WEIGHT_ULTRABOLD,
	                       PANGO_SCALE_XX_LARGE);

	gtk_box_pack_start (GTK_BOX (wizard->m_title_hbox), wizard->m_title_label,
	                    FALSE, FALSE, 4);
	gtk_box_pack_end (GTK_BOX (wizard->m_title_hbox), wizard->m_title_image,
	                  FALSE, FALSE, 4);

	// containee
	wizard->m_containee = gtk_event_box_new();

	//** Help box
	GtkWidget *help_scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (help_scrolled_window),
	                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (help_scrolled_window),
	                                     GTK_SHADOW_IN);
	// shared with "Release Notes" button
	wizard->m_help_vbox = gtk_vbox_new (FALSE, 0);

	wizard->m_help_widget = ygtk_richtext_new();
	gtk_container_add (GTK_CONTAINER (help_scrolled_window), wizard->m_help_widget);
	// an image background will be set on realize

	wizard->m_release_notes_button = gtk_button_new_with_mnemonic ("");

	gtk_container_add (GTK_CONTAINER (wizard->m_help_vbox), help_scrolled_window);
	gtk_box_pack_start (GTK_BOX (wizard->m_help_vbox),
	                    wizard->m_release_notes_button, FALSE, FALSE, 0);

	gtk_widget_set_size_request (wizard->m_help_vbox,
		ygutils_getCharsWidth (wizard->m_help_widget, HELP_BOX_CHARS_WIDTH), -1);

	//** Steps/tree pane
	wizard->m_pane_box = gtk_event_box_new();
	// (steps/tree will be built later when calling 

	//** Adding the bottom buttons
	wizard->m_back_button  = gtk_button_new_with_mnemonic ("");
	wizard->m_abort_button = gtk_button_new_with_mnemonic ("");
	wizard->m_next_button  = gtk_button_new_with_mnemonic ("");
	// we need to protect this button against insensitive in some cases
	// this is a flag to enable that
	ygtk_wizard_protect_next_button (wizard, FALSE);

	g_signal_connect (G_OBJECT (wizard->m_back_button), "clicked",
	                  G_CALLBACK (button_clicked_cb), wizard);
	g_signal_connect (G_OBJECT (wizard->m_abort_button), "clicked",
	                  G_CALLBACK (button_clicked_cb), wizard);
	g_signal_connect (G_OBJECT (wizard->m_next_button), "clicked",
	                  G_CALLBACK (button_clicked_cb), wizard);

	g_signal_connect (G_OBJECT (wizard->m_release_notes_button), "clicked",
	                  G_CALLBACK (button_clicked_cb), wizard);

	wizard->m_button_box = gtk_hbutton_box_new();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (wizard->m_button_box), GTK_BUTTONBOX_END);
	gtk_container_set_border_width (GTK_CONTAINER (wizard->m_button_box), 10);
	gtk_box_set_spacing (GTK_BOX (wizard->m_button_box), 4);
	gtk_container_add (GTK_CONTAINER (wizard->m_button_box), wizard->m_abort_button);
	gtk_container_add (GTK_CONTAINER (wizard->m_button_box), wizard->m_back_button);
	gtk_container_add (GTK_CONTAINER (wizard->m_button_box), wizard->m_next_button);


	//** Setting general layouts
	wizard->m_help_program_pane = gtk_hpaned_new();
	gtk_paned_pack1 (GTK_PANED (wizard->m_help_program_pane), wizard->m_help_vbox,
	                 FALSE, TRUE);
	gtk_paned_pack2 (GTK_PANED (wizard->m_help_program_pane), wizard->m_containee,
	                 TRUE, TRUE);
	gtk_container_set_border_width (GTK_CONTAINER (wizard->m_help_program_pane),
	                                CONTENT_PADDING);

	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (main_vbox), wizard->m_menu, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (main_vbox), wizard->m_title_hbox, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (main_vbox), wizard->m_help_program_pane, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (main_vbox), wizard->m_button_box, FALSE, TRUE, 0);

	wizard->m_main_hbox = gtk_hbox_new (FALSE, 0);
	if (wizard->m_pane_box)
		gtk_box_pack_start (GTK_BOX (wizard->m_main_hbox), wizard->m_pane_box,
		                    FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (wizard->m_main_hbox), main_vbox);

	gtk_container_set_border_width (GTK_CONTAINER (wizard->m_pane_box), MAIN_BORDER);
	gtk_container_set_border_width (GTK_CONTAINER (main_vbox), MAIN_BORDER);
	gtk_container_add (GTK_CONTAINER (wizard), wizard->m_main_hbox);

	//** Lights on!
	gtk_widget_show_all (GTK_WIDGET (wizard));
	// some widgets need to start invisibly
	gtk_widget_hide (wizard->m_menu);
	gtk_widget_hide (wizard->m_pane_box);
	gtk_widget_hide (wizard->m_release_notes_button);
	gtk_widget_hide (wizard->m_back_button);
	gtk_widget_hide (wizard->m_abort_button);
	gtk_widget_hide (wizard->m_next_button);
}

void ygtk_wizard_realize (GtkWidget *widget)
{
	// realize widgets we contain first
	GTK_WIDGET_CLASS (parent_class)->realize (widget);

	YGtkWizard *wizard = YGTK_WIZARD (widget);
	gtk_widget_realize (wizard->m_help_widget);
	ygtk_richtext_set_background (YGTK_RICHTEXT (wizard->m_help_widget),
	                              THEMEDIR "/wizard/help-background.png");

	// set next button as default (for when pressing Enter)
	// must be here, so that the button has already been added to a window...
	GTK_WIDGET_SET_FLAGS (wizard->m_next_button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (wizard->m_next_button);
	gtk_widget_grab_focus (wizard->m_next_button);
}

static void ygtk_wizard_finalize (GObject *object)
{
	YGtkWizard *wizard = YGTK_WIZARD (object);
	g_hash_table_destroy (wizard->menu_ids);
	g_hash_table_destroy (wizard->tree_ids);
	g_hash_table_destroy (wizard->steps_ids);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget* ygtk_wizard_new()
{
	YGtkWizard* wizard = (YGtkWizard*) g_object_new (YGTK_TYPE_WIZARD, NULL);
	return GTK_WIDGET (wizard);
}

void ygtk_wizard_enable_steps (YGtkWizard *wizard)
{
	if (gtk_bin_get_child (GTK_BIN (wizard->m_pane_box))) {
		g_error ("YGtkWizard: a tree or steps widgets have already been enabled.");
		return;
	}

	GtkWidget *pane_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (pane_frame), GTK_SHADOW_OUT);

	wizard->m_steps_widget = ygtk_steps_new();
	gtk_container_add (GTK_CONTAINER (pane_frame), wizard->m_steps_widget);
	gtk_container_add (GTK_CONTAINER (wizard->m_pane_box), pane_frame);
	gtk_widget_show_all (wizard->m_pane_box);
}

void ygtk_wizard_enable_tree (YGtkWizard *wizard)
{
	if (gtk_bin_get_child (GTK_BIN (wizard->m_pane_box))) {
		g_error ("YGtkWizard: a tree or steps widgets have already been enabled.");
		return;
	}

	GtkWidget *pane_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (pane_frame), GTK_SHADOW_OUT);

	wizard->m_tree_widget = gtk_tree_view_new_with_model
		(GTK_TREE_MODEL (gtk_tree_store_new (1, G_TYPE_STRING)));
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (wizard->m_tree_widget),
		0, "(no title)", gtk_cell_renderer_text_new(), "text", 0, NULL);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (wizard->m_tree_widget), FALSE);
	g_signal_connect (G_OBJECT (wizard->m_tree_widget), "cursor-changed",
	                  G_CALLBACK (tree_item_selected_cb), wizard);

	GtkWidget *tree_scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tree_scrolled_window),
	                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (tree_scrolled_window),
	                                     GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (tree_scrolled_window), wizard->m_tree_widget);
	gtk_container_add (GTK_CONTAINER (wizard->m_pane_box), tree_scrolled_window);
	gtk_widget_set_size_request (tree_scrolled_window, 180, -1);
	gtk_widget_show_all (wizard->m_pane_box);
}

void ygtk_wizard_set_child (YGtkWizard *wizard, GtkWidget *new_child)
{
	GtkWidget *cur_child = gtk_bin_get_child (GTK_BIN (wizard->m_containee));
	if (cur_child)
		gtk_container_remove (GTK_CONTAINER (wizard->m_containee), cur_child);
	if (new_child)
		gtk_container_add (GTK_CONTAINER (wizard->m_containee), new_child);
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
	ygtk_richtext_set_text (YGTK_RICHTEXT (wizard->m_help_widget),
	                        text, FALSE, FALSE);
}

gboolean ygtk_wizard_add_tree_item (YGtkWizard *wizard, const char *parent_id,
                                    const char *text, const char *id)
{
	GtkTreeModel *model = gtk_tree_view_get_model
	                          (GTK_TREE_VIEW (wizard->m_tree_widget));
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
	GtkTreeView *tree = GTK_TREE_VIEW (wizard->m_tree_widget);
	gtk_tree_store_clear (GTK_TREE_STORE (gtk_tree_view_get_model (tree)));
	hash_table_clear (wizard->tree_ids);
}

gboolean ygtk_wizard_select_tree_item (YGtkWizard *wizard, const char *id)
{
	GtkTreePath *path = g_hash_table_lookup (wizard->tree_ids, id);
	if (path == NULL)
		return FALSE;

	g_signal_handlers_block_by_func (wizard->m_tree_widget,
	                                 (gpointer) tree_item_selected_cb, wizard);

	gtk_tree_view_expand_to_path (GTK_TREE_VIEW (wizard->m_tree_widget), path);
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (wizard->m_tree_widget), path,
	                          NULL, FALSE);

	g_signal_handlers_unblock_by_func (wizard->m_tree_widget,
	                                   (gpointer) tree_item_selected_cb, wizard);
	return TRUE;
}

void ygtk_wizard_set_header_text (YGtkWizard *wizard, GtkWindow *window,
                                  const char *text)
{
	gtk_label_set_text (GTK_LABEL (wizard->m_title_label), text);
	if (window)
		gtk_window_set_title (window, text);
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
	                       gfree_data_cb, 0);
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
	ygtk_steps_append_heading (YGTK_STEPS (wizard->m_steps_widget), text);
}

void ygtk_wizard_add_step (YGtkWizard *wizard, const char* text, const char *id)
{
	guint step_nb = ygtk_steps_append (YGTK_STEPS (wizard->m_steps_widget), text);
	g_hash_table_insert (wizard->steps_ids, g_strdup (id), GINT_TO_POINTER (step_nb));
}

gboolean ygtk_wizard_set_current_step (YGtkWizard *wizard, const char *id)
{
	gpointer step_nb = g_hash_table_lookup (wizard->steps_ids, id);
	if (!step_nb)
		return FALSE;
	ygtk_steps_set_current (YGTK_STEPS (wizard->m_steps_widget),
	                        GPOINTER_TO_INT (step_nb));
	return TRUE;
}

void ygtk_wizard_clear_steps (YGtkWizard *wizard)
{
	ygtk_steps_clear (YGTK_STEPS (wizard->m_steps_widget));
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
	gtk_tree_view_get_cursor (GTK_TREE_VIEW (wizard->m_tree_widget), &path, &column);
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

void ygtk_wizard_size_request (YGtkWizard *wizard, GtkRequisition *requisition)
{
	requisition->width = requisition->height = 0;
	GtkRequisition req;

	// horizontal
	if (GTK_WIDGET_VISIBLE (wizard->m_pane_box)) {
		gtk_widget_size_request (wizard->m_pane_box, &req);
		requisition->width += req.width +
		    gtk_container_get_border_width (GTK_CONTAINER (wizard->m_pane_box));
	}
	// pane's handler
	requisition->width += GTK_PANED (wizard->m_help_program_pane)->handle_pos.width;

	requisition->width += wizard->m_help_vbox->allocation.width
	                      + CONTENT_PADDING * 2;
	requisition->width += MAIN_BORDER * 2;

	// vertical
	if (GTK_WIDGET_VISIBLE (wizard->m_menu)) {
		gtk_widget_size_request (wizard->m_menu, &req);
		requisition->height += req.height +
		    gtk_container_get_border_width (GTK_CONTAINER (wizard->m_menu));
	}
	gtk_widget_size_request (wizard->m_title_hbox, &req);
	requisition->height += req.height + CONTENT_PADDING * 2;
	requisition->height += MAIN_BORDER * 2;
	gtk_widget_size_request (wizard->m_button_box, &req);
	requisition->height += req.height +
	    gtk_container_get_border_width (GTK_CONTAINER (wizard->m_button_box));
}

gboolean expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	// We'll draw a nice simple frame. Since some themes seem to draw some
	// color on GtkEventBox, I was forced to draw the frame on top, rather
	// than on bottom. This means that the header label and image is drawn
	// twice.
	GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);

	YGtkWizard *wizard = YGTK_WIZARD (widget);
	GtkWidget *title_widget = wizard->m_title_hbox;
	GtkWidget *help_box     = wizard->m_help_vbox;

	// Let's paint the square boxes
	// (a filled for the title and a stroke around the content area)
	int x, y, w, h;
	cairo_t *cr = gdk_cairo_create (widget->window);
	gdk_cairo_set_source_color (cr, &title_widget->style->bg[GTK_STATE_SELECTED]);

	// title
	x = title_widget->allocation.x;
	y = title_widget->allocation.y;
	w = title_widget->allocation.width;
	h = title_widget->allocation.height;

	cairo_rectangle (cr, x, y, w, h);
	cairo_fill (cr);

	// content area
	x += 1; w -= 2;
	y += h;
	h = help_box->allocation.height + CONTENT_PADDING*2;

	cairo_rectangle (cr, x, y, w, h);
	cairo_stroke (cr);

	cairo_destroy (cr);

	GTK_WIDGET_GET_CLASS (title_widget)->expose_event (title_widget, event);
	return TRUE;
}

void button_clicked_cb (GtkButton *button, YGtkWizard *wizard)
{
	gpointer id = g_object_get_data (G_OBJECT (button), "id");
	g_signal_emit (wizard, action_triggered_signal, 0, id, G_TYPE_POINTER);
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

void gfree_data_cb (gpointer data, GClosure *closure)
{ g_free (data); }
