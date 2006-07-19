/* Yast GTK interface module. */

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include <string>
#include "YGWidget.h"

class YGSpinBox : public YGLabeledWidget
{
	GtkWidget *m_spiner, *m_slider;

public:
	YGSpinBox (YWidget *y_widget, YGWidget *parent,
	           const YWidgetOpt &opt, const YCPString &label,
	           int minValue, int maxValue, int initialValue,
	           bool show_slider)
	: YGLabeledWidget (y_widget, parent, label, YD_VERT, true,
	                   GTK_TYPE_HBOX, NULL)
	{
		m_spiner = gtk_spin_button_new_with_range  (minValue, maxValue, 1);

		if (show_slider) {
			m_slider = gtk_hscale_new_with_range (minValue, maxValue, 1);
			gtk_scale_set_draw_value (GTK_SCALE (m_slider), FALSE);
			YGLabeledWidget::setBuddy (m_slider);
			gtk_widget_set_size_request (m_slider, 120, -1);

			gtk_container_add (GTK_CONTAINER (getWidget()), m_slider);
			gtk_container_add (GTK_CONTAINER (getWidget()), m_spiner);
			gtk_box_set_child_packing (GTK_BOX (getWidget()), m_spiner,
			                      FALSE, FALSE, 5, GTK_PACK_START);
			gtk_widget_show (m_slider);
		}
		else {
			m_slider = NULL;
			YGLabeledWidget::setBuddy (m_spiner);

			gtk_container_add (GTK_CONTAINER (getWidget()), m_spiner);
		}
		gtk_widget_show (m_spiner);

		doSetValue (initialValue);
		g_signal_connect (G_OBJECT (m_spiner), "value-changed",
		                  G_CALLBACK (spiner_changed_cb), this);
		g_signal_connect (G_OBJECT (m_slider), "change-value",
		                  G_CALLBACK (slider_changed_cb), this);
	}

	GtkSpinButton *getSpiner()
	{ return GTK_SPIN_BUTTON (m_spiner); }

	bool useSlider()
	{ return m_slider != NULL; }
	GtkHScale *getSlider()
	{ return GTK_HSCALE (m_slider); }

	virtual void reportValue (int newValue) {}

	void doSetValue (int newValue)
	{
		gtk_spin_button_set_value (getSpiner(), newValue);
		if (useSlider())
			gtk_range_set_value (GTK_RANGE (getSlider()), newValue);
	}

	// Events callbacks
	static void spiner_changed_cb (GtkSpinButton *widget, YGSpinBox *pThis)
	{
		int newValue = gtk_spin_button_get_value_as_int (pThis->getSpiner());
		pThis->reportValue (newValue);
		if (pThis->useSlider())
			gtk_range_set_value (GTK_RANGE (pThis->getSlider()), newValue);
		pThis->emitEvent (YEvent::ValueChanged);
	}

	static gboolean slider_changed_cb (GtkRange *widget, GtkScrollType *scroll,
	                                        gdouble newValue, YGSpinBox *pThis)
	{
		gtk_spin_button_set_value (pThis->getSpiner(), newValue);
		pThis->reportValue ((int)newValue);
		pThis->emitEvent (YEvent::ValueChanged);
		return FALSE;
	}
};

#define YGSPIN_BOX_IMPL_SET_VALUE_CHAIN(ParentClass) \
	virtual void reportValue (int newValue) {          \
		ParentClass::setValue (newValue);                \
	}                                                  \
	virtual void setValue (int newValue) {             \
		doSetValue (newValue);                           \
		reportValue (newValue);                          \
	}

#include "YIntField.h"

class YGIntField : public YIntField, public YGSpinBox
{
public:
	YGIntField (const YWidgetOpt &opt, YGWidget *parent, const YCPString &label,
	            int minValue, int maxValue, int initialValue)
	: YIntField (opt, label, minValue, maxValue, initialValue)
	, YGSpinBox (this, parent, opt, label, minValue, maxValue, initialValue, false)
	{ }

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_KEYBOARD_FOCUS
	// YGLabelWidget
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YIntField)
	// YGSpinBox
	YGSPIN_BOX_IMPL_SET_VALUE_CHAIN (YIntField)
};

YWidget *
YGUI::createIntField (YWidget *parent, YWidgetOpt &opt,
                                const YCPString &label,
          int minValue, int maxValue, int initialValue)
{
	IMPL
	return new YGIntField (opt, YGWidget::get (parent), label,
	                       minValue, maxValue, initialValue);
}

#include "YSlider.h"

class YGSlider : public YSlider, public YGSpinBox
{
public:
	YGSlider (const YWidgetOpt &opt, YGWidget *parent, const YCPString &label,
	          int minValue, int maxValue, int initialValue)
	: YSlider (opt, label, minValue, maxValue, initialValue)
	, YGSpinBox (this, parent, opt, label, minValue, maxValue, initialValue, true)
	{ }

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_KEYBOARD_FOCUS
	// YGLabelWidget
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YSlider)
	// YGSpinBox
	YGSPIN_BOX_IMPL_SET_VALUE_CHAIN (YSlider)
};

YWidget *
YGUI::createSlider (YWidget *parent, YWidgetOpt &opt,
                                const YCPString &label,
          int minValue, int maxValue, int initialValue)
{
	IMPL
	return new YGSlider (opt, YGWidget::get (parent), label,
	                      minValue, maxValue, initialValue);
}
