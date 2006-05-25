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

	virtual void setSize( long newWidth, long newHeight )
	{
		fprintf (stderr, "%s:%s -G%p-Y%p- %ld, %ld\n", G_STRLOC, G_STRFUNC, \
				 m_widget, m_y_widget, newWidth, newHeight);
		YSplit::setSize (newWidth, newHeight);
	}

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
