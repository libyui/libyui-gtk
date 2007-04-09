//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

/* YGtkCellRendererArrow widget */
// check the header file for information about this widget

#include "ygtkcellrendererarrow.h"
// images
#include "cell-spin-up.xpm"
#include "cell-spin-down.xpm"

static void ygtk_cell_renderer_arrow_finalize (GObject *object);

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
	cell_arrow->up_arrow_pixbuf = gdk_pixbuf_new_from_xpm_data (cell_spin_up_xpm);
	cell_arrow->down_arrow_pixbuf = gdk_pixbuf_new_from_xpm_data (cell_spin_down_xpm);
}

static void ygtk_cell_renderer_arrow_class_init (YGtkCellRendererArrowClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);

	object_class->finalize = ygtk_cell_renderer_arrow_finalize;
	object_class->get_property = ygtk_cell_renderer_arrow_get_property;
	object_class->set_property = ygtk_cell_renderer_arrow_set_property;

	cell_class->get_size = ygtk_cell_renderer_arrow_get_size;
	cell_class->render = ygtk_cell_renderer_arrow_render;
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

static void ygtk_cell_renderer_arrow_finalize (GObject *object)
{
	YGtkCellRendererArrow *cell_arrow = YGTK_CELL_RENDERER_ARROW (object);
	if (cell_arrow->up_arrow_pixbuf)
		g_object_unref (G_OBJECT (cell_arrow->up_arrow_pixbuf));
	if (cell_arrow->down_arrow_pixbuf)
		g_object_unref (G_OBJECT (cell_arrow->down_arrow_pixbuf));

	G_OBJECT_CLASS (ygtk_cell_renderer_arrow_parent_class)->finalize (object);
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

static gint get_arrow_height (YGtkCellRendererArrow *cell_arrow)
{
	return gdk_pixbuf_get_height (cell_arrow->up_arrow_pixbuf);
}

static void ygtk_cell_renderer_arrow_get_size (GtkCellRenderer *cell,
	GtkWidget *widget, GdkRectangle *cell_area, gint *x_offset, gint *y_offset,
	gint *width, gint *height)
{
	YGtkCellRendererArrow *cell_arrow = YGTK_CELL_RENDERER_ARROW (cell);

	int w, h;
	if (cell_arrow->can_go_up || cell_arrow->can_go_down) {
		w = cell->xpad*2 + gdk_pixbuf_get_width (cell_arrow->up_arrow_pixbuf);
		h = cell->ypad*2 + get_arrow_height (cell_arrow)*2 + 8;
	}
	else
		w = h = 0;

	if (width)
		*width = w;
	if (height)
		*height = h;

	if (x_offset)
		*x_offset = MAX (cell->xalign * (cell_area->width - w), 0);
	if (y_offset)
		*y_offset = MAX (cell->yalign * (cell_area->height - h), 0);
}

static void ygtk_cell_renderer_arrow_render (GtkCellRenderer *cell,
	GdkDrawable *window, GtkWidget *widget, GdkRectangle *background_area,
	GdkRectangle *cell_area, GdkRectangle *expose_area, GtkCellRendererState flags)
{
	YGtkCellRendererArrow *cell_arrow = YGTK_CELL_RENDERER_ARROW (cell);
	int x, y, h = get_arrow_height (cell_arrow);
	ygtk_cell_renderer_arrow_get_size (cell, widget, cell_area, &x, &y, NULL, NULL);
	x += cell_area->x;
	y += cell_area->y;

	// TODO:
	//if (!cell->sensitive)
	//if (flags & GTK_CELL_RENDERER_SELECTED)

	if (cell_arrow->can_go_up)
		gdk_draw_pixbuf (window, NULL, cell_arrow->up_arrow_pixbuf, 0, 0,
		                 x, y, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
	if (cell_arrow->can_go_down)
		gdk_draw_pixbuf (window, NULL, cell_arrow->down_arrow_pixbuf, 0, 0,
		                 x, y + h + 4, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
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
			h = get_arrow_height (cell_arrow);

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
