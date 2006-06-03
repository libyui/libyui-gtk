#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YRadioButton.h"
#include "YRadioButtonGroup.h"
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

class YGRadioButton : public YRadioButton, public YGWidget
{
public:
	static void toggled_cb (GtkButton *button, YGRadioButton *pThis)
	{
		IMPL;
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
			pThis->buttonGroup()->uncheckOtherButtons (pThis);
		fprintf (stderr, "Send event!\n");
		if (pThis->getNotify())
			YGUI::ui()->sendEvent( new YWidgetEvent( pThis, YEvent::ValueChanged ) );
	}

	YGRadioButton( const YWidgetOpt &opt,
		       YGWidget         *parent,
		       YCPString         label,
		       YRadioButtonGroup *rbg, 
		       bool checked )
		:  YRadioButton( opt, label, rbg ),
		   YGWidget( this, parent, true,
			   getCheckRadioButtonType(), NULL )
	{
		IMPL;
		gtk_button_set_use_underline (GTK_BUTTON (getWidget()), TRUE);
		g_signal_connect (G_OBJECT (getWidget ()),
				  "toggled", G_CALLBACK (toggled_cb), this);
		setLabel (label);
	}

	// YRadioButton
	virtual void setLabel( const YCPString &label )
	{
		IMPL;
		char *str = YGWidget::mapKBAccel (label->value_cstr());
		gtk_button_set_label (GTK_BUTTON (getWidget()), str);
		g_free (str);
	}

	virtual void setValue( const YCPBoolean & checked )
	{
		IMPL;
		g_signal_handlers_block_by_func
			(getWidget(), (gpointer)toggled_cb, this);

		if (checked->value())
			buttonGroup()->uncheckOtherButtons (this);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (getWidget()),
					      checked->value());

		g_signal_handlers_unblock_by_func
			(getWidget(), (gpointer)toggled_cb, this);
	}
	virtual YCPBoolean getValue()
	{
		IMPL;
		return YCPBoolean( gtk_toggle_button_get_active (
			GTK_TOGGLE_BUTTON (getWidget() ) ) );
	}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
};

class YGRadioButtonGroup : public YRadioButtonGroup, public YGWidget
{
public:
	YGRadioButtonGroup( const YWidgetOpt &opt,
			    YGWidget         *parent ) :
		YRadioButtonGroup(opt),
		YGWidget( this, parent ) {}
	virtual void addRadioButton( YRadioButton *button )
	{
		IMPL;

		YRadioButtonGroup::addRadioButton( button );

		if ( button->getValue()->value() )	// if this new button is active
			uncheckOtherButtons( button );	// make it the only active
	}

	// YWidget
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE_CHAIN(YRadioButtonGroup)
};

YWidget *
YGUI::createRadioButton( YWidget *parent, YWidgetOpt &opt,
			 YRadioButtonGroup *rbg, const YCPString & label,
			 bool checked )
{
	IMPL;
	return new YGRadioButton (opt, YGWidget::get (parent), label, rbg, checked);
}

YContainerWidget *
YGUI::createRadioButtonGroup( YWidget *parent, YWidgetOpt & opt )
{
	IMPL;
	return new YGRadioButtonGroup (opt, YGWidget::get (parent));
}
