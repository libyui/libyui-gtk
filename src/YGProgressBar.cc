#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YGWidget.h"

#include "YProgressBar.h"

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

	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YProgressBar)

	// YProgressBar
	virtual void setProgress (const YCPInteger& newProgress)
	{
		IMPL
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

#include "YDownloadProgress.h"

class YGDownloadProgress : public YDownloadProgress, public YGLabeledWidget
{
	guint timeout_id;

public:
	YGDownloadProgress (const YWidgetOpt &opt, YGWidget *parent,
	                    const YCPString& label,
	                    const YCPString &filename, int expectedSize)
	: YDownloadProgress (opt, label, filename, expectedSize)
	, YGLabeledWidget (this, parent, label, YD_VERT, true,
	                   GTK_TYPE_PROGRESS_BAR, NULL)
	{
		timeout_id = g_timeout_add (250, timeout_cb, this);
	}

	virtual ~YGDownloadProgress()
	{
		g_source_remove (timeout_id);
	}

	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YDownloadProgress)

	virtual void setExpectedSize (int expectedSize)
	{
		YDownloadProgress::setExpectedSize (expectedSize);
		timeout_cb(this);  // force an update
	}

	static gboolean timeout_cb (void *pData)
	{
		YGDownloadProgress *pThis = (YGDownloadProgress*) pData;
		float fraction = (float) pThis->currentFileSize() / pThis->expectedSize();
		if (fraction > 1) fraction = 1;
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pThis->getWidget()), fraction);
		return TRUE;
	}
};

YWidget *
YGUI::createDownloadProgress (YWidget *parent, YWidgetOpt &opt,
                                 const YCPString &label,
                                 const YCPString &filename,
                                 int expectedSize)
{
	IMPL;
	return new YGDownloadProgress (opt, YGWidget::get (parent),
	                               label, filename, expectedSize);
}
