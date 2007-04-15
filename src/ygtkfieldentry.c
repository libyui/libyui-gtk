//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

/* YGtkFieldEntry widget */
// check the header file for information about this widget

#include "ygtkfieldentry.h"
#include <gtk/gtk.h>
#include <string.h>

/* YGtkFilterEntry */

static void ygtk_filter_entry_class_init (YGtkFilterEntryClass *klass);
static void ygtk_filter_entry_init       (YGtkFilterEntry      *entry);
static void ygtk_filter_entry_destroy    (GtkObject            *object);
static void ygtk_filter_entry_insert_text (GtkEditable *editable, const gchar *new_text,
                                           gint new_text_length, gint *position);

G_DEFINE_TYPE (YGtkFilterEntry, ygtk_filter_entry, GTK_TYPE_ENTRY)

static void ygtk_filter_entry_class_init (YGtkFilterEntryClass *klass)
{
	ygtk_filter_entry_parent_class = g_type_class_peek_parent (klass);

	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
	gtkobject_class->destroy = ygtk_filter_entry_destroy;
}

static void ygtk_filter_entry_init (YGtkFilterEntry *entry)
{
	g_signal_connect (G_OBJECT (entry), "insert-text",
	                  G_CALLBACK (ygtk_filter_entry_insert_text), NULL);
}

void ygtk_filter_entry_destroy (GtkObject *object)
{
	YGtkFilterEntry *entry = YGTK_FILTER_ENTRY (object);
	if (entry->valid_chars)
		g_free (entry->valid_chars);
	entry->valid_chars = NULL;

	GTK_OBJECT_CLASS (ygtk_filter_entry_parent_class)->destroy (object);
}

GtkWidget *ygtk_filter_entry_new()
{
	return g_object_new (YGTK_TYPE_FILTER_ENTRY, NULL);
}

void ygtk_filter_entry_set_valid_chars (YGtkFilterEntry *entry, const gchar *valid_chars)
{
	if (entry->valid_chars)
		g_free (entry->valid_chars);
	entry->valid_chars = g_strdup (valid_chars);
}

void ygtk_filter_entry_insert_text (GtkEditable *editable, const gchar *new_text,
                                    gint new_text_length, gint *position)
{
	const gchar *valid_chars = YGTK_FILTER_ENTRY (editable)->valid_chars;
	if (!valid_chars)
		return;
	const gchar *i, *j;
	for (i = new_text; *i; i++) {
		for (j = valid_chars; *j; j++) {
			if (*i == *j)
				break;
		}
		if (!*j) {
			// not valid text
			gdk_beep();
			g_signal_stop_emission_by_name (editable, "insert_text");
		}
	}
}

/* YGtkFieldEntry */

static void ygtk_field_entry_class_init (YGtkFieldEntryClass *klass);
static void ygtk_field_entry_init       (YGtkFieldEntry      *entry);
static guint filter_entry_signal;

static YGtkFilterEntry *ygtk_field_entry_get_field (YGtkFieldEntry *entry, guint nb);
// callbacks
static void ygtk_field_entry_entry_changed (GtkEditable *editable,
                                            YGtkFieldEntry *fields);
static void ygtk_field_entry_insert_text (GtkEditable *editable, const gchar *new_text,
                                          gint new_text_length, gint *position,
                                          YGtkFieldEntry *fields);
static void ygtk_field_entry_move_cursor (GtkEntry       *entry,
                                          GtkMovementStep move,
                                          gint count, gboolean selection,
                                          YGtkFieldEntry *fields);

G_DEFINE_TYPE (YGtkFieldEntry, ygtk_field_entry, GTK_TYPE_HBOX)

static void ygtk_field_entry_class_init (YGtkFieldEntryClass *klass)
{
	ygtk_field_entry_parent_class = g_type_class_peek_parent (klass);

	filter_entry_signal = g_signal_new ("field_entry_changed",
		G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET (YGtkFieldEntryClass, filter_entry_changed),
		NULL, NULL, gtk_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);
}

static void ygtk_field_entry_init (YGtkFieldEntry *entry)
{
	gtk_box_set_spacing (GTK_BOX (entry), 4);
}

GtkWidget *ygtk_field_entry_new()
{
	return g_object_new (YGTK_TYPE_FIELD_ENTRY, NULL);
}

void ygtk_field_entry_add_field (YGtkFieldEntry *fields, gchar separator,
                                 guint max_length, const gchar *valid_chars)
{
	GtkWidget *label = 0, *entry;
	if (fields->use_separator) {
		const gchar str[2] = { separator, '\0' };
		label = gtk_label_new (str);
	}
	entry = ygtk_filter_entry_new();
	gtk_entry_set_max_length (GTK_ENTRY (entry), max_length);
	gtk_entry_set_width_chars (GTK_ENTRY (entry), (max_length == 0) ? -1 : max_length);
	ygtk_filter_entry_set_valid_chars (YGTK_FILTER_ENTRY (entry), valid_chars);

	g_signal_connect (G_OBJECT (entry), "insert-text",
	                  G_CALLBACK (ygtk_field_entry_insert_text), fields);
	g_signal_connect (G_OBJECT (entry), "move-cursor",
	                  G_CALLBACK (ygtk_field_entry_move_cursor), fields);

	g_signal_connect (G_OBJECT (entry), "changed",
	                  G_CALLBACK (ygtk_field_entry_entry_changed), fields);

	GtkBox *box = GTK_BOX (fields);
	if (label)
		gtk_box_pack_start (box, label, FALSE, FALSE, 0);
	gtk_box_pack_start (box, entry, TRUE, TRUE, 0);

	fields->use_separator = TRUE;
}

void ygtk_field_entry_entry_changed (GtkEditable *editable, YGtkFieldEntry *fields)
{
	GList *children = gtk_container_get_children (GTK_CONTAINER (fields));
	gint nb = g_list_index (children, YGTK_FILTER_ENTRY (editable)) / 2;
	g_list_free (children);

	g_signal_emit (fields, filter_entry_signal, 0, nb);
}

YGtkFilterEntry *ygtk_field_entry_get_field (YGtkFieldEntry *fields, guint nb)
{
	YGtkFilterEntry *entry;

	GList *children = gtk_container_get_children (GTK_CONTAINER (fields));
	entry = g_list_nth_data (children, nb * 2);
	g_list_free (children);

	return entry;
}

void ygtk_field_entry_set_field_text (YGtkFieldEntry *fields, guint nb, const gchar *text)
{
	YGtkFilterEntry *entry = ygtk_field_entry_get_field (fields, nb);
	g_signal_handlers_block_by_func (entry,
		(gpointer) ygtk_field_entry_entry_changed, fields);
	g_signal_handlers_block_by_func (entry,
		(gpointer) ygtk_field_entry_insert_text, fields);

	gtk_entry_set_text (GTK_ENTRY (entry), text);

	g_signal_handlers_unblock_by_func (entry,
		(gpointer) ygtk_field_entry_entry_changed, fields);
	g_signal_handlers_unblock_by_func (entry,
		(gpointer) ygtk_field_entry_insert_text, fields);
}

const gchar *ygtk_field_entry_get_field_text (YGtkFieldEntry *fields, guint nb)
{
	YGtkFilterEntry *entry = ygtk_field_entry_get_field (fields, nb);
	return gtk_entry_get_text (GTK_ENTRY (entry));
}

static YGtkFilterEntry *ygtk_field_entry_focus_next_entry (YGtkFieldEntry *fields,
                                                   YGtkFilterEntry *current_entry,
                                                   gint side)
{
	GList *children = gtk_container_get_children (GTK_CONTAINER (fields));
	gint pos = g_list_index (children, current_entry);

	YGtkFilterEntry *next_entry = g_list_nth_data (children, pos + (2 * side));
	g_list_free (children);

	if (next_entry)
		gtk_widget_grab_focus (GTK_WIDGET (next_entry));
	return next_entry;
}

// If max characters reached, jump to next field
void ygtk_field_entry_insert_text (GtkEditable *editable, const gchar *new_text,
                                   gint new_text_length, gint *position,
                                   YGtkFieldEntry *fields)
{
	if (*position == gtk_entry_get_max_length (GTK_ENTRY (editable))) {
		YGtkFilterEntry *next_entry = ygtk_field_entry_focus_next_entry (fields,
		                                  YGTK_FILTER_ENTRY (editable), 1);
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

void ygtk_field_entry_move_cursor (GtkEntry *entry, GtkMovementStep move,
                                   gint count, gboolean selection,
                                   YGtkFieldEntry *fields)
{
	if (move == GTK_MOVEMENT_VISUAL_POSITIONS) {
		if (count > 0)
			ygtk_field_entry_focus_next_entry (fields, YGTK_FILTER_ENTRY (entry), 1);
		else
			ygtk_field_entry_focus_next_entry (fields, YGTK_FILTER_ENTRY (entry), -1);
	}
}
