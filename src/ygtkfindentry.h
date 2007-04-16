//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

/* YGtkFindEntry is a widget with a GtkEntry that has a search icon
   on the left and an erase one on the right. Having the search icon
   is very useful to avoid a Search label and also a Customize button,
   since you can hook a menu on it.

   Initially, the idea was to extend GtkEntry to provide two GdkWindows,
   at each corner, just like GtkTextView provides. We would use inner-border
   attribute (provided since 2.10) so text wouldn't overlap it. However, inner-
   -border didn't quite work as I'd like and I just decided for the simpler
   approach used by the Banshee music player where they have a GtkFrame with a
   GtkHBox with an entry without frame. This code was written from scratch
   and improves the frame look and size.
*/

#ifndef YGTK_FIND_ENTRY_H
#define YGTK_FIND_ENTRY_H

#include <gtk/gtkframe.h>
G_BEGIN_DECLS

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

typedef struct _YGtkFindEntry       YGtkFindEntry;
typedef struct _YGtkFindEntryClass  YGtkFindEntryClass;

struct _YGtkFindEntry
{
	GtkFrame frame;
	// for GtkEntry API, use:
	GtkWidget *entry;
	GtkWidget *find_icon, *clear_icon;
};

struct _YGtkFindEntryClass
{
	GtkFrameClass parent_class;
};

GtkWidget* ygtk_find_entry_new (gboolean will_use_find_icon);
GType ygtk_find_entry_get_type (void) G_GNUC_CONST;

G_END_DECLS
#endif /*YGTK_FIND_ENTRY_H*/
