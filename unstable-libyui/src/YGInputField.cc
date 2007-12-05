/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YInputField.h"
#include "YGWidget.h"
#include "YGUtils.h"
#include "ygtkfieldentry.h"

class YGInputField : public YInputField, public YGLabeledWidget
{
public:
	YGInputField (YWidget *parent, const string &label, bool passwordMode)
	: YInputField (parent, label, passwordMode),
	  YGLabeledWidget (this, parent, label, YD_HORIZ, true,
	                   GTK_TYPE_ENTRY, NULL)
	{
		gtk_entry_set_activates_default (GTK_ENTRY (getWidget()), TRUE);
		if (passwordMode)
			gtk_entry_set_visibility (GTK_ENTRY (getWidget()), FALSE);

		// Signals to know of any text modification
		g_signal_connect (G_OBJECT (getWidget()), "changed",
		                  G_CALLBACK (text_changed_cb), this);
	}

	// YTextEntry
	virtual string value()
	{
		return gtk_entry_get_text (GTK_ENTRY (getWidget()));
	}

	virtual void setValue (const string &text)
	{
		/* No need to check for valid chars as that's the responsible of the YCP
		   application programmer. */
		gtk_entry_set_text (GTK_ENTRY (getWidget()), text.c_str());
	}

	virtual void setInputMaxLength (int nb)
	{
		gtk_entry_set_width_chars (GTK_ENTRY (getWidget()), nb);
	}

	virtual void setValidChars (const string &validChars)
	{
		YGUtils::setFilter (GTK_ENTRY (getWidget()), validChars);
		YInputField::setValidChars (validChars);
	}

	static void text_changed_cb (GtkEditable *editable, YGInputField *pThis)
	{
		pThis->emitEvent (YEvent::ValueChanged);
	}

	YGWIDGET_IMPL_COMMON
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YInputField)
};

YInputField *YGWidgetFactory::createInputField (YWidget *parent, const string &label,
                                                bool passwordMode)
{
	return new YGInputField (parent, label, passwordMode);
}

