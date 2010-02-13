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
#define DEPRESS_PAD 2

enum {
	PROP_0,
	PROP_ACTIVE,
};

static guint toggle_cell_signal = 0;

G_DEFINE_TYPE (YGtkCellRendererButton, ygtk_cell_renderer_button, YGTK_TYPE_CELL_RENDERER_TEXT_PIXBUF)

static void ygtk_cell_renderer_button_init (YGtkCellRendererButton *bcell)
{
	bcell->active = FALSE;
	GtkCellRenderer *cell = GTK_CELL_RENDERER (bcell);
	cell->xpad = BORDER;
	cell->ypad = BORDER;
	cell->mode = GTK_CELL_RENDERER_MODE_ACTIVATABLE;
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
		}
	}
	else
		G_OBJECT_CLASS (ygtk_cell_renderer_button_parent_class)->set_property (
			object, param_id, value, pspec);
}

static void ygtk_cell_renderer_button_get_size (GtkCellRenderer *cell,
	GtkWidget *widget, GdkRectangle *cell_area, gint *xoffset, gint *yoffset,
	gint *width, gint *height)
{
	GTK_CELL_RENDERER_CLASS (ygtk_cell_renderer_button_parent_class)->get_size (
		cell, widget, NULL, NULL, NULL, width, height);
	if (xoffset) *xoffset = 0;
	if (yoffset) *yoffset = 0;
}

static void ygtk_cell_renderer_button_render (GtkCellRenderer *cell,
	GdkDrawable *window, GtkWidget *widget, GdkRectangle *background_area,
	GdkRectangle *cell_area, GdkRectangle *expose_area, GtkCellRendererState flags)
{
	YGtkCellRendererButton *bcell = YGTK_CELL_RENDERER_BUTTON (cell);

	gboolean has_focus = FALSE;
	if (flags & GTK_CELL_RENDERER_SELECTED)
		has_focus = GTK_WIDGET_HAS_FOCUS (widget);

	GtkStateType state = GTK_STATE_NORMAL;
	if (!cell->sensitive || GTK_WIDGET_STATE (widget) == GTK_STATE_INSENSITIVE)
		state = GTK_STATE_INSENSITIVE;
	else if ((flags & GTK_CELL_RENDERER_PRELIT))
		state = GTK_STATE_PRELIGHT;

	GtkShadowType shadow = GTK_SHADOW_OUT;
	if (bcell->active) {
		shadow = GTK_SHADOW_IN;
		state = GTK_STATE_ACTIVE;
	}

	int width, height;
	GTK_CELL_RENDERER_CLASS (ygtk_cell_renderer_button_parent_class)->get_size (
		cell, widget, NULL, NULL, NULL, &width, &height);
	int x = cell_area->x + OUTER_BORDER;
	width -= OUTER_BORDER*2;
	int y = cell_area->y + (cell_area->height - height)/2 + OUTER_BORDER;
	height -= OUTER_BORDER*2;

	gtk_paint_box (widget->style, window, state, shadow, expose_area, widget,
		"button", x, y, width, height);

	int cell_area_x = cell_area->x, cell_area_y = cell_area->y;
	if (bcell->active) {
		cell_area->x += DEPRESS_PAD;
		cell_area->y += DEPRESS_PAD;
	}

	GTK_CELL_RENDERER_CLASS (ygtk_cell_renderer_button_parent_class)->render (
		cell, window, widget, background_area, cell_area, expose_area, 0);

	cell_area->x = cell_area_x; cell_area->y = cell_area_y;
}

static gboolean ygtk_cell_renderer_button_activate (GtkCellRenderer *cell,
	GdkEvent *event, GtkWidget *widget, const gchar *path, GdkRectangle *background_area,
	GdkRectangle *cell_area, GtkCellRendererState flags)
{
	g_signal_emit (cell, toggle_cell_signal, 0, path);
	return TRUE;
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

	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);
	cell_class->get_size = ygtk_cell_renderer_button_get_size;
	cell_class->render   = ygtk_cell_renderer_button_render;
	cell_class->activate = ygtk_cell_renderer_button_activate;

	g_object_class_install_property (object_class, PROP_ACTIVE,
		g_param_spec_boolean ("active", "Toggle state", "The toggle state of the button",
			FALSE, G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB));

	toggle_cell_signal = g_signal_new ("toggled", G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (YGtkCellRendererButtonClass, toggled),
		NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
}

