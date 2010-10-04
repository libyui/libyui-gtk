/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/*
  Textdomain "gtk"
 */

#define YUILogComponent "gtk"
#include "config.h"
#include <string.h>
#include "YGUtils.h"
#include "YGUI.h"
#include "YGi18n.h"

static inline void skipSpace (const char *instr, int *i)
{ while (g_ascii_isspace (instr[*i])) (*i)++; }

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

	unsigned int i;
	for (i = 0; i < G_N_ELEMENTS (early_closers); i++)
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

/* Some entities are translated by the xhtml parser, but not all... */
typedef struct EntityMap {
	const gchar *html, *text;
} EntityMap;

static const EntityMap entities[] = {
	{ "nbsp", " " },
	{ "product", 0 },  // dynamic
};

static const EntityMap *lookup_entity (const char *html)
{
	unsigned int i;
	for (i = 0; i < sizeof (entities) / sizeof (EntityMap); i++)
		if (!g_ascii_strncasecmp (html+1, entities[i].html, strlen (entities[i].html)))
			return entities+i;
	return NULL;
}

// We have to:
//   + rewrite <br> and <hr> tags
//   + deal with <a attrib=noquotes>
gchar *ygutils_convert_to_xhtml (const char *instr)
{
	GString *outp = g_string_new ("");
	GQueue *tag_queue = g_queue_new();
	int i = 0;

	gboolean allow_space = FALSE, pre_mode = FALSE;
	skipSpace (instr, &i);

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
			skipSpace (instr, &i);

			if (instr[i] == '/') {
			    i++;
			    is_close = TRUE;
			}

			skipSpace (instr, &i);

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
			unsigned int k;
			for (k = 0; k < tag->len; k++) {
				if (tag->str[k] == '=') {
					gboolean unquote = tag->str[k+1] != '"';
					if (unquote)
						g_string_insert_c (tag, k+1, '"');
					else
						k++;
					for (k++; tag->str[k]; k++) {
						if (unquote && g_ascii_isspace (tag->str[k]))
							break;
						else if (!unquote && tag->str[k] == '"')
							break;
					}
					if (unquote)
						g_string_insert_c (tag, k, '"');
				}
				else
					tag->str[k] = g_ascii_tolower (tag->str[k]);
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

			allow_space = is_close;  // don't allow space after opening a tag
		}

		else if (instr[i] == '&') {  // Entity
			const EntityMap *entity = lookup_entity (instr+i);
			if (entity) {
				if (!strcmp (entity->html, "product"))
					g_string_append (outp, YUI::app()->productName().c_str());
				else
					g_string_append (outp, entity->text);
				i += strlen (entity->html);
				if (instr[i+1] == ';') i++;
			}
			else {
				int j;
				// check it is a valid entity - not a floating '&' in a <pre> tag eg.
				for (j = i + 1; instr[j] != '\0'; j++) {
					if (!g_ascii_isalnum (instr[j]) && instr[j] != '#')
						break;
				}
				if (instr[j] != ';') // entity terminator
					g_string_append (outp, "&amp;");
				else
					g_string_append_c (outp, instr[i]);
			}
			allow_space = TRUE;
		}

		else {  // Normal text
			if (!pre_mode && g_ascii_isspace (instr[i])) {
				if (allow_space)
					g_string_append_c (outp, ' ');
				allow_space = FALSE;  // one space is enough
			}
			else {
				allow_space = TRUE;
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

std::string YGUtils::mapKBAccel (const std::string &src)
{
	// conversion pairs: ('_', '__') ('&&', '&') ('&', '_')
	std::string::size_type length = src.length(), i;
	string str;
	str.reserve (length);
	for (i = 0; i < length; i++) {
		if (src[i] == '_')
			str += "__";
		else if (src[i] == '&') {
			if (i+1 < length && src[i+1] == '&') {
				str += '&';  // escaping
				i++;
			}
			else
				str += '_';
		}
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
						gtk_widget_error_bell (GTK_WIDGET (editable));
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

static gboolean scroll_down_cb (void *pData)
{
	GtkAdjustment *vadj = (GtkAdjustment *) pData;
	gtk_adjustment_set_value (vadj, vadj->upper - vadj->page_size);
	return FALSE;
}

void YGUtils::scrollWidget (GtkAdjustment *vadj, bool top)
{
	if (top)
		gtk_adjustment_set_value (vadj, vadj->lower);
	else
		// since we usually want to call this together with a text change, we
		// must wait till that gets in effect
		g_idle_add_full (G_PRIORITY_LOW, scroll_down_cb, vadj, NULL);
}

void ygutils_scrollAdj (GtkAdjustment *vadj, gboolean top)
{ YGUtils::scrollWidget (vadj, top); }

std::string YGUtils::escapeMarkup (const std::string &ori)
{
	std::string::size_type length = ori.length(), i;
	std::string ret;
	ret.reserve (length * 1.5);
	for (i = 0; i < length; i++)
		switch (ori[i]) {
			case '<':
				ret += "&lt;";
				break;
			case '>':
				ret += "&gt;";
				break;
			case '&':
				ret += "&amp;";
				break;
			default:
				ret += ori[i];
				break;
		}
	return ret;
}

bool YGUtils::endsWith (const std::string &str, const std::string &key)
{
	if (str.size() < key.size())
		return false;
	return str.compare (str.size()-key.size(), key.size(), key) == 0;
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

void YGUtils::setWidgetFont (GtkWidget *widget, PangoStyle style, PangoWeight weight,
                             double scale)
{
	PangoFontDescription *font_desc = widget->style->font_desc;
	int size = pango_font_description_get_size (font_desc);
	PangoFontDescription* font = pango_font_description_new();
	pango_font_description_set_weight (font, weight);
	pango_font_description_set_size   (font, (int)(size * scale));
	pango_font_description_set_style (font, style);
	gtk_widget_modify_font (widget, font);
	pango_font_description_free (font);
}

void ygutils_setWidgetFont (GtkWidget *widget, PangoStyle style, PangoWeight weight, double scale)
{ YGUtils::setWidgetFont (widget, style, weight, scale); }

static void paned_allocate_cb (GtkWidget *paned, GtkAllocation *alloc, gpointer _rel)
{
	if (!g_object_get_data (G_OBJECT (paned), "init")) {  // only once
		gdouble rel = GPOINTER_TO_INT (_rel) / 100.;
		gint parent_size;
		if (gtk_orientable_get_orientation (GTK_ORIENTABLE (paned)) == GTK_ORIENTATION_HORIZONTAL)
			parent_size = paned->allocation.width;
		else
			parent_size = paned->allocation.height;
		int pos = parent_size * rel;
		gtk_paned_set_position (GTK_PANED (paned), pos);
		g_object_set_data (G_OBJECT (paned), "init", GINT_TO_POINTER (1));
	}
}

void YGUtils::setPaneRelPosition (GtkWidget *paned, gdouble rel)
{
	gint _rel = rel * 100;
	g_signal_connect_after (G_OBJECT (paned), "size-allocate",
	                        G_CALLBACK (paned_allocate_cb), GINT_TO_POINTER (_rel));
}

void ygutils_setPaneRelPosition (GtkWidget *paned, gdouble rel)
{ YGUtils::setPaneRelPosition (paned, rel); }

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

// Code from Banshee: shades a pixbuf a bit, used e.g. for hover effects
static inline guchar pixel_clamp (int val)
{ return MAX (0, MIN (255, val)); }
GdkPixbuf *YGUtils::setOpacity (const GdkPixbuf *src, int opacity, bool touchAlpha)
{
	if (!src) return NULL;
	int shift = 255 - ((opacity * 255) / 100);
	int rgb_shift = 0, alpha_shift = 0;
	if (touchAlpha)
		alpha_shift = shift;
	else
		rgb_shift = shift;

	int width = gdk_pixbuf_get_width (src), height = gdk_pixbuf_get_height (src);
	gboolean has_alpha = gdk_pixbuf_get_has_alpha (src);

	GdkPixbuf *dest = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (src),
		has_alpha, gdk_pixbuf_get_bits_per_sample (src), width, height);

	guchar *src_pixels_orig = gdk_pixbuf_get_pixels (src);
	guchar *dest_pixels_orig = gdk_pixbuf_get_pixels (dest);

	int src_rowstride = gdk_pixbuf_get_rowstride (src);
	int dest_rowstride = gdk_pixbuf_get_rowstride (dest);
	int i, j;
	for (i = 0; i < height; i++) {
		guchar *src_pixels = src_pixels_orig + (i * src_rowstride);
		guchar *dest_pixels = dest_pixels_orig + (i * dest_rowstride);
		for (j = 0; j < width; j++) {
			*(dest_pixels++) = pixel_clamp (*(src_pixels++) + rgb_shift);
			*(dest_pixels++) = pixel_clamp (*(src_pixels++) + rgb_shift);
			*(dest_pixels++) = pixel_clamp (*(src_pixels++) + rgb_shift);
			if (has_alpha)
				*(dest_pixels++) = pixel_clamp (*(src_pixels++) - alpha_shift);
		}
	}
	return dest;
}

GdkPixbuf *YGUtils::setGray (const GdkPixbuf *src)
{
	int width = gdk_pixbuf_get_width (src), height = gdk_pixbuf_get_height (src);
	gboolean has_alpha = gdk_pixbuf_get_has_alpha (src);

	GdkPixbuf *dest = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (src),
		has_alpha, gdk_pixbuf_get_bits_per_sample (src), width, height);

	guchar *src_pixels_orig = gdk_pixbuf_get_pixels (src);
	guchar *dest_pixels_orig = gdk_pixbuf_get_pixels (dest);

	int src_rowstride = gdk_pixbuf_get_rowstride (src);
	int dest_rowstride = gdk_pixbuf_get_rowstride (dest);
	int i, j;
	for (i = 0; i < height; i++) {
		guchar *src_pixels = src_pixels_orig + (i * src_rowstride);
		guchar *dest_pixels = dest_pixels_orig + (i * dest_rowstride);
		for (j = 0; j < width; j++) {
			int clr = (src_pixels[0] + src_pixels[1] + src_pixels[2]) / 3;
			*(dest_pixels++) = clr;
			*(dest_pixels++) = clr;
			*(dest_pixels++) = clr;
			src_pixels += 3;
			if (has_alpha)
				*(dest_pixels++) = *(src_pixels++);
		}
	}
	return dest;
}

GdkPixbuf *ygutils_setOpacity (const GdkPixbuf *src, int opacity, gboolean useAlpha)
{ return YGUtils::setOpacity (src, opacity, useAlpha); }

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

struct StockMap {
	const char *english, *locale, *stock;
};
static const StockMap stock_map[] = {
	{ "Apply", _("Apply"), GTK_STOCK_APPLY },
	{ "Accept", _("Accept"), GTK_STOCK_APPLY },
	{ "Install", _("Install"), GTK_STOCK_APPLY },
	{ "OK", _("OK"), GTK_STOCK_OK },
	{ "Cancel", _("Cancel"), GTK_STOCK_CANCEL },
	{ "Close", _("Close"), GTK_STOCK_CLOSE },
	{ "Yes", _("Yes"), GTK_STOCK_YES },
	{ "No", _("No"), GTK_STOCK_NO },
	{ "Add", _("Add"), GTK_STOCK_ADD },
	{ "Edit", _("Edit"), GTK_STOCK_EDIT },
	{ "Delete", _("Delete"), GTK_STOCK_DELETE },
	{ "Up", _("Up"), GTK_STOCK_GO_UP },
	{ "Down", _("Down"), GTK_STOCK_GO_DOWN },
	{ "Enable", _("Enable"), GTK_STOCK_YES },
	{ "Disable", _("Disable"), GTK_STOCK_NO },
	{ "Exit", _("Exit"), GTK_STOCK_QUIT },
};
#define stock_map_length (sizeof (stock_map) / sizeof (StockMap))

const char *YGUtils::mapStockIcon (const std::string &label)
{
	static bool firstTime = true; static std::map <std::string, std::string> stockMap;
	if (firstTime) {
		firstTime = false;

		// match GTK stock labels to yast ones
		GSList *list = gtk_stock_list_ids();
		for (GSList *i = list; i; i = i->next) {
			gchar *id = (gchar *) i->data;
			GtkStockItem item;
			if (gtk_stock_lookup (id, &item)) {
				const gchar *_id = id;
				if (!strcmp (id, GTK_STOCK_MEDIA_NEXT) || !strcmp (id, GTK_STOCK_MEDIA_FORWARD))
					_id = GTK_STOCK_GO_FORWARD;
				else if (!strcmp (id, GTK_STOCK_MEDIA_PREVIOUS) || !strcmp (id, GTK_STOCK_MEDIA_REWIND))
					_id = GTK_STOCK_GO_BACK;
				else if (!strcmp (id, GTK_STOCK_MEDIA_RECORD))
					_id = GTK_STOCK_SAVE;
				else if (!strcmp (id, GTK_STOCK_CLEAR))
					_id = GTK_STOCK_DELETE;
				else if (!strcmp (id, GTK_STOCK_QUIT))
					_id = GTK_STOCK_APPLY;
				else if (!strcmp (id, GTK_STOCK_JUMP_TO))
					_id = GTK_STOCK_OK;
				else if (!strncmp (id, "gtk-dialog-", 11))
					_id = 0;

				if (_id)
					stockMap[cutUnderline (item.label)] = _id;
			}
			// some may not have a stock item because they can't be set on a label
			// e.g.: gtk-directory, gtk-missing-image, gtk-dnd
			g_free (id);
		}
		g_slist_free (list);

		for (unsigned int j = 0; j < 2; j++)  // add both current locale & english terms
			for (unsigned int i = 0; i < stock_map_length; i++)
				stockMap [stock_map[i].english+j] = stock_map[i].stock;
	}

	std::string id = cutUnderline (label);
	stripStart (id, ' ');
	stripEnd (id, ' ');
	stripEnd (id, '.');

	std::map <std::string, std::string>::const_iterator it;
	it = stockMap.find (id);
	if (it != stockMap.end())
		return it->second.c_str();
	return NULL;
}

const char *YGUtils::setStockIcon (GtkWidget *button, const std::string &label,
                                   const char *fallbackIcon)
{
	const char *icon = mapStockIcon (label);
	if (!icon && label.size() < 22)
		icon = fallbackIcon;
	if (icon) {
		if (gtk_style_lookup_icon_set (button->style, icon)) {
			// we want to use GtkImage stock mode so it honors sensitive
			GtkWidget *image = gtk_image_new_from_stock (icon, GTK_ICON_SIZE_BUTTON);
			gtk_button_set_image (GTK_BUTTON (button), image);
		}
	}
	else {
		GtkWidget *image = gtk_button_get_image (GTK_BUTTON (button));
		if (image)
			gtk_widget_hide (image);
	}
	return icon;
}

/*
 * construct a help string by dropping the title, and mentioning
 * the first sentence for a dialog sub-title
 */
gchar *
ygutils_headerize_help (const char *help_text, gboolean *cut)
{
	char *text = ygutils_convert_to_xhtml (help_text);

	GString *str = g_string_new ("");
	int i;
	gboolean copy_word = FALSE;
	for (i = 0; text[i]; i++) {
		if (text[i] == '<') {
			int a = i;
			for (; text[i]; i++)
				if (text[i] == '>')
					break;
			
			if (!strncasecmp (text+a, "<h", 2) || !strncasecmp (text+a, "<big>", 5) ||
			    (!str->len && !strncasecmp (text+a, "<b>", 3))) {
				for (i++; text[i]; i++) {
					if (text[i] == '<')
						a = i;
					if (text[i] == '>') {
						if (!strncasecmp (text+a, "</h", 3) || !strncasecmp (text+a, "</big>", 6) ||
						    !strncasecmp (text+a, "</b>", 4))
							break;
					}
				}
			}
		}
		else if (g_ascii_isspace (text[i])) {
			if (copy_word)
				g_string_append_c (str, ' ');
			copy_word = FALSE;
		}
		else {
			copy_word = TRUE;
			g_string_append_c (str, text[i]);
			if (text[i] == '.') {
				if (g_ascii_isspace (text[i+1]) || text[i+1] == '<') {
					i++;
					break;
				}
			}
		}
	}
	*cut = FALSE;
	gboolean markup = FALSE;
	for (; text[i]; i++) {
		if (markup) {
			if (text[i] == '>')
				markup = FALSE;
		}
		else {
			if (text[i] == '<')
				markup = TRUE;
			else if (!g_ascii_isspace (text[i])) {
				*cut = TRUE;
				break;
			}
		}
	}
	g_free (text);
	return g_string_free (str, FALSE);
}

const char *ygutils_mapStockIcon (const char *label)
{ return YGUtils::mapStockIcon (label); }

const char *ygutils_setStockIcon (GtkWidget *button, const char *label, const char *fallback)
{ return YGUtils::setStockIcon (button, label, fallback); }

/* interactive busy cursor */
// half cursor, half clock cursor is not a Xlib theme icon, but there is
// a hack to load it like: (if we ever want to use it...)
#if 0
__LEFT_PTR_WATCH = None
def set_busy_cursor (window):
    global __LEFT_PTR_WATCH
    if __LEFT_PTR_WATCH is None:
        os.environ['XCURSOR_DISCOVER'] = '1' #Turn on logging in Xlib
        # Busy cursor code from Padraig Brady <P@draigBrady.com>
        # cursor_data hash is 08e8e1c95fe2fc01f976f1e063a24ccd
        cursor_data = "\
\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\
\x0c\x00\x00\x00\x1c\x00\x00\x00\x3c\x00\x00\x00\
\x7c\x00\x00\x00\xfc\x00\x00\x00\xfc\x01\x00\x00\
\xfc\x3b\x00\x00\x7c\x38\x00\x00\x6c\x54\x00\x00\
\xc4\xdc\x00\x00\xc0\x44\x00\x00\x80\x39\x00\x00\
\x80\x39\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00"

        try:
            pix = gtk.gdk.bitmap_create_from_data(None, cursor_data, 32, 32)
            color = gtk.gdk.Color()
            __LEFT_PTR_WATCH = gtk.gdk.Cursor(pix, pix, color, color, 2, 2)
        except TypeError:
            # old bug http://bugzilla.gnome.org/show_bug.cgi?id=103616
            # default "WATCH" cursor
            __LEFT_PTR_WATCH = gtk.gdk.Cursor(gtk.gdk.WATCH)
    window.set_cursor (__LEFT_PTR_WATCH)
#endif


gboolean YGUtils::empty_row_is_separator_cb (
	GtkTreeModel *model, GtkTreeIter *iter, gpointer _text_col)
{
	int text_col = GPOINTER_TO_INT (_text_col);
	gchar *str;
	gtk_tree_model_get (model, iter, text_col, &str, -1);
	bool ret = !str || !(*str);
	g_free (str);
	return ret;
}

