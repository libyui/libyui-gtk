/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#define YUILogComponent "gtk"

#include "ygdkmngloader.h"
#include "YGUI.h"
#include "YGWidget.h"
#include "YImage.h"
#include "ygtkimage.h"
#include <string.h>
#include <boost/filesystem.hpp>

static inline bool endsWith (const std::string &str1, const char *str2)
{
	size_t len = strlen (str2);
	if (str1.size() < len) return false;
	return str1.compare (str1.size()-len, len, str2, len) == 0;
}

class YGImage : public YImage, public YGWidget
{
public:
	YGImage (YWidget *parent, const std::string &filename, bool animated)
	: YImage (NULL, filename, animated),
	  YGWidget (this, parent, YGTK_TYPE_IMAGE, NULL)
	{
		setImage( filename, animated );
	}

	virtual void setImage( const std::string & filename, bool animated )
	{
		YImage::setImage ( filename, animated );

		YGtkImage *image = YGTK_IMAGE (getWidget());
		const char *iconname = NULL;
		if (endsWith (filename, "/msg_question.png"))
			iconname = "dialog-question";
		else if (endsWith (filename, "/msg_info.png"))
			iconname = "dialog-information";
		else if (endsWith (filename, "/msg_warning.png"))
			iconname = "dialog-warning";
		else if (endsWith (filename, "/msg_error.png"))
			iconname = "dialog-error";

		if (iconname && gtk_icon_theme_lookup_icon (gtk_icon_theme_get_default(), iconname, GTK_ICON_SIZE_DIALOG, GTK_ICON_LOOKUP_USE_BUILTIN)) {
			GdkPixbuf *pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(), iconname, GTK_ICON_SIZE_DIALOG, GTK_ICON_LOOKUP_USE_BUILTIN,NULL);
			ygtk_image_set_from_pixbuf (image, pixbuf);
		}
		else
		{
			if (animated)
				ygtk_image_set_from_file (image, filename.c_str(), animated);
			else {
				if (boost::filesystem::path(filename).has_extension()) {
					ygtk_image_set_from_file (image, filename.c_str(), animated);
				}
				else {
					if (gtk_icon_theme_lookup_icon (gtk_icon_theme_get_default(), filename.c_str(), GTK_ICON_SIZE_DIALOG, GTK_ICON_LOOKUP_USE_BUILTIN)) {
						GdkPixbuf *pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(), filename.c_str(), GTK_ICON_SIZE_DIALOG, GTK_ICON_LOOKUP_USE_BUILTIN,NULL);
						ygtk_image_set_from_pixbuf (image, pixbuf);
					}
					else // last chance to find an image
						ygtk_image_set_from_file (image, filename.c_str(), animated);
				}
			}
		}
	}

	virtual void setAutoScale (bool scale)
	{
		YGtkImageAlign align = CENTER_IMAGE_ALIGN;
		if (scale)
			align = SCALE_IMAGE_ALIGN;
		ygtk_image_set_props (YGTK_IMAGE (getWidget()), align, NULL);
	}

	YGWIDGET_IMPL_COMMON (YImage)
};

YImage *YGWidgetFactory::createImage (YWidget *parent, const std::string &filename, bool animated)
{ return new YGImage (parent, filename, animated); }

