/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* GtkImage doesn't provide all the functionality asked by libyui,
   such as scaling and tiling. Thus, YGtkImage is a more powerful
   GtkImage.
*/

#ifndef YGTK_IMAGE_H
#define YGTK_IMAGE_H

#include <gtk/gtkdrawingarea.h>
#include <gdk/gdkpixbuf.h>

G_BEGIN_DECLS

#define YGTK_TYPE_IMAGE            (ygtk_image_get_type ())
#define YGTK_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                    YGTK_TYPE_IMAGE, YGtkImage))
#define YGTK_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                    YGTK_TYPE_IMAGE, YGtkImageClass))
#define YGTK_IS_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                    YGTK_TYPE_IMAGE))
#define YGTK_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                    YGTK_TYPE_IMAGE))
#define YGTK_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                    YGTK_TYPE_IMAGE, YGtkImageClass))

typedef enum {
	CENTER_IMAGE_ALIGN, SCALE_IMAGE_ALIGN, TILE_IMAGE_ALIGN
} YGtkImageAlign;

struct _YGtkImageAnimation {
	GdkPixbufAnimation *pixbuf;
	GdkPixbufAnimationIter *frame;
	guint timeout_id;
};

typedef struct _YGtkImage
{
	GtkDrawingArea parent;

	// properties:
	YGtkImageAlign align;

	gboolean animated;
	union {
		GdkPixbuf *pixbuf;
		struct _YGtkImageAnimation *animation;
	};

	gboolean loaded;
	gchar *alt_text;

} YGtkImage;

typedef struct _YGtkImageClass
{
	GtkDrawingAreaClass parent_class;
} YGtkImageClass;

GtkWidget* ygtk_image_new (void);
GType ygtk_image_get_type (void) G_GNUC_CONST;

void ygtk_image_set_from_file (YGtkImage *image, const char *filename, gboolean anim);
void ygtk_image_set_from_data (YGtkImage *image, const guint8 *data, long size, gboolean anim);
void ygtk_image_set_props (YGtkImage *image, YGtkImageAlign align, const gchar *alt_text);

// as we don't have a window, 

G_END_DECLS

#endif /* YGTK_IMAGE_H */

