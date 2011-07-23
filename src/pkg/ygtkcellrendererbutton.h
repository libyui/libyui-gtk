/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* GtkButton meets GtkCellRenderer.
*/

#ifndef YGTK_CELL_RENDERER_BUTTON_H
#define YGTK_CELL_RENDERER_BUTTON_H

#include <gtk/gtk.h>
G_BEGIN_DECLS

#define YGTK_TYPE_CELL_RENDERER_BUTTON  (ygtk_cell_renderer_button_get_type ())
#define YGTK_CELL_RENDERER_BUTTON(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	YGTK_TYPE_CELL_RENDERER_BUTTON, YGtkCellRendererButton))
#define YGTK_CELL_RENDERER_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
	YGTK_TYPE_CELL_RENDERER_BUTTON, YGtkCellRendererButtonClass))
#define YGTK_IS_CELL_RENDERER_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	YGTK_TYPE_CELL_RENDERER_BUTTON))
#define YGTK_IS_CELL_RENDERER_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
	YGTK_TYPE_CELL_RENDERER_BUTTON))
#define YGTK_CELL_RENDERER_BUTTON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
	YGTK_TYPE_CELL_RENDERER_BUTTON, YGtkCellRendererButtonClass))

typedef struct _YGtkCellRendererButton
{
	GtkCellRendererText parent;

	// private:
	GdkPixbuf *pixbuf;
	gchar *icon_name, *stock_id;
	gint icon_size;
	guint active : 2;
} YGtkCellRendererButton;

typedef struct _YGtkCellRendererButtonClass
{
	GtkCellRendererTextClass parent_class;

	void (* toggled) (YGtkCellRendererButton *renderer, const gchar *path);
} YGtkCellRendererButtonClass;

GtkCellRenderer *ygtk_cell_renderer_button_new (void);
GType ygtk_cell_renderer_button_get_type (void) G_GNUC_CONST;

gboolean ygtk_cell_renderer_button_get_active (YGtkCellRendererButton *cell);

G_END_DECLS
#endif /*YGTK_CELL_RENDERER_BUTTON_H*/

