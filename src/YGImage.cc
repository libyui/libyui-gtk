#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YImage.h"
#include "YGWidget.h"

class YGImage : public YImage, public YGWidget
{
	bool m_zeroWidth, m_zeroHeight, m_scale, m_tiled;
//	GdkPixBuf *pixbuf_ori;  // after resize quality is lost, so we need an original

	void construct (const YWidgetOpt &opt, void* pixbuf)
	{
		if (pixbuf == NULL) {
			g_warning ("Couldn't load pixmap.");
			return;
		}

		if (opt.animated.value())
			gtk_image_set_from_animation (GTK_IMAGE (getWidget()),
			                        (GdkPixbufAnimation*) pixbuf);
		else
			gtk_image_set_from_pixbuf (GTK_IMAGE (getWidget()),
			                              (GdkPixbuf*) pixbuf);

		m_zeroWidth  = opt.zeroWidth.value();
		m_zeroHeight = opt.zeroHeight.value();
		// TODO implement:
		m_scale      = opt.scaleToFit.value();
		m_tiled      = opt.tiled.value();
	}

public:
	YGImage (const YWidgetOpt &opt, YGWidget *parent,
	         const YCPString &filename, YCPString text)
	: YImage (opt),
	  YGWidget (this, parent, true, GTK_TYPE_IMAGE, NULL)
	{
		GError *error = 0;
		if (opt.animated.value())
			construct (opt, gdk_pixbuf_animation_new_from_file
			                    (filename->value_cstr(), &error));
		else
			construct (opt, gdk_pixbuf_new_from_file
			                    (filename->value_cstr(), &error));
	}

	YGImage (const YWidgetOpt &opt, YGWidget *parent,
	         const YCPByteblock &byteblock, YCPString text)
	: YImage (opt),
	  YGWidget (this, parent, true, GTK_TYPE_IMAGE, NULL)

	{
		GError *error = 0;
		GdkPixbufLoader* loader = gdk_pixbuf_loader_new();
		if (!gdk_pixbuf_loader_write (loader,
		        byteblock->value(), byteblock->size(), &error)) {
			g_warning ("Could not load image from data blocks.");
			return;
		}

		if (opt.animated.value())
			construct (opt, gdk_pixbuf_loader_get_animation (loader));
		else
			construct (opt, gdk_pixbuf_loader_get_pixbuf (loader));

		gdk_pixbuf_loader_close (loader, &error);
	}

	virtual ~YGImage() {}

	// YWidget
	virtual bool stretchable (YUIDimension dimension) const
		IMPL_RET(true)
	virtual long nicesize (YUIDimension dim)
	{
		IMPL
		if (dim == YD_HORIZ && m_zeroWidth)  return 0;
		if (dim == YD_VERT  && m_zeroHeight) return 0;
		return getNiceSize (dim);
	}
	YGWIDGET_IMPL_SET_SIZE
//	virtual void setSize (long newWidth, long newHeight)  // TODO: overload for scaling
};

YWidget *
YGUI::createImage (YWidget *parent, YWidgetOpt &opt,
                   YCPByteblock imagedata, YCPString defaulttext)
{
	return new YGImage (opt, YGWidget::get (parent), imagedata, defaulttext);
}

YWidget *
YGUI::createImage (YWidget *parent, YWidgetOpt &opt,
                   YCPString file_name, YCPString defaulttext)
{
	return new YGImage (opt, YGWidget::get (parent), file_name, defaulttext);
}
