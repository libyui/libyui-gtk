/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkCellRendererButton widget */
// check the header file for information about this widget

#include "ygtkcellrendererbutton.h"
#include <gtk/gtk.h>

#define INNER_BORDER 4
#define OUTER_BORDER 1
#define BORDER (INNER_BORDER+OUTER_BORDER)
#define DEPRESS_PAD 1
#define PIXBUF_TEXT_SPACING 4

enum {
	PROP_0,
	PROP_ACTIVE,
	PROP_PIXBUF,
	PROP_ICON_NAME,
	PROP_STOCK_ID,
	PROP_ICON_SIZE,
};

static guint toggle_cell_signal = 0;

G_DEFINE_TYPE (YGtkCellRendererButton, ygtk_cell_renderer_button, GTK_TYPE_CELL_RENDERER_TEXT)

static void ygtk_cell_renderer_button_init (YGtkCellRendererButton *bcell)
{
	bcell->active = FALSE;
	GtkCellRenderer *cell = GTK_CELL_RENDERER (bcell);
        g_object_set(cell, "xpad", BORDER, "ypad", BORDER, "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
	bcell->icon_size = 16;
}

static void unset_image_properties (YGtkCellRendererButton *cell)
{
	if (cell->icon_name) {
		g_free (cell->icon_name);
		cell->icon_name = NULL;
	}
	if (cell->pixbuf) {
		g_object_unref (cell->pixbuf);
		cell->pixbuf = NULL;
	}
}

static void ygtk_cell_renderer_button_finalize (GObject *object)
{
	YGtkCellRendererButton *ccell = YGTK_CELL_RENDERER_BUTTON (object);
	unset_image_properties (ccell);
	G_OBJECT_CLASS (ygtk_cell_renderer_button_parent_class)->finalize (object);
}

static void ygtk_cell_renderer_button_get_property (GObject *object,
	guint param_id, GValue *value, GParamSpec *pspec)
{
	if (pspec->owner_type == YGTK_TYPE_CELL_RENDERER_BUTTON) {
		YGtkCellRendererButton *bcell = YGTK_CELL_RENDERER_BUTTON (object);
		switch (param_id) {
			case PROP_ACTIVE:
				g_value_set_boolean (value, bcell->active);
				break;
			case PROP_PIXBUF:
				g_value_set_object (value, G_OBJECT (bcell->pixbuf));
				break;
			case PROP_STOCK_ID:
				g_value_set_string (value, bcell->stock_id);
				break;
			case PROP_ICON_NAME:
				g_value_set_string (value, bcell->icon_name);
				break;
			case PROP_ICON_SIZE:
				g_value_set_uint (value, bcell->icon_size);
				break;
		}
	}
	else
		G_OBJECT_CLASS (ygtk_cell_renderer_button_parent_class)->get_property (
			object, param_id, value, pspec);
}

static void ygtk_cell_renderer_button_set_property (GObject *object,
	guint param_id, const GValue *value, GParamSpec *pspec)
{
	if (pspec->owner_type == YGTK_TYPE_CELL_RENDERER_BUTTON) {
		YGtkCellRendererButton *bcell = YGTK_CELL_RENDERER_BUTTON (object);
		switch (param_id) {
			case PROP_ACTIVE:
				bcell->active = g_value_get_boolean (value);
				break;
			case PROP_PIXBUF:
				unset_image_properties (bcell);
				bcell->pixbuf = (GdkPixbuf *) g_value_dup_object (value);
				break;
			case PROP_STOCK_ID:
				unset_image_properties (bcell);
				bcell->stock_id = g_value_dup_string (value);
				break;
			case PROP_ICON_NAME:
				unset_image_properties (bcell);
				bcell->icon_name = g_value_dup_string (value);
				break;
			case PROP_ICON_SIZE:
				bcell->icon_size = g_value_get_uint (value);
				break;
		}
	}
	else
		G_OBJECT_CLASS (ygtk_cell_renderer_button_parent_class)->set_property (
			object, param_id, value, pspec);
}

static PangoLayout *create_layout (YGtkCellRendererButton *bcell, GtkWidget *widget)
{
	GtkCellRendererText *tcell = GTK_CELL_RENDERER_TEXT (bcell);
	gchar *text = NULL;
	g_object_get (G_OBJECT (tcell), "text", &text, NULL);
	PangoLayout *layout = NULL;
	if (text) {
		layout = gtk_widget_create_pango_layout (widget, text);
		g_free(text);
	}
	return layout;
}

static void ensure_pixbuf (YGtkCellRendererButton *cell, GtkWidget *widget)
{
	if (!cell->pixbuf && (cell->icon_name || cell->stock_id)) {
		if (cell->icon_name) {
			GtkIconTheme *theme = gtk_icon_theme_get_default();
			GError *error = 0;
			cell->pixbuf = gtk_icon_theme_load_icon (theme, cell->icon_name,
				cell->icon_size, GTK_ICON_LOOKUP_FORCE_SIZE, &error);
			if (!cell->pixbuf)
				g_warning ("Couldn't load ygtk-cell-renderer-button icon: %s\nGtk: %s\n",
					cell->icon_name, error->message);
		}
		else // stock-id
			cell->pixbuf = gtk_widget_render_icon_pixbuf (
				widget, cell->stock_id, GTK_ICON_SIZE_BUTTON);
	}
}

static void ygtk_cell_renderer_button_get_size_full (GtkCellRenderer *cell,
	GtkWidget *widget, const GdkRectangle *cell_area, gint *_xoffset, gint *_yoffset,
	gint *_width, gint *_height, gint *_pixbuf_xoffset, gint *_pixbuf_yoffset,
	gint *_pixbuf_width, gint *_pixbuf_height, gint *_text_xoffset, gint *_text_yoffset)
{
	YGtkCellRendererButton *bcell = YGTK_CELL_RENDERER_BUTTON (cell);
	ensure_pixbuf (bcell, widget);

	int pixbuf_width = 0, pixbuf_height = 0;
	if (bcell->pixbuf) {
		pixbuf_width = gdk_pixbuf_get_width (bcell->pixbuf);
		pixbuf_height = gdk_pixbuf_get_height (bcell->pixbuf);
	}

	PangoLayout *layout = create_layout (bcell, widget);
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

        gfloat xalign, yalign;
        guint xpad, ypad;
        g_object_get(cell,
                     "xalign", &xalign,
                     "yalign", &yalign,
                     "xpad", &xpad,
                     "ypad", &ypad, NULL);

	if (cell_area) {
		gboolean reverse = gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL;
		if (reverse)
			xalign = 1.0 - xalign;

		int cell_width = cell_area->width - xpad*2,
		    cell_height = cell_area->height - ypad*2;
		int xoffset = (xalign * (cell_width - width)) + xpad;
		int yoffset = (yalign * (cell_height - height)) + ypad;
		if (_xoffset) *_xoffset = xoffset;
		if (_yoffset) *_yoffset = yoffset;

		int text_x = xoffset, text_y;
		if (pixbuf_width && !reverse)
			text_x += (pixbuf_width + PIXBUF_TEXT_SPACING);
		text_y = (yalign * (cell_height - text_height)) + ypad;

		int pixbuf_x = xoffset, pixbuf_y;
		if (text_width && reverse)
			pixbuf_x += (text_width + PIXBUF_TEXT_SPACING);
		pixbuf_y = (yalign * (cell_height - pixbuf_height)) + ypad;

		if (_pixbuf_xoffset) *_pixbuf_xoffset = pixbuf_x;
		if (_pixbuf_yoffset) *_pixbuf_yoffset = pixbuf_y;
		if (_pixbuf_width) *_pixbuf_width = pixbuf_width;
		if (_pixbuf_height) *_pixbuf_height = pixbuf_height;
		if (_text_xoffset) *_text_xoffset = text_x;
		if (_text_yoffset) *_text_yoffset = text_y;
	}

	if (_width) *_width = width + (xpad * 2);
	if (_height) *_height = height + (ypad * 2);
}

static void ygtk_cell_renderer_button_get_size (GtkCellRenderer *cell,
	GtkWidget *widget, const GdkRectangle *cell_area, gint *xoffset, gint *yoffset,
	gint *width, gint *height)
{
	ygtk_cell_renderer_button_get_size_full (cell, widget, cell_area,
		xoffset, yoffset, width, height, NULL, NULL, NULL, NULL, NULL, NULL);
}

static void ygtk_cell_renderer_button_render(
	GtkCellRenderer *cell, cairo_t *cr, GtkWidget *widget,
	const GdkRectangle *background_area, const GdkRectangle *cell_area,
	GtkCellRendererState flags)
{
	YGtkCellRendererButton *bcell = YGTK_CELL_RENDERER_BUTTON (cell);

	GtkStateType state = GTK_STATE_NORMAL;
	if (!gtk_cell_renderer_get_sensitive(cell) || gtk_widget_get_state (widget) == GTK_STATE_INSENSITIVE)
		state = GTK_STATE_INSENSITIVE;
	else if ((flags & GTK_CELL_RENDERER_PRELIT))
		state = GTK_STATE_PRELIGHT;
	if (bcell->active)
		state = GTK_STATE_ACTIVE;

	GtkStyleContext *style = gtk_widget_get_style_context(widget);
	gtk_style_context_save(style);
	gtk_style_context_set_state(style, state);
	gtk_style_context_add_class (style, GTK_STYLE_CLASS_BUTTON);

	int text_xoffset, text_yoffset, pixbuf_xoffset, pixbuf_yoffset, pixbuf_width,
	    pixbuf_height, width, height;
	ygtk_cell_renderer_button_get_size_full (cell, widget, cell_area,
		NULL, NULL, &width, &height, &pixbuf_xoffset, &pixbuf_yoffset, &pixbuf_width,
		&pixbuf_height, &text_xoffset, &text_yoffset);

	int x = cell_area->x + OUTER_BORDER + 1;
	int y = cell_area->y + (cell_area->height - height)/2 + OUTER_BORDER;
	width -= OUTER_BORDER*2; height -= OUTER_BORDER*2;

	gtk_render_background (style, cr, x, y, width, height);
	gtk_render_frame (style, cr, x, y, width, height);

	// paint content

	ensure_pixbuf (bcell, widget);
	if (bcell->pixbuf) {
		int _x = x + pixbuf_xoffset, _y = y + pixbuf_yoffset;
		if (bcell->active) {
			_x += DEPRESS_PAD; _y += DEPRESS_PAD;
		}
		gdk_cairo_set_source_pixbuf (cr, bcell->pixbuf, _x, _y);
		cairo_rectangle (cr, _x, _y, pixbuf_width, pixbuf_height);
		cairo_fill (cr);
	}

	PangoLayout *layout = create_layout (bcell, widget);
	if (layout) {
		int _x = cell_area->x + text_xoffset, _y = cell_area->y + text_yoffset;
		if (bcell->active) {
			_x += DEPRESS_PAD; _y += DEPRESS_PAD;
		}

		gtk_render_layout(style, cr, _x, _y, layout);
		g_object_unref (G_OBJECT (layout));
	}

	gtk_style_context_restore(style);
}

static gboolean ygtk_cell_renderer_button_activate (GtkCellRenderer *cell,
	GdkEvent *event, GtkWidget *widget, const gchar *path, const GdkRectangle *background_area,
	const GdkRectangle *cell_area, GtkCellRendererState flags)
{
	GdkEventButton *_event = &event->button;
	if (_event->x >= cell_area->x && _event->x <= cell_area->x + cell_area->width) {
		g_signal_emit (cell, toggle_cell_signal, 0, path);
		return TRUE;
	}
	return FALSE;
}

GtkCellRenderer *ygtk_cell_renderer_button_new (void)
{ return g_object_new (YGTK_TYPE_CELL_RENDERER_BUTTON, NULL); }

gboolean ygtk_cell_renderer_button_get_active (YGtkCellRendererButton *cell)
{ return cell->active; }

static void ygtk_cell_renderer_button_class_init (YGtkCellRendererButtonClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	object_class->get_property = ygtk_cell_renderer_button_get_property;
	object_class->set_property = ygtk_cell_renderer_button_set_property;
	object_class->finalize = ygtk_cell_renderer_button_finalize;

	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);
	cell_class->get_size = ygtk_cell_renderer_button_get_size;
	cell_class->render   = ygtk_cell_renderer_button_render;
	cell_class->activate = ygtk_cell_renderer_button_activate;

	GParamFlags readwrite_flag =
		G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB;
	g_object_class_install_property (object_class, PROP_ACTIVE,
		g_param_spec_boolean ("active", "Toggle state", "The toggle state of the button",
			FALSE, readwrite_flag));
	g_object_class_install_property (object_class, PROP_PIXBUF,
		g_param_spec_object ("pixbuf", "Image", "Side image", GDK_TYPE_PIXBUF, readwrite_flag));
	g_object_class_install_property (object_class, PROP_ICON_NAME,
		g_param_spec_string ("icon-name", "Icon name", "Theme icon to render", NULL, readwrite_flag));
	g_object_class_install_property (object_class, PROP_STOCK_ID,
		g_param_spec_string ("stock-id", "Stock id", "Stock icon to render", NULL, readwrite_flag));
	g_object_class_install_property (object_class, PROP_ICON_SIZE,
		g_param_spec_uint ("icon-size", "Size", "Size of the icon to render",
		0, G_MAXUINT, 22, readwrite_flag));

	toggle_cell_signal = g_signal_new ("toggled", G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (YGtkCellRendererButtonClass, toggled),
		NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
}

