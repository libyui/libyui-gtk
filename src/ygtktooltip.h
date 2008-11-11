/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkTooltip, unlike GtkTooltip, can be used independently of a widget.
   Also displays an arrow since it's meant to descript some element.
*/

#ifndef YGTK_TOOLTIP_H
#define YGTK_TOOLTIP_H

#include <gtk/gtkwidget.h>
G_BEGIN_DECLS

typedef enum { YGTK_POINTER_NONE, YGTK_POINTER_UP_LEFT, YGTK_POINTER_UP_RIGHT,
               YGTK_POINTER_DOWN_LEFT, YGTK_POINTER_DOWN_RIGHT } YGtkPointerType;

// will display the tooltip and destroy it after a few seconds
// note: no need to free the thing yourself
void ygtk_tooltip_show_at (gint x, gint y, YGtkPointerType pointer,
                           const gchar *label, const gchar *stock_id);
void ygtk_tooltip_show_at_widget (GtkWidget *widget, YGtkPointerType pointer,
                                  const gchar *label, const gchar *stock_id);

G_END_DECLS
#endif /*YGTK_TOOLTIP_H*/

