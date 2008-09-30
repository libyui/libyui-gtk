/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkTooltip, unlike GtkTooltip, can be used independently of a widget.
   Also displays an arrow since it's meant to descript some element.
*/

#ifndef YGTK_TOOLTIP_H
#define YGTK_TOOLTIP_H

#include <gtk/gtkwindow.h>
G_BEGIN_DECLS

#define YGTK_TYPE_TOOLTIP            (ygtk_tooltip_get_type ())
#define YGTK_TOOLTIP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                        YGTK_TYPE_TOOLTIP, YGtkTooltip))
#define YGTK_TOOLTIP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                        YGTK_TYPE_TOOLTIP, YGtkTooltipClass))
#define YGTK_IS_TOOLTIP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                        YGTK_TYPE_TOOLTIP))
#define YGTK_IS_TOOLTIP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                        YGTK_TYPE_TOOLTIP))
#define YGTK_TOOLTIP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                        YGTK_TYPE_TOOLTIP, YGtkTooltipClass))

typedef enum { YGTK_POINTER_NONE, YGTK_POINTER_UP_LEFT, YGTK_POINTER_UP_RIGHT,
               YGTK_POINTER_DOWN_LEFT, YGTK_POINTER_DOWN_RIGHT } YGtkPointerType;

typedef struct YGtkTooltip
{
	GtkWindow parent;
	// private:
	YGtkPointerType pointer;
	guint timeout_id;
} YGtkTooltip;

typedef struct YGtkTooltipClass
{
	GtkWindowClass parent_class;
} YGtkTooltipClass;

GtkWidget *ygtk_tooltip_new (void);
GType ygtk_tooltip_get_type (void) G_GNUC_CONST;

// will display the tooltip and destroy it after a few seconds
// note: no need to free the thing yourself
void ygtk_tooltip_show_at (YGtkTooltip *tooltip, gint x, gint y, YGtkPointerType pointer);
void ygtk_tooltip_show_at_widget (YGtkTooltip *tooltip, GtkWidget *widget,
                                  YGtkPointerType pointer);

G_END_DECLS
#endif /*YGTK_TOOLTIP_H*/

