/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkRatioBox uses weights instead of the single state expand boolean.
*/

#ifndef YGTK_RATIO_BOX_H
#define YGTK_RATIO_BOX_H

#include <gtk/gtkcontainer.h>
G_BEGIN_DECLS

#define YGTK_TYPE_RATIO_BOX            (ygtk_ratio_box_get_type ())
#define YGTK_RATIO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                        YGTK_TYPE_RATIO_BOX, YGtkRatioBox))
#define YGTK_RATIO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                        YGTK_TYPE_RATIO_BOX, YGtkRatioBoxClass))
#define YGTK_IS_RATIO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                        YGTK_TYPE_RATIO_BOX))
#define YGTK_IS_RATIO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                        YGTK_TYPE_RATIO_BOX))
#define YGTK_RATIO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                        YGTK_TYPE_RATIO_BOX, YGtkRatioBoxClass))

typedef struct _YGtkRatioBox
{
	GtkContainer parent;

	// private (read-only):
	GList *children;
	guint spacing;
} YGtkRatioBox;

typedef struct _YGtkRatioBoxClass
{
	GtkContainerClass parent_class;
} YGtkRatioBoxClass;

typedef struct _YGtkRatioBoxChild
{
	GtkWidget *widget;
	// members
	gfloat ratio;
} YGtkRatioBoxChild;

GType ygtk_ratio_box_get_type (void) G_GNUC_CONST;

void ygtk_ratio_box_set_spacing (YGtkRatioBox *box, guint spacing);

void ygtk_ratio_box_pack (YGtkRatioBox *box, GtkWidget *child, gfloat ratio);
void ygtk_ratio_box_set_child_packing (YGtkRatioBox *box, GtkWidget *child, gfloat ratio);

/* RatioHBox */

#define YGTK_TYPE_RATIO_HBOX            (ygtk_ratio_hbox_get_type ())
#define YGTK_RATIO_HBOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                        YGTK_TYPE_RATIO_HBOX, YGtkRatioHBox))
#define YGTK_RATIO_HBOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                        YGTK_TYPE_RATIO_HBOX, YGtkRatioHBoxClass))
#define YGTK_IS_RATIO_HBOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                        YGTK_TYPE_RATIO_HBOX))
#define YGTK_IS_RATIO_HBOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                        YGTK_TYPE_RATIO_HBOX))
#define YGTK_RATIO_HBOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                        YGTK_TYPE_RATIO_HBOX, YGtkRatioHBoxClass))

typedef struct _YGtkRatioHBox
{
	YGtkRatioBox parent;
} YGtkRatioHBox;

typedef struct _YGtkRatioHBoxClass
{
	YGtkRatioBoxClass parent_class;
} YGtkRatioHBoxClass;

GtkWidget* ygtk_ratio_hbox_new (gint spacing);
GType ygtk_ratio_hbox_get_type (void) G_GNUC_CONST;

/* RatioVBox */

#define YGTK_TYPE_RATIO_VBOX            (ygtk_ratio_vbox_get_type ())
#define YGTK_RATIO_VBOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                        YGTK_TYPE_RATIO_VBOX, YGtkRatioVBox))
#define YGTK_RATIO_VBOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                        YGTK_TYPE_RATIO_VBOX, YGtkRatioVBoxClass))
#define YGTK_IS_RATIO_VBOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                        YGTK_TYPE_RATIO_VBOX))
#define YGTK_IS_RATIO_VBOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                        YGTK_TYPE_RATIO_VBOX))
#define YGTK_RATIO_VBOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                        YGTK_TYPE_RATIO_VBOX, YGtkRatioVBoxClass))

typedef struct _YGtkRatioVBox
{
	YGtkRatioBox parent;
} YGtkRatioVBox;

typedef struct _YGtkRatioVBoxClass
{
	YGtkRatioBoxClass parent_class;
} YGtkRatioVBoxClass;

GtkWidget* ygtk_ratio_vbox_new (gint spacing);
GType ygtk_ratio_vbox_get_type (void) G_GNUC_CONST;

G_END_DECLS
#endif /* YGTK_RATIO_BOX_H */

/* YGtkAdjSize container allows for better fine grained size constrains. */
#ifndef YGTK_ADJ_SIZE_H
#define YGTK_ADJ_SIZE_H

#include <gtk/gtkbin.h>
G_BEGIN_DECLS

#define YGTK_TYPE_ADJ_SIZE            (ygtk_adj_size_get_type ())
#define YGTK_ADJ_SIZE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                       YGTK_TYPE_ADJ_SIZE, YGtkAdjSize))
#define YGTK_ADJ_SIZE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                       YGTK_TYPE_ADJ_SIZE, YGtkAdjSizeClass))
#define YGTK_IS_ADJ_SIZE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                       YGTK_TYPE_ADJ_SIZE))
#define YGTK_IS_ADJ_SIZE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                       YGTK_TYPE_ADJ_SIZE))
#define YGTK_ADJ_SIZE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                       YGTK_TYPE_ADJ_SIZE, YGtkAdjSizeClass))

typedef void (*LimitSizeCb) (guint *width, guint *height, gpointer data);

typedef struct _YGtkAdjSize
{
	GtkBin parent;
	// members
	guint min_width, min_height, max_width, max_height;
	LimitSizeCb min_size_cb;
	gpointer min_size_data;
	gint only_expand : 2;
} YGtkAdjSize;

typedef struct _YGtkAdjSizeClass
{
	GtkBinClass parent_class;
} YGtkAdjSizeClass;

GType ygtk_adj_size_get_type (void) G_GNUC_CONST;
GtkWidget* ygtk_adj_size_new (void);

void ygtk_adj_size_set_min (YGtkAdjSize *adj_size, guint min_width, guint min_height);
void ygtk_adj_size_set_max (YGtkAdjSize *adj_size, guint max_width, guint max_height);

void ygtk_adj_size_set_min_cb (YGtkAdjSize *adj_size, LimitSizeCb min_size_cb, gpointer data);

/* Only allow the child to grow (ie. to ask for bigger sizes). */
void ygtk_adj_size_set_only_expand (YGtkAdjSize *adj_size, gboolean only_expand);

G_END_DECLS
#endif /*YGTK_ADJ_SIZE_H*/

