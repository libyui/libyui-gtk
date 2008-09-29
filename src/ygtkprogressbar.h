/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkProgressBar makes the progress moves go smooth.
*/

#ifndef YGTK_PROGRESS_BAR_H
#define YGTK_PROGRESS_BAR_H

#include <gtk/gtkprogressbar.h>

G_BEGIN_DECLS

#define YGTK_TYPE_PROGRESS_BAR            (ygtk_progress_bar_get_type ())
#define YGTK_PROGRESS_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                           YGTK_TYPE_PROGRESS_BAR, YGtkProgressBar))
#define YGTK_PROGRESS_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                           YGTK_TYPE_PROGRESS_BAR, YGtkProgressBarClass))
#define YGTK_IS_PROGRESS_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                           YGTK_TYPE_PROGRESS_BAR))
#define YGTK_IS_PROGRESS_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                           YGTK_TYPE_PROGRESS_BAR))
#define YGTK_PROGRESS_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                           YGTK_TYPE_PROGRESS_BAR, YGtkProgresBarClass))

typedef struct _YGtkProgressBar
{
	GtkProgressBar parent;
	// private:
	gdouble vel, dist, fraction;
	guint step_timeout_id;
	GTimeVal time_i;
	gboolean continuous;
} YGtkProgressBar;

typedef struct _YGtkProgressBarClass
{
	GtkProgressBarClass parent_class;
} YGtkProgressBarClass;

GtkWidget* ygtk_progress_bar_new (void);
GType ygtk_progress_bar_get_type (void) G_GNUC_CONST;

void ygtk_progress_bar_set_fraction (YGtkProgressBar *bar, gdouble fraction);
void ygtk_progress_bar_disable_continuous (YGtkProgressBar *bar);

G_END_DECLS

#endif /* YGTK_PROGRESS_BAR_H */

