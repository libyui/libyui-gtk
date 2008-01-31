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

	virtual void setSize (int width, int height);
	virtual void setEnabled (bool enabled);
	virtual int  preferredWidth()  { return 0; }
	virtual int  preferredHeight() { return 0; }

        virtual void activate();
        virtual void openInternal();

	YGWIDGET_IMPL_CHILD_ADDED (m_containee)
	YGWIDGET_IMPL_CHILD_REMOVED (m_containee)
};

#endif // YGDIALOG_H

