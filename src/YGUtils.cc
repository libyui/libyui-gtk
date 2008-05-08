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

std::string YGUtils::truncate (const std::string &str, unsigned int length, int pos)
{
	std::string ret (str);
	if (ret.size() > length) {
		if (pos > 0) {
			ret.erase (length-3);
			ret.append ("...");
		}
		else if (pos < 0) {
			ret.erase (0, ret.size()-(length-3));
			ret.insert (0, "...");
		}
		else /* (pos == 0) */ {
			int delta = ret.size()-(length-3);
			ret.erase ((ret.size()/2)-(delta/2), delta);
			ret.insert (ret.size()/2, "...");
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
// gtk_widget_redraw .... // FIXME: package selector change bug
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
gchar *ygutils_convert_to_xhmlt_and_subst (const char *instr)
{
	GString *outp = g_string_new ("");
	GQueue *tag_queue = g_queue_new();
	int i = 0;

	skipSpace (instr, i);

	// We must add an outer tag to make GMarkup happy
	gboolean addOuterTag = TRUE;
//	gboolean addOuterTag = FALSE;
//	if ((addOuterTag = (instr[i] != '<')))
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
			if ( !is_close && tag_len == 2 &&
			     (!g_ascii_strncasecmp (tag->str, "hr", 2) ||
			      !g_ascii_strncasecmp (tag->str, "br", 2)) &&
			     tag->str[tag->len - 1] != '/')
				g_string_append_c (tag, '/');

			// Add quoting for un-quoted attributes
			for (int j = 0; j < (signed) tag->len; j++) {
				if (tag->str[j] == '=') {
					if (tag->str[j+1] != '"') {
						g_string_insert_c (tag, j+1, '"');
						for (j++; !g_ascii_isspace (tag->str[j]) && tag->str[j]; j++) ;
						g_string_insert_c (tag, j, '"');
					}
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
		}

#if 0
		// removing new-lines chars sure isn't a xhtml conversion
		// but it's still valid xhtml and GtkTextView appreciates it
		else if (/*cut_breaklines &&*/ instr[i] == '\n') {
			// In HTML a breakline should be treated as a white space when
			// not in the start of a paragraph.
			if (i > 0 && instr[i-1] != '>' && !g_ascii_isspace (instr[i-1]))
				g_string_append_c (outp, ' ');
		}
#endif

		else // Normal text
			g_string_append_c (outp, instr[i]);
	}

	emit_unclosed_tags_for (outp, tag_queue, "", 0);
	g_queue_free (tag_queue);

	if (addOuterTag)
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

std::list <string> YGUtils::splitString (const string &str, char separator)
{
	std::list <string> parts;
	std::string::size_type i, j;
	// ignore first character, if separator
	i = j = (str[0] == separator) ? 1 : 0;
	for (; i < str.length(); i++)
		if (str[i] == separator) {
			parts.push_back (str.substr (j, i - j));
			j = ++i;
		}
	parts.push_back (str.substr (j));
	return parts;
}

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

#define ENGLISH_STOCK_ITEMS

#ifdef ENGLISH_STOCK_ITEMS
struct StockMap { const char *ycp_label, *gtk_stock; };
static const StockMap stockMap[] = {
	// keep them sorted!
	{"Abort",     GTK_STOCK_CANCEL      },
	{"Accept",    GTK_STOCK_APPLY       },
	{"Add",       GTK_STOCK_ADD         },
	{"Apply",     GTK_STOCK_APPLY       },
	{"Back",      GTK_STOCK_GO_BACK     },
	{"Cancel",    GTK_STOCK_CANCEL      },
	{"Close",     GTK_STOCK_CLOSE       },
	{"Configure", GTK_STOCK_PREFERENCES },
	{"Continue",  GTK_STOCK_OK          },
	{"Delete",    GTK_STOCK_DELETE      },
	{"Down",      GTK_STOCK_GO_DOWN     },
	{"Edit",      GTK_STOCK_EDIT        },
	{"Finish",    GTK_STOCK_APPLY       },
	{"Launch",    GTK_STOCK_EXECUTE     },
	{"Next",      GTK_STOCK_GO_FORWARD  },
	{"No",        GTK_STOCK_NO          },
	{"OK",        GTK_STOCK_OK          },
	{"Quit",      GTK_STOCK_QUIT        },
	{"Reset",     GTK_STOCK_REVERT_TO_SAVED },
	{"Reset all", GTK_STOCK_REVERT_TO_SAVED },
	{"Retry",     GTK_STOCK_REFRESH     },
	{"Search",    GTK_STOCK_FIND        },
	{"Up",        GTK_STOCK_GO_UP       },
	{"Yes",       GTK_STOCK_YES         },
};
#define STOCKMAP_SIZE (sizeof (stockMap)/sizeof(StockMap))
static int strcmp_cb (const void *a, const void *b)
{ return strcmp ((char *) a, ((StockMap *) b)->ycp_label); }

void YGUtils::setStockIcon (GtkWidget *button, std::string ycp_str)
{
	// is English the current locale?
	static bool firstTime = true, isEnglish;
	if (firstTime) {
		char *lang = getenv ("LANG");
		isEnglish = !lang || (!*lang) || !strcmp (lang, "C") ||
		            (lang[0] == 'e' && lang[1] == 'n') ||
		            !strcmp (lang, "POSIX");
		firstTime = false;
	}
	if (!isEnglish)
		return;

	std::string::size_type i = 0;
	while ((i = ycp_str.find ('_', i)) != string::npos)
		ycp_str.erase (i, 1);

	bool failed = true;
	void *ptr;
	ptr = bsearch (ycp_str.c_str(), stockMap, STOCKMAP_SIZE,
	               sizeof(stockMap[0]), strcmp_cb);
	if (ptr) {
		const char *stock = ((StockMap *) ptr)->gtk_stock;
		GdkPixbuf *pixbuf;
		pixbuf = gtk_widget_render_icon (button, stock,
		                                 GTK_ICON_SIZE_BUTTON, NULL);
		if (pixbuf) {
			GtkWidget *image = gtk_image_new_from_pixbuf (pixbuf);
			gtk_button_set_image (GTK_BUTTON (button), image);
			g_object_unref (G_OBJECT (pixbuf));
			failed = false;
		}
	}
	if (failed)
		gtk_button_set_image (GTK_BUTTON (button), NULL);
}

void ygutils_setStockIcon (GtkWidget *button, const char *ycp_str)
{ YGUtils::setStockIcon (button, ycp_str); }

#else
void ygutils_setStockIcon (GtkWidget *button, const char *ycp_str) {}
#endif

