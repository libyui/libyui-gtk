/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
#include <config.h>
#include "YGUI.h"
#include "YGDialog.h"
#include <gdk/gdkkeysyms.h>
#include <math.h>  // easter

/* In the main dialog case, it doesn't necessarly have a window of its own. If
   there is already a main window, it should replace its content -- and when closed,
   the previous dialog restored.

   Therefore, we have a YGDialog (the YDialog implementation), and a YGWindow
   that does the windowing work and has a YWidget has its children, which can
   be a YGDialog and is swap-able.
*/

#define DEFAULT_WIDTH  650
#define DEFAULT_HEIGHT 550

class YGWindow;
static YGWindow *main_window = 0;

class YGWindow
{
	GtkWidget *m_widget;
	int m_refcount;
	// we keep a pointer of the child just for debugging
	// (ie. dump yast tree)
	YWidget *m_child;
	GdkCursor *m_busyCursor;

public:
        YGWindowCloseFn m_canClose;
        void *m_canCloseData;

	YGWindow (bool _main_window, YGDialog *ydialog)
	{
		m_widget = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		g_object_ref_sink (G_OBJECT (m_widget));
		g_object_set (G_OBJECT (m_widget), "allow-shrink", TRUE, NULL);

		m_refcount = 0;
		m_child = NULL;
		m_canClose = NULL;
		m_busyCursor = NULL;

		{
			std::stack<YDialog *> &stack = YDialog::_dialogStack;
			YDialog *ylast = stack.size() ? stack.top() : 0;
			if (ylast == ydialog) {
				if (stack.size() > 1) {
					YDialog *t = ylast;
					stack.pop();
					ylast = stack.top();
					stack.push (t);
				}
				else
					ylast = NULL;
			}

			GtkWindow *parent = NULL;
			if (ylast) {
				YGDialog *yglast = static_cast <YGDialog *> (ylast);
				parent = GTK_WINDOW (yglast->m_window->getWidget());
			}
		    GtkWindow *window = GTK_WINDOW (m_widget);

		    if (parent) {
		        // if there is a parent, this would be a dialog
		        gtk_window_set_title (window, "");
		        gtk_window_set_modal (window, TRUE);
		        gtk_window_set_transient_for (window, parent);
		        gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_DIALOG);
		    }
		    else {
		        gtk_window_set_title (window, "YaST");
		        if (YGUI::ui()->unsetBorder())
		            gtk_window_set_decorated (window, FALSE);
		    }

		    if (_main_window) {
		        gtk_window_set_default_size (window, DEFAULT_WIDTH, DEFAULT_HEIGHT);
				if (YGUI::ui()->setFullscreen())
					gtk_window_fullscreen (window);
		    }

		    gtk_window_set_role (window, "yast2-gtk");
		    if (!YGUI::ui()->hasWM())
		        g_signal_connect (G_OBJECT (m_widget), "expose-event",
		                          G_CALLBACK (draw_border_cb), this);
		}

		if (_main_window)
			main_window = this;

		g_signal_connect (G_OBJECT (m_widget), "delete-event",
		                  G_CALLBACK (close_window_cb), this);
		g_signal_connect_after (G_OBJECT (m_widget), "key-press-event",
		                        G_CALLBACK (key_pressed_cb), this);
		// set busy cursor at start
		g_signal_connect_after (G_OBJECT (m_widget), "realize",
		                        G_CALLBACK (realize_cb), this);
	}

	~YGWindow()
	{
		IMPL
		setChild (NULL);
		if (m_busyCursor)
			gdk_cursor_unref (m_busyCursor);
		gtk_widget_destroy (m_widget);
		g_object_unref (G_OBJECT (m_widget));
	}

	void show()
	{ gtk_widget_show (m_widget); }

	void normalCursor()
	{
		gdk_window_set_cursor (m_widget->window, NULL);
	}

	void busyCursor()
	{
		GdkDisplay *display = gtk_widget_get_display (m_widget);
		if (!m_busyCursor) {
			m_busyCursor = gdk_cursor_new_for_display (display, GDK_WATCH);
			gdk_cursor_ref (m_busyCursor);
		}
		gdk_window_set_cursor (m_widget->window, m_busyCursor);
	}

	void setChild (YWidget *new_child)
	{
		IMPL
			GtkWidget *child = gtk_bin_get_child (GTK_BIN (m_widget));
		if (child)
		    gtk_container_remove (GTK_CONTAINER (m_widget), child);
		if (new_child) {
		    child = YGWidget::get (new_child)->getLayout();
		    gtk_container_add (GTK_CONTAINER (m_widget), child);
		}
		m_child = new_child;
	}

	static void ref (YGWindow *window)
	{
		window->m_refcount++;
	}

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
	GtkWidget *getWidget() { return m_widget; }
    YWidget   *getChild() { return m_child; }

private:
	void close()
	{
		if (!m_canClose || m_canClose (m_canCloseData))
			YGUI::ui()->sendEvent (new YCancelEvent());
	}

	static gboolean close_window_cb (GtkWidget *widget, GdkEvent  *event,
	                                 YGWindow  *pThis)
	{
		IMPL
		// never let GTK+ destroy the window! just inform YCP, and let it
		// do its thing.
		pThis->close();
		return TRUE;
	}

	static gboolean key_pressed_cb (GtkWidget *widget, GdkEventKey *event,
		                            YGWindow *pThis)
	{
		IMPL
		// if not main dialog, close it on escape
		if (event->keyval == GDK_Escape &&
		    /* not main window */ main_window != pThis) {
			pThis->close();
		    return TRUE;
		    
		}

		if (event->state & GDK_SHIFT_MASK) {
		    switch (event->keyval) {
				case GDK_F8:
				    YGUI::ui()->askSaveLogs();
				    return TRUE;
				default:
					break;
			}
		}
		if ((event->state & GDK_CONTROL_MASK) && (event->state & GDK_SHIFT_MASK)
		    && (event->state & GDK_MOD1_MASK)) {
		    yuiMilestone() << "Caught YaST2 magic key combination\n";
		    switch (event->keyval) {
				case GDK_S:
				    YGUI::ui()->makeScreenShot();
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
				    yuiMilestone() << "Starting xterm\n";
				    system ("/usr/bin/xterm &");
				    return TRUE;
				case GDK_T:
				    dumpYastTree (pThis->getChild());
				    dumpGtkTree (pThis->m_widget);
				    return TRUE;
				case GDK_H:
				    dumpYastHtml (pThis->getChild());
				    return TRUE;
				case GDK_E:  // easter egg
				    static guint explode_timeout = 0;
				    if (explode_timeout == 0)
				        explode_timeout = g_timeout_add (10000,
				                                         expode_window_timeout_cb, pThis);
				    else {
				        g_source_remove (explode_timeout);
				        explode_timeout = 0;
				    }
				    return TRUE;
				default:
					break;
		    }
		}
		return FALSE;
	}

	static gboolean draw_border_cb (GtkWidget *widget, GdkEventExpose *event,
			                        YGWindow *pThis)
	{
		IMPL
		// to avoid background from overlapping, we emit the expose to the containee
		// ourselves
		gtk_container_propagate_expose (GTK_CONTAINER (widget),
		                                gtk_bin_get_child (GTK_BIN (widget)),
		                                event);

		GtkAllocation *alloc = &widget->allocation;
		gtk_draw_shadow (gtk_widget_get_style (widget), widget->window,
		                 GTK_STATE_NORMAL, GTK_SHADOW_ETCHED_OUT,
		                 alloc->x, alloc->y, alloc->width, alloc->height);
		return TRUE;
	}

	static void realize_cb (GtkWidget *widget, YGWindow *pThis)
	{ pThis->busyCursor(); }

	static gboolean expode_window_timeout_cb (gpointer data)
	{
		YGWindow *pThis = (YGWindow *) data;
		GtkWindow *window = GTK_WINDOW (pThis->m_widget);
		srand (time (NULL));
		gint x, y;
		gtk_window_get_position (window, &x, &y);
		#if 0
		// OVAL MOVE
		for (int i = 180; i < 360+180; i++) {
		    gtk_window_move (window, x+(int)(sin((i*G_PI)/180)*50),
		                     y+(int)(cos((i*G_PI)/180)*50)+50);
		    while (gtk_events_pending())
		        gtk_main_iteration();
		    usleep (25);
		}
		#else
		// EXPLOSION
		for (int i = 0; i < 40; i++) {
		    gtk_window_move (window, x+(int)((((float)(rand())/RAND_MAX)*40)-20),
		                     y+(int)((((float)(rand())/RAND_MAX)*40)-20));
		    while (gtk_events_pending())
		        gtk_main_iteration();
		    usleep (200);
		}
		#endif
		gtk_window_move (window, x, y);
		return TRUE;
	}
};

YGDialog::YGDialog (YDialogType dialogType, YDialogColorMode colorMode)
	: YDialog (dialogType, colorMode),
	   YGWidget (this, NULL, FALSE, GTK_TYPE_HBOX, NULL)
{
    setBorder (0);
    m_containee = gtk_event_box_new();
    if (dialogType == YMainDialog && main_window)
		m_window = main_window;
    else
		m_window = new YGWindow (dialogType == YMainDialog, this);
    YGWindow::ref (m_window);

    if (colorMode != YDialogNormalColor) {
        // emulate a warning / info dialog
        GtkWidget *icon = gtk_image_new_from_stock
            (colorMode == YDialogWarnColor ? GTK_STOCK_DIALOG_WARNING : GTK_STOCK_DIALOG_INFO,
             GTK_ICON_SIZE_DIALOG);
        gtk_misc_set_alignment (GTK_MISC (icon), 0.5, 0);
        gtk_misc_set_padding   (GTK_MISC (icon), 0, 12);

        gtk_box_pack_start (GTK_BOX (getWidget()), icon,    FALSE, FALSE, 12);
        gtk_box_pack_start (GTK_BOX (getWidget()), m_containee, TRUE, TRUE, 0);
    }
    else
        gtk_box_pack_start (GTK_BOX (getWidget()), m_containee, TRUE, TRUE, 0);
    gtk_widget_show_all (getWidget());

    // NOTE: we need to add this containter to the window right here, else
    // weird stuff happens (like if we set a pango font description to a
    // GtkLabel, size request would output the size without that description
    // set...)
    m_window->setChild (this);
}

YGDialog::~YGDialog()
{
    YGWindow::unref (m_window);
}

void YGDialog::openInternal()
{
    m_window->show();
}

void YGDialog::activate()
{
    m_window->setChild (this);
}

YGDialog *YGDialog::currentDialog()
{
	YDialog *ydialog = YDialog::currentDialog (false);
	if (ydialog)
		return static_cast <YGDialog *> (ydialog);
	return NULL;
}

GtkWindow *YGDialog::currentWindow()
{
	YGDialog *ydialog = YGDialog::currentDialog();
	if (ydialog)
		return GTK_WINDOW (ydialog->m_window->getWidget());
	return NULL;
}

void YGDialog::setCloseCallback (YGWindowCloseFn canClose, void *canCloseData)
{
	m_window->m_canClose = canClose;
	m_window->m_canCloseData = canCloseData;
}

void YGDialog::unsetCloseCallback()
{
	m_window->m_canClose = NULL;
}

void YGDialog::normalCursor()
{
	m_window->normalCursor();
}

void YGDialog::busyCursor()
{
	m_window->busyCursor();
}

// YWidget

void YGDialog::setEnabled (bool enabled)
{
	doSetEnabled (enabled);
	YWidget::setEnabled (enabled);
}

void YGDialog::setSize (int width, int height)
{
	// libyui calls YDialog::setSize() to force a geometry recalculation as a
	// result of changed layout properties
	GtkWidget *window = m_window->getWidget();
	if (GTK_WIDGET_REALIZED (window)) {
		gtk_widget_queue_resize (window);
		if (!isMainDialog())
			gtk_window_resize (GTK_WINDOW (window), width, height);
	}
}

YDialog *YGWidgetFactory::createDialog (YDialogType dialogType, YDialogColorMode colorMode)
{
	IMPL
	return new YGDialog (dialogType, colorMode);
}

YEvent *YGDialog::waitForEventInternal (int timeout_millisec)
{ return YGUI::ui()->waitInput (timeout_millisec, true); }

YEvent *YGDialog::pollEventInternal()
{ return YGUI::ui()->waitInput (0, false); }

