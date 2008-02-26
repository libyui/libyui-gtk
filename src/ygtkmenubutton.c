/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkMenuButton widget */
// check the header file for information about this widget

#include <config.h>
#include "ygtkmenubutton.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

//** YGtkPopupWindow

G_DEFINE_TYPE (YGtkPopupWindow, ygtk_popup_window, GTK_TYPE_WINDOW)

static void ygtk_popup_window_init (YGtkPopupWindow *popup)
{
	GtkWindow *window = GTK_WINDOW (popup);
	gtk_window_set_resizable (window, FALSE);

	GtkWidget *frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
	gtk_widget_show (frame);
	gtk_container_add (GTK_CONTAINER (window), frame);
}

static void ygtk_popup_window_hide (GtkWidget *widget)
{
	gtk_grab_remove (widget);
	GTK_WIDGET_CLASS (ygtk_popup_window_parent_class)->hide (widget);
}

static gboolean ygtk_popup_window_key_press_event (GtkWidget *widget, GdkEventKey *event)
{
	if (event->keyval == GDK_Escape) {
		gtk_widget_hide (widget);
		return TRUE;
	}
	return GTK_WIDGET_CLASS (ygtk_popup_window_parent_class)->key_press_event
	                                                              (widget, event);
}

static gboolean ygtk_popup_window_button_press_event (GtkWidget *widget,
                                                      GdkEventButton *event)
{
	// NOTE: You can't rely on events x and y since the event may take place
	// outside of the window.
	// So, we'll check if this widget (or any of its kids) got the event
	// If that's not the case, close the menu

	GtkWidget *child = gtk_get_event_widget ((GdkEvent *) event);
	if (child != widget)
		while (child) {
			if (child == widget)
				return FALSE;
			child = child->parent;
		}
	gtk_widget_hide (widget);
	return TRUE;
}

GtkWidget *ygtk_popup_window_new (GtkWidget *child)
{
	GtkWidget *widget = g_object_new (YGTK_TYPE_POPUP_WINDOW,
	                                  "type", GTK_WINDOW_POPUP, NULL);
	GtkWidget *frame = gtk_bin_get_child (GTK_BIN (widget));
	gtk_container_add (GTK_CONTAINER (frame), child);
	return widget;
}

static void ygtk_popup_window_frame_position (GtkWidget *widget, gint *x,  gint *y)
{	// don't let it go outside the screen
	GtkRequisition req;
  	gtk_widget_size_request (widget, &req);

	GdkScreen *screen = gtk_widget_get_screen (widget);
	gint monitor_num = gdk_screen_get_monitor_at_window (screen, gtk_widget_get_root_window (widget));
	GdkRectangle monitor;
	gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

	if (*x < monitor.x)
		*x = monitor.x;
	else if (*x + req.width > monitor.x + monitor.width)
		*x = monitor.x + monitor.width - req.width;

	if (*y < monitor.y)
		*y = monitor.y;
	else if (*y + req.height > monitor.y + monitor.height)
		*y = monitor.y + monitor.height - req.height;
} 

void ygtk_popup_window_popup (GtkWidget *widget, gint x, gint y, guint activate_time)
{
	ygtk_popup_window_frame_position (widget, &x, &y);

	gtk_grab_add (widget);
	gtk_window_move (GTK_WINDOW (widget), x, y);
	gtk_widget_grab_focus (widget);
	gtk_widget_show (widget);

	// grab this with your teeth
	if (gdk_pointer_grab (widget->window, TRUE,
	        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK,
	        NULL, NULL, activate_time) == 0)
		if (gdk_keyboard_grab (widget->window, TRUE, activate_time) != 0)
			gdk_pointer_ungrab (activate_time);
}

static void ygtk_popup_window_class_init (YGtkPopupWindowClass *klass)
{
	ygtk_popup_window_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->key_press_event = ygtk_popup_window_key_press_event;
	widget_class->button_press_event = ygtk_popup_window_button_press_event;
	widget_class->hide = ygtk_popup_window_hide;
}

//** YGtkMenuButton

G_DEFINE_TYPE (YGtkMenuButton, ygtk_menu_button, GTK_TYPE_TOGGLE_BUTTON)

static void ygtk_menu_button_init (YGtkMenuButton *button)
{
	button->popup = NULL;
}

static void ygtk_menu_button_free_popup (YGtkMenuButton *button)
{
	if (button->popup) {
		gtk_widget_destroy (GTK_WIDGET (button->popup));
		g_object_unref (G_OBJECT (button->popup));
		button->popup = NULL;
	}
}

static void ygtk_menu_button_finalize (GObject *object)
{
	ygtk_menu_button_free_popup (YGTK_MENU_BUTTON (object));
	G_OBJECT_CLASS (ygtk_menu_button_parent_class)->finalize (object);
}

static void ygtk_menu_button_get_menu_pos (GtkMenu *menu, gint *x, gint *y,
                                           gboolean *push_in, gpointer pointer)
{
	GtkWidget *widget = (GtkWidget*) pointer;
	gdk_window_get_origin (widget->window, x, y);
	*x += widget->allocation.x;
	*y += widget->allocation.y + widget->allocation.height;
	*push_in = TRUE;
}

static void ygtk_menu_button_show_popup (YGtkMenuButton *button)
{
	GtkWidget *widget = GTK_WIDGET (button);
	GtkWidget *popup = button->popup;
	if (!popup)
		return;

	guint activate_time = gtk_get_current_event_time();

	if (GTK_IS_MENU (popup))
		gtk_menu_popup (GTK_MENU (popup), NULL, NULL, ygtk_menu_button_get_menu_pos,
		                widget, 0, activate_time);
	else {  // GTK_IS_WINDOW
		gint x, y;
		gdk_window_get_origin (GDK_WINDOW (widget->window), &x, &y);
		x += widget->allocation.x;
		y += widget->allocation.y + widget->allocation.height;

		ygtk_popup_window_popup (popup, x, y, activate_time);
	}

	if (widget->allocation.width > popup->allocation.width)
		gtk_widget_set_size_request (popup, widget->allocation.width, -1);
}

static void ygtk_menu_button_hide_popup (YGtkMenuButton *button)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
}

static void ygtk_menu_button_button_toggle (GtkToggleButton *button)
{
	if (gtk_toggle_button_get_active (button))
		ygtk_menu_button_show_popup (YGTK_MENU_BUTTON (button));
	else
		ygtk_menu_button_hide_popup (YGTK_MENU_BUTTON (button));
}

static gint ygtk_menu_button_button_press (GtkWidget *widget, GdkEventButton *event)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
		if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
			ygtk_menu_button_show_popup (YGTK_MENU_BUTTON (widget));
		}
		else
			ygtk_menu_button_hide_popup (YGTK_MENU_BUTTON (widget));
		return TRUE;
	}
	return FALSE;
}

GtkWidget *ygtk_menu_button_new (void)
{
	return g_object_new (YGTK_TYPE_MENU_BUTTON, NULL);
}

GtkWidget *ygtk_menu_button_new_with_label (const gchar *label)
{
	GtkWidget *button = ygtk_menu_button_new();
	ygtk_menu_button_set_label (YGTK_MENU_BUTTON (button), label);
	return button;
}

void ygtk_menu_button_set_label (YGtkMenuButton *button, const gchar *label)
{
	if (!button->label) {
		GtkWidget *hbox, *arrow;
		hbox = gtk_hbox_new (FALSE, 4);
		arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_IN);
		button->label = gtk_label_new ("");
		gtk_box_pack_start (GTK_BOX (hbox), button->label, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), arrow, FALSE, TRUE, 0);
		gtk_container_add (GTK_CONTAINER (button), hbox);
		gtk_widget_show_all (hbox);
	}
	if (label && *label) {
		gtk_widget_show (button->label);
		gtk_label_set_text_with_mnemonic (GTK_LABEL (button->label), label);
	}
	else
		gtk_widget_hide (button->label);
}

static void menu_button_hide_popup (GtkWidget *widget, YGtkMenuButton *button)
{ ygtk_menu_button_hide_popup (button); }

void ygtk_menu_button_set_popup (YGtkMenuButton *button, GtkWidget *popup)
{
	ygtk_menu_button_free_popup (button);

	if (!GTK_IS_MENU (popup) && !IS_YGTK_POPUP_WINDOW (popup)) {
		// install widget on a YGtkPopupMenu
		button->popup = ygtk_popup_window_new (popup);
	}
	else
		button->popup = popup;

	g_object_ref (G_OBJECT (button->popup));
	gtk_object_sink (GTK_OBJECT (button->popup));

	g_signal_connect (G_OBJECT (popup), "hide",
	                  G_CALLBACK (menu_button_hide_popup), button);
}

static void ygtk_menu_button_class_init (YGtkMenuButtonClass *klass)
{
	ygtk_menu_button_parent_class = g_type_class_peek_parent (klass);

	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = ygtk_menu_button_finalize;

	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->button_press_event = ygtk_menu_button_button_press;

	GtkToggleButtonClass *toggle_button_class = GTK_TOGGLE_BUTTON_CLASS (klass);
	toggle_button_class->toggled = ygtk_menu_button_button_toggle;
}
