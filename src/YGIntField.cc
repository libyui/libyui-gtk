/* Yast GTK interface module. */

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include <string>
#include "YGWidget.h"
#include "ygtkslider.h"

#include "YIntField.h"

class YGIntField : public YIntField, public YGLabeledWidget
{
public:
	YGIntField (const YWidgetOpt &opt, YGWidget *parent, const YCPString &label,
	            int minValue, int maxValue, int initialValue)
	: YIntField (opt, label, minValue, maxValue, initialValue)
	, YGLabeledWidget (this, parent, label, YD_VERT, true, YGTK_TYPE_SLIDER, NULL)
	{
		ygtk_slider_configure (YGTK_SLIDER (getWidget()), minValue, maxValue,
		                       FALSE, FALSE);
		g_signal_connect (G_OBJECT (getWidget()), "value-changed",
		                  G_CALLBACK (slider_changed_cb), this);
	}

	virtual void setValue (int newValue)
	{
		ygtk_slider_set_value (YGTK_SLIDER (getWidget()), newValue);
		YIntField::setValue (newValue);
	}

	static void slider_changed_cb (YGtkSlider *slider, YGIntField *pThis)
	{
		pThis->emitEvent (YEvent::ValueChanged);
	}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_KEYBOARD_FOCUS
	// YGLabelWidget
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YIntField)
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

class YGSlider : public YSlider, public YGLabeledWidget
{
public:
	YGSlider (const YWidgetOpt &opt, YGWidget *parent, const YCPString &label,
	            int minValue, int maxValue, int initialValue)
	: YSlider (opt, label, minValue, maxValue, initialValue)
	, YGLabeledWidget (this, parent, label, YD_VERT, true, YGTK_TYPE_SLIDER, NULL)
	{
		ygtk_slider_configure (YGTK_SLIDER (getWidget()), minValue, maxValue,
		                       TRUE, FALSE);
		g_signal_connect (G_OBJECT (getWidget()), "value-changed",
		                  G_CALLBACK (slider_changed_cb), this);
	}

	virtual void setValue (int newValue)
	{
		ygtk_slider_set_value (YGTK_SLIDER (getWidget()), newValue);
		YSlider::setValue (newValue);
	}

	static void slider_changed_cb (YGtkSlider *slider, YGIntField *pThis)
	{
		pThis->emitEvent (YEvent::ValueChanged);
	}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_KEYBOARD_FOCUS
	// YGLabelWidget
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YSlider)
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
