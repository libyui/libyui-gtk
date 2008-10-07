/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkRichText is a very simple widget that displays HTML code.
   It was done, since GTK+ doesn't offer one out of box, and to
   avoid dependencies.
*/

#ifndef YGTK_RICH_TEXT_H
#define YGTK_RICH_TEXT_H

#include <gtk/gtktextview.h>
G_BEGIN_DECLS

#define YGTK_TYPE_RICH_TEXT            (ygtk_rich_text_get_type ())
#define YGTK_RICH_TEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                       YGTK_TYPE_RICH_TEXT, YGtkRichText))
#define YGTK_RICH_TEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                       YGTK_TYPE_RICH_TEXT, YGtkRichTextClass))
#define YGTK_IS_RICH_TEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                       YGTK_TYPE_RICH_TEXT))
#define YGTK_IS_RICH_TEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                       YGTK_TYPE_RICH_TEXT))
#define YGTK_RICH_TEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                       YGTK_TYPE_RICH_TEXT, YGtkRichTextClass))

typedef struct _YGtkRichText
{
	GtkTextView parent;
	// members:
	GdkCursor *hand_cursor;
	GdkPixbuf *background_pixbuf;
} YGtkRichText;

typedef struct _YGtkRichTextClass
{
	GtkTextViewClass parent_class;

	// signals:
	void (*link_clicked) (YGtkRichText *rich_text, const gchar *link);
} YGtkRichTextClass;

GtkWidget *ygtk_rich_text_new (void);
GType ygtk_rich_text_get_type (void) G_GNUC_CONST;

/* Sets some text to YGtkRichText, may be HTML or plain text, as indicated by
   rich_text. */
void ygtk_rich_text_set_text (YGtkRichText* rtext, const gchar* text, gboolean plain_mode);

// To be used together with an entry box to search for text
gboolean ygtk_rich_text_mark_text (YGtkRichText *rtext, const gchar *text);
gboolean ygtk_rich_text_forward_mark (YGtkRichText *rtext, const gchar *text);  // F3

void ygtk_rich_text_set_background (YGtkRichText *rtext, GdkPixbuf *pixbuf);

G_END_DECLS
#endif /* YGTK_RICH_TEXT_H */

