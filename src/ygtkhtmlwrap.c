/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkHtmlWrap widget */
// check the header file for information about this widget


#include <gtk/gtk.h>
#include <string.h>
#include "ygtkhtmlwrap.h"

// ygutils
void ygutils_scrollAdj (GtkAdjustment *vadj, gboolean top);

GtkWidget *ygtk_html_wrap_new (void)
{
	GtkWidget *widget = g_object_new (ygtk_html_wrap_get_type(), NULL);
	ygtk_html_wrap_init (widget);
	return widget;
}

//** WebKit
#ifdef USE_WEBKIT
#include <webkit/webkit.h>

GType ygtk_html_wrap_get_type()
{
	return WEBKIT_TYPE_WEB_VIEW;
}

static void copy_activate_cb (GtkMenuItem *item, WebKitWebView *view)
{ webkit_web_view_copy_clipboard (view); }
static void select_all_activate_cb (GtkMenuItem *item, WebKitWebView *view)
{ webkit_web_view_select_all (view); }

static gboolean button_press_event_cb (GtkWidget *widget, GdkEventButton *event,
                                       WebKitWebView *view)
{
	if (event && event->button != 3)
		return FALSE;
	// GtkMenu API is horrible for non-persistant menus. Make it persistant.
	static GtkWidget *menu = 0;
	if (menu)
		gtk_widget_destroy (menu);
	menu = gtk_menu_new();
	// add a couple of items (based on GtkTextView)
	GtkWidget *item;
	item = gtk_image_menu_item_new_from_stock (GTK_STOCK_COPY, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	if (webkit_web_view_can_copy_clipboard (view))
		g_signal_connect (item, "activate", G_CALLBACK (copy_activate_cb), widget);
	else
		gtk_widget_set_sensitive (item, FALSE);
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	item = gtk_image_menu_item_new_from_stock (GTK_STOCK_SELECT_ALL, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (item, "activate", G_CALLBACK (select_all_activate_cb), widget);

	int button, event_time;
	if (event) {
		button = event->button;
		event_time = event->time;
	}
	else {
		button = 0;
		event_time = gtk_get_current_event_time();
	}
	gtk_menu_attach_to_widget (GTK_MENU (menu), widget, NULL);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, button, event_time);
	gtk_widget_show_all (menu);
	return TRUE;
}

static gboolean popup_menu_cb (GtkWidget *widget, WebKitWebView *view)
{
	button_press_event_cb (widget, NULL, view);
	return TRUE;
}

static gboolean button_release_event_cb (GtkWidget *widget, GdkEventButton *event)
{
	if (event->button != 3) return FALSE;
	return TRUE;
}

void ygtk_html_wrap_init (GtkWidget *widget)
{
	/* WebKit popup menu has entries such as "reload", "go back" and "search web"
	   that we rather not show. "popup-menu" isn't working, use gtkwidget stuff... */
	g_signal_connect (G_OBJECT (widget), "popup-menu",
	                  G_CALLBACK (popup_menu_cb), widget);
	g_signal_connect (G_OBJECT (widget), "button-press-event",
	                  G_CALLBACK (button_press_event_cb), widget);
	// webkit crashes if we don't overload the release event as well
	g_signal_connect (G_OBJECT (widget), "button-release-event",
	                  G_CALLBACK (button_release_event_cb), widget);
}

// CSS roughly based on Yelp style
const char *CSS = "<style type=\"text/css\">"
"h1 { color: #5c5c5c; font-size: xx-large; font-weight: 900; border-bottom: thin solid cdcfd1; }"
"h2 { color: #5c5c5c; font-size: x-large; font-weight: 800; border-bottom: thin solid cdcfd1; }"
"h3 { color: #5c5c5c; font-size: large; font-weight: 700; border-bottom: thin solid cdcfd1; }"
"h4 { color: #5c5c5c; font-size: large; font-weight: 600; }"
"h5 { color: #5c5c5c; font-size: large; }"
"pre { background-color: #f0f0f0; border: thin solid #bfbfbf; padding: 2px; }"
"a { text-decoration: none; }"
"a:hover { text-decoration: underline; }"
"</style>";

void ygtk_html_wrap_set_text (GtkWidget *widget, const gchar *text, gboolean plain_mode)
{
	WebKitWebView *view = WEBKIT_WEB_VIEW (widget);
	// webkit prepends base-uri to non-uri hrefs
	if (plain_mode)
			webkit_web_view_load_string (view, text, "text/plain", "UTF-8", "/");
	else {
		GString *str = NULL;
		int i, last_i = 0;
		for (i = 0; text[i]; i++) {
			if (!strncmp (text+i, "href=", 5)) {
				i += 5;
				if (text[i] == '"')
					i++;
				int j;
				for (j = i; text[j] && text[j] != ':'; j++)
					if (text[j] == '"' || g_ascii_isspace (text[j])) {
						if (!str)
							str = g_string_new ("");
						str = g_string_append_len (str, text+last_i, i-last_i);
						last_i = i;
						str = g_string_append (str, "label:/");
						break;
					}
			}
		}
		if (str) {
			str = g_string_append (str, text+last_i);
			text = g_string_free (str, FALSE);
		}

		const gchar *extra_css = g_object_get_data (G_OBJECT (widget), "extra-css");
		if (!extra_css) extra_css = "";

		gchar *html = g_strdup_printf ("%s\n%s\n%s", CSS, extra_css, text);
		if (str)
			g_free ((gchar *) text);
		webkit_web_view_load_string (view, html, "text/html", "UTF-8", "/");
		g_free (html);
	}
}

void ygtk_html_wrap_scroll (GtkWidget *widget, gboolean top)
{
	GtkWidget *scroll_win = gtk_widget_get_parent (widget);
	ygutils_scrollAdj (gtk_scrolled_window_get_vadjustment (
		GTK_SCROLLED_WINDOW (scroll_win)), top);
}

gboolean ygtk_html_wrap_search (GtkWidget *widget, const gchar *text)
{
	WebKitWebView *view = WEBKIT_WEB_VIEW (widget);
	webkit_web_view_unmark_text_matches (view);
	if (*text) {
		gboolean found = webkit_web_view_mark_text_matches (view, text, FALSE, -1);
		webkit_web_view_set_highlight_text_matches (view, TRUE);
		if (found)
			ygtk_html_wrap_search_next (widget, text);
		return found;
	}
	// we want to un-select previous search (no such api though)
	return TRUE;
}

gboolean ygtk_html_wrap_search_next (GtkWidget *widget, const gchar *text)
{
	WebKitWebView *view = WEBKIT_WEB_VIEW (widget);
	return webkit_web_view_search_text (view, text, FALSE, TRUE, TRUE);
}

static WebKitNavigationResponse ygtk_webkit_navigation_requested_cb (
	WebKitWebView *view, WebKitWebFrame *frame, WebKitNetworkRequest *request, LinkClickedCb callback)
{
	const gchar *uri = webkit_network_request_get_uri (request);
	// look for set_text to see why we need to cut the uri in some cases
	// (hint: not an uri)
	if (!strncmp (uri, "label:/", sizeof ("label:/")-1))
		uri = uri + sizeof ("label:/")-1;
	gpointer data = g_object_get_data (G_OBJECT (view), "pointer");
	(*callback) (GTK_WIDGET (view), uri, data);
	return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
}

void ygtk_html_wrap_connect_link_clicked (GtkWidget *widget, LinkClickedCb callback, gpointer data)
{
	g_object_set_data (G_OBJECT (widget), "pointer", data);
	g_signal_connect (G_OBJECT (widget), "navigation-requested",
	                  G_CALLBACK (ygtk_webkit_navigation_requested_cb), callback);
}

void ygtk_html_wrap_set_background (GtkWidget *widget, GdkPixbuf *pixbuf, const gchar *filename)
{
	int width = gdk_pixbuf_get_width (pixbuf);
	gchar *bg_css = g_strdup_printf (
		"<style type=\"text/css\">"
		"body { background-image: url('%s'); background-repeat: no-repeat;"
		"background-position: %dpx 100%%; background-attachment: fixed; }"
		"</style>", filename, -width+40);
	g_object_set_data_full (G_OBJECT (widget), "extra-css", bg_css, g_free);
}

#else

//** GtkHTML
#ifdef USE_GTKHTML
#include <gtkhtml/gtkhtml.h>
#include <gtkhtml/gtkhtml-stream.h>
#include <gtkhtml/gtkhtml-search.h>

GType ygtk_html_wrap_get_type (void)
{
	return GTK_TYPE_HTML;
}

static void gtkhtml_url_requested_cb (GtkHTML *html, const gchar *url, GtkHTMLStream *stream)
{	// to load images (and possibly other external embed files)
	FILE *file = fopen (url, "rb");
	if (!file) {
		g_warning ("Error: couldn't open file '%s'\n", url);
		return;
	}

	fseek (file, 0, SEEK_END);
	size_t file_size = ftell (file);
	rewind (file);

	gboolean error;
	gchar *data = g_new (gchar, file_size);
	error = fread (data, 1, file_size, file) < file_size;
	fclose (file);

	if (!error)
		gtk_html_stream_write (stream, data, file_size);
	g_free (data);
}

void ygtk_html_wrap_init (GtkWidget *widget)
{
	gtk_html_set_editable (GTK_HTML (widget), FALSE);
	g_signal_connect (G_OBJECT (widget), "url-requested",
	                  G_CALLBACK (gtkhtml_url_requested_cb), NULL);
}

void ygtk_html_wrap_set_text (GtkWidget *widget, const gchar* text, gboolean plain_mode)
{
	// TODO: implement plain_mode
	GtkHTMLStream *stream = gtk_html_begin (GTK_HTML (widget));
	gtk_html_write (GTK_HTML (widget), stream, text, strlen (text));
	gtk_html_end (GTK_HTML (widget), stream, GTK_HTML_STREAM_OK);
}

void ygtk_html_wrap_scroll (GtkWidget *widget, gboolean top)
{
	ygutils_scrollAdj (GTK_LAYOUT (widget)->vadjustment, top);
}

gboolean ygtk_html_wrap_search (GtkWidget *widget, const gchar *text)
{
/*	if (*text == '\0')
		return TRUE;*/
	return gtk_html_engine_search (GTK_HTML (widget), text, FALSE, TRUE, FALSE);
}

gboolean ygtk_html_wrap_search_next (GtkWidget *widget, const gchar *text)
{
	return gtk_html_engine_search_next (GTK_HTML (widget));
}

void ygtk_html_wrap_connect_link_clicked (GtkWidget *widget, LinkClickedCb callback, gpointer data)
{
	g_signal_connect (G_OBJECT (widget), "link-clicked", G_CALLBACK (callback), data);
}

void ygtk_html_wrap_set_background (GtkWidget *widget, GdkPixbuf *pixbuf, const gchar *filename)
{
	// TODO
}

#else
//** YGtkRichText (internal)
#include "ygtkrichtext.h"

GType ygtk_html_wrap_get_type()
{
	return YGTK_TYPE_RICH_TEXT;
}

void ygtk_html_wrap_init (GtkWidget *widget)
{
}

void ygtk_html_wrap_set_text (GtkWidget *widget, const gchar* text, gboolean plain_mode)
{
	YGtkRichText *rtext = YGTK_RICH_TEXT (widget);
	if (plain_mode)
		ygtk_rich_text_set_plain_text (rtext, text);
	else
		ygtk_rich_text_set_text (rtext, text);
}

void ygtk_html_wrap_scroll (GtkWidget *widget, gboolean top)
{
        ygutils_scrollAdj (gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (widget)), top);
}

gboolean ygtk_html_wrap_search (GtkWidget *widget, const gchar *text)
{
	gboolean ret = ygtk_rich_text_mark_text (YGTK_RICH_TEXT (widget), text);
	ygtk_rich_text_forward_mark (YGTK_RICH_TEXT (widget), text);
	return ret;
}

gboolean ygtk_html_wrap_search_next (GtkWidget *widget, const gchar *text)
{
	return ygtk_rich_text_forward_mark (YGTK_RICH_TEXT (widget), text);
}

void ygtk_html_wrap_connect_link_clicked (GtkWidget *widget, LinkClickedCb callback, gpointer data)
{
	g_signal_connect (G_OBJECT (widget), "link-clicked", G_CALLBACK (callback), data);
}

void ygtk_html_wrap_set_background (GtkWidget *widget, GdkPixbuf *pixbuf, const gchar *filename)
{
	ygtk_rich_text_set_background (YGTK_RICH_TEXT (widget), pixbuf);
}

#endif
#endif

