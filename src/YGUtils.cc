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

static gboolean scroll_down_cb (void *pData)
{
	GtkAdjustment *vadj = (GtkAdjustment *) pData;
	gtk_adjustment_set_value (vadj, vadj->upper - vadj->page_size);
	return FALSE;
}
void YGUtils::scrollWidget (GtkAdjustment *vadj, bool top)
{
	// for some widgets, we need to change adjustment before moving down...
	gtk_adjustment_set_value (vadj, vadj->lower);
	if (!top) {
		// since we usually want to call this together with a text change, we
		// must wait till that gets in effect
		g_timeout_add_full (G_PRIORITY_LOW, 25, scroll_down_cb, vadj, NULL);
	}
}

void ygutils_scrollAdj (GtkAdjustment *vadj, gboolean top)
{ YGUtils::scrollWidget (vadj, top); }

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

bool YGUtils::setStockIcon (GtkWidget *button, const std::string &label,
                            const char *fallbackIcon)
{
	static bool firstTime = true; static std::map <std::string, std::string> stockMap;
	if (firstTime) {
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
				stockMap[cutUnderline (item.label)] = _id;
			}
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
	bool foundIcon = it != stockMap.end();
	const char *icon = NULL;
	if (foundIcon)
		icon = it->second.c_str();
	else if (id.size() < 22)
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
	return foundIcon;
}

gboolean ygutils_setStockIcon (GtkWidget *button, const char *label,
                               const char *fallbackIcon)
{ return YGUtils::setStockIcon (button, label, fallbackIcon); }

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

