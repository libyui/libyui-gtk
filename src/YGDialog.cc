/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#define YUILogComponent "gtk"
#include <yui/Libyui_config.h>
#include "YGUI.h"
#include "YGDialog.h"
#include "YGUtils.h"
#include <YDialogSpy.h>
#include <YPushButton.h>
#include <gdk/gdkkeysyms.h>
#include <math.h>  // easter
#include <string.h>
#include "ygtkwindow.h"
#include "YGMacros.h"


/* In the main dialog case, it doesn't necessarly have a window of its own. If
   there is already a main window, it should replace its content -- and when closed,
   the previous dialog restored.

   Therefore, we have a YGDialog (the YDialog implementation), and a YGWindow
   that does the windowing work and has a YWidget has its children, which can
   be a YGDialog and is swap-able.
*/

//#define DEFAULT_WIDTH  750
//#define DEFAULT_HEIGHT 650
#define DEFAULT_CHAR_WIDTH  60
#define DEFAULT_CHAR_HEIGHT 28
#define DEFAULT_PIXEL_WIDTH  330
#define DEFAULT_PIXEL_HEIGHT 200

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
	bool m_isBusy;

public:
	YGWindowCloseFn m_canClose;
	void *m_canCloseData;

	YGWindow (bool _main_window, YGDialog *ydialog)
	{
		m_widget = ygtk_window_new();

#		if GTK_CHECK_VERSION (3, 12, 0)
#		else
		gtk_container_set_resize_mode (GTK_CONTAINER (m_widget), GTK_RESIZE_PARENT);
#		endif

		g_object_ref_sink (G_OBJECT (m_widget));
		gtk_window_set_has_resize_grip (GTK_WINDOW (m_widget), TRUE);

		m_refcount = 0;
		m_child = NULL;
		m_canClose = NULL;
		m_busyCursor = NULL;
		m_isBusy = false;

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
			// to be back compatible
		    std::string dialogTitle = "YaSt";

#ifdef LIBYUI_VERSION_NUM
 #if LIBYUI_VERSION_AT_LEAST(2,42,3)	
		    dialogTitle = YUI::app()->applicationTitle();
 #endif
#endif
		    if (parent) {
		        // if there is a parent, this would be a dialog
		        gtk_window_set_title (window, dialogTitle.c_str());
		        gtk_window_set_modal (window, TRUE);
		        gtk_window_set_transient_for (window, parent);
		        gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_DIALOG);
				AtkObject *peer = gtk_widget_get_accessible (GTK_WIDGET (window));
				if (peer != NULL)
					atk_object_set_role (peer, ATK_ROLE_DIALOG);
		    }
		    else {
						gtk_window_set_title (window, dialogTitle.c_str());
#ifdef LIBYUI_VERSION_NUM
 #if LIBYUI_VERSION_AT_LEAST(2,42,3)	
						GdkPixbuf *pixbuf = YGUtils::loadPixbuf (YUI::app()->applicationIcon());
						if (pixbuf) {  // default window icon
							gtk_window_set_default_icon (pixbuf);
							g_object_unref (G_OBJECT (pixbuf));
						}
 #endif
#endif
		        if (YGUI::ui()->unsetBorder())
		            gtk_window_set_decorated (window, FALSE);
		    }

			if (_main_window) {
				// window default width is calculated as a proportion of a default
				// char and pixel width to compensate for the fact that each widget's
				// required size comes from a proportion of both parameters
				int width = YGUtils::getCharsWidth (m_widget, DEFAULT_CHAR_WIDTH);
				width += DEFAULT_PIXEL_WIDTH;
				int height = YGUtils::getCharsHeight (m_widget, DEFAULT_CHAR_HEIGHT);
				height += DEFAULT_PIXEL_HEIGHT;

		    	if (YGUI::ui()->isSwsingle())
		    		height += YGUtils::getCharsHeight (m_widget, 10);

			width = MIN (width, YUI::app()->displayWidth());
			height = MIN (height, YUI::app()->displayHeight());

		        gtk_window_set_default_size (window, width, height);
		        gtk_window_resize(window, width, height);

			if (YGUI::ui()->setFullscreen())
				gtk_window_fullscreen (window);
			else if (YUI::app()->displayWidth() <= 800 || YUI::app()->displayHeight() <= 600)
				// maximize window for small displays
				gtk_window_maximize (window);
		    }

		    gtk_window_set_role (window, "yast2");
		}

		if (_main_window)
			main_window = this;

		g_signal_connect (G_OBJECT (m_widget), "delete-event",
		                  G_CALLBACK (close_window_cb), this);
		g_signal_connect_after (G_OBJECT (m_widget), "key-press-event",
		                        G_CALLBACK (key_pressed_cb), this);
		g_signal_connect (G_OBJECT (m_widget), "focus-in-event",
		                  G_CALLBACK (focus_in_event_cb), this);
		// set busy cursor at start
		g_signal_connect_after (G_OBJECT (m_widget), "realize",
		                        G_CALLBACK (realize_cb), this);
	}

	~YGWindow()
	{
		setChild (NULL);
		if (m_busyCursor)
                        g_object_unref (G_OBJECT (m_busyCursor));
		gtk_widget_destroy (m_widget);
		g_object_unref (G_OBJECT (m_widget));
	}

	void show()
	{ gtk_widget_show (m_widget); }

	void normalCursor()
	{
		if (m_isBusy)
                    gdk_window_set_cursor (gtk_widget_get_window(m_widget), NULL);
		m_isBusy = false;
	}

	void busyCursor()
	{
		if (!m_busyCursor) {
			GdkDisplay *display = gtk_widget_get_display (m_widget);
			m_busyCursor = gdk_cursor_new_for_display (display, GDK_WATCH);
			g_object_ref (G_OBJECT (m_busyCursor));
		}
		if (!m_isBusy) {
			GdkDisplay *display = gtk_widget_get_display (m_widget);
			gdk_window_set_cursor (gtk_widget_get_window(m_widget), m_busyCursor);
			gdk_display_sync(display);
		}
		m_isBusy = true;
	}

	void setChild (YWidget *new_child)
	{
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
		// never let GTK+ destroy the window! just inform YCP, and let it
		// do its thing.
		pThis->close();
		return TRUE;
	}

	static gboolean key_pressed_cb (GtkWidget *widget, GdkEventKey *event,
		                            YGWindow *pThis)
	{
		// if not main dialog, close it on escape
		if (event->keyval == GDK_KEY_Escape &&
		    /* not main window */ main_window != pThis) {
			pThis->close();
		    return TRUE;

		}

		if (event->state & GDK_SHIFT_MASK) {
		    switch (event->keyval) {
				case GDK_KEY_F8:
				    YGUI::ui()->askSaveLogs();
				    return TRUE;
				default:
					break;
			}
		}
		if ((event->state & GDK_CONTROL_MASK) && (event->state & GDK_SHIFT_MASK)
		    && (event->state & GDK_MOD1_MASK)) {
		    yuiMilestone() << "Caught YaST2 magic key combination\n";
			int ret = -1;
		    switch (event->keyval) {
				case GDK_KEY_S:
				    YGUI::ui()->makeScreenShot();
				    return TRUE;
				case GDK_KEY_M:
				    YGUI::ui()->toggleRecordMacro();
				    return TRUE;
				case GDK_KEY_P:
				    YGUI::ui()->askPlayMacro();
				    return TRUE;
				case GDK_KEY_D:
				    YGUI::ui()->sendEvent (new YDebugEvent());
				    return TRUE;
				case GDK_KEY_X:
				    yuiMilestone() << "Starting xterm\n";
				    ret = system ("/usr/bin/xterm &");
				    if (ret != 0)
				    	yuiError() << "Can't launch xterm (error code" << ret << ")" << std::endl;
				    return TRUE;
				case GDK_KEY_Y:
					yuiMilestone() << "Opening dialog spy" << std::endl;
					YDialogSpy::showDialogSpy();
					YGUI::ui()->normalCursor();
					break;
				default:
					break;
		    }
		}
		return FALSE;
	}

	static gboolean focus_in_event_cb (GtkWidget *widget, GdkEventFocus *event)
	{ gtk_window_set_urgency_hint (GTK_WINDOW (widget), FALSE); return FALSE; }

	static void realize_cb (GtkWidget *widget, YGWindow *pThis)
	{ pThis->busyCursor(); }
};

YGDialog::YGDialog (YDialogType dialogType, YDialogColorMode colorMode)
	: YDialog (dialogType, colorMode),
	   YGWidget (this, NULL, YGTK_HBOX_NEW(0), NULL)
{
    setBorder (0);
    m_stickyTitle = false;
    m_containee = gtk_event_box_new();
    if (dialogType == YMainDialog && main_window)
		m_window = main_window;
    else
		m_window = new YGWindow (dialogType == YMainDialog, this);
    YGWindow::ref (m_window);

    if (colorMode != YDialogNormalColor) {
        // emulate a warning / info dialog
          GtkWidget *icon = gtk_image_new_from_icon_name 
            (colorMode == YDialogWarnColor ? "dialog-warning" : "dialog-information",
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

void YGDialog::setDefaultButton(YPushButton* newDefaultButton)
{
   YDialog::setDefaultButton( 0 ); // prevent complaints about multiple default buttons
   if ( newDefaultButton ) 
   {
     newDefaultButton->setKeyboardFocus();
     YDialog::setDefaultButton(newDefaultButton);
   }
}

void YGDialog::openInternal()
{
    m_window->show();
}

void YGDialog::activate()
{
    m_window->setChild (this);
}

void YGDialog::present()
{
	GtkWindow *window = GTK_WINDOW (m_window->getWidget());
	if (!gtk_window_is_active (window))
		gtk_window_set_urgency_hint (window, TRUE);
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

void YGDialog::doSetSize (int width, int height)
{
	// libyui calls YDialog::setSize() to force a geometry recalculation as a
	// result of changed layout properties
	bool resize = false;
	GtkWidget *window = m_window->getWidget();

        gint w,h;
        gtk_window_get_size(GTK_WINDOW (window), &w, &h);
        
        if (w < width || h < height) {
            resize = true;
            width  = MAX (width,  w),
            height = MAX (height, h);
        }                        

	if (gtk_widget_get_realized (window)) {
		gtk_widget_queue_resize (window);
		width = MIN (width, YUI::app()->displayWidth());
		height = MIN (height, YUI::app()->displayHeight());
		if (isMainDialog()) {
			GtkAllocation allocation;
			gtk_widget_get_allocation(window, &allocation);
			if (allocation.width < width || allocation.height < height) {
				resize = true;
				width = MAX (width, allocation.width),
				height = MAX (height, allocation.height);
			}
		}
		else
			resize = true;
	}
	if (resize)
		gtk_window_resize (GTK_WINDOW (window), width, height);
	else
		gtk_window_set_default_size (GTK_WINDOW (window), width, height);
}

void YGDialog::highlight (YWidget *ywidget)
{
	struct inner {
		static gboolean draw_highlight_cb (GtkWidget *widget, cairo_t *cr)
		{
			int w = gtk_widget_get_allocated_width(widget);
			int h = gtk_widget_get_allocated_height(widget);

			cairo_rectangle (cr, 0, 0, w, h);
			cairo_set_source_rgb (cr, 0xff/255.0, 0x88/255.0, 0);
			cairo_fill (cr);
			return FALSE;
		}

		static bool hasWindow (GtkWidget *widget)
		{
			if (gtk_widget_get_has_window(widget))
				return true;
			// widgets like GtkButton add their windows to parent's
			for (GList *children = gdk_window_peek_children (gtk_widget_get_window(widget));
				 children; children = children->next) {
				GdkWindow *child = (GdkWindow *) children->data;
				gpointer data;
				gdk_window_get_user_data (child, &data);
				if ((GtkWidget *) data == widget)
					return true;
			}
			return false;
		}

	};
	static YWidget *previousWidget = NULL;
	if (previousWidget && previousWidget->isValid()) {
		YGWidget *prev = YGWidget::get (previousWidget);
		if (prev) {
			GtkWidget *widget = prev->getWidget();
			if (inner::hasWindow (widget)) {
				gtk_widget_override_background_color (widget, GTK_STATE_FLAG_NORMAL, NULL);
				gtk_widget_override_color (widget, GTK_STATE_FLAG_NORMAL, NULL);
			}
			else {
				g_signal_handlers_disconnect_by_func (widget,
					(gpointer) inner::draw_highlight_cb, NULL);
				gtk_widget_queue_draw (widget);
			}
		}
	}
	if (ywidget) {
		YGWidget *ygwidget = YGWidget::get (ywidget);
		if (ygwidget) {
			GtkWidget *widget = ygwidget->getWidget();
			if (inner::hasWindow (widget)) {
				GdkRGBA bg_color = { 0, 0xffff, 0xaaaa, 0 };
				GdkRGBA base_color = { 0, 0xffff, 0xeeee, 0 };
				gtk_widget_override_background_color (widget, GTK_STATE_FLAG_NORMAL, &bg_color);
				gtk_widget_override_color (widget, GTK_STATE_FLAG_NORMAL, &base_color);
			}
			else {
				g_signal_connect (G_OBJECT (widget), "draw",
				                  G_CALLBACK (inner::draw_highlight_cb), NULL);
				gtk_widget_queue_draw (widget);
			}
		}
	}
	previousWidget = ywidget;
}

void YGDialog::setTitle (const std::string &title, bool sticky)
{
	if (title.empty())
		return;
	if (!m_stickyTitle || sticky) {
		GtkWindow *window = GTK_WINDOW (m_window->getWidget());
		gchar *str = g_strdup_printf ("%s - YaST", title.c_str());
		gtk_window_set_title (window, str);
		g_free (str);
		m_stickyTitle = sticky;
	}
	present();
}

extern "C" {
	void ygdialog_setTitle (const gchar *title, gboolean sticky);
};

void ygdialog_setTitle (const gchar *title, gboolean sticky)
{
	YGDialog::currentDialog()->setTitle (title, sticky);
}

void YGDialog::setIcon (const std::string &icon)
{
	GtkWindow *window = GTK_WINDOW (m_window->getWidget());
	GdkPixbuf *pixbuf = YGUtils::loadPixbuf (icon);
	if (pixbuf) {
		gtk_window_set_icon (window, pixbuf);
		g_object_unref (G_OBJECT (pixbuf));
	}
}

typedef bool (*FindWidgetsCb) (YWidget *widget, void *data) ;

static void findWidgets (
	std::list <YWidget *> *widgets, YWidget *widget, FindWidgetsCb find_cb, void *cb_data)
{
	if (find_cb (widget, cb_data))
		widgets->push_back (widget);
	for (YWidgetListConstIterator it = widget->childrenBegin();
	     it != widget->childrenEnd(); it++)
		findWidgets (widgets, *it, find_cb, cb_data);
}

static bool IsFunctionWidget (YWidget *widget, void *data)
{ return widget->functionKey() == GPOINTER_TO_INT (data); }

YWidget *YGDialog::getFunctionWidget (int key)
{
	std::list <YWidget *> widgets;
	findWidgets (&widgets, this, IsFunctionWidget, GINT_TO_POINTER (key));
	return widgets.empty() ? NULL : widgets.front();
}

static bool IsClassWidget (YWidget *widget, void *data)
{ return !strcmp (widget->widgetClass(), (char *) data); }

std::list <YWidget *> YGDialog::getClassWidgets (const char *className)
{
	std::list <YWidget *> widgets;
	findWidgets (&widgets, this, IsClassWidget, (void *) className);
	return widgets;
}

YDialog *YGWidgetFactory::createDialog (YDialogType dialogType, YDialogColorMode colorMode)
{ return new YGDialog (dialogType, colorMode); }

YEvent *YGDialog::waitForEventInternal (int timeout_millisec)
{ return YGUI::ui()->waitInput (timeout_millisec, true); }

YEvent *YGDialog::pollEventInternal()
{ return YGUI::ui()->waitInput (0, false); }

