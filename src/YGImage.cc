#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YImage.h"
#include "YGWidget.h"

class YGImage : public YImage, public YGWidget
{
	void construct (const YWidgetOpt &opt, GdkPixbuf* pixbuf)
	{
	if (pixbuf == NULL) {
fprintf(stderr, "COULDN'T LOAD PIXMAP\n");
		y2error("Couldn't load pixmap.");
		return;
	}

	gtk_image_set_from_pixbuf (GTK_IMAGE (getWidget()), pixbuf);
	}

public:
	YGImage (const YWidgetOpt &opt, YGWidget *parent,
	         const YCPString &filename, YCPString text)
	: YImage (opt),
	  YGWidget (this, parent, true, GTK_TYPE_IMAGE, NULL)
	{
		GError *error;
		construct (opt, gdk_pixbuf_new_from_file (filename->value_cstr(), &error));
	}

	YGImage (const YWidgetOpt &opt, YGWidget *parent,
	         const YCPByteblock &byteblock, YCPString text)
	: YImage (opt),
	  YGWidget (this, parent, true, GTK_TYPE_IMAGE, NULL)

	{  // TODO

/* byteblock->size()
Returns the number of bytes in the block.
long size() const;
*/
		GError *error;
/*
		GdkPixbuf* pixbuf = gdk_pixbuf_new_from_data (byteblock->value(),
			GDK_COLORSPACE_RGB, FALSE, 8, 50, 50, 0, NULL, NULL);
*/
	GdkPixbuf* pixbuf = gdk_pixbuf_new_from_inline
		(byteblock->size(), byteblock->value(), TRUE, &error);

		construct (opt, pixbuf);

// Options:
//  opt.zeroWidth.value()
//  opt.zeroHeight.value()
//  opt.animated.value()
//  opt.tiled.value()
//  opt.scaleToFit.value()
	}

	virtual ~YGImage() {}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	virtual bool stretchable (YUIDimension dimension) const
		IMPL_RET(true)
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

#if 0

YWidget *
YGUI::createImage( YWidget *parent, YWidgetOpt & opt,
				   YCPByteblock imagedata, YCPString defaulttext )
{
	IMPL;
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_inline (
		imagedata->size(), imagedata->value(),
		TRUE, NULL);
	IMPL;
	GtkWidget *image = gtk_image_new_from_pixbuf (pixbuf);
	return new YGImage (opt, YGWidget::get (parent), image, defaulttext);

	return NULL;
}

YWidget *
YGUI::createImage( YWidget *parent, YWidgetOpt & opt,
				   YCPString file_name, YCPString defaulttext )
{
	IMPL;

	GtkWidget *image = gtk_image_new_from_file (file_name->value_cstr());
	return new YGImage (opt, YGWidget::get (parent), image, defaulttext);
	return NULL;
}
#endif
