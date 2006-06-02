#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YSplit.h"
#include "YEmpty.h"
#include "YSpacing.h"
#include "YGWidget.h"

#define YGWIDGET_DEBUG_NICESIZE_CHAIN(ParentClass) \
	virtual long nicesize( YUIDimension dim ) \
	{ \
		long ret = ParentClass::nicesize (dim); \
		fprintf (stderr, "NiceSize for '%s' %s %ld\n", \
			 getWidgetName(), dim == YD_HORIZ ? "width" : "height", ret); \
		return ret; \
	}

#define YGWIDGET_CONTAINER_IMPL(ParentClass) \
	YGWIDGET_IMPL_SET_ENABLING \
	YGWIDGET_DEBUG_NICESIZE_CHAIN(ParentClass) \
	YGWIDGET_IMPL_SET_SIZE_CHAIN(ParentClass)


// YSplit

class YGSplit : public YSplit, public YGWidget
{
public:
	YGSplit( const YWidgetOpt &opt,
		 YGWidget         *parent,
		 YUIDimension dimension ) :
		YSplit( opt, dimension ),
		YGWidget( this, parent ) {}
	// YSplit
	YGWIDGET_IMPL_MOVE_CHILD
	// YWidget
	YGWIDGET_CONTAINER_IMPL(YSplit)
};

YContainerWidget *
YGUI::createSplit( YWidget *parent, YWidgetOpt & opt,
		   YUIDimension dimension )
{
	IMPL;
	return new YGSplit (opt, YGWidget::get (parent), dimension);
}

// YEmpty

class YGEmpty : public YEmpty, public YGWidget
{
public:
	YGEmpty( const YWidgetOpt &opt,
		 YGWidget         *parent ) :
		YEmpty( opt ),
		YGWidget( this, parent ) {}
	// YWidget
	YGWIDGET_CONTAINER_IMPL(YEmpty)
};

YWidget *
YGUI::createEmpty( YWidget *parent, YWidgetOpt & opt )
{
    IMPL;
    return new YGEmpty( opt, YGWidget::get (parent) );
}

// YSpacing

class YGSpacing : public YSpacing, public YGWidget
{
public:
	YGSpacing (const YWidgetOpt &opt,
		   YGWidget         *parent,
		   float 		 size,
		   bool 		 horizontal,
		   bool 		 vertical ) :
		YSpacing (opt, size, horizontal, vertical),
		YGWidget(this, parent) {}
	YGWIDGET_CONTAINER_IMPL(YSpacing)
};

YWidget *
YGUI::createSpacing (YWidget *parent, YWidgetOpt & opt, float size,
		     bool horizontal, bool vertical)
{
	IMPL;
	return new YGSpacing (opt, YGWidget::get (parent),
			      size, horizontal, vertical);
}
