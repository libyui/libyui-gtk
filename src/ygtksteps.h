/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkSteps is a widget that displays a list of steps, useful
   to show the progress of some configuration tool as it does
   the work of updating files or services or whatever.

   You use the append functions to initializate the steps. If,
   for some reason, you want one step to count as more than one
   (ie. so that a ygtk_steps_advance() needs to be called more than
    once to actually advance for a given step), you may append a
   step with the same name of the previous, that they'll be collapsed.
   (Internally, we call that the 'strength' of the step.)

	TODO: the PangoLayout usage is getting a bit hacky. We may want to
	replace it with Cairo methods.
*/

#ifndef YGTK_STEPS_H
#define YGTK_STEPS_H

#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

#define YGTK_TYPE_STEPS            (ygtk_steps_get_type ())
#define YGTK_STEPS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                    YGTK_TYPE_STEPS, YGtkSteps))
#define YGTK_STEPS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                    YGTK_TYPE_STEPS, YGtkStepsClass))
#define YGTK_IS_STEPS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                    YGTK_TYPE_STEPS))
#define YGTK_IS_STEPS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                    YGTK_TYPE_STEPS))
#define YGTK_STEPS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                    YGTK_TYPE_STEPS, YGtkStepsClass))

typedef struct _YGtkSteps
{
	GtkWidget parent;

	// private:
	GList *steps;  // of YGtkSingleStep
	guint current_step;
	PangoLayout *check_mark_layout, *current_mark_layout;
	// for current_mark little animation
	guint current_mark_timeout_id, current_mark_frame;
	
} YGtkSteps;

typedef struct _YGtkStepsClass
{
	GtkWidgetClass parent_class;
} YGtkStepsClass;

typedef struct _YGtkSingleStep
{
	gboolean is_heading;
	gchar *text;
	guint strength;  // check the text at top

	// private -- don't access it, call ygtk_step_get_layout() instead
	PangoLayout *layout, *layout_bold;  // cache
} YGtkSingleStep;

GtkWidget* ygtk_steps_new (void);
GType ygtk_steps_get_type (void) G_GNUC_CONST;

guint ygtk_steps_append (YGtkSteps *steps, const gchar *label);
void ygtk_steps_append_heading (YGtkSteps *steps, const gchar *heading);

void ygtk_steps_advance (YGtkSteps *steps);
void ygtk_steps_set_current (YGtkSteps *steps, guint step);
guint ygtk_steps_total (YGtkSteps *steps);

void ygtk_steps_clear (YGtkSteps *steps);

G_END_DECLS

#endif /* YGTK_STEPS_H */
