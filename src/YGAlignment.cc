#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YAlignment.h"
#include "YGWidget.h"

class YGAlignment : public YAlignment, public YGWidget
{
public:
	YGAlignment( const YWidgetOpt &opt,
		     YGWidget         *parent,
		     YAlignmentType halign,
		     YAlignmentType valign ) :
		YAlignment( opt, halign, valign ),
		YGWidget( this, parent ) {}
	virtual ~YGAlignment() {}

	// YAlignment
	YGWIDGET_IMPL_MOVE_CHILD

	// YWidget
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE_CHAIN(YAlignment)
	virtual bool setKeyboardFocus() IMPL_RET(false);
	virtual void startMultipleChanges() IMPL;
	virtual void doneMultipleChanges() IMPL;
	virtual void saveUserInput( YMacroRecorder *macroRecorder ) IMPL;
};

YContainerWidget *
YGUI::createAlignment( YWidget *parent, YWidgetOpt & opt,
					   YAlignmentType halign,
					   YAlignmentType valign )
{
	IMPL;
	return new YGAlignment( opt, YGWidget::get (parent),
							halign, valign);
}
