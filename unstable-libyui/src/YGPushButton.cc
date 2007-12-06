/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YPushButton.h"
#include "YGUtils.h"
#include "YGWidget.h"

class YGPushButton : public YPushButton, public YGWidget
{
GtkWidget *m_image;

public:
	YGPushButton (YWidget *parent, const string &label)
	:  YPushButton (NULL, label),
	   YGWidget (this, parent, true, GTK_TYPE_BUTTON, "can-default", TRUE, NULL)
	{
		IMPL
		m_image = NULL;
		setMinSizeInChars (10, 0);
		gtk_button_set_use_underline (GTK_BUTTON (getWidget()), TRUE);
		setLabel (label);
		g_signal_connect (G_OBJECT (getWidget ()), "clicked",
		                  G_CALLBACK (clicked_cb), this);
	}

	// YPushButton
	virtual void setLabel (const string &label)
	{
		IMPL
		string str = YGUtils::mapKBAccel (label);
		gtk_button_set_label (GTK_BUTTON (getWidget()), str.c_str());
		YGUtils::setStockIcon (getWidget(), str);
		YPushButton::setLabel (label);
	}

	virtual void setIcon (const string &icon)
	{
		IMPL
		if (icon.empty())
			// no need to worry about freeing m_image, let it live with button
			gtk_widget_hide (m_image);
		else {
			string path (icon);
			if (path[0] != '/')
				path = ICON_DIR + path;

			GError *error = 0;
			GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (path.c_str(), &error);
			if (pixbuf) {
				m_image = gtk_image_new_from_pixbuf (pixbuf);
				gtk_button_set_image (GTK_BUTTON (getWidget()), m_image);
				g_object_unref (G_OBJECT (pixbuf));
			}
			else
				y2warning ("YGPushButton: Couldn't load icon image: %s.\n"
				           "Reason: %s", path.c_str(), error->message);
		}
	}

	virtual void setDefaultButton (bool isDefault)
	{
		if (isDefault) {
			gtk_widget_grab_focus (getWidget());
			gtk_widget_grab_default (getWidget());
		}
		YPushButton::setDefaultButton (isDefault);
	}

	static void clicked_cb (GtkButton *button, YGPushButton *pThis)
	{ pThis->emitEvent (YEvent::Activated, false); }

	YGWIDGET_IMPL_COMMON
};

YPushButton *YGWidgetFactory::createPushButton (YWidget *parent, const string &label)
{
	return new YGPushButton (parent, label);
}

