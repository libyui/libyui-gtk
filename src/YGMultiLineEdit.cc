/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include <string>
#include "YGUtils.h"
#include "YGWidget.h"

class YGTextView : public YGScrolledWidget
{
int maxChars;

public:
	YGTextView (YWidget *y_widget, YGWidget *parent, const YWidgetOpt &opt,
	            const YCPString &label, const YCPString &text, bool editable)
		: YGScrolledWidget (y_widget, parent, label, YD_VERT, true,
		                    GTK_TYPE_TEXT_VIEW, "wrap-mode", GTK_WRAP_WORD, NULL)
	{
		IMPL
		if (!opt.isShrinkable.value())
			setMinSizeInChars (20, 10);
		setPolicy (GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

		maxChars = -1;
		setText (text);

		if (!editable)
		{
			gtk_text_view_set_editable (GTK_TEXT_VIEW (getWidget()), FALSE);
			gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (getWidget()), FALSE);
		}

		g_signal_connect (G_OBJECT (getBuffer()), "changed",
		                  G_CALLBACK (text_changed_cb), this);
	}

	GtkTextBuffer* getBuffer()
	{ return gtk_text_view_get_buffer (GTK_TEXT_VIEW (getWidget())); }

	int getCharsNb()
	{
		IMPL
		return gtk_text_buffer_get_char_count (getBuffer());
	}

	void setCharsNb (int max_chars)
	{
		IMPL
		maxChars = max_chars;
		if (maxChars != -1 && getCharsNb() > maxChars)
			truncateText (maxChars);
	}

	void truncateText(int pos)
	{
		IMPL
		GtkTextIter start_it, end_it;
		gtk_text_buffer_get_iter_at_offset (getBuffer(), &start_it, pos);
		gtk_text_buffer_get_end_iter (getBuffer(), &end_it);

		g_signal_handlers_block_by_func (getWidget(), (gpointer) text_changed_cb, this);
		gtk_text_buffer_delete (getBuffer(), &start_it, &end_it);
		g_signal_handlers_unblock_by_func (getWidget(), (gpointer) text_changed_cb, this);
	}

	void setText (const YCPString &text)
	{
		IMPL
		gtk_text_buffer_set_text (getBuffer(), text->value_cstr(), -1);
	}

	YCPString getText()
	{
		IMPL
		GtkTextIter start_it, end_it;
		gtk_text_buffer_get_bounds (getBuffer(), &start_it, &end_it);

		gchar* text = gtk_text_buffer_get_text (getBuffer(), &start_it, &end_it, FALSE);
		YCPString str (text);
		g_free (text);
		return str;
	}

	void scrollToBottom()
	{
		YGUtils::scrollTextViewDown (GTK_TEXT_VIEW (getWidget()));
	}

	// Event callbacks
	static void text_changed_cb (GtkTextBuffer *buffer, YGTextView *pThis)
	{
		if (pThis->maxChars != -1 && pThis->getCharsNb() > pThis->maxChars) {
			pThis->truncateText (pThis->maxChars);
			gdk_beep();
		}
		pThis->emitEvent (YEvent::ValueChanged);
	}
};

#include "YMultiLineEdit.h"

class YGMultiLineEdit : public YMultiLineEdit, public YGTextView
{
public:
	YGMultiLineEdit (const YWidgetOpt &opt, YGWidget *parent,
	                 const YCPString &label, const YCPString &text)
	: YMultiLineEdit (opt, label)
	, YGTextView (this, parent, opt, label, text, true)
	{ }

	// YMultiLineEdit
	virtual void setText (const YCPString &text)
	{ YGTextView::setText (text); }

	virtual YCPString text()
	{ return YGTextView::getText(); }

	virtual void setInputMaxLength (const YCPInteger &numberOfChars)
	{ YGTextView::setCharsNb (numberOfChars->asInteger()->value()); }

	YGWIDGET_IMPL_COMMON
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN(YMultiLineEdit)
};

YWidget *
YGUI::createMultiLineEdit (YWidget *parent, YWidgetOpt & opt,
                           const YCPString & label, const YCPString & text)
{
	return new YGMultiLineEdit (opt, YGWidget::get (parent), label, text);
}

#include "YLogView.h"

class YGLogView : public YLogView, public YGTextView
{
public:
	YGLogView (const YWidgetOpt &opt, YGWidget *parent,
	           const YCPString &label, int visibleLines, int maxLines)
	: YLogView (opt, label, visibleLines, maxLines)
	, YGTextView (this, parent, opt, label, YCPString(""), false)
	{
		setMinSizeInChars (0, visibleLines);
	}

	// YLogView
	virtual void setLogText (const YCPString &text)
	{
		setText (text);
		scrollToBottom();
	}

	YGWIDGET_IMPL_COMMON
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN(YLogView)
};

YWidget *
YGUI::createLogView (YWidget *parent, YWidgetOpt &opt,
                     const YCPString &label, int visibleLines,
                     int maxLines)
{
	return new YGLogView (opt, YGWidget::get (parent),
	                      label, visibleLines, maxLines);
}

#include "YRichText.h"

class YGPlainText : public YRichText, public YGTextView
{
bool m_autoScrollDown;

public:
	YGPlainText(const YWidgetOpt &opt, YGWidget *parent, const YCPString &text)
	: YRichText (opt, text)
	, YGTextView (this, parent, opt, YCPString(""), text, false)
	{
		IMPL
		m_autoScrollDown = opt.autoScrollDown.value();
	}

	// YRichText
	virtual void setText (const YCPString &text)
	{
		IMPL
		YGTextView::setText (text);
		if (m_autoScrollDown)
			scrollToBottom();
		YRichText::setText (text);
	}

	YGWIDGET_IMPL_COMMON
};

#include "ygtkhtmlwrap.h"

class YGRichText : public YRichText, public YGScrolledWidget
{
bool m_autoScrollDown;

public:
	YGRichText(const YWidgetOpt &opt, YGWidget *parent, const YCPString &text)
	: YRichText (opt, text)
	, YGScrolledWidget (this, parent, true, ygtk_html_wrap_get_type(), NULL)
	{
		IMPL
		if (!opt.isShrinkable.value())
			setMinSizeInChars (20, 8);
		m_autoScrollDown = opt.autoScrollDown.value();

		ygtk_html_wrap_init (getWidget());
		ygtk_html_wrap_connect_link_clicked (getWidget(), G_CALLBACK (link_clicked_cb), this);

		setText (text);
	}

	// YRichText
	virtual void setText (const YCPString &_text)
	{
		IMPL
		string text (_text->value());
		YGUtils::replace (text, "&product;", 9, YUI::ui()->productName().c_str());

		ygtk_html_wrap_set_text (getWidget(), text.c_str());
		if (m_autoScrollDown)
			ygtk_html_wrap_scroll (getWidget(), FALSE);
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
	if (opt.plainTextMode.value())
		return new YGPlainText (opt, YGWidget::get (parent), text);
	else
		return new YGRichText (opt, YGWidget::get (parent), text);
}

