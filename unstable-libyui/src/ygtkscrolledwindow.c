/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkScrolledWindow widget */
// check the header file for information about this widget

#include <config.h>
#include "ygtkscrolledwindow.h"
#include <gtk/gtk.h>

#define CHILD_SPACING 2

G_DEFINE_TYPE (YGtkScrolledWindow, ygtk_scrolled_window, GTK_TYPE_SCROLLED_WINDOW)

static void ygtk_scrolled_window_init (YGtkScrolledWindow *scroll)
{
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_IN);
}

static void ygtk_scrolled_window_destroy (GtkObject *object)
{
	GTK_OBJECT_CLASS (ygtk_scrolled_window_parent_class)->destroy (object);
	YGtkScrolledWindow *scroll = YGTK_SCROLLED_WINDOW (object);
	ygtk_scrolled_window_set_corner_widget (scroll, NULL);
}

static void ygtk_scrolled_window_size_allocate (GtkWidget *widget,
                                                GtkAllocation *allocation)
{
	GTK_WIDGET_CLASS (ygtk_scrolled_window_parent_class)->size_allocate
	                                                    (widget, allocation);
	if (!GTK_WIDGET_REALIZED (widget))
		return;

	YGtkScrolledWindow *scroll = YGTK_SCROLLED_WINDOW (widget);
	if (scroll->corner_child) {
		GtkRequisition req;
		gtk_widget_size_request (scroll->corner_child, &req);

		GtkWidget *vbar;
		vbar = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (scroll));

		vbar->allocation.height = MAX (0, widget->allocation.height - req.height - CHILD_SPACING);
		GtkAllocation alloc = { vbar->allocation.x,
			vbar->allocation.y + vbar->allocation.height + CHILD_SPACING,
			vbar->allocation.width,
			req.height };
		gtk_widget_size_allocate (scroll->corner_child, &alloc);
	}
}

static void ygtk_scrolled_window_forall (GtkContainer *container,
	gboolean include_internals, GtkCallback callback, gpointer callback_data)
{
	GTK_CONTAINER_CLASS (ygtk_scrolled_window_parent_class)->forall (container,
		include_internals, callback, callback_data);
	if (include_internals) {
		YGtkScrolledWindow *scroll = YGTK_SCROLLED_WINDOW (container);
		if (scroll->corner_child)
			callback (scroll->corner_child, callback_data);
	}
}

void ygtk_scrolled_window_set_corner_widget (YGtkScrolledWindow *scroll, GtkWidget *child)
{
	if (scroll->corner_child)
		gtk_widget_unparent (scroll->corner_child);
	scroll->corner_child = child;
	if (child)
		gtk_widget_set_parent (child, GTK_WIDGET (scroll));
}

void ygtk_scrolled_window_replace (YGtkScrolledWindow *scroll, GtkWidget *child)
{
	GtkWidget *current = gtk_bin_get_child (GTK_BIN (scroll));
	if (current)
		gtk_container_remove (GTK_CONTAINER (scroll), current);
	if (child)
		gtk_container_add (GTK_CONTAINER (scroll), child);
}

GtkWidget *ygtk_scrolled_window_new (void)
{ return g_object_new (YGTK_TYPE_SCROLLED_WINDOW, NULL); }

static void ygtk_scrolled_window_class_init (YGtkScrolledWindowClass *klass)
{
	GtkContainerClass *gtkcontainer_class = GTK_CONTAINER_CLASS (klass);
	gtkcontainer_class->forall = ygtk_scrolled_window_forall;

	GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);
	gtkwidget_class->size_allocate = ygtk_scrolled_window_size_allocate;

	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
	gtkobject_class->destroy = ygtk_scrolled_window_destroy;
}

