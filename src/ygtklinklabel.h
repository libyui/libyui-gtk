/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* A label with embed links. Because we only need a link at the end, that's
   only what we support.

   In the future, we may want to use GtkLabel's new link support instead of
   this custom widget.
*/

#ifndef YGTK_LINK_LABEL_H
#define YGTK_LINK_LABEL_H

#include <gtk/gtk.h>
G_BEGIN_DECLS

#define YGTK_TYPE_LINK_LABEL            (ygtk_link_label_get_type ())
#define YGTK_LINK_LABEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                         YGTK_TYPE_LINK_LABEL, YGtkLinkLabel))
#define YGTK_LINK_LABEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                         YGTK_TYPE_LINK_LABEL, YGtkLinkLabelClass))
#define YGTK_IS_LINK_LABEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                         YGTK_TYPE_LINK_LABEL))
#define YGTK_IS_LINK_LABEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                         YGTK_TYPE_LINK_LABEL))
#define YGTK_LINK_LABEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                         YGTK_TYPE_LINK_LABEL, YGtkLinkLabelClass))

typedef struct _YGtkLinkLabel YGtkLinkLabel;
typedef struct _YGtkLinkLabelClass YGtkLinkLabelClass;

struct _YGtkLinkLabel
{
	GtkWidget parent;
	// private (read-only):
	gchar *text, *link;
	gboolean link_always_visible;
	PangoLayout *layout, *link_layout;
	GdkWindow *link_window;
};

struct _YGtkLinkLabelClass
{
	GtkWidgetClass parent_class;
	// signals:
	void (*link_clicked) (YGtkLinkLabel *label);
};

GtkWidget *ygtk_link_label_new (const gchar *text, const gchar *link);
GType ygtk_link_label_get_type (void) G_GNUC_CONST;

void ygtk_link_label_set_text (YGtkLinkLabel *label, const gchar *text, const gchar *link,
                                gboolean always_show_link);

G_END_DECLS
#endif /*YGTK_LINK_LABEL_H*/

