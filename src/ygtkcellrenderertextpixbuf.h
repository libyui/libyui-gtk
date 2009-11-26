/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkCellRendererTextPixbuf is a combination of GtkCellRendererText and
   GtkCellRendererPixbuf. It allows text and icons to be mixed on the same
   column.
   We derive from GtkCellRendererText, so we don't have to implement the
   ATK hooks.
*/

#ifndef YGTK_CELL_RENDERER_TEXT_PIXBUF_H
#define YGTK_CELL_RENDERER_TEXT_PIXBUF_H

#include <gtk/gtkcellrenderertext.h>
G_BEGIN_DECLS

#define YGTK_TYPE_CELL_RENDERER_TEXT_PIXBUF  (ygtk_cell_renderer_text_pixbuf_get_type ())
#define YGTK_CELL_RENDERER_TEXT_PIXBUF(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	YGTK_TYPE_CELL_RENDERER_TEXT_PIXBUF, YGtkCellRendererTextPixbuf))
#define YGTK_CELL_RENDERER_TEXT_PIXBUF_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
	YGTK_TYPE_CELL_RENDERER_TEXT_PIXBUF, YGtkCellRendererTextPixbufClass))
#define YGTK_IS_CELL_RENDERER_TEXT_PIXBUF(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	YGTK_TYPE_CELL_RENDERER_TEXT_PIXBUF))
#define YGTK_IS_CELL_RENDERER_TEXT_PIXBUF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
	YGTK_TYPE_CELL_RENDERER_TEXT_PIXBUF))
#define YGTK_CELL_RENDERER_TEXT_PIXBUF_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
	YGTK_TYPE_CELL_RENDERER_TEXT_PIXBUF, YGtkCellRendererTextPixbufClass))

typedef struct _YGtkCellRendererTextPixbuf
{
	GtkCellRendererText parent;

	// private:
	GdkPixbuf *pixbuf;
	gchar *stock_id;
	GtkIconSize stock_size;
} YGtkCellRendererTextPixbuf;

typedef struct _YGtkCellRendererTextPixbufClass
{
	GtkCellRendererTextClass parent_class;
} YGtkCellRendererTextPixbufClass;

GtkCellRenderer *ygtk_cell_renderer_text_pixbuf_new (void);
GType ygtk_cell_renderer_text_pixbuf_get_type (void) G_GNUC_CONST;

G_END_DECLS
#endif /*YGTK_CELL_RENDERER_TEXT_PIXBUF_H*/

