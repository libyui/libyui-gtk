#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YRichText.h"
#include "YGWidget.h"

// TODO: replace g_warning() by y2error()
// maybe it would be better to create a method that would
// print errors for both y2error() and stdout
// (at least on debug mode)

/* Rich Text parsing methods. */

struct HTMLList
{
	bool ordered;
	char enumeration;
	HTMLList (bool ordered)
	: ordered (ordered), enumeration(1) {}
};
#define LISTS_MARGIN      10
#define BLOCKQUOTE_MARGIN 20

struct GRTPTag {
	GtkTextMark *mark;
	GtkTextTag  *tag;
};
struct GRTParseState {
	GtkTextBuffer *buffer;
	GtkTextTagTable *tags;
	std::vector <GRTPTag> htags;

	// Attributes for tags that affect their children
	bool pre_mode;
	int left_margin;
	list <HTMLList> html_list;

	GRTParseState(GtkTextBuffer *buffer) :
		buffer(buffer),
		pre_mode(false),
		left_margin(0)
	{
		tags = gtk_text_buffer_get_tag_table (buffer);
	}
};

static char *elide_whitespace (const char *instr, int len);
static GdkColor link_color = { 0, 0, 0, 0xeeee };

// Tags to support: <p> and not </p>:
// either 'elide' \ns (turn off with <pre> I guess
static void
rt_start_element (GMarkupParseContext *context,
                  const gchar         *element_name,
                  const gchar        **attribute_names,
                  const gchar        **attribute_values,
                  gpointer             user_data,
                  GError             **error)
{	// Called for open tags <foo bar="baz">
	GRTParseState *state = (GRTParseState*) user_data;
	if (!g_ascii_strcasecmp (element_name, "p"))
	{
		if (state->pre_mode)
			g_warning ("Odd <p> inside <pre>");
		return;
	}
	if (!g_ascii_strcasecmp (element_name, "pre"))
		state->pre_mode = true;

	// FIXME - create marks here instead & push on-list etc.
	// FIXME - what does this comment mean?
	GRTPTag tag;

	GtkTextIter iter;
	gtk_text_buffer_get_end_iter (state->buffer, &iter);
	tag.mark = gtk_text_buffer_create_mark (state->buffer, NULL, &iter, TRUE);

	char *lower = g_ascii_strdown (element_name, -1);
	tag.tag = gtk_text_tag_table_lookup (state->tags, lower);
	// Special tags that must be inserted manually
	if (!tag.tag) {
		if (!g_ascii_strcasecmp (element_name, "font")) {
			if (!g_ascii_strcasecmp (attribute_names[0], "color"))
				tag.tag = gtk_text_buffer_create_tag (state->buffer, NULL,
				                                      "foreground", attribute_values[0],
				                                      NULL);
			else
				g_warning ("Unknown font attribute: '%s'", attribute_names[0]);
		}
		else if (!g_ascii_strcasecmp (element_name, "a")) {
			if (!g_ascii_strcasecmp (attribute_names[0], "href")) {
				tag.tag = gtk_text_buffer_create_tag (state->buffer, NULL,
				                                      "foreground-gdk", &link_color,
				                                      "underline", PANGO_UNDERLINE_SINGLE,
				                                      NULL);
				g_object_set_data (G_OBJECT (tag.tag), "link", g_strdup (attribute_values[0]));
			}
			else
				g_warning ("Unknown font attribute: '%s'", attribute_names[0]);
		}
		else if (!g_ascii_strcasecmp (element_name, "li")) {
			if (state->html_list.front().ordered) {
				gchar *str = g_strdup_printf ("%d. ", state->html_list.front().enumeration++);
				gtk_text_buffer_insert (state->buffer, &iter, str, -1);
				g_free (str);
			}
			else                             // \u2022 for smaller bullets
				gtk_text_buffer_insert (state->buffer, &iter, "\u25cf ", -1);
		}
		// Tags that affect the margin
		else if (!g_ascii_strcasecmp (element_name, "ul") ||
		         !g_ascii_strcasecmp (element_name, "ol")) {
			
			state->html_list.push_front (HTMLList (!g_ascii_strcasecmp (element_name, "ol")));
			state->left_margin += LISTS_MARGIN;
			tag.tag = gtk_text_buffer_create_tag (state->buffer, NULL,
			                 "left-margin", state->left_margin, NULL);
		}
		else if (!g_ascii_strcasecmp (element_name, "blockquote")) {
			state->left_margin += BLOCKQUOTE_MARGIN;
			tag.tag = gtk_text_buffer_create_tag (state->buffer, NULL,
			                 "left-margin", state->left_margin, NULL);
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
{	// Called for close tags </foo>
	GRTParseState *state = (GRTParseState*) user_data;
	GtkTextIter start, end;
	gtk_text_buffer_get_end_iter (state->buffer, &end);

	if (!g_ascii_strcasecmp (element_name, "p")) {
		gtk_text_buffer_insert (state->buffer, &end, "\n", -1);
		return;
	}

	if (state->htags.empty()) {
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

	if (!g_ascii_strcasecmp (element_name, "pre")) {
		state->pre_mode = false;
		char *txt = gtk_text_buffer_get_text (state->buffer,
		                                      &start, &end, TRUE);
		char endc = txt ? txt[strlen(txt) - 1] : '\0';
		if (endc != '\r' && endc != '\n')
			appendNewline = true;
	}
	else if (!g_ascii_strcasecmp (element_name, "li"))
		appendNewline = true;

	else if (!g_ascii_strcasecmp (element_name, "blockquote"))
		state->left_margin -= BLOCKQUOTE_MARGIN;
	else if (!g_ascii_strcasecmp (element_name, "ul") ||
	         !g_ascii_strcasecmp (element_name, "ol")) {
		state->left_margin -= LISTS_MARGIN;
		state->html_list.pop_front();
	}

	if (appendNewline) {
		gtk_text_buffer_insert (state->buffer, &end, "\n", -1);
		gtk_text_buffer_get_iter_at_mark (state->buffer, &start, tag.mark);
		gtk_text_buffer_get_end_iter (state->buffer, &end);
	}

	if (tag.tag)
		gtk_text_buffer_apply_tag (state->buffer, tag.tag, &start, &end);

	gtk_text_buffer_delete_mark (state->buffer, tag.mark);
}

static void
rt_text (GMarkupParseContext *context,
         const gchar         *text,
         gsize                text_len,
         gpointer             user_data,
         GError             **error)
{  // Called for character data, NB. text NOT nul-terminated
	GRTParseState *state = (GRTParseState*) user_data;
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

static char *xmlize_string (const char *instr, const char *prodname);

class YGRichText : public YRichText, public YGScrolledWidget
{
	bool m_shrinkable;
	bool m_plainText;

public:
	YGRichText(const YWidgetOpt &opt,
	           YGWidget *parent,
	           const YCPString &text)
	: YRichText (opt, text),
	  YGScrolledWidget (this, parent, true,
	  		GTK_TYPE_TEXT_VIEW, "wrap-mode", GTK_WRAP_WORD,
	  		"editable", FALSE, "cursor-visible", FALSE, NULL)
	{
		IMPL;
		m_plainText = opt.plainTextMode.value();
		m_shrinkable = opt.isShrinkable.value();

		GtkTextView *view = GTK_TEXT_VIEW (getWidget());
		gtk_text_view_set_pixels_below_lines (view, 10);  // FIXME: points ? cf. foo.

		// Init link support
		if (!ref_cursor) {
			hand_cursor    = gdk_cursor_new (GDK_HAND2);
			regular_cursor = gdk_cursor_new (GDK_XTERM);
		}
		ref_cursor++;

#if GTK_CHECK_VERSION(2,10,0)
		gtk_widget_style_get (GTK_WIDGET (view), "link_color", &link_color, NULL);
#endif

		g_signal_connect (view, "event-after",
		                  G_CALLBACK (event_after), NULL);
		g_signal_connect (view, "motion-notify-event",
		                  G_CALLBACK (motion_notify_event), NULL);
		g_signal_connect (view, "visibility-notify-event",
		                  G_CALLBACK (visibility_notify_event), NULL);

		// Init tags
		GtkTextBuffer *buffer = gtk_text_view_get_buffer (view);
		// Create a few tags like 'h3', 'b', 'i'
		gtk_text_buffer_create_tag (buffer, "h1", "weight", PANGO_WEIGHT_HEAVY,
		                            "size", 22 * PANGO_SCALE, NULL);
		gtk_text_buffer_create_tag (buffer, "h2", "weight", PANGO_WEIGHT_ULTRABOLD,
		                            "size", 18 * PANGO_SCALE, NULL);
		gtk_text_buffer_create_tag (buffer, "h3", "weight", PANGO_WEIGHT_BOLD,
		                            "size", 15 * PANGO_SCALE, NULL);
		gtk_text_buffer_create_tag (buffer, "h4", "weight", PANGO_WEIGHT_SEMIBOLD,
		                            "size", 13 * PANGO_SCALE, NULL);
		gtk_text_buffer_create_tag (buffer, "h5", "weight", PANGO_WEIGHT_SEMIBOLD,
		                            "size", 11 * PANGO_SCALE, NULL);
		gtk_text_buffer_create_tag (buffer, "b", "weight", PANGO_WEIGHT_BOLD, NULL);
		gtk_text_buffer_create_tag (buffer, "i", "style", PANGO_STYLE_ITALIC, NULL);
		gtk_text_buffer_create_tag (buffer, "u", "underline", PANGO_UNDERLINE_SINGLE, NULL);
		gtk_text_buffer_create_tag (buffer, "pre", "family", "monospace", NULL);
		setText (text);
	}

	virtual ~YGRichText()
	{
		if (--ref_cursor == 0) {
			gdk_cursor_unref (hand_cursor);
			gdk_cursor_unref (regular_cursor);
		}
	}

	// YRichText
	virtual void setText (const YCPString &text)
	{
		IMPL;
		GtkTextBuffer *buffer;
		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (getWidget()));
		GRTParseState state (buffer);

		if (m_plainText)
		{
			gtk_text_buffer_set_text (state.buffer, text->value_cstr(), -1);
			YRichText::setText (text);
			return;
		}

		GMarkupParseContext *ctx;
		ctx = g_markup_parse_context_new (&rt_parser, (GMarkupParseFlags)0, &state, NULL);

		char *xml;
		xml = xmlize_string (text->value_cstr(), YUI::ui()->productName().c_str());

		GError *error = NULL;
		if (!g_markup_parse_context_parse (ctx, xml, -1, &error))
			g_warning ("Markup parse error");
		g_free (xml);

		g_markup_parse_context_free(ctx);

		if (autoScrollDown) {
			GtkTextIter end_iter;
			gtk_text_buffer_get_end_iter (state.buffer, &end_iter);
			GtkTextMark *end_mark;
			end_mark = gtk_text_buffer_create_mark
			               (state.buffer, NULL, &end_iter, FALSE);
			gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (getWidget()),
			               end_mark, 0.0, FALSE, 0, 0);
			gtk_text_buffer_delete_mark (state.buffer, end_mark);
		}

		YRichText::setText (text);
	}

	// YWidget
	// TODO: used by a few more widgets... maybe use some macro approach for this?
	virtual long nicesize (YUIDimension dim)
	{
		IMPL;
		long size = getNiceSize (dim);
		return MAX (m_shrinkable ? 10 : 100, size);
	}
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_KEYBOARD_FOCUS

	/* Methods to add hyperlink support for GtkTextView (based from gtk-demos). */
	static GdkCursor *hand_cursor;
	static GdkCursor *regular_cursor;
	static unsigned int ref_cursor;

	// Looks at all tags covering the position of iter in the text view,
	// and returns the link the text points to, in case that text is a link.
	static char*
	get_link (GtkTextView *text_view, gint x, gint y)
	{
		GtkTextIter iter;
		gtk_text_view_get_iter_at_location (text_view, &iter, x, y);

		char *link = NULL;
		GSList *tags = gtk_text_iter_get_tags (&iter), *tagp;
		for (tagp = tags; tagp != NULL; tagp = tagp->next) {
			GtkTextTag *tag = (GtkTextTag*) tagp->data;
			link = (char*) g_object_get_data (G_OBJECT (tag), "link");
			if (link)
				break;
		}

		if (tags)
			g_slist_free (tags);
		return link;
	}

	// Links can also be activated by clicking.
	static gboolean
	event_after (GtkWidget *text_view,
	             GdkEvent  *ev)
	{
		GtkTextIter start, end;
		GtkTextBuffer *buffer;
		GdkEventButton *event;
		gint x, y;

		if (ev->type != GDK_BUTTON_RELEASE)
			return FALSE;

		event = (GdkEventButton *)ev;
		if (event->button != 1)
			return FALSE;

		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));

		// We shouldn't follow a link if the user is selecting something.
		gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
		if (gtk_text_iter_get_offset (&start) != gtk_text_iter_get_offset (&end))
			return FALSE;

		gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view),
			GTK_TEXT_WINDOW_WIDGET, (gint) event->x, (gint) event->y, &x, &y);

		char *link = get_link (GTK_TEXT_VIEW (text_view), x, y);
		if (link)  // report link
			YGUI::ui()->sendEvent (new YMenuEvent (YCPString (link)));

		return FALSE;
	}

	// Change the cursor to the "hands" cursor typically used by web browsers,
	// if there is a link in the given position.
	static void
	set_cursor_if_appropriate (GtkTextView    *text_view,
	                           gint            x,
	                           gint            y)
	{
		static bool hovering_over_link = false;
		bool hovering = get_link (text_view, x, y);

		if (hovering != hovering_over_link) {
			hovering_over_link = hovering;

			if (hovering_over_link)
				gdk_window_set_cursor (gtk_text_view_get_window
						(text_view, GTK_TEXT_WINDOW_TEXT), hand_cursor);
			else
				gdk_window_set_cursor (gtk_text_view_get_window
						(text_view, GTK_TEXT_WINDOW_TEXT), regular_cursor);
			}
	}

	// Update the cursor image if the pointer moved.
	static gboolean
	motion_notify_event (GtkWidget      *text_view,
											GdkEventMotion *event)
	{
		gint x, y;
		gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view),
		                                       GTK_TEXT_WINDOW_WIDGET,
		                                       (gint) event->x, (gint) event->y,
		                                       &x, &y);
		set_cursor_if_appropriate (GTK_TEXT_VIEW (text_view), x, y);
	
		gdk_window_get_pointer (text_view->window, NULL, NULL, NULL);
		return FALSE;
	}

	// Also update the cursor image if the window becomes visible
	// (e.g. when a window covering it got iconified).
	static gboolean
	visibility_notify_event (GtkWidget          *text_view,
	                         GdkEventVisibility *event)
	{
		gint wx, wy, bx, by;

		gdk_window_get_pointer (text_view->window, &wx, &wy, NULL);
		gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view), 
		                                       GTK_TEXT_WINDOW_WIDGET,
		                                       wx, wy, &bx, &by);
		set_cursor_if_appropriate (GTK_TEXT_VIEW (text_view), bx, by);
	
		return FALSE;
	}
};

unsigned int YGRichText::ref_cursor = 0;
GdkCursor *YGRichText::hand_cursor, *YGRichText::regular_cursor;

YWidget *
YGUI::createRichText (YWidget *parent, YWidgetOpt &opt,
                      const YCPString &text )
{
	IMPL;
	return new YGRichText( opt, YGWidget::get (parent), text );
}

/* String preparation methods. */

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
		else if (instr[i] == '&' && !g_ascii_strncasecmp (instr + i, PROD_ENTITY,
		                                               sizeof (PROD_ENTITY) - 1)) {
			g_string_append (outp, prodname);
			i += sizeof (PROD_ENTITY) - 1;
		}
		else if (inTag) {
				// This is bothersome
				for (; instr[i] != '>'; i++) {
					g_string_append_c (outp, instr[i]);
					if (instr[i] == '=' && instr[i+1] != '"') {
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
