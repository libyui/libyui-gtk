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

/* YGtkTreeView widget */
// check the header file for information about this widget

static guint right_click_signal = 0;

G_DEFINE_TYPE (YGtkTreeView, ygtk_tree_view, GTK_TYPE_TREE_VIEW)

static void ygtk_tree_view_init (YGtkTreeView *view)
{
}

static void _gtk_widget_destroy (gpointer widget)
{ gtk_widget_destroy (GTK_WIDGET (widget)); }

void ygtk_tree_view_popup_menu (YGtkTreeView *view, GtkWidget *menu)
{
	GtkWidget *widget = GTK_WIDGET (view);
	// popup hack -- we can't destroy the menu at hide because it may not have
	// emitted signals yet -- destroy it when replaced or the widget destroyed
	g_object_set_data_full (G_OBJECT (view), "popup", menu, _gtk_widget_destroy);

	guint32 time = gtk_get_current_event_time();
	gtk_menu_attach_to_widget (GTK_MENU (menu), widget, NULL);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,  3, time);
	gtk_widget_show_all (menu);
}

static gboolean ygtk_tree_view_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	// workaround (based on gedit): we want the tree view to receive this press in order
	// to select the row, but we can't use connect_after, so we throw a dummy mouse press
	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		static gboolean safeguard = FALSE;
		if (safeguard) return FALSE;
		safeguard = TRUE;

		GtkTreeView *view = GTK_TREE_VIEW (widget);
		GtkTreeSelection *selection = gtk_tree_view_get_selection (view);

		gboolean outreach;
		outreach = !gtk_tree_view_get_path_at_pos (view, event->x, event->y, 0, 0, 0, 0);
		if (gtk_tree_selection_count_selected_rows (selection) <= 1) {
			// if there is a selection, let it be
			event->button = 1;
			if (!gtk_widget_event (widget, (GdkEvent *) event))
				return FALSE;
		}
		g_signal_emit (widget, right_click_signal, 0, outreach);
		safeguard = FALSE;
		return TRUE;
	}
	return GTK_WIDGET_CLASS (ygtk_tree_view_parent_class)->button_press_event (widget, event);
}

static gboolean _ygtk_tree_view_popup_menu (GtkWidget *widget)
{
	GtkTreeView *view = GTK_TREE_VIEW (widget);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
	gboolean outreach = gtk_tree_selection_count_selected_rows (selection) == 0;

	g_signal_emit (widget, right_click_signal, 0, outreach);
	return TRUE;
}

GtkWidget *ygtk_tree_view_new (void)
{ return g_object_new (YGTK_TYPE_TREE_VIEW, NULL); }

static void ygtk_tree_view_class_init (YGtkTreeViewClass *klass)
{
	GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);
	gtkwidget_class->button_press_event = ygtk_tree_view_button_press_event;
	gtkwidget_class->popup_menu = _ygtk_tree_view_popup_menu;

	right_click_signal = g_signal_new ("right-click",
		G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (YGtkTreeViewClass, right_click),
		NULL, NULL, gtk_marshal_VOID__BOOLEAN, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

