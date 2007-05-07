//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YPushButton.h"
#include "YGUtils.h"
#include "YGWidget.h"

class YGPushButton : public YPushButton, public YGWidget
{
public:
	YGPushButton (const YWidgetOpt &opt, YGWidget *parent, YCPString label)
	:  YPushButton (opt, label),
	   YGWidget (this, parent, true, GTK_TYPE_BUTTON, "can-default", TRUE, NULL)
	{
		IMPL
		if (!opt.isShrinkable.value())
			setMinSizeInChars (10, 0);

		gtk_button_set_use_underline (GTK_BUTTON (getWidget()), TRUE);
		setLabel (label);

		g_signal_connect (G_OBJECT (getWidget ()), "clicked",
		                  G_CALLBACK (clicked_cb), this);
		if (opt.isDefaultButton.value())
			g_signal_connect (G_OBJECT (getWidget ()), "realize",
			                  G_CALLBACK (set_default_cb), this);
	}

	virtual ~YGPushButton() {}

	// YPushButton
	virtual void setLabel (const YCPString &label)
	{
		IMPL;
		string str = YGUtils::mapKBAccel (label->value_cstr());
		gtk_button_set_label (GTK_BUTTON (getWidget()), str.c_str());
		YPushButton::setLabel (label);
	}

	virtual void setIcon (const YCPString &icon_name)
	{
		IMPL
		string path = icon_name->value();
		if (path[0] != '/')
			path = ICON_DIR + path;

		GError *error = 0;
		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (path.c_str(), &error);
		if (pixbuf) {
			GtkWidget *image = gtk_image_new_from_pixbuf (pixbuf);
			gtk_button_set_image (GTK_BUTTON (getWidget()), image);
		}
		else
			y2warning ("YGPushButton: Couldn't load icon image: %s.\n"
			           "Reason: %s", path.c_str(), error->message);
	}

	static void set_default_cb (GtkButton *button, YGPushButton *pThis)
	{ gtk_widget_grab_default (GTK_WIDGET (button)); }

	static void clicked_cb (GtkButton *button, YGPushButton *pThis)
	{ pThis->emitEvent (YEvent::Activated, false); }

	YGWIDGET_IMPL_COMMON
};


YWidget *
YGUI::createPushButton (YWidget *parent, YWidgetOpt &opt,
                        const YCPString &label)
{
	return new YGPushButton (opt, YGWidget::get (parent), label);
}
