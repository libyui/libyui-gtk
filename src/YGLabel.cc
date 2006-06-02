#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YEvent.h"
#include "YLabel.h"
#include "YGWidget.h"

class YGLabel : public YLabel, public YGWidget
{
	const char *m_start_tag;
	const char *m_end_tag;
public:
	YGLabel (const YWidgetOpt &opt,
	         YGWidget *parent,
	         YCPString text)
	: YLabel (opt, text)
	, YGWidget (this, parent, true, GTK_TYPE_LABEL, NULL)
	{
		IMPL;
		if (opt.isHeading.value()) {
			m_start_tag = "span size=\"larger\"";
			m_end_tag = "span";
		} else if (opt.boldFont.value())
			m_start_tag = m_end_tag = "b";
		else
			m_start_tag = m_end_tag = NULL;

		if (opt.isOutputField.value())
			fprintf (stderr, "Should use an insensitive entry instead\n");

		setLabel (text);
	}

	// YLabel
	virtual void setLabel (const YCPString & label)
	{
		YGUtils::setLabel (GTK_LABEL(getWidget()), label);
		YLabel::setLabel( text );
	}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_KEYBOARD_FOCUS
};

YWidget *
YGUI::createLabel (YWidget *parent, YWidgetOpt & opt,
	                 const YCPString & text)
{
	return new YGLabel (opt, YGWidget::get (parent), text);
}
