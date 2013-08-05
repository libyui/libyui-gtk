/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#define YUILogComponent "gtk"
#include <yui/Libyui_config.h>
#include "YGUI.h"
#include <YPushButton.h>
#include "YGUtils.h"
#include "YGWidget.h"
#include <string.h>

#include <YLayoutBox.h>
#include "ygtkratiobox.h"

class YGPushButton : public YPushButton, public YGWidget
{
bool m_customIcon, m_labelIcon;

public:
	YGPushButton (YWidget *parent, const std::string &label)
	:  YPushButton (NULL, label),
	   YGWidget (this, parent, GTK_TYPE_BUTTON, "can-default", TRUE, NULL)
	{
		m_customIcon = m_labelIcon = false;
		gtk_button_set_use_underline (GTK_BUTTON (getWidget()), TRUE);
		setLabel (label);
		connect (getWidget(), "clicked", G_CALLBACK (clicked_cb), this);
		g_signal_connect (getWidget(), "realize", G_CALLBACK (realize_cb), this);
	}

	void setStockIcon (const std::string &label)
	{
		if (!m_customIcon) {
			const char *icon = NULL;
			switch (functionKey()) {
				case 1: icon = "help-contents"; break;
				case 2: icon = "dialog-information"; break;  // Info
				case 3: icon = "list-add"; break;
				case 4: icon = "edit-copy"; break;
				case 5: icon = "edit-delete"; break;
				case 6: icon = "system-run"; break;  // Test
				case 7: icon = "edit-copy"; break;  // Expert
				// old expert icon: GTK_STOCK_PREFERENCES
				// case 8: icon = "go-previous"; break;
				case 9: icon = "application-exit"; break;
				case 10: icon = "application-exit"; break;  // Next/Finish/OK
				default: break;
			}
			switch (role()) {
				case YOKButton:     icon = "document-save"; break;
				case YApplyButton:  icon = "document-save"; break;
				case YCancelButton: icon = "document-revert"; break;
				case YHelpButton:   icon = "help-contents"; break;
				case YCustomButton: case YMaxButtonRole: break;
			}
			m_labelIcon = YGUtils::setStockIcon (getWidget(), label, icon);
		}
	}

	// YPushButton
	virtual void setLabel (const std::string &label)
	{
		YPushButton::setLabel (label);
		std::string str = YGUtils::mapKBAccel (label);
		gtk_button_set_label (GTK_BUTTON (getWidget()), str.c_str());
		setStockIcon (str);
	}

	virtual void setRole (YButtonRole role)
	{
		YPushButton::setRole (role);
		if (!m_labelIcon && role != YCustomButton)  // to avoid duplications
			setStockIcon (label());
	}

	virtual void setFunctionKey (int key)
	{
		YPushButton::setFunctionKey (key);
		if (!m_labelIcon && hasFunctionKey())
			setStockIcon (label());
	}

	virtual void setHelpButton (bool helpButton)
	{
		YPushButton::setHelpButton (helpButton);
		if (!m_labelIcon && helpButton)
			setStockIcon (label());
	}

	virtual void setIcon (const std::string &icon)
	{
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
			std::string path (icon);
			if (path[0] != '/')
				path = std::string (THEMEDIR) + "/" + path;

			GError *error = 0;
			GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (path.c_str(), &error);
			if (pixbuf) {
				GtkWidget *image = gtk_image_new_from_pixbuf (pixbuf);
				gtk_button_set_image (button, image);
				// disregard gtk-button-images setting for explicitly set icons
				gtk_widget_show (image);
				g_object_unref (G_OBJECT (pixbuf));
			}
			else
				yuiWarning() << "YGPushButton: Couldn't load icon image: " << path << std::endl
				             << "Reason: " << error->message << std::endl;
		}
	}

	virtual void setDefaultButton (bool isDefault)
	{
		struct inner {
			static void realize_cb (GtkWidget *widget)
			{
				gtk_widget_grab_default (widget);
				gtk_widget_grab_focus (widget);
			}
		};

		YPushButton::setDefaultButton (isDefault);
		if (isDefault) {
			GtkWidget *button = getWidget();
			gtk_widget_set_can_default(button, TRUE);
			if (gtk_widget_get_realized (button))
				inner::realize_cb (button);
			g_signal_connect (G_OBJECT (button), "realize",
			                  G_CALLBACK (inner::realize_cb), this);
		}
	}

	static bool hasIcon (YWidget *ywidget)
	{
		if (dynamic_cast <YPushButton *> (ywidget)) {
			GtkWidget *button = YGWidget::get (ywidget)->getWidget();
			GtkWidget *icon = gtk_button_get_image (GTK_BUTTON (button));
			return icon && gtk_widget_get_visible (icon);
		}
		return true;
	}

#if 0
	static gboolean treat_icon_cb (GtkWidget *widget, GdkEventExpose *event,
	                               YGPushButton *pThis)
	{
		YLayoutBox *ybox = dynamic_cast <YLayoutBox *> (pThis->parent());
		if (ybox && !pThis->m_customIcon) {
			if (ybox->primary() == YD_HORIZ) {
				// only set stock icons if all to the left have them
				YWidget *ylast = 0;
				for (YWidgetListConstIterator it = ybox->childrenBegin();
					 it != ybox->childrenEnd(); it++) {
					if ((YWidget *) pThis == *it) {
						if (ylast && !hasIcon (ylast))
							pThis->setIcon ("");
						break;
					}
					ylast = *it;
					if (!dynamic_cast <YPushButton *> (ylast))
						ylast = NULL;
				}
			}
			else {  // YD_VERT
				// different strategy: set icons for all or none
				bool disableIcons = false;
				for (YWidgetListConstIterator it = ybox->childrenBegin();
					 it != ybox->childrenEnd(); it++)
					if (!hasIcon (*it))
						disableIcons = true;
				if (disableIcons)
					for (YWidgetListConstIterator it = ybox->childrenBegin();
						 it != ybox->childrenEnd(); it++)
						if (dynamic_cast <YPushButton *> (*it)) {
							YGPushButton *button = (YGPushButton *)
								YGWidget::get (*it);
							if (!button->m_customIcon)
								button->setIcon ("");
						}
			}
		}
		g_signal_handlers_disconnect_by_func (widget, (gpointer) treat_icon_cb, pThis);
		return FALSE;
	}
#endif

	// callbacks
	static void clicked_cb (GtkButton *button, YGPushButton *pThis)
	{ pThis->emitEvent (YEvent::Activated, IGNORE_NOTIFY_EVENT); }

// default values from gtkbbox.c; can vary from style to style, but no way to query those
#define DEFAULT_CHILD_MIN_WIDTH 85
#define DEFAULT_CHILD_MIN_HEIGHT 27

	static void realize_cb (GtkWidget *widget, YGPushButton *pThis)
	{	// enlarge button if parent is ButtonBox
		YWidget *yparent = pThis->m_ywidget->parent();
		if (yparent && !strcmp (yparent->widgetClass(), "YButtonBox"))
			ygtk_adj_size_set_min (YGTK_ADJ_SIZE(pThis->getLayout()),
				DEFAULT_CHILD_MIN_WIDTH, DEFAULT_CHILD_MIN_HEIGHT);
	}

	YGWIDGET_IMPL_COMMON (YPushButton)
};

YPushButton *YGWidgetFactory::createPushButton (YWidget *parent, const std::string &label)
{ return new YGPushButton (parent, label); }

