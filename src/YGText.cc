/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#define YUILogComponent "gtk"
#include "config.h"
#include "YGUI.h"
#include <string>
#include "YGUtils.h"
#include "YGWidget.h"
#include "ygtktextview.h"

class YGTextView : public YGScrolledWidget
{
int maxChars;

public:
	YGTextView (YWidget *ywidget, YWidget *parent, const std::string &label, bool editable)
		: YGScrolledWidget (ywidget, parent, label, YD_VERT,
		                    YGTK_TYPE_TEXT_VIEW, "wrap-mode", GTK_WRAP_WORD_CHAR,
		                    "editable", editable, NULL)
	{
		setPolicy (GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
		maxChars = -1;
		connect (getBuffer(), "changed", G_CALLBACK (text_changed_cb), this);
	}

	GtkTextBuffer* getBuffer()
	{ return gtk_text_view_get_buffer (GTK_TEXT_VIEW (getWidget())); }

	int getCharsNb()
	{ return gtk_text_buffer_get_char_count (getBuffer()); }

	void setCharsNb (int max_chars)
	{
		maxChars = max_chars;
		if (maxChars != -1 && getCharsNb() > maxChars)
			truncateText (maxChars);
	}

	void truncateText(int pos)
	{
		BlockEvents block (this);
		GtkTextIter start_it, end_it;
		gtk_text_buffer_get_iter_at_offset (getBuffer(), &start_it, pos);
		gtk_text_buffer_get_end_iter (getBuffer(), &end_it);
		gtk_text_buffer_delete (getBuffer(), &start_it, &end_it);
	}

	void setText (const std::string &text)
	{
		BlockEvents block (this);
		gtk_text_buffer_set_text (getBuffer(), text.c_str(), -1);
	}

	void appendText (const std::string &text)
	{
		GtkTextIter end_it;
		gtk_text_buffer_get_end_iter (getBuffer(), &end_it);
		gtk_text_buffer_insert (getBuffer(), &end_it, text.c_str(), -1);
	}

	std::string getText()
	{
		GtkTextIter start_it, end_it;
		gtk_text_buffer_get_bounds (getBuffer(), &start_it, &end_it);

		gchar* text = gtk_text_buffer_get_text (getBuffer(), &start_it, &end_it, FALSE);
		std::string str (text);
		g_free (text);
		return str;
	}

	void scrollToBottom()
	{
#if 1
                YGUtils::scrollWidget (gtk_scrollable_get_vadjustment(GTK_SCROLLABLE (getWidget())), false);
#else
		GtkTextBuffer *buffer = getBuffer();
		GtkTextIter iter;
		gtk_text_buffer_get_end_iter (buffer, &iter);
		gtk_text_iter_set_line_offset (&iter, 0);

		GtkTextMark *mark = gtk_text_buffer_get_mark (buffer, "scroll");
		if (mark)
			gtk_text_buffer_move_mark (buffer, mark, &iter);
		else
			mark = gtk_text_buffer_create_mark (buffer, "scroll", &iter, TRUE);

		GtkTextView *view = GTK_TEXT_VIEW (getWidget());
		gtk_text_view_scroll_mark_onscreen (view, mark);
#endif
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
	YGMultiLineEdit (YWidget *parent, const std::string &label)
	: YMultiLineEdit (NULL, label)
	, YGTextView (this, parent, label, true)
	{}

	// YMultiLineEdit
	virtual void setValue (const std::string &text)
	{ YGTextView::setText (text); }

	virtual std::string value()
	{ return YGTextView::getText(); }

	virtual void setInputMaxLength (int nb)
	{
		YGTextView::setCharsNb (nb);
		YMultiLineEdit::setInputMaxLength (nb);
	}

	// YGWidget

	virtual unsigned int getMinSize (YUIDimension dim)
	{
		if (dim == YD_VERT) {
			int height = YGUtils::getCharsHeight (getWidget(), defaultVisibleLines());
			return MAX (10, height);
		}
		return 30;
	}

	YGLABEL_WIDGET_IMPL (YMultiLineEdit)
};

YMultiLineEdit *YGWidgetFactory::createMultiLineEdit (YWidget *parent, const std::string &label)
{
	return new YGMultiLineEdit (parent, label);
}

#include "YLogView.h"

class YGLogView : public YLogView, public YGTextView
{
std::string m_text;

public:
	YGLogView (YWidget *parent, const std::string &label, int visibleLines, int maxLines)
	: YLogView (NULL, label, visibleLines, maxLines)
	, YGTextView (this, parent, label, false)
	{}

	// YLogView
	virtual void displayLogText (const std::string &text)
	{
		// libyui calls clearText before setting it: let's ignore it
		if (text.empty()) return;

		if (text.compare (0, m_text.size(), m_text) == 0) {
			if (text.size() == m_text.size()) return;

			// appending text: avoid flickering and allow user to scroll freely
			GtkAdjustment *vadj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE (getWidget()));
			bool autoScroll = gtk_adjustment_get_value(vadj) >= gtk_adjustment_get_upper(vadj) - gtk_adjustment_get_page_size(vadj);

			std::string diff (text.substr (m_text.size()));
			YGTextView::appendText (diff);
			if (autoScroll)
				YGTextView::scrollToBottom();

		}
		else {
			YGTextView::setText (text);
			YGTextView::scrollToBottom();
		}
		m_text = text;
	}

	// YGWidget
	virtual unsigned int getMinSize (YUIDimension dim)
	{
		if (dim == YD_VERT) {
			int height = YGUtils::getCharsHeight (getWidget(), visibleLines());
			return MAX (80, height);
		}
		return 50;
	}

	YGLABEL_WIDGET_IMPL (YLogView)
};

YLogView *YGWidgetFactory::createLogView (YWidget *parent, const std::string &label,
                                          int visibleLines, int maxLines)
{
	return new YGLogView (parent, label, visibleLines, maxLines);
}

#include "YRichText.h"
#include "ygtkhtmlwrap.h"

class YGRichText : public YRichText, YGScrolledWidget
{
public:
	YGRichText (YWidget *parent, const std::string &text, bool plainText)
	: YRichText (NULL, text, plainText)
	, YGScrolledWidget (this, parent, ygtk_html_wrap_get_type(), NULL)
	{
		ygtk_html_wrap_init (getWidget());
		ygtk_html_wrap_connect_link_clicked (getWidget(), link_clicked_cb, this);
		setText (text, plainText);
	}

	void setPlainText (const std::string &text)
	{
		ygtk_html_wrap_set_text (getWidget(), text.c_str(), TRUE);
	}

	void setRichText (const std::string &text)
	{
#if 0  // current done at the XHTML treatment level, we may want to enable
       // this code so that we replace the entity for all widgets
		std::string text (_text);
		std::string productName = YUI::app()->productName();
		YGUtils::replace (text, "&product;", 9, productName.c_str());
#endif
		ygtk_html_wrap_set_text (getWidget(), text.c_str(), FALSE);
	}

	void scrollToBottom()
	{
		ygtk_html_wrap_scroll (getWidget(), FALSE);
	}

	void setText (const std::string &text, bool plain_mode)
	{
		plain_mode ?  setPlainText (text) : setRichText (text);
		if (autoScrollDown())
			scrollToBottom();
	}

	// YRichText
	virtual void setValue (const std::string &text)
	{
		YRichText::setValue (text);
		setText (text, plainTextMode());
	}

    virtual void setAutoScrollDown (bool on)
    {
    	YRichText::setAutoScrollDown (on);
    	if (on) scrollToBottom();
    }

    virtual void setPlainTextMode (bool plain_mode)
    {
    	YRichText::setPlainTextMode (plain_mode);
    	if (plain_mode != plainTextMode())
	    	setText (value(), plain_mode);
    }

	// callbacks
	static void link_clicked_cb (GtkWidget *widget, const char *url, gpointer data)
	{ YGUI::ui()->sendEvent (new YMenuEvent (url)); }

	// YGWidget
	virtual unsigned int getMinSize (YUIDimension dim)
	{ return shrinkable() ? 10 : 100; }

	YGWIDGET_IMPL_COMMON (YRichText)
};


YRichText *YGWidgetFactory::createRichText (YWidget *parent, const std::string &text,
                                            bool plainTextMode)
{
	return new YGRichText (parent, text, plainTextMode);
}

