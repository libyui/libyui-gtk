/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkNotebook widget */
// check the header file for information about this widget

#include <config.h>
#include "ygtknotebook.h"
#include <gtk/gtk.h>

extern void ygutils_setWidgetFont (GtkWidget *widget, PangoStyle style,
                                   PangoWeight weight, double scale);

G_DEFINE_TYPE (YGtkNotebook, ygtk_notebook, GTK_TYPE_NOTEBOOK)

static void ygtk_notebook_init (YGtkNotebook *view)
{
}

static void ygtk_notebook_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	GTK_WIDGET_CLASS (ygtk_notebook_parent_class)->size_request (widget, requisition);
	YGtkNotebook *notebook = YGTK_NOTEBOOK (widget);
	if (notebook->corner_widget) {
		GtkRequisition child_req;  // many widgets like size_request asked
		gtk_widget_size_request (notebook->corner_widget, &child_req);
	}
}

static void ygtk_notebook_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	GTK_WIDGET_CLASS (ygtk_notebook_parent_class)->size_allocate (widget, allocation);
	gboolean reverse = gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL;
	YGtkNotebook *ynotebook = YGTK_NOTEBOOK (widget);
	if (ynotebook->corner_widget) {
		GtkRequisition child_req;
		gtk_widget_get_child_requisition (ynotebook->corner_widget, &child_req);

		GtkNotebook *notebook = GTK_NOTEBOOK (widget);
		int tabs_width = 0;
		int npages = gtk_notebook_get_n_pages (notebook);
		GtkWidget *last_label = gtk_notebook_get_nth_page (notebook, npages-1);
		if (last_label)
			last_label = gtk_notebook_get_tab_label (notebook, last_label);
		if (last_label) {
			if (reverse)
				tabs_width = (allocation->width + allocation->x) - last_label->allocation.x + 8;
			else
				tabs_width = (last_label->allocation.width + last_label->allocation.x) - allocation->x + 8;
		}

		GtkAllocation child_alloc;
		child_alloc.width = MIN (allocation->width - allocation->x - tabs_width, child_req.width);
		child_alloc.height = child_req.height;
		if (reverse)
			child_alloc.x = allocation->x;
		else
			child_alloc.x = allocation->x + (allocation->width - child_alloc.width);
		child_alloc.y = allocation->y + 3;

		gtk_widget_size_allocate (ynotebook->corner_widget, &child_alloc);
	}
}

static void ygtk_notebook_forall (GtkContainer *container, gboolean include_internals,
                                  GtkCallback callback, gpointer callback_data)
{
	g_return_if_fail (callback != NULL);
	if (include_internals) {
		YGtkNotebook *notebook = YGTK_NOTEBOOK (container);
		if (notebook->corner_widget)
			(* callback) (notebook->corner_widget, callback_data);
	}
	GTK_CONTAINER_CLASS (ygtk_notebook_parent_class)->forall (
		container, include_internals, callback, callback_data);
}

static void ygtk_notebook_map (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (ygtk_notebook_parent_class)->map (widget);
	YGtkNotebook *notebook = YGTK_NOTEBOOK (widget);
	if (notebook->corner_widget) {
		GtkWidget *corner = notebook->corner_widget;
		if (corner && GTK_WIDGET_VISIBLE (corner) && !GTK_WIDGET_MAPPED (corner))
			gtk_widget_map (corner);
	}
}

static void ygtk_notebook_switch_page (
	GtkNotebook *notebook, GtkNotebookPage *_page, guint page_nb)
{  // don't let user select insensitive tab
	GtkWidget *page = gtk_notebook_get_nth_page (notebook, page_nb);
	GtkWidget *label = gtk_notebook_get_tab_label (notebook, page);
	if (label && !GTK_WIDGET_IS_SENSITIVE (label))
		;  // insensitive; don't call parent
	else
		GTK_NOTEBOOK_CLASS (ygtk_notebook_parent_class)->switch_page (
			notebook, _page, page_nb);
}

void ygtk_notebook_set_corner_widget (YGtkNotebook *ynotebook, GtkWidget *child)
{
	ynotebook->corner_widget = child;
	gtk_widget_set_parent (child, GTK_WIDGET (ynotebook));
}

GtkWidget *ygtk_notebook_new (void)
{ return g_object_new (YGTK_TYPE_NOTEBOOK, NULL); }

static void ygtk_notebook_class_init (YGtkNotebookClass *klass)
{
	GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);
	gtkwidget_class->size_request = ygtk_notebook_size_request;
	gtkwidget_class->size_allocate = ygtk_notebook_size_allocate;
	gtkwidget_class->map = ygtk_notebook_map;

	GtkContainerClass *gtkcontainer_class = GTK_CONTAINER_CLASS (klass);
	gtkcontainer_class->forall = ygtk_notebook_forall;

	GtkNotebookClass *gtknotebook_class = GTK_NOTEBOOK_CLASS (klass);
	gtknotebook_class->switch_page = ygtk_notebook_switch_page;
}

