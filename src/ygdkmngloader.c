/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGdkMngLoader image loader */
// check the header file for information about this loader

#include "ygdkmngloader.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

/* A few chunks ids */
#define MNG_UINT_MHDR 0x4d484452L
#define MNG_UINT_BACK 0x4241434bL
#define MNG_UINT_PLTE 0x504c5445L
#define MNG_UINT_tRNS 0x74524e53L
#define MNG_UINT_IHDR 0x49484452L
#define MNG_UINT_IDAT 0x49444154L
#define MNG_UINT_IEND 0x49454e44L
#define MNG_UINT_MEND 0x4d454e44L
#define MNG_UINT_FRAM 0x4652414dL
#define MNG_UINT_LOOP 0x4c4f4f50L
#define MNG_UINT_ENDL 0x454e444cL
#define MNG_UINT_TERM 0x5445524dL

//** Utilities to read the MNG file

typedef struct DataStream {
	const guint8 *data;
	const long size;
	long offset;
} DataStream;
static DataStream data_stream_constructor (const guint8 *raw_data, long size)
{
	DataStream data = { raw_data, size, 0 };
	return data;
}

static gboolean read_signature (DataStream *data)
{
	if (data->offset+8 > data->size)
		return FALSE;
	data->offset += 8;
	return memcmp (data->data + data->offset-8, "\212MNG\r\n\032\n", 8) == 0;
}

static gboolean read_uint8 (DataStream *data, guint8 *value)
{
	if (data->offset+1 > data->size)
		return FALSE;
	*value = data->data [data->offset++];
	return TRUE;
}

static gboolean read_uint32 (DataStream *data, guint32 *value)
{
	if (data->offset+4 > data->size)
		return FALSE;
	*value  = data->data[data->offset+0] << 24;
	*value |= data->data[data->offset+1] << 16;
	*value |= data->data[data->offset+2] << 8;
	*value |= data->data[data->offset+3];
	data->offset += 4;
	return TRUE;
}

static gboolean read_data (DataStream *data, guint32 size, GdkPixbufLoader *loader,
                           GError **error)
{
	if (data->offset+size > data->size)
	{
		g_set_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
		             "Unexpected end of file when reading PNG chunk");
		return FALSE;
	}
	gboolean ret;
	ret = gdk_pixbuf_loader_write (loader, data->data + data->offset, size, error);
	data->offset += size;
	return ret;
}

static void image_prepared_cb (GdkPixbufLoader *loader, YGdkMngPixbuf *mng_pixbuf)
{
	GdkPixbuf *pix = gdk_pixbuf_loader_get_pixbuf (loader);
	mng_pixbuf->frames = g_list_append (mng_pixbuf->frames, pix);
}

//** YGdkMngPixbuf

G_DEFINE_TYPE (YGdkMngPixbuf, ygdk_mng_pixbuf, GDK_TYPE_PIXBUF_ANIMATION)

static void ygdk_mng_pixbuf_init (YGdkMngPixbuf *pixbuf)
{
}

gboolean ygdk_mng_pixbuf_is_file_mng (const gchar *filename)
{
	FILE *file = fopen (filename, "rb");
	if (!file)
		goto is_file_mng_failed;

	guchar raw_data [8];
	if (fread (raw_data, 1, 8, file) < 8)
		goto is_file_mng_failed;

	gboolean ret = ygdk_mng_pixbuf_is_data_mng (raw_data, 8);
	fclose (file);
	return ret;

is_file_mng_failed:
	if (file)
		fclose (file);
	return FALSE;
}

gboolean ygdk_mng_pixbuf_is_data_mng (const guint8 *raw_data, long size)
{
	DataStream data = data_stream_constructor (raw_data, size);
	return read_signature (&data);
}

#define SET_ERROR(msg) { error = TRUE; \
	g_set_error (error_msg, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_CORRUPT_IMAGE, msg); }

GdkPixbufAnimation *ygdk_mng_pixbuf_new_from_file (const gchar *filename,
                                                   GError **error_msg)
{
	gboolean error = FALSE;
	FILE *file = fopen (filename, "rb");
	if (!file) {
		SET_ERROR ("Could not open specified file")
		return NULL;
	}

	fseek (file, 0, SEEK_END);
	long file_size = ftell (file);
	fseek (file, 0, SEEK_SET);

	GdkPixbufAnimation *mng_pixbuf = 0;
	guchar *data = mmap (NULL, file_size, PROT_READ, MAP_PRIVATE,
	                     fileno (file), 0);
	if (data == MAP_FAILED)
		SET_ERROR ("Could not map file")
	else {
		mng_pixbuf = ygdk_mng_pixbuf_new_from_data (data, file_size, error_msg);
		munmap (data, file_size);
	}
	fclose (file);
	return mng_pixbuf;
}

GdkPixbufAnimation *ygdk_mng_pixbuf_new_from_data (const guint8 *raw_data, long size,
                                                   GError **error_msg)
{
	DataStream data = data_stream_constructor (raw_data, size);

	gboolean error = FALSE;
	if (!read_signature (&data)) {
		SET_ERROR ("Not a MNG file")
		return NULL;
	}

	YGdkMngPixbuf *mng_pixbuf = g_object_new (YGDK_TYPE_MNG_PIXBUF, NULL);
	mng_pixbuf->iteration_max = 0x7fffffff;

	guint32 chunk_size, chunk_id;
	long chunk_offset;
	GdkPixbufLoader *loader = NULL;  /* currently loading... */
	gboolean first_read = TRUE;

    do {
		error = !read_uint32 (&data, &chunk_size);
		error = error || !read_uint32 (&data, &chunk_id);
		if (error) {
			SET_ERROR ("Unexpected end of file on new chunk")
			break;
		}
		chunk_offset = data.offset + chunk_size + 4/*CRC*/;

		if (first_read && chunk_id != MNG_UINT_MHDR)
		{
			SET_ERROR ("MHDR chunk must come first")
			break;
		}

		// not currently reading a PNG stream 
		if (!loader)
		{
			switch (chunk_id)
			{
				case MNG_UINT_MHDR:
					if (!first_read)
					{
						SET_ERROR ("Only one MHDR chunk allowed")
						break;
					}

					if (chunk_size == 7*4)
					{
						// Read MHDR chunk data
						error = !read_uint32 (&data, &mng_pixbuf->frame_width);
						error = error || !read_uint32 (&data, &mng_pixbuf->frame_height);
						error = error || !read_uint32 (&data, &mng_pixbuf->ticks_per_second);
						if (error)
							SET_ERROR ("Unexpected end of file on MHDR chunk")
						/* Next atttributes: Nominal_layer_count, Nominal_frame_count,
							                 Nominal_play_time, Simplicity_profile */
						else if (mng_pixbuf->frame_width <= 0 ||
							     mng_pixbuf->frame_height <= 0 ||
							     mng_pixbuf->ticks_per_second < 0)
							SET_ERROR ("Invalid MHDR parameter")
//fprintf(stderr, "ticks per second: %d\n", mng_pixbuf->ticks_per_second);
					}
					else
						SET_ERROR ("MHDR chunk must be 28 bytes long")
					break;
				case MNG_UINT_IHDR:
					loader = gdk_pixbuf_loader_new_with_type ("png", NULL);
					g_signal_connect (G_OBJECT (loader), "area-prepared",
					                  G_CALLBACK (image_prepared_cb), mng_pixbuf);
					gdk_pixbuf_loader_set_size (loader, mng_pixbuf->frame_width,
					                            mng_pixbuf->frame_height);

					{
						const guchar sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
						if(!gdk_pixbuf_loader_write (loader, sig, 8, error_msg))
						{
							error = TRUE;
							break;
						}
					}
					break;
				case MNG_UINT_TERM:
					if (chunk_size > 1)
					{
						// Read TERM chunk data
						guint8 t = 0;
						error = !read_uint8 (&data, &t);  // term action
						if (t == 3 && chunk_size == 2+8)
						{
							error = error || !read_uint8 (&data, &t);  // after term action
							error = error || !read_uint32 (&data, &mng_pixbuf->last_frame_delay);
							error = error || !read_uint32 (&data, &mng_pixbuf->iteration_max);
							if (error)
								SET_ERROR ("Unexpected end of file on TERM chunk")
						}
						else
							SET_ERROR ("TERM chunk must be 10 bytes when term action is 3")
					}
					else
						SET_ERROR ("TERM chunk must have at least 1 byte")
					break;
				case MNG_UINT_BACK:
					// TODO:
					break;
				case MNG_UINT_IDAT:
				case MNG_UINT_IEND:
					if (loader != NULL)
						SET_ERROR ("Corrupted PNG chunk closures")
					break;
				case MNG_UINT_MEND:
				default:
					break;
			}
		}

		if (error)
			break;

		// loading a PNG stream
		if (loader)
		{
			data.offset -= 8;
			if (!read_data (&data, chunk_size+8+4, loader, error_msg))
				error = TRUE;
			else if (chunk_id == MNG_UINT_IEND)
			{
				if (!gdk_pixbuf_loader_close (loader, error_msg))
				{
					error = TRUE;
					break;
				}
				loader = NULL;
			}
		}

		data.offset = chunk_offset;
		first_read = FALSE;
    } while (chunk_id != MNG_UINT_MEND && !error);

	if (error)
	{
		g_object_unref (G_OBJECT (mng_pixbuf));
		return NULL;
	}
    return GDK_PIXBUF_ANIMATION (mng_pixbuf);
}

#undef SET_ERROR

static gboolean ygdk_mng_pixbuf_is_static_image (GdkPixbufAnimation *anim)
{
	YGdkMngPixbuf *mng_anim = YGDK_MNG_PIXBUF (anim);
	return g_list_length (mng_anim->frames) == 1;
}

static GdkPixbuf *ygdk_mng_pixbuf_get_static_image (GdkPixbufAnimation *anim)
{
	YGdkMngPixbuf *mng_anim = YGDK_MNG_PIXBUF (anim);
	return g_list_nth_data (mng_anim->frames, 0);
}

static void ygdk_mng_pixbuf_get_size (GdkPixbufAnimation *anim, int *width, int *height)
{
	YGdkMngPixbuf *mng_anim = YGDK_MNG_PIXBUF (anim);
	if (width) *width = mng_anim->frame_width;
	if (height) *height = mng_anim->frame_height;
}

static GdkPixbufAnimationIter *ygdk_mng_pixbuf_get_iter (GdkPixbufAnimation *anim,
                                                         const GTimeVal     *start_time)
{
	YGdkMngPixbufIter *iter = g_object_new (YGDK_TYPE_MNG_PIXBUF_ITER, NULL);
	iter->mng_pixbuf = YGDK_MNG_PIXBUF( anim );
	iter->cur_frame = 0;
	return GDK_PIXBUF_ANIMATION_ITER( iter );
}

static void ygdk_mng_pixbuf_class_init (YGdkMngPixbufClass *klass)
{
	ygdk_mng_pixbuf_parent_class = g_type_class_peek_parent (klass);

	GdkPixbufAnimationClass *pixbuf_class = GDK_PIXBUF_ANIMATION_CLASS (klass);
	pixbuf_class->is_static_image  = ygdk_mng_pixbuf_is_static_image;
	pixbuf_class->get_static_image  = ygdk_mng_pixbuf_get_static_image;
	pixbuf_class->get_size  = ygdk_mng_pixbuf_get_size;
	pixbuf_class->get_iter  = ygdk_mng_pixbuf_get_iter;
}

//** YGdkMngPixbufIter

G_DEFINE_TYPE (YGdkMngPixbufIter, ygdk_mng_pixbuf_iter, GDK_TYPE_PIXBUF_ANIMATION_ITER)

static void ygdk_mng_pixbuf_iter_init (YGdkMngPixbufIter *iter)
{
}

static GdkPixbuf *ygdk_mng_pixbuf_iter_get_pixbuf (GdkPixbufAnimationIter *iter)
{
	YGdkMngPixbufIter *mng_iter = YGDK_MNG_PIXBUF_ITER (iter);
	return g_list_nth_data (mng_iter->mng_pixbuf->frames, mng_iter->cur_frame);	
}

static gboolean ygdk_mng_pixbuf_iter_on_currently_loading_frame (GdkPixbufAnimationIter *iter)
{
	return FALSE;
}

static int ygdk_mng_pixbuf_iter_get_delay_time (GdkPixbufAnimationIter *iter)
{
	YGdkMngPixbufIter *mng_iter = YGDK_MNG_PIXBUF_ITER (iter);
	int delay = 1000.0 / mng_iter->mng_pixbuf->ticks_per_second;
	if (mng_iter->cur_frame == g_list_length (mng_iter->mng_pixbuf->frames)-1)
		delay += mng_iter->mng_pixbuf->last_frame_delay;
	return delay;
}

static gboolean ygdk_mng_pixbuf_iter_advance (GdkPixbufAnimationIter *iter,
                                              const GTimeVal         *current_time)
{
	YGdkMngPixbufIter *mng_iter = YGDK_MNG_PIXBUF_ITER (iter);
	YGdkMngPixbuf *mng_pixbuf = mng_iter->mng_pixbuf;
	if (!mng_pixbuf->frames)
		return FALSE;

	gboolean can_advance = TRUE;
	int frames_len = g_list_length (mng_pixbuf->frames);
	if (mng_iter->cur_frame+1 == frames_len)
	{
		if (mng_pixbuf->iteration_max == 0x7fffffff || 
		    mng_iter->cur_iteration < mng_pixbuf->iteration_max)
			mng_iter->cur_iteration++;
		else
			can_advance = FALSE;
	}

	if (can_advance)
		mng_iter->cur_frame = (mng_iter->cur_frame+1) % frames_len;
	return can_advance;
}

static void ygdk_mng_pixbuf_iter_class_init (YGdkMngPixbufIterClass *klass)
{
	ygdk_mng_pixbuf_iter_parent_class = g_type_class_peek_parent (klass);

	GdkPixbufAnimationIterClass *iter_class = GDK_PIXBUF_ANIMATION_ITER_CLASS (klass);
	iter_class->get_delay_time = ygdk_mng_pixbuf_iter_get_delay_time;
	iter_class->get_pixbuf = ygdk_mng_pixbuf_iter_get_pixbuf;
	iter_class->on_currently_loading_frame = ygdk_mng_pixbuf_iter_on_currently_loading_frame;
	iter_class->advance = ygdk_mng_pixbuf_iter_advance;
}

