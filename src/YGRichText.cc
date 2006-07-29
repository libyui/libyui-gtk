/* Yast GTK */

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
	YGRichText(const YWidgetOpt &opt,
	           YGWidget *parent,
	           const YCPString &text)
	: YRichText (opt, text),
	  YGScrolledWidget (this, parent, true, YGTK_TYPE_RICHTEXT, NULL)
	{
		IMPL;
		m_plainText = opt.plainTextMode.value();
		m_shrinkable = opt.isShrinkable.value();

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
		ygtk_richtext_set_text (YGTK_RICHTEXT (getWidget()), text->value_cstr(), m_plainText);

		if (autoScrollDown)
			YGUtils::scrollTextViewDown (GTK_TEXT_VIEW (getWidget()));
		YRichText::setText (text);
	}

	// YWidget
	// TODO: used by a few more widgets... maybe use some macro approach for this?
	virtual long nicesize (YUIDimension dim)
	{
		IMPL
		long size = getNiceSize (dim);
		return MAX (m_shrinkable ? 10 : 100, size);
	}
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_KEYBOARD_FOCUS

	static void link_pressed_cb (YGtkRichText *rtext, const char *url, YGRichText *pThis)
	{
		YGUI::ui()->sendEvent (new YMenuEvent (YCPString (url)));
	}
};


YWidget *
YGUI::createRichText (YWidget *parent, YWidgetOpt &opt, const YCPString &text)
{
	return new YGRichText (opt, YGWidget::get (parent), text);
}
