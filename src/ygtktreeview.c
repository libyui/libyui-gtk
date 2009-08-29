/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkTreeView widget */
// check the header file for information about this widget

#include <config.h>
#include "ygtktreeview.h"
#include <gtk/gtk.h>

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

// Ensure selected row remains visible on window size changes
static void ygtk_tree_view_size_allocate (GtkWidget *widget, GtkAllocation *alloc)
{
	GTK_WIDGET_CLASS (ygtk_tree_view_parent_class)->size_allocate (widget, alloc);

	GtkTreeView *view = GTK_TREE_VIEW (widget);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
	GList *paths;
	paths = gtk_tree_selection_get_selected_rows (selection, NULL);

	if (paths && !paths->next) /*only one row selected*/ {
		GtkTreePath *path = (GtkTreePath *) paths->data;
		gtk_tree_view_scroll_to_cell (view, path, NULL, FALSE, 0, 0);
	}
	g_list_foreach (paths, (GFunc) gtk_tree_path_free, 0);
	g_list_free (paths);
}


GtkWidget *ygtk_tree_view_new (void)
{ return g_object_new (YGTK_TYPE_TREE_VIEW, NULL); }

static void ygtk_tree_view_class_init (YGtkTreeViewClass *klass)
{
	GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);
	gtkwidget_class->button_press_event = ygtk_tree_view_button_press_event;
	gtkwidget_class->popup_menu = _ygtk_tree_view_popup_menu;
	gtkwidget_class->size_allocate = ygtk_tree_view_size_allocate;

	right_click_signal = g_signal_new ("right-click",
		G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (YGtkTreeViewClass, right_click),
		NULL, NULL, gtk_marshal_VOID__BOOLEAN, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

