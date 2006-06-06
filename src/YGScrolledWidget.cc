/* Yast GTK interface module */

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGScrolledWidget.h"

YGScrolledWidget::YGScrolledWidget (
		YWidget *y_widget, YGWidget *parent,
		bool show, GType type, const char *property_name, ...)
: YGLabeledWidget (y_widget, parent,
                   YCPString ("<no label>"), YD_VERT,
                   show, GTK_TYPE_SCROLLED_WINDOW, NULL)
{
	va_list args;
	va_start (args, property_name);
	construct(type, property_name, args);
	va_end (args);

	setLabelVisible (false);
}

YGScrolledWidget::YGScrolledWidget (
		YWidget *y_widget, YGWidget *parent,
		YCPString label_text, YUIDimension label_ori,
		bool show, GType type, const char *property_name, ...)
: YGLabeledWidget (y_widget, parent, label_text, label_ori, show,
                   GTK_TYPE_SCROLLED_WINDOW, NULL)
{
	va_list args;
	va_start (args, property_name);
	construct(type, property_name, args);
	va_end (args);
}

void YGScrolledWidget::construct
		(GType type, const char *property_name, va_list args)
{
	m_widget = GTK_WIDGET (g_object_new_valist (type, property_name, args));

	gtk_scrolled_window_set_policy (
		GTK_SCROLLED_WINDOW (YGLabeledWidget::getWidget()),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (YGLabeledWidget::getWidget()), m_widget);
	gtk_widget_show (m_widget);
}
