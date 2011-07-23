/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkCellRendererText is a sub-class of GtkCellRendererText that
   strips any markup in the cell when the row is selected.
*/

#ifndef YGTK_CELL_RENDERER_TEXT_H
#define YGTK_CELL_RENDERER_TEXT_H

#include <gtk/gtk.h>
G_BEGIN_DECLS

#define YGTK_TYPE_CELL_RENDERER_TEXT (ygtk_cell_renderer_text_get_type())
#define YGTK_CELL_RENDERER_TEXT(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	YGTK_TYPE_CELL_RENDERER_TEXT, YGtkCellRendererText))
#define YGTK_CELL_RENDERER_TEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
	YGTK_TYPE_CELL_RENDERER_TEXT, YGtkCellRendererTextClass))
#define YGTK_IS_CELL_RENDERER_TEXT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	YGTK_TYPE_CELL_RENDERER_TEXT))
#define YGTK_IS_CELL_RENDERER_TEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
	YGTK_TYPE_CELL_RENDERER_TEXT))
#define YGTK_CELL_RENDERER_TEXT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
	YGTK_TYPE_CELL_TEXT, YGtkCellRendererTextClass))

typedef struct YGtkCellRendererText
{
	GtkCellRendererText parent;
} YGtkCellRendererText;

typedef struct YGtkCellRendererTextClass
{
	GtkCellRendererTextClass parent_class;
} YGtkCellRendererTextClass;

GtkCellRenderer *ygtk_cell_renderer_text_new (void);
GType ygtk_cell_renderer_text_get_type (void) G_GNUC_CONST;

G_END_DECLS
#endif

