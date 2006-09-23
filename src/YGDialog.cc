//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YDialog.h"
#include "YGWidget.h"
#include <gdk/gdkkeysyms.h>

class YGDialog : public YDialog, public YGWidget
{
	GtkRequisition m_userSize;
	GtkWidget *m_fixed;
	bool userResized () { return m_userSize.width > 0 && m_userSize.height > 0; }
	int m_padding;

public:
	YGDialog (const YWidgetOpt &opt)
		: YDialog (opt),
		  YGWidget (this, NULL, FALSE,
		            GTK_TYPE_WINDOW, "type", GTK_WINDOW_TOPLEVEL,
		            "allow_shrink", TRUE, NULL)
	{
		IMPL
		m_userSize.width = -1;
		m_userSize.height = -1;
		m_padding = 0;

		GtkWindow *window = GTK_WINDOW (getWidget());
		m_fixed = gtk_fixed_new();

		if (opt.hasWarnColor.value() || opt.hasInfoColor.value()) {
			// emulate a warning / info dialog
			GtkWidget *hbox = gtk_hbox_new (FALSE, 0);

			GtkWidget *icon = gtk_image_new_from_stock
				(opt.hasWarnColor.value() ? GTK_STOCK_DIALOG_WARNING : GTK_STOCK_DIALOG_INFO,
				 GTK_ICON_SIZE_DIALOG);
			gtk_misc_set_alignment (GTK_MISC (icon), 0.5, 0);
			gtk_misc_set_padding   (GTK_MISC (icon), 0, 12);

			gtk_box_pack_start (GTK_BOX (hbox), icon,    FALSE, FALSE, 12);
			gtk_box_pack_start (GTK_BOX (hbox), m_fixed, FALSE, FALSE, 0);

			gtk_container_add (GTK_CONTAINER (getWidget()), hbox);
			gtk_widget_show_all (hbox);

			GtkRequisition req;
			gtk_widget_size_request (icon, &req);
			m_padding = req.width + 24;
		}
		else {
			gtk_container_add (GTK_CONTAINER (getWidget()), m_fixed);
			gtk_widget_show (m_fixed);
		}

		gtk_window_set_modal (GTK_WINDOW (m_widget),
		                      !opt.hasDefaultSize.value());

		if (!opt.hasDefaultSize.value())
		{	// set it the icon of the parent (if it exists)
			GtkWindow *parent = YGUI::ui()->currentWindow();
			if (parent) {
				GdkPixbuf *icon = gtk_window_get_icon (parent);
				if (icon)
					gtk_window_set_icon (window, icon);
				gtk_window_set_transient_for (window, parent);
				gtk_window_set_title (window, "");
			}
			else
				gtk_window_set_title (window, "YaST");
		}
		else
			gtk_window_set_title (window, "YaST");

	// FIXME: set default size to getDefaultSize ?
	// gtk_window_set_default_size (GTK_WINDOW (m_widget), 250, 250);

		g_signal_connect (G_OBJECT (m_widget), "configure_event",
		                  G_CALLBACK (configure_event_cb), this);
	
		g_signal_connect (G_OBJECT (m_widget), "delete_event",
		                  G_CALLBACK (close_window_cb), this);

		g_signal_connect_after (G_OBJECT (m_widget), "key-press-event",
		                        G_CALLBACK (key_pressed_cb), this);

		if (!opt.hasDefaultSize.value()) {
			gtk_window_set_modal (window, TRUE);
			gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_DIALOG);
		}

		if (!hasDefaultSize() && !YGUI::ui()->haveWM())
			gtk_window_set_has_frame (window, TRUE);
	}

	virtual ~YGDialog()
	{ }

	// YWidget
	long nicesize (YUIDimension dim)
	{
		IMPL // FIXME - hack.
		long nice;
		if (hasDefaultSize())
		{
#ifdef IMPL_DEBUG
			fprintf (stderr, "YGDialog:: has default size\n");
#endif
			if (userResized()) {
#ifdef IMPL_DEBUG
				fprintf (stderr, "YGDialog:: has user size\n");
#endif
				nice = dim == YD_HORIZ ? m_userSize.width : m_userSize.height;

			} else {
#ifdef IMPL_DEBUG
				fprintf (stderr, "YGDialog:: get default size\n");
#endif
				nice = YGUI::ui()->defaultSize (dim);
			}
		} else {
#ifdef IMPL_DEBUG
			fprintf (stderr, "YGDialog:: no default size\n");
#endif
			nice = YDialog::nicesize (dim) + m_padding;
		}
#ifdef IMPL_DEBUG
		fprintf (stderr, "YGDialog::nicesize %s = %ld\n",
			 dim == YD_HORIZ ? "width" : "height", nice);
#endif
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
			YDialog::setSize (newWidth - m_padding, newHeight);
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
		if (event->width != widget->allocation.width ||
		    event->height != widget->allocation.height) {

#ifdef IMPL_DEBUG
			fprintf (stderr, "configure event %d, %d (%d %d)\n",
				 event->width, event->height,
				 widget->allocation.width,
				 widget->allocation.height);
#endif

			pThis->setSize (event->width, event->height);
			pThis->m_userSize.width = event->width;
			pThis->m_userSize.height = event->height;
		}

		return FALSE;
	}

	static gboolean key_pressed_cb (GtkWidget *widget, GdkEventKey *event,
	                                YGDialog *pThis)
	{
		// if not main dialog, close it on escape
		if (!event->state && event->keyval == GDK_Escape &&
		    !pThis->hasDefaultSize())
			return close_window_cb (widget, NULL, pThis);

		// special key combinations
		if ((event->state & GDK_CONTROL_MASK) && (event->state & GDK_SHIFT_MASK)
		    && (event->state & GDK_MOD1_MASK)) {
			y2milestone ("Caught YaST2 magic key combination");
			switch (event->keyval) {
				case GDK_S:
					YGUI::ui()->makeScreenShot("");
					return TRUE;
				case GDK_M:
					YGUI::ui()->toggleRecordMacro();
					return TRUE;
				case GDK_P:
					YGUI::ui()->askPlayMacro();
					return TRUE;
				case GDK_D:
					YGUI::ui()->sendEvent (new YDebugEvent());
					return TRUE;
				case GDK_X:
					y2milestone ("Starting xterm");
					system ("/usr/bin/xterm &");
					return TRUE;
				default:
					return FALSE;
			}
		}
		return FALSE;
	}
};

YDialog *
YGUI::createDialog (YWidgetOpt &opt)
{
	IMPL
	return new YGDialog (opt);
}


/* show and closeDialog() don't just show or hide a GtkWindow, but
   can actually be used to stack main dialogs. ie. if there is already
   a main window, this new window should replace the contents of main
   window with theirs. */
static std::list <YGDialog *> dialogs_stack;

void
YGUI::showDialog (YDialog *_dialog)
{
	IMPL
	// FIXME: quick solution
	YGDialog *dialog = static_cast <YGDialog *> (_dialog);

	if (dialog->hasDefaultSize()) {
		if (!dialogs_stack.empty())
			gtk_widget_hide (dialogs_stack.back()->getWidget());

		dialogs_stack.push_back (dialog);
	}

	gtk_widget_show (dialog->getWidget());

	// debug
	dumpWidgetTree (dialog->getWidget());
	dumpYastTree (dialog);
}

void
YGUI::closeDialog (YDialog *_dialog)
{
	IMPL
	YGDialog *dialog = static_cast <YGDialog *> (_dialog);
	gtk_widget_hide (dialog->getWidget());

	if (dialog->hasDefaultSize()) {
		dialogs_stack.pop_back();

		if (!dialogs_stack.empty())
			gtk_widget_show (dialogs_stack.back()->getWidget());
	}
}
