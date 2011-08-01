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

	Using GtkLabels, so it should be ATK friendly.
*/

#ifndef YGTK_STEPS_H
#define YGTK_STEPS_H
#include <gtk/gtkvbox.h>
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
	GtkVBox parent;

	// private:
	gint current_step;
	PangoLayout *check_mark_layout, *current_mark_layout, *todo_mark_layout;
	// for current_mark little animation
	guint current_mark_timeout_id, current_mark_frame;
	
} YGtkSteps;

typedef struct _YGtkStepsClass
{
	GtkVBoxClass parent_class;
} YGtkStepsClass;

GtkWidget* ygtk_steps_new (void);
GType ygtk_steps_get_type (void) G_GNUC_CONST;

gint ygtk_steps_append (YGtkSteps *steps, const gchar *label);
void ygtk_steps_append_heading (YGtkSteps *steps, const gchar *heading);

void ygtk_steps_set_current (YGtkSteps *steps, gint step);  /* -1 = none */
gint ygtk_steps_total (YGtkSteps *steps);
const gchar *ygtk_steps_get_nth_label (YGtkSteps *steps, gint n);

void ygtk_steps_clear (YGtkSteps *steps);

G_END_DECLS

#endif /* YGTK_STEPS_H */
