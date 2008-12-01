/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <YGUI.h>
#include <YPushButton.h>
#include "YGUtils.h"
#include "YGWidget.h"

#include <YLayoutBox.h>

class YGPushButton : public YPushButton, public YGWidget
{
bool m_customIcon, m_labelIcon;

public:
	YGPushButton (YWidget *parent, const string &label)
	:  YPushButton (NULL, label),
	   YGWidget (this, parent, GTK_TYPE_BUTTON, "can-default", TRUE, NULL)
	{
		IMPL
		m_customIcon = m_labelIcon = false;
		gtk_button_set_use_underline (GTK_BUTTON (getWidget()), TRUE);
		setLabel (label);
		connect (getWidget(), "clicked", G_CALLBACK (clicked_cb), this);
		g_signal_connect (G_OBJECT (getWidget()), "expose-event",
		                        G_CALLBACK (treat_icon_cb), this);
	}

	void setStockIcon (const std::string &label)
	{
		if (!m_customIcon) {
			const char *stock = NULL;
			if (hasFunctionKey())
				switch (functionKey()) {
					case 2:
						stock = GTK_STOCK_INFO;
						break;
					case 3:
						stock = GTK_STOCK_ADD;
						break;
					case 4:
						stock = GTK_STOCK_EDIT;
						break;
					case 5:
						stock = GTK_STOCK_DELETE;
						break;
					case 6:
						stock = GTK_STOCK_EXECUTE;
						break;
					case 7:
						stock = GTK_STOCK_PREFERENCES;
						break;
					default:
						break;
				}
#if YAST2_VERSION >= 2017006
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
#endif
			if (isHelpButton())
				stock = GTK_STOCK_HELP;
			m_labelIcon = YGUtils::setStockIcon (getWidget(), label, stock);
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

#if YAST2_VERSION >= 2017006
	virtual void setRole (YButtonRole role)
	{
		YPushButton::setRole (role);
		if (!m_labelIcon && role != YCustomButton)  // to avoid duplications
			setStockIcon (label());
	}
#endif

	virtual void setHelpButton (bool helpButton)
	{
		YPushButton::setHelpButton (helpButton);
		if (!m_labelIcon && helpButton)
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
				path = std::string (THEMEDIR) + "/" + path;

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
		struct inner {
			static void realize_cb (GtkWidget *widget)
			{ gtk_widget_grab_focus (widget); }
		};

		YPushButton::setDefaultButton (isDefault);
		if (isDefault) {
			GtkWidget *button = getWidget();
			GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
			gtk_widget_grab_default (button);
			if (GTK_WIDGET_REALIZED (button))
				gtk_widget_grab_focus (button);
			else
				g_signal_connect (G_OBJECT (button), "realize",
				                  G_CALLBACK (inner::realize_cb), this);
		}
	}

	static gboolean treat_icon_cb (GtkWidget *widget, GdkEventExpose *event,
	                               YGPushButton *pThis)
	{
		// only set stock icons if all to the left have them
		YLayoutBox *ybox = dynamic_cast <YLayoutBox *> (pThis->parent());
		if (ybox && ybox->primary() == YD_HORIZ && !pThis->m_customIcon) {
			YWidget *ylast = 0;
			for (YWidgetListConstIterator it = ybox->childrenBegin();
			     it != ybox->childrenEnd(); it++) {
				if ((YWidget *) pThis == *it) {
					if (ylast) {
						GtkWidget *button = YGWidget::get (ylast)->getWidget();
						GtkWidget *icon = gtk_button_get_image (GTK_BUTTON (button));
						if (!icon || !GTK_WIDGET_VISIBLE (icon))
							pThis->setIcon ("");
					}
					break;
				}
				ylast = *it;
				if (!dynamic_cast <YPushButton *> (ylast))
					ylast = NULL;
			}
		}
		g_signal_handlers_disconnect_by_func (widget, (gpointer) treat_icon_cb, pThis);
		return FALSE;
	}

	// Events
	static void clicked_cb (GtkButton *button, YGPushButton *pThis)
	{ pThis->emitEvent (YEvent::Activated, IGNORE_NOTIFY_EVENT); }

	YGWIDGET_IMPL_COMMON
};

YPushButton *YGWidgetFactory::createPushButton (YWidget *parent, const string &label)
{
	return new YGPushButton (parent, label);
}

