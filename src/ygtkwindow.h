/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkWindow implements the gtk2 'allow-shrink' property.
*/

#ifndef YGTK_WINDOW_H
#define YGTK_WINDOW_H
#include <gtk/gtk.h>
G_BEGIN_DECLS

#define YGTK_TYPE_WINDOW            (ygtk_window_get_type ())
#define YGTK_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                    YGTK_TYPE_WINDOW, YGtkWindow))
#define YGTK_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                    YGTK_TYPE_WINDOW, YGtkWindowClass))
#define YGTK_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                    YGTK_TYPE_WINDOW))
#define YGTK_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                    YGTK_TYPE_WINDOW))
#define YGTK_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                    YGTK_TYPE_WINDOW, YGtkWindowClass))

typedef struct _YGtkWindow
{
	GtkWindow parent;
} YGtkWindow;

typedef struct _YGtkWindowClass
{
	GtkWindowClass parent_class;
} YGtkWindowClass;

GtkWidget* ygtk_window_new (void);
GType ygtk_window_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* YGTK_WINDOW_H */

