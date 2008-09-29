/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkProgressBar widget */
// check the header file for information about this widget

#include <config.h>
#include "ygtkprogressbar.h"

#define ENABLE_CONTINUOUS
#define STEP_TIME 50

G_DEFINE_TYPE (YGtkProgressBar, ygtk_progress_bar, GTK_TYPE_PROGRESS_BAR)

static void ygtk_progress_bar_init (YGtkProgressBar *bar)
{
	bar->continuous = TRUE;
}

static void remove_timeout (YGtkProgressBar *bar)
{
	if (bar->step_timeout_id) {
		g_source_remove (bar->step_timeout_id);
		bar->step_timeout_id = 0;
	}
}

static void ygtk_progress_bar_finalize (GObject *object)
{
	YGtkProgressBar *bar = YGTK_PROGRESS_BAR (object);
	remove_timeout (bar);
	G_OBJECT_CLASS (ygtk_progress_bar_parent_class)->finalize (object);
}

static void ygtk_progress_bar_realize (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (ygtk_progress_bar_parent_class)->realize (widget);
	YGtkProgressBar *bar = YGTK_PROGRESS_BAR (widget);
	if (bar->continuous)
		ygtk_progress_bar_set_fraction (bar, 0);
}

GtkWidget* ygtk_progress_bar_new (void)
{ return g_object_new (YGTK_TYPE_PROGRESS_BAR, NULL); }

#ifdef ENABLE_CONTINUOUS

static gboolean step_timeout_cb (void *pdata)
{
	YGtkProgressBar *ybar = (YGtkProgressBar *) pdata;
	GtkProgressBar *bar = GTK_PROGRESS_BAR (ybar);

	gdouble vel = (ybar->vel/1000) * STEP_TIME;
	gdouble fraction = gtk_progress_bar_get_fraction (bar) + vel;
	if (fraction >= ybar->dist) {
		gtk_progress_bar_set_fraction (bar, ybar->dist);
		ybar->step_timeout_id = 0;
		return FALSE;
	}
	gtk_progress_bar_set_fraction (bar, fraction);
	return TRUE;
}

static gdouble delta_time (GTimeVal *time_i)
{
	GTimeVal time_f;
	g_get_current_time (&time_f);
	gdouble sec = time_f.tv_sec - time_i->tv_sec;
	gdouble usec = time_f.tv_usec - time_i->tv_usec;
	return sec + usec/G_USEC_PER_SEC;
}

void ygtk_progress_bar_set_fraction (YGtkProgressBar *ybar, gdouble fraction)
{
	GtkProgressBar *bar = GTK_PROGRESS_BAR (ybar);
	if (ybar->continuous) {
		if (fraction == 0) {
#if 1
			ybar->dist = ybar->vel = 0;
#else  // kick some movement already
			ybar->dist = 0.10;
			ybar->vel = 0.02;
#endif
			gtk_progress_bar_set_fraction (bar, 0);
		}
		else {
			if (ybar->fraction == 0 && fraction == 1) {
				gtk_progress_bar_set_fraction (bar, 1);
				return;
			}
			gdouble delta_t = delta_time (&ybar->time_i);
			if (delta_t > 10 || fraction < ybar->fraction || !GTK_WIDGET_REALIZED (bar)) {
				ybar->fraction = fraction;
				ygtk_progress_bar_disable_continuous (ybar);
				return;
			}

			gdouble anim_fraction = gtk_progress_bar_get_fraction (bar);
			ybar->dist = MIN (2*fraction - ybar->fraction, 1);
			if (delta_t < 0.01)
				ybar->vel = 0.10;
			else {
				ybar->vel = MAX ((ybar->dist - anim_fraction) / delta_t, 0);
				if (ybar->dist == 1)  // finishing up!
					ybar->vel += 0.020;
				else  // better slower than wait
					ybar->vel = MAX (ybar->vel-0.005, 0);
			}
/*			char *text = g_strdup_printf ("%d %%", (int)(fraction*100));
			gtk_progress_bar_set_text (bar, text);
			g_free (text);*/
		}
		g_get_current_time (&ybar->time_i);
		ybar->fraction = fraction;
		if (!ybar->step_timeout_id)
			ybar->step_timeout_id = g_timeout_add (STEP_TIME, step_timeout_cb, ybar);
	}
	else
		gtk_progress_bar_set_fraction (bar, fraction);
}

#else

void ygtk_progress_bar_set_fraction (YGtkProgressBar *ybar, gdouble fraction)
{
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (ybar), fraction);
}

#endif

void ygtk_progress_bar_disable_continuous (YGtkProgressBar *bar)
{
	bar->continuous = FALSE;
	remove_timeout (bar);
	if (!GTK_WIDGET_REALIZED (bar))
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), bar->fraction);
}

static void ygtk_progress_bar_class_init (YGtkProgressBarClass *klass)
{
	ygtk_progress_bar_parent_class = g_type_class_peek_parent (klass);

	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = ygtk_progress_bar_finalize;

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->realize  = ygtk_progress_bar_realize;
}

