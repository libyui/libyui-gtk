#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YDialog.h"
#include "YGWidget.h"

class YGGenericButton;
class YGWizard;

class YGDialog : public YDialog, public YGWidget
{
	GtkRequisition m_userSize;
	GtkWidget *m_fixed;
	bool userResized () { return m_userSize.width > 0 && m_userSize.height > 0; }
public:
	YGDialog (const YWidgetOpt &opt)
		: YDialog (opt),
		  YGWidget (this, NULL, FALSE,
		            GTK_TYPE_WINDOW, "type", GTK_WINDOW_TOPLEVEL,
		            "allow_shrink", TRUE, NULL)
	{
		m_userSize.width = -1;
		m_userSize.height = -1;

		g_object_set (G_OBJECT (getWidget()), NULL);

		m_fixed = gtk_fixed_new();
		gtk_widget_show (m_fixed);
		gtk_container_add (GTK_CONTAINER (getWidget()), m_fixed);

		fprintf (stderr, "%s (%s)\n", G_STRLOC, G_STRFUNC);
		gtk_window_set_modal (GTK_WINDOW (m_widget),
		                      !opt.hasDefaultSize.value());
		gtk_window_set_title (GTK_WINDOW (m_widget),
		                      (!opt.hasDefaultSize.value()) ?  "Yast2" : "");
	// FIXME: set default size to getDefaultSize ?
	// gtk_window_set_default_size (GTK_WINDOW (m_widget), 250, 250);

		g_signal_connect (G_OBJECT (m_widget), "configure_event",
		                  G_CALLBACK (configure_event_cb), this);
	
		g_signal_connect (G_OBJECT (m_widget), "delete_event",
		                  G_CALLBACK (close_window_cb), this);

		if (opt.hasWarnColor.value()) {
			GdkColor color = { 0, 0xf000, 0xb900, 0xb900 };  // red tone
			gtk_widget_modify_bg (getWidget(), GTK_STATE_NORMAL, &color);
		}
		else if (opt.hasInfoColor.value()) {
			GdkColor color = { 0, 0xf500, 0xffff, 0x8000 };  // yellow tone
			gtk_widget_modify_bg (getWidget(), GTK_STATE_NORMAL, &color);
		}

		if (!opt.hasDefaultSize.value()) {
			gtk_window_set_modal (GTK_WINDOW (getWidget()), TRUE);
			gtk_window_set_type_hint (GTK_WINDOW (getWidget()),
			                          GDK_WINDOW_TYPE_HINT_DIALOG);
		}

		if (!hasDefaultSize() && !YGUI::ui()->haveWM())
			gtk_window_set_has_frame (GTK_WINDOW (getWidget()), TRUE);
	}

	virtual ~YGDialog()
	{ }

	// YWidget
	long nicesize (YUIDimension dim)
	{
		IMPL // FIXME - hack.
		long nice;
		fprintf (stderr, "YGDialog::nicesize\n");
		if (hasDefaultSize())
		{
			if  (userResized())
				nice = dim == YD_HORIZ ? m_userSize.width : m_userSize.height;
			else
				nice = YGUI::ui()->defaultSize (dim);
		}
		else
			nice = YDialog::nicesize (dim);
		return nice;
	}

	void setSize (long newWidth, long newHeight)
	{
		int reqw, reqh;

		gtk_window_get_default_size (GTK_WINDOW (m_widget), &reqw, &reqh);
		if (reqw == newWidth && reqh == newHeight)
				return;
#ifdef IMPL_DEBUG
		fprintf (stdout, "YGDialog::setSize %ld, %ld (%d,%d)\n",
		         newWidth, newHeight, reqw, reqh);
#endif
		gtk_window_resize (GTK_WINDOW (m_widget), newWidth, newHeight);
//		gtk_window_set_default_size (GTK_WINDOW (m_widget), newWidth, newHeight);

		if (hasChildren())
			YDialog::setSize (newWidth, newHeight);
	}

	// YGWidget
	GtkFixed *getFixed()
	{
		return GTK_FIXED (m_fixed);
	}

	static gboolean close_window_cb (GtkWidget *widget,
	                                 GdkEvent  *event,
	                                 YGDialog  *pThis)
	{
		YGUI::ui()->sendEvent (new YCancelEvent());
		return TRUE;
	}

	static gboolean configure_event_cb (GtkWidget *widget,
	                                    GdkEventConfigure *event,
	                                    YGDialog  *pThis)
	{
		fprintf (stderr, "configure event %d, %d\n",
		         event->width, event->height);
		pThis->setSize (event->width, event->height);
		pThis->m_userSize.width = event->width;
		pThis->m_userSize.height = event->height;

		return FALSE;
	}
};

YDialog *
YGUI::createDialog (YWidgetOpt &opt)
{
	IMPL;
	return new YGDialog (opt);
}

void
YGUI::showDialog (YDialog *dialog)
{
	IMPL;
	gtk_widget_show (YGWidget::get (dialog)->getWidget());
	dumpWidgetTree (YGWidget::get (dialog)->getWidget());
	dumpYastTree (dialog);
}

void
YGUI::closeDialog (YDialog *dialog)
{
	IMPL; // FIXME - destroy ? lifecycle etc. ...
	gtk_widget_hide (YGWidget::get (dialog)->getWidget());
}
