#ifndef YGDIALOG_H
#define YGDIALOG_H

#include "YGWidget.h"
#include "YDialog.h"

typedef bool (*YGWindowCloseFn) (void *closure);

class YGWindow
{
	GtkWidget *m_widget;
	int m_refcount;
	// we keep a pointer of the child just for debugging
	// (ie. dump yast tree)
	YWidget *m_child;
    YGWindowCloseFn m_canClose;
    void *m_canCloseData;

private:
    static gboolean close_window_cb (GtkWidget *widget, GdkEvent  *event,
                                     YGWindow  *pThis);
public:
	YGWindow (bool main_window);
	~YGWindow();
	void setChild (YWidget *new_child);
	static void ref (YGWindow *window);
	static void unref (YGWindow *window);

	void normalCursor();
	void busyCursor();

    void setCloseCallback (YGWindowCloseFn canClose, void *canCloseData);
    void unsetCloseCallback();
    void closeWindow();

	// Y(G)Widget-like methods
	GtkWidget *getWidget() { return m_widget; }
    YWidget   *getChild() { return m_child; }
};

class YGDialog : public YDialog, public YGWidget
{
	int m_padding;
	GtkWidget *m_containee;

	YGWindow *m_window;

public:
	YGDialog (YWidgetOpt &opt);
	virtual ~YGDialog();
	void showWindow();
	void hideWindow();
	GtkWindow *getWindow() { return GTK_WINDOW (m_window->getWidget()); }

    void setCloseCallback (YGWindowCloseFn closeCallback, void *closeData)
        { m_window->setCloseCallback (closeCallback, closeData); }
    void unsetCloseCallback()
        { m_window->unsetCloseCallback(); }

	void normalCursor();
	void busyCursor();

	YGWIDGET_IMPL_COMMON

	YGWIDGET_IMPL_CHILD_ADDED (m_containee)
	YGWIDGET_IMPL_CHILD_REMOVED (m_containee)
};

#endif // YGDIALOG_H

