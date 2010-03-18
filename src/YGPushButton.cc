/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#define YUILogComponent "gtk"
#include "config.h"
#include "YGUI.h"
#include <YPushButton.h>
#include "YGUtils.h"
#include "YGWidget.h"
#include <string.h>

#include <YLayoutBox.h>

class YGPushButton : public YPushButton, public YGWidget
{
bool m_customIcon, m_labelIcon;

public:
	YGPushButton (YWidget *parent, const string &label)
	:  YPushButton (NULL, label),
	   YGWidget (this, parent, GTK_TYPE_BUTTON, "can-default", TRUE, NULL)
	{
		m_customIcon = m_labelIcon = false;
		gtk_button_set_use_underline (GTK_BUTTON (getWidget()), TRUE);
		setLabel (label);
		connect (getWidget(), "clicked", G_CALLBACK (clicked_cb), this);
		g_signal_connect (getWidget(), "size-request", G_CALLBACK (size_request_cb), this);
	}

	void setStockIcon (const std::string &label)
	{
		if (!m_customIcon) {
			const char *stock = NULL;
			switch (functionKey()) {
				case 1: stock = GTK_STOCK_HELP; break;
				case 2: stock = GTK_STOCK_INFO; break;  // Info
				case 3: stock = GTK_STOCK_ADD; break;
				case 4: stock = GTK_STOCK_EDIT; break;
				case 5: stock = GTK_STOCK_DELETE; break;
				case 6: stock = GTK_STOCK_EXECUTE; break;  // Test
				case 7: stock = GTK_STOCK_EDIT; break;  // Expert
				// old expert icon: GTK_STOCK_PREFERENCES
				//case 8: stock = GTK_STOCK_GO_BACK; break;
				case 9: stock = GTK_STOCK_CANCEL; break;
				case 10: stock = GTK_STOCK_OK; break;  // Next/Finish/OK
				default: break;
			}
#if YAST2_VERSION >= 2017006
			switch (role()) {
				case YOKButton:     stock = GTK_STOCK_OK; break;
				case YApplyButton:  stock = GTK_STOCK_APPLY; break;
				case YCancelButton: stock = GTK_STOCK_CANCEL; break;
				case YHelpButton:   stock = GTK_STOCK_HELP; break;
				case YCustomButton: case YMaxButtonRole: break;
			}
#endif
			m_labelIcon = YGUtils::setStockIcon (getWidget(), label, stock);
		}
	}

	// YPushButton
	virtual void setLabel (const std::string &label)
	{
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

	virtual void setIcon (const string &icon)
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
			{
				gtk_widget_grab_default (widget);
				gtk_widget_grab_focus (widget);
			}
		};

		YPushButton::setDefaultButton (isDefault);
		if (isDefault) {
			GtkWidget *button = getWidget();
			GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
			if (GTK_WIDGET_REALIZED (button))
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
			return icon && GTK_WIDGET_VISIBLE (icon);
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

	static void size_request_cb (GtkWidget *widget, GtkRequisition *req, YGPushButton *pThis)
	{	// enlarge button if parent is ButtonBox
		YWidget *yparent = pThis->m_ywidget->parent();
		if (!strcmp (yparent->widgetClass(), "YButtonBox")) {
			req->width = MAX (req->width, DEFAULT_CHILD_MIN_WIDTH);
			req->height = MAX (req->height, DEFAULT_CHILD_MIN_HEIGHT);
		}
	}

	YGWIDGET_IMPL_COMMON (YPushButton)
};

YPushButton *YGWidgetFactory::createPushButton (YWidget *parent, const string &label)
{ return new YGPushButton (parent, label); }

