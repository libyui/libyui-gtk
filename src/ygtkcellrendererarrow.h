//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

/* YGtkCellRendererArrow can be used as a light replacement for the
   GtkCellRenderCombo or GtkCellRendererSpin. It renders two arrows
   in the cell. If the user presses one of them, the program gets
   the signal and does whatever it likes. One can hide one, or both,
   of the arrows through the can-go-up and can-go-down proprieties.
*/

#ifndef YGTK_CELL_RENDERER_ARROW_H
#define YGTK_CELL_RENDERER_ARROW_H

#include <gtk/gtkcellrenderer.h>
G_BEGIN_DECLS

#define YGTK_TYPE_CELL_RENDERER_ARROW  (ygtk_cell_renderer_arrow_get_type ())
#define YGTK_CELL_RENDERER_ARROW(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	YGTK_TYPE_CELL_RENDERER_ARROW, YGtkCellRendererArrow))
#define YGTK_CELL_RENDERER_ARROW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
	YGTK_TYPE_CELL_RENDERER_ARROW, YGtkCellRendererArrowClass))
#define YGTK_IS_CELL_RENDERER_ARROW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	YGTK_TYPE_CELL_RENDERER_ARROW))
#define YGTK_IS_CELL_RENDERER_ARROW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
	YGTK_TYPE_CELL_RENDERER_ARROW))
#define YGTK_CELL_RENDERER_ARROW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
	YGTK_TYPE_CELL_RENDERER_ARROW, YGtkCellRendererArrowClass))

typedef struct YGtkCellRendererArrow
{
	GtkCellRenderer cell_renderer;

	GdkPixbuf *up_arrow_pixbuf, *down_arrow_pixbuf;

	// proprieties:
	gboolean can_go_up, can_go_down;
} YGtkCellRendererArrow;

typedef struct YGtkCellRendererArrowClass
{
	GtkCellRendererClass parent_class;
	// signals:
	void (* pressed) (guint arrow_type, const gchar *path);
} YGtkCellRendererArrowClass;

GtkCellRenderer *ygtk_cell_renderer_arrow_new();
GType ygtk_cell_renderer_arrow_get_type (void) G_GNUC_CONST;

G_END_DECLS
#endif /*YGTK_CELL_RENDERER_ARROW_H*/
