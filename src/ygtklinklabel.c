/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkLinkLabel container */
// check the header file for information about this container

#include <config.h>
#include <math.h>
#include <gtk/gtk.h>
#include "ygtklinklabel.h"

static guint link_clicked_signal;

G_DEFINE_TYPE (YGtkLinkLabel, ygtk_link_label, GTK_TYPE_WIDGET)

static void ygtk_link_label_init (YGtkLinkLabel *label)
{
	GTK_WIDGET_SET_FLAGS (label, GTK_NO_WINDOW);
}

static void ygtk_link_label_realize (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (ygtk_link_label_parent_class)->realize (widget);
	GdkWindowAttr attributes;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.event_mask = gtk_widget_get_events (widget) |
		GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK;
	gint attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_CURSOR;
	attributes.cursor = gdk_cursor_new_for_display (
		gtk_widget_get_display (widget), GDK_HAND2);

	YGtkLinkLabel *label = YGTK_LINK_LABEL (widget);
	label->link_window = gdk_window_new (widget->window, &attributes, attributes_mask);
	gdk_window_set_user_data (label->link_window, widget);
	GdkColor white = { 0, 0xffff, 0xffff, 0xffff };
	gdk_rgb_find_color (gtk_widget_get_colormap (widget), &white);
	gdk_window_set_background (label->link_window, &white);
	gdk_cursor_unref (attributes.cursor);
}

static void ygtk_link_label_unrealize (GtkWidget *widget)
{
	YGtkLinkLabel *label = YGTK_LINK_LABEL (widget);
	if (label->link_window) {
		gdk_window_set_user_data (label->link_window, NULL);
		gdk_window_destroy (label->link_window);
		label->link_window = NULL;
	}
	GTK_WIDGET_CLASS (ygtk_link_label_parent_class)->unrealize (widget);
}

static void ygtk_link_label_clear_layout (YGtkLinkLabel *label)
{
	if (label->layout) {
		g_object_unref (label->layout);
		label->layout = NULL;
	}
	if (label->link_layout) {
		g_object_unref (label->link_layout);
		label->link_layout = NULL;
	}
}

static void ygtk_link_label_ensure_layout (YGtkLinkLabel *label)
{
	GtkWidget *widget = GTK_WIDGET (label);
	if (!label->layout) {
		label->layout = gtk_widget_create_pango_layout (widget, label->text);
		pango_layout_set_single_paragraph_mode (label->layout, TRUE);
		pango_layout_set_ellipsize (label->layout, PANGO_ELLIPSIZE_END);
		pango_layout_set_width (label->layout, widget->allocation.width * PANGO_SCALE);
	}
	if (!label->link_layout) {
		label->link_layout = gtk_widget_create_pango_layout (widget, label->link);
		PangoAttrList *attrbs = pango_attr_list_new();
		pango_attr_list_insert (attrbs, pango_attr_underline_new (PANGO_UNDERLINE_SINGLE));
		pango_attr_list_insert (attrbs, pango_attr_foreground_new (0, 0, 0xffff));
		pango_layout_set_attributes (label->link_layout, attrbs);
		pango_attr_list_unref (attrbs);
	}
}

static void ygtk_link_label_finalize (GObject *object)
{
	YGtkLinkLabel *label = YGTK_LINK_LABEL (object);
	g_free (label->text);
	g_free (label->link);
	ygtk_link_label_clear_layout (label);
	G_OBJECT_CLASS (ygtk_link_label_parent_class)->finalize (object);
}

static void ygtk_link_label_size_request (GtkWidget      *widget,
                                           GtkRequisition *requisition)
{
	YGtkLinkLabel *label = YGTK_LINK_LABEL (widget);
	ygtk_link_label_ensure_layout (label);
	requisition->width = 0;

	PangoContext *context;
	PangoFontMetrics *metrics;
	gint ascent, descent;
	context = pango_layout_get_context (label->layout);
	metrics = pango_context_get_metrics (context, widget->style->font_desc,
	                                     pango_context_get_language (context));
	ascent = pango_font_metrics_get_ascent (metrics);
	descent = pango_font_metrics_get_descent (metrics);
	pango_font_metrics_unref (metrics);
	requisition->height = PANGO_PIXELS (ascent + descent);
}

#define SPACING 4

static void ygtk_link_label_size_allocate (GtkWidget      *widget,
                                            GtkAllocation  *allocation)
{
	GTK_WIDGET_CLASS (ygtk_link_label_parent_class)->size_allocate (widget, allocation);
	YGtkLinkLabel *label = YGTK_LINK_LABEL (widget);

	gint width = allocation->width * PANGO_SCALE;
	PangoRectangle logical;
	pango_layout_set_width (label->layout, -1);
	pango_layout_get_extents (label->layout, NULL, &logical);
	if (label->link_window) {
		if (*label->text && (logical.width > width || label->link_always_visible)) {
			PangoRectangle link_logical;
			pango_layout_get_extents (label->link_layout, NULL, &link_logical);
			gint link_width = link_logical.width / PANGO_SCALE;
			gint x;
			width = width - link_logical.width - SPACING*PANGO_SCALE;
			if (logical.width < width && label->link_always_visible)
				x = allocation->x + logical.width/PANGO_SCALE + SPACING;
			else
				x = allocation->x + (allocation->width - link_width);
			gdk_window_move_resize (label->link_window, x,
				allocation->y, link_width, allocation->height);
			pango_layout_set_width (label->layout, width);
			gdk_window_show (label->link_window);
		}
		else
			gdk_window_hide (label->link_window);
	}
}

static gboolean ygtk_link_label_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	YGtkLinkLabel *label = YGTK_LINK_LABEL (widget);
	ygtk_link_label_ensure_layout (label);

	gint x = 0, y = 0;
	PangoLayout *layout = 0;
	if (event->window == widget->window) {
		x += widget->allocation.x;
		y += widget->allocation.y;
		layout = label->layout;
	}
	else if (event->window == label->link_window)
		layout = label->link_layout;

	if (layout)
		gtk_paint_layout (widget->style, event->window, GTK_WIDGET_STATE (widget),
			FALSE, &event->area, widget, "label", x, y, layout);
	return FALSE;
}

static gboolean ygtk_link_label_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	g_signal_emit (widget, link_clicked_signal, 0, NULL);
	return FALSE;
}

void ygtk_link_label_set_text (YGtkLinkLabel *label, const gchar *text, const gchar *link,
                                gboolean link_always_visible)
{
	g_free (label->text);
	label->text = g_strdup (text);
	if (link) {
		g_free (label->link);
		label->link = g_strdup (link);
	}
	label->link_always_visible = link_always_visible;
	ygtk_link_label_clear_layout (label);
	gtk_widget_queue_resize (GTK_WIDGET (label));
}

GtkWidget *ygtk_link_label_new (const gchar *text, const gchar *link)
{
	YGtkLinkLabel *label = g_object_new (YGTK_TYPE_LINK_LABEL, NULL);
	ygtk_link_label_set_text (label, text, link, TRUE);
	return (GtkWidget *) label;
}

static void ygtk_link_label_class_init (YGtkLinkLabelClass *klass)
{
	ygtk_link_label_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->realize = ygtk_link_label_realize;
	widget_class->unrealize = ygtk_link_label_unrealize;
	widget_class->size_request = ygtk_link_label_size_request;
	widget_class->size_allocate = ygtk_link_label_size_allocate;
	widget_class->expose_event = ygtk_link_label_expose_event;
	widget_class->button_press_event = ygtk_link_label_button_press_event;

	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = ygtk_link_label_finalize;

	link_clicked_signal = g_signal_new ("link-clicked",
		G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (YGtkLinkLabelClass, link_clicked), NULL, NULL,
		g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

