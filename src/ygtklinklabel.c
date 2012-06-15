/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkLinkLabel container */
// check the header file for information about this container

#include <Libyui_config.h>
#include <math.h>
#include <gtk/gtk.h>
#include "ygtklinklabel.h"

static guint link_clicked_signal;

G_DEFINE_TYPE (YGtkLinkLabel, ygtk_link_label, GTK_TYPE_WIDGET)

static void ygtk_link_label_init (YGtkLinkLabel *label)
{
	gtk_widget_set_has_window(GTK_WIDGET(label), FALSE);
}

static void ygtk_link_label_realize (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (ygtk_link_label_parent_class)->realize (widget);
	GdkWindowAttr attributes;
        GtkAllocation alloc;
        gtk_widget_get_allocation(widget, &alloc);
	attributes.x = alloc.x;
	attributes.y = alloc.y;
	attributes.width = alloc.width;
	attributes.height = alloc.height;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.event_mask = gtk_widget_get_events (widget) |
		GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK;
	gint attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_CURSOR;
	attributes.cursor = gdk_cursor_new_for_display (
		gtk_widget_get_display (widget), GDK_HAND2);

	YGtkLinkLabel *label = YGTK_LINK_LABEL (widget);
	label->link_window = gdk_window_new (gtk_widget_get_window(widget), &attributes, attributes_mask);
	gdk_window_set_user_data (label->link_window, widget);
	GdkColor white = { 0, 0xffff, 0xffff, 0xffff };
	gdk_window_set_background (label->link_window, &white);
	g_object_unref (G_OBJECT(attributes.cursor));
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

static void ygtk_link_label_map (GtkWidget *widget)
{
	// "more" hides on unmap and for some reason showing it clears the
	// text...
	gtk_widget_queue_resize (widget);
	GTK_WIDGET_CLASS (ygtk_link_label_parent_class)->map (widget);
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
	requisition->width = requisition->height = 0;
        GtkStyleContext *style_ctx;
        style_ctx = gtk_widget_get_style_context(widget);
//	if (label->text && *label->text)
	{
		PangoContext *context;
		PangoFontMetrics *metrics;
		gint ascent, descent;
		context = pango_layout_get_context (label->layout);
		metrics = pango_context_get_metrics (context, gtk_style_context_get_font(style_ctx, GTK_STATE_FLAG_NORMAL),
			                                 pango_context_get_language (context));
		ascent = pango_font_metrics_get_ascent (metrics);
		descent = pango_font_metrics_get_descent (metrics);
		pango_font_metrics_unref (metrics);
		requisition->height = PANGO_PIXELS (ascent + descent);
	}
}

static void
ygtk_link_label_get_preferred_width (GtkWidget *widget,
                                     gint      *minimal_width,
                                     gint      *natural_width)
{
  GtkRequisition requisition;
  ygtk_link_label_size_request (widget, &requisition);
  *minimal_width = *natural_width = requisition.width;
}

static void
ygtk_link_label_get_preferred_height (GtkWidget *widget,
                                      gint      *minimal_height,
                                      gint      *natural_height)
{
  GtkRequisition requisition;
  ygtk_link_label_size_request (widget, &requisition);
  *minimal_height = *natural_height = requisition.height;
}

#define SPACING 4

static void ygtk_link_label_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
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
			if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
				x = (2*allocation->x + allocation->width) - (x + link_width);
			gdk_window_move_resize (label->link_window, x,
				allocation->y, link_width, allocation->height);
			if (logical.width > width)
				pango_layout_set_width (label->layout, width);
			gdk_window_show (label->link_window);
		}
		else
			gdk_window_hide (label->link_window);
	}
}

static gboolean ygtk_link_label_on_draw (GtkWidget *widget, cairo_t *cr)
{
	YGtkLinkLabel *label = YGTK_LINK_LABEL (widget);
	ygtk_link_label_ensure_layout (label);

    GtkStyleContext *style = gtk_widget_get_style_context(widget);
	if (gtk_cairo_should_draw_window(cr, gtk_widget_get_window(widget))) {
		gint x = 0;
		gint width = gtk_widget_get_allocated_width (widget);
		if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) {
			PangoRectangle extent;
			pango_layout_get_extents (label->layout, NULL, &extent);
			x = width - extent.width/PANGO_SCALE;
		}
		gtk_render_layout (style, cr, x, 0, label->layout);
	}

	if (gtk_cairo_should_draw_window(cr, label->link_window)) {
		gtk_cairo_transform_to_window (cr, widget, label->link_window);
		gtk_render_layout (style, cr, 0, 0, label->link_layout);
	}
	return FALSE;
}

static gboolean ygtk_link_label_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	g_signal_emit (widget, link_clicked_signal, 0, NULL);
	return TRUE;
}

void ygtk_link_label_set_text (YGtkLinkLabel *label,
	const gchar *text, const gchar *link, gboolean link_always_visible)
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
	widget_class->map = ygtk_link_label_map;

        widget_class->get_preferred_height = ygtk_link_label_get_preferred_height;
        widget_class->get_preferred_width = ygtk_link_label_get_preferred_width;

	widget_class->size_allocate = ygtk_link_label_size_allocate;
	widget_class->draw = ygtk_link_label_on_draw;
	widget_class->button_press_event = ygtk_link_label_button_press_event;

	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = ygtk_link_label_finalize;

	link_clicked_signal = g_signal_new ("link-clicked",
		G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (YGtkLinkLabelClass, link_clicked), NULL, NULL,
		g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

