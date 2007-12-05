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
	YGMultiLineEdit (YWidget *parent, const string &label)
	: YMultiLineEdit (parent, label)
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
	: YLogView (parent, label, visibleLines, maxLines)
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

class YGPlainText : public YRichText, public YGTextView
{
bool m_autoScrollDown;

public:
	YGPlainText(YWidget *parent, const string &text)
	: YRichText (parent, text)
	, YGTextView (this, parent, string(), false)
	{
		if (shrinkable())
			setMinSizeInChars (0, 0);
	}

	// YRichText
	virtual void setValue (const string &text)
	{
		IMPL
		YGTextView::setText (text);
		if (autoScrollDown())
			scrollToBottom();
		YRichText::setValue (text);
	}

	YGWIDGET_IMPL_COMMON
};

#include "ygtkhtmlwrap.h"

class YGRichText : public YRichText, public YGScrolledWidget
{
public:
	YGRichText (YWidget *parent, const string &text)
	: YRichText (parent, text)
	, YGScrolledWidget (this, parent, true, ygtk_html_wrap_get_type(), NULL)
	{
		IMPL
		if (!shrinkable())
			setMinSizeInChars (20, 8);

		ygtk_html_wrap_init (getWidget());
		ygtk_html_wrap_connect_link_clicked (getWidget(), G_CALLBACK (link_clicked_cb), this);
		setValue (text);
	}

	// YRichText
	virtual void setValue (const string &_text)
	{
		IMPL
		string text (_text);
		YGUtils::replace (text, "&product;", 9, YUI::ui()->productName().c_str());

		ygtk_html_wrap_set_text (getWidget(), text.c_str());
		if (autoScrollDown())
			ygtk_html_wrap_scroll (getWidget(), FALSE);
		YRichText::setValue (_text);
	}

	static void link_clicked_cb (GtkWidget *widget, const char *url, YGRichText *pThis)
	{
		YGUI::ui()->sendEvent (new YMenuEvent (YCPString (url)));
	}

	YGWIDGET_IMPL_COMMON
};

YRichText *YGWidgetFactory::createRichText (YWidget *parent, const string &text,
                                            bool plainTextMode)
{
	if (plainTextMode)
		return new YGPlainText (parent, text);
	else
		return new YGRichText (parent, text);
}

#if 0
/* Wrote this code, in order to allow the user to print a package license.
   But they are written in HTML, so that was too much for me. Anyway, might
   be needed in the future -- it prints text okay. Maybe we could adapt it to
   YGtkRichText, so that it just iterates through its tags (its a GtkTextView
   afterall), rather than doing html parsing. ugh. Maybe we could somehow re-use
   GtkTextView GtkLayouts...*/

typedef struct {
	gchar *title, *text;
	PangoLayout *title_layout, *text_layout;
	gint cur_line;
} PrintData;


static void free_print_data_cb (gpointer _data, GClosure *closure)
{
	PrintData *data = _data;
fprintf (stderr, "freeing print data '%s'\n", data->title);
	g_free (data->title);
	g_free (data->text);
	if (data->title_layout)
		g_object_unref (G_OBJECT (data->title_layout));
	if (data->text_layout)
		g_object_unref (G_OBJECT (data->text_layout));
	g_free (data);
}

static void print_begin_cb (GtkPrintOperation *print, GtkPrintContext *context, PrintData *data)
{
	int width = gtk_print_context_get_width (context);
	int height = gtk_print_context_get_height (context);

	PangoFontDescription *desc;
	desc = pango_font_description_from_string ("Sans 12");

	data->text_layout = gtk_print_context_create_pango_layout (context);
	pango_layout_set_font_description (data->text_layout, desc);
	pango_layout_set_width (data->text_layout, width * PANGO_SCALE);
	pango_layout_set_text (data->text_layout, data->text, -1);

	data->title_layout = gtk_print_context_create_pango_layout (context);
	pango_layout_set_font_description (data->title_layout, desc);
	pango_layout_set_width (data->title_layout, width * PANGO_SCALE);
	pango_layout_set_text (data->title_layout, data->title, -1);

	pango_font_description_free (desc);

	int title_height, text_height;
	pango_layout_get_pixel_size (data->title_layout, NULL, &title_height);
	title_height += 10;
	pango_layout_get_pixel_size (data->text_layout, NULL, &text_height);

	int pages_nb, page_height = height - title_height;
	pages_nb = text_height / page_height;
	if (text_height % page_height != 0)
		pages_nb++;
	data->cur_line = 0;
fprintf (stderr, "print begin: will use %d pages\n", pages_nb);
	gtk_print_operation_set_n_pages (print, pages_nb);
}

static void print_draw_page_cb (GtkPrintOperation *print, GtkPrintContext *context,
                                gint page_nb, PrintData *data)
{
	int title_height;
	pango_layout_get_pixel_size (data->title_layout, NULL, &title_height);
	title_height += 10;

	cairo_t *cr = gtk_print_context_get_cairo_context (context);
	cairo_set_source_rgb (cr, 0, 0, 0);

	cairo_move_to (cr, 6, 3);
	pango_cairo_show_layout (cr, data->title_layout);

	cairo_move_to (cr, 0, title_height-4);
	cairo_line_to (cr, gtk_print_context_get_width (context), title_height-4);
	cairo_stroke (cr);

	int y = title_height, page_height = 0;
	for (;; data->cur_line++) {
fprintf (stderr, "printing line: %d\n", data->cur_line);
		PangoLayoutLine *line = pango_layout_get_line_readonly (data->text_layout,
		                                                        data->cur_line);
		if (!line)
{
fprintf (stderr, "no more lines\n");
			break;
}
		PangoRectangle rect;
		pango_layout_line_get_pixel_extents (line, NULL, &rect);
fprintf (stderr, "paragraph rect: %d x %d , %d x %d\n", rect.x, rect.y, rect.width, rect.height);
		page_height += rect.height;
		if (page_height >= gtk_print_context_get_height (context))
{
fprintf (stderr, "end of page: %f x %d\n", gtk_print_context_get_height (context), page_height);
			break;
}

		cairo_move_to (cr, rect.x, y + rect.height);
		pango_cairo_show_layout_line (cr, line);
		y += rect.height;
	}
}

void ygtk_html_print_text (GtkWindow *parent_window, const gchar *title, const gchar *text)
{
	PrintData *data = g_new (PrintData, 1);
	data->title = g_strdup (title);
	data->text = g_strdup (text);
	data->title_layout = data->text_layout = NULL;

	GtkPrintOperation *print = gtk_print_operation_new();

	static GtkPrintSettings *print_settings = NULL;
	if (print_settings)
		gtk_print_operation_set_print_settings (print, print_settings);

	g_signal_connect (G_OBJECT (print), "begin-print", G_CALLBACK (print_begin_cb), data);
  	g_signal_connect_data (G_OBJECT (print), "draw-page", G_CALLBACK (print_draw_page_cb),
  	                       data, free_print_data_cb, 0);

	GError *error;
	GtkPrintOperationResult res;
	res = gtk_print_operation_run (print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
	                               parent_window, &error);
	if (res == GTK_PRINT_OPERATION_RESULT_ERROR) {
		GtkWidget *dialog;
		dialog = gtk_message_dialog_new (parent_window, GTK_DIALOG_DESTROY_WITH_PARENT,
		                                 GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
		                                 "<b>Print error</b>\n%s", error->message);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_error_free (error);
	}
	if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
		if (print_settings)
			g_object_unref (print_settings);
		print_settings = g_object_ref (gtk_print_operation_get_print_settings (print));
	}
}
#endif

