/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkTooltip widget */
// check the header file for information about this widget

#include <config.h>
#include "ygtktooltip.h"
#include <gtk/gtk.h>

#define TOOLTIP_TIMEOUT 10000
#define POINTER_LENGTH  10

// header

#define YGTK_TYPE_TOOLTIP            (ygtk_tooltip_get_type ())
#define YGTK_TOOLTIP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                        YGTK_TYPE_TOOLTIP, YGtkTooltip))
#define YGTK_TOOLTIP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                        YGTK_TYPE_TOOLTIP, YGtkTooltipClass))
#define YGTK_IS_TOOLTIP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                        YGTK_TYPE_TOOLTIP))
#define YGTK_IS_TOOLTIP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                        YGTK_TYPE_TOOLTIP))
#define YGTK_TOOLTIP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                        YGTK_TYPE_TOOLTIP, YGtkTooltipClass))

typedef struct YGtkTooltip
{
	GtkWindow parent;
	// private:
	YGtkPointerType pointer;
	guint timeout_id;
} YGtkTooltip;

typedef struct YGtkTooltipClass
{
	GtkWindowClass parent_class;
} YGtkTooltipClass;

GtkWidget *ygtk_tooltip_new (void);
GType ygtk_tooltip_get_type (void) G_GNUC_CONST;

// implementation

G_DEFINE_TYPE (YGtkTooltip, ygtk_tooltip, GTK_TYPE_WINDOW)

static void ygtk_tooltip_init (YGtkTooltip *tooltip)
{
	GtkWidget *widget = GTK_WIDGET (tooltip);
	GtkWindow *window = GTK_WINDOW (tooltip);
	// we may need to do this if _new() not used
	//g_object_set (G_OBJECT (tooltip), "type", GTK_WINDOW_POPUP, NULL);
	gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_TOOLTIP);
	gtk_widget_set_app_paintable (widget, TRUE);
	gtk_window_set_resizable (window, FALSE);
	gtk_widget_set_name (widget, "gtk-tooltip");
}

static void ygtk_tooltip_finalize (GObject *object)
{
	YGtkTooltip *tooltip = YGTK_TOOLTIP (object);
	if (tooltip->timeout_id) {
		g_source_remove (tooltip->timeout_id);
		tooltip->timeout_id = 0;
	}
	G_OBJECT_CLASS (ygtk_tooltip_parent_class)->finalize (object);
}

static void get_border (YGtkTooltip *tooltip, gint *left_border, gint *right_border,
                        gint *up_border, gint *down_border)
{
	GtkWidget *widget = GTK_WIDGET (tooltip);
	*left_border = *right_border = widget->style->xthickness;
	*up_border = *down_border = widget->style->ythickness;
	int len = POINTER_LENGTH + 2;
	switch (tooltip->pointer) {
		case YGTK_POINTER_NONE: break;
		case YGTK_POINTER_UP_LEFT:
			*left_border += len;
			break;
		case YGTK_POINTER_UP_RIGHT:
			*right_border += len;
			break;
		case YGTK_POINTER_DOWN_LEFT:
		case YGTK_POINTER_DOWN_RIGHT:
			*down_border += len;
			break;
	}
}

static void ygtk_tooltip_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	GTK_WIDGET_CLASS (ygtk_tooltip_parent_class)->size_request (widget, requisition);
	gint left_border, right_border, up_border, down_border;
	get_border (YGTK_TOOLTIP (widget), &left_border, &right_border,
	            &up_border, &down_border);
	requisition->width += left_border + right_border;
	requisition->height += up_border + down_border;
}

static void ygtk_tooltip_size_allocate (GtkWidget *widget, GtkAllocation *alloc)
{
	GTK_WIDGET_CLASS (ygtk_tooltip_parent_class)->size_allocate (widget, alloc);
	gint left_border, right_border, up_border, down_border;
	get_border (YGTK_TOOLTIP (widget), &left_border, &right_border,
	            &up_border, &down_border);
	GtkAllocation child_alloc = {
		alloc->x + left_border, alloc->y + up_border,
		alloc->width - (left_border+right_border),
		alloc->height - (up_border+down_border)
	};
	gtk_widget_size_allocate (GTK_BIN (widget)->child, &child_alloc);
}

static gboolean ygtk_tooltip_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	gtk_paint_flat_box (widget->style, widget->window, GTK_STATE_NORMAL,
		GTK_SHADOW_OUT, NULL, widget, "tooltip", 0, 0, widget->allocation.width,
		widget->allocation.height);
	YGtkPointerType pointer = YGTK_TOOLTIP (widget)->pointer;
	if (pointer) {
		gint x = 0, y = 0, len_x = 0, len_y = 0;
		switch (pointer) {
			case YGTK_POINTER_NONE: break;
			case YGTK_POINTER_UP_LEFT:
			case YGTK_POINTER_DOWN_LEFT:
				x = 2;
				len_x = POINTER_LENGTH;
				break;
			case YGTK_POINTER_UP_RIGHT:
			case YGTK_POINTER_DOWN_RIGHT:
				x = widget->allocation.width - 2;
				len_x = -POINTER_LENGTH;
				break;
		}
		switch (pointer) {
			case YGTK_POINTER_NONE: break;
			case YGTK_POINTER_UP_LEFT:
			case YGTK_POINTER_UP_RIGHT:
				y = 2;
				len_y = POINTER_LENGTH;
				break;
			case YGTK_POINTER_DOWN_LEFT:
			case YGTK_POINTER_DOWN_RIGHT:
				y = widget->allocation.height - 2;
				len_y = -POINTER_LENGTH;
				break;
		}

		GdkPoint points[3] = {
			{ x, y }, { x + len_x, y }, { x, y + len_y } };
		gdk_draw_polygon (widget->window, *widget->style->dark_gc, TRUE, points, 3);
	}
	GTK_WIDGET_CLASS (ygtk_tooltip_parent_class)->expose_event (widget, event);
	return FALSE;
}

static gboolean tooltip_timeout_cb (void *pdata)
{
	YGtkTooltip *tooltip = (YGtkTooltip *) pdata;
	tooltip->timeout_id = 0;
	gtk_widget_destroy (GTK_WIDGET (tooltip));
	return FALSE;
}

static YGtkTooltip *ygtk_tooltip_create (const gchar *text, const gchar *stock)
{
	GtkWidget *tooltip, *box, *label, *image = 0;
	tooltip = ygtk_tooltip_new();
	label = gtk_label_new (text);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	box = gtk_hbox_new (FALSE, 6);
	if (stock) {
		image = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_BUTTON);
		gtk_box_pack_start (GTK_BOX (box), image, FALSE, TRUE, 0);
	}
	gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
	gtk_widget_show_all (box);
	gtk_container_add (GTK_CONTAINER (tooltip), box);
	return YGTK_TOOLTIP (tooltip);
}

static YGtkTooltip *singleton = 0;

static void ygtk_tooltip_show (YGtkTooltip *tooltip, gint x, gint y)
{
	gtk_window_move (GTK_WINDOW (tooltip), x, y);
	gtk_widget_show (GTK_WIDGET (tooltip));
	if (singleton)
		gtk_widget_destroy (GTK_WIDGET (singleton));
	singleton = tooltip;
	tooltip->timeout_id = g_timeout_add (TOOLTIP_TIMEOUT, tooltip_timeout_cb, tooltip);
}

void ygtk_tooltip_show_at (gint x, gint y, YGtkPointerType pointer,
                           const gchar *label, const gchar *stock)
{
	YGtkTooltip *tooltip = ygtk_tooltip_create (label, stock);
	tooltip->pointer = pointer;
	ygtk_tooltip_show (tooltip, x, y);
}

#define XMARGIN 8
#define YMARGIN 2

void ygtk_tooltip_show_at_widget (GtkWidget *widget, YGtkPointerType pointer,
                                  const gchar *label, const gchar *stock)
{
	YGtkTooltip *tooltip = ygtk_tooltip_create (label, stock);
	tooltip->pointer = pointer;
	gint x, y;
	gdk_window_get_origin (widget->window, &x, &y);
	if (GTK_WIDGET_NO_WINDOW (widget)) {
		x += widget->allocation.x;
		y += widget->allocation.y;
	}
	GtkRequisition tooltip_req;
	gtk_widget_size_request (GTK_WIDGET (tooltip), &tooltip_req);
	switch (pointer) {
		case YGTK_POINTER_NONE: break;
		case YGTK_POINTER_UP_RIGHT:
		case YGTK_POINTER_DOWN_RIGHT:
			x -= (tooltip_req.width - widget->allocation.width) + XMARGIN;
			break;
		case YGTK_POINTER_UP_LEFT:
		case YGTK_POINTER_DOWN_LEFT:
			x += XMARGIN;
			break;
	}
	switch (pointer) {
		case YGTK_POINTER_NONE: break;
		case YGTK_POINTER_UP_RIGHT:
		case YGTK_POINTER_UP_LEFT:
			y += widget->allocation.height + YMARGIN;
			break;
		case YGTK_POINTER_DOWN_RIGHT:
		case YGTK_POINTER_DOWN_LEFT:
			y -= tooltip_req.height + YMARGIN;
			break;
	}
	ygtk_tooltip_show (tooltip, x, y);
}

GtkWidget *ygtk_tooltip_new (void)
{
	return g_object_new (YGTK_TYPE_TOOLTIP, "type", GTK_WINDOW_POPUP, NULL);
}

static void ygtk_tooltip_class_init (YGtkTooltipClass *klass)
{
	ygtk_tooltip_parent_class = g_type_class_peek_parent (klass);

	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = ygtk_tooltip_finalize;

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->size_request  = ygtk_tooltip_size_request;
	widget_class->size_allocate  = ygtk_tooltip_size_allocate;
	widget_class->expose_event  = ygtk_tooltip_expose_event;
}

