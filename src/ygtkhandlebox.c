/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkHandleBox widget */
// check the header file for information about this widget

#include <config.h>
#include "ygtkhandlebox.h"

G_DEFINE_TYPE (YGtkHandleBox, ygtk_handle_box, GTK_TYPE_HANDLE_BOX)

static void ygtk_handle_box_init (YGtkHandleBox *box)
{
}

static void ygtk_handle_box_realize (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (ygtk_handle_box_parent_class)->realize (widget);

	GtkHandleBox *box = GTK_HANDLE_BOX (widget);
	GdkWindow *window = box->float_window;
	gdk_window_set_decorations (window, GDK_DECOR_RESIZEH);
	gdk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_DIALOG);
	gdk_window_set_title (window, "");
}

static gboolean ygtk_handle_box_configure_event (GtkWidget *widget, GdkEventConfigure *event)
{
	GtkHandleBox *box = GTK_HANDLE_BOX (widget);
	if (event->window == box->float_window) {
		gtk_widget_queue_resize (widget);
		return TRUE;
	}
	return FALSE;
}

static void ygtk_handle_box_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	GtkHandleBox *box = GTK_HANDLE_BOX (widget);
	if (box->child_detached) {
		gdk_window_move_resize (widget->window, allocation->x, allocation->y,
		                        allocation->width, allocation->height);
		gint width, height;
		gdk_drawable_get_size (box->float_window, &width, &height);
		gdk_window_resize (box->bin_window, width, height);
		GtkAllocation child_alloc = { 0, 0, width, height };
		gtk_widget_size_allocate (GTK_BIN (widget)->child, &child_alloc);
	}
	else
		GTK_WIDGET_CLASS (ygtk_handle_box_parent_class)->size_allocate (widget, allocation);
}

GtkWidget* ygtk_handle_box_new (void)
{ return g_object_new (YGTK_TYPE_HANDLE_BOX, NULL); }

static void ygtk_handle_box_class_init (YGtkHandleBoxClass *klass)
{
	ygtk_handle_box_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->configure_event = ygtk_handle_box_configure_event;
	widget_class->realize = ygtk_handle_box_realize;
	widget_class->size_allocate = ygtk_handle_box_size_allocate;
}

