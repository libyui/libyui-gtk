//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YImage.h"
#include "YGWidget.h"

class YGImage : public YImage, public YGWidget
{
	bool m_hasZeroWidth, m_hasZeroHeight, m_isScaled, m_isTiled, m_isAnimation;
	bool m_imageLoaded;

	struct Animation {
		GdkPixbufAnimation *pixbuf;
		GdkPixbufAnimationIter *frame;
		guint timeout_id;
	};
	union {
		GdkPixbuf *m_pixbuf;
		Animation *m_animation;
	};
	gchar *alt_text;

	void initOptions (const YWidgetOpt &opt)
	{
		IMPL
		m_imageLoaded   = false;
		m_hasZeroWidth  = opt.zeroWidth.value();
		m_hasZeroHeight = opt.zeroHeight.value();

if (m_hasZeroWidth || m_isScaled || m_isTiled)
	YGWidget::setStretchable (YD_HORIZ, true);
if (m_hasZeroHeight || m_isScaled || m_isTiled)
	YGWidget::setStretchable (YD_VERT, true);


		m_isAnimation   = opt.animated.value();
		m_isScaled      = opt.scaleToFit.value();
		m_isTiled       = opt.tiled.value();
		if (m_isScaled && m_isTiled) {
			y2warning ("YImage can't be scaled and tiled at the same time");
			m_isTiled = false;
		}

		if (m_isAnimation)
			m_animation = NULL;
		else
			m_pixbuf = NULL;

		g_signal_connect (G_OBJECT (getWidget()), "expose-event",
		                  G_CALLBACK (expose_event_cb), this);
		gtk_widget_queue_draw (getWidget());
	}

	void loadImage (GdkPixbuf *pixbuf, const char *error_msg)
	{
		IMPL
		if (pixbuf == NULL) {
			g_warning ("Couldn't load image - %s", error_msg);
			return;
		}

		m_imageLoaded = true;
		m_pixbuf = pixbuf;
		gtk_widget_set_size_request (getWidget(), gdk_pixbuf_get_width (pixbuf),
		                             gdk_pixbuf_get_height (pixbuf));
	}

	void loadAnimation (GdkPixbufAnimation *pixbuf, const char *error_msg)
	{
		IMPL
		if (pixbuf == NULL) {
			g_warning ("Couldn't load animation - %s", error_msg);
			return;
		}

		m_imageLoaded = true;
		m_animation = new Animation;
		m_animation->pixbuf = pixbuf;

		m_animation->frame = NULL;
		advance_frame_cb (this);

		g_signal_connect (G_OBJECT (getWidget()), "expose-event",
		                  G_CALLBACK (expose_event_cb), this);
		gtk_widget_queue_draw (getWidget());
	}

public:
	YGImage (const YWidgetOpt &opt, YGWidget *parent,
	         const YCPString &filename, const YCPString &text)
	: YImage (opt),
	  YGWidget (this, parent, true, GTK_TYPE_DRAWING_AREA, NULL)
	{
		IMPL
		alt_text = g_strdup (text->value_cstr());
		initOptions (opt);

		GError *error = 0;
		if (m_isAnimation) {
			GdkPixbufAnimation *pixbuf =
				gdk_pixbuf_animation_new_from_file (filename->value_cstr(), &error);
			loadAnimation (pixbuf, error ? error->message : "(undefined)");
		}
		else {
			GdkPixbuf *pixbuf =
				gdk_pixbuf_new_from_file (filename->value_cstr(), &error);
			loadImage (pixbuf, error ? error->message : "(undefined)");
		}
	}

	YGImage (const YWidgetOpt &opt, YGWidget *parent,
	         const YCPByteblock &byteblock, const YCPString &text)
	: YImage (opt),
	  YGWidget (this, parent, true, GTK_TYPE_DRAWING_AREA, NULL)
	{
		IMPL
		alt_text = g_strdup (text->value_cstr());
		initOptions (opt);

		GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
		g_signal_connect (G_OBJECT (loader), "area-prepared",
		                  G_CALLBACK (image_loaded_cb), this);

		GError *error = 0;
		if (!gdk_pixbuf_loader_write (loader,
		    byteblock->value(), byteblock->size(), &error))
			g_warning ("Could not load image from data blocks: %s", error->message);
		gdk_pixbuf_loader_close (loader, &error);
	}

	virtual ~YGImage()
	{
		IMPL
		if (m_imageLoaded) {
			if (m_isAnimation) {
				g_object_unref (G_OBJECT (m_animation->pixbuf));
				if (m_animation->timeout_id)
					g_source_remove (m_animation->timeout_id);
				delete m_animation;
			}
			else
				g_object_unref (G_OBJECT (m_pixbuf));
		}
		g_free (alt_text);
	}

	// callback for image loading
	static void image_loaded_cb (GdkPixbufLoader *loader, YGImage *pThis)
	{
		IMPL
		if (pThis->m_isAnimation) {
			if (pThis->m_animation) {
				// a new frame loaded -- just redraw the widget
				if (gdk_pixbuf_animation_iter_on_currently_loading_frame
				       (pThis->m_animation->frame))
					gtk_widget_queue_draw (pThis->getWidget());
			}
			else {
				GdkPixbufAnimation *pixbuf = gdk_pixbuf_loader_get_animation (loader);
				g_object_ref (G_OBJECT (pixbuf));
				pThis->loadAnimation (pixbuf, " on block data reading callback");
			}
		}
		else {
			GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
			g_object_ref (G_OBJECT (pixbuf));
			pThis->loadImage (pixbuf, " on block data reading callback");
		}
	}

	// callback for image displaying
	static gboolean advance_frame_cb (void *pData)
	{
		IMPL
		YGImage *pThis = (YGImage *) pData;
		Animation *animation = pThis->m_animation;

		if (!animation->frame)  // no frame yet loaded
			animation->frame = gdk_pixbuf_animation_get_iter (animation->pixbuf, NULL);
		else
			if (gdk_pixbuf_animation_iter_advance (animation->frame, NULL))
				gtk_widget_queue_draw (pThis->getWidget());

		// shedule next frame
		int delay = gdk_pixbuf_animation_iter_get_delay_time (animation->frame);
		if (delay != -1)
			animation->timeout_id = g_timeout_add (delay, advance_frame_cb, pThis);

		GdkPixbuf *pixbuf = gdk_pixbuf_animation_iter_get_pixbuf (pThis->m_animation->frame);
		gtk_widget_set_size_request (pThis->getWidget(), gdk_pixbuf_get_width (pixbuf),
		                             gdk_pixbuf_get_height (pixbuf));
		return FALSE;
	}

	static gboolean expose_event_cb (GtkWidget *widget, GdkEventExpose *event,
	                                 YGImage *pThis)
	{
		IMPL
		int x, y, width, height;
		x = widget->allocation.x + 6;
		y = widget->allocation.y + 6;
		width  = widget->allocation.width;
		height = widget->allocation.height;

		cairo_t *cr = gdk_cairo_create (widget->window);

		if (!pThis->m_imageLoaded) {
			// show default text if no image was loaded
			PangoLayout *layout;
			layout = gtk_widget_create_pango_layout (widget, pThis->alt_text);

			int text_width, text_height;
			pango_layout_get_size (layout, &text_width, &text_height);
			text_width /= PANGO_SCALE;
			text_height /= PANGO_SCALE;

			x += (width - text_width) / 2;
			y += (height - text_height) / 2;

			cairo_move_to (cr, x, y);
			pango_cairo_show_layout (cr, layout);

			g_object_unref (layout);
			cairo_destroy (cr);
			return TRUE;
		}

		GdkPixbuf *pixbuf;
		if (pThis->m_isAnimation)
			pixbuf = gdk_pixbuf_animation_iter_get_pixbuf (pThis->m_animation->frame);
		else
			pixbuf = pThis->m_pixbuf;
		gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);

		if (pThis->m_isTiled)
			cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);

		if (pThis->m_isScaled) {
			double scale_x = (double) gdk_pixbuf_get_width (pixbuf) / width;
			double scale_y = (double) gdk_pixbuf_get_height (pixbuf) / height;
			cairo_matrix_t matrix;
			cairo_matrix_init_scale (&matrix, scale_x, scale_y);
			cairo_pattern_set_matrix (cairo_get_source (cr), &matrix);
		}

		cairo_rectangle (cr, 0, 0, width, height);
		cairo_fill (cr);
		
		cairo_destroy (cr);
		return TRUE;
	}

	YGWIDGET_IMPL_COMMON
/*
	virtual bool stretchable (YUIDimension dim) const
	{
		IMPL
		if (m_isScaled)
			return true;
		return (dim == YD_HORIZ) ? m_hasZeroWidth : m_hasZeroHeight;
	}
*/
};

YWidget *
YGUI::createImage (YWidget *parent, YWidgetOpt &opt,
                   YCPByteblock image_data, YCPString default_text)
{
	IMPL
	return new YGImage (opt, YGWidget::get (parent), image_data, default_text);
}

YWidget *
YGUI::createImage (YWidget *parent, YWidgetOpt &opt,
                   YCPString file_name, YCPString default_text)
{
	IMPL
	return new YGImage (opt, YGWidget::get (parent), file_name, default_text);
}
