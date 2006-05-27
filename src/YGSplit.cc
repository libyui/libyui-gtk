#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YSplit.h"
#include "YGWidget.h"

class YGSplit : public YSplit, public YGWidget
{
public:
    YGSplit( const YWidgetOpt &opt,
			 YGWidget         *parent,
			 YUIDimension dimension ) :
			YSplit( opt, dimension ),
			YGWidget( this, parent ) IMPL;

	virtual ~YGSplit() {}

    // YSplit
	YGWIDGET_IMPL_MOVE_CHILD

	virtual long nicesize( YUIDimension dim )
	{
		long ret = YSplit::nicesize (dim);
	    fprintf (stderr, "NiceSize for '%s' %s %ld\n",
				 getWidgetName(), dim == YD_HORIZ ? "width" : "height",
				 ret);
		return ret;
	}

	YGWIDGET_IMPL_SET_SIZE_CHAIN(YSplit)

    // YWidget
    YGWIDGET_IMPL_SET_ENABLING
};

YContainerWidget *
YGUI::createSplit( YWidget *parent, YWidgetOpt & opt,
		   YUIDimension dimension )
{
	IMPL;
	return new YGSplit (opt, YGWidget::get (parent), dimension);
}
