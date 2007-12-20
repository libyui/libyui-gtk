/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkToggleButton extends GtkToggleButton to add groups, just like GtkRadioButtons.
*/

#ifndef YGTK_TOGGLE_BUTTON_H
#define YGTK_TOGGLE_BUTTON_H

#include <gtk/gtktogglebutton.h>
G_BEGIN_DECLS

#define YGTK_TYPE_TOGGLE_BUTTON            (ygtk_toggle_button_get_type ())
#define YGTK_TOGGLE_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	YGTK_TYPE_TOGGLE_BUTTON, YGtkToggleButton))
#define YGTK_TOGGLE_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
	YGTK_TYPE_TOGGLE_BUTTON, YGtkToggleButtonClass))
#define YGTK_IS_TOGGLE_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                              YGTK_TYPE_TOGGLE_BUTTON))
#define YGTK_IS_TOGGLE_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                              YGTK_TYPE_TOGGLE_BUTTON))
#define YGTK_TOGGLE_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
	YGTK_TYPE_TOGGLE_BUTTON, YGtkToggleButtonClass))

typedef struct _YGtkToggleButton
{
	GtkToggleButton parent;
	// members:
	GSList *group;
	gboolean foreign_group;  // (shouldn't be needed...)
} YGtkToggleButton;

typedef struct _YGtkToggleButtonClass
{
	GtkToggleButtonClass parent_class;

	// signals:
	void (*toggle_changed) (GtkToggleButton *toggle, gint nb);
} YGtkToggleButtonClass;

GtkWidget* ygtk_toggle_button_new (GSList *group);
GType ygtk_toggle_button_get_type (void) G_GNUC_CONST;

GSList *ygtk_toggle_button_get_group (YGtkToggleButton *button);

G_END_DECLS
#endif /*YGTK_TOGGLE_BUTTON_H*/

