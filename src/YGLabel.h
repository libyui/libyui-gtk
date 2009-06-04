/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#ifndef YGLABEL_H
#define YGLABEL_H
#define YUILogComponent "gtk"
#include <config.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YGWidget.h"

#include "YLabel.h"

class YGLabel : public YLabel, public YGWidget
{
public:
	YGLabel (YWidget *parent, const string &text, bool heading, bool outputField);

	virtual void setText (const string &label);

	YGWIDGET_IMPL_COMMON (YLabel)
	YGWIDGET_IMPL_USE_BOLD (YLabel)
};
#endif
