/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>
#include "YGUI.h"
#include "YGUtils.h"
#include "YGWidget.h"

#if 0
//** Older approach:
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
#endif

#include "YRadioButton.h"
#include "YRadioButtonGroup.h"

class YGRadioButton : public YRadioButton, public YGWidget
{
public:
	YGRadioButton (YWidget *parent, const std::string &label, bool isChecked)
	:  YRadioButton (NULL, label),
	   YGWidget (this, parent, true, GTK_TYPE_RADIO_BUTTON, NULL)
	{
		IMPL
		setBorder (0);
		setLabel (label);
		gtk_button_set_use_underline (GTK_BUTTON (getWidget()), TRUE);

		g_signal_connect_after (G_OBJECT (getWidget()), "toggled",
		                        G_CALLBACK (toggled_cb), this);
	}

	GSList *setGroup (GSList *group)
	{
		gtk_radio_button_set_group (GTK_RADIO_BUTTON (getWidget()), group);
		return gtk_radio_button_get_group (GTK_RADIO_BUTTON (getWidget()));
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
		if (checked) {  // can't uncheck a radio button
			g_signal_handlers_block_by_func (getWidget(), (gpointer) toggled_cb, this);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (getWidget()), checked);
			g_signal_handlers_unblock_by_func (getWidget(), (gpointer) toggled_cb, this);
		}
	}

	YGWIDGET_IMPL_COMMON
	YGWIDGET_IMPL_USE_BOLD (YRadioButton)

	// callbacks
	static void toggled_cb (GtkButton *button, YGRadioButton *pThis)
	{
		IMPL
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
			pThis->emitEvent (YEvent::ValueChanged);
	}
};

YRadioButton *YGWidgetFactory::createRadioButton (YWidget *parent, const string &label,
                                                  bool isChecked)
{
	IMPL
	YRadioButton *button = new YGRadioButton (parent, label, isChecked);

	// libyui instructs us to do it here due to vfuncs craziness
	YRadioButtonGroup *group = button->buttonGroup();
	if (group)
		group->addRadioButton (button);
	button->setValue (isChecked);

	return button;
}

// YRadioButtonGroup

class YGRadioButtonGroup : public YRadioButtonGroup, public YGWidget
{
GSList *group;

public:
	YGRadioButtonGroup(YWidget *parent)
	: YRadioButtonGroup (NULL),
	  YGWidget (this, parent, true, GTK_TYPE_EVENT_BOX, NULL)
	{
		group = NULL;
	}

	virtual void addRadioButton (YRadioButton *_button)
	{
		YGRadioButton *button = (YGRadioButton *) YGWidget::get (_button);
		group = button->setGroup (group);
		YRadioButtonGroup::addRadioButton (_button);
	}

	virtual void removeRadioButton (YRadioButton *_button)
	{
		YGRadioButton *button = (YGRadioButton *) YGWidget::get (_button);
		button->setGroup (NULL);
		YRadioButtonGroup::removeRadioButton (_button);
	}

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

