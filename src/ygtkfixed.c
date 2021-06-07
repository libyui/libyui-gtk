/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkFixed container */
// check the header file for information about this container


#include <math.h>
#include <gtk/gtk.h>
#include "ygtkfixed.h"

G_DEFINE_TYPE (YGtkFixed, ygtk_fixed, GTK_TYPE_CONTAINER)

static void ygtk_fixed_init (YGtkFixed *fixed)
{
        gtk_widget_set_has_window(GTK_WIDGET(fixed), FALSE);
	gtk_widget_set_redraw_on_allocate (GTK_WIDGET (fixed), FALSE);
}

void ygtk_fixed_setup (YGtkFixed *fixed, YGtkPreferredWidth cb1, YGtkPreferredHeight cb2, YGtkSetSize cb3, gpointer data)
{
	fixed->preferred_width_cb = cb1;
	fixed->preferred_height_cb = cb2;
	fixed->set_size_cb = cb3;
	fixed->data = data;
}

static YGtkFixedChild *ygtk_fixed_get_child (YGtkFixed *fixed, GtkWidget *widget)
{
	GSList *i;
	for (i = fixed->children; i; i = i->next) {
		YGtkFixedChild *child = i->data;
		if (child->widget == widget)
			return child;
	}
	g_warning ("YGtkFixed: could not find child.");
	return NULL;
}

void ygtk_fixed_set_child_pos (YGtkFixed *fixed, GtkWidget *widget, gint x, gint y)
{
	YGtkFixedChild *child = ygtk_fixed_get_child (fixed, widget);
	child->x = x;
	child->y = y;
}

void ygtk_fixed_set_child_size (YGtkFixed *fixed, GtkWidget *widget, gint width, gint height)
{
	YGtkFixedChild *child = ygtk_fixed_get_child (fixed, widget);
	child->width = width;
	child->height = height;
}

static void ygtk_fixed_add (GtkContainer *container, GtkWidget *widget)
{
	YGtkFixed *fixed = YGTK_FIXED (container);
	YGtkFixedChild *child = g_new0 (YGtkFixedChild, 1);
	child->widget = widget;
	child->width = child->height = 50;
	fixed->children = g_slist_append (fixed->children, child);
	gtk_widget_set_parent (widget, GTK_WIDGET (fixed));
}

static void ygtk_fixed_remove (GtkContainer *container, GtkWidget *widget)
{
	YGtkFixed *fixed = YGTK_FIXED (container);
	GSList *i;
	for (i = fixed->children; i; i = i->next) {
		YGtkFixedChild *child = i->data;
		if (child->widget == widget) {
			gboolean was_visible = gtk_widget_get_visible (widget);
			gtk_widget_unparent (widget);
			fixed->children = g_slist_delete_link (fixed->children, i);
			g_free (child);
			if (was_visible)
				gtk_widget_queue_resize (GTK_WIDGET (container));
			break;
		}
	}
}

static void ygtk_fixed_forall (GtkContainer *container, gboolean include_internals,
                               GtkCallback callback, gpointer callback_data)
{
	g_return_if_fail (callback != NULL);
	YGtkFixed *fixed = YGTK_FIXED (container);
	GSList *i = fixed->children;
	while (i) {
		YGtkFixedChild *child = i->data;
		i = i->next;  // current node might get removed...
		(* callback) (child->widget, callback_data);
	}
}

static void
ygtk_fixed_get_preferred_width (GtkWidget *widget,
                               gint      *minimum_width,
                               gint      *natural_width)
{
	YGtkFixed *fixed = YGTK_FIXED (widget);
	*natural_width = *minimum_width =
		fixed->preferred_width_cb (fixed, fixed->data);
}

static void
ygtk_fixed_get_preferred_height (GtkWidget *widget,
                                gint      *minimum_height,
                                gint      *natural_height)
{
	YGtkFixed *fixed = YGTK_FIXED (widget);
	*natural_height = *minimum_height =
		fixed->preferred_height_cb (fixed, fixed->data);
}

static void ygtk_fixed_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	YGtkFixed *fixed = YGTK_FIXED (widget);
	fixed->set_size_cb (fixed, allocation->width, allocation->height, fixed->data);

	GSList *i;
	for (i = fixed->children; i; i = i->next) {
		YGtkFixedChild *child = i->data;
		int x = child->x;
		if (gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL)
			x = allocation->width - (child->x + child->width);
		x += allocation->x;
		int y = child->y + allocation->y;
		GtkAllocation child_alloc =
			{ x, y, MAX (child->width, 1), MAX (child->height, 1) };

		GtkRequisition min_child_req;
		GtkRequisition nat_child_req;
		gtk_widget_get_preferred_size (child->widget, &min_child_req, &nat_child_req);

		gtk_widget_size_allocate (child->widget, &child_alloc);
	}
	GTK_WIDGET_CLASS (ygtk_fixed_parent_class)->size_allocate (widget, allocation);
}

static GType ygtk_fixed_child_type (GtkContainer *container)
{ return GTK_TYPE_WIDGET; }

static void ygtk_fixed_class_init (YGtkFixedClass *klass)
{
	ygtk_fixed_parent_class = g_type_class_peek_parent (klass);

	GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);
	container_class->add = ygtk_fixed_add;
	container_class->remove = ygtk_fixed_remove;
	container_class->forall = ygtk_fixed_forall;
	container_class->child_type = ygtk_fixed_child_type;

	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->get_preferred_width = ygtk_fixed_get_preferred_width;
	widget_class->get_preferred_height = ygtk_fixed_get_preferred_height;
	widget_class->size_allocate = ygtk_fixed_size_allocate;
}

