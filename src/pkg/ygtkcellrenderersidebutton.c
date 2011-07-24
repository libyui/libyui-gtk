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

#define DEPRESS_PAD 1

static void ygtk_cell_renderer_side_button_init (YGtkCellRendererSideButton *bcell)
{ g_object_set(GTK_CELL_RENDERER(bcell), "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL); }

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
	GtkCellRenderer *cell, GtkWidget *widget, const GdkRectangle *cell_area,
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
	GtkCellRenderer *cell, cairo_t *cr, GtkWidget *widget,
	const GdkRectangle *background_area, const GdkRectangle *cell_area,
	GtkCellRendererState flags)
{
	YGtkCellRendererSideButton *bcell = YGTK_CELL_RENDERER_SIDE_BUTTON (cell);
	GdkRectangle text_area = *cell_area;
	gint icon_width, icon_height;
	if (bcell->button_visible) {
		gtk_icon_size_lookup (GTK_ICON_SIZE_BUTTON, &icon_width, &icon_height);
		if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
			text_area.x += icon_width + 8;
		text_area.width -= icon_width + 8 + 4;
	}

	GTK_CELL_RENDERER_CLASS (ygtk_cell_renderer_side_button_parent_class)->render (
		cell, cr, widget, background_area, &text_area, flags);

	if (bcell->button_visible) {
		GtkStateType state = GTK_STATE_NORMAL;
                gboolean sensitive = TRUE;
                g_object_get(cell, "sensitive", &sensitive, NULL);
		if (!sensitive || gtk_widget_get_state (widget) == GTK_STATE_INSENSITIVE)
			state = GTK_STATE_INSENSITIVE;
	/*	else if ((flags & GTK_CELL_RENDERER_PRELIT))
			state = GTK_STATE_PRELIGHT;*/
		if (bcell->active)
			state = GTK_STATE_ACTIVE;

		GtkStyleContext *style = gtk_widget_get_style_context(widget);
		gtk_style_context_save(style);
		gtk_style_context_set_state(style, state);
		gtk_style_context_add_class (style, GTK_STYLE_CLASS_BUTTON);

		GdkRectangle button_area = *cell_area;
		if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
			button_area.x = cell_area->x + 2;
		else
			button_area.x = cell_area->x + cell_area->width - icon_width - 8;
		button_area.width = icon_width + 4;
		button_area.height = icon_height + 4;
		button_area.y = cell_area->y + ((cell_area->height - button_area.height) / 2);

		gtk_render_background (style, cr, button_area.x, button_area.y, button_area.width, button_area.height);
		gtk_render_frame (style, cr, button_area.x, button_area.y, button_area.width, button_area.height);

		GdkRectangle icon_area = button_area;
		icon_area.x += 2;
		icon_area.y += 2;
		if (bcell->active) {
			icon_area.x += DEPRESS_PAD; icon_area.y += DEPRESS_PAD;
		}

		ensure_pixbuf (bcell, widget);
		gdk_cairo_set_source_pixbuf (cr, bcell->pixbuf, icon_area.x, icon_area.y);
		cairo_rectangle (cr, icon_area.x, icon_area.y, icon_width, icon_height);
		cairo_fill (cr);

		gtk_style_context_restore(style);
	}
}

static gboolean ygtk_cell_renderer_side_button_activate (
	GtkCellRenderer *cell, GdkEvent *event, GtkWidget *widget, const gchar *path,
	const GdkRectangle *background_area, const GdkRectangle *cell_area, GtkCellRendererState flags)
{
	YGtkCellRendererSideButton *bcell = YGTK_CELL_RENDERER_SIDE_BUTTON (cell);
	if (bcell->button_visible) {
		GdkEventButton *_event = (GdkEventButton *) event;
		gint icon_width, icon_height;
		gtk_icon_size_lookup (GTK_ICON_SIZE_BUTTON, &icon_width, &icon_height);
		int icon_x, x = _event->x - cell_area->x;
		if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
			icon_x = 2;
		else
			icon_x = cell_area->width - (icon_width+12);
		icon_width += 8;
		if (x >= icon_x && x <= icon_x + icon_width) {
			g_signal_emit (cell, toggle_cell_signal, 0, path);
			return TRUE;
		}
	}
	return FALSE;
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

