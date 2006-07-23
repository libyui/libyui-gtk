#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YGUtils.h"
#include "YGWidget.h"

// YRadioButton
#include "YRadioButton.h"
#include "YRadioButtonGroup.h"

// YRadioButtonGroup
class YGRadioButton;

class YGRadioButtonGroup : public YRadioButtonGroup, public YGWidget
{
GSList *group;  // of GtkRadioButton
friend class YGRadioButton;

public:
	YGRadioButtonGroup(const YWidgetOpt &opt, YGWidget *parent)
	: YRadioButtonGroup (opt),
	  YGWidget (this, parent)
	{
		IMPL
		group = NULL;
	}

	~YGRadioButtonGroup()
	{
		IMPL
		if (group)
			g_slist_free (group);
	}

	virtual void addRadioButton (YRadioButton *button)
	{
		IMPL
		GtkWidget *widget = ((YGWidget *) button->widgetRep())->getWidget();
		group = g_slist_append (group, (gpointer) widget);

		YRadioButtonGroup::addRadioButton (button);
	}

	virtual void removeRadioButton (YRadioButton *button)
	{
		IMPL
		GtkWidget *widget = ((YGWidget *) button->widgetRep())->getWidget();
		group = g_slist_remove (group, (gpointer) widget);

		YRadioButtonGroup::removeRadioButton (button);
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


// YRadioButton
class YGRadioButton : public YRadioButton, public YGWidget
{
	bool m_isBold;

public:
	YGRadioButton (const YWidgetOpt  &opt,
	               YGWidget          *parent,
	               YCPString          label,
	               YRadioButtonGroup *rb_group,
	               bool               checked)
	:  YRadioButton (opt, label, rb_group),
	   YGWidget (this, parent, true, GTK_TYPE_RADIO_BUTTON, NULL)
	{
		IMPL
		m_isBold = opt.boldFont.value();

		GSList *group = ((YGRadioButtonGroup *) rb_group)->group;
		if (group)
			gtk_radio_button_set_group (GTK_RADIO_BUTTON (getWidget()), group);
		if (checked)
			setValue (YCPBoolean (true));

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

	static void toggled_cb (GtkButton *button, YGRadioButton *pThis)
	{
		IMPL
		pThis->emitEvent (YEvent::ValueChanged);
	}
};

YWidget *
YGUI::createRadioButton (YWidget *parent, YWidgetOpt &opt,
                         YRadioButtonGroup *rbg, const YCPString &label,
                         bool checked)
{
	IMPL
	return new YGRadioButton (opt, YGWidget::get (parent), label, rbg, checked);
}

// YCheckBox
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
		if (gtk_toggle_button_get_inconsistent (button))
		{
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
