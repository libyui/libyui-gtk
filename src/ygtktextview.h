/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkTextView polishes GtkTextView a little bit, especially on
   read-only mode.
*/

#ifndef YGTK_TEXT_VIEW_H
#define YGTK_TEXT_VIEW_H

#include <gtk/gtktextview.h>
G_BEGIN_DECLS

#define YGTK_TYPE_TEXT_VIEW            (ygtk_text_view_get_type ())
#define YGTK_TEXT_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                        YGTK_TYPE_TEXT_VIEW, YGtkTextView))
#define YGTK_TEXT_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                        YGTK_TYPE_TEXT_VIEW, YGtkTextViewClass))
#define YGTK_IS_TEXT_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                              YGTK_TYPE_TEXT_VIEW))
#define YGTK_IS_TEXT_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                        YGTK_TYPE_TEXT_VIEW))
#define YGTK_TEXT_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                        YGTK_TYPE_TEXT_VIEW, YGtkTextViewClass))

typedef struct _YGtkTextView
{
	GtkTextView parent;
} YGtkTextView;

typedef struct _YGtkTextViewClass
{
	GtkTextViewClass parent_class;
} YGtkTextViewClass;

GtkWidget* ygtk_text_view_new (gboolean editable);
GType ygtk_text_view_get_type (void) G_GNUC_CONST;

G_END_DECLS
#endif /*YGTK_SCROLLED_WINDOW_H*/

