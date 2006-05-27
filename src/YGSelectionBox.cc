#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YSelectionBox.h"
#include "YGWidget.h"

class YGSelectionBox : public YSelectionBox, public YGWidget
{
public:
    YGSelectionBox( const YWidgetOpt &opt,
		    YGWidget         *parent,
		    const YCPString &label ) :
	YSelectionBox( opt, label ),
	YGWidget( this, parent, true, GTK_TYPE_VBOX, NULL )
    {
	// FIXME: pack the label in & set it etc ...
    }
    virtual ~YGSelectionBox() {}

    // YSelectionBox
    virtual int getCurrentItem() IMPL_RET(0);
    virtual void setCurrentItem( int index ) IMPL;

    // YSelectionWidget
    virtual void itemAdded( const YCPString & string, int index, bool selected ) IMPL;
    virtual void setLabel( const YCPString & label ) IMPL;
    virtual void addItem( const YCPValue  & id,
			  const YCPString & text,
			  const YCPString & icon,
			  bool selected ) IMPL;
    virtual void deleteAllItems() IMPL;
    virtual YCPValue changeLabel( const YCPValue & newValue ) IMPL_VOID;
    virtual YCPValue changeItems ( const YCPValue & newValue ) IMPL_VOID;

    // YWidget
    YGWIDGET_IMPL_NICESIZE
    YGWIDGET_IMPL_SET_ENABLING
    YGWIDGET_IMPL_SET_SIZE
    virtual bool setKeyboardFocus() IMPL_RET(false);
    virtual void startMultipleChanges() IMPL;
    virtual void doneMultipleChanges() IMPL;
    virtual void saveUserInput( YMacroRecorder *macroRecorder ) IMPL;
};

YWidget *
YGUI::createSelectionBox( YWidget *parent, YWidgetOpt & opt,
			  const YCPString & label )
{
    IMPL;
    return new YGSelectionBox (opt, YGWidget::get (parent), label);
}
