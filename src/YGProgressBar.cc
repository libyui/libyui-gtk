#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YProgressBar.h"
#include "YGWidget.h"

class YGProgressBar : public YProgressBar, public YGWidget
{
public:
    YGProgressBar( const YWidgetOpt &opt,
				   YGWidget         *parent,
				   const YCPString & label,
				   const YCPInteger & maxprogress,
				   const YCPInteger & progress ) :
			YProgressBar( opt, label, maxprogress, progress ),
			YGWidget( this, parent, true, GTK_TYPE_PROGRESS_BAR, NULL )
	{
		if (label->value() != "")
			gtk_progress_bar_set_text (GTK_PROGRESS_BAR (getWidget()),
									   label->value_cstr());
	}
	virtual ~YGProgressBar() {}

	// YProgressBar
    virtual void setLabel( const YCPString & label )
	{
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR (getWidget()),
								   label->value_cstr());
	}
	virtual void setProgress( const YCPInteger & newProgress );

	// YWidget
    YGWIDGET_IMPL_NICESIZE
    YGWIDGET_IMPL_SET_ENABLING
    YGWIDGET_IMPL_SET_SIZE
    virtual bool setKeyboardFocus() IMPL_RET(false);
    virtual void startMultipleChanges() IMPL;
    virtual void doneMultipleChanges() IMPL;
    virtual void saveUserInput( YMacroRecorder *macroRecorder ) IMPL;
};

void YGProgressBar::setProgress( const YCPInteger & newProgress )
{
	float value = newProgress->value();

	value /= maxProgress->value();
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (getWidget()), value);
    YProgressBar::setProgress( newProgress );
}

YWidget *
YGUI::createProgressBar( YWidget *parent, YWidgetOpt & opt,
						 const YCPString & label,
						 const YCPInteger & maxprogress,
						 const YCPInteger & progress )
{
	IMPL;
	return new YGProgressBar( opt, YGWidget::get (parent),
							  label, maxprogress, progress );
}
