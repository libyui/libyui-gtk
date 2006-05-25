#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YTextEntry.h"
#include "YGWidget.h"

class YGTextEntry : public YTextEntry, public YGWidget
{
	GtkWidget *m_label;
	GtkWidget *m_entry;
public:
    YGTextEntry( const YWidgetOpt &opt,
				   YGWidget         *parent,
				   const YCPString &label,
				   const YCPString &text ) :
			YTextEntry( opt, label ),
			YGWidget( this, parent, true,
					  GTK_TYPE_VBOX, NULL )
	{
		// FIXME: get alignment right here [ GtkAlignment ? ]
		GtkWidget *align_left = gtk_vbox_new (FALSE, 0);
		gtk_widget_show (align_left);
		gtk_box_pack_start (GTK_BOX (getWidget()), align_left, FALSE, TRUE, 0);
		
		m_label = gtk_label_new ("");
		if (label->value() != "")
			gtk_widget_show (m_label);
		gtk_box_pack_start (GTK_BOX (align_left), m_label, TRUE, TRUE, 0);
		setLabel (label);

		// LineEdit (?)
		m_entry = gtk_entry_new ();
		gtk_widget_show (m_entry);
		gtk_box_pack_start (GTK_BOX (getWidget()), m_entry, TRUE, TRUE, 0);
		setText (text);

		// FIXME: set the label 'buddy' [ an a11y issue ? ] to the entry.
		
		// FIXME: connect to changed, linechanged
		if ( opt.passwordMode.value() )
			gtk_entry_set_visibility (GTK_ENTRY (getWidget()), FALSE);
	}
	virtual ~YGTextEntry() {}

	// YTextEntry
    virtual void setText( const YCPString & text );
    virtual YCPString getText()
	{
		return YCPString (gtk_entry_get_text (GTK_ENTRY (m_entry)));
	}
    virtual void setLabel( const YCPString & label )
	{
		gtk_label_set_text (GTK_LABEL (m_label),
							label->value_cstr());
	    YTextEntry::setLabel( label );
	}
    virtual void setValidChars( const YCPString & validChars )
	{
		IMPL;
		YTextEntry::setValidChars( validChars );
	}
    virtual void setInputMaxLength( const YCPInteger & numberOfChars )
	{
		gtk_entry_set_width_chars (GTK_ENTRY (m_entry),
								   numberOfChars->asInteger()->value());
	}

	// YWidget
    YGWIDGET_IMPL_NICESIZE
    YGWIDGET_IMPL_SET_ENABLING
    YGWIDGET_IMPL_SET_SIZE
    virtual bool setKeyboardFocus() IMPL_RET(false);
    virtual void startMultipleChanges() IMPL;
    virtual void doneMultipleChanges() IMPL;
    virtual void saveUserInput( YMacroRecorder *macroRecorder ) IMPL;
};

void
YGTextEntry::setText( const YCPString & text )
{
	// FIXME: Block signals
	gtk_entry_set_text (GTK_ENTRY (m_entry), text->value_cstr());
	// FIXME: UnBlock signals
}

YWidget *
YGUI::createTextEntry( YWidget *parent, YWidgetOpt & opt,
					   const YCPString & label, const YCPString & text )
{
	IMPL;
	return new YGTextEntry( opt, YGWidget::get (parent), label, text );
}
