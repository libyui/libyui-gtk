//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

/* YGtkRichText widget */
// check the header file for information about this widget

#include "ygtkrichtext.h"
#include <gtk/gtkversion.h>
#include <string.h>

#define LISTS_MARGIN      10
#define BLOCKQUOTE_MARGIN 20
#define PARAGRAPH_SPACING 12

// Sucky - but we mix C & C++ so ...
/* Convert html to xhtml (or at least try) */
gchar *ygutils_convert_to_xhmlt_and_subst (const char *instr, const char *product,
                                           gboolean cut_breaklines);

G_DEFINE_TYPE (YGtkRichText, ygtk_richtext, GTK_TYPE_TEXT_VIEW)

static GdkCursor *hand_cursor, *regular_cursor;
static guint ref_cursor = 0;
static guint link_pressed_signal;
static GdkColor link_color = { 0, 0, 0, 0xeeee };

// utilities
// Looks at all tags covering the position of iter in the text view,
// and returns the link the text points to, in case that text is a link.
static const char* get_link (GtkTextView *text_view, gint x, gint y)
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

// callbacks
// Links can also be activated by clicking.
static gboolean event_after (GtkWidget *text_view, GdkEvent *ev)
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

	const char *link = get_link (GTK_TEXT_VIEW (text_view), x, y);
	if (link)  // report link
		g_signal_emit (YGTK_RICHTEXT (text_view), link_pressed_signal, 0, link);
	return FALSE;
}

void ygtk_richtext_init (YGtkRichText *rtext)
{
	rtext->prodname = NULL;

	GtkTextView *tview = GTK_TEXT_VIEW (rtext);
	gtk_text_view_set_wrap_mode (tview, GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_editable (tview, FALSE);
	gtk_text_view_set_cursor_visible (tview, FALSE);
	gtk_text_view_set_pixels_below_lines (tview, 6);

	// Init link support
	if (!ref_cursor) {
		GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (rtext));
		hand_cursor    = gdk_cursor_new_for_display (display, GDK_HAND2);
		regular_cursor = gdk_cursor_new_for_display (display, GDK_XTERM);
	}
	ref_cursor++;

#if GTK_CHECK_VERSION(2,10,0)
	gtk_widget_style_get (GTK_WIDGET (rtext), "link_color", &link_color, NULL);
#endif

	g_signal_connect (tview, "event-after",
	                  G_CALLBACK (event_after), NULL);

	// Init tags
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (tview);
	// Create a few tags like 'h3', 'b', 'i'
	PangoFontDescription *font_desc = GTK_WIDGET (rtext)->style->font_desc;
	int size = pango_font_description_get_size (font_desc);
	if (pango_font_description_get_size_is_absolute (font_desc))
		size /= PANGO_SCALE;

	gtk_text_buffer_create_tag (buffer, "h1", "weight", PANGO_WEIGHT_HEAVY,
	                            "size", (int)(size * PANGO_SCALE_XX_LARGE),
	                            "pixels-below-lines", PARAGRAPH_SPACING, NULL);
	gtk_text_buffer_create_tag (buffer, "h2", "weight", PANGO_WEIGHT_ULTRABOLD,
	                            "size", (int)(size * PANGO_SCALE_X_LARGE),
	                            "pixels-below-lines", PARAGRAPH_SPACING, NULL);
	gtk_text_buffer_create_tag (buffer, "h3", "weight", PANGO_WEIGHT_BOLD,
	                            "size", (int)(size * PANGO_SCALE_LARGE),
	                            "pixels-below-lines", PARAGRAPH_SPACING, NULL);
	gtk_text_buffer_create_tag (buffer, "h4", "weight", PANGO_WEIGHT_SEMIBOLD,
	                            "size", (int)(size * PANGO_SCALE_LARGE),
	                            "pixels-below-lines", PARAGRAPH_SPACING, NULL);
	gtk_text_buffer_create_tag (buffer, "h5",
	                            "size", (int)(size * PANGO_SCALE_LARGE),
	                            "pixels-below-lines", PARAGRAPH_SPACING, NULL);
	gtk_text_buffer_create_tag (buffer, "big",
	                            "size", (int)(size * PANGO_SCALE_LARGE), NULL);
	gtk_text_buffer_create_tag (buffer, "small",
	                            "size", (int)(size * PANGO_SCALE_SMALL), NULL);
	gtk_text_buffer_create_tag (buffer, "tt",
	                            "family", "monospace", NULL);
	gtk_text_buffer_create_tag (buffer, "b", "weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_buffer_create_tag (buffer, "i", "style", PANGO_STYLE_ITALIC, NULL);
	gtk_text_buffer_create_tag (buffer, "u", "underline", PANGO_UNDERLINE_SINGLE, NULL);
	gtk_text_buffer_create_tag (buffer, "pre", "family", "monospace", NULL);
	gtk_text_buffer_create_tag (buffer, "p", "pixels-below-lines", PARAGRAPH_SPACING,
	                            NULL);
	gtk_text_buffer_create_tag (buffer, "center", "justification", GTK_JUSTIFY_CENTER,
	                            NULL);
	gtk_text_buffer_create_tag (buffer, "keyword", "background", "yellow", NULL);
}

static void ygtk_richtext_destroy (GtkObject *object)
{
	// FIXME: if we have multiple richtexts, one will unref too many
	if (ref_cursor > 0 && (--ref_cursor == 0)) {
		gdk_cursor_unref (hand_cursor);
		gdk_cursor_unref (regular_cursor);
	}

	YGtkRichText *rtext = YGTK_RICHTEXT (object);
	if (rtext->prodname)
		g_free (rtext->prodname);
	rtext->prodname = NULL;

	GTK_OBJECT_CLASS (ygtk_richtext_parent_class)->destroy (object);
}

// Change the cursor to the "hands" cursor typically used by web browsers,
// if there is a link in the given position.
static void set_cursor_if_appropriate (GtkTextView *text_view, gint x, gint y)
	{
	static gboolean hovering_over_link = FALSE;
	gboolean hovering = get_link (text_view, x, y) != NULL;

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
static gboolean ygtk_richtext_motion_notify_event (GtkWidget *text_view,
                                                   GdkEventMotion *event)
{
	gint x, y;
	gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view),
	                                       GTK_TEXT_WINDOW_WIDGET,
	                                       (gint) event->x, (gint) event->y,
	                                       &x, &y);
	set_cursor_if_appropriate (GTK_TEXT_VIEW (text_view), x, y);

	gdk_window_get_pointer (text_view->window, NULL, NULL, NULL);
	return TRUE;
}

// Also update the cursor image if the window becomes visible
// (e.g. when a window covering it got iconified).
static gboolean ygtk_richtext_visibility_notify_event (GtkWidget *text_view,
                                                       GdkEventVisibility *event)
{
	gint wx, wy, bx, by;

	gdk_window_get_pointer (text_view->window, &wx, &wy, NULL);
	gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view), 
	                                       GTK_TEXT_WINDOW_WIDGET,
	                                       wx, wy, &bx, &by);
	set_cursor_if_appropriate (GTK_TEXT_VIEW (text_view), bx, by);
	return TRUE;
}

/* Rich Text parsing methods. */

typedef struct _HTMLList
{
	gboolean ordered;
	char enumeration;
} HTMLList;

static void HTMLList_init (HTMLList *list, gboolean ordered)
{
	list->ordered = ordered;
	list->enumeration = 1;
}

typedef struct GRTPTag {
	GtkTextMark *mark;
	GtkTextTag  *tag;
} GRTPTag;
typedef struct GRTParseState {
	GtkTextBuffer *buffer;
	GtkTextTagTable *tags;
	GList *htags;  // of GRTPTag

	// Attributes for tags that affect their children
	gboolean pre_mode;
	gboolean default_color;
	int left_margin;
	GList *html_list;  // of HTMLList
} GRTParseState;

static void GRTParseState_init (GRTParseState *state, GtkTextBuffer *buffer)
{
	state->buffer = buffer;
	state->pre_mode = FALSE;
	state->default_color = TRUE;
	state->left_margin = 0;
	state->tags = gtk_text_buffer_get_tag_table (buffer);
	state->html_list = NULL;
	state->htags = NULL;
}

static void free_list (GList *list)
{
	GList *i;
	for (i = g_list_first (list); i; i = i->next)
		g_free (i->data);
	g_list_free (list);
}

static void GRTParseState_free (GRTParseState *state)
{
	// NOTE: some elements might not have been freed because of bad html
	free_list (state->html_list);
	free_list (state->htags);
}

static char *elide_whitespace (const char *instr, int len);

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
	if (!g_ascii_strcasecmp (element_name, "body"))
		return;

	GRTParseState *state = (GRTParseState*) user_data;
	GRTPTag *tag = g_malloc (sizeof (GRTPTag));
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter (state->buffer, &iter);
	tag->mark = gtk_text_buffer_create_mark (state->buffer, NULL, &iter, TRUE);

	if (!g_ascii_strcasecmp (element_name, "pre"))
		state->pre_mode = TRUE;

	char *lower = g_ascii_strdown (element_name, -1);
	tag->tag = gtk_text_tag_table_lookup (state->tags, lower);
	// Special tags that must be inserted manually
	if (!tag->tag) {
		if (!g_ascii_strcasecmp (element_name, "font")) {
			if (!g_ascii_strcasecmp (attribute_names[0], "color")) {
				tag->tag = gtk_text_buffer_create_tag (state->buffer, NULL,
				                                       "foreground", attribute_values[0],
				                                       NULL);
				state->default_color = FALSE;
			}
			else
				g_warning ("Unknown font attribute: '%s'", attribute_names[0]);
		}
		else if (!g_ascii_strcasecmp (element_name, "a")) {
			if (!g_ascii_strcasecmp (attribute_names[0], "href")) {
				tag->tag = gtk_text_buffer_create_tag (state->buffer, NULL,
				                                       "underline", PANGO_UNDERLINE_SINGLE,
				                                       NULL);
				if (state->default_color)
					g_object_set (tag->tag, "foreground-gdk", &link_color, NULL);
				g_object_set_data (G_OBJECT (tag->tag), "link", g_strdup (attribute_values[0]));
			}
			else
				g_warning ("Unknown font attribute: '%s'", attribute_names[0]);
		}
		else if (!g_ascii_strcasecmp (element_name, "li")) {
			HTMLList *front_list;

			if (state->html_list &&
			    (front_list = g_list_first (state->html_list)->data) &&
			    (front_list->ordered)) {;
				gchar *str = g_strdup_printf ("%d. ", front_list->enumeration++);
				gtk_text_buffer_insert (state->buffer, &iter, str, -1);
				g_free (str);
			}
			else {                           // \\u25cf for bigger bullets
				gtk_text_buffer_insert (state->buffer, &iter, "\u2022 ", -1);
			}
		}
		// Tags that affect the margin
		else if (!g_ascii_strcasecmp (element_name, "ul") ||
		         !g_ascii_strcasecmp (element_name, "ol")) {

			HTMLList *list = g_malloc (sizeof (HTMLList));
			HTMLList_init (list, !g_ascii_strcasecmp (element_name, "ol"));
			state->html_list = g_list_append (state->html_list, list);
			state->left_margin += LISTS_MARGIN;
			tag->tag = gtk_text_buffer_create_tag (state->buffer, NULL,
			                   "left-margin", state->left_margin, NULL);
		}
		else if (!g_ascii_strcasecmp (element_name, "blockquote")) {
			state->left_margin += BLOCKQUOTE_MARGIN;
			tag->tag = gtk_text_buffer_create_tag (state->buffer, NULL,
			                   "left-margin", state->left_margin, NULL);
		}

		else if (!g_ascii_strcasecmp (element_name, "img")) {
			if (!g_ascii_strcasecmp (attribute_names[0], "src")) {
				GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (attribute_values[0], NULL);
				gtk_text_buffer_insert_pixbuf (state->buffer, &iter, pixbuf);
				g_object_unref (G_OBJECT (pixbuf));
			}
			else
				g_warning ("Unknown img attribute: '%s'", attribute_names[0]);
		}

		// for tags like <br/>, GMarkup will pass them through the end
		// tag callback too, so we'll deal with them there
		else if (!g_ascii_strcasecmp (element_name, "br"))
			;

		else
			g_warning ("Unknown tag '%s'", element_name);
	}

	// Check if this tag is of paragraph type
	if (tag->tag) {
		gint spacing;
		g_object_get (G_OBJECT (tag->tag), "pixels-below-lines", &spacing, NULL);
		if (spacing > 0) {  // is paragraph
			// make sure this opens a new paragraph
			if (!gtk_text_iter_starts_line (&iter))
				gtk_text_buffer_insert (state->buffer, &iter, "\n", -1);
		}
	}

	g_free (lower);
	state->htags = g_list_append (state->htags, tag);
}

static void
rt_end_element (GMarkupParseContext *context,
                const gchar         *element_name,
                gpointer             user_data,
                GError             **error)
{	// Called for close tags </foo>
	if (!g_ascii_strcasecmp (element_name, "body"))
		return;

	GRTParseState *state = (GRTParseState*) user_data;
	GtkTextIter start, end;
	gtk_text_buffer_get_end_iter (state->buffer, &end);

	if (g_list_length (state->htags) == 0) {
		g_warning ("Urgh - empty tag queue closing '%s'", element_name);
		return;
	}

	g_return_if_fail (state->htags != NULL);
	GRTPTag *tag = g_list_last (state->htags)->data;
	state->htags = g_list_remove (state->htags, tag);

	gtk_text_buffer_get_iter_at_mark (state->buffer, &start, tag->mark);
	gtk_text_buffer_get_end_iter (state->buffer, &end);

	gboolean appendNewline = FALSE;

	// Check if this tag is of paragraph type
	if (tag->tag) {
		gint spacing;
		g_object_get (G_OBJECT (tag->tag), "pixels-below-lines", &spacing, NULL);
		if (spacing > 0) {  // is paragraph
			appendNewline = TRUE;
			// if last paragraph didn't set "pixe-below-lines", set "pixels-above-lines"
			GtkTextIter last_p = start;
			gint offset = gtk_text_iter_get_offset (&last_p) - 1;
			if (offset >= 0) {
				gtk_text_buffer_get_iter_at_offset (state->buffer, &last_p, offset);
				if (!gtk_text_iter_has_tag (&last_p,
				         gtk_text_tag_table_lookup (state->tags, "p"))) {
					GtkTextTag *tag;
					tag = gtk_text_buffer_create_tag (state->buffer, NULL,
					          "pixels-above-lines", PARAGRAPH_SPACING, NULL);
					gtk_text_buffer_apply_tag (state->buffer, tag, &start, &end);
				}
			}
		}
	}

	if (!g_ascii_strcasecmp (element_name, "li"))
		appendNewline = TRUE;

	else if (!g_ascii_strcasecmp (element_name, "blockquote"))
		state->left_margin -= BLOCKQUOTE_MARGIN;
	else if (!g_ascii_strcasecmp (element_name, "ul") ||
	         !g_ascii_strcasecmp (element_name, "ol")) {
		state->left_margin -= LISTS_MARGIN;

		HTMLList *last_list = g_list_last (state->html_list)->data;
		state->html_list = g_list_remove (state->html_list, last_list);
		g_free (last_list);
	}
	else if (!g_ascii_strcasecmp (element_name, "font"))
		state->default_color = TRUE;

	else if (!g_ascii_strcasecmp (element_name, "pre")) {
		state->pre_mode = FALSE;
		char *txt = gtk_text_buffer_get_text (state->buffer,
		                                      &start, &end, TRUE);
		char endc = txt ? txt[strlen(txt) - 1] : '\0';
		appendNewline = endc != '\r' && endc != '\n';
		g_free (txt);
	}

	else if (!g_ascii_strcasecmp (element_name, "br"))
		appendNewline = TRUE;

	if (appendNewline) {
		gtk_text_buffer_insert (state->buffer, &end, "\n", -1);
		gtk_text_buffer_get_iter_at_mark (state->buffer, &start, tag->mark);
		gtk_text_buffer_get_end_iter (state->buffer, &end);
	}

	if (tag->tag)
		gtk_text_buffer_apply_tag (state->buffer, tag->tag, &start, &end);

	gtk_text_buffer_delete_mark (state->buffer, tag->mark);
	g_free (tag);
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
		                                  text, text_len, NULL, NULL);
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

GtkWidget *ygtk_richtext_new()
{
	return g_object_new (YGTK_TYPE_RICHTEXT, NULL);
}

/* String preparation methods. */

#define IS_WHITE(c) (g_ascii_isspace (c) || (c) == '\t')

static char *elide_whitespace (const char *instr, int len)
{
	GString *dest = g_string_new ("");
	if (len < 0)
		len = strlen (instr);
	gboolean last_white = FALSE;
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
		gboolean cur_white = IS_WHITE(c);
		if (!(cur_white && last_white))
			g_string_append_c (dest, c);
		last_white = cur_white;
	}

	return g_string_free (dest, FALSE);
}

void ygtk_richttext_set_prodname (YGtkRichText *rtext, const char *prodname)
{
	rtext->prodname = g_strdup (prodname);
}

void ygtk_richtext_set_text (YGtkRichText* rtext, const gchar* text,
                             gboolean plain_text, gboolean honor_br_char)
{
	GtkTextBuffer *buffer;
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (rtext));

	if (plain_text) {
		gtk_text_buffer_set_text (buffer, text, -1);
		return;
	}

	// remove any possible existing text
	gtk_text_buffer_set_text (buffer, "", 0);

	GRTParseState state;
	GRTParseState_init (&state, buffer);

	GMarkupParseContext *ctx;
	ctx = g_markup_parse_context_new (&rt_parser, (GMarkupParseFlags)0, &state, NULL);

	char *xml = ygutils_convert_to_xhmlt_and_subst (text, rtext->prodname,
	                                                !honor_br_char);
	GError *error = NULL;
	if (!g_markup_parse_context_parse (ctx, xml, -1, &error))
		g_warning ("Markup parse error '%s'", error ? error->message : "Unknown");
	g_free (xml);

	g_markup_parse_context_free (ctx);
	GRTParseState_free (&state);
}

void ygtk_richtext_set_background (YGtkRichText *rtext, const char *image)
{
	g_return_if_fail (GTK_WIDGET_REALIZED (GTK_WIDGET (rtext)));

	GdkWindow *window = gtk_text_view_get_window
		(GTK_TEXT_VIEW (rtext), GTK_TEXT_WINDOW_TEXT);

	if (!image) {
		gdk_window_clear (window);
		return;
	}

	GError *error = 0;
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (image, &error);
	if (!pixbuf) {
		g_warning ("ygtkrichtext: could not open background image: '%s'"
		           " - %s", image, error->message);
		return;
	}

	GdkPixmap *pixmap;
	gdk_pixbuf_render_pixmap_and_mask_for_colormap (pixbuf,
		gdk_drawable_get_colormap (GDK_DRAWABLE (window)), &pixmap, NULL, 0);
	g_object_unref (G_OBJECT (pixbuf));

	gdk_window_set_back_pixmap (window, pixmap, FALSE);
}

/* gtk_text_iter_forward_search() is case-sensitive so we roll our own.
   The idea is to keep use get_text and strstr there, but to be more
   efficient we check per line. */
static gboolean ygtk_richtext_forward_search (const GtkTextIter *begin,
	const GtkTextIter *end, const gchar *_key, GtkTextIter *match_start,
	GtkTextIter *match_end)
{
	if (*_key == 0)
		return FALSE;

	/* gtk_text_iter_get_char() returns a gunichar (ucs4 coding), so we
	   convert the given string (which is utf-8, like anyhting in gtk+) */
	gunichar *key = g_utf8_to_ucs4 (_key, -1, NULL, NULL, NULL);
	if (!key)  // conversion error -- should not happen
		return FALSE;

	// convert key to lower case, to avoid work later
	gunichar *k;
	for (k = key; *k; k++)
		*k = g_unichar_tolower (*k);

	GtkTextIter iter = *begin, iiter;
	while (!gtk_text_iter_is_end (&iter) && gtk_text_iter_compare (&iter, end) <= 0) {
		iiter = iter;
		for (k = key; *k == g_unichar_tolower (gtk_text_iter_get_char (&iiter)) && (*k);
		     k++, gtk_text_iter_forward_char (&iiter))
			;
		if (!*k) {
			*match_start = iter;
			*match_end = iiter;
			return TRUE;
		}
		gtk_text_iter_forward_char (&iter);
	}
	return FALSE;
}

gboolean ygtk_richtext_mark_text (YGtkRichText *rtext, const gchar *text)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (rtext));
	GtkTextIter iter, end, match_start, match_end;

	gtk_text_buffer_get_bounds (buffer, &iter, &end);
	gtk_text_buffer_remove_tag_by_name (buffer, "keyword", &iter, &end);

	gtk_text_buffer_select_range (buffer, &iter, &iter);  // unselect text
	if (!text || *text == '\0')
		return TRUE;

	gboolean found = FALSE;
	while (ygtk_richtext_forward_search (&iter, &end, text,
	                                     &match_start, &match_end)) {
		found = TRUE;
		gtk_text_buffer_apply_tag_by_name (buffer, "keyword", &match_start, &match_end);
		iter = match_end;
		gtk_text_iter_forward_char (&iter);
	}
	return found;
}

gboolean ygtk_richtext_forward_mark (YGtkRichText *rtext, const gchar *text)
{
	GtkTextIter start_iter, end_iter;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (rtext));
	gtk_text_buffer_get_iter_at_mark (buffer, &start_iter,
	                                  gtk_text_buffer_get_selection_bound (buffer));
	gtk_text_buffer_get_end_iter (buffer, &end_iter);

	gboolean found;
	found = ygtk_richtext_forward_search (&start_iter, &end_iter, text,
	                                      &start_iter, &end_iter);
	if (!found) {
		gtk_text_buffer_get_start_iter (buffer, &start_iter);
		found = ygtk_richtext_forward_search (&start_iter, &end_iter, text,
		                                      &start_iter, &end_iter);
	}

	if (found) {
		gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (rtext), &start_iter, 0.10,
		                              FALSE, 0, 0);
		gtk_text_buffer_select_range (buffer, &start_iter, &end_iter);
		return TRUE;
	}
	return FALSE;
}

void ygtk_richtext_class_init (YGtkRichTextClass *klass)
{
	GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);
	gtkwidget_class->motion_notify_event = ygtk_richtext_motion_notify_event;
	gtkwidget_class->visibility_notify_event = ygtk_richtext_visibility_notify_event;

	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
	gtkobject_class->destroy = ygtk_richtext_destroy;

	link_pressed_signal = g_signal_new ("link-pressed",
		G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (YGtkRichTextClass, link_pressed), NULL, NULL,
		g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
}
