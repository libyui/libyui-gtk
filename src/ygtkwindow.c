/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkWindow widget */
// check the header file for information about this widget

#include <Libyui_config.h>
#include "ygtkwindow.h"
#include <gtk/gtk.h>

G_DEFINE_TYPE (YGtkWindow, ygtk_window, GTK_TYPE_WINDOW)

static void ygtk_window_init (YGtkWindow *window)
{
}

GtkWidget* ygtk_window_new (void)
{ return g_object_new (YGTK_TYPE_WINDOW, NULL); }

static void ygtk_window_get_preferred_width (GtkWidget *widget,
					     gint *minimum_width, gint *natural_width)
{
	GTK_WIDGET_CLASS (ygtk_window_parent_class)->get_preferred_width(widget, minimum_width, natural_width);
	*minimum_width = 0;
}

static void ygtk_window_get_preferred_height (GtkWidget *widget,
					      gint *minimum_height, gint *natural_height)
{
	GTK_WIDGET_CLASS (ygtk_window_parent_class)->get_preferred_height(widget, minimum_height, natural_height);
	*minimum_height = 0;
}

static void
ygtk_window_size_allocate (GtkWidget     *widget,
			   GtkAllocation *allocation)
{
	GTK_WIDGET_CLASS (ygtk_window_parent_class)->size_allocate(widget, allocation);
}

static void ygtk_window_class_init (YGtkWindowClass *klass)
{
	ygtk_window_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->get_preferred_width = ygtk_window_get_preferred_width;
	widget_class->get_preferred_height = ygtk_window_get_preferred_height;
	widget_class->size_allocate = ygtk_window_size_allocate;
}

