/* Yast GTK interface module */

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YGLabelWidget.h"

YGLabelWidget::YGLabelWidget(
	     YWidget *y_widget,
	     YGWidget *parent,
	     YCPString label_text, YUIDimension label_ori,
	     bool show, GType type,
	     const char *property_name, ...)
	: YGWidget (y_widget, parent, show,
	    label_ori == YD_VERT ? GTK_TYPE_VBOX : GTK_TYPE_HBOX,
	    "homogeneous", TRUE, "spacing", 4, NULL)
{
	va_list args;
	va_start (args, property_name);

	m_label = gtk_label_new ("");
	gtk_label_set_use_underline (GTK_LABEL (m_widget), TRUE);
	doSetLabel (label_text);
	m_field = GTK_WIDGET (g_object_new_valist (type, property_name, args));

	gtk_container_add (GTK_CONTAINER (m_widget), m_label);
	gtk_container_add (GTK_CONTAINER (m_widget), m_field);

	if(show)
	{
		gtk_widget_show (m_label);
		gtk_widget_show (m_field);
	}

	va_end (args);
}

void
YGLabelWidget::doSetLabel (const YCPString & label)
{
	YGUtils::setLabel (GTK_LABEL (m_label), label);
}
