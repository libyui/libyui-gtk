/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YGWidget.h"
#include "YRichText.h"
#include "ygtkrichtext.h"

class YGRichText : public YRichText, public YGScrolledWidget
{
	bool m_shrinkable;
	bool m_plainText;

public:
	YGRichText(const YWidgetOpt &opt, YGWidget *parent, const YCPString &text)
	: YRichText (opt, text),
	  YGScrolledWidget (this, parent, true, YGTK_TYPE_RICHTEXT, NULL)
	{
		IMPL
		if (!opt.isShrinkable.value())
			setMinSizeInChars (20, 8);

		m_plainText = opt.plainTextMode.value();
		ygtk_richttext_set_prodname (YGTK_RICHTEXT (getWidget()),
		                             YUI::ui()->productName().c_str());

		g_signal_connect (G_OBJECT (getWidget()), "link-pressed",
		                  G_CALLBACK (link_pressed_cb), this);

		setText (text);
	}

	virtual ~YGRichText()
	{ }

	// YRichText
	virtual void setText (const YCPString &text)
	{
		IMPL
		ygtk_richtext_set_text (YGTK_RICHTEXT (getWidget()), text->value_cstr(),
		                        !m_plainText);

		if (autoScrollDown)
			YGUtils::scrollTextViewDown (GTK_TEXT_VIEW (getWidget()));
		YRichText::setText (text);
	}

	static void link_pressed_cb (YGtkRichText *rtext, const char *url, YGRichText *pThis)
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
