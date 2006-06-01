/* Yast GTK interface module */

/* This is a convenience class that allows for a label next to the
   intended widget. It should be used, in case you have the need for
   such, as it gives an uniform API.                               */

#include "YGWidget.h"

class YGLabelWidget : public YGWidget
{
	public:
		YGLabelWidget(YWidget *y_widget, YGWidget *parent,
		              YCPString label_text, YUIDimension label_ori,
		              bool show, GType type, const char *property_name, ...);
		virtual ~YGLabelWidget () {}

		virtual GtkWidget* getWidget() { return m_field; }

		virtual void setLabel (const YCPString & label);

	protected:
		GtkWidget *m_label, *m_field;

		void construct(YGWidget *parent, YCPString label_text,
		              bool show, GType type, const char *property_name, va_list args);
};
