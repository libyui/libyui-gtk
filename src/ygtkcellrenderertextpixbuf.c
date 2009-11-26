/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkCellRendererTextPixbuf widget */
// check the header file for information about this widget

#include <config.h>
#include "ygtkcellrenderertextpixbuf.h"

extern GdkPixbuf *ygutils_setOpacity (const GdkPixbuf *src, int opacity, gboolean alpha);

#define PIXBUF_TEXT_SPACING 4

enum {
	PROP_0,
	PROP_PIXBUF,
	PROP_STOCK_ID,
	PROP_STOCK_SIZE,
};

G_DEFINE_TYPE (YGtkCellRendererTextPixbuf, ygtk_cell_renderer_text_pixbuf, GTK_TYPE_CELL_RENDERER_TEXT)

static void ygtk_cell_renderer_text_pixbuf_init (YGtkCellRendererTextPixbuf *tpcell)
{
	GtkCellRenderer *cell = GTK_CELL_RENDERER (tpcell);
	cell->mode = GTK_CELL_RENDERER_MODE_ACTIVATABLE;
	cell->xpad = cell->ypad = 0;
	cell->xalign = 0;
	cell->yalign = 0.5;
	tpcell->stock_size = GTK_ICON_SIZE_MENU;
}

static void unset_image_properties (YGtkCellRendererTextPixbuf *cell)
{
	if (cell->stock_id) {
		g_free (cell->stock_id);
		cell->stock_id = NULL;
	}
	if (cell->pixbuf) {
		g_object_unref (cell->pixbuf);
		cell->pixbuf = NULL;
	}
}

static void ygtk_cell_renderer_text_pixbuf_finalize (GObject *object)
{
	YGtkCellRendererTextPixbuf *tpcell = YGTK_CELL_RENDERER_TEXT_PIXBUF (object);
	unset_image_properties (tpcell);
	G_OBJECT_CLASS (ygtk_cell_renderer_text_pixbuf_parent_class)->finalize (object);
}

static void ygtk_cell_renderer_text_pixbuf_get_property (GObject *object,
	guint param_id, GValue *value, GParamSpec *pspec)
{
	if (pspec->owner_type == YGTK_TYPE_CELL_RENDERER_TEXT_PIXBUF) {
		YGtkCellRendererTextPixbuf *tpcell = YGTK_CELL_RENDERER_TEXT_PIXBUF (object);
		switch (param_id) {
			case PROP_PIXBUF:
				g_value_set_object (value, G_OBJECT (tpcell->pixbuf));
				break;
			case PROP_STOCK_ID:
				g_value_set_string (value, tpcell->stock_id);
				break;
			case PROP_STOCK_SIZE:
				g_value_set_uint (value, tpcell->stock_size);
				break;
		}
	}
	else
		G_OBJECT_CLASS (ygtk_cell_renderer_text_pixbuf_parent_class)->get_property (
			object, param_id, value, pspec);
}

static void ygtk_cell_renderer_text_pixbuf_set_property (GObject *object,
	guint param_id, const GValue *value, GParamSpec *pspec)
{
	if (pspec->owner_type == YGTK_TYPE_CELL_RENDERER_TEXT_PIXBUF) {
		YGtkCellRendererTextPixbuf *tpcell = YGTK_CELL_RENDERER_TEXT_PIXBUF (object);
		switch (param_id) {
			case PROP_PIXBUF:
				unset_image_properties (tpcell);
				tpcell->pixbuf = (GdkPixbuf *) g_value_dup_object (value);
				break;
			case PROP_STOCK_ID:
				unset_image_properties (tpcell);
				tpcell->stock_id = g_value_dup_string (value);
				break;
			case PROP_STOCK_SIZE:
				tpcell->stock_size = g_value_get_uint (value);
				break;
		}
	}
	else
		G_OBJECT_CLASS (ygtk_cell_renderer_text_pixbuf_parent_class)->set_property (
			object, param_id, value, pspec);
}

static PangoLayout *create_layout (YGtkCellRendererTextPixbuf *tpcell, GtkWidget *widget)
{
	GtkCellRendererText *tcell = GTK_CELL_RENDERER_TEXT (tpcell);
	if (tcell->text)
		return gtk_widget_create_pango_layout (widget, tcell->text);
	return NULL;
}

static void ensure_pixbuf (YGtkCellRendererTextPixbuf *cell, GtkWidget *widget)
{
	if (cell->stock_id && !cell->pixbuf)
		cell->pixbuf = gtk_widget_render_icon (widget, cell->stock_id, cell->stock_size, NULL);
}

static void ygtk_cell_renderer_text_pixbuf_get_size_full (GtkCellRenderer *cell,
	GtkWidget *widget, GdkRectangle *cell_area, gint *_xoffset, gint *_yoffset,
	gint *_width, gint *_height, gint *_pixbuf_xoffset, gint *_pixbuf_yoffset,
	gint *_pixbuf_width, gint *_pixbuf_height, gint *_text_xoffset, gint *_text_yoffset)
{
	YGtkCellRendererTextPixbuf *tpcell = YGTK_CELL_RENDERER_TEXT_PIXBUF (cell);
	ensure_pixbuf (tpcell, widget);

	int pixbuf_width = 0, pixbuf_height = 0;
	if (tpcell->pixbuf) {
		pixbuf_width = gdk_pixbuf_get_width (tpcell->pixbuf);
		pixbuf_height = gdk_pixbuf_get_height (tpcell->pixbuf);
	}

	PangoLayout *layout = create_layout (tpcell, widget);
	int text_width = 0, text_height = 0;
	if (layout) {
		PangoRectangle rect;
		pango_layout_get_pixel_extents (layout, NULL, &rect);
		text_width = rect.x + rect.width;
		text_height = rect.y + rect.height;
	}

	int width, height;
	width = pixbuf_width + text_width;
	if (pixbuf_width && text_width)
		width += PIXBUF_TEXT_SPACING;
	height = MAX (pixbuf_height, text_height);

	if (cell_area) {
		gboolean reverse = gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL;
		gfloat xalign = cell->xalign, yalign = cell->yalign;
		if (reverse)
			xalign = 1.0 - xalign;

		int cell_width = cell_area->width - cell->xpad*2,
		    cell_height = cell_area->height - cell->ypad*2;
		int xoffset = (xalign * (cell_width - width)) + cell->xpad;
		int yoffset = (yalign * (cell_height - height)) + cell->ypad;
		if (_xoffset) *_xoffset = xoffset;
		if (_yoffset) *_yoffset = yoffset;

		int text_x = xoffset, text_y;
		if (pixbuf_width && !reverse)
			text_x += (pixbuf_width + PIXBUF_TEXT_SPACING);
		text_y = (yalign * (cell_height - text_height)) + cell->ypad;

		int pixbuf_x = xoffset, pixbuf_y;
		if (text_width && reverse)
			pixbuf_x += (text_width + PIXBUF_TEXT_SPACING);
		pixbuf_y = (yalign * (cell_height - pixbuf_height)) + cell->ypad;

		if (_pixbuf_xoffset) *_pixbuf_xoffset = pixbuf_x;
		if (_pixbuf_yoffset) *_pixbuf_yoffset = pixbuf_y;
		if (_pixbuf_width) *_pixbuf_width = pixbuf_width;
		if (_pixbuf_height) *_pixbuf_height = pixbuf_height;
		if (_text_xoffset) *_text_xoffset = text_x;
		if (_text_yoffset) *_text_yoffset = text_y;
	}

	if (_width) *_width = width + (cell->xpad * 2);
	if (_height) *_height = height + (cell->ypad * 2);
}

static void ygtk_cell_renderer_text_pixbuf_get_size (GtkCellRenderer *cell,
	GtkWidget *widget, GdkRectangle *cell_area, gint *xoffset, gint *yoffset,
	gint *width, gint *height)
{
	ygtk_cell_renderer_text_pixbuf_get_size_full (cell, widget, cell_area,
		xoffset, yoffset, width, height, NULL, NULL, NULL, NULL, NULL, NULL);
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

	int text_xoffset, text_yoffset, pixbuf_xoffset, pixbuf_yoffset, pixbuf_width,
	    pixbuf_height;
	ygtk_cell_renderer_text_pixbuf_get_size_full (cell, widget, cell_area,
		NULL, NULL, NULL, NULL, &pixbuf_xoffset, &pixbuf_yoffset, &pixbuf_width,
		&pixbuf_height, &text_xoffset, &text_yoffset);

	// paint
	ensure_pixbuf (tpcell, widget);
	if (tpcell->pixbuf) {
		int x = cell_area->x + pixbuf_xoffset, y = cell_area->y + pixbuf_yoffset;
		cairo_t *cr = gdk_cairo_create (window);
		gdk_cairo_set_source_pixbuf (cr, tpcell->pixbuf, x, y);
		cairo_rectangle (cr, x, y, pixbuf_width, pixbuf_height);
		cairo_fill (cr);
		cairo_destroy (cr);
	}
	PangoLayout *layout = create_layout (tpcell, widget);
	if (layout) {
		int x = cell_area->x + text_xoffset, y = cell_area->y + text_yoffset;
		GtkStyle *style = gtk_widget_get_style (widget);
		gtk_paint_layout (style, window, state, TRUE, expose_area, widget,
			              "cellrenderertext", x, y, layout);
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

	GParamFlags readwrite_flag =
		G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB;
	g_object_class_install_property (object_class, PROP_PIXBUF,
		g_param_spec_object ("pixbuf", "Image", "Side image", GDK_TYPE_PIXBUF, readwrite_flag));
	g_object_class_install_property (object_class, PROP_STOCK_ID,
		g_param_spec_string ("stock-id", "Stock ID", "Stock icon to render", NULL, readwrite_flag));
	g_object_class_install_property (object_class, PROP_STOCK_SIZE,
		g_param_spec_uint ("stock-size", "Size", "GtkIconSize of the rendered icon",
		0, G_MAXUINT, GTK_ICON_SIZE_MENU, readwrite_flag));
}

