/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkMenuButton is a button that displays a widget when pressed.
   This widget can either be of type GtkMenu or another, like a
   GtkCalendar and we'll do the proper "emulation" (the work of
   the YGtkPopupWindow widget).
*/

#ifndef YGTK_POPUP_WINDOW_H
#define YGTK_POPUP_WINDOW_H

#include <gtk/gtk.h>
G_BEGIN_DECLS

#define YGTK_TYPE_POPUP_WINDOW            (ygtk_popup_window_get_type ())
#define YGTK_POPUP_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                          YGTK_TYPE_POPUP_WINDOW, YGtkPopupWindow))
#define YGTK_POPUP_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                          YGTK_TYPE_POPUP_WINDOW, YGtkPopupWindowClass))
#define IS_YGTK_POPUP_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                          YGTK_TYPE_POPUP_WINDOW))
#define IS_YGTK_POPUP_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                          YGTK_TYPE_POPUP_WINDOW))
#define YGTK_POPUP_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                          YGTK_TYPE_POPUP_WINDOW, YGtkPopupWindowClass))

typedef struct _YGtkPopupWindow
{
	GtkWindow parent;
} YGtkPopupWindow;

typedef struct _YGtkPopupWindowClass
{
	GtkWindowClass parent_class;
} YGtkPopupWindowClass;

// don't forget to use gtk_widget_show() on the child!
GtkWidget* ygtk_popup_window_new (GtkWidget *child);
GType ygtk_popup_window_get_type (void) G_GNUC_CONST;

void ygtk_popup_window_popup (GtkWidget *widget, gint x, gint y, guint activate_time);

G_END_DECLS
#endif /*YGTK_POPUP_WINDOW_H*/

#ifndef YGTK_MENU_BUTTON_H
#define YGTK_MENU_BUTTON_H

G_BEGIN_DECLS

#define YGTK_TYPE_MENU_BUTTON            (ygtk_menu_button_get_type ())
#define YGTK_MENU_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                          YGTK_TYPE_MENU_BUTTON, YGtkMenuButton))
#define YGTK_MENU_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                          YGTK_TYPE_MENU_BUTTON, YGtkMenuButtonClass))
#define IS_YGTK_MENU_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                          YGTK_TYPE_MENU_BUTTON))
#define IS_YGTK_MENU_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                          YGTK_TYPE_MENU_BUTTON))
#define YGTK_MENU_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                          YGTK_TYPE_MENU_BUTTON, YGtkMenuButtonClass))

typedef struct _YGtkMenuButton
{
	GtkToggleButton parent;

	// private:
	GtkWidget *label, *popup;
	gfloat xalign, yalign;
} YGtkMenuButton;

typedef struct _YGtkMenuButtonClass
{
	GtkToggleButtonClass parent_class;
} YGtkMenuButtonClass;

GtkWidget* ygtk_menu_button_new (void);
GtkWidget* ygtk_menu_button_new_with_label (const gchar *label);
GType ygtk_menu_button_get_type (void) G_GNUC_CONST;

void ygtk_menu_button_set_label (YGtkMenuButton *button, const gchar *label);

/* Popup must be either a GtkMenu or a YGtkPopupWindow. */
// You may hide your popup "manually" issueing a gtk_widget_hide() on it
void ygtk_menu_button_set_popup (YGtkMenuButton *button, GtkWidget *popup);
void ygtk_menu_button_set_popup_align (YGtkMenuButton *button, GtkWidget *popup,
                                       gfloat xalign, gfloat yalign);

G_END_DECLS
#endif /*YGTK_MENU_BUTTON_H*/
