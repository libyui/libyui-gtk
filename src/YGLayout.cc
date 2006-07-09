/* Yast GTK */

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGLayout.h"

// Look for YGLayout.h for the actual implementation
// Since these classes are so thin and are only included
// in one point, there isn't anything to win by splitting them.

YContainerWidget *
YGUI::createSplit (YWidget *parent, YWidgetOpt &opt,
                   YUIDimension dimension)
{
	IMPL
	return new YGSplit (opt, YGWidget::get (parent), dimension);
}

YWidget *
YGUI::createEmpty (YWidget *parent, YWidgetOpt &opt)
{
	IMPL
	return new YGEmpty (opt, YGWidget::get (parent));
}

YWidget *
YGUI::createSpacing (YWidget *parent, YWidgetOpt & opt, float size,
                     bool horizontal, bool vertical)
{
	IMPL
	return new YGSpacing (opt, YGWidget::get (parent),
	                      size, horizontal, vertical);
}

YContainerWidget *
YGUI::createReplacePoint( YWidget *parent, YWidgetOpt & opt )
{
	IMPL
	return new YGReplacePoint (opt, YGWidget::get (parent));
}

YContainerWidget *
YGUI::createSquash (YWidget *parent, YWidgetOpt &opt,
                    bool hsquash, bool vsquash)
{
	IMPL
	return new YGSquash (opt, YGWidget::get (parent), hsquash, vsquash);
}

YContainerWidget *
YGUI::createAlignment (YWidget *parent, YWidgetOpt &opt,
                       YAlignmentType halign, YAlignmentType valign)
{
	IMPL
	return new YGAlignment (opt, YGWidget::get (parent),
	                        halign, valign);
}
