#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YRichText.h"
#include <queue>
#include "YGScrolledWidget.h"

class YGRichText : public YRichText, public YGScrolledWidget
{
	GtkWidget *m_label;
	GtkWidget *m_entry;
	bool m_shrinkable;
	bool m_plainText;
public:
    YGRichText( const YWidgetOpt &opt,
				YGWidget         *parent,
				const YCPString &text );
	virtual ~YGRichText() {}

	// YRichText
    virtual void setText( const YCPString & text );

	// YWidget
	virtual long nicesize( YUIDimension dim )
	{
		IMPL;
		long size = getNiceSize (dim);
		return MAX (m_shrinkable ? 10 : 100, size);
	}
    YGWIDGET_IMPL_SET_ENABLING
    YGWIDGET_IMPL_SET_SIZE
    virtual bool setKeyboardFocus() IMPL_RET(false);
    virtual void startMultipleChanges() IMPL;
    virtual void doneMultipleChanges() IMPL;
    virtual void saveUserInput( YMacroRecorder *macroRecorder ) IMPL;
};


YGRichText::YGRichText( const YWidgetOpt &opt,
						YGWidget         *parent,
						const YCPString &text ) :
	YRichText (opt, text),
	YGScrolledWidget (this, parent, true,
	                  GTK_TYPE_TEXT_VIEW, NULL)
{
	// FIXME: get alignment right here [ GtkAlignment ? ]
	GtkWidget *align_left = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (align_left);
	gtk_box_pack_start (GTK_BOX (getWidget()), align_left, FALSE, TRUE, 0);

	IMPL;
	m_plainText = opt.plainTextMode.value();

	m_shrinkable = opt.isShrinkable.value();

	GtkTextView *tview = GTK_TEXT_VIEW (getWidget());
	// Set defaults
	gtk_text_view_set_wrap_mode (tview, GTK_WRAP_WORD);
	gtk_text_view_set_editable (tview, FALSE);
	// FIXME: points ? cf. foo.
	gtk_text_view_set_pixels_below_lines (tview, 10);

	GtkTextBuffer *buffer = gtk_text_view_get_buffer (tview);

	// Create tags:
	// 'h3', 'b', 'i'
	gtk_text_buffer_create_tag (buffer, "h3",
								"weight", PANGO_WEIGHT_BOLD,
								"size", 15 * PANGO_SCALE,
								NULL);
	gtk_text_buffer_create_tag (buffer, "i",
								"style", PANGO_STYLE_ITALIC, NULL);
	gtk_text_buffer_create_tag (buffer, "b",
								"weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_buffer_create_tag (buffer, "pre",
								"family", "monospace", NULL);

	// FIXME: hyperlink goodness ...
	setText (text);
}

struct GRTPTag {
	GtkTextMark *mark;
	GtkTextTag  *tag;
};
struct GRTParseState {
	GtkTextBuffer *buffer;
	GtkTextTagTable *tags;
	bool pre_mode;
	std::vector<GRTPTag> htags;

	GRTParseState(GtkTextBuffer *_buffer) :
		buffer(_buffer),
		pre_mode(false)
	{
		tags = gtk_text_buffer_get_tag_table (buffer);
	}
};

/*
 * Tags to support: <p> and not </p>:
 * either 'elide' \ns (turn off with <pre> I gues
 */

static void
rt_start_element (GMarkupParseContext *context,
				  const gchar         *element_name,
				  const gchar        **attribute_names,
				  const gchar        **attribute_values,
				  gpointer             user_data,
				  GError             **error)
{	/* Called for open tags <foo bar="baz"> */
	GRTParseState *state = (GRTParseState *)user_data;

	if (!g_ascii_strcasecmp (element_name, "p"))
	{
		if (state->pre_mode)
			g_warning ("Odd <p> inside <pre>");
		return;
	}
	if (!g_ascii_strcasecmp (element_name, "pre"))
		state->pre_mode = true;

	// FIXME - create marks here instead & push on-list etc.
	GRTPTag tag;

	GtkTextIter iter;
	gtk_text_buffer_get_end_iter (state->buffer, &iter);
	tag.mark = gtk_text_buffer_create_mark (state->buffer, NULL, &iter, TRUE);

	char *lower = g_ascii_strdown (element_name, -1);
	tag.tag = gtk_text_tag_table_lookup (state->tags, lower);
	if (!tag.tag)
	{
		if (!g_ascii_strcasecmp (element_name, "font"))
		{
			if (!g_ascii_strcasecmp (attribute_names[0], "color"))
				tag.tag = gtk_text_buffer_create_tag (state->buffer, NULL,
													  "foreground",
													  attribute_values[0],
													  NULL);
			else
				g_warning ("Unknown font attribute");
		}
		else
			g_warning ("Unknown tag '%s'", lower);
	}
	g_free (lower);

	state->htags.push_back(tag);
}

static void
rt_end_element (GMarkupParseContext *context,
				const gchar         *element_name,
				gpointer             user_data,
				GError             **error)
{	/* Called for close tags </foo> */
	GRTParseState *state = (GRTParseState *)user_data;

	GtkTextIter start, end;
	gtk_text_buffer_get_end_iter (state->buffer, &end);

	if (!g_ascii_strcasecmp (element_name, "p"))
	{
		gtk_text_buffer_insert (state->buffer, &end, "\n", strlen("\n"));
		return;
	}

	if (state->htags.empty())
	{
		g_warning ("Urgh - empty tag queue closing '%s'", element_name);
		return;
	}

	GRTPTag tag;
	tag = state->htags.back ();
	state->htags.pop_back ();

	gtk_text_buffer_get_iter_at_mark (state->buffer, &start, tag.mark);
	gtk_text_buffer_get_end_iter (state->buffer, &end);

	bool appendNewline = false;
	if (g_ascii_tolower (element_name[0]) == 'h' &&
		g_ascii_isdigit (element_name[1]) &&
		element_name[2] == '\0') // heading - implies newline:
		appendNewline = true;

	if (!g_ascii_strcasecmp (element_name, "pre"))
	{
		state->pre_mode = false;
		char *txt = gtk_text_buffer_get_text (state->buffer,
											  &start, &end, TRUE);
		char endc = txt ? txt[strlen(txt) - 1] : '\0';
		if (endc != '\r' && endc != '\n')
			appendNewline = true;
	}

	if (appendNewline) {
		gtk_text_buffer_insert (state->buffer, &end, "\n", strlen("\n"));
		gtk_text_buffer_get_iter_at_mark (state->buffer, &start, tag.mark);
		gtk_text_buffer_get_end_iter (state->buffer, &end);
	}

	if (tag.tag)
		gtk_text_buffer_apply_tag (state->buffer, tag.tag, &start, &end);

	g_warning ("Delete mark %p", tag.mark);
	gtk_text_buffer_delete_mark (state->buffer, tag.mark);
}

#define IS_WHITE(c) (g_ascii_isspace (c) || (c) == '\t')

static char *
elide_whitespace (const char *instr, int len)
{
	GString *dest = g_string_new ("");
	if (len < 0)
		len = strlen (instr);
	bool last_white = false;
	int i;
// FIXME: whitespace elision needs to happen across tags [urk]
// FIXME: perhaps post-process non-pre sections when they are complete ?
	for (i = 0; i < len ; i++)
	{
		if (instr[i] == '\r' && instr[i] == '\n')
			continue;
		char c = instr[i];
		if (c == '\t')
			c = ' ';
		bool cur_white = IS_WHITE(c);
		if (!(cur_white && last_white))
			g_string_append_c (dest, c);
		last_white = cur_white;
	}

	return g_string_free (dest, FALSE);
}

static void
rt_text (GMarkupParseContext *context,
		 const gchar         *text,
		 gsize                text_len,  
		 gpointer             user_data,
		 GError             **error)
{  /* Called for character data, NB. text NOT nul-terminated */
	GRTParseState *state = (GRTParseState *)user_data;
	GtkTextIter start, end;
	gtk_text_buffer_get_end_iter (state->buffer, &start);
	if (state->pre_mode)
		gtk_text_buffer_insert_with_tags (state->buffer, &start,
										  text, text_len,
										  NULL, NULL);
	else {
		char *real = elide_whitespace (text, text_len);
		gtk_text_buffer_insert_with_tags (state->buffer, &start,
										  real, -1, NULL, NULL);
		g_free (real);
	}
	gtk_text_buffer_get_end_iter (state->buffer, &end);
}

static void
rt_passthrough (GMarkupParseContext *context,
				const gchar         *passthrough_text,
				gsize                text_len,  
				gpointer             user_data,
				GError             **error)
{
	// ignore comments etc.
}

static void
rt_error (GMarkupParseContext *context,
		  GError              *error,
		  gpointer             user_data)
{
}

static GMarkupParser rt_parser = {
		rt_start_element,
		rt_end_element,
		rt_text,
		rt_passthrough,
		rt_error
};

#define PROD_ENTITY "&product;"

// We have to manually substitute the product entity.
// Also we have to deal with <a attrib=noquotes>
static char *
xmlize_string (const char *instr, const char *prodname)
{
	GString *outp = g_string_new ("");
	int i = 0;
	bool inTag = false;
	for (i = 0; instr[i] != '\0'; i++)
	{
		if (instr[i] == '<')
			inTag = true;
		if (instr[i] == '&' &&
			!g_ascii_strncasecmp (instr + i, PROD_ENTITY,
								  sizeof (PROD_ENTITY) - 1))
		{
			g_string_append (outp, prodname);
			i += sizeof (PROD_ENTITY) - 2;
		}
		else if (inTag) {
				// This is bothersome
				for (; instr[i] != '>'; i++) {
					g_string_append_c (outp, instr[i]);
					if (instr[i] == '=' && instr[i+1] != '"')
					{
						g_string_append_c (outp, '"');
						for (i++; !g_ascii_isspace (instr[i]) && instr[i] != '>'; i++)
							g_string_append_c (outp, instr[i]);
						g_string_append_c (outp, '"');
						g_string_append_c (outp, instr[i]);	
				}
				}
				inTag = false;
		}
		g_string_append_c (outp, instr[i]);
	}
	return g_string_free (outp, FALSE);
}

void
YGRichText::setText( const YCPString & text )
{
	GRTParseState state(gtk_text_view_get_buffer (GTK_TEXT_VIEW (getWidget())));

	if (m_plainText)
	{
		gtk_text_buffer_set_text (state.buffer, text->value_cstr(), -1);
		return;
	}

	GMarkupParseContext *ctx;
	
	ctx = g_markup_parse_context_new (&rt_parser, (GMarkupParseFlags)0, &state, NULL);

	char *xml;
	xml = xmlize_string (text->value_cstr(), YUI::ui()->productName().c_str());
//	fprintf (stderr, "Insert text '%s', after '%s'\n",
//			 text->value_cstr(), xml);

	GError *error = NULL;
	if (!g_markup_parse_context_parse (ctx, xml, -1, &error))
			g_warning ("Markup parse error");
	g_free (xml);

	g_markup_parse_context_free(ctx);

	// FIXME: scroll to 0,0 (?)
	// if 'autoScrollDown' -> scroll down ...
	// FIXME: if PlainText: search & replace &product; with productName ...
}

YWidget *
YGUI::createRichText( YWidget *parent, YWidgetOpt & opt,
					  const YCPString & text )
{
	IMPL;
	return new YGRichText( opt, YGWidget::get (parent), text );
}
