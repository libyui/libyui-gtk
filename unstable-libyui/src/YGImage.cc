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
public:
	YGImage (YWidget *parent, const string &filename, bool animated)
	: YImage (parent, filename, animated),
	  YGWidget (this, parent, true, YGTK_TYPE_IMAGE, NULL)
	{
		IMPL
		ygtk_image_set_from_file (YGTK_IMAGE (getWidget()), filename.c_str(), animated);
	}

	virtual void setAutoScale (bool scale)
	{
		YGtkImageAlign align = CENTER_IMAGE_ALIGN;
		if (scale)
			align = SCALE_IMAGE_ALIGN;
		ygtk_image_set_props (YGTK_IMAGE (getWidget()), align, NULL);
	}

	YGWIDGET_IMPL_COMMON
};

YImage *YGWidgetFactory::createImage (YWidget *parent, const string &filename, bool animated)
{
	return new YGImage (parent, filename, animated);
}

