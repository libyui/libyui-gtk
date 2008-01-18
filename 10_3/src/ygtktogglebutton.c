/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkToggleButton widget */
// check the header file for information about this widget

#include <config.h>
#include "ygtktogglebutton.h"
#include <gtk/gtk.h>

static guint toggle_changed_signal;

G_DEFINE_TYPE (YGtkToggleButton, ygtk_toggle_button, GTK_TYPE_TOGGLE_BUTTON)

static void ygtk_toggle_button_init (YGtkToggleButton *button)
{
}

static void ygtk_toggle_button_destroy (GtkObject *object)
{
	GTK_OBJECT_CLASS (ygtk_toggle_button_parent_class)->destroy (object);
	YGtkToggleButton *button = YGTK_TOGGLE_BUTTON (object);
	if (button->group && !button->foreign_group) {
		g_slist_free (button->group);
		button->group = NULL;
	}
}

static void ygtk_toggle_button_toggled (GtkToggleButton *toggle)
{
	GSList *group = YGTK_TOGGLE_BUTTON (toggle)->group;

	if (gtk_toggle_button_get_active (toggle)) {
		// disable any active
		GSList *i;
		for (i = group; i; i = i->next) {
			GtkToggleButton *t = i->data;
			if (t->active && t != toggle) {
				gtk_toggle_button_set_active (t, FALSE);
				break;
			}
		}

		if (i) {
			gint nb = g_slist_index (group, toggle);
			g_signal_emit (YGTK_TOGGLE_BUTTON (toggle), toggle_changed_signal, 0, nb);
		}
	}
	else {
		// force it to be enabled, if no other is (other could be enabled; eg. when
		// this code gets triggered from the previous case).
		GSList *i;
		for (i = group; i; i = i->next) {
			GtkToggleButton *t = i->data;
			if (t->active && t != toggle)
				break;
		}
		if (!i)
			gtk_toggle_button_set_active (toggle, TRUE);
	}
}

GSList *ygtk_toggle_button_get_group (YGtkToggleButton *button)
{
	return button->group;
}

GtkWidget *ygtk_toggle_button_new (GSList *group)
{
	YGtkToggleButton *button = g_object_new (YGTK_TYPE_TOGGLE_BUTTON, NULL);
	button->group = g_slist_append (group, button);
	button->foreign_group = group != NULL;
	return (GtkWidget *) button;
}

static void ygtk_toggle_button_class_init (YGtkToggleButtonClass *klass)
{
	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
	gtkobject_class->destroy = ygtk_toggle_button_destroy;

	GtkToggleButtonClass *gtktogglebutton_class = GTK_TOGGLE_BUTTON_CLASS (klass);
	gtktogglebutton_class->toggled = ygtk_toggle_button_toggled;

	toggle_changed_signal = g_signal_new ("toggle-changed",
		G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (YGtkToggleButtonClass, toggle_changed), NULL, NULL,
		g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);
}

