/* Yast GTK interface module. */

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include <string>
#include "YEvent.h"
#include "YIntField.h"
#include "YGWidget.h"

class YGIntField : public YIntField, public YGLabeledWidget
{
public:
	YGIntField (const YWidgetOpt &opt, YGWidget *parent, const YCPString &label,
	            int minValue, int maxValue, int initialValue)
	: YIntField (opt, label, minValue, maxValue, initialValue)
	, YGLabeledWidget (this, parent, label, YD_VERT, true,
	                   GTK_TYPE_SPIN_BUTTON, "numeric", TRUE, NULL)
	{
		gtk_spin_button_set_range (GTK_SPIN_BUTTON (getWidget()),
		                                     minValue, maxValue);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (getWidget()), initialValue);
		gtk_spin_button_set_increments (GTK_SPIN_BUTTON (getWidget()), 1, 2);

		g_signal_connect (G_OBJECT (getWidget()), "value-changed",
		                  G_CALLBACK (value_changed_cb), this);
	}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_KEYBOARD_FOCUS
	// YGLabelWidget
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN(YIntField)

	// YIntField
	virtual void setValue (int newValue)
	{
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (getWidget()), newValue);
		YIntField::setValue (newValue);
	}

	// Events callbacks
	static void value_changed_cb (GtkSpinButton *widget, YGIntField *pThis)
	{
		pThis->YIntField::setValue
			(gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget)));
		if (pThis->getNotify())
			YGUI::ui()->sendEvent (new YWidgetEvent (pThis, YEvent::ValueChanged));
	}
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
