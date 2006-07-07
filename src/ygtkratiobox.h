/* RatioBox container */

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

typedef struct _YGtkRatioBox	     YGtkRatioBox;
typedef struct _YGtkRatioBoxClass  YGtkRatioBoxClass;
typedef struct _YGtkRatioBoxChild  YGtkRatioBoxChild;

struct _YGtkRatioBox
{
	GtkContainer container;

	/*< public (read-only) >*/
	GList *children;
	gint16 spacing;

	/*< private >*/
	guint orientation : 1;
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

typedef enum YGtkRatioBoxOrientation {
	YGTK_RATIO_BOX_HORIZONTAL_ORIENTATION = 0,
	YGTK_RATIO_BOX_VERTICAL_ORIENTATION   = 1
} YGtkRatioBoxOrientation;

GtkWidget* ygtk_ratio_box_new (YGtkRatioBoxOrientation orientation, gint spacing);
GType ygtk_ratio_box_get_type (void) G_GNUC_CONST;

void ygtk_ratio_box_set_spacing (YGtkRatioBox *box, gint spacing);
gint ygtk_ratio_box_get_spacing (YGtkRatioBox *box);

void ygtk_ratio_box_query_child_packing (YGtkRatioBox *box, GtkWidget *child,
                                         gfloat *ratio, gboolean *fill,
                                         guint *padding);
void ygtk_ratio_box_set_child_packing (YGtkRatioBox *box, GtkWidget *child, gfloat ratio,
                                       gboolean fill, guint padding);

G_END_DECLS

#endif /* YGTK_RATIO_BOX_H */
