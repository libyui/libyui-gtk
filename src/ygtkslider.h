/* YGtkSlider widget */

#ifndef YGTK_SLIDER_H
#define YGTK_SLIDER_H

#include <gdk/gdk.h>
#include <gtk/gtkhbox.h>

G_BEGIN_DECLS

#define YGTK_TYPE_SLIDER            (ygtk_slider_get_type ())
#define YGTK_SLIDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                     YGTK_TYPE_SLIDER, YGtkSlider))
#define YGTK_SLIDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                     YGTK_TYPE_SLIDER, YGtkSliderClass))
#define IS_YGTK_SLIDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                     YGTK_TYPE_SLIDER))
#define IS_YGTK_SLIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                     YGTK_TYPE_SLIDER))
#define YGTK_SLIDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                     YGTK_TYPE_SLIDER, YGtkSliderClass))

typedef struct _YGtkSlider       YGtkSlider;
typedef struct _YGtkSliderClass  YGtkSliderClass;

struct _YGtkSlider
{
	GtkHBox hbox;

	// members
	GtkWidget *spinner;
	GtkWidget *scale;
	GtkWidget *remain_spinner;
};

struct _YGtkSliderClass
{
	GtkHBoxClass parent_class;

	// signal
	void (*value_changed) (YGtkSlider *slider);
};

GType ygtk_slider_get_type (void) G_GNUC_CONST;
GtkWidget *ygtk_slider_new (gdouble min, gdouble max,
                            gboolean show_scale,
                            gboolean show_remain_spinner);
// to be called only once (and in case you haven't used ygtk_slider_new()):
void ygtk_slider_configure (YGtkSlider *slider,
                            gdouble min, gdouble max,
                            gboolean show_scale,
                            gboolean show_remain_spinner);

int  ygtk_slider_get_value (YGtkSlider* slider);
void ygtk_slider_set_value (YGtkSlider* slider, gdouble value);

G_END_DECLS

#endif /*YGTK_SLIDER_H*/
