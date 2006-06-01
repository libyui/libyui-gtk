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

	construct (parent, label_text, show, type, property_name, args);

	va_end (args);
}

void YGLabelWidget::construct (
               YGWidget *parent,
               YCPString label_text,
               bool show,
               GType type,
               const char *property_name,
               va_list args)
{
	m_label = gtk_label_new ("");
	setLabel (label_text);
	m_field = GTK_WIDGET (g_object_new_valist (type, property_name, args));

	gtk_container_add (GTK_CONTAINER(m_widget), m_label);
	gtk_container_add (GTK_CONTAINER(m_widget), m_field);

	if(show)
		{
		gtk_widget_show (m_label);
		gtk_widget_show (m_field);
		}
#if 0
		gtk_button_set_use_underline (GTK_BUTTON (m_label), TRUE);
		g_signal_connect (G_OBJECT (getWidget ()),
				  "toggled", G_CALLBACK (toggled_cb), this);
#endif
}

void
YGLabelWidget::setLabel (const YCPString & label)
{
	YGUtils::setLabel (GTK_LABEL(m_label), label);
}
