/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* A icon GtkButton next to a GtkLabel.
*/

#ifndef YGTK_CELL_RENDERER_SIDE_BUTTON_H
#define YGTK_CELL_RENDERER_SIDE_BUTTON_H

#include <gtk/gtk.h>
G_BEGIN_DECLS

#define YGTK_TYPE_CELL_RENDERER_SIDE_BUTTON (ygtk_cell_renderer_side_button_get_type())
#define YGTK_CELL_RENDERER_SIDE_BUTTON(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	YGTK_TYPE_CELL_RENDERER_SIDE_BUTTON, YGtkCellRendererSideButton))
#define YGTK_CELL_RENDERER_SIDE_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
	YGTK_TYPE_CELL_RENDERER_SIDE_BUTTON, YGtkCellRendererSideButtonClass))
#define YGTK_IS_CELL_RENDERER_SIDE_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	YGTK_TYPE_CELL_RENDERER_SIDE_BUTTON))
#define YGTK_IS_CELL_RENDERER_SIDE_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
	YGTK_TYPE_CELL_RENDERER_SIDE_BUTTON))
#define YGTK_CELL_RENDERER_SIDE_BUTTON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
	YGTK_TYPE_CELL_RENDERER_SIDE_BUTTON, YGtkCellRendererSideButtonClass))

typedef struct YGtkCellRendererSideButton
{
	GtkCellRendererText parent;

	// private:
	guint active : 2, button_visible : 2;
	gchar *stock_id;
	GdkPixbuf *pixbuf;
} YGtkCellRendererSideButton;

typedef struct YGtkCellRendererSideButtonClass
{
	GtkCellRendererTextClass parent_class;

	void (* toggled) (YGtkCellRendererSideButton *renderer, const gchar *path);
} YGtkCellRendererSideButtonClass;

GtkCellRenderer *ygtk_cell_renderer_side_button_new (void);
GType ygtk_cell_renderer_side_button_get_type (void) G_GNUC_CONST;

gboolean ygtk_cell_renderer_side_button_get_active (YGtkCellRendererSideButton *cell);

G_END_DECLS
#endif

