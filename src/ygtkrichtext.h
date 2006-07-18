/* RatioBox container */

#ifndef YGTK_RICHTEXT_H
#define YGTK_RICHTEXT_H

#include <gdk/gdk.h>
#include <gtk/gtktextview.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

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
	GdkPixbuf *background;
};

struct _YGtkRichTextClass
{
	GtkTextViewClass parent_class;

	// signal
	void (*link_pressed) (YGtkRichText *rich_text, const gchar* link);
};

GtkWidget* ygtk_richtext_new();
GType ygtk_richtext_get_type (void) G_GNUC_CONST;

void ygtk_richtext_set_text (YGtkRichText* rtext, const gchar* text,
                             gboolean plain_text);

void ygtk_richttext_set_prodname (YGtkRichText *rtext, const char *prodname);

void ygtk_richtext_set_background (YGtkRichText *rtext, const char *image);

G_END_DECLS

#endif /* YGTK_RICHTEXT_H */
