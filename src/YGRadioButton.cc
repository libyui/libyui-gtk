/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>
#include "YGUI.h"
#include "YGUtils.h"
#include "YGWidget.h"

// Sub-class GtkRadioButton to get a widget that renders like
// a radio-button, but behaves like a check/toggle-button.
static GType getCheckRadioButtonType()
{
	static GType type = 0;

	if (type)
		return type;

	static const GTypeInfo info = {
		sizeof (GtkRadioButtonClass), NULL, NULL,
		NULL, NULL, NULL,
		sizeof (GtkRadioButton), 0, NULL
		};
	type = g_type_register_static (GTK_TYPE_RADIO_BUTTON, "YGRadioButton",
	                               &info, GTypeFlags(0));
	// save a class_init function
	GtkButtonClass *klass_new = GTK_BUTTON_CLASS (g_type_class_ref (type));
	GtkButtonClass *klass_sane =
		GTK_BUTTON_CLASS (g_type_class_ref (GTK_TYPE_TOGGLE_BUTTON));
	klass_new->clicked = klass_sane->clicked;
	return type;
}

#include "YRadioButtonGroup.h"
#include "YRadioButton.h"

class YGRadioButton : public YRadioButton, public YGWidget
{
public:
	YGRadioButton (YWidget *parent, const std::string &label, bool isChecked)
	:  YRadioButton (NULL, label),
	   YGWidget (this, parent, true, getCheckRadioButtonType(), NULL)
	{
		IMPL
		setBorder (0);
		setValue (isChecked);

		gtk_button_set_use_underline (GTK_BUTTON (getWidget()), TRUE);
		setLabel (label);

		g_signal_connect (G_OBJECT (getWidget()), "toggled",
		                  G_CALLBACK (toggled_cb), this);
	}

	// YRadioButton
	virtual void setLabel (const string &text)
	{
		// NOTE: we can't just set a gtk_widget_modify() at the initialization
		// because each gtk_button_set_label() creates a new label
		IMPL
		string str = YGUtils::mapKBAccel(text.c_str());
		gtk_button_set_label (GTK_BUTTON (getWidget()), str.c_str());
		YRadioButton::setLabel (text);
	}

	virtual bool value()
	{
		IMPL
		return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (getWidget()));
	}

	virtual void setValue (bool checked)
	{
		IMPL
		g_signal_handlers_block_by_func (getWidget(), (gpointer) toggled_cb, this);

		if (checked)
			buttonGroup()->uncheckOtherButtons ((YRadioButton *) this);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (getWidget()), checked);

		g_signal_handlers_unblock_by_func (getWidget(), (gpointer) toggled_cb, this);
	}

	YGWIDGET_IMPL_COMMON
	YGWIDGET_IMPL_USE_BOLD (YRadioButton)

	// callbacks
	static void toggled_cb (GtkButton *button, YGRadioButton *pThis)
	{
		IMPL
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button))) {
			pThis->buttonGroup()->uncheckOtherButtons ((YRadioButton *) pThis);
			pThis->emitEvent (YEvent::ValueChanged);
		}
		else {
			// leave it active
			g_signal_handlers_block_by_func (button, (gpointer) toggled_cb, pThis);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
			g_signal_handlers_unblock_by_func (button, (gpointer) toggled_cb, pThis);
		}
	}
};

YRadioButton *YGWidgetFactory::createRadioButton (YWidget *parent, const string &label,
                                                  bool isChecked)
{
	IMPL
	return new YGRadioButton (parent, label, isChecked);
}

// YRadioButtonGroup

class YGRadioButtonGroup : public YRadioButtonGroup, public YGWidget
{
public:
	YGRadioButtonGroup(YWidget *parent)
	: YRadioButtonGroup (NULL),
	  YGWidget (this, parent, true, GTK_TYPE_EVENT_BOX, NULL)
	{}

	YGWIDGET_IMPL_COMMON
	YGWIDGET_IMPL_CHILD_ADDED (m_widget)
	YGWIDGET_IMPL_CHILD_REMOVED (m_widget)
};

YRadioButtonGroup *YGWidgetFactory::createRadioButtonGroup (YWidget *parent)
{
	return new YGRadioButtonGroup (parent);
}

#include "YCheckBox.h"

class YGCheckBox : public YCheckBox, public YGWidget
{
	bool m_isBold;

public:
	YGCheckBox(YWidget *parent, const string &label, bool isChecked)
	:  YCheckBox (NULL, label),
	   YGWidget (this, parent, true, GTK_TYPE_CHECK_BUTTON, NULL)
	{
		IMPL
		setBorder (0);

		gtk_button_set_use_underline (GTK_BUTTON (getWidget()), TRUE);
		setLabel (label);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (getWidget()), isChecked);

		g_signal_connect (G_OBJECT (getWidget ()), "toggled",
		                  G_CALLBACK (toggled_cb), this);
	}

	// YCheckButton
	virtual void setLabel (const string &text)
	{
		IMPL
		string str = YGUtils::mapKBAccel(text);
		gtk_button_set_label (GTK_BUTTON (getWidget()), str.c_str());
		YCheckBox::setLabel (text);
	}

	virtual YCheckBoxState value()
	{
		IMPL
		GtkToggleButton *button = GTK_TOGGLE_BUTTON (getWidget());

		if (gtk_toggle_button_get_inconsistent (button))
			return YCheckBox_dont_care;
		return gtk_toggle_button_get_active (button) ? YCheckBox_on : YCheckBox_off;
	}

	virtual void setValue (YCheckBoxState value)
	{
		IMPL
		GtkToggleButton *button = GTK_TOGGLE_BUTTON (getWidget());

		g_signal_handlers_block_by_func (getWidget(), (gpointer) toggled_cb, this);

		switch (value) {
			case YCheckBox_dont_care:
				gtk_toggle_button_set_inconsistent (button, TRUE);
				break;
			case YCheckBox_on:
				gtk_toggle_button_set_inconsistent (button, FALSE);
				gtk_toggle_button_set_active (button, TRUE);
				break;
			case YCheckBox_off:
				gtk_toggle_button_set_inconsistent (button, FALSE);
				gtk_toggle_button_set_active (button, FALSE);
				break;
		}

		g_signal_handlers_unblock_by_func (getWidget(), (gpointer) toggled_cb, this);
	}

	static void toggled_cb (GtkBox *box, YGCheckBox *pThis)
	{
		IMPL
		GtkToggleButton *button = GTK_TOGGLE_BUTTON (box);
		if (gtk_toggle_button_get_inconsistent (button))
			pThis->setValue (YCheckBox_on);
		pThis->emitEvent (YEvent::ValueChanged);
	}

	YGWIDGET_IMPL_COMMON
	YGWIDGET_IMPL_USE_BOLD (YCheckBox)
};

YCheckBox *YGWidgetFactory::createCheckBox (YWidget *parent, const string &label,
                                            bool isChecked)
{
	IMPL
	return new YGCheckBox (parent, label, isChecked);
}

