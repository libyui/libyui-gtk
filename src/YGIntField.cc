/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#define YUILogComponent "gtk"
#include "YGUI.h"
#include "YGUtils.h"
#include "YGWidget.h"

class YGSpinBox : public YGLabeledWidget
{
	YIntField *m_yfield;
	GtkWidget *m_spiner, *m_slider;

public:
	YGSpinBox (YWidget *ywidget, YWidget *parent, const std::string &label,
	           int minValue, int maxValue, int initialValue, bool show_slider)
	: YGLabeledWidget (ywidget, parent, label, YD_HORIZ,
	                   gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0), "spacing", 6, NULL)
	{
		m_spiner = gtk_spin_button_new_with_range  (minValue, maxValue, 1);

		if (show_slider) {
			m_slider = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, minValue, maxValue, 1);
			if (maxValue - minValue < 20)
				// GtkScale by default uses a step of 10 -- use a lower for low values
				gtk_range_set_increments (GTK_RANGE (m_slider), 1, 2);
			gtk_scale_set_draw_value (GTK_SCALE (m_slider), FALSE);
			YGLabeledWidget::setBuddy (m_slider);
			gtk_widget_set_size_request (m_slider, 100, -1);

			gtk_box_pack_start (GTK_BOX (getWidget()), m_slider, TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (getWidget()), m_spiner, FALSE, TRUE, 0);
			gtk_widget_show (m_slider);
		}
		else {
			m_slider = NULL;
			YGLabeledWidget::setBuddy (m_spiner);
			gtk_container_add (GTK_CONTAINER (getWidget()), m_spiner);
		}
		gtk_widget_show (m_spiner);

		doSetValue (initialValue);
		connect (m_spiner, "value-changed", G_CALLBACK (spiner_changed_cb), this);
        if (m_slider)
		    connect (m_slider, "value-changed", G_CALLBACK (slider_changed_cb), this);
	}

	GtkSpinButton *getSpiner()
	{ return GTK_SPIN_BUTTON (m_spiner); }

	bool useSlider()
    { return m_slider != NULL; }
	GtkRange *getSlider()
	{ return GTK_RANGE (m_slider); }

	virtual void reportValue (int value) = 0;

	int doGetValue()
	{ return gtk_spin_button_get_value_as_int (getSpiner()); }

	void doSetValue (int value)
	{
		BlockEvents block (this);
		gtk_spin_button_set_value (getSpiner(), value);
		if (useSlider())
			gtk_range_set_value (getSlider(), value);
	}

	// Events callbacks
	static void spiner_changed_cb (GtkSpinButton *widget, YGSpinBox *pThis)
	{
		int value = gtk_spin_button_get_value_as_int (pThis->getSpiner());
		pThis->reportValue (value);
		if (pThis->useSlider())
			gtk_range_set_value (pThis->getSlider(), value);
		pThis->emitEvent (YEvent::ValueChanged);
	}

	static void slider_changed_cb (GtkRange *range, YGSpinBox *pThis)
	{
		int value = (int) gtk_range_get_value (range);
		gtk_spin_button_set_value (pThis->getSpiner(), value);
		pThis->reportValue (value);
		pThis->emitEvent (YEvent::ValueChanged);
	}
};

#define YGSPIN_BOX_IMPL(ParentClass) \
	virtual void reportValue (int value) {           \
		ParentClass::setValue (value);               \
	}                                                \
	virtual int value() {                            \
		return doGetValue();                         \
	}                                                \
	virtual void setValueInternal (int value) {      \
		doSetValue (value);                          \
	}

#include "YIntField.h"

class YGIntField : public YIntField, public YGSpinBox
{
public:
	YGIntField (YWidget *parent, const std::string &label, int minValue, int maxValue,
	            int initialValue)
	: YIntField (NULL, label, minValue, maxValue)
	, YGSpinBox (this, parent, label, minValue, maxValue, initialValue, false)
	{}

	YGLABEL_WIDGET_IMPL (YIntField)
	YGSPIN_BOX_IMPL (YIntField)
};

YIntField *YGWidgetFactory::createIntField (YWidget *parent, const std::string &label,
                                            int minValue, int maxValue, int initialValue)
{ return new YGIntField (parent, label, minValue, maxValue, initialValue); }

#include "YSlider.h"

class YGSlider : public YSlider, public YGSpinBox
{
public:
	YGSlider (YWidget *parent, const std::string &label, int minValue, int maxValue,
	          int initialValue)
	: YSlider (NULL, label, minValue, maxValue)
	, YGSpinBox (this, parent, label, minValue, maxValue, initialValue, true)
	{}

	virtual unsigned int getMinSize (YUIDimension dim)
	{ return dim == YD_HORIZ ? 200 : 0; }

	YGLABEL_WIDGET_IMPL (YSlider)
	YGSPIN_BOX_IMPL (YSlider)
};

YSlider *YGOptionalWidgetFactory::createSlider (YWidget *parent, const std::string &label,
                                                int minValue, int maxValue, int initialValue)
{ return new YGSlider (parent, label, minValue, maxValue, initialValue); }

