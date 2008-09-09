/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <YGUI.h>
#include "YPushButton.h"
#include "YGUtils.h"
#include "YGWidget.h"

class YGPushButton : public YPushButton, public YGWidget
{
bool m_customIcon, m_labelIcon;

public:
	YGPushButton (YWidget *parent, const string &label)
	:  YPushButton (NULL, label),
	   YGWidget (this, parent, true, GTK_TYPE_BUTTON, "can-default", TRUE, NULL)
	{
		IMPL
		m_customIcon = m_labelIcon = false;
		setMinSizeInChars (10, 0);
		gtk_button_set_use_underline (GTK_BUTTON (getWidget()), TRUE);
		setLabel (label);
		g_signal_connect (G_OBJECT (getWidget ()), "clicked",
		                  G_CALLBACK (clicked_cb), this);
	}

	void setStockIcon (const std::string &label)
	{
		if (!m_customIcon) {
			if (YGUtils::setStockIcon (label, getWidget()))
				m_labelIcon = true;
			else {
				m_labelIcon = false;
				const char *stock = NULL;
				switch (role()) {
					case YOKButton:
						stock = GTK_STOCK_OK;
						break;
					case YApplyButton:
						stock = GTK_STOCK_APPLY;
						break;
					case YCancelButton:
						stock = GTK_STOCK_CANCEL;
						break;
					default:
						break;
				}
				GtkWidget *image = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_BUTTON);
				gtk_button_set_image (GTK_BUTTON (getWidget()), image);
			}
		}
	}

	// YPushButton
	virtual void setLabel (const string &label)
	{
		IMPL
		YPushButton::setLabel (label);
		string str = YGUtils::mapKBAccel (label);
		gtk_button_set_label (GTK_BUTTON (getWidget()), str.c_str());
		setStockIcon (str);
	}

	virtual void setRole (YButtonRole role)
	{
		YPushButton::setRole (role);
		if (!m_labelIcon && role != YCustomButton)  // to avoid duplications
			setStockIcon (label());
	}

	virtual void setIcon (const string &icon)
	{
		IMPL
		GtkButton *button = GTK_BUTTON (getWidget());
		if (icon.empty()) {
			m_customIcon = false;
			// no need to worry about freeing the image, let it live with button
			GtkWidget *image = gtk_button_get_image (button);
			if (image)
				gtk_widget_hide (image);
		}
		else {
			m_customIcon = true;
			string path (icon);
			if (path[0] != '/')
				path = ICON_DIR + path;

			GError *error = 0;
			GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (path.c_str(), &error);
			if (pixbuf) {
				GtkWidget *image = gtk_image_new_from_pixbuf (pixbuf);
				gtk_button_set_image (button, image);
				g_object_unref (G_OBJECT (pixbuf));
			}
			else
				yuiWarning() << "YGPushButton: Couldn't load icon image: " << path << endl
				             << "Reason: " << error->message << endl;
		}
	}

	virtual void setDefaultButton (bool isDefault)
	{
		YPushButton::setDefaultButton (isDefault);
		if (isDefault) {
			GtkWidget *button = getWidget();
			GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
			gtk_widget_grab_default (button);
			if (GTK_WIDGET_REALIZED (button))
				gtk_widget_grab_focus (button);
			else
				g_signal_connect (G_OBJECT (button), "realize",
				                  G_CALLBACK (realize_cb), this);
		}
	}

	virtual void setHelpButton (bool helpButton)
	{
		YPushButton::setHelpButton (helpButton);
		if (helpButton) {
			GtkWidget *image;
			image = gtk_image_new_from_stock (GTK_STOCK_HELP, GTK_ICON_SIZE_BUTTON);
			gtk_button_set_image (GTK_BUTTON (getWidget()), image);
		}
	}

	// Events
	static void clicked_cb (GtkButton *button, YGPushButton *pThis)
	{ pThis->emitEvent (YEvent::Activated, false); }

	// give focus to default buttons, once they are realized
	static void realize_cb (GtkWidget *widget)
	{
		gtk_widget_grab_focus (widget);
	}

	YGWIDGET_IMPL_COMMON
};

YPushButton *YGWidgetFactory::createPushButton (YWidget *parent, const string &label)
{
	return new YGPushButton (parent, label);
}

