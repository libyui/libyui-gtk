#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YPushButton.h"
#include "YGUtils.h"
#include "YGWidget.h"

class YGPushButton : public YPushButton, public YGWidget
{
public:
	YGPushButton( const YWidgetOpt &opt,
				  YGWidget         *parent,
				  YCPString         label );
	virtual ~YGPushButton() {}

    // YPushButton
    virtual void setLabel( const YCPString &label )
	{
		IMPL;
		string str = YGUtils::mapKBAccel(label->value_cstr());
		gtk_button_set_label (GTK_BUTTON (getWidget()), str.c_str());
	}

    virtual void setIcon (const YCPString & icon_name) IMPL;

    // YWidget
    YGWIDGET_IMPL_NICESIZE
    YGWIDGET_IMPL_SET_SIZE
    YGWIDGET_IMPL_SET_ENABLING
};

static void
clicked_cb (GtkButton *button, YGPushButton *pThis)
{
    YGUI::ui()->sendEvent( new YWidgetEvent( pThis, YEvent::Activated ) );
}

YGPushButton::YGPushButton( const YWidgetOpt &opt,
							YGWidget         *parent,
							YCPString         label )
		:  YPushButton( opt, label ),
		   YGWidget( this, parent, true,
					 GTK_TYPE_BUTTON, NULL )
{
	IMPL;
	gtk_button_set_use_underline (GTK_BUTTON (getWidget()), TRUE);
	g_signal_connect (G_OBJECT (getWidget ()),
					  "clicked", G_CALLBACK (clicked_cb), this);
	setLabel (label);
}

YWidget *
YGUI::createPushButton( YWidget *parent, YWidgetOpt &opt,
						const YCPString &label )
{
	return new YGPushButton (opt, YGWidget::get (parent), label);
}
