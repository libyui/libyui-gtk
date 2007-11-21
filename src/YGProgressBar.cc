/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"

#include "YProgressBar.h"

class YGProgressBar : public YProgressBar, public YGLabeledWidget
{
    bool m_pulse;
public:
	YGProgressBar (const YWidgetOpt &opt, YGWidget *parent,
	               const YCPString& label,
	               const YCPInteger& maxprogress, const YCPInteger& progress)
	: YProgressBar (opt, label, maxprogress, progress)
	, YGLabeledWidget (this, parent, label, YD_VERT, true,
	                   GTK_TYPE_PROGRESS_BAR, NULL)
	{
        m_pulse = maxprogress->value() <= 0;
    }
	// NOTE: its label widget is positionated at the vertical, because its label
	// may change often and so will its size, which will look odd (we may want
	// to make the label widget to only grow).

	virtual ~YGProgressBar() {}

	// YProgressBar
	virtual void setProgress (const YCPInteger& newProgress)
	{
		IMPL
        if (m_pulse)
            gtk_progress_bar_pulse (GTK_PROGRESS_BAR (getWidget()));
        else {
            float fraction = (float) newProgress->value() / maxProgress->value();
            gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (getWidget()), fraction);
        }
        YProgressBar::setProgress (newProgress);
	}

	YGWIDGET_IMPL_COMMON
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YProgressBar)
};

YWidget *
YGUI::createProgressBar (YWidget* parent, YWidgetOpt& opt,
                         const YCPString& label, const YCPInteger& maxprogress,
                         const YCPInteger& progress)
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
	                    const YCPString& label, const YCPString &filename,
	                    int expectedSize)
	: YDownloadProgress (opt, label, filename, expectedSize)
	, YGLabeledWidget (this, parent, label, YD_HORIZ, true,
	                   GTK_TYPE_PROGRESS_BAR, NULL)
	{
		timeout_id = g_timeout_add (250, timeout_cb, this);
	}

	virtual ~YGDownloadProgress()
	{
		g_source_remove (timeout_id);
	}

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

	YGWIDGET_IMPL_COMMON
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YDownloadProgress)
};

YWidget *
YGUI::createDownloadProgress (YWidget *parent, YWidgetOpt &opt,
                              const YCPString &label, const YCPString &filename,
                              int expectedSize)
{
	IMPL
	return new YGDownloadProgress (opt, YGWidget::get (parent),
	                               label, filename, expectedSize);
}

#include "ygtkratiobox.h"
#include "YMultiProgressMeter.h"

class YGMultiProgressMeter : public YMultiProgressMeter, public YGWidget
{
public:
	YGMultiProgressMeter (const YWidgetOpt &opt, YGWidget *parent,
	                      bool horizontal, const YCPList &maxValues)
	: YMultiProgressMeter (opt, horizontal, maxValues)
	, YGWidget (this, parent, true,
	            horizontal ? YGTK_TYPE_RATIO_HBOX : YGTK_TYPE_RATIO_VBOX, NULL)
	{
//		ygtk_ratio_box_set_homogeneous (YGTK_RATIO_BOX (getWidget()), TRUE);
		ygtk_ratio_box_set_spacing (YGTK_RATIO_BOX (getWidget()), 2);

		for (int i = segments()-1; i >= 0; i--) {
			GtkWidget* bar = gtk_progress_bar_new();
			gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (bar),
				horizontal ? GTK_PROGRESS_LEFT_TO_RIGHT : GTK_PROGRESS_BOTTOM_TO_TOP);
			ygtk_ratio_box_pack (YGTK_RATIO_BOX (getWidget()), bar,
			                     maxValue (i), TRUE, TRUE, 0);
		}

		ygtk_adj_size_set_max (YGTK_ADJ_SIZE (m_adj_size), horizontal ? 200 : 0,
		                                                   horizontal ? 0 : 200);
		gtk_widget_show_all (getWidget());
	}

	virtual void doUpdate()
	{
		GList* children = gtk_container_get_children (GTK_CONTAINER (getWidget()));
		int n = segments()-1;
		for (GList *i = children; i && n >= 0; i = i->next, n--) {
			GtkProgressBar *bar = GTK_PROGRESS_BAR (i->data);
			gfloat fraction = 0;
			if (currentValue (n) != -1)
				fraction = 1.0 - ((gfloat) currentValue(n) / maxValue(n));
			gtk_progress_bar_set_fraction (bar, fraction);
		}
		g_list_free (children);
	}

	YGWIDGET_IMPL_COMMON
};

YWidget *
YGUI::createMultiProgressMeter (YWidget *parent, YWidgetOpt &opt,
                      bool horizontal, const YCPList &maxValues)
{
	return new YGMultiProgressMeter (opt, YGWidget::get (parent),
	                                      horizontal, maxValues);
}
