/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkCellRendererSideButton widget */
// check the header file for information about this widget

#include "ygtkcellrenderersidebutton.h"
#include <gtk/gtk.h>

enum {
	PROP_0,
	PROP_ACTIVE,
	PROP_STOCK_ID,
	PROP_BUTTON_VISIBLE,
};

static guint toggle_cell_signal = 0;

G_DEFINE_TYPE (YGtkCellRendererSideButton, ygtk_cell_renderer_side_button, GTK_TYPE_CELL_RENDERER_TEXT)

static void ygtk_cell_renderer_side_button_init (YGtkCellRendererSideButton *bcell)
{
}

static void free_pixbuf (YGtkCellRendererSideButton *cell)
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

static void ensure_pixbuf (YGtkCellRendererSideButton *cell, GtkWidget *widget)
{
	if (!cell->pixbuf && cell->stock_id)
		cell->pixbuf = gtk_widget_render_icon (widget, cell->stock_id, GTK_ICON_SIZE_BUTTON, NULL);
}

static void ygtk_cell_renderer_side_button_finalize (GObject *object)
{
	YGtkCellRendererSideButton *bcell = YGTK_CELL_RENDERER_SIDE_BUTTON (object);
	free_pixbuf (bcell);
	G_OBJECT_CLASS (ygtk_cell_renderer_side_button_parent_class)->finalize (object);
}

static void ygtk_cell_renderer_side_button_get_property (
	GObject *object, guint param_id, GValue *value, GParamSpec *pspec)
{
	if (pspec->owner_type == YGTK_TYPE_CELL_RENDERER_SIDE_BUTTON) {
		YGtkCellRendererSideButton *bcell = YGTK_CELL_RENDERER_SIDE_BUTTON (object);
		switch (param_id) {
			case PROP_ACTIVE:
				g_value_set_boolean (value, bcell->active);
				break;
			case PROP_STOCK_ID:
				g_value_set_string (value, bcell->stock_id);
				break;
			case PROP_BUTTON_VISIBLE:
				g_value_set_boolean (value, bcell->button_visible);
				break;
		}
	}
	else
		G_OBJECT_CLASS (ygtk_cell_renderer_side_button_parent_class)->get_property (
			object, param_id, value, pspec);
}

static void ygtk_cell_renderer_side_button_set_property (GObject *object,
	guint param_id, const GValue *value, GParamSpec *pspec)
{
	if (pspec->owner_type == YGTK_TYPE_CELL_RENDERER_SIDE_BUTTON) {
		YGtkCellRendererSideButton *bcell = YGTK_CELL_RENDERER_SIDE_BUTTON (object);
		switch (param_id) {
			case PROP_ACTIVE:
				bcell->active = g_value_get_boolean (value);
				break;
			case PROP_STOCK_ID:
				free_pixbuf (bcell);
				bcell->stock_id = g_value_dup_string (value);
				break;
			case PROP_BUTTON_VISIBLE:
				bcell->button_visible = g_value_get_boolean (value);
				break;
		}
	}
	else
		G_OBJECT_CLASS (ygtk_cell_renderer_side_button_parent_class)->set_property (
			object, param_id, value, pspec);
}

static void ygtk_cell_renderer_side_button_get_size (
	GtkCellRenderer *cell, GtkWidget *widget, GdkRectangle *cell_area,
	gint *x_offset, gint *y_offset, gint *width, gint *height)
{
	GTK_CELL_RENDERER_CLASS (ygtk_cell_renderer_side_button_parent_class)->get_size (
		cell, widget, cell_area, x_offset, y_offset, width, height);
	YGtkCellRendererSideButton *bcell = YGTK_CELL_RENDERER_SIDE_BUTTON (cell);
	if (bcell->button_visible) {
		gint icon_width, icon_height;
		gtk_icon_size_lookup (GTK_ICON_SIZE_BUTTON, &icon_width, &icon_height);
		*width += icon_width + 8 + 4;
	}
}

static void ygtk_cell_renderer_side_button_render (
	GtkCellRenderer *cell, GdkDrawable *window, GtkWidget *widget,
	GdkRectangle *background_area, GdkRectangle *cell_area,
	GdkRectangle *expose_area, GtkCellRendererState flags)
{
	YGtkCellRendererSideButton *bcell = YGTK_CELL_RENDERER_SIDE_BUTTON (cell);
	GdkRectangle text_area = *cell_area;
	gint icon_width, icon_height;
	if (bcell->button_visible) {
		gtk_icon_size_lookup (GTK_ICON_SIZE_BUTTON, &icon_width, &icon_height);
		text_area.width -= icon_width + 8 + 4;
	}
	GTK_CELL_RENDERER_CLASS (ygtk_cell_renderer_side_button_parent_class)->render (
		cell, window, widget, background_area, &text_area, expose_area, flags);

	if (bcell->button_visible) {
		GtkStateType state = GTK_STATE_NORMAL;
		GtkShadowType shadow = GTK_SHADOW_OUT;
		if (!cell->sensitive || GTK_WIDGET_STATE (widget) == GTK_STATE_INSENSITIVE)
			state = GTK_STATE_INSENSITIVE;
	/*	else if ((flags & GTK_CELL_RENDERER_PRELIT))
			state = GTK_STATE_PRELIGHT;*/
		if (bcell->active) {
			shadow = GTK_SHADOW_IN;
			state = GTK_STATE_ACTIVE;
		}

		GdkRectangle button_area = *cell_area;
		button_area.x = cell_area->x + cell_area->width - icon_width - 8;
		button_area.width = icon_width + 4;
		button_area.height = icon_height + 4;
		button_area.y = cell_area->y + ((cell_area->height - button_area.height) / 2);

		gtk_paint_box (widget->style, window, state, shadow, expose_area, widget,
			"button", button_area.x, button_area.y, button_area.width, button_area.height);

		GdkRectangle icon_area = button_area;
		icon_area.x += 2;
		icon_area.y += 2;
		if (bcell->active) {
			cell_area->x += 2;
			cell_area->y += 2;
		}

		ensure_pixbuf (bcell, widget);
		cairo_t *cr = gdk_cairo_create (window);
		gdk_cairo_set_source_pixbuf (cr, bcell->pixbuf, icon_area.x, icon_area.y);
		cairo_rectangle (cr, icon_area.x, icon_area.y, icon_width, icon_height);
		cairo_fill (cr);
		cairo_destroy (cr);
	}
}

static gboolean ygtk_cell_renderer_side_button_activate (
	GtkCellRenderer *cell, GdkEvent *event, GtkWidget *widget, const gchar *path,
	GdkRectangle *background_area, GdkRectangle *cell_area, GtkCellRendererState flags)
{
	g_signal_emit (cell, toggle_cell_signal, 0, path);
	return TRUE;
}

GtkCellRenderer *ygtk_cell_renderer_side_button_new (void)
{ return g_object_new (YGTK_TYPE_CELL_RENDERER_SIDE_BUTTON, NULL); }

gboolean ygtk_cell_renderer_side_button_get_active (YGtkCellRendererSideButton *cell)
{ return cell->active; }

static void ygtk_cell_renderer_side_button_class_init (YGtkCellRendererSideButtonClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	object_class->get_property = ygtk_cell_renderer_side_button_get_property;
	object_class->set_property = ygtk_cell_renderer_side_button_set_property;
	object_class->finalize = ygtk_cell_renderer_side_button_finalize;

	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);
	cell_class->get_size = ygtk_cell_renderer_side_button_get_size;
	cell_class->render   = ygtk_cell_renderer_side_button_render;
	cell_class->activate = ygtk_cell_renderer_side_button_activate;

	GParamFlags readwrite_flag =
		G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB;
	g_object_class_install_property (object_class, PROP_ACTIVE,
		g_param_spec_boolean ("active", "Toggle state", "The toggle state of the button",
			FALSE, readwrite_flag));
	g_object_class_install_property (object_class, PROP_STOCK_ID,
		g_param_spec_string ("stock-id", "Stock ID", "Stock icon to render", NULL, readwrite_flag));
	g_object_class_install_property (object_class, PROP_BUTTON_VISIBLE,
		g_param_spec_boolean ("button-visible", "Is Button Visible", "Whether to show side button", TRUE, readwrite_flag));

	toggle_cell_signal = g_signal_new ("toggled", G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (YGtkCellRendererSideButtonClass, toggled),
		NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
}

