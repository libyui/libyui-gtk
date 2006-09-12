//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YPushButton.h"
#include "YGUtils.h"
#include "YGWidget.h"

// comment next line for older core versions:
//#define USE_STOCK_ICONS

class YGPushButton : public YPushButton, public YGWidget
{
public:
	YGPushButton (const YWidgetOpt &opt,
	              YGWidget         *parent,
	              YCPString         label)
	:  YPushButton (opt, label),
	   YGWidget (this, parent, true, GTK_TYPE_BUTTON, NULL)
{
	IMPL
	setMinSize (14, 0);

#ifdef USE_STOCK_ICONS
	if (opt.setIcon.defined()) {
		string stock = "gtk-" + opt.setIcon.value();
		GtkWidget *icon = gtk_image_new_from_stock (stock.c_str(), GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image (GTK_BUTTON (getWidget()), icon);
	}
#endif

	gtk_button_set_use_underline (GTK_BUTTON (getWidget()), TRUE);
	g_signal_connect (G_OBJECT (getWidget ()), "clicked",
	                  G_CALLBACK (clicked_cb), this);
	setLabel (label);
}

	virtual ~YGPushButton() {}

	// YPushButton
	virtual void setLabel (const YCPString &label)
	{
		IMPL;
		string str = YGUtils::mapKBAccel(label->value_cstr());
		gtk_button_set_label (GTK_BUTTON (getWidget()), str.c_str());
		YPushButton::setLabel (label);
	}

	static void clicked_cb (GtkButton *button, YGPushButton *pThis)
	{ pThis->emitEvent (YEvent::Activated, false); }

	virtual void setIcon (const YCPString &icon_name)
	{
		IMPL
		string path = icon_name->value();
		if (path[0] != '/')
			path = ICON_DIR + path;

		GError *error = 0;
		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file
		                    (path.c_str(), &error);
		if (pixbuf) {
			GtkWidget *image = gtk_image_new_from_pixbuf (pixbuf);
			gtk_button_set_image (GTK_BUTTON (getWidget()), image);
		}
		else
			y2warning ("YGPushButton: Couldn't load icon image: %s.\n"
			           "Reason: %s", path.c_str(), error->message);
	}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
};


YWidget *
YGUI::createPushButton (YWidget *parent, YWidgetOpt &opt,
                        const YCPString &label)
{
	return new YGPushButton (opt, YGWidget::get (parent), label);
}
