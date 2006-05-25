#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YLabel.h"
#include "YGWidget.h"

class YGLabel : public YLabel, public YGWidget
{
	const char *m_start_tag;
	const char *m_end_tag;
public:
    YGLabel( const YWidgetOpt &opt,
	     YGWidget         *parent,
	     YCPString         text );
    virtual ~YGLabel();

    // YLabel
    virtual void setLabel( const YCPString & label );

    // YWidget
    virtual void childDeleted( YWidget *child ) IMPL;
    YGWIDGET_IMPL_NICESIZE
    virtual bool stretchable( YUIDimension dimension ) const IMPL_RET(true);
    YGWIDGET_IMPL_SET_SIZE
    YGWIDGET_IMPL_SET_ENABLING
    virtual bool setKeyboardFocus() IMPL_RET(false);
    virtual void startMultipleChanges() IMPL;
    virtual void doneMultipleChanges() IMPL;
    virtual void saveUserInput( YMacroRecorder *macroRecorder ) IMPL;
};

YGLabel::YGLabel( const YWidgetOpt &opt,
		  YGWidget         *parent,
		  YCPString         text )
	:  YLabel( opt, text ),
	   YGWidget( this, parent, true, GTK_TYPE_LABEL, NULL )
{
	IMPL;
    if ( opt.isHeading.value() ) {
		m_start_tag = "span size=\"larger\"";
		m_end_tag = "span";
	} else if (opt.boldFont.value())
		m_start_tag = m_end_tag = "b";
	else
		m_start_tag = m_end_tag = NULL;

    if ( opt.isOutputField.value() )
		fprintf (stderr, "Should use an insensitive entry instead\n");

	setLabel (text);
}

YGLabel::~YGLabel()
{
}

void
YGLabel::setLabel( const YCPString & label )
{
	IMPL;
	char *escaped = g_markup_escape_text (label->value_cstr(), -1);
	char *str;
	if (m_start_tag) {
			str = g_strdup_printf ("<%s>%s</%s>",
								   m_start_tag, escaped, m_end_tag);
			g_free (escaped);
	} else
			str = escaped;
	fprintf (stderr, "Set label '%s'\n", str);
	gtk_label_set_markup (GTK_LABEL (getWidget()), str);
	g_free (str);
}

YWidget *
YGUI::createLabel( YWidget *parent, YWidgetOpt & opt,
		   const YCPString & text )
{
	return new YGLabel (opt, YGWidget::get (parent), text);
}
