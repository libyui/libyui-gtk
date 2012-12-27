/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/*
  Textdomain "gtk"
 */

#define YUILogComponent "gtk"
#include <yui/Libyui_config.h>
#include "YGUI.h"
#include "YGWidget.h"
#include "YGi18n.h"

std::string size_stdform (YFileSize_t size)
{
	long double mantissa = size;
	int unit = 0;
	for (; mantissa/1024 > 1; unit++)
		mantissa /= 1024;

	const char *unit_str = "";
	switch (unit) {
		// translator: byte shorthand
		case 0: unit_str = _("B"); break;
		case 1: unit_str = _("KB"); break;
		case 2: unit_str = _("MB"); break;
		case 3: unit_str = _("GB"); break;
		case 4: unit_str = _("TB"); break;
		default: mantissa = 0; break;
	}

	gchar *text = g_strdup_printf ("%.1f %s", (float) mantissa, unit_str);
	std::string str (text);
	g_free (text);
	return str;
}

#include "YProgressBar.h"

class YGProgressBar : public YProgressBar, public YGLabeledWidget
{
public:
	YGProgressBar (YWidget *parent, const std::string &label, int maxValue)
	: YProgressBar (NULL, label, maxValue)
		// NOTE: its label widget is positionated at the vertical, because its label
		// may change often and so will its size, which will look odd (we may want
		// to make the label widget to only grow).
	, YGLabeledWidget (this, parent, label, YD_VERT, GTK_TYPE_PROGRESS_BAR, NULL)
	{}

	// YProgressBar
	virtual void setValue (int value)
	{
		YProgressBar::setValue (value);
		GtkProgressBar *bar = GTK_PROGRESS_BAR (getWidget());
		float fraction = CLAMP ((float) value / maxValue(), 0, 1);
		gtk_progress_bar_set_fraction (bar, fraction);
		gtk_main_iteration_do(false);
/*
		char *text = g_strdup_printf ("%d %%", (int) (fraction*100));
		gtk_progress_bar_set_text (bar, text);
		g_free (text);
*/
	}

	virtual unsigned int getMinSize (YUIDimension dim)
	{ return dim == YD_HORIZ ? 200 : 0; }

	YGLABEL_WIDGET_IMPL (YProgressBar)
};

YProgressBar *YGWidgetFactory::createProgressBar (YWidget *parent, const std::string &label,
                                                  int maxValue)
{
	return new YGProgressBar (parent, label, maxValue);
}

#include "YDownloadProgress.h"

class YGDownloadProgress : public YDownloadProgress, public YGLabeledWidget
{
guint timeout_id;

public:
	YGDownloadProgress (YWidget *parent, const std::string &label,
	                    const std::string &filename, YFileSize_t expectedFileSize)
	: YDownloadProgress (NULL, label, filename, expectedFileSize)
	, YGLabeledWidget (this, parent, label, YD_HORIZ, GTK_TYPE_PROGRESS_BAR, NULL)
	{
		timeout_id = g_timeout_add (250, timeout_cb, this);
	}

	virtual ~YGDownloadProgress()
	{
		g_source_remove (timeout_id);
	}

	virtual void setExpectedSize (YFileSize_t size)
	{
		YDownloadProgress::setExpectedSize (size);
		timeout_cb (this);  // force an update
	}

	static gboolean timeout_cb (void *pData)
	{
		YGDownloadProgress *pThis = (YGDownloadProgress*) pData;
		GtkProgressBar *bar = GTK_PROGRESS_BAR (pThis->getWidget());

		gtk_progress_bar_set_fraction (bar, pThis->currentPercent() / 100.0);
		if (pThis->expectedSize() > 0) {
			std::string current (size_stdform (pThis->currentFileSize()));
			std::string total (size_stdform (pThis->expectedSize()));
			std::string text = current + " " + _("of") + " " + total;
			gtk_progress_bar_set_text (GTK_PROGRESS_BAR (bar), text.c_str());
		}
		return TRUE;
	}

	YGLABEL_WIDGET_IMPL (YDownloadProgress)
};

YDownloadProgress *YGOptionalWidgetFactory::createDownloadProgress (YWidget *parent,
		const std::string &label, const std::string &filename, YFileSize_t expectedFileSize)
{ return new YGDownloadProgress (parent, label, filename, expectedFileSize); }

#include "ygtkratiobox.h"
#include "YMultiProgressMeter.h"

class YGMultiProgressMeter : public YMultiProgressMeter, public YGWidget
{
public:
	YGMultiProgressMeter (YWidget *parent, YUIDimension dim, const std::vector <float> &maxValues)
	: YMultiProgressMeter (NULL, dim, maxValues)
	, YGWidget (this, parent,
	            horizontal() ? YGTK_TYPE_RATIO_HBOX : YGTK_TYPE_RATIO_VBOX, NULL)
	{
		ygtk_ratio_box_set_spacing (YGTK_RATIO_BOX (getWidget()), 2);
		for (int s = 0; s < segments(); s++) {
			GtkWidget *bar = gtk_progress_bar_new();
                        gtk_orientable_set_orientation(GTK_ORIENTABLE(bar), horizontal() ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);
                        gtk_progress_bar_set_inverted(GTK_PROGRESS_BAR (bar), horizontal() ? FALSE : TRUE);
			// Progress bars would ask for too much size with weight...
			const int min_size = 5;
			if (horizontal())
				gtk_widget_set_size_request (bar, min_size, -1);
			else
				gtk_widget_set_size_request (bar, -1, min_size);
			ygtk_ratio_box_pack (YGTK_RATIO_BOX (getWidget()), bar, getSegmentWeight (s));
		}

		ygtk_adj_size_set_max (YGTK_ADJ_SIZE (m_adj_size), horizontal() ? 200 : 0,
		                                                   horizontal() ? 0 : 200);
		gtk_widget_show_all (getWidget());
	}

	virtual void doUpdate()
	{
		GList* children = gtk_container_get_children (GTK_CONTAINER (getWidget()));
		int s = 0;
		for (GList *i = children; i && s < segments(); i = i->next, s++) {
			GtkProgressBar *bar = GTK_PROGRESS_BAR (i->data);
			gtk_progress_bar_set_fraction (bar, getSegmentValue (s));
		}
		g_list_free (children);
	}

	int getSegmentWeight (int n)
	{
		if (vertical())
			n = (segments() - n) - 1;
		return maxValue (n);
	}
	float getSegmentValue (int n)
	{
		if (vertical())
			n = (segments() - n) - 1;
		if (currentValue (n) < 0)
			return 0;
		return 1.0 - (((float) currentValue (n)) / maxValue (n));
	}

	YGWIDGET_IMPL_COMMON (YMultiProgressMeter)
};

YMultiProgressMeter *YGOptionalWidgetFactory::createMultiProgressMeter (YWidget *parent,
		YUIDimension dim, const std::vector <float> &maxValues)
{ return new YGMultiProgressMeter (parent, dim, maxValues); }

#include "YBusyIndicator.h"

/* YBusyIndicator semantics are pretty contrived. It seems we should animate the widget
   until timeout is reached. The application will ping setAlive(true) calls -- and we
   reset the timeout -- as an indication that the program hasn't hang in some operation. */

#define PULSE_INTERVAL 100
#define PULSE_STEP  0.050

class YGBusyIndicator : public YBusyIndicator, public YGLabeledWidget
{
guint pulse_timeout_id;
int alive_timeout;

public:
	YGBusyIndicator (YWidget *parent, const std::string &label, int timeout)
	: YBusyIndicator (NULL, label, timeout)
	, YGLabeledWidget (this, parent, label, YD_VERT,
	                   GTK_TYPE_PROGRESS_BAR, "pulse-step", PULSE_STEP, NULL)
	{
		pulse_timeout_id = 0;
		pulse();
	}

	virtual ~YGBusyIndicator()
	{ stop(); }

	void pulse()
	{
		alive_timeout = timeout();
		if (!pulse_timeout_id)
			pulse_timeout_id = g_timeout_add (PULSE_INTERVAL, pulse_timeout_cb, this);
	}

	void stop()
	{
		alive_timeout = 0;
		if (pulse_timeout_id) {
			g_source_remove (pulse_timeout_id);
			pulse_timeout_id = 0;
		}
	}

	// YBusyIndicator
    virtual void setAlive (bool alive)
    {
    	YBusyIndicator::setAlive (alive);
    	alive ? pulse() : stop();
    }

	// callbacks
	static gboolean pulse_timeout_cb (void *pData)
	{
		YGBusyIndicator *pThis = (YGBusyIndicator*) pData;
		gtk_progress_bar_pulse (GTK_PROGRESS_BAR (pThis->getWidget()));
		pThis->alive_timeout -= PULSE_INTERVAL;
		if (pThis->alive_timeout <= 0) {
			pThis->pulse_timeout_id = 0;
			return FALSE;
		}
			return TRUE;
	}

	YGLABEL_WIDGET_IMPL (YBusyIndicator)
};

YBusyIndicator *YGWidgetFactory::createBusyIndicator (YWidget *parent, const std::string &label, int timeout)
{ return new YGBusyIndicator (parent, label, timeout); }

