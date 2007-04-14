//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

/* YGtkCellRendererArrow widget */
// check the header file for information about this widget

#include "ygtkcellrendererarrow.h"

#define ARROW_WIDTH  18
#define ARROW_HEIGHT 14

static void ygtk_cell_renderer_arrow_get_property (GObject *object,
	guint param_id, GValue *value, GParamSpec *pspec);
static void ygtk_cell_renderer_arrow_set_property (GObject *object,
	guint param_id, const GValue *value, GParamSpec *pspec);

static void ygtk_cell_renderer_arrow_get_size (GtkCellRenderer *cell,
	GtkWidget *widget, GdkRectangle *cell_area, gint *x_offset, gint *y_offset,
	gint *width, gint*height);
static void ygtk_cell_renderer_arrow_render (GtkCellRenderer *cell,
	GdkWindow *window, GtkWidget *widget, GdkRectangle *background_area,
	GdkRectangle *cell_area, GdkRectangle *expose_area, GtkCellRendererState flags);
static gboolean ygtk_cell_renderer_arrow_activate (GtkCellRenderer *cell,
	GdkEvent *event, GtkWidget *widget, const gchar *path,
	GdkRectangle *background_area, GdkRectangle *cell_area, GtkCellRendererState flags);

enum {
  PROP_0,
  PROP_CAN_GO_UP,
  PROP_CAN_GO_DOWN
};

static guint pressed_signal;
// VOID:UINT, STRING marshal
static void ygtk_marshal_VOID__UINT_STRING (GClosure *closure, GValue *return_value,
	guint n_param_values, const GValue *param_values, gpointer invocation_hint,
	gpointer marshal_data)
{
	typedef void (*GMarshalFunc_VOID__UINT_STRING) (gpointer data1, guint arg_1,
	                                                gpointer arg_2, gpointer data2);
	register GMarshalFunc_VOID__UINT_STRING callback;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;

	g_return_if_fail (n_param_values == 3);
	if (G_CCLOSURE_SWAP_DATA (closure)) {
		data1 = closure->data;
		data2 = g_value_peek_pointer (param_values + 0);
	}
	else {
		data1 = g_value_peek_pointer (param_values + 0);
		data2 = closure->data;
	}
	callback = (GMarshalFunc_VOID__UINT_STRING) (marshal_data ? marshal_data : cc->callback);
	callback (data1, g_value_get_uint (param_values + 1),
	          (char*) g_value_get_string (param_values + 2), data2);
}

G_DEFINE_TYPE (YGtkCellRendererArrow, ygtk_cell_renderer_arrow, GTK_TYPE_CELL_RENDERER)
static void ygtk_cell_renderer_arrow_init (YGtkCellRendererArrow *cell_arrow)
{
	GTK_CELL_RENDERER (cell_arrow)->mode = GTK_CELL_RENDERER_MODE_ACTIVATABLE;
	GTK_CELL_RENDERER (cell_arrow)->xalign = 0.0;
	GTK_CELL_RENDERER (cell_arrow)->yalign = 0.5;
	GTK_CELL_RENDERER (cell_arrow)->xpad = 0;
	GTK_CELL_RENDERER (cell_arrow)->ypad = 0;

	cell_arrow->can_go_up = cell_arrow->can_go_down = TRUE;
}

static void ygtk_cell_renderer_arrow_class_init (YGtkCellRendererArrowClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);

	object_class->get_property = ygtk_cell_renderer_arrow_get_property;
	object_class->set_property = ygtk_cell_renderer_arrow_set_property;

	cell_class->get_size = ygtk_cell_renderer_arrow_get_size;
	cell_class->render   = ygtk_cell_renderer_arrow_render;
	cell_class->activate = ygtk_cell_renderer_arrow_activate;

	g_object_class_install_property (object_class, PROP_CAN_GO_UP,
		g_param_spec_boolean ("can-go-up", "Can Go Up",
		"Whether the up arrow can be pressed", TRUE,
		G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB));
	g_object_class_install_property (object_class, PROP_CAN_GO_DOWN,
		g_param_spec_boolean ("can-go-down", "Can Go Down",
		"Whether the down arrow can be pressed", TRUE,
		G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB));

	/* signal sent when an arrow is pressed */
	pressed_signal = g_signal_new ("pressed", G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (YGtkCellRendererArrowClass, pressed),
		NULL, NULL, ygtk_marshal_VOID__UINT_STRING, G_TYPE_NONE, 2,
		G_TYPE_UINT, G_TYPE_STRING);
}

static void ygtk_cell_renderer_arrow_get_property (GObject *object,
	guint param_id, GValue *value, GParamSpec *pspec)
{
	YGtkCellRendererArrow *cell_arrow = YGTK_CELL_RENDERER_ARROW (object);
	switch (param_id) {
		case PROP_CAN_GO_UP:
			g_value_set_boolean (value, cell_arrow->can_go_up);
			break;
		case PROP_CAN_GO_DOWN:
			g_value_set_boolean (value, cell_arrow->can_go_down);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
			break;
	}
}

static void ygtk_cell_renderer_arrow_set_property (GObject *object,
	guint param_id, const GValue *value, GParamSpec *pspec)
{
	YGtkCellRendererArrow *cell_arrow = YGTK_CELL_RENDERER_ARROW (object);
	switch (param_id) {
		case PROP_CAN_GO_UP:
			cell_arrow->can_go_up = g_value_get_boolean (value);
			break;
		case PROP_CAN_GO_DOWN:
			cell_arrow->can_go_down = g_value_get_boolean (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
			break;
	}
}

GtkCellRenderer *ygtk_cell_renderer_arrow_new()
{ return g_object_new (YGTK_TYPE_CELL_RENDERER_ARROW, NULL); }

static void ygtk_cell_renderer_arrow_get_size (GtkCellRenderer *cell,
	GtkWidget *widget, GdkRectangle *cell_area, gint *x_offset, gint *y_offset,
	gint *width, gint *height)
{
	int w, h;
	w = cell->xpad*2 + ARROW_WIDTH;
	h = cell->ypad*2 + ARROW_HEIGHT*2 + 8;

	if (width)
		*width = w;
	if (height)
		*height = h;

	if (x_offset)
		*x_offset = MAX (cell->xalign * (cell_area->width - w), 0);
	if (y_offset)
		*y_offset = MAX (cell->yalign * (cell_area->height - h), 0);
}

static void draw_arrow (GdkWindow *window, GtkArrowType arrow, GdkColor *color,
                        GdkRectangle *area, gint x, gint y, gint width, gint height)
{
	/* Adapted from gtkstyle.c. We should avoid using gtk_style_paint_arrow(),
	   since some styles may draw weird arrows. It would also not be nice to
	   specify colors. And we may want to improve the arrow. */

	cairo_t *cr = gdk_cairo_create (window);
	gdk_cairo_set_source_color (cr, color);

	if (area) {
		gdk_cairo_rectangle (cr, area);
		cairo_clip (cr);
	}

	if (arrow == GTK_ARROW_DOWN) {
		cairo_move_to (cr, x, y);
		cairo_line_to (cr, x + width, y);
		cairo_line_to (cr, x + width / 2, y + height);
	}
	else /*if (arrow_type == GTK_ARROW_UP)*/ {
		cairo_move_to (cr, x, y + height);
		cairo_line_to (cr, x + width / 2, y);
		cairo_line_to (cr, x + width, y + height);
	}

	cairo_close_path (cr);
	cairo_fill (cr);

	cairo_destroy (cr);
}

static void ygtk_cell_renderer_arrow_render (GtkCellRenderer *cell,
	GdkDrawable *window, GtkWidget *widget, GdkRectangle *background_area,
	GdkRectangle *cell_area, GdkRectangle *expose_area, GtkCellRendererState flags)
{
	YGtkCellRendererArrow *cell_arrow = YGTK_CELL_RENDERER_ARROW (cell);
	int x, y, w = ARROW_WIDTH, h = ARROW_HEIGHT;
	ygtk_cell_renderer_arrow_get_size (cell, widget, cell_area, &x, &y, NULL, NULL);
	x += cell_area->x;
	y += cell_area->y;

	static GdkColor grey = { 0, 225 << 8, 225 << 8, 225 << 8 };

	// a bit ugly, but let's use a cycle here to avoid calling a function
	int i;
	GtkArrowType arrow = GTK_ARROW_UP;
	// doesn't seem to be a way to see if the arrow is being pressed or hovered
	gboolean cell_selected = flags & GTK_CELL_RENDERER_SELECTED;
	gboolean sensitive;
	for (i = 0; i < 2; i++) {
		sensitive = cell->sensitive &&
		            (i == 0 ? cell_arrow->can_go_up : cell_arrow->can_go_down);

		if (!sensitive) {
			if (!cell_selected)
				draw_arrow (window, arrow, &grey, expose_area, x, y, w, h);
		}
		else {  // normal
			if (cell_selected) {
				GdkColor *color = &widget->style->fg [GTK_STATE_SELECTED];
				draw_arrow (window, arrow, color, expose_area, x, y, w, h);
			}
			else {
				GdkColor *color = &widget->style->bg [GTK_STATE_SELECTED];
				draw_arrow (window, arrow, color, expose_area, x, y, w, h);
			}
		}

		y += h + 4;
		arrow = GTK_ARROW_DOWN;
	}
}

static gboolean ygtk_cell_renderer_arrow_activate (GtkCellRenderer *cell,
	GdkEvent *event, GtkWidget *widget, const gchar *path,
	GdkRectangle *background_area, GdkRectangle *cell_area, GtkCellRendererState flags)
{
	YGtkCellRendererArrow *cell_arrow = YGTK_CELL_RENDERER_ARROW (cell);

	if (event->type == GDK_BUTTON_PRESS) {
		int pressed_x, pressed_y;
		pressed_x = event->button.x - cell_area->x;
		pressed_y = event->button.y - cell_area->y;

		int ox, oy, w, h;
		ygtk_cell_renderer_arrow_get_size (cell, widget, cell_area, &ox, &oy, &w, &h);
		if (pressed_x >= ox && pressed_x <= ox + w &&
		    pressed_y >= oy && pressed_y <= oy + h) {
			guint arrow_type = GTK_ARROW_NONE;
			h = ARROW_HEIGHT;

			if (cell_arrow->can_go_up && pressed_y <= h)
				arrow_type = GTK_ARROW_UP;
			else if (cell_arrow->can_go_down && pressed_y >= h+8)
				arrow_type = GTK_ARROW_DOWN;

			if (arrow_type != GTK_ARROW_NONE) {
				g_signal_emit (cell, pressed_signal, 0, arrow_type, path);
				return TRUE;
			}
		}
	}
	return FALSE;
}
