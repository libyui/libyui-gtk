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
	union {
		GdkPixbuf *m_pixbuf;
		GdkPixbufAnimation *m_animated_pixbuf;
	};

	void initOptions (const YWidgetOpt &opt)
	{
		m_hasZeroWidth  = opt.zeroWidth.value();
		m_hasZeroHeight = opt.zeroHeight.value();
		m_isAnimation   = opt.animated.value();
		m_imageLoaded   = false;

		// TODO implement:
		m_isScaled     = opt.scaleToFit.value();
		m_isTiled      = opt.tiled.value();

		g_signal_connect (G_OBJECT (getWidget()), "expose-event",
		                  G_CALLBACK (expose_event_cb), this);
	}

	void loadImage (GdkPixbuf *pixbuf, const char *error_msg)
	{
printf ("load image\n");
		if (pixbuf == NULL) {
			g_warning ("Couldn't load image - %s", error_msg);
			return;
		}

		m_imageLoaded = true;
		m_pixbuf = pixbuf;
	}

	void loadAnimation (GdkPixbufAnimation *pixbuf, const char *error_msg)
	{
		if (pixbuf == NULL) {
			g_warning ("Couldn't load animation - %s", error_msg);
			return;
		}

		m_imageLoaded = true;
		m_animated_pixbuf = pixbuf;
	}

public:
	YGImage (const YWidgetOpt &opt, YGWidget *parent,
	         const YCPString &filename, const YCPString &text)
	: YImage (opt),
	  YGWidget (this, parent, true, GTK_TYPE_LABEL,
	            "label", text->value_cstr(), NULL)
	{
		initOptions (opt);

		GError *error = 0;
		if (m_isAnimation) {
			GdkPixbufAnimation *pixbuf =
				gdk_pixbuf_animation_new_from_file (filename->value_cstr(), &error);
			loadAnimation (pixbuf, error->message);
		}
		else {
			GdkPixbuf *pixbuf =
				gdk_pixbuf_new_from_file (filename->value_cstr(), &error);
			loadImage (pixbuf, error->message);
		}
	}

	YGImage (const YWidgetOpt &opt, YGWidget *parent,
	         const YCPByteblock &byteblock, const YCPString &text)
	: YImage (opt),
	  YGWidget (this, parent, true, GTK_TYPE_LABEL,
	            "label", text->value_cstr(), NULL)
	{
printf ("init options\n");
		initOptions (opt);

printf("creating loader\n");
		GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
printf ("connecting loader\n");
		g_signal_connect (G_OBJECT (loader), "area-prepared",
		                  G_CALLBACK (image_loaded_cb), this);

printf ("calling loader write\n");
		GError *error = 0;
		if (!gdk_pixbuf_loader_write (loader,
		    byteblock->value(), byteblock->size(), &error))
			g_warning ("Could not load image from data blocks: %s", error->message);
		gdk_pixbuf_loader_close (loader, &error);
printf ("image widget done\n");
	}

	virtual ~YGImage()
	{
		if (m_imageLoaded) {
			if (m_isAnimation)
				g_object_unref (G_OBJECT (m_animated_pixbuf));
			else
				g_object_unref (G_OBJECT (m_pixbuf));
		}
	}

	// YWidget
	virtual bool stretchable (YUIDimension dimension) const
		IMPL_RET(true)
	virtual long nicesize (YUIDimension dim)
	{
		IMPL
		if (!m_imageLoaded)
			return getNiceSize (dim);

		if (dim == YD_HORIZ && m_hasZeroWidth)  return 0;
		if (dim == YD_VERT  && m_hasZeroHeight) return 0;

		if (m_isAnimation) {
			if (dim == YD_HORIZ)
				return gdk_pixbuf_animation_get_width (m_animated_pixbuf);
			else
				return gdk_pixbuf_animation_get_height (m_animated_pixbuf);
		}
		else {
			if (dim == YD_HORIZ)
				return gdk_pixbuf_get_width (m_pixbuf);
			else
				return gdk_pixbuf_get_height (m_pixbuf);
		}

		return getNiceSize (dim);
	}
	YGWIDGET_IMPL_SET_SIZE
//	virtual void setSize (long newWidth, long newHeight)  // TODO: overload for scaling

	// callback for image loading
	static void image_loaded_cb (GdkPixbufLoader *loader, YGImage *pThis)
	{
printf("image loaded cb\n");
		if (pThis->m_isAnimation) {
			GdkPixbufAnimation *pixbuf = gdk_pixbuf_loader_get_animation (loader);
			g_object_ref (G_OBJECT (pixbuf));
			pThis->loadAnimation (pixbuf, " on block data reading callback");
		}
		else {
			GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
			g_object_ref (G_OBJECT (pixbuf));
			pThis->loadImage (pixbuf, " on block data reading callback");
		}
	}

	// callback for image displaying
	static gboolean expose_event_cb (GtkWidget *widget, GdkEventExpose *event,
	                                 YGImage *pThis)
	{
printf ("expose event\n");
		if (!pThis->m_imageLoaded)
			return FALSE;

		GdkPixbuf *pixbuf;
		if (pThis->m_isAnimation) {
			// TODO: get current frame's pixbuf
			return FALSE;
		}
		else
			pixbuf = pThis->m_pixbuf;

		int x, y, w, h;
		x = widget->allocation.x;
		y = widget->allocation.y;
		w = widget->allocation.width;
		h = widget->allocation.height;

		// drawing
		cairo_t *cr = gdk_cairo_create (widget->window);
		gdk_cairo_set_source_pixbuf (cr, pixbuf, x, y);

// TODO: scaling and tiling

		cairo_rectangle (cr, x, y, w, h);
 
		cairo_fill (cr);
		cairo_destroy (cr);

		return TRUE;
	}
};

YWidget *
YGUI::createImage (YWidget *parent, YWidgetOpt &opt,
                   YCPByteblock image_data, YCPString default_text)
{
	return new YGImage (opt, YGWidget::get (parent), image_data, default_text);
}

YWidget *
YGUI::createImage (YWidget *parent, YWidgetOpt &opt,
                   YCPString file_name, YCPString default_text)
{
	return new YGImage (opt, YGWidget::get (parent), file_name, default_text);
}
