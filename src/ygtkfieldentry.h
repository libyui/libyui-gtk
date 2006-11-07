//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

/* YGtkFieldEntry is an extension of GtkEntry with the added
   functionality of being able to define fields (useful for when
   you need the user to set a IP address or time/date). The number
   of fields, their individual range and separation character
   is all customizable.

   YGtkFilterEntry is an extension to GtkEntry that only accepts
   input that is specified in an array of valid characters
*/

#ifndef YGTK_FILTER_ENTRY_H
#define YGTK_FILTER_ENTRY_H

#include <gtk/gtkentry.h>
G_BEGIN_DECLS

#define YGTK_TYPE_FILTER_ENTRY            (ygtk_filter_entry_get_type ())
#define YGTK_FILTER_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                       YGTK_TYPE_FILTER_ENTRY, YGtkFilterEntry))
#define YGTK_FILTER_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                 YGTK_TYPE_FILTER_ENTRY, YGtkFilterEntryClass))
#define YGTK_IS_FILTER_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                           YGTK_TYPE_FILTER_ENTRY))
#define YGTK_IS_FILTER_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                           YGTK_TYPE_FILTER_ENTRY))
#define YGTK_FILTER_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                  YGTK_TYPE_FILTER_ENTRY, YGtkFilterEntryClass))

typedef struct _YGtkFilterEntry       YGtkFilterEntry;
typedef struct _YGtkFilterEntryClass  YGtkFilterEntryClass;

struct _YGtkFilterEntry
{
	GtkEntry entry;

	gchar *valid_chars;
};

struct _YGtkFilterEntryClass
{
	GtkEntryClass parent_class;
};

GtkWidget* ygtk_filter_entry_new();
GType ygtk_filter_entry_get_type (void) G_GNUC_CONST;

void ygtk_filter_entry_set_valid_chars (YGtkFilterEntry *entry,
                                        const gchar *valid_chars);

G_END_DECLS
#endif /*YGTK_FILTER_ENTRY_H*/

#ifndef YGTK_FIELD_ENTRY_H
#define YGTK_FIELD_ENTRY_H

#include <gtk/gtkhbox.h>
G_BEGIN_DECLS

#define YGTK_TYPE_FIELD_ENTRY            (ygtk_field_entry_get_type ())
#define YGTK_FIELD_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                          YGTK_TYPE_FIELD_ENTRY, YGtkFieldEntry))
#define YGTK_FIELD_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                          YGTK_TYPE_FIELD_ENTRY, YGtkFieldEntryClass))
#define IS_YGTK_FIELD_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                          YGTK_TYPE_FIELD_ENTRY))
#define IS_YGTK_FIELD_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                          YGTK_TYPE_FIELD_ENTRY))
#define YGTK_FIELD_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                          YGTK_TYPE_FIELD_ENTRY, YGtkFieldEntryClass))

typedef struct _YGtkFieldEntry       YGtkFieldEntry;
typedef struct _YGtkFieldEntryClass  YGtkFieldEntryClass;

struct _YGtkFieldEntry
{
	GtkHBox hbox;

	// used to disable separator for the first field
	gboolean use_separator;
};

struct _YGtkFieldEntryClass
{
	GtkHBoxClass parent_class;

	void (* filter_entry_changed) (YGtkFieldEntry *entry, gint field_nb);
};

GtkWidget* ygtk_field_entry_new();
GType ygtk_field_entry_get_type (void) G_GNUC_CONST;

// if this is the first field, separator will be ignored. max_length can be 0 to
// disable it. valids_chars can be NULL to disable it.
void ygtk_field_entry_add_field (YGtkFieldEntry *entry, gchar separator,
                                 guint max_length, const gchar *valid_chars);

// convinience
void ygtk_field_entry_set_field_text (YGtkFieldEntry *entry, guint nb, const gchar *text);
const gchar *ygtk_field_entry_get_field_text (YGtkFieldEntry *entry, guint nb);

G_END_DECLS
#endif /*YGTK_FIELD_ENTRY_H*/
