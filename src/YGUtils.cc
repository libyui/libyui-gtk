/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <string.h>
#include "YGUI.h"
#include "YGUtils.h"

string YGUtils::mapKBAccel (const string &src)
{
	// we won't use use replace since we also want to escape _ to __
	std::string::size_type length = src.length(), i;
	string str;
	str.reserve (length);
	for (i = 0; i < length; i++) {
		if (src[i] == '_')
			str += "__";
		else if (src[i] == '&')
			str += '_';
		else
			str += src[i];
	}
	return str;
}

void YGUtils::setFilter (GtkEntry *entry, const string &validChars)
{
	struct inner {
		static void insert_text_cb (GtkEditable *editable, const gchar *new_text,
		                            gint new_text_length, gint *pos)
		{
			const gchar *valid_chars = (gchar *) g_object_get_data (G_OBJECT (editable),
			                                                        "valid-chars");
			if (valid_chars) {
				const gchar *i, *j;
				for (i = new_text; *i; i++) {
					for (j = valid_chars; *j; j++) {
						if (*i == *j)
							break;
					}
					if (!*j) {
						// not valid text
						g_signal_stop_emission_by_name (editable, "insert_text");
						gdk_beep();
						return;
					}
				}
			}
		}
	};

	if (g_object_get_data (G_OBJECT (entry), "insert-text-set"))
		g_object_disconnect (G_OBJECT (entry), "insert-text",
		                     G_CALLBACK (inner::insert_text_cb), NULL);

	if (!validChars.empty()) {
		gchar *chars = g_strdup (validChars.c_str());
		g_object_set_data_full (G_OBJECT (entry), "valid-chars", chars, g_free);
		g_signal_connect (G_OBJECT (entry), "insert-text",
		                  G_CALLBACK (inner::insert_text_cb), NULL);
		g_object_set_data (G_OBJECT (entry), "insert-text-set", GINT_TO_POINTER (1));
	}
	else
		g_object_set_data (G_OBJECT (entry), "insert-text-set", GINT_TO_POINTER (0));
}

void ygutils_setFilter (GtkEntry *entry, const char *validChars)
{ YGUtils::setFilter (entry, validChars); }

void YGUtils::replace (string &str, const char *mouth, int mouth_len, const char *food)
{
	if (mouth_len < 0)
		mouth_len = strlen (mouth);
	std::string::size_type i = 0;
	while ((i = str.find (mouth, i)) != string::npos) {
		str.erase (i, mouth_len);
		str.insert (i, food);
	}
}

std::string YGUtils::truncate (const std::string &str, int length, int pos)
{
	std::string ret (str);
	const char *pstr = ret.c_str(); char *pi;
	int size = g_utf8_strlen (pstr, -1);
	if (size > length) {
		if (pos > 0) {
			pi = g_utf8_offset_to_pointer (pstr, length-3);
			ret.erase (pi-pstr);
			ret.append ("...");
		}
		else if (pos < 0) {
			pi = g_utf8_offset_to_pointer (pstr, size-(length-3));
			ret.erase (0, pi-pstr);
			ret.insert (0, "...");
		}
		else /* (pos == 0) */ {
			pi = g_utf8_offset_to_pointer (pstr, size/2);
			int delta = size - (length-3);
			gchar *pn = pi, *pp = pi;
			for (int i = 0;;) {
				if (i++ == delta) break;
				pn = g_utf8_next_char (pn);
				if (i++ == delta) break;
				pp = g_utf8_prev_char (pp);
			}
			g_assert (pp != NULL && pn != NULL);

			ret.erase (pp-pstr, pn-pp);
			ret.insert (pp-pstr, "...");
		}
	}
	return ret;
}

void YGUtils::scrollWidget (GtkAdjustment *vadj, bool top)
{
	if (top)
		gtk_adjustment_set_value (vadj, vadj->lower);	
	else  /* bottom */
		gtk_adjustment_set_value (vadj, vadj->upper - vadj->page_size);
}

void YGUtils::scrollWidget (GtkTextView *view, bool top)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (view);
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter (buffer, &iter);
	GtkTextMark *mark = gtk_text_buffer_get_mark (buffer, "scroll");
	if (mark)
		gtk_text_buffer_move_mark (buffer, mark, &iter);
	else
		mark = gtk_text_buffer_create_mark (buffer, "scroll", &iter, FALSE);
	gtk_text_view_scroll_mark_onscreen (view, mark);
	// GTK 2.12.9 suffers from glitches otherwise:
	gtk_widget_queue_draw (GTK_WIDGET (view));
}

void ygutils_scrollAdj (GtkAdjustment *vadj, gboolean top)
{ YGUtils::scrollWidget (vadj, top); }
void ygutils_scrollView (GtkTextView *view, gboolean top)
{ YGUtils::scrollWidget (view, top); }

void YGUtils::escapeMarkup (std::string &str)
{
	bool modify = false;
	std::string::size_type i;
	for (i = 0; i < str.length() && !modify; i++) {
		switch (str[i]) {
			case '<':
			case '>':
			case '&':
				modify = true;
				break;
			default:
				break;
		}
	}
	if (modify) {
		std::string ori (str);
		str.clear();
		str.reserve (ori.length()+50);
		for (i = 0; i < ori.length(); i++) {
			switch (ori[i]) {
				case '<':
					str += "&lt;";
					break;
				case '>':
					str += "&gt;";
					break;
				case '&':
					str += "&amp;";
					break;
				default:
					str += ori[i];
					break;
			}
		}
	}
}

inline void skipSpace(const char *instr, int &i)
{
	while (g_ascii_isspace (instr[i])) i++;
}

typedef struct {
	GString     *tag;
	int          tag_len : 31;
	unsigned int early_closer : 1;
} TagEntry;

static TagEntry *
tag_entry_new (GString *tag, int tag_len)
{
	static const char *early_closers[] = { "p", "li" };
	TagEntry *entry = g_new (TagEntry, 1);
	entry->tag = tag;
	entry->tag_len = tag_len;
	entry->early_closer = FALSE;

	for (unsigned int i = 0; i < G_N_ELEMENTS (early_closers); i++)
		if (!g_ascii_strncasecmp (tag->str, early_closers[i], tag_len))
			entry->early_closer = TRUE;
	return entry;
}

static void
tag_entry_free (TagEntry *entry)
{
	if (entry && entry->tag)
		g_string_free (entry->tag, TRUE);
	g_free (entry);
}

static void
emit_unclosed_tags_for (GString *outp, GQueue *tag_queue, const char *tag_str, int tag_len)
{
	gboolean matched = FALSE;

	// top-level tag ...
	if (g_queue_is_empty (tag_queue))
		return;

	do {
		TagEntry *last_entry = (TagEntry *)g_queue_pop_tail (tag_queue);
		if (!last_entry)
			break;

		if (last_entry->tag_len != tag_len ||
		    g_ascii_strncasecmp (last_entry->tag->str, tag_str, tag_len)) {
			/* different tag - emit a close ... */
			g_string_append (outp, "</");
			g_string_append_len (outp, last_entry->tag->str, last_entry->tag_len);
			g_string_append_c (outp, '>');
		} else
			matched = TRUE;

		tag_entry_free (last_entry);
	} while (!matched);
}

static gboolean
check_early_close (GString *outp, GQueue *tag_queue, TagEntry *entry)
{
	TagEntry *last_tag;

        // Early closers:
	if (!entry->early_closer)
		return FALSE;

	last_tag = (TagEntry *) g_queue_peek_tail (tag_queue);
	if (!last_tag || !last_tag->early_closer)
		return FALSE;

	if (entry->tag_len != last_tag->tag_len ||
	    g_ascii_strncasecmp (last_tag->tag->str, entry->tag->str, entry->tag_len))
		return FALSE;

	// Emit close & leave last tag on the stack

	g_string_append (outp, "</");
	g_string_append_len (outp, entry->tag->str, entry->tag_len);
	g_string_append_c (outp, '>');

	return TRUE;
}

// We have to:
//   + rewrite <br> and <hr> tags
//   + deal with <a attrib=noquotes>
gchar *ygutils_convert_to_xhtml (const char *instr)
{
	GString *outp = g_string_new ("");
	GQueue *tag_queue = g_queue_new();
	int i = 0;

	gboolean was_space = TRUE, pre_mode = FALSE;
	skipSpace (instr, i);

	// we must add an outer tag to make GMarkup happy
	g_string_append (outp, "<body>");

	for (; instr[i] != '\0'; i++)
	{
		// Tag foo
		if (instr[i] == '<') {
			// ignore comments
			if (strncmp (&instr[i], "<!--", 4) == 0) {
				for (i += 3; instr[i] != '\0'; i++)
					if (strncmp (&instr[i], "-->", 3) == 0) {
						i += 2;
						break;
					}
				continue;
			}

			gint j;
			gboolean is_close = FALSE;
			gboolean in_tag;
			int tag_len;
			GString *tag = g_string_sized_new (20);

			i++;
			skipSpace (instr, i);

			if (instr[i] == '/') {
			    i++;
			    is_close = TRUE;
			}

			skipSpace (instr, i);

			// find the tag name piece
			in_tag = TRUE;
			tag_len = 0;
			for (; instr[i] != '>' && instr[i]; i++) {
				if (in_tag) {
					if (!g_ascii_isalnum(instr[i]))
						in_tag = FALSE;
					else
						tag_len++;
				}
				g_string_append_c (tag, instr[i]);
			}

			// Unmatched tags
			if (!is_close && tag_len == 2 &&
			      (!g_ascii_strncasecmp (tag->str, "hr", 2) ||
			      !g_ascii_strncasecmp (tag->str, "br", 2)) &&
			      tag->str[tag->len - 1] != '/')
				g_string_append_c (tag, '/');

			if (!g_ascii_strncasecmp (tag->str, "pre", 3))
				pre_mode = !is_close;

			// Add quoting for un-quoted attributes
			for (int j = 0; j < (signed) tag->len; j++) {
				if (tag->str[j] == '=') {
					gboolean unquote = tag->str[j+1] != '"';
					if (unquote)
						g_string_insert_c (tag, j+1, '"');
					else
						j++;
					for (j++; tag->str[j]; j++) {
						if (unquote && g_ascii_isspace (tag->str[j]))
							break;
						else if (!unquote && tag->str[j] == '"')
							break;
					}
					if (unquote)
						g_string_insert_c (tag, j, '"');
				}
			}

			// Is it an open or close ?
			j = tag->len - 1;

			while (j > 0 && g_ascii_isspace (tag->str[j])) j--;

			gboolean is_open_close = (tag->str[j] == '/');
			if (is_open_close)
				; // ignore it
			else if (is_close)
				emit_unclosed_tags_for (outp, tag_queue, tag->str, tag_len);
			else {
				TagEntry *entry = tag_entry_new (tag, tag_len);

				entry->tag = tag;
				entry->tag_len = tag_len;

				if (!check_early_close (outp, tag_queue, entry))
					g_queue_push_tail (tag_queue, entry);
				else {
					entry->tag = NULL;
					tag_entry_free (entry);
				}
			}

			g_string_append_c (outp, '<');
			if (is_close)
			    g_string_append_c (outp, '/');
			g_string_append_len (outp, tag->str, tag->len);
			g_string_append_c (outp, '>');

			if (is_close || is_open_close)
				g_string_free (tag, TRUE);
		}

		// non-break space entity
		else if (instr[i] == '&' &&
			 !g_ascii_strncasecmp (instr + i, "&nbsp;",
					       sizeof ("&nbsp;") - 1)) {
			// Replace this by a white-space
			g_string_append (outp, " ");
			i += sizeof ("&nbsp;") - 2;
			was_space = FALSE;
		}

		else {  // Normal text
			if (!pre_mode && g_ascii_isspace (instr[i])) {
				if (!was_space)
					g_string_append_c (outp, ' ');
				was_space = TRUE;
			}
			else {
				was_space = FALSE;
				g_string_append_c (outp, instr[i]);
			}
		}
	}

	emit_unclosed_tags_for (outp, tag_queue, "", 0);
	g_queue_free (tag_queue);
	g_string_append (outp, "</body>");

	gchar *ret = g_string_free (outp, FALSE);
	return ret;
}

int YGUtils::getCharsWidth (GtkWidget *widget, int chars_nb)
{
	PangoContext *context = gtk_widget_get_pango_context (widget);
	PangoFontMetrics *metrics = pango_context_get_metrics (context,
		widget->style->font_desc, NULL);

	int width = pango_font_metrics_get_approximate_char_width (metrics);
	pango_font_metrics_unref (metrics);

	return PANGO_PIXELS (width) * chars_nb;
}

int YGUtils::getCharsHeight (GtkWidget *widget, int chars_nb)
{
	PangoContext *context = gtk_widget_get_pango_context (widget);
	PangoFontMetrics *metrics = pango_context_get_metrics (context,
		widget->style->font_desc, NULL);

	int height = pango_font_metrics_get_ascent (metrics) +
	             pango_font_metrics_get_descent (metrics);
	pango_font_metrics_unref (metrics);

	return PANGO_PIXELS (height) * chars_nb;
}

void YGUtils::setWidgetFont (GtkWidget *widget, PangoWeight weight, double scale)
{
	PangoFontDescription *font_desc = widget->style->font_desc;
	int size = pango_font_description_get_size (font_desc);
	PangoFontDescription* font = pango_font_description_new();
	pango_font_description_set_weight (font, weight);
	pango_font_description_set_size   (font, (int)(size * scale));

	gtk_widget_modify_font (widget, font);
	pango_font_description_free (font);
}

int ygutils_getCharsWidth (GtkWidget *widget, int chars_nb)
{ return YGUtils::getCharsWidth (widget, chars_nb); }
int ygutils_getCharsHeight (GtkWidget *widget, int chars_nb)
{ return YGUtils::getCharsHeight (widget, chars_nb); }
void ygutils_setWidgetFont (GtkWidget *widget, PangoWeight weight, double scale)
{ YGUtils::setWidgetFont (widget, weight, scale); }

GdkPixbuf *YGUtils::loadPixbuf (const string &filename)
{
	GdkPixbuf *pixbuf = NULL;
	if (!filename.empty()) {
		GError *error = 0;
		pixbuf = gdk_pixbuf_new_from_file (filename.c_str(), &error);
		if (!pixbuf)
			yuiWarning() << "Could not load icon: " << filename << "\n"
			                "Reason: " << error->message << "\n";
	}
	return pixbuf;
}

static std::string cutUnderline (const std::string &str)
{
	std::string ret (str);
	std::string::size_type i = 0;
	if ((i = ret.find ('_', i)) != std::string::npos)
		ret.erase (i, 1);
	return ret;
}

static void stripStart (std::string &str, char ch)
{
	while (!str.empty() && str[0] == ch)
		str.erase (0, 1);
}
static void stripEnd (std::string &str, char ch)
{
	while (!str.empty() && str[str.size()-1] == ch)
		str.erase (str.size()-1, 1);
}

void YGUtils::setStockIcon (const std::string &label, GtkWidget *button)
{
	static bool firstTime = true; static std::map <std::string, std::string> stockMap;
	if (firstTime) {
		GSList *list = gtk_stock_list_ids();
		for (GSList *i = list; i; i = i->next) {
			gchar *id = (gchar *) i->data;
			GtkStockItem item;
			if (gtk_stock_lookup (id, &item))
				stockMap[cutUnderline (item.label)] = id;
			// some may not have a stock item because they can't be set on a label
			// e.g.: gtk-directory, gtk-missing-image, gtk-dnd
			g_free (id);
		}
		g_slist_free (list);
		firstTime = false;
	}

	std::string id = cutUnderline (label);
	stripStart (id, ' ');
	stripEnd (id, ' ');
	stripEnd (id, '.');

	std::map <std::string, std::string>::const_iterator it;
	it = stockMap.find (id);
	if (it != stockMap.end()) {
		const std::string &stock_id = it->second;
		GdkPixbuf *pixbuf;
		pixbuf = gtk_widget_render_icon (button, stock_id.c_str(), GTK_ICON_SIZE_BUTTON, NULL);
		if (pixbuf) {
			GtkWidget *image = gtk_image_new_from_pixbuf (pixbuf);
			gtk_button_set_image (GTK_BUTTON (button), image);
			g_object_unref (G_OBJECT (pixbuf));
		}
		else
			g_warning ("yast2-gtk: setStockIcon(): id '%s' exists, but has no pixbuf",
			           stock_id.c_str());
	}
	else
		gtk_button_set_image (GTK_BUTTON (button), NULL);
}

void ygutils_setStockIcon (const char *label, GtkWidget *button)
{ YGUtils::setStockIcon (label, button); }

