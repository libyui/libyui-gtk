#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YProgressBar.h"
#include "YGWidget.h"

class YGProgressBar : public YProgressBar, public YGLabeledWidget
{
public:
	YGProgressBar (const YWidgetOpt &opt, YGWidget *parent,
	               const YCPString& label,
	               const YCPInteger& maxprogress, const YCPInteger& progress)
	: YProgressBar (opt, label, maxprogress, progress)
	, YGLabeledWidget (this, parent, label, YD_VERT, true,
	                   GTK_TYPE_PROGRESS_BAR, NULL)
	{ }

	virtual ~YGProgressBar() {}

	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YProgressBar)
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE

	// YProgressBar
	virtual void setProgress (const YCPInteger& newProgress)
	{
		float fraction = (float) newProgress->value() / maxProgress->value();
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (getWidget()), fraction);
		YProgressBar::setProgress (newProgress);
	}
};

YWidget *
YGUI::createProgressBar (YWidget* parent, YWidgetOpt& opt,
                         const YCPString& label,
                         const YCPInteger& maxprogress,
                         const YCPInteger& progress )
{
	IMPL;
	return new YGProgressBar (opt, YGWidget::get (parent),
	                          label, maxprogress, progress);
}
