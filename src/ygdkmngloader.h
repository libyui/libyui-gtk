/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* GdkPixbuf doesn't support MNG files... MNG extends PNG adding movie playback
   capabilities.
   This code aims for MNG-VLC (smallest subset of the format), though
   it isn't completely compliable.
*/

#ifndef YGDK_MNG_PIXBUF_H
#define YGDK_MNG_PIXBUF_H

#define GDK_PIXBUF_ENABLE_BACKEND
#include <gdk-pixbuf/gdk-pixbuf-animation.h>
G_BEGIN_DECLS

#define YGDK_TYPE_MNG_PIXBUF            (ygdk_mng_pixbuf_get_type ())
#define YGDK_MNG_PIXBUF(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                         YGDK_TYPE_MNG_PIXBUF, YGdkMngPixbuf))
#define YGDK_MNG_PIXBUF_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                         YGDK_TYPE_MNG_PIXBUF, YGdkMngPixbufClass))
#define YGDK_IS_MNG_PIXBUF(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                         YGDK_TYPE_MNG_PIXBUF))
#define YGDK_IS_MNG_PIXBUF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                         YGDK_TYPE_MNG_PIXBUF))
#define YGDK_MNG_PIXBUF_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                         YGK_TYPE_MNG_PIXBUF, YGdkMngPixbufClass))

typedef struct YGdkMngPixbuf
{
	GdkPixbufAnimation parent;

	// private: (use GdkPixbufAnimation API)
	GList *frames;  // of GdkPixbufs
	// MHDR header
	guint32 frame_width, frame_height, ticks_per_second;
	// TERM header
	guint32 last_frame_delay, iteration_max;
} YGdkMngPixbuf;

typedef struct YGdkMngPixbufClass
{
	GdkPixbufAnimationClass parent_class;
} YGdkMngPixbufClass;

GdkPixbufAnimation *ygdk_mng_pixbuf_new_from_file (const gchar *filename, GError **error_msg);
gboolean ygdk_mng_pixbuf_is_file_mng (const gchar *filename);

GType ygdk_mng_pixbuf_get_type (void) G_GNUC_CONST;

#define YGDK_TYPE_MNG_PIXBUF_ITER            (ygdk_mng_pixbuf_iter_get_type ())
#define YGDK_MNG_PIXBUF_ITER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                         YGDK_TYPE_MNG_PIXBUF_ITER, YGdkMngPixbufIter))
#define YGDK_MNG_PIXBUF_ITER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), YGDK_TYPE_MNG_PIXBUF_ITER, YGdkMngPixbufIterClass))
#define YGDK_IS_MNG_PIXBUF_ITER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), YGDK_TYPE_MNG_PIXBUF_ITER))
#define YGDK_MNG_PIXBUF_ITER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), YGDK_TYPE_MNG_PIXBUF_ITER, YGdkMngPixbufIterClass))

typedef struct YGdkMngPixbufIter {
        GdkPixbufAnimationIter parent;

		// private:
		YGdkMngPixbuf *mng_pixbuf;
        int cur_frame, cur_iteration;
} YGdkMngPixbufIter;

typedef struct YGdkMngPixbufIterClass {
        GdkPixbufAnimationIterClass parent_class;
} YGdkMngPixbufIterClass;

GType ygdk_mng_pixbuf_iter_get_type (void) G_GNUC_CONST;

G_END_DECLS
#endif /*YGDK_MNG_PIXBUF_H*/

