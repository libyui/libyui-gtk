#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YSpacing.h"
#include "YGWidget.h"

class YGSpacing : public YSpacing, public YGWidget
{
public:
    YGSpacing( const YWidgetOpt &opt,
			   YGWidget         *parent,
			   float 			size,
			   bool 			horizontal,
			   bool 			vertical ) :
			YSpacing( opt, size, horizontal, vertical ),
			YGWidget( this, parent, true, GTK_TYPE_FIXED, NULL ) {}
	virtual ~YGSpacing() {}

	// YWidget
    YGWIDGET_IMPL_SET_ENABLING
    YGWIDGET_IMPL_SET_SIZE
    virtual bool setKeyboardFocus() IMPL_RET(false);
    virtual void startMultipleChanges() IMPL;
    virtual void doneMultipleChanges() IMPL;
    virtual void saveUserInput( YMacroRecorder *macroRecorder ) IMPL;
};

YWidget *
YGUI::createSpacing( YWidget *parent, YWidgetOpt & opt, float size,
					 bool horizontal, bool vertical )
{
	return new YGSpacing( opt, YGWidget::get (parent),
						  size, horizontal, vertical );
}
