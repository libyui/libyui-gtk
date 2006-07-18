/* YGTKSteps widget */

#ifndef YGTK_STEPS_H
#define YGTK_STEPS_H

#include <gdk/gdk.h>
#include <gtk/gtkvbox.h>

G_BEGIN_DECLS

#define YGTK_TYPE_STEPS            (ygtk_steps_get_type ())
#define YGTK_STEPS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                    YGTK_TYPE_STEPS, YGtkSteps))
#define YGTK_STEPS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                    YGTK_TYPE_STEPS, YGtkStepsClass))
#define IS_YGTK_STEPS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                    YGTK_TYPE_STEPS))
#define IS_YGTK_STEPS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                    YGTK_TYPE_STEPS))
#define YGTK_STEPS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                    YGTK_TYPE_STEPS, YGtkStepsClass))

typedef struct _YGtkSteps       YGtkSteps;
typedef struct _YGtkStepsClass  YGtkStepsClass;
typedef struct _YGtkSingleStep  YGtkSingleStep;

struct _YGtkSteps
{
	GtkVBox box;

	// members
	GList *steps;  // of YGtkSingleStep
	guint current_step;
};

struct _YGtkStepsClass
{
	GtkVBoxClass parent_class;
};

struct _YGtkSingleStep 
{
	gboolean is_heading;
	// can either be a GtkLabel or a GtkHBox, with a GtkImage and a GtkLabel,
	// depending on is_heading
	GtkWidget *widget;
	gboolean is_alias;  // duplicated step -- read ygtksteps.c intro text
};

GtkWidget* ygtk_steps_new();
GType ygtk_steps_get_type (void) G_GNUC_CONST;

guint ygtk_steps_append (YGtkSteps *steps, const gchar *label);
void ygtk_steps_append_heading (YGtkSteps *steps, const gchar *heading);

void ygtk_steps_advance (YGtkSteps *steps);
void ygtk_steps_set_current (YGtkSteps *steps, guint step);
guint ygtk_steps_total (YGtkSteps *steps);

void ygtk_steps_clear (YGtkSteps *steps);

G_END_DECLS

#endif /* YGTK_STEPS_H */
