/* Yast GTK interface module */

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YGLabeledWidget.h"

YGLabeledWidget::YGLabeledWidget(
	     YWidget *y_widget,
	     YGWidget *parent,
	     YCPString label_text, YUIDimension label_ori,
	     bool show, GType type,
	     const char *property_name, ...)
	: YGWidget (y_widget, parent, show,
	    label_ori == YD_VERT ? GTK_TYPE_VBOX : GTK_TYPE_HBOX,
	    "spacing", 4, NULL)
{
	// Create the field widget
	va_list args;
	va_start (args, property_name);
	m_field = GTK_WIDGET (g_object_new_valist (type, property_name, args));
	va_end (args);

	// Create the label
	m_label = gtk_label_new ("");
	gtk_label_set_mnemonic_widget (GTK_LABEL (m_label), m_field);
	doSetLabel (label_text);

	// Set the container and show widgets
	gtk_container_add (GTK_CONTAINER (m_widget), m_label);
	gtk_container_add (GTK_CONTAINER (m_widget), m_field);
	gtk_box_set_child_packing (GTK_BOX (m_widget), m_label,
	                      FALSE, FALSE, 4, GTK_PACK_START);
	if(show) {
		gtk_widget_show (m_label);
		gtk_widget_show (m_field);
	}
}

void
YGLabeledWidget::setLabelVisible(bool show)
{
  if (show)
    gtk_widget_show (m_label);
  else
    gtk_widget_hide (m_label);
}

void
YGLabeledWidget::doSetLabel (const YCPString & label)
{
	string str = YGUtils::mapKBAccel (label->value_cstr());
	gtk_label_set_text (GTK_LABEL (m_label), str.c_str());
	gtk_label_set_use_underline (GTK_LABEL (m_label), str.compare(label->value_cstr()));
}
