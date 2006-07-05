/* RatioBox container */

#ifndef RATIO_BOX_H
#define RATIO_BOX_H

#include <gdk/gdk.h>
#include <gtk/gtkcontainer.h>


#define TYPE_RATIO_BOX            (YGtk::ratio_box_get_type ())
#define RATIO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                   TYPE_RATIO_BOX, YGtk::RatioBox))
#define RATIO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                   TYPE_RATIO_BOX, YGtk::RatioBoxClass))
#define IS_RATIO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                   TYPE_RATIO_BOX))
#define IS_RATIO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                   TYPE_RATIO_BOX))
#define RATIO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                   TYPE_RATIO_BOX, YGtk::RatioBoxClass))

namespace YGtk
{

typedef struct _RatioBox	     RatioBox;
typedef struct _RatioBoxClass  RatioBoxClass;
typedef struct _RatioBoxChild  RatioBoxChild;

struct _RatioBox
{
	GtkContainer container;

	/*< public (read-only) >*/
	GList *children;
	gint16 spacing;

  /*< private >*/
  guint orientation : 1;
  gfloat ratios_sum;
};

struct _RatioBoxClass
{
  GtkContainerClass parent_class;
};

struct _RatioBoxChild
{
  GtkWidget *widget;
  // Proprieties
  guint16 padding;
  guint fill : 1;
  gfloat ratio;
};

typedef enum RatioBoxOrientation {
	HORIZONTAL_RATIO_BOX_ORIENTATION = 0,
	VERTICAL_RATIO_BOX_ORIENTATION   = 1
};

GtkWidget* ratio_box_new (RatioBoxOrientation orientation, gint spacing);
GType ratio_box_get_type (void) G_GNUC_CONST;

void ratio_box_set_spacing (RatioBox *box, gint spacing);
gint ratio_box_get_spacing (RatioBox *box);

void ratio_box_query_child_packing (RatioBox *box, GtkWidget *child,
                                    gfloat *ratio, gboolean *fill,
                                    guint *padding);
void ratio_box_set_child_packing (RatioBox *box, GtkWidget *child, gfloat ratio,
                                  gboolean fill, guint padding);

}; /*namespace YGtk*/

#endif /* RATIO_BOX_H */
