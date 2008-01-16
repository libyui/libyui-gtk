/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkCellRendererTextPixbuf widget */
// check the header file for information about this widget

#include <config.h>
#include "ygtkcellrenderertextpixbuf.h"

#define PIXBUF_TEXT_SPACING 4

enum {
  PROP_0,
  PROP_TEXT,
  PROP_PIXBUF
};

G_DEFINE_TYPE (YGtkCellRendererTextPixbuf, ygtk_cell_renderer_text_pixbuf, GTK_TYPE_CELL_RENDERER)

static void ygtk_cell_renderer_text_pixbuf_init (YGtkCellRendererTextPixbuf *tpcell)
{
	GtkCellRenderer *cell = GTK_CELL_RENDERER (tpcell);
	cell->mode = GTK_CELL_RENDERER_MODE_ACTIVATABLE;
	cell->xalign = 0.0;
	cell->yalign = 0.5;
	cell->xpad = 0;
	cell->ypad = 0;
	tpcell->text = NULL;
	tpcell->pixbuf = NULL;
}

static void ygtk_cell_renderer_text_pixbuf_finalize (GObject *object)
{
	YGtkCellRendererTextPixbuf *tpcell = YGTK_CELL_RENDERER_TEXT_PIXBUF (object);
	if (tpcell->text) {
		g_free (tpcell->text);
		tpcell->text = NULL;
	}
	if (tpcell->pixbuf) {
		g_object_unref (G_OBJECT (tpcell->pixbuf));
		tpcell->pixbuf = NULL;
	}
	G_OBJECT_CLASS (ygtk_cell_renderer_text_pixbuf_parent_class)->finalize (object);
}

static void ygtk_cell_renderer_text_pixbuf_get_property (GObject *object,
	guint param_id, GValue *value, GParamSpec *pspec)
{
	YGtkCellRendererTextPixbuf *tpcell = YGTK_CELL_RENDERER_TEXT_PIXBUF (object);
	switch (param_id) {
		case PROP_TEXT:
			g_value_set_string (value, tpcell->text);
			break;
		case PROP_PIXBUF:
			g_value_set_object (value, G_OBJECT (tpcell->pixbuf));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
			break;
	}
}

static void ygtk_cell_renderer_text_pixbuf_set_property (GObject *object,
	guint param_id, const GValue *value, GParamSpec *pspec)
{
	YGtkCellRendererTextPixbuf *tpcell = YGTK_CELL_RENDERER_TEXT_PIXBUF (object);
	switch (param_id) {
		case PROP_TEXT:
			if (tpcell->text)
				g_free (tpcell->text);
			tpcell->text = g_strdup (g_value_get_string (value));
			break;
		case PROP_PIXBUF:
			if (tpcell->pixbuf)
				g_object_unref (G_OBJECT (tpcell->pixbuf));
			tpcell->pixbuf = (GdkPixbuf *) g_value_dup_object (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
			break;
	}
}

static PangoLayout *create_layout (YGtkCellRendererTextPixbuf *tpcell, GtkWidget *widget)
{
	if (tpcell->text)
		return gtk_widget_create_pango_layout (widget, tpcell->text);
	return NULL;
}

static void ygtk_cell_renderer_text_pixbuf_get_size (GtkCellRenderer *cell,
	GtkWidget *widget, GdkRectangle *cell_area, gint *xoffset, gint *yoffset,
	gint *width, gint *height)
{
	YGtkCellRendererTextPixbuf *tpcell = YGTK_CELL_RENDERER_TEXT_PIXBUF (cell);

	// will be calculated at expose, as both pixbuf and text have their offsets...
	if (xoffset) *xoffset = 0;
	if (yoffset) *yoffset = 0;
	if (!width || !height)
		return;
	*width = 0;
	*height = 0;

	if (tpcell->pixbuf) {
		*width += gdk_pixbuf_get_width (tpcell->pixbuf);
		*height = MAX (*height, gdk_pixbuf_get_height (tpcell->pixbuf));
	}
	if (tpcell->text) {
		if (tpcell->pixbuf)
			*width += PIXBUF_TEXT_SPACING;

		PangoLayout *layout = create_layout (tpcell, widget);
		int lw, lh;
		pango_layout_get_pixel_size (layout, &lw, &lh);
		*width += lw;
		*height = MAX (*height, lh);

		g_object_unref (G_OBJECT (layout));
	}
	*width += cell->xpad*2;
	*height += cell->ypad*2 + 2;
}

static void ygtk_cell_renderer_text_pixbuf_render (GtkCellRenderer *cell,
	GdkDrawable *window, GtkWidget *widget, GdkRectangle *background_area,
	GdkRectangle *cell_area, GdkRectangle *expose_area, GtkCellRendererState flags)
{
	YGtkCellRendererTextPixbuf *tpcell = YGTK_CELL_RENDERER_TEXT_PIXBUF (cell);

	GtkStateType state;
	// like GtkCellRendererText...
	if (!cell->sensitive)
		state = GTK_STATE_INSENSITIVE;
	else if ((flags & GTK_CELL_RENDERER_SELECTED) == GTK_CELL_RENDERER_SELECTED) {
		gboolean has_focus = GTK_WIDGET_HAS_FOCUS (widget);
		state = has_focus ? GTK_STATE_SELECTED : GTK_STATE_ACTIVE;
	}
	else if ((flags & GTK_CELL_RENDERER_PRELIT) == GTK_CELL_RENDERER_PRELIT &&
	         GTK_WIDGET_STATE (widget) == GTK_STATE_PRELIGHT) {
		state = GTK_STATE_PRELIGHT;
	}
	else {
		if (GTK_WIDGET_STATE (widget) == GTK_STATE_INSENSITIVE)
			state = GTK_STATE_INSENSITIVE;
		else
			state = GTK_STATE_NORMAL;
	}

	int x = cell_area->x, y = cell_area->y;

	gfloat xalign = cell->xalign, yalign = cell->yalign;
	if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
		xalign = 1.0 - xalign;

	if (tpcell->pixbuf) {
		int w, h;
		w = gdk_pixbuf_get_width (tpcell->pixbuf);
		h = gdk_pixbuf_get_height (tpcell->pixbuf);

		int xoffset, yoffset;
		xoffset = xalign * (cell_area->width - (w + (2*cell->xpad)));
		yoffset = yalign * (cell_area->height - (h + (2*cell->ypad)));

		cairo_t *cr = gdk_cairo_create (window);
		gdk_cairo_set_source_pixbuf (cr, tpcell->pixbuf, x+xoffset, y+yoffset);

		cairo_rectangle (cr, x+xoffset, y+yoffset, w, h);
		cairo_fill (cr);
		cairo_destroy (cr);

		x += w + PIXBUF_TEXT_SPACING;
	}

	if (tpcell->text) {
		PangoLayout *layout = create_layout (tpcell, widget);

		PangoRectangle rect;
		pango_layout_get_pixel_extents (layout, NULL, &rect);

		int xoffset, yoffset;
		xoffset = xalign * (cell_area->width - (rect.width + (2*cell->xpad)));
		yoffset = yalign * (cell_area->height - (rect.height + (2*cell->ypad)));

		GtkStyle *style = gtk_widget_get_style (widget);
		gtk_paint_layout (style, window, state, TRUE, expose_area, widget,
		                   "cellrenderertext", x+xoffset, y+yoffset, layout);

		g_object_unref (G_OBJECT (layout));
	}
}

GtkCellRenderer *ygtk_cell_renderer_text_pixbuf_new (void)
{
    return g_object_new (YGTK_TYPE_CELL_RENDERER_TEXT_PIXBUF, NULL);
}

static void ygtk_cell_renderer_text_pixbuf_class_init (YGtkCellRendererTextPixbufClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	object_class->get_property = ygtk_cell_renderer_text_pixbuf_get_property;
	object_class->set_property = ygtk_cell_renderer_text_pixbuf_set_property;
	object_class->finalize = ygtk_cell_renderer_text_pixbuf_finalize;

	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);
	cell_class->get_size = ygtk_cell_renderer_text_pixbuf_get_size;
	cell_class->render   = ygtk_cell_renderer_text_pixbuf_render;

	g_object_class_install_property (object_class, PROP_TEXT,
		g_param_spec_string ("text", "Text", "The text", NULL,
		G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class, PROP_PIXBUF,
		g_param_spec_object ("pixbuf", "Image", "Side image", GDK_TYPE_PIXBUF,
		G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB));
}

