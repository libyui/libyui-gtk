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
#define IS_YGTK_RICHTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                       YGTK_TYPE_RICHTEXT))
#define IS_YGTK_RICHTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                       YGTK_TYPE_RICHTEXT))
#define YGTK_RICHTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                       YGTK_TYPE_RICHTEXT, YGtkRichTextClass))

typedef struct _YGtkRichText       YGtkRichText;
typedef struct _YGtkRichTextClass  YGtkRichTextClass;

struct _YGtkRichText
{
	GtkTextView text_view;

	char *prodname;
};

struct _YGtkRichTextClass
{
	GtkTextViewClass parent_class;

	// signal
	void (*link_pressed) (YGtkRichText *rich_text, const gchar* link);
};

GtkWidget* ygtk_richtext_new();
GType ygtk_richtext_get_type (void) G_GNUC_CONST;

/* Sets some text to YGtkRichText that may be HTML or just plain, according
   to "plain_text". In HTML case, "honor_br_char" means if the renderer
   should break lines on the string '\n' characters. */
void ygtk_richtext_set_text (YGtkRichText* rtext, const gchar* text,
                             gboolean plain_text, gboolean honor_br_char);

void ygtk_richttext_set_prodname (YGtkRichText *rtext, const char *prodname);

// Set a background image -- widget must have be realized
void ygtk_richtext_set_background (YGtkRichText *rtext, const char *image);

G_END_DECLS

#endif /* YGTK_RICHTEXT_H */
