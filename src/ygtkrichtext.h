/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkRichText is a very simple widget that displays HTML code.
   It was done, since GTK+ doesn't offer one out of box, and to
   avoid dependencies.
*/

#ifndef YGTK_RICHTEXT_H
#define YGTK_RICHTEXT_H

#include <gtk/gtktextview.h>
G_BEGIN_DECLS

#define YGTK_TYPE_RICHTEXT            (ygtk_richtext_get_type ())
#define YGTK_RICHTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                       YGTK_TYPE_RICHTEXT, YGtkRichText))
#define YGTK_RICHTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                       YGTK_TYPE_RICHTEXT, YGtkRichTextClass))
#define YGTK_IS_RICHTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                       YGTK_TYPE_RICHTEXT))
#define YGTK_IS_RICHTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                       YGTK_TYPE_RICHTEXT))
#define YGTK_RICHTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                       YGTK_TYPE_RICHTEXT, YGtkRichTextClass))

typedef struct _YGtkRichText
{
	GtkTextView parent;
	// private:
	char *prodname;
} YGtkRichText;

typedef struct _YGtkRichTextClass
{
	GtkTextViewClass parent_class;

	// signals:
	void (*link_pressed) (YGtkRichText *rich_text, const gchar *link);
} YGtkRichTextClass;

GtkWidget* ygtk_richtext_new();
GType ygtk_richtext_get_type (void) G_GNUC_CONST;

/* Sets some text to YGtkRichText, may be HTML or plain text, as indicated by
   rich_text. */
void ygtk_richtext_set_text (YGtkRichText* rtext, const gchar* text,
                             gboolean rich_text);

void ygtk_richttext_set_prodname (YGtkRichText *rtext, const char *prodname);

// Set a background image -- widget must be realized
void ygtk_richtext_set_background (YGtkRichText *rtext, const char *image);

// To be used together with an entry box to search for text
gboolean ygtk_richtext_mark_text (YGtkRichText *rtext, const gchar *text);
gboolean ygtk_richtext_forward_mark (YGtkRichText *rtext, const gchar *text);  // F3

G_END_DECLS
#endif /* YGTK_RICHTEXT_H */
