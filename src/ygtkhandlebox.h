/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkHandleBox changes the GtkHandleBox behavior so that the detached
   window can be user resized.
*/

#ifndef YGTK_HANDLE_BOX_H
#define YGTK_HANDLE_BOX_H

#include <gtk/gtkhandlebox.h>

G_BEGIN_DECLS

#define YGTK_TYPE_HANDLE_BOX            (ygtk_handle_box_get_type ())
#define YGTK_HANDLE_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                    YGTK_TYPE_HANDLE_BOX, YGtkHandleBox))
#define YGTK_HANDLE_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                    YGTK_TYPE_HANDLE_BOX, YGtkHandleBoxClass))
#define YGTK_IS_HANDLE_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                    YGTK_TYPE_HANDLE_BOX))
#define YGTK_IS_HANDLE_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                    YGTK_TYPE_HANDLE_BOX))
#define YGTK_HANDLE_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                    YGTK_TYPE_HANDLE_BOX, YGtkHandleboxClass))

typedef struct _YGtkHandleBox
{
	GtkHandleBox parent;
} YGtkHandleBox;

typedef struct _YGtkHandleBoxClass
{
	GtkHandleBoxClass parent_class;
} YGtkHandleBoxClass;

GtkWidget* ygtk_handle_box_new (void);
GType ygtk_handle_box_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* YGTK_HANDLE_BOX_H */

