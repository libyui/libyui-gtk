/* Yast-GTK */
/* YGtkMenuButton is a button that displays a menu when pressed.
*/

#include "ygtkmenubutton.h"
#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkarrow.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

static void ygtk_menu_button_class_init (YGtkMenuButtonClass *klass);
static void ygtk_menu_button_init       (YGtkMenuButton      *button);
static void ygtk_menu_button_finalize   (GObject             *object);

static gint ygtk_menu_button_button_press (GtkWidget          *widget,
                                           GdkEventButton     *event);
static void ygtk_menu_button_button_toggle (GtkToggleButton   *button);

static void ygtk_menu_button_show_popup (YGtkMenuButton *button);
static void ygtk_menu_button_hide_popup (YGtkMenuButton *button);

static gint popup_key_press (GtkWidget *widget, GdkEventKey *event,
                             YGtkMenuButton *button);
static gint popup_button_press (GtkWidget *widget, GdkEventButton *event,
                                YGtkMenuButton *button);

static GtkToggleButtonClass *parent_class = NULL;

GType ygtk_menu_button_get_type()
{
	static GType type = 0;
	if (!type) {
		static const GTypeInfo info = {
			sizeof (YGtkMenuButtonClass),
			NULL, NULL, (GClassInitFunc) ygtk_menu_button_class_init, NULL, NULL,
			sizeof (YGtkMenuButton), 0, (GInstanceInitFunc) ygtk_menu_button_init, NULL
		};

		type = g_type_register_static (GTK_TYPE_TOGGLE_BUTTON, "YGtkMenuButton",
		                               &info, (GTypeFlags) 0);
	}
	return type;
}

static void ygtk_menu_button_class_init (YGtkMenuButtonClass *klass)
{
	parent_class = g_type_class_peek_parent (klass);

	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = ygtk_menu_button_finalize;

	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->button_press_event = ygtk_menu_button_button_press;

	GtkToggleButtonClass *toggle_button_class = GTK_TOGGLE_BUTTON_CLASS (klass);
	toggle_button_class->toggled = ygtk_menu_button_button_toggle;
}

static void ygtk_menu_button_init (YGtkMenuButton *button)
{
	button->popup = NULL;
	button->label = gtk_label_new ("");

	GtkWidget *hbox, *arrow;
	hbox = gtk_hbox_new (FALSE, 0);
	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	gtk_widget_show (arrow);

	if (gtk_widget_get_direction (arrow) == GTK_TEXT_DIR_LTR) {
		gtk_container_add (GTK_CONTAINER (hbox), button->label);
		gtk_container_add (GTK_CONTAINER (hbox), arrow);
	}
	else {
		gtk_container_add (GTK_CONTAINER (hbox), arrow);
		gtk_container_add (GTK_CONTAINER (hbox), button->label);
	}

	gtk_box_set_child_packing (GTK_BOX (hbox), arrow, FALSE, FALSE,
	                           5, GTK_PACK_START);

	gtk_container_add (GTK_CONTAINER (button), hbox);

}

static void ygtk_menu_button_free_popup (YGtkMenuButton *button)
{
	if (button->popup) {
		gtk_widget_destroy (GTK_WIDGET (button->popup));
		g_object_unref (G_OBJECT (button->popup));
		button->popup = NULL;
	}
}

void ygtk_menu_button_finalize (GObject *object)
{
	ygtk_menu_button_free_popup (YGTK_MENU_BUTTON (object));
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget *ygtk_menu_button_new()
{
	YGtkMenuButton* button = (YGtkMenuButton*) g_object_new (YGTK_TYPE_MENU_BUTTON, NULL);
	return GTK_WIDGET (button);
}

void ygtk_menu_button_set_label (YGtkMenuButton *button, const gchar *label)
{
	gtk_label_set_text_with_mnemonic (GTK_LABEL (button->label), label);
	gtk_widget_show (button->label);
}

static void menu_button_hide_popup (GtkWidget *widget, YGtkMenuButton *button)
{ ygtk_menu_button_hide_popup (button); }

void ygtk_menu_button_set_popup (YGtkMenuButton *button, GtkWidget *popup)
{
	ygtk_menu_button_free_popup (button);

	if (GTK_IS_MENU (popup))
		button->popup = popup;
	else {
		// if it isn't of type GtkMenu, we will emulate the popup
		button->popup = gtk_window_new(GTK_WINDOW_POPUP);
		gtk_window_set_resizable (GTK_WINDOW (button->popup), FALSE);

		GtkWidget *frame = gtk_frame_new (NULL);
		gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
		gtk_container_add (GTK_CONTAINER (frame), popup);

		gtk_container_add (GTK_CONTAINER (button->popup), frame);

		g_signal_connect(button->popup, "key_press_event",
		                 G_CALLBACK (popup_key_press), button);
		g_signal_connect(button->popup, "button_press_event",
		                 G_CALLBACK (popup_button_press), button);
	}

	g_object_ref (G_OBJECT (button->popup));
	gtk_object_sink (GTK_OBJECT (button->popup));

	g_signal_connect (G_OBJECT (popup), "hide",
	                  G_CALLBACK (menu_button_hide_popup), button);
	gtk_widget_show_all (popup);
}

static void ygtk_menu_button_get_menu_pos (GtkMenu *menu, gint *x, gint *y,
                                           gboolean *push_in, gpointer pointer)
{
	// Based on code from GtkComboBox
	GtkWidget *widget = (GtkWidget*) pointer;
	gint sx, sy;
	GtkRequisition req;

	gdk_window_get_origin (widget->window, &sx, &sy);

	if (GTK_WIDGET_NO_WINDOW (widget)) {
		sx += widget->allocation.x;
		sy += widget->allocation.y;
	}

	gtk_widget_size_request (GTK_WIDGET (menu), &req);

	if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
		*x = sx;
	else
		*x = sx + widget->allocation.width - req.width;
	*y = sy;

	GdkRectangle monitor;
	GdkScreen *screen = gtk_widget_get_screen (widget);
	int monitor_num = gdk_screen_get_monitor_at_window (screen, widget->window);
	gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

	if (*x < monitor.x)
		*x = monitor.x;
	else if (*x + req.width > monitor.x + monitor.width)
		*x = monitor.x + monitor.width - req.width;

	if (monitor.y + monitor.height - *y - widget->allocation.height >= req.height)
		*y += widget->allocation.height;
	else if (*y - monitor.y >= req.height)
		*y -= req.height;
	else if (monitor.y + monitor.height - *y - widget->allocation.height > *y - monitor.y)
		*y += widget->allocation.height;
	else
		*y -= req.height;

	*push_in = FALSE;
}

void ygtk_menu_button_show_popup (YGtkMenuButton *button)
{
	GtkWidget *widget = GTK_WIDGET (button);
	GtkWidget *popup = button->popup;
	if (!popup)
		return;

	guint activate_time = gtk_get_current_event_time();

	if (GTK_IS_MENU (popup)) {
		GtkMenu *menu = GTK_MENU (popup);
		gtk_menu_popup (menu, NULL, NULL, ygtk_menu_button_get_menu_pos,
		                widget, 0, activate_time);
	}
	else {
		gtk_grab_add (popup);
		int x, y;
		gdk_window_get_origin (GDK_WINDOW (widget->window), &x, &y);
		x += widget->allocation.x;
		y += widget->allocation.y + widget->allocation.height;
		gtk_window_move (GTK_WINDOW (popup), x, y);
		gtk_widget_grab_focus (popup);
		gtk_widget_show_all (popup);

		// grab this with your teeth
		if (gdk_pointer_grab (popup->window, TRUE,
		        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK,
		        NULL, NULL, activate_time) == 0)
			if (gdk_keyboard_grab (popup->window, TRUE, activate_time) != 0)
				gdk_pointer_ungrab(activate_time);
	}

	if (widget->allocation.width > popup->allocation.width)
		gtk_widget_set_size_request (popup, widget->allocation.width, -1);
}

void ygtk_menu_button_hide_popup (YGtkMenuButton *button)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);

	if (!GTK_IS_MENU (button->popup)) {
		gtk_grab_remove (button->popup);
		gtk_widget_hide (button->popup);
	}
}

void ygtk_menu_button_button_toggle (GtkToggleButton *button)
{
	if (gtk_toggle_button_get_active (button))
		ygtk_menu_button_show_popup (YGTK_MENU_BUTTON (button));
	else
		ygtk_menu_button_hide_popup (YGTK_MENU_BUTTON (button));
}

gint ygtk_menu_button_button_press (GtkWidget *widget, GdkEventButton *event)
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

gint popup_key_press (GtkWidget *widget, GdkEventKey *event,
                      YGtkMenuButton *button)
{
	if (event->keyval == GDK_Escape) {
		g_signal_stop_emission_by_name(widget, "key_press_event");
		ygtk_menu_button_hide_popup (button);
		return TRUE;
	}
	return FALSE;
}

gint popup_button_press (GtkWidget *widget, GdkEventButton *event,
                         YGtkMenuButton *button)
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

	ygtk_menu_button_hide_popup (button);
	return TRUE;
}
