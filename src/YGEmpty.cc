#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YEmpty.h"
#include "YGWidget.h"

class YGEmpty : public YEmpty, public YGWidget
{
public:
    YGEmpty( const YWidgetOpt &opt,
			 YGWidget         *parent ) :
			YEmpty( opt ),
			YGWidget( this, parent ) {}
	virtual ~YGEmpty() {}

	// YWidget
    YGWIDGET_IMPL_SET_ENABLING
    YGWIDGET_IMPL_SET_SIZE
    virtual bool setKeyboardFocus() IMPL_RET(false);
    virtual void startMultipleChanges() IMPL;
    virtual void doneMultipleChanges() IMPL;
    virtual void saveUserInput( YMacroRecorder *macroRecorder ) IMPL;
};

YWidget *
YGUI::createEmpty( YWidget *parent, YWidgetOpt & opt )
{
	IMPL;
	return new YGEmpty( opt, YGWidget::get (parent) );
}
