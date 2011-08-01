/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkCellRendererText cell renderer */
// check the header file for information about this cell renderer

#include "ygtkcellrenderertext.h"
#include <gtk/gtk.h>
#include <string.h>

G_DEFINE_TYPE (YGtkCellRendererText, ygtk_cell_renderer_text, GTK_TYPE_CELL_RENDERER_TEXT)

static void ygtk_cell_renderer_text_init (YGtkCellRendererText *bcell)
{}

gboolean filter_color_cb (PangoAttribute *attrb, gpointer data)
{
	PangoAttrType type = attrb->klass->type;
	return type == PANGO_ATTR_FOREGROUND || type == PANGO_ATTR_BACKGROUND;
}

static void ygtk_cell_renderer_text_render (
	GtkCellRenderer *cell, GdkDrawable *window, GtkWidget *widget,
	GdkRectangle *background_area, GdkRectangle *cell_area,
	GdkRectangle *expose_area, GtkCellRendererState flags)
{
	GtkCellRendererText *tcell = GTK_CELL_RENDERER_TEXT (cell);
	PangoAttrList *old_extra_attrs = 0, *new_extra_attrs = 0;
	if (flags & (GTK_CELL_RENDERER_SELECTED | GTK_CELL_RENDERER_INSENSITIVE)) {
		old_extra_attrs = pango_attr_list_copy (tcell->extra_attrs);
		new_extra_attrs = tcell->extra_attrs;

		PangoAttrList *t = pango_attr_list_filter (tcell->extra_attrs,
			filter_color_cb, NULL);
		pango_attr_list_unref (t);
	}

	GTK_CELL_RENDERER_CLASS (ygtk_cell_renderer_text_parent_class)->render (
		cell, window, widget, background_area, cell_area, expose_area, flags);

	if (old_extra_attrs) {
		tcell->extra_attrs = old_extra_attrs;
		pango_attr_list_unref (new_extra_attrs);
	}
}

GtkCellRenderer *ygtk_cell_renderer_text_new (void)
{ return g_object_new (YGTK_TYPE_CELL_RENDERER_TEXT, NULL); }

static void ygtk_cell_renderer_text_class_init (YGtkCellRendererTextClass *class)
{
	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);
	cell_class->render   = ygtk_cell_renderer_text_render;
}

