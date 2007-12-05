#ifndef YGDIALOG_H
#define YGDIALOG_H

#include "YGWidget.h"
#include "YDialog.h"

class YGWindow;
typedef bool (*YGWindowCloseFn) (void *closure);

class YGDialog : public YDialog, public YGWidget
{
	GtkWidget *m_containee;
	YGWindow *m_window;

public:
	YGDialog (YDialogType dialogType, YDialogColorMode colorMode);
	virtual ~YGDialog();
	void showWindow();
	void hideWindow();

    void setCloseCallback (YGWindowCloseFn closeCallback, void *closeData);
    void unsetCloseCallback();

	void normalCursor();
	void busyCursor();

	// convenience function to be used rather than currentDialog()
	static YGDialog *currentDialog();
	static GtkWindow *currentWindow();

	YGWIDGET_IMPL_COMMON
	YGWIDGET_IMPL_CHILD_ADDED (m_containee)
	YGWIDGET_IMPL_CHILD_REMOVED (m_containee)
};

#endif // YGDIALOG_H

