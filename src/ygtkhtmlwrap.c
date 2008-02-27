/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkHtmlWrap widget */
// check the header file for information about this widget

#include <config.h>
#include <gtk/gtkversion.h>
#include <string.h>
#include "ygtkhtmlwrap.h"

GtkWidget *ygtk_html_wrap_new (void)
{
	return g_object_new (ygtk_html_wrap_get_type(), NULL);
}


// Utilities
static inline void adjust_scroll (GtkAdjustment *vadj, gboolean top)
{
	g_assert (vadj != 0);
	if (top)
		gtk_adjustment_set_value (vadj, vadj->lower);	
	else  /* bottom */
		gtk_adjustment_set_value (vadj, vadj->upper - vadj->page_size);		
}

static void gdkwindow_set_background (GdkWindow *window, const char *image)
{
	if (image) {
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
	else
		gdk_window_clear (window);
}

// GtkHTML
#ifdef USE_GTKHTML
#include <libgtkhtml-3.14/gtkhtml/gtkhtml.h>
#include <libgtkhtml-3.14/gtkhtml/gtkhtml-stream.h>
#include <libgtkhtml-3.14/gtkhtml/gtkhtml-search.h>

GType ygtk_html_wrap_get_type (void)
{
	return GTK_TYPE_HTML;
}

static void gtkhtml_url_requested_cb (GtkHTML *html, const gchar *url, GtkHTMLStream *stream)
{	// to load images (and possibly other external embed files)
	FILE *file = fopen (url, "rb");

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
	adjust_scroll (GTK_LAYOUT (widget)->vadjustment, top);
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

void ygtk_html_wrap_set_background (GtkWidget *widget, const char *image)
{
	// TODO
}

// YGtkRichText
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
	adjust_scroll (GTK_TEXT_VIEW (widget)->vadjustment, top);
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

void ygtk_html_wrap_set_background (GtkWidget *widget, const char *image)
{
	g_return_if_fail (GTK_WIDGET_REALIZED (widget));
	GdkWindow *window = gtk_text_view_get_window
		(GTK_TEXT_VIEW (widget), GTK_TEXT_WINDOW_TEXT);
	gdkwindow_set_background (window, image);
}

#endif

