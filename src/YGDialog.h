#ifndef YGDIALOG_H
#define YGDIALOG_H

#include "YGWidget.h"
#include "YDialog.h"

class YGWindow;
typedef bool (*YGWindowCloseFn) (void *closure);

class YGDialog : public YDialog, public YGWidget
{
	friend class YGWindow;
	GtkWidget *m_containee;
	YGWindow *m_window;
	bool m_stickyTitle;

public:
	YGDialog (YDialogType dialogType, YDialogColorMode colorMode);
	virtual ~YGDialog();

	virtual GtkWidget *getContainer() { return m_containee; }

    void setCloseCallback (YGWindowCloseFn closeCallback, void *closeData);
    void unsetCloseCallback();

	void normalCursor();
	void busyCursor();

	// convenience function to be used rather than currentDialog()
	static YGDialog *currentDialog();
	static GtkWindow *currentWindow();

	virtual void doSetSize (int width, int height);
	void setMinSize (int width, int height);  // used internally by YGPackageSelector.cc

	virtual void openInternal();
	virtual void activate();
	void present();

	virtual YEvent *waitForEventInternal (int timeout_millisec);
	virtual YEvent *pollEventInternal();

    virtual void highlight (YWidget * child);

	void setTitle (const std::string &title, bool sticky = false);
	void setIcon (const std::string &icon);

	YGWIDGET_IMPL_CONTAINER (YDialog)
};

#endif // YGDIALOG_H

