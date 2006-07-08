/* Yast GTK */
#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "YMultiProgressMeter.h"
#include "ygtkratiobox.h"

class YGMultiProgressMeter : public YMultiProgressMeter, public YGWidget
{
public:
	YGMultiProgressMeter (const YWidgetOpt &opt, YGWidget *parent,
	                      bool horizontal, const YCPList &maxValues)
	: YMultiProgressMeter (opt, horizontal, maxValues)
	, YGWidget (this, parent, true, YGTK_TYPE_RATIO_BOX, NULL)
	{
		ygtk_ratio_box_set_spacing (YGTK_RATIO_BOX (getWidget()), 2);

		for (int i = 0; i < segments(); i++) {
			GtkWidget* bar = gtk_progress_bar_new();
			gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (bar),
				horizontal ? GTK_PROGRESS_LEFT_TO_RIGHT : GTK_PROGRESS_TOP_TO_BOTTOM );

			// Progress bars just ask for too much size -- let's cut it
			const int min_size = 5;
			if (horizontal)
				gtk_widget_set_size_request (bar, min_size, -1);
			else
				gtk_widget_set_size_request (bar, -1, min_size);

			gtk_container_add (GTK_CONTAINER (getWidget()), bar);
			ygtk_ratio_box_set_child_packing
				(YGTK_RATIO_BOX (getWidget()), bar, maxValue (i), TRUE, 0);
		}

		gtk_widget_show_all (getWidget());
	}

	virtual void doUpdate()
	{
		GList* children = gtk_container_get_children (GTK_CONTAINER (getWidget()));
		for (int i = 0; i < segments() && i < (signed)g_list_length (children); i++) {
			GtkProgressBar *bar = GTK_PROGRESS_BAR (g_list_nth_data (children, i));
			gfloat fraction = 0;
			if (currentValue (i) != -1)
				fraction = 1.0 - ((gfloat) currentValue(i) / maxValue(i));
			gtk_progress_bar_set_fraction (bar, fraction);
		}
		g_list_free (children);
	}

	// YWidget
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_NICESIZE
};

YWidget *
YGUI::createMultiProgressMeter (YWidget *parent, YWidgetOpt &opt,
                      bool horizontal, const YCPList &maxValues)
{
	return new YGMultiProgressMeter (opt, YGWidget::get (parent),
	                                      horizontal, maxValues);
}
