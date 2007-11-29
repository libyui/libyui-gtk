/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkExtEntry extends GtkEntry adding two optional GdkWindows, at each
   horizontal corner, which are created at demand. Very useful to create
   a panel, an icon or something inside of it.
   GtkTextView provides something similar and would be nice to have this
   right on GtkEntry; in fact, GtkSpinButton could be simplified if it
   were for this.
   Right-to-left is supported at this level (borders are mirrored, if it
   is set). If you don't want that, use gtk_widget_set_direction().

   YGtkFindEntry is a convinience widget that has a find icon, with an
   optional menu, at the left, and an erase icon at the right.
*/

#ifndef YGTK_FIND_ENTRY_H
#define YGTK_FIND_ENTRY_H

#include <gtk/gtkentry.h>
G_BEGIN_DECLS

//** YGtkExtEntry

#define YGTK_TYPE_EXT_ENTRY            (ygtk_ext_entry_get_type ())
#define YGTK_EXT_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                        YGTK_TYPE_EXT_ENTRY, YGtkExtEntry))
#define YGTK_EXT_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                        YGTK_TYPE_EXT_ENTRY, YGtkExtEntryClass))
#define YGTK_IS_EXT_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                        YGTK_TYPE_EXT_ENTRY))
#define YGTK_IS_EXT_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                        YGTK_TYPE_EXT_ENTRY))
#define YGTK_EXT_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                        YGTK_TYPE_EXT_ENTRY, YGtkExtEntryClass))

typedef struct _YGtkExtEntry
{
	GtkEntry parent;
	// members:
	GdkWindow *left_window, *right_window;
} YGtkExtEntry;

typedef struct _YGtkExtEntryClass
{
	GtkEntryClass parent_class;
} YGtkExtEntryClass;

GtkWidget* ygtk_ext_entry_new (void);
GType ygtk_ext_entry_get_type (void) G_GNUC_CONST;

// API based on that of GtkTextView -- limited to our needs
typedef enum
{ YGTK_EXT_ENTRY_WIDGET_WIN, YGTK_EXT_ENTRY_TEXT_WIN,
  YGTK_EXT_ENTRY_LEFT_WIN, YGTK_EXT_ENTRY_RIGHT_WIN
} YGtkExtEntryWindowType;

GdkWindow *ygtk_ext_entry_get_window (YGtkExtEntry *entry,
                                      YGtkExtEntryWindowType type);
void ygtk_ext_entry_set_border_window_size (YGtkExtEntry *entry,
                                            YGtkExtEntryWindowType type, gint size);
gint ygtk_ext_entry_get_border_window_size (YGtkExtEntry *entry,
                                            YGtkExtEntryWindowType type);

//** YGtkFindEntry

#define YGTK_TYPE_FIND_ENTRY            (ygtk_find_entry_get_type ())
#define YGTK_FIND_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                         YGTK_TYPE_FIND_ENTRY, YGtkFindEntry))
#define YGTK_FIND_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                         YGTK_TYPE_FIND_ENTRY, YGtkFindEntryClass))
#define YGTK_IS_FIND_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                         YGTK_TYPE_FIND_ENTRY))
#define YGTK_IS_FIND_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                         YGTK_TYPE_FIND_ENTRY))
#define YGTK_FIND_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                         YGTK_TYPE_FIND_ENTRY, YGtkFindEntryClass))

typedef struct _YGtkFindEntry
{
	YGtkExtEntry parent;
	// members
	GdkPixbuf *find_icon, *clear_icon, *find_hover_icon, *clear_hover_icon;
	GtkMenu *context_menu;
} YGtkFindEntry;

typedef struct _YGtkFindEntryClass
{
	YGtkExtEntryClass parent_class;
} YGtkFindEntryClass;

GtkWidget* ygtk_find_entry_new (void);
GType ygtk_find_entry_get_type (void) G_GNUC_CONST;

void ygtk_find_entry_attach_menu (YGtkFindEntry *entry, GtkMenu *popup_menu);

G_END_DECLS
#endif /*YGTK_FIND_ENTRY_H*/
