/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkFieldEntry widget */
// check the header file for information about this widget


#include "ygtkfieldentry.h"
#include <gtk/gtk.h>
#include <string.h>

void ygutils_setFilter (GtkEntry *entry, const char *validChars);
static guint filter_entry_signal = 0;

//** YGtkFieldEntry

G_DEFINE_TYPE (YGtkFieldEntry, ygtk_field_entry, GTK_TYPE_BOX)

static void ygtk_field_entry_init (YGtkFieldEntry *entry)
{
	gtk_box_set_spacing (GTK_BOX (entry), 4);
}

static GtkEntry *ygtk_field_entry_focus_next_entry (YGtkFieldEntry *fields,
                                                    GtkEntry *current_entry,
                                                    gint side)
{
	GList *children = gtk_container_get_children (GTK_CONTAINER (fields));
	gint pos = g_list_index (children, current_entry);

	GtkEntry *next_entry = g_list_nth_data (children, pos + (2 * side));
	g_list_free (children);

	if (next_entry)
		gtk_widget_grab_focus (GTK_WIDGET (next_entry));
	return next_entry;
}

// If max characters reached, jump to next field
static void ygtk_field_entry_insert_text (GtkEditable *editable, const gchar *new_text,
	gint new_text_length, gint *position, YGtkFieldEntry *fields)
{
	if (*position == gtk_entry_get_max_length (GTK_ENTRY (editable))) {
		GtkEntry *next_entry = ygtk_field_entry_focus_next_entry (fields,
		                                  GTK_ENTRY (editable), 1);
		if (next_entry) {
			gint pos = 0;
			gtk_editable_insert_text (GTK_EDITABLE (next_entry), new_text,
			                          new_text_length, &pos);
			gtk_editable_set_position (GTK_EDITABLE (next_entry), pos);

			// it would not insert the text anyway, but to avoid the beep
			g_signal_stop_emission_by_name (editable, "insert_text");
		}
	}
}

static void ygtk_field_entry_move_cursor (GtkEntry *entry, GtkMovementStep move,
	gint count, gboolean selection, YGtkFieldEntry *fields)
{
	if (move == GTK_MOVEMENT_VISUAL_POSITIONS) {
		if (count > 0)
			ygtk_field_entry_focus_next_entry (fields, GTK_ENTRY (entry), 1);
		else
			ygtk_field_entry_focus_next_entry (fields, GTK_ENTRY (entry), -1);
	}
}


GtkWidget *ygtk_field_entry_new (void)
{ return g_object_new (YGTK_TYPE_FIELD_ENTRY, NULL); }

static void ygtk_field_entry_entry_changed (GtkEditable *editable, YGtkFieldEntry *fields)
{
	GList *children = gtk_container_get_children (GTK_CONTAINER (fields));
	gint nb = g_list_index (children, editable) / 2;
	g_list_free (children);

	g_signal_emit (fields, filter_entry_signal, 0, nb);
}

static guint ygtk_field_entry_length (YGtkFieldEntry *fields)
{
	guint length;
	GList *children = gtk_container_get_children (GTK_CONTAINER (fields));
	length = g_list_length (children);
	g_list_free (children);
	return length;
}

static inline guint child_to_index (YGtkFieldEntry *fields, guint child_i)
{ return child_i / 2; }
static inline guint index_to_child (YGtkFieldEntry *fields, guint index)
{ return index * 2; }

GtkEntry *ygtk_field_entry_get_field_widget (YGtkFieldEntry *fields, guint index)
{
	GtkEntry *entry;
	GList *children = gtk_container_get_children (GTK_CONTAINER (fields));
	entry = g_list_nth_data (children, index_to_child (fields, index));
	g_list_free (children);
	g_assert (GTK_IS_ENTRY (entry));
	return entry;
}
guint ygtk_field_entry_add_field (YGtkFieldEntry *fields, gchar separator)
{
	guint new_index = child_to_index (fields, ygtk_field_entry_length (fields)+1);

	GtkWidget *label = 0, *entry;
	if (new_index > 0) {
		const gchar str[2] = { separator, '\0' };
		label = gtk_label_new (str);
	}
	entry = gtk_entry_new();
	g_signal_connect (G_OBJECT (entry), "insert-text",
	                  G_CALLBACK (ygtk_field_entry_insert_text), fields);
	g_signal_connect (G_OBJECT (entry), "move-cursor",
	                  G_CALLBACK (ygtk_field_entry_move_cursor), fields);

	g_signal_connect (G_OBJECT (entry), "changed",
	                  G_CALLBACK (ygtk_field_entry_entry_changed), fields);

	GtkBox *box = GTK_BOX (fields);
	if (label) {
		gtk_box_pack_start (box, label, FALSE, TRUE, 0);
		gtk_widget_show (label);
	}
	gtk_box_pack_start (box, entry, TRUE, TRUE, 0);
	gtk_widget_show (entry);
	return new_index;
}

void ygtk_field_entry_setup_field (YGtkFieldEntry *fields, guint index,
                                   gint max_length, const gchar *valid_chars)
{
	GtkEntry *entry = ygtk_field_entry_get_field_widget (fields, index);
	gboolean disable_len = (max_length <= 0);
	gtk_entry_set_max_length (entry, disable_len ? 0 : max_length);
	gtk_entry_set_width_chars (entry, disable_len ? -1 : max_length);
	gtk_box_set_child_packing (GTK_BOX (fields), GTK_WIDGET (entry),
	                           disable_len, TRUE, 0, GTK_PACK_START);
	ygutils_setFilter (entry, valid_chars);
}

void ygtk_field_entry_set_field_text (YGtkFieldEntry *fields, guint index, const gchar *text)
{
	GtkEntry *entry = ygtk_field_entry_get_field_widget (fields, index);

	g_signal_handlers_block_by_func (entry,
		(gpointer) ygtk_field_entry_entry_changed, fields);
	g_signal_handlers_block_by_func (entry,
		(gpointer) ygtk_field_entry_insert_text, fields);

	gtk_entry_set_text (entry, text);

	g_signal_handlers_unblock_by_func (entry,
		(gpointer) ygtk_field_entry_entry_changed, fields);
	g_signal_handlers_unblock_by_func (entry,
		(gpointer) ygtk_field_entry_insert_text, fields);
}

const gchar *ygtk_field_entry_get_field_text (YGtkFieldEntry *fields, guint index)
{
	GtkEntry *entry = ygtk_field_entry_get_field_widget (fields, index);
	return gtk_entry_get_text (entry);
}

static gboolean ygtk_field_entry_mnemonic_activate (GtkWidget *widget, gboolean cycling)
{
	YGtkFieldEntry *fields = YGTK_FIELD_ENTRY (widget);
	GtkEntry *entry = ygtk_field_entry_get_field_widget (fields, 0);
	gtk_widget_grab_focus (GTK_WIDGET (entry));
	return TRUE;
}

static void ygtk_field_entry_class_init (YGtkFieldEntryClass *klass)
{
	ygtk_field_entry_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->mnemonic_activate  = ygtk_field_entry_mnemonic_activate;

	filter_entry_signal = g_signal_new ("field_entry_changed",
		G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET (YGtkFieldEntryClass, filter_entry_changed),
		NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);
}

gboolean ygtk_field_entry_set_focus (YGtkFieldEntry *fields)
{
	GList *children = gtk_container_get_children (GTK_CONTAINER (fields));
    g_return_val_if_fail (children != NULL, FALSE);
    GtkWidget *widget = GTK_WIDGET (children->data);
    g_list_free (children);

	gtk_editable_select_region (GTK_EDITABLE (widget), 0, -1);
	gtk_widget_grab_focus (widget);
	return gtk_widget_is_focus (widget);
}

