//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YTextEntry.h"
#include "YGWidget.h"
#include "ygtkfieldentry.h"

class YGTextEntry : public YTextEntry, public YGLabeledWidget
{
public:
	YGTextEntry (const YWidgetOpt &opt, YGWidget *parent,
	             const YCPString &label, const YCPString &text)
	: YTextEntry (opt, label),
	  YGLabeledWidget (this, parent, label, YD_VERT, true,
	                   YGTK_TYPE_FILTER_ENTRY, NULL)
	{
		setText (text);

		if (opt.passwordMode.value())
			gtk_entry_set_visibility (GTK_ENTRY (getWidget()), FALSE);

		// Signals to know of any text modification
		g_signal_connect (G_OBJECT (getWidget()), "changed",
		                  G_CALLBACK (text_changed_cb), this);
	}

	virtual ~YGTextEntry() {}

	// YTextEntry
	virtual YCPString getText()
	{
		return YCPString (gtk_entry_get_text (GTK_ENTRY (getWidget())));
	}

	virtual void setText (const YCPString &text)
	{
		/* No need to check for valid chars as that's the responsible of the YCP
		   application programmer. */
		gtk_entry_set_text (GTK_ENTRY (getWidget()), text->value_cstr());
	}

	virtual void setInputMaxLength (const YCPInteger &numberOfChars)
	{
		gtk_entry_set_width_chars (GTK_ENTRY (getWidget()),
		                           numberOfChars->asInteger()->value());
	}

	virtual void setValidChars (const YCPString &validChars)
	{
		ygtk_filter_entry_set_valid_chars (YGTK_FILTER_ENTRY (getWidget()),
		                                   validChars->value_cstr());
		YTextEntry::setValidChars (validChars);
	}

	static void text_changed_cb (GtkEditable *editable, YGTextEntry *pThis)
	{
		pThis->emitEvent (YEvent::ValueChanged);
	}

	YGWIDGET_IMPL_COMMON
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN(YTextEntry)
};

YWidget *
YGUI::createTextEntry (YWidget *parent, YWidgetOpt & opt,
                       const YCPString & label, const YCPString & text)
{
	return new YGTextEntry (opt, YGWidget::get (parent), label, text);
}
