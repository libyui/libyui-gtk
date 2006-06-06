/* Yast GTK interface module */

/* This is a convenience class for widgets that need scrollbars. */

#ifndef YGSCROLLEDWIDGET_H
#define YGSCROLLEDWIDGET_H

#include "YGLabeledWidget.h"

class YGScrolledWidget : public YGLabeledWidget
{
	public:
		YGScrolledWidget(YWidget *y_widget, YGWidget *parent,
		                 bool show, GType type, const char *property_name, ...);
		// if you want a label, use:
		YGScrolledWidget(YWidget *y_widget, YGWidget *parent,
		                 YCPString label_text, YUIDimension label_ori,
		                 bool show, GType type, const char *property_name, ...);
		virtual ~YGScrolledWidget () {}

		virtual GtkWidget* getWidget() { return m_widget; }

	protected:
		void construct(GType type, const char *property_name, va_list args);
		GtkWidget *m_widget;
};

#endif // YGSCROLLEDWIDGET_H
