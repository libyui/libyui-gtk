//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

/* YGtkRatioBox is an improvement over the GtkBox container that
   allows the programmer to set the size containees as a ratio,
   as opposed to just define their size as expandable or not.
   (ratio = 0 would be the equivalent of expand = FALSE, while
    ratio = 1 is the equivalent to expand = TRUE.)

   YGtkRatioBox could be API (and ABI for the matter) compatible with
   GtkBox, but isn't, as there is no compelling reason for that, since it
   would require not only initial work, but the code would get bigger,
   which is obviously undesirable for maintance.
   However, if you want to use YGtkRatioBox as a replacement for GtkBox,
   feel free to contact us that we may give you hand in accomplishing that.

   Limitations:
     * containees visibility is not supported
     * text direction is not honored
*/

#ifndef YGTK_RATIO_BOX_H
#define YGTK_RATIO_BOX_H

#include <gdk/gdk.h>
#include <gtk/gtkcontainer.h>

G_BEGIN_DECLS

#define YGTK_TYPE_RATIO_BOX            (ygtk_ratio_box_get_type ())
#define YGTK_RATIO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                        YGTK_TYPE_RATIO_BOX, YGtkRatioBox))
#define YGTK_RATIO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                        YGTK_TYPE_RATIO_BOX, YGtkRatioBoxClass))
#define IS_YGTK_RATIO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                        YGTK_TYPE_RATIO_BOX))
#define IS_YGTK_RATIO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                        YGTK_TYPE_RATIO_BOX))
#define YGTK_RATIO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                        YGTK_TYPE_RATIO_BOX, YGtkRatioBoxClass))

typedef struct _YGtkRatioBox       YGtkRatioBox;
typedef struct _YGtkRatioBoxClass  YGtkRatioBoxClass;
typedef struct _YGtkRatioBoxChild  YGtkRatioBoxChild;

struct _YGtkRatioBox
{
	GtkContainer container;

	/*< public (read-only) >*/
	GList *children;
	gint16 spacing;

	/*< private >*/
	gfloat ratios_sum;
};

struct _YGtkRatioBoxClass
{
	GtkContainerClass parent_class;
};

struct _YGtkRatioBoxChild
{
	GtkWidget *widget;
	// Proprieties
	guint16 padding;
	guint fill : 1;
	gfloat ratio;
};

GType ygtk_ratio_box_get_type (void) G_GNUC_CONST;

void ygtk_ratio_box_set_spacing (YGtkRatioBox *box, gint spacing);
gint ygtk_ratio_box_get_spacing (YGtkRatioBox *box);

void ygtk_ratio_box_pack (YGtkRatioBox *box, GtkWidget *child,
                          gfloat ratio, gboolean fill, guint padding);

void ygtk_ratio_box_query_child_packing (YGtkRatioBox *box, GtkWidget *child,
                                         gfloat *ratio, gboolean *fill,
                                         guint *padding);
void ygtk_ratio_box_set_child_packing (YGtkRatioBox *box, GtkWidget *child, gfloat ratio,
                                       gboolean fill, guint padding);

/* RatioHBox */

#define YGTK_TYPE_RATIO_HBOX            (ygtk_ratio_hbox_get_type ())
#define YGTK_RATIO_HBOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                        YGTK_TYPE_RATIO_HBOX, YGtkRatioHBox))
#define YGTK_RATIO_HBOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                        YGTK_TYPE_RATIO_HBOX, YGtkRatioHBoxClass))
#define IS_YGTK_RATIO_HBOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                        YGTK_TYPE_RATIO_HBOX))
#define IS_YGTK_RATIO_HBOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                        YGTK_TYPE_RATIO_HBOX))
#define YGTK_RATIO_HBOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                        YGTK_TYPE_RATIO_HBOX, YGtkRatioHBoxClass))

typedef struct _YGtkRatioHBox       YGtkRatioHBox;
typedef struct _YGtkRatioHBoxClass  YGtkRatioHBoxClass;

struct _YGtkRatioHBox
{
	YGtkRatioBox ratiobox;
};

struct _YGtkRatioHBoxClass
{
	YGtkRatioBoxClass parent_class;
};

GtkWidget* ygtk_ratio_hbox_new (gint spacing);
GType ygtk_ratio_hbox_get_type (void) G_GNUC_CONST;

/* RatioVBox */

#define YGTK_TYPE_RATIO_VBOX            (ygtk_ratio_vbox_get_type ())
#define YGTK_RATIO_VBOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                        YGTK_TYPE_RATIO_VBOX, YGtkRatioVBox))
#define YGTK_RATIO_VBOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                        YGTK_TYPE_RATIO_VBOX, YGtkRatioVBoxClass))
#define IS_YGTK_RATIO_VBOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                        YGTK_TYPE_RATIO_VBOX))
#define IS_YGTK_RATIO_VBOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                        YGTK_TYPE_RATIO_VBOX))
#define YGTK_RATIO_VBOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                        YGTK_TYPE_RATIO_VBOX, YGtkRatioVBoxClass))

typedef struct _YGtkRatioVBox       YGtkRatioVBox;
typedef struct _YGtkRatioVBoxClass  YGtkRatioVBoxClass;

struct _YGtkRatioVBox
{
	YGtkRatioBox ratiobox;
};

struct _YGtkRatioVBoxClass
{
	YGtkRatioBoxClass parent_class;
};

GtkWidget* ygtk_ratio_vbox_new (gint spacing);
GType ygtk_ratio_vbox_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* YGTK_RATIO_BOX_H */
