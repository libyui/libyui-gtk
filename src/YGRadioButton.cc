#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YRadioButton.h"
#include "YRadioButtonGroup.h"
#include "YGWidget.h"

class YGRadioButton : public YRadioButton, public YGWidget
{
public:
	YGRadioButton( const YWidgetOpt &opt,
				   YGWidget         *parent,
				   YCPString         label,
				   YRadioButtonGroup *rbg, 
				   bool checked );
	virtual ~YGRadioButton()
	{
		g_warning ("destroy radio button %p", this);
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
		// FIXME: block signal emissions from this ... (?)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (getWidget()),
									  checked->value());
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


static void
toggled_cb (GtkButton *button, YGRadioButton *pThis)
{
	IMPL;
    YGUI::ui()->sendEvent( new YWidgetEvent( pThis, YEvent::ValueChanged ) );
}

YGRadioButton::YGRadioButton( const YWidgetOpt &opt,
							  YGWidget         *parent,
							  YCPString         label,
							  YRadioButtonGroup *rbg, 
							  bool checked )
		:  YRadioButton( opt, label, rbg ),
		   YGWidget( this, parent, true,
					 GTK_TYPE_RADIO_BUTTON, NULL )
{
	g_warning ("create radio button %p", this);
	IMPL;
	gtk_button_set_use_underline (GTK_BUTTON (getWidget()), TRUE);
	g_signal_connect (G_OBJECT (getWidget ()),
					  "toggled", G_CALLBACK (toggled_cb), this);
	setLabel (label);
}

class YGRadioButtonGroup : public YRadioButtonGroup, public YGWidget
{
	// YRadioButtons ...
	static GtkRadioButton *get( YRadioButton *button )
	{
		return GTK_RADIO_BUTTON (YGWidget::get(button)->getWidget());
	}
public:
	std::list<GtkRadioButton *> m_group;
	YGRadioButtonGroup( const YWidgetOpt &opt,
						YGWidget         *parent );
	~YGRadioButtonGroup()
	{
		g_warning ("destroy radio group %p (%d)", this, m_group.size());
		while (m_group.size() > 0)
		{
			GtkRadioButton *rb = *(m_group.begin());
			g_signal_handlers_disconnect_matched (
				G_OBJECT (rb),
				(GSignalMatchType) G_SIGNAL_MATCH_FUNC,
				0, 0, 0, (void *)toggled_cb, 0);
			gtk_radio_button_set_group (rb, NULL);
			m_group.pop_front();
		}
	}
	virtual void addRadioButton( YRadioButton *button );
    virtual void removeRadioButton( YRadioButton *button )
	{
		IMPL;
		g_warning ("remove radio button %p from group %p", button, this);
		GtkRadioButton *rb = get (button);
		gtk_radio_button_set_group (rb, NULL);
		m_group.remove (rb);
	}

    // YWidget
    YGWIDGET_IMPL_SET_ENABLING
};

YGRadioButtonGroup::YGRadioButtonGroup( const YWidgetOpt &opt,
										YGWidget         *parent ) :
	YRadioButtonGroup(opt),
	YGWidget( this, parent )
{
	m_group.clear();
	g_warning ("create group %p", this);
}

#if 0
static void
group_changed_cb (GtkRadioButton *rb, YGRadioButtonGroup *pThis)
{
	if (gtk_radio_button_get_group(rb))
		return;
	g_warning ("group changed on group %p remove %p", pThis, rb);
	pThis->m_group.remove (rb);
}
#endif

void YGRadioButtonGroup::addRadioButton( YRadioButton *button )
{
	IMPL;
	g_warning ("add button %p to group %p", button, this);
	GtkRadioButton *rb = get (button);
	GSList *group = NULL;
	if (m_group.size() > 0)
		group = gtk_radio_button_get_group (
				GTK_RADIO_BUTTON (*(m_group.begin())));
	gtk_radio_button_set_group (rb, group);
//	g_signal_connect (G_OBJECT (rb), "group_changed",
//					  G_CALLBACK (group_changed_cb), this);
	m_group.push_back (rb);
}

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
