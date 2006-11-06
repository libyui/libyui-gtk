//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

/* YGtkFieldEntry widget */
// check the header file for information about this widget

#include "ygtkfieldentry.h"
#include <gtk/gtk.h>
#include <string.h>

/* YGtkFilterEntry */

gchar *ygutils_filterText (const char *text, int length, const char *valid_chars);

static void ygtk_filter_entry_class_init (YGtkFilterEntryClass *klass);
static void ygtk_filter_entry_init       (YGtkFilterEntry      *entry);
static void ygtk_filter_entry_destroy    (GtkObject            *object);
static void ygtk_filter_entry_insert_text (GtkEditable *editable, const gchar *new_text,
                                           gint new_text_length, gint *position);

/*
G_DEFINE_TYPE_WITH_CODE (YGtkFilterEntry, ygtk_filter_entry, GTK_TYPE_ENTRY,
  G_IMPLEMENT_INTERFACE (GTK_TYPE_EDITABLE, ygtk_filter_entry_editable_init))
*/
G_DEFINE_TYPE (YGtkFilterEntry, ygtk_filter_entry, GTK_TYPE_ENTRY)

static void ygtk_filter_entry_class_init (YGtkFilterEntryClass *klass)
{
	ygtk_filter_entry_parent_class = g_type_class_peek_parent (klass);

	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
	gtkobject_class->destroy = ygtk_filter_entry_destroy;

	GtkEditableClass *editable_class = GTK_EDITABLE_CLASS (klass);
	editable_class->insert_text = ygtk_filter_entry_insert_text;

}

static void ygtk_filter_entry_init (YGtkFilterEntry *entry)
{
}

void ygtk_filter_entry_destroy (GtkObject *object)
{
	YGtkFilterEntry *entry = YGTK_FILTER_ENTRY (object);
	if (entry->valid_chars)
		g_free (entry->valid_chars);
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
#if 0
	const gchar *valid_chars = YGTK_FILTER_ENTRY (editable)->valid_chars;

	if (valid_chars) {
		gchar *filtered = ygutils_filterText (new_text, new_text_length, valid_chars);
		if (!strcmp (filtered, new_text))
			gdk_beep();
		GTK_EDITABLE_CLASS (ygtk_filter_entry_parent_class)->insert_text
			(editable, filtered, -1, position);
		g_free (filtered);
	}
	else
		GTK_EDITABLE_CLASS (ygtk_filter_entry_parent_class)->insert_text
			(editable, new_text, new_text_length, position);
#endif
}

/* YGtkFieldEntry */

static void ygtk_field_entry_class_init (YGtkFieldEntryClass *klass);
static void ygtk_field_entry_init       (YGtkFieldEntry      *entry);
static guint value_changed_signal;

static void ygtk_field_entry_entry_changed (GtkEditable *editable, gpointer _nb);

G_DEFINE_TYPE (YGtkFieldEntry, ygtk_field_entry, GTK_TYPE_HBOX)

static void ygtk_field_entry_class_init (YGtkFieldEntryClass *klass)
{
	ygtk_field_entry_parent_class = g_type_class_peek_parent (klass);

	value_changed_signal = g_signal_new ("value_changed", G_OBJECT_CLASS_TYPE (klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET (YGtkFieldEntryClass, value_changed),
		NULL, NULL, gtk_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);
}

static void ygtk_field_entry_init (YGtkFieldEntry *entry)
{
	gtk_box_set_spacing (GTK_BOX (entry), 2);
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
	g_signal_connect (G_OBJECT (entry), "changed",
	                  G_CALLBACK (ygtk_field_entry_entry_changed),
	                  GINT_TO_POINTER (ygtk_field_entry_entries_nb (fields)));

	GtkBox *box = GTK_BOX (fields);
	if (label)
		gtk_box_pack_start (box, label, FALSE, FALSE, 0);
	gtk_box_pack_start (box, entry, TRUE, TRUE, 0);

	fields->use_separator = TRUE;
}

void ygtk_field_entry_entry_changed (GtkEditable *editable, gpointer _nb)
{
	gint nb = GPOINTER_TO_INT (_nb);
	g_signal_emit (editable, value_changed_signal, nb);
}

YGtkFilterEntry *ygtk_field_entry_get_field (YGtkFieldEntry *fields, guint nb)
{
	YGtkFilterEntry *entry;

	GList *children = gtk_container_get_children (GTK_CONTAINER (fields));
	entry = g_list_nth_data (children, (nb * 2) - 1);
	g_list_free (children);

	return entry;
}

guint ygtk_field_entry_entries_nb (YGtkFieldEntry *fields)
{
	guint nb;
	GList *children = gtk_container_get_children (GTK_CONTAINER (fields));
	nb = (g_list_length (children) + 1) / 2;
	g_list_free (children);
	return nb;
}

void ygtk_field_entry_set_field_text (YGtkFieldEntry *fields, guint nb, const gchar *text)
{
	YGtkFilterEntry *entry = ygtk_field_entry_get_field (fields, nb);
	gtk_entry_set_text (GTK_ENTRY (entry), text);
}

const gchar *ygtk_field_entry_get_field_text (YGtkFieldEntry *fields, guint nb)
{
	YGtkFilterEntry *entry = ygtk_field_entry_get_field (fields, nb);
	return gtk_entry_get_text (GTK_ENTRY (entry));
}
