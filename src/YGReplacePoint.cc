#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YReplacePoint.h"
#include "YGWidget.h"

class YGReplacePoint : public YReplacePoint, public YGWidget
{
public:
    YGReplacePoint( const YWidgetOpt &opt,
					YGWidget         *parent );
	virtual ~YGReplacePoint();

    // YContainerWidget
	virtual void childAdded( YWidget *child );

    // YWidget
    YGWIDGET_IMPL_SET_ENABLING
    virtual bool setKeyboardFocus() IMPL_RET(false);
    virtual void startMultipleChanges() IMPL;
    virtual void doneMultipleChanges() IMPL;
    virtual void saveUserInput( YMacroRecorder *macroRecorder ) IMPL;
};

YGReplacePoint::YGReplacePoint( const YWidgetOpt &opt,
								YGWidget         *parent )
	: YReplacePoint( opt ),
	  YGWidget( this, parent )
{
	IMPL;
}

YGReplacePoint::~YGReplacePoint()
{
}

void YGReplacePoint::childAdded( YWidget * child )
{
	YGWidget::get(child)->show();
}

YContainerWidget *
YGUI::createReplacePoint( YWidget *parent, YWidgetOpt & opt )
{
	return new YGReplacePoint (opt, YGWidget::get (parent));
}
