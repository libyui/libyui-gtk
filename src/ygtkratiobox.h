/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkRatioBox is an improvement over the GtkBox container that
   allows the programmer to set stretch weights to containees.

   It is similar to GtkBox in usage but instead of feeding an
   expand boolean, a weight is given instead. In fact, it should
   behave just the same as a GtkBox if you give 0 for not expand
   and 1 to.
*/
/*
   Quirks (cause of yast-core):
     if one of the children has certain special properties, then only widgets with
     the same properties will be expanded. These are, by order:
       both stretch and rubber-band
       both stretch and weight
       weight
       stretch

    We use different naming:
      stretch = expand
      rubber-band or layout-stretch = must expand
      weight = ratio
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
	gint16 spacing;
	guint weight_length;  // min-length for weight widgets
	guint has_must_expand : 1;  // cache
} YGtkRatioBox;

typedef struct _YGtkRatioBoxClass
{
	GtkContainerClass parent_class;
} YGtkRatioBoxClass;

typedef struct _YGtkRatioBoxChild
{
	GtkWidget *widget;
	// members
	guint16 padding;
	gfloat ratio;
	guint xfill : 1;
	guint yfill : 1;
	guint expand : 1;  // twilight zone flag; use ratio
	guint must_expand : 1;  // higher order expand
} YGtkRatioBoxChild;

GType ygtk_ratio_box_get_type (void) G_GNUC_CONST;

void ygtk_ratio_box_set_spacing (YGtkRatioBox *box, gint spacing);

void ygtk_ratio_box_pack (YGtkRatioBox *box, GtkWidget *child, gfloat ratio,
                          gboolean xfill, gboolean yfill, guint padding);

// You can either use ratios or the expand boolean. If some child has a ratio set (to
// a value other than 0), then expand flags will be ignored -- use must_expand in such
// cases, when you want a child to expand like the child of the most ratio.
// (rules based on those of libyui -- check top)
void ygtk_ratio_box_set_child_packing (YGtkRatioBox *box, GtkWidget *child,
	gboolean expand, gboolean must_expand, gfloat ratio, gboolean xfill, gboolean yfill,
	guint padding);

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

typedef struct _YGtkAdjSize
{
	GtkBin parent;
	// members
	guint min_width, min_height, max_width, max_height;
	gboolean only_expand;
} YGtkAdjSize;

typedef struct _YGtkAdjSizeClass
{
	GtkBinClass parent_class;
} YGtkAdjSizeClass;

GType ygtk_adj_size_get_type (void) G_GNUC_CONST;
GtkWidget* ygtk_adj_size_new (void);

void ygtk_adj_size_set_min (YGtkAdjSize *adj_size, guint min_width, guint min_height);
void ygtk_adj_size_set_max (YGtkAdjSize *adj_size, guint max_width, guint max_height);

/* Only allow the child to grow (ie. to ask for bigger sizes). */
void ygtk_adj_size_set_only_expand (YGtkAdjSize *adj_size, gboolean only_expand);

G_END_DECLS
#endif /*YGTK_ADJ_SIZE_H*/

/* YGtkScrolledWindow gives some a more fine-grained automatic scroll policy.
   It allows the user to specify from which size scrolling should be applied. */
#ifndef YGTK_TUNED_SCROLLED_WIDOW_H
#define YGTK_TUNED_SCROLLED_WIDOW_H

#include <gtk/gtkscrolledwindow.h>
G_BEGIN_DECLS

#define YGTK_TYPE_TUNED_SCROLLED_WINDOW            (ygtk_tuned_scrolled_window_get_type ())
#define YGTK_TUNED_SCROLLED_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	YGTK_TYPE_TUNED_SCROLLED_WINDOW, YGtkTunedScrolledWindow))
#define YGTK_TUNED_SCROLLED_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
	YGTK_TYPE_TUNED_SCROLLED_WINDOW, YGtkTunedScrolledWindowClass))
#define YGTK_IS_TUNED_SCROLLED_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                              YGTK_TYPE_TUNED_SCROLLED_WINDOW))
#define YGTK_IS_TUNED_SCROLLED_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                              YGTK_TYPE_TUNED_SCROLLED_WINDOW))
#define YGTK_TUNED_SCROLLED_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
	YGTK_TYPE_TUNED_SCROLLED_WINDOW, YGtkTunedScrolledWindowClass))

typedef struct _YGtkTunedScrolledWindow
{
	GtkScrolledWindow parent;
	// members
	guint max_width, max_height;
} YGtkTunedScrolledWindow;

typedef struct _YGtkTunedScrolledWindowClass
{
	GtkScrolledWindowClass parent_class;
} YGtkTunedScrolledWindowClass;

GType ygtk_tuned_scrolled_window_get_type (void) G_GNUC_CONST;
GtkWidget* ygtk_tuned_scrolled_window_new (GtkWidget *child /*or NULL*/);

void ygtk_tuned_scrolled_window_set_auto_policy (YGtkTunedScrolledWindow *scroll,
                                                 guint max_width, guint max_height);

G_END_DECLS
#endif /*YGTK_TUNED_SCROLLED_WIDOW_H*/

