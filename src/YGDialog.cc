//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YDialog.h"
#include "YGWidget.h"
#include <gdk/gdkkeysyms.h>

/* NOTE: In the main dialog case (when opt.hasDefaultSize is set), it doesn't
   necessarly have a window of its own. If there is already a main window, it
   should replace its content -- and when closed, the previous dialog restored.
   For that, we use two classes:
     YGWindow: this one is a simple class that does the window-ing work.
     YGDialog: does the YDialog work and has a refence to some YGWindow, which
     may not be exclusive in the case of a main dialog.
     showDialog() and hideDialog() are the calls that set the current dialog.
*/

/*
  FIXME: YGWindow should know nothing about YGDialog. Instead, it should
  be a GObject (or GtkWindow-inherited) and use Gtk signal mechanism. */

class YGWindow;
static YGWindow *main_window = NULL;

class YGWindow
{
	GtkRequisition m_userSize;

	GtkWidget *m_widget;
	int m_refcount;

	// we need some cross-reference here for setSize()
	YDialog *m_ydialog;

public:
	YGWindow (bool main_window)
	{
		m_widget = (GtkWidget *) g_object_new (GTK_TYPE_WINDOW,
			"type", GTK_WINDOW_TOPLEVEL, "allow_shrink", TRUE, NULL);
		g_object_ref (G_OBJECT (m_widget));
		gtk_object_sink (GTK_OBJECT (m_widget));

		m_userSize.width = m_userSize.height = -1;
		m_refcount = 0;
		m_ydialog = 0;
		if (main_window)
			::main_window = this;

		if (main_window)
			gtk_window_set_title (GTK_WINDOW (m_widget), "YaST");
		else {
			// set the icon of the parent (if it exists)
			GtkWindow *parent = YGUI::ui()->currentWindow();
			GtkWindow *window = GTK_WINDOW (m_widget);
			if (parent) {
				GdkPixbuf *icon = gtk_window_get_icon (parent);
				if (icon)
					gtk_window_set_icon (window, icon);

				gtk_window_set_transient_for (window, parent);
				gtk_window_set_title (window, "");
			}
			else
				gtk_window_set_title (window, "YaST");

			gtk_window_set_modal (GTK_WINDOW (m_widget), TRUE);
			gtk_window_set_type_hint (GTK_WINDOW (m_widget),
			                          GDK_WINDOW_TYPE_HINT_DIALOG);
			if (!YGUI::ui()->haveWM())
				gtk_window_set_has_frame (GTK_WINDOW (m_widget), TRUE);
		}

		g_signal_connect (G_OBJECT (m_widget), "configure_event",
		                  G_CALLBACK (configure_event_cb), this);
		g_signal_connect (G_OBJECT (m_widget), "delete_event",
		                  G_CALLBACK (close_window_cb), this);
		g_signal_connect (G_OBJECT (m_widget), "key-press-event",
		                  G_CALLBACK (key_pressed_cb), NULL);
	}

	~YGWindow()
	{
		IMPL
		setChild (NULL, NULL);
		gtk_widget_destroy (m_widget);
		g_object_unref (G_OBJECT (m_widget));
	}

	void setChild (GtkWidget *new_child, YDialog *new_child_y)
	{
		IMPL
		GtkWidget *child = gtk_bin_get_child (GTK_BIN (m_widget));
		if (child)
			gtk_container_remove (GTK_CONTAINER (m_widget), child);
		if (new_child) {
			gtk_container_add (GTK_CONTAINER (m_widget), new_child);

			// signal our size to the dialog
			if (new_child_y->hasChildren()) {
				int width, height;
				gtk_window_get_size (GTK_WINDOW (m_widget), &width, &height);
				new_child_y->child (0)->setSize (width, height);
			}
		}
		m_ydialog = new_child_y;
	}

	// YGDialog should not destroy its YGWindow. Instead, they should use this
	// simple ref-count mechanism to ensure there is no other window open.
	static void ref (YGWindow *window)
	{ window->m_refcount++; }
	static void unref (YGWindow *window)
	{
		if (--window->m_refcount == 0) {
			bool is_main_window = (window == main_window);
			delete window;
			if (is_main_window)
				main_window = NULL;
		}
	}

	// Y(G)Widget-like methods
	GtkWidget *getWidget()
	{ return m_widget; }

	bool userResized ()
	{ return m_userSize.width > 0 && m_userSize.height > 0; }

	long getSize (YUIDimension dim)
	{
		if (dim == YD_HORIZ)
			return m_userSize.width;
		else
			return m_userSize.height;
	}

	void setSize (long width, long height)
	{
		IMPL
		int reqw, reqh;
		gtk_window_get_default_size (GTK_WINDOW (m_widget), &reqw, &reqh);
		if (reqw == width && reqh == height)
				return;
		gtk_window_resize (GTK_WINDOW (m_widget), width, height);
		if (m_ydialog && m_ydialog->hasChildren())
			m_ydialog->child (0)->setSize (width /*- m_padding*/, height);
	}

	static gboolean close_window_cb (GtkWidget *widget, GdkEvent  *event,
	                                 YGWindow  *pThis)
	{
		IMPL
		YGUI::ui()->sendEvent (new YCancelEvent());
		return TRUE;
	}

	static gboolean configure_event_cb (GtkWidget *widget,
	                                    GdkEventConfigure *event,
	                                    YGWindow  *pThis)
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
	                                YGWindow *pThis)
	{
		IMPL
		// if not main dialog, close it on escape
		if (!event->state && event->keyval == GDK_Escape &&
		    /* not main window */ main_window != pThis)
			return close_window_cb (widget, NULL, pThis);

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

class YGDialog : public YDialog, public YGWidget
{
	GtkWidget *m_fixed;
	int m_padding;

	YGWindow *m_window;

public:
	YGDialog (YWidgetOpt &opt)
		: YDialog (opt),
		  YGWidget (this, NULL, FALSE, GTK_TYPE_HBOX, NULL)
	{
		m_padding = 0;
		if (hasDefaultSize() && main_window)
			m_window = main_window;
		else
			m_window = new YGWindow (hasDefaultSize());
		YGWindow::ref (m_window);

		m_fixed = gtk_fixed_new();

		if (hasWarnColor() || hasInfoColor()) {
			// emulate a warning / info dialog
			GtkWidget *icon = gtk_image_new_from_stock
				(hasWarnColor() ? GTK_STOCK_DIALOG_WARNING : GTK_STOCK_DIALOG_INFO,
				 GTK_ICON_SIZE_DIALOG);
			gtk_misc_set_alignment (GTK_MISC (icon), 0.5, 0);
			gtk_misc_set_padding   (GTK_MISC (icon), 0, 12);

			gtk_box_pack_start (GTK_BOX (getWidget()), icon,    FALSE, FALSE, 12);
			gtk_box_pack_start (GTK_BOX (getWidget()), m_fixed, TRUE, TRUE, 0);

			GtkRequisition req;
			gtk_widget_size_request (icon, &req);
			m_padding = req.width + 24;
		}
		else
			gtk_box_pack_start (GTK_BOX (getWidget()), m_fixed, TRUE, TRUE, 0);
		gtk_widget_show_all (getWidget());

		// NOTE: we need to add this containter to the window, else weird stuff
		// happens (like if we set a pango font description to a GtkLabel, size
		// request would output the size without that description set...)
		m_window->setChild (m_widget, this);
	}

	virtual ~YGDialog()
	{
		YGWindow::unref (m_window);
	}

	void showWindow()
	{
		IMPL
		m_window->setChild (m_widget, this);
		gtk_widget_show (m_window->getWidget());
	}

	void hideWindow()
	{
		IMPL
		gtk_widget_hide (m_window->getWidget());
	}

	// YGWidget
	GtkFixed *getFixed()
	{ return GTK_FIXED (m_fixed); }

	GtkWindow *getWindow()
	{ return GTK_WINDOW (m_window->getWidget()); }

	long nicesize (YUIDimension dim)
	{
		IMPL
		long nice;
		if (hasDefaultSize()) {
			if (m_window && m_window->userResized())
				nice = m_window->getSize (dim);
			else
				nice = YGUI::ui()->defaultSize (dim);
		}
		else
			nice = child (0)->nicesize (dim) + m_padding;
		return nice;
	}

	void setSize (long width, long height)
	{
		IMPL
		m_window->setSize (width, height);
	}
};

YDialog *
YGUI::createDialog (YWidgetOpt &opt)
{
	IMPL
	return new YGDialog (opt);
}

static list <YGDialog *> dialogs_stack;

void
YGUI::showDialog (YDialog *_dialog)
{
	IMPL
	YGDialog *dialog = static_cast <YGDialog *> (_dialog);
	if (dialog->hasDefaultSize())
		dialogs_stack.push_back (dialog);
	dialog->showWindow();

	// debug
	dumpWidgetTree (dialog->getWidget());
	dumpYastTree (dialog);
}

void
YGUI::closeDialog (YDialog *_dialog)
{
	IMPL
	YGDialog *dialog = static_cast <YGDialog *> (_dialog);
	if (dialog->hasDefaultSize()) {
		dialogs_stack.pop_back();
		if (!dialogs_stack.empty()) {
			YGDialog *old_dialog = dialogs_stack.back();
			old_dialog->showWindow();
		}
		else
			dialog->hideWindow();
	}
	else
		dialog->hideWindow();
}

GtkWindow *
YGUI::currentWindow()
{
	YDialog *ydialog = YGUI::ui()->currentDialog();
	if (ydialog)
		return ((YGDialog *) ydialog)->getWindow();
	else
		return NULL;
}
