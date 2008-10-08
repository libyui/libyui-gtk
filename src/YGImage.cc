/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include "ygdkmngloader.h"
#include <config.h>
#include "YGUI.h"
#include "YGWidget.h"
#include "YImage.h"
#include "ygtkimage.h"
#include <string.h>

static bool endsWith (const std::string &str1, const char *str2)
{
	size_t len = strlen (str2);
	if (str1.size() < len) return false;
	return str1.compare (str1.size()-len, len, str2, len) == 0;
}

class YGImage : public YImage, public YGWidget
{
public:
	YGImage (YWidget *parent, const string &filename, bool animated)
	: YImage (NULL, filename, animated),
	  YGWidget (this, parent, true, YGTK_TYPE_IMAGE, NULL)
	{
		IMPL
		YGtkImage *image = YGTK_IMAGE (getWidget());
		const char *stock = NULL;
		if (endsWith (filename, "/msg_question.png"))
			stock = GTK_STOCK_DIALOG_QUESTION;
		else if (endsWith (filename, "/msg_info.png"))
			stock = GTK_STOCK_DIALOG_INFO;
		else if (endsWith (filename, "/msg_warning.png"))
			stock = GTK_STOCK_DIALOG_WARNING;
		else if (endsWith (filename, "/msg_error.png"))
			stock = GTK_STOCK_DIALOG_ERROR;
		if (stock && gtk_style_lookup_icon_set (m_widget->style, stock)) {
			GdkPixbuf *pixbuf = gtk_widget_render_icon (m_widget, stock, GTK_ICON_SIZE_DIALOG, NULL);
			ygtk_image_set_from_pixbuf (image, pixbuf);
		}
		else
			ygtk_image_set_from_file (image, filename.c_str(), animated);
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

