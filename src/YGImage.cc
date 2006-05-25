#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YImage.h"
#include "YGWidget.h"

#ifdef REFACTOR_CONSTRUCTION ...

class YGImage : public YImage, public YGWidget
{
public:
	YGImage( const YWidgetOpt &opt,
			 YGWidget         *parent,
			 GtkWidget        *widget,
			 YCPString         defaulttext );
	virtual ~YGImage() {}

    // YWidget
    YGWIDGET_IMPL_NICESIZE
    virtual bool stretchable( YUIDimension dimension ) const IMPL_RET(true);
    YGWIDGET_IMPL_SET_SIZE
    YGWIDGET_IMPL_SET_ENABLING
};

YGImage::YGImage( const YWidgetOpt &opt,
				  YGWidget         *parent,
				  GtkWidget        *widget,
				  YCPString         defaulttext )
	:  YImage( opt ),
	   YGWidget( this, parent, widget )
{
	// FIXME: defaulttext ... ?
}

#endif

YWidget *
YGUI::createImage( YWidget *parent, YWidgetOpt & opt,
				   YCPByteblock imagedata, YCPString defaulttext )
{
	IMPL;
#if 0
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_inline (
		imagedata->size(), imagedata->value(),
		TRUE, NULL);
	IMPL;
	GtkWidget *image = gtk_image_new_from_pixbuf (pixbuf);
	return new YGImage (opt, YGWidget::get (parent), image, defaulttext);
#endif
	return NULL;
}

YWidget *
YGUI::createImage( YWidget *parent, YWidgetOpt & opt,
				   YCPString file_name, YCPString defaulttext )
{
	IMPL;
#if 0
	GtkWidget *image = gtk_image_new_from_file (file_name->value_cstr());
	return new YGImage (opt, YGWidget::get (parent), image, defaulttext);
#endif
	return NULL;
}
