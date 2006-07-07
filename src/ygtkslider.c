/* YGtkBarGraph widget */

#include "ygtkslider.h"
#include <gtk/gtkhscale.h>
#include <gtk/gtkspinbutton.h>

static void ygtk_slider_class_init (YGtkSliderClass *klass);
static void ygtk_slider_init       (YGtkSlider      *bar);

static void ygtk_slider_spiner_changed (GtkSpinButton *widget, YGtkSlider *slider);
static void ygtk_slider_remain_spiner_changed (GtkSpinButton *widget, YGtkSlider *slider);
static gboolean ygtk_slider_scale_changed
                                  (GtkRange *widget, GtkScrollType *scroll,
                                   gdouble newValue, YGtkSlider *slider);

static gboolean ygtk_slider_mnemonic_activate (GtkWidget *widget, gboolean group_cycling);

static GtkHBoxClass *parent_class = NULL;
static guint value_changed_signal;

GType ygtk_slider_get_type()
{
	static GType slider_type = 0;
	if (!slider_type) {
		static const GTypeInfo slider_info = {
			sizeof (YGtkSliderClass),
			NULL, NULL, (GClassInitFunc) ygtk_slider_class_init, NULL, NULL,
			sizeof (YGtkSlider), 0, (GInstanceInitFunc) ygtk_slider_init, NULL
		};

		slider_type = g_type_register_static (GTK_TYPE_HBOX, "YGtkSlider",
		                                      &slider_info, (GTypeFlags) 0);
	}
	return slider_type;
}

static void ygtk_slider_class_init (YGtkSliderClass *klass)
{
	parent_class = (GtkHBoxClass*) g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->mnemonic_activate  = ygtk_slider_mnemonic_activate;

  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	value_changed_signal = g_signal_new ("value_changed",
	                       G_TYPE_FROM_CLASS (gobject_class),
	                       G_SIGNAL_RUN_LAST,
	                       G_STRUCT_OFFSET (YGtkSliderClass, value_changed),
	                       NULL, NULL,
	                       g_cclosure_marshal_VOID__VOID,
	                       G_TYPE_NONE, 0);
}

static void ygtk_slider_init (YGtkSlider *slider)
{
	slider->spinner = slider->scale = slider->remain_spinner = NULL;
}

void ygtk_slider_configure (YGtkSlider *slider,
                      gdouble min, gdouble max,
                      gboolean show_scale,
                      gboolean show_remain_spinner)
{
	slider->spinner = gtk_spin_button_new_with_range (min, max, 1);
	gtk_widget_show (slider->spinner);

	if (show_remain_spinner) {
		slider->remain_spinner = gtk_spin_button_new_with_range (min, max, 1);
		gtk_spin_button_set_range (GTK_SPIN_BUTTON (slider->remain_spinner), min, max);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (slider->remain_spinner), max);
		gtk_container_add (GTK_CONTAINER (slider), slider->remain_spinner);
		gtk_widget_show (slider->remain_spinner);
	}

	if (show_scale) {
		slider->scale = gtk_hscale_new_with_range (min, max, 1);
		gtk_scale_set_draw_value (GTK_SCALE (slider->scale), FALSE);

		gtk_container_add (GTK_CONTAINER (slider), slider->scale);
		gtk_widget_show (slider->scale);
	}

	gtk_container_add (GTK_CONTAINER (slider), slider->spinner);

	if (show_scale) {
		gtk_box_set_child_packing (GTK_BOX (slider), slider->spinner,
		                           FALSE, FALSE, 5, GTK_PACK_START);
		if (show_remain_spinner)
			gtk_box_set_child_packing (GTK_BOX (slider), slider->remain_spinner,
			                           FALSE, FALSE, 5, GTK_PACK_START);
	}

	g_signal_connect (G_OBJECT (slider->spinner), "value-changed",
	                  G_CALLBACK (ygtk_slider_spiner_changed), slider);
	if (show_scale)
		g_signal_connect (G_OBJECT (slider->scale), "change-value",
		                  G_CALLBACK (ygtk_slider_scale_changed), slider);
	if (show_remain_spinner)
		g_signal_connect (G_OBJECT (slider->remain_spinner), "value-changed",
		                  G_CALLBACK (ygtk_slider_remain_spiner_changed), slider);
}

GtkWidget *ygtk_slider_new (gdouble min, gdouble max,
      gboolean show_scale, gboolean show_remain_spinner)
{
	YGtkSlider* slider = (YGtkSlider*) g_object_new (YGTK_TYPE_SLIDER, NULL);
	ygtk_slider_configure (slider, min, max, show_scale, show_remain_spinner);

	return GTK_WIDGET (slider);
}

void ygtk_slider_set_value (YGtkSlider* slider, gdouble value)
{
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (slider->spinner), value);
	if (slider->scale)
		gtk_range_set_value (GTK_RANGE (slider->scale), value);
	if (slider->remain_spinner) {
		gdouble max;
		gtk_spin_button_get_range (GTK_SPIN_BUTTON (slider->remain_spinner), NULL, &max);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (slider->remain_spinner), max - value);
	}
}

int ygtk_slider_get_value (YGtkSlider* slider)
{
	return gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (slider->spinner));
}

static void ygtk_slider_spiner_changed (GtkSpinButton *widget, YGtkSlider *slider)
{
	int newValue = gtk_spin_button_get_value_as_int (widget);
	if (slider->scale)
		gtk_range_set_value (GTK_RANGE (slider->scale), newValue);
	if (slider->remain_spinner) {
		gdouble max;
		gtk_spin_button_get_range (GTK_SPIN_BUTTON (slider->remain_spinner), NULL, &max);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (slider->remain_spinner), max - newValue);
	}

	g_signal_emit (slider, value_changed_signal, 0);
}

static gboolean ygtk_slider_scale_changed
                                  (GtkRange *widget, GtkScrollType *scroll,
                                   gdouble newValue, YGtkSlider *slider)
{
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (slider->spinner), newValue);
	if (slider->remain_spinner) {
		gdouble max;
		gtk_spin_button_get_range (GTK_SPIN_BUTTON (slider->remain_spinner), NULL, &max);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (slider->remain_spinner), max - newValue);
	}

	g_signal_emit (slider, value_changed_signal, 0);
	return FALSE;
}

static void ygtk_slider_remain_spiner_changed (GtkSpinButton *widget, YGtkSlider *slider)
{
	int newValue = gtk_spin_button_get_value_as_int (widget);
	gdouble max;
	gtk_spin_button_get_range (GTK_SPIN_BUTTON (slider->remain_spinner), NULL, &max);
	newValue = max - newValue;

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (slider->spinner), newValue);
	if (slider->scale)
		gtk_range_set_value (GTK_RANGE (slider->scale), newValue);

	g_signal_emit (slider, value_changed_signal, 0);
}

static gboolean ygtk_slider_mnemonic_activate (GtkWidget *widget, gboolean group_cycling)
{
	YGtkSlider *slider = YGTK_SLIDER (widget);
	if (slider->scale)
		gtk_widget_grab_focus (slider->scale);
	else
		gtk_widget_grab_focus (slider->spinner);
	return FALSE;
}
