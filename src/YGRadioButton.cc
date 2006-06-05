#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YCheckBox.h"
#include "YRadioButton.h"
#include "YRadioButtonGroup.h"
#include "YGUtils.h"
#include "YGWidget.h"

// Sub-class GtkRadioButton to get a widget that renders like
// a radio-button, but behaves like a check/toggle-button.
static GType getCheckRadioButtonType()
{
	static GType type = 0;

	if (type)
		return type;

	static const GTypeInfo info =
		{ sizeof (GtkRadioButtonClass), NULL, NULL,
		  NULL, NULL, NULL,
		  sizeof (GtkRadioButton), 0, NULL };
	type = g_type_register_static (GTK_TYPE_RADIO_BUTTON, "YGRadioButton",
				       &info, GTypeFlags(0));
	// save a class_init fn.
	GtkButtonClass *klass_new = GTK_BUTTON_CLASS (g_type_class_ref (type));
	GtkButtonClass *klass_sane = GTK_BUTTON_CLASS (g_type_class_ref (GTK_TYPE_TOGGLE_BUTTON));
	klass_new->clicked = klass_sane->clicked;
	return type;
}

// YRadioButton

class YGRadioButton : public YRadioButton, public YGWidget
{
public:
	static void toggled_cb (GtkButton *button, YGRadioButton *pThis)
	{
		IMPL;
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
			pThis->buttonGroup()->uncheckOtherButtons (pThis);
		if (pThis->getNotify())
			YGUI::ui()->sendEvent( new YWidgetEvent( pThis, YEvent::ValueChanged ) );
	}

	gulong m_signal;

	YGRadioButton (const YWidgetOpt &opt,
		       YGWidget         *parent,
		       YCPString         label,
		       YRadioButtonGroup *rbg, 
		       bool               checked)
		:  YRadioButton (opt, label, rbg),
		   YGWidget (this, parent, true,
			     getCheckRadioButtonType(), NULL)
	{
		IMPL;
		gtk_button_set_use_underline (GTK_BUTTON (getWidget()), TRUE);
		m_signal = g_signal_connect (G_OBJECT (getWidget()),
					     "toggled", G_CALLBACK (toggled_cb), this);
		setLabel (label);
	}

	// YRadioButton
	virtual void setLabel (const YCPString &label)
	{
		IMPL;
		gtk_button_set_label (GTK_BUTTON (getWidget()),
			YGUtils::mapKBAccel (label->value_cstr()).c_str());
	}

	virtual void setValue (const YCPBoolean &checked)
	{
		IMPL;
		g_signal_handler_block (getWidget(), m_signal);

		if (checked->value())
			buttonGroup()->uncheckOtherButtons (this);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (getWidget()),
					      checked->value());

		g_signal_handler_unblock (getWidget(), m_signal);
	}

	virtual YCPBoolean getValue()
	{
		IMPL;
		return YCPBoolean (gtk_toggle_button_get_active (
			GTK_TOGGLE_BUTTON (getWidget())));
	}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
};

YWidget *
YGUI::createRadioButton (YWidget *parent, YWidgetOpt &opt,
			 YRadioButtonGroup *rbg, const YCPString &label,
			 bool checked)
{
	IMPL;
	return new YGRadioButton (opt, YGWidget::get (parent), label, rbg, checked);
}

// YRadioButtonGroup

class YGRadioButtonGroup : public YRadioButtonGroup, public YGWidget
{
public:
	YGRadioButtonGroup(const YWidgetOpt &opt,
			   YGWidget         *parent) :
		YRadioButtonGroup (opt),
		YGWidget (this, parent) {}
	virtual void addRadioButton (YRadioButton *button)
	{
		IMPL;

		YRadioButtonGroup::addRadioButton (button);

		if (button->getValue()->value())
			uncheckOtherButtons (button);
	}

	// YWidget
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE_CHAIN (YRadioButtonGroup)
};

YContainerWidget *
YGUI::createRadioButtonGroup (YWidget *parent, YWidgetOpt &opt)
{
	IMPL;
	return new YGRadioButtonGroup (opt, YGWidget::get (parent));
}


// YCheckBox

class YGCheckBox : public YCheckBox, public YGWidget
{
public:
	static void toggled_cb (GtkBox *box, YGCheckBox *pThis)
	{
		IMPL;
		GtkToggleButton *button = GTK_TOGGLE_BUTTON (box);
		if (gtk_toggle_button_get_inconsistent (button))
		{
			gtk_toggle_button_set_inconsistent (button, false);
			pThis->setValue (YCPBoolean (true));
		}
		if (pThis->getNotify())
			YGUI::ui()->sendEvent( new YWidgetEvent( pThis, YEvent::ValueChanged ) );
	}

	gulong m_signal;

	YGCheckBox(const YWidgetOpt &opt,
		   YGWidget         *parent,
		   YCPString         label,
		   bool              checked)
		:  YCheckBox (opt, label),
		   YGWidget (this, parent, true, GTK_TYPE_CHECK_BUTTON, NULL)
	{
		IMPL;
		gtk_button_set_use_underline (GTK_BUTTON (getWidget()), TRUE);
		m_signal = g_signal_connect (G_OBJECT (getWidget ()),
					     "toggled", G_CALLBACK (toggled_cb), this);
		setLabel (label);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (getWidget()), checked);
	}

	virtual void setLabel (const YCPString &label)
	{
		IMPL;
		gtk_button_set_label (GTK_BUTTON (getWidget()),
			YGUtils::mapKBAccel (label->value_cstr()).c_str());
	}

	virtual void setValue (const YCPValue &val)
	{
		IMPL;
		GtkToggleButton *button = GTK_TOGGLE_BUTTON (getWidget());

		g_signal_handler_block (getWidget(), m_signal);

		if (val->isBoolean()) {
			gtk_toggle_button_set_inconsistent (button, FALSE);
			gtk_toggle_button_set_active (button, 
						      val->asBoolean()->value());
		} else
			gtk_toggle_button_set_inconsistent (button, TRUE);

		g_signal_handler_unblock (getWidget(), m_signal);
	}
	virtual YCPValue getValue()
	{
		IMPL;

		GtkToggleButton *button = GTK_TOGGLE_BUTTON (getWidget());

		if (gtk_toggle_button_get_inconsistent (button))
			return YCPVoid();
		else
			return YCPBoolean (gtk_toggle_button_get_active (button));
	}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
};

YWidget *
YGUI::createCheckBox (YWidget *parent, YWidgetOpt &opt,
		      const YCPString &label,
		      bool checked)
{
	IMPL;
	return new YGCheckBox (opt, YGWidget::get (parent), label, checked);
}
