/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkHtmlWrap widget */
// check the header file for information about this widget

#include <config.h>
#include <gtk/gtkversion.h>
#include <string.h>
#include "ygtkhtmlwrap.h"

//#define USE_YGTKRICHTEXT
//#define USE_GTKHTML
//#define USE_WEBKIT

// ygutils
#include <gtk/gtktextview.h>
void ygutils_scrollView (GtkTextView *view, gboolean top);
void ygutils_scrollAdj (GtkAdjustment *vadj, gboolean top);
GtkWidget *ygtk_html_wrap_new (void)
{ return g_object_new (ygtk_html_wrap_get_type(), NULL); }

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

void ygtk_html_wrap_connect_link_clicked (GtkWidget *widget, GCallback callback, gpointer data)
{
	g_signal_connect (G_OBJECT (widget), "link-clicked", callback, data);
}

//** WebKit
#else
#if USE_WEBKIT
#include <webkit/webkit.h>

GType ygtk_html_wrap_get_type()
{
	return WEBKIT_TYPE_WEB_VIEW;
}

void ygtk_html_wrap_init (GtkWidget *widget)
{
}

void ygtk_html_wrap_set_text (GtkWidget *widget, const gchar* text, gboolean plain_mode)
{
	webkit_web_view_load_html_string (WEBKIT_WEB_VIEW (widget), text, ".");
}

void ygtk_html_wrap_scroll (GtkWidget *widget, gboolean top)
{
	// TODO
}

gboolean ygtk_html_wrap_search (GtkWidget *widget, const gchar *text)
{
	WebKitWebView *view = WEBKIT_WEB_VIEW (widget);
	gboolean found = webkit_web_view_mark_text_matches (view, text, FALSE, -1);
	webkit_web_view_set_highlight_text_matches (view, TRUE);
	return found;
}

gboolean ygtk_html_wrap_search_next (GtkWidget *widget, const gchar *text)
{
	WebKitWebView *view = WEBKIT_WEB_VIEW (widget);
	return webkit_web_view_search_text (view, text, FALSE, TRUE, TRUE);
}

static gpointer data2;
static WebKitNavigationResponse ygtk_webkit_navigation_requested_cb (
	WebKitWebView *view, WebKitWebFrame *frame, WebKitNetworkRequest *request, void (*callback) (gpointer d))
{
	(*callback) (data2);
	return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
}

void ygtk_html_wrap_connect_link_clicked (GtkWidget *widget, GCallback callback, gpointer data)
{
	data2 = data;
	g_signal_connect (G_OBJECT (widget), "navigation-requested",
	                  G_CALLBACK (ygtk_webkit_navigation_requested_cb), callback);
}

//** YGtkRichText
#else
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
	ygtk_rich_text_set_text (YGTK_RICH_TEXT (widget), text, plain_mode);
}

void ygtk_html_wrap_scroll (GtkWidget *widget, gboolean top)
{
	ygutils_scrollView (GTK_TEXT_VIEW (widget), top);
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

void ygtk_html_wrap_connect_link_clicked (GtkWidget *widget, GCallback callback, gpointer data)
{
	g_signal_connect (G_OBJECT (widget), "link-clicked", callback, data);
}

void ygtk_html_wrap_set_background (GtkWidget *widget, GdkPixbuf *pixbuf)
{
	ygtk_rich_text_set_background (YGTK_RICH_TEXT (widget), pixbuf);
}

#endif
#endif

