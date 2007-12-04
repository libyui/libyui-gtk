/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include "ygdkmngloader.h"
#include <config.h>
#include <ycp/y2log.h>
#include "YGUI.h"
#include "YGWidget.h"
#include "YImage.h"
#include "ygtkimage.h"

class YGImage : public YImage, public YGWidget
{
	void setProps (const YWidgetOpt &opt, const YCPString &alt_text)
	{
		bool scale = opt.scaleToFit.value(), tile = opt.tiled.value();
		YGtkImageAlign align = CENTER_IMAGE_ALIGN;
		if (tile)
			align = TILE_IMAGE_ALIGN;
		if (scale)
			align = SCALE_IMAGE_ALIGN;
		if (scale && tile)
			y2warning ("YImage can't be scaled and tiled at the same time");
		ygtk_image_set_props (YGTK_IMAGE (getWidget()), align, alt_text->value_cstr());

		bool zeroWidth = opt.zeroWidth.value(), zeroHeight = opt.zeroHeight.value();
		if (zeroWidth || scale || tile)
			setStretchable (YD_HORIZ, true);
		if (zeroHeight || scale || tile)
			setStretchable (YD_VERT, true);
		gtk_widget_set_size_request (getWidget(), zeroWidth ? 1 : -1, zeroHeight ? 1 : -1);
	}

public:
	YGImage (const YWidgetOpt &opt, YGWidget *parent,
	         const YCPString &filename_str, const YCPString &text)
	: YImage (opt),
	  YGWidget (this, parent, true, YGTK_TYPE_IMAGE, NULL)
	{
		IMPL
		setProps (opt, text);
		const char *filename = filename_str->value_cstr();
		bool animated = opt.animated.value();
		ygtk_image_set_from_file (YGTK_IMAGE (getWidget()), filename, animated);
	}

	YGImage (const YWidgetOpt &opt, YGWidget *parent,
	         const YCPByteblock &byteblock, const YCPString &text)
	: YImage (opt),
	  YGWidget (this, parent, true, YGTK_TYPE_IMAGE, NULL)
	{
		IMPL
		setProps (opt, text);
		const guint8 *data = byteblock->value();
		long data_size = byteblock->size();
		bool animated = opt.animated.value();
		ygtk_image_set_from_data (YGTK_IMAGE (getWidget()), data, data_size, animated);
	}

	YGWIDGET_IMPL_COMMON
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
