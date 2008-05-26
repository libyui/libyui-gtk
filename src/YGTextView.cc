/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <YGUI.h>
#include <string>
#include "YGUtils.h"
#include "YGWidget.h"

class YGTextView : public YGScrolledWidget
{
int maxChars;

public:
	YGTextView (YWidget *ywidget, YWidget *parent, const string &label, bool editable)
		: YGScrolledWidget (ywidget, parent, label, YD_VERT, true,
		                    GTK_TYPE_TEXT_VIEW, "wrap-mode", GTK_WRAP_WORD, NULL)
	{
		IMPL
		setMinSizeInChars (20, 10);
		setPolicy (GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

		maxChars = -1;
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

	void setText (const string &text)
	{
		IMPL
		gtk_text_buffer_set_text (getBuffer(), text.c_str(), -1);
	}

	string getText()
	{
		IMPL
		GtkTextIter start_it, end_it;
		gtk_text_buffer_get_bounds (getBuffer(), &start_it, &end_it);

		gchar* text = gtk_text_buffer_get_text (getBuffer(), &start_it, &end_it, FALSE);
		string str (text);
		g_free (text);
		return str;
	}

	void scrollToBottom()
	{
		YGUtils::scrollWidget (GTK_TEXT_VIEW (getWidget()), false);
	}

	// Event callbacks
	static void text_changed_cb (GtkTextBuffer *buffer, YGTextView *pThis)
	{
		if (pThis->maxChars != -1 && pThis->getCharsNb() > pThis->maxChars) {
			pThis->truncateText (pThis->maxChars);
			gtk_widget_error_bell (pThis->getWidget());
		}
		pThis->emitEvent (YEvent::ValueChanged);
	}
};

#include "YMultiLineEdit.h"

class YGMultiLineEdit : public YMultiLineEdit, public YGTextView
{
public:
	YGMultiLineEdit (YWidget *parent, const string &label)
	: YMultiLineEdit (NULL, label)
	, YGTextView (this, parent, label, true)
	{}

	// YMultiLineEdit
	virtual void setValue (const string &text)
	{ YGTextView::setText (text); }

	virtual string value()
	{ return YGTextView::getText(); }

	virtual void setInputMaxLength (int nb)
	{
		YGTextView::setCharsNb (nb);
		YMultiLineEdit::setInputMaxLength (nb);
	}

	YGWIDGET_IMPL_COMMON
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN(YMultiLineEdit)
};

YMultiLineEdit *YGWidgetFactory::createMultiLineEdit (YWidget *parent, const string &label)
{
	return new YGMultiLineEdit (parent, label);
}

#include "YLogView.h"

class YGLogView : public YLogView, public YGTextView
{
public:
	YGLogView (YWidget *parent, const string &label, int visibleLines, int maxLines)
	: YLogView (NULL, label, visibleLines, maxLines)
	, YGTextView (this, parent, label, false)
	{}

	// YLogView
	virtual void displayLogText (const string &text)
	{
		setText (text);
		scrollToBottom();
	}

	YGWIDGET_IMPL_COMMON
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN(YLogView)
};

YLogView *YGWidgetFactory::createLogView (YWidget *parent, const string &label,
                                          int visibleLines, int maxLines)
{
	return new YGLogView (parent, label, visibleLines, maxLines);
}

#include "YRichText.h"
#include "ygtkhtmlwrap.h"

class YGRichText : public YRichText, YGScrolledWidget
{
public:
	YGRichText (YWidget *parent, const string &text, bool plainText)
	: YRichText (NULL, text, plainText)
	, YGScrolledWidget (this, parent, true, ygtk_html_wrap_get_type(), NULL)
	{
		IMPL
		if (!shrinkable())
			setMinSizeInChars (20, 8);

		ygtk_html_wrap_init (getWidget());
		ygtk_html_wrap_connect_link_clicked (getWidget(), G_CALLBACK (link_clicked_cb), this);
		setText (text, plainText);
	}

	void setPlainText (const string &text)
	{
		ygtk_html_wrap_set_text (getWidget(), text.c_str(), TRUE);
	}

	void setRichText (const string &_text)
	{
		string text (_text);
		std::string productName = YUI::app()->productName();
		YGUtils::replace (text, "&product;", 9, productName.c_str());
		ygtk_html_wrap_set_text (getWidget(), text.c_str(), FALSE);
	}

	void scrollToBottom()
	{
		ygtk_html_wrap_scroll (getWidget(), FALSE);
	}

	void setText (const string &text, bool plain_mode)
	{
		plain_mode ?  setPlainText (text) : setRichText (text);
		if (autoScrollDown())
			scrollToBottom();
	}

	// YRichText
	virtual void setValue (const string &text)
	{
		IMPL
		setText (text, plainTextMode());
		YRichText::setValue (text);
	}

    virtual void setAutoScrollDown (bool on)
    {
    	if (on) scrollToBottom();
    	YRichText::setAutoScrollDown (on);
    }

    virtual void setPlainTextMode (bool plain_mode)
    {
    	if (plain_mode != plainTextMode())
	    	setText (value(), plain_mode);
    	YRichText::setPlainTextMode (plain_mode);
    }

	// events
	static void link_clicked_cb (GtkWidget *widget, const char *url, YGRichText *pThis)
	{
		YGUI::ui()->sendEvent (new YMenuEvent (url));
	}

	YGWIDGET_IMPL_COMMON
};


YRichText *YGWidgetFactory::createRichText (YWidget *parent, const string &text,
                                            bool plainTextMode)
{
	return new YGRichText (parent, text, plainTextMode);
}

