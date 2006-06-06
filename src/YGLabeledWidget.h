/* Yast GTK interface module */

/* This is a convenience class that allows for a label next to the
   intended widget. It should be used, in case you have the need for
   such, as it gives an uniform API.                               */

#ifndef YGLABELEDWIDGET_H
#define YGLABELEDWIDGET_H

#include "YGWidget.h"

class YGLabeledWidget : public YGWidget
{
	public:
		YGLabeledWidget(YWidget *y_widget, YGWidget *parent,
		                YCPString label_text, YUIDimension label_ori,
		                bool show, GType type, const char *property_name, ...);
		virtual ~YGLabeledWidget () {}

		virtual GtkWidget* getWidget() { return m_field; }

		void setLabelVisible(bool show);
		virtual void doSetLabel (const YCPString & label);

	protected:
		GtkWidget *m_label, *m_field;
};

#define YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN(ParentClass) \
	virtual void setLabel (const YCPString &label) { \
		fprintf (stderr, "%s:%s -G%p-Y%p- '%s' + chain\n", G_STRLOC, G_STRFUNC, \
		         m_widget, m_y_widget, label->value_cstr()); \
		doSetLabel (label); \
		ParentClass::setLabel (label); \
	}

#endif // YGLABELEDWIDGET_H
