/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkTextView widget */
// check the header file for information about this widget

#include <yui/Libyui_config.h>
#include "ygtktextview.h"
#include <gtk/gtk.h>

G_DEFINE_TYPE (YGtkTextView, ygtk_text_view, GTK_TYPE_TEXT_VIEW)

static void ygtk_text_view_init (YGtkTextView *view)
{
}

static void ygtk_text_view_realize (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (ygtk_text_view_parent_class)->realize (widget);

	GtkTextView *view = GTK_TEXT_VIEW (widget);
	if (!gtk_text_view_get_editable (view)) {
		gtk_text_view_set_cursor_visible (view, FALSE);
		GdkWindow *window = gtk_text_view_get_window (view, GTK_TEXT_WINDOW_TEXT);
		gdk_window_set_cursor (window, NULL);
	}
}

// popup utilities
static void copy_activate_cb (GtkMenuItem *item, GtkTextBuffer *buffer)
{
	gtk_text_buffer_copy_clipboard (buffer, gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));
}
static void select_all_activate_cb (GtkMenuItem *item, GtkTextBuffer *buffer)
{
	GtkTextIter start, end;
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gtk_text_buffer_select_range (buffer, &start, &end);
}

static void ygtk_text_view_populate_popup (GtkTextView *view, GtkWidget *menu)
{
	if (gtk_text_view_get_editable (view))
		return;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (view);

	GList *items = gtk_container_get_children (GTK_CONTAINER (menu)), *i;
	for (i = items; i; i = i->next)
		gtk_container_remove (GTK_CONTAINER (menu), i->data);
	g_list_free (items);

	GtkWidget *item;
	item = gtk_image_menu_item_new_from_stock (GTK_STOCK_COPY, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	if (gtk_text_buffer_get_has_selection (buffer))
		g_signal_connect (item, "activate", G_CALLBACK (copy_activate_cb), buffer);
	else
		gtk_widget_set_sensitive (item, FALSE);
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	item = gtk_image_menu_item_new_from_stock (GTK_STOCK_SELECT_ALL, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (item, "activate", G_CALLBACK (select_all_activate_cb), buffer);
	gtk_widget_show_all (GTK_WIDGET (menu));
}

static inline gboolean is_space (gunichar ch)
{ return g_unichar_isspace (ch) || ch == 0xfffc; }

static gboolean ygtk_text_view_button_press_event (GtkWidget *widget, GdkEventButton *event)
{	// on right-click, select word under cursor if there is no selection
	if (event->button == 3) {
		GtkTextView *view = GTK_TEXT_VIEW (widget);
		GtkTextBuffer *buffer = gtk_text_view_get_buffer (view);
		if (!gtk_text_buffer_get_has_selection (buffer)) {
			gint buffer_x, buffer_y;
			gtk_text_view_window_to_buffer_coords (view,
				GTK_TEXT_WINDOW_WIDGET, (gint) event->x, (gint) event->y, &buffer_x, &buffer_y);
			GtkTextIter iter;
			gtk_text_view_get_iter_at_location (view, &iter, buffer_x, buffer_y);

			if (!is_space (gtk_text_iter_get_char (&iter))) {
				GtkTextIter start, end = iter, temp = iter;
				do {
					start = temp;
					if (!gtk_text_iter_backward_char (&temp))
						break;
				} while (!is_space (gtk_text_iter_get_char (&temp)));
				do {
					if (!gtk_text_iter_forward_char (&end))
						break;
				} while (!is_space (gtk_text_iter_get_char (&end)));

				gtk_text_buffer_select_range (buffer, &start, &end);
			}
		}
	}
	return GTK_WIDGET_CLASS (ygtk_text_view_parent_class)->button_press_event (widget, event);
}

GtkWidget *ygtk_text_view_new (gboolean editable)
{ return g_object_new (YGTK_TYPE_TEXT_VIEW, "editable", editable, NULL); }

static void ygtk_text_view_class_init (YGtkTextViewClass *klass)
{
	GtkTextViewClass *gtktextview_class = GTK_TEXT_VIEW_CLASS (klass);
	gtktextview_class->populate_popup = ygtk_text_view_populate_popup;

	GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);
	gtkwidget_class->realize = ygtk_text_view_realize;
	gtkwidget_class->button_press_event = ygtk_text_view_button_press_event;
}

