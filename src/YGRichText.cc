/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YGWidget.h"
#include "YRichText.h"
#include "ygtkhtmlwrap.h"

class YGRichText : public YRichText, public YGScrolledWidget
{
bool m_autoScrollDown;

public:
	YGRichText(const YWidgetOpt &opt, YGWidget *parent, const YCPString &text)
	: YRichText (opt, text),
	  YGScrolledWidget (this, parent, true, ygtk_htmlwrap_get_type (opt.plainTextMode.value()), NULL)
	{
		IMPL
		if (!opt.isShrinkable.value())
			setMinSizeInChars (20, 8);
		m_autoScrollDown = opt.autoScrollDown.value();

		ygtk_htmlwrap_init (getWidget());
		ygtk_htmlwrap_connect_link_clicked (getWidget(), G_CALLBACK (link_clicked_cb), this);

		setText (text);
	}

	virtual ~YGRichText()
	{ }

	// YRichText
	virtual void setText (const YCPString &_text)
	{
		IMPL
		string text (_text->value());
		YGUtils::replace (text, "&product;", 9, YUI::ui()->productName().c_str());

		ygtk_htmlwrap_set (getWidget(), text.c_str());
		if (m_autoScrollDown)
			ygtk_htmlwrap_scroll (getWidget(), FALSE);
		YRichText::setText (_text);
	}

	static void link_clicked_cb (GtkWidget *widget, const char *url, YGRichText *pThis)
	{
		YGUI::ui()->sendEvent (new YMenuEvent (YCPString (url)));
	}

	YGWIDGET_IMPL_COMMON
};


YWidget *
YGUI::createRichText (YWidget *parent, YWidgetOpt &opt, const YCPString &text)
{
	return new YGRichText (opt, YGWidget::get (parent), text);
}

