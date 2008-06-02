/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* GtkFixed just doesn't cut it... gtk_fixed_move_child() does a queue_resize
   and the all thing is just not quite appropriate... */

#ifndef YGTK_FIXED_H
#define YGTK_FIXED_H

#include <gtk/gtkcontainer.h>
G_BEGIN_DECLS

#define YGTK_TYPE_FIXED            (ygtk_fixed_get_type ())
#define YGTK_FIXED(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                    YGTK_TYPE_FIXED, YGtkFixed))
#define YGTK_FIXED_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                    YGTK_TYPE_FIXED, YGtkFixedClass))
#define YGTK_IS_FIXED(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                    YGTK_TYPE_FIXED))
#define YGTK_IS_FIXED_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                    YGTK_TYPE_FIXED))
#define YGTK_FIXED_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                    YGTK_TYPE_FIXED, YGtkFixedClass))

typedef struct _YGtkFixed YGtkFixed;
typedef struct _YGtkFixedClass YGtkFixedClass;

typedef void (*YGtkPreferredSize) (YGtkFixed *, gint *, gint *, gpointer);
typedef void (*YGtkSetSize) (YGtkFixed *, gint, gint, gpointer);

struct _YGtkFixed
{
	GtkContainer parent;
	// private (read-only):
	GSList *children;
	YGtkPreferredSize preferred_size_cb;
	YGtkSetSize set_size_cb;
	gpointer data;
};

struct _YGtkFixedClass
{
	GtkContainerClass parent_class;
};

typedef struct _YGtkFixedChild
{
	GtkWidget *widget;
	// members
	// post-pone all position and size setting, to avoid unnecessary work
	gint x, y, width, height;
} YGtkFixedChild;

GType ygtk_fixed_get_type (void) G_GNUC_CONST;

void ygtk_fixed_setup (YGtkFixed *fixed, YGtkPreferredSize cb1, YGtkSetSize cb2, gpointer data);

void ygtk_fixed_set_child_pos (YGtkFixed *fixed, GtkWidget *widget, gint x, gint y);
void ygtk_fixed_set_child_size (YGtkFixed *fixed, GtkWidget *widget, gint width, gint height);

G_END_DECLS
#endif /*YGTK_FIXED_H*/

