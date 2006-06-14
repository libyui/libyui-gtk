/* Yast GTK ... */

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include <string>
#include "YEvent.h"
#include "YMultiLineEdit.h"
#include "YGWidget.h"

class YGMultiLineEdit : public YMultiLineEdit, public YGScrolledWidget
{
int maxChars;

public:
	YGMultiLineEdit (const YWidgetOpt &opt,
		               YGWidget *parent,
		               const YCPString &label,
		               const YCPString &text)
		: YMultiLineEdit (opt, label)
		, YGScrolledWidget (this, parent, label, YD_VERT, true,
		                    GTK_TYPE_TEXT_VIEW, NULL)
	{
		IMPL
		maxChars = -1;
		setText (text);

		g_signal_connect (G_OBJECT (getBuffer()), "changed",
		                  G_CALLBACK (text_changed_cb), this);
	}

	virtual ~YGMultiLineEdit() {}

	GtkTextBuffer* getBuffer()
	{ return gtk_text_view_get_buffer (GTK_TEXT_VIEW (getWidget())); }

	int getCharsNb()
	{
		IMPL
		return gtk_text_buffer_get_char_count (getBuffer());
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

	// YMultiLineEdit
	virtual void setText (const YCPString &text)
	{
		IMPL
		gtk_text_buffer_set_text (getBuffer(), text->value_cstr(), -1);
	}

	virtual YCPString text()
	{
		IMPL
		GtkTextIter start_it, end_it;
		gtk_text_buffer_get_bounds (getBuffer(), &start_it, &end_it);

		gchar* text = gtk_text_buffer_get_text (getBuffer(), &start_it, &end_it, FALSE);
		YCPString str (text);
		g_free (text);
		return str;
	}

	virtual void setInputMaxLength (const YCPInteger &numberOfChars)
	{
		IMPL
		maxChars = numberOfChars->asInteger()->value();
		if (maxChars != -1 && getCharsNb() > maxChars)
			truncateText (maxChars);
	}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_KEYBOARD_FOCUS
	// YGLabelWidget
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN(YMultiLineEdit)

	// Event callbacks
	static void text_changed_cb (GtkTextBuffer *buffer, YGMultiLineEdit *pThis)
	{
		if (pThis->maxChars != -1 && pThis->getCharsNb() > pThis->maxChars) {
			pThis->truncateText (pThis->maxChars);
			gdk_beep();
		}
		if (pThis->getNotify())
			YGUI::ui()->sendEvent (new YWidgetEvent (pThis, YEvent::ValueChanged));
	}
};

YWidget *
YGUI::createMultiLineEdit (YWidget *parent, YWidgetOpt & opt,
                           const YCPString & label, const YCPString & text)
{
	return new YGMultiLineEdit (opt, YGWidget::get (parent), label, text);
}
