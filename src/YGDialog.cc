//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"
#include <gdk/gdkkeysyms.h>

/* In the main dialog case (when opt.hasDefaultSize is set), it doesn't
   necessarly have a window of its own. If there is already a main window, it
   should replace its content -- and when closed, the previous dialog restored.

   Therefore, we have a YGDialog (the YDialog implementation), and a YGWindow
   that does the windowing work and has a YWidget has its children, which can
   be a YGDialog and is swap-able.
*/

class YGWindow;
static YGWindow *main_window = NULL;

class YGWindow
{
	GtkWidget *m_widget;
	int m_refcount;

public:
	YGWindow (bool main_window)
	{
		m_widget = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		g_object_ref (G_OBJECT (m_widget));
		gtk_object_sink (GTK_OBJECT (m_widget));

		m_refcount = 0;
		if (main_window)
			::main_window = this;

		{
			GtkWindow *parent = YGUI::ui()->currentWindow();
			GtkWindow *window = GTK_WINDOW (m_widget);

			if (parent) {
				// if there is a parent, this would be a dialog
				GdkPixbuf *icon = gtk_window_get_icon (parent);
				if (icon)
					gtk_window_set_icon (window, icon);

				gtk_window_set_transient_for (window, parent);
				gtk_window_set_title (window, "");

				gtk_window_set_modal (window, TRUE);
				gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_DIALOG);
			}
			else {
				gtk_window_set_title (window, "YaST");
				if (YGUI::ui()->unsetBorder())
					gtk_window_set_decorated (window, FALSE);
			}

			if (main_window) {
				int w = YGUI::ui()->getDefaultSize (YD_HORIZ),
				    h = YGUI::ui()->getDefaultSize (YD_VERT);
				gtk_window_set_default_size (window, w, h);
			}

			gtk_window_set_role (window, "yast-gtk");
			if (!YGUI::ui()->hasWM())
				g_signal_connect (G_OBJECT (m_widget), "expose-event",
				                  G_CALLBACK (draw_border_cb), this);
		}

		g_signal_connect (G_OBJECT (m_widget), "delete_event",
		                  G_CALLBACK (close_window_cb), this);
		g_signal_connect_after (G_OBJECT (m_widget), "key-press-event",
		                        G_CALLBACK (key_pressed_cb), this);
	}

	~YGWindow()
	{
		IMPL
		setChild (NULL);
		gtk_widget_destroy (m_widget);
		g_object_unref (G_OBJECT (m_widget));
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

	static gboolean close_window_cb (GtkWidget *widget, GdkEvent  *event,
	                                 YGWindow  *pThis)
	{
		IMPL
		YGUI::ui()->sendEvent (new YCancelEvent());
		return TRUE;
	}

	static gboolean key_pressed_cb (GtkWidget *widget, GdkEventKey *event,
	                                YGWindow *pThis)
	{
		IMPL
		// if not main dialog, close it on escape
		if (event->keyval == GDK_Escape &&
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
};

#include "YDialog.h"

class YGDialog : public YDialog, public YGWidget
{
	int m_padding;
	GtkWidget *m_containee;

	YGWindow *m_window;

public:
	YGDialog (YWidgetOpt &opt)
		: YDialog (opt),
		  YGWidget (this, NULL, FALSE, GTK_TYPE_HBOX, NULL)
	{
		setBorder (0);
		m_padding = 0;
		m_containee = gtk_event_box_new();
		if (hasDefaultSize() && main_window)
			m_window = main_window;
		else
			m_window = new YGWindow (hasDefaultSize());
		YGWindow::ref (m_window);

		if (hasWarnColor() || hasInfoColor()) {
			// emulate a warning / info dialog
			GtkWidget *icon = gtk_image_new_from_stock
				(hasWarnColor() ? GTK_STOCK_DIALOG_WARNING : GTK_STOCK_DIALOG_INFO,
				 GTK_ICON_SIZE_DIALOG);
			gtk_misc_set_alignment (GTK_MISC (icon), 0.5, 0);
			gtk_misc_set_padding   (GTK_MISC (icon), 0, 12);

			gtk_box_pack_start (GTK_BOX (getWidget()), icon,    FALSE, FALSE, 12);
			gtk_box_pack_start (GTK_BOX (getWidget()), m_containee, TRUE, TRUE, 0);

			GtkRequisition req;
			gtk_widget_size_request (icon, &req);
			m_padding = req.width + 24;
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

	virtual ~YGDialog()
	{
		YGWindow::unref (m_window);
	}

	void showWindow()
	{
		IMPL
		m_window->setChild (this);
		gtk_widget_show (m_window->getWidget());
	}

	void hideWindow()
	{
		IMPL
		m_window->setChild (NULL);
		gtk_widget_hide (m_window->getWidget());
	}

	GtkWindow *getWindow()
	{ return GTK_WINDOW (m_window->getWidget()); }

	YGWIDGET_IMPL_COMMON

	virtual void childAdded (YWidget *ychild)
	{
		// This widget must align to the top, not be in the center of the window
		YGWidget *child = YGWidget::get (ychild);
		gtk_container_add (GTK_CONTAINER (m_containee), child->getLayout());
		child->setAlignment (YAlignUnchanged, YAlignBegin);
	}
	YGWIDGET_IMPL_CHILD_REMOVED (m_containee)
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
