#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
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
	bool m_isBold;

public:

	YGRadioButton (const YWidgetOpt  &opt,
	               YGWidget          *parent,
	               YCPString          label,
	               YRadioButtonGroup *rbg, 
	               bool               checked)
	:  YRadioButton (opt, label, rbg),
	   YGWidget (this, parent, true, getCheckRadioButtonType(), NULL)
	{
		IMPL
		setBorder (0);
		setValue (YCPBoolean (checked));

		m_isBold = opt.boldFont.value();
		gtk_button_set_use_underline (GTK_BUTTON (getWidget()), TRUE);
		setLabel (label);

		g_signal_connect (G_OBJECT (getWidget()), "toggled",
		                  G_CALLBACK (toggled_cb), this);
	}

	// YRadioButton
	virtual void setLabel (const YCPString &text)
	{
		// NOTE: we can't just set a gtk_widget_modify() at the initialization
		// because each gtk_button_set_label() creates a new label
		IMPL
		string str = YGUtils::mapKBAccel(text->value_cstr());
		if (m_isBold)
			str = "<b>" + str + "</b>";
		gtk_button_set_label (GTK_BUTTON (getWidget()), str.c_str());
		YRadioButton::setLabel (text);

		GtkWidget *label = gtk_bin_get_child (GTK_BIN (getWidget()));
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	}

	virtual void setValue (const YCPBoolean &checked)
	{
		IMPL
		g_signal_handlers_block_by_func (getWidget(), (gpointer) toggled_cb, this);

		if (checked->value())
			buttonGroup()->uncheckOtherButtons (this);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (getWidget()),
					      checked->value());

		g_signal_handlers_unblock_by_func (getWidget(), (gpointer) toggled_cb, this);
	}

	virtual YCPBoolean getValue()
	{
		IMPL
		return YCPBoolean (gtk_toggle_button_get_active
		                       (GTK_TOGGLE_BUTTON (getWidget())));
	}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING

	// callbacks
	static void toggled_cb (GtkButton *button, YGRadioButton *pThis)
	{
		IMPL
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
			pThis->buttonGroup()->uncheckOtherButtons (pThis);
		else {
			g_signal_handlers_block_by_func (button, (gpointer) toggled_cb, pThis);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
			g_signal_handlers_unblock_by_func (button, (gpointer) toggled_cb, pThis);
		}

		pThis->emitEvent (YEvent::ValueChanged);
	}
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
	                   YGWidget         *parent)
	: YRadioButtonGroup (opt),
	  YGWidget (this, parent)
	{ }

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

#include "YCheckBox.h"

class YGCheckBox : public YCheckBox, public YGWidget
{
	bool m_isBold;

public:
	YGCheckBox(const YWidgetOpt &opt,
	           YGWidget         *parent,
	           const YCPString  &label_str,
	           bool              checked)
	:  YCheckBox (opt, label_str),
	   YGWidget (this, parent, true, GTK_TYPE_CHECK_BUTTON, NULL)
	{
		IMPL
		setBorder (0);
		m_isBold = opt.boldFont.value();

		gtk_button_set_use_underline (GTK_BUTTON (getWidget()), TRUE);
		setLabel (label_str);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (getWidget()), checked);

		g_signal_connect (G_OBJECT (getWidget ()), "toggled",
		                  G_CALLBACK (toggled_cb), this);
	}

	// YCheckButton
	virtual void setLabel (const YCPString &text)
	{
		IMPL
		string str = YGUtils::mapKBAccel(text->value_cstr());
		if (m_isBold)
			str = "<b>" + str + "</b>";
		gtk_button_set_label (GTK_BUTTON (getWidget()), str.c_str());
		YCheckBox::setLabel (text);

		GtkWidget *label = gtk_bin_get_child (GTK_BIN (getWidget()));
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	}

	virtual void setValue (const YCPValue &val)
	{
		IMPL;
		GtkToggleButton *button = GTK_TOGGLE_BUTTON (getWidget());

		g_signal_handlers_block_by_func (getWidget(), (gpointer) toggled_cb, this);

		if (val->isBoolean()) {
			gtk_toggle_button_set_inconsistent (button, FALSE);
			gtk_toggle_button_set_active (button, val->asBoolean()->value());
		}
		else
			gtk_toggle_button_set_inconsistent (button, TRUE);

		g_signal_handlers_unblock_by_func (getWidget(), (gpointer) toggled_cb, this);
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

	static void toggled_cb (GtkBox *box, YGCheckBox *pThis)
	{
		IMPL
		GtkToggleButton *button = GTK_TOGGLE_BUTTON (box);
		if (gtk_toggle_button_get_inconsistent (button)) {
			gtk_toggle_button_set_inconsistent (button, false);
			pThis->setValue (YCPBoolean (true));
		}

		pThis->emitEvent (YEvent::ValueChanged);
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
