/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#ifndef YGWIDGET_H
#define YGWIDGET_H

#include <gtk/gtk.h>
#include <stdarg.h>
#include "YGUI.h"
#include "YEvent.h"

class YGWidget
{
public:
	YGWidget (YWidget *ywidget, YWidget *yparent,
	          GType type, const char *property_name, ...);
	virtual ~YGWidget();

	// get the YGWidget associated with a YWidget
	static YGWidget *get (YWidget *y_widget);

	virtual inline GtkWidget *getWidget() { return m_widget; }
	GtkWidget *getLayout() { return m_adj_size; }  // add this to a container
	virtual GtkWidget *getContainer() { return m_widget; }  // add children here

	// overload YWidget methods with these ones
	virtual bool doSetKeyboardFocus();
	virtual void doSetEnabled (bool enabled);
	virtual void doSetUseBoldFont (bool useBold);
	virtual void doAddChild (YWidget *child, GtkWidget *container);
	virtual void doRemoveChild (YWidget *child, GtkWidget *container);

	// layout
	virtual int doPreferredSize (YUIDimension dimension);
	virtual void doSetSize (int width, int height);

	// debug
	const char *getWidgetName() const { return m_ywidget->widgetClass(); }
	virtual string getDebugLabel() const  // let YWidget::debugLabel() be overloaded
	{ if (m_ywidget->hasChildren()) return string(); return m_ywidget->debugLabel(); }

	// aesthetics
	void setBorder (unsigned int border);  // in pixels
	virtual unsigned int getMinSize (YUIDimension dim) { return 0; }

protected:
	// event emission
	enum EventFlags
	{ DELAY_EVENT = 2, IGNORE_NOTIFY_EVENT = 4, IF_NOT_PENDING_EVENT = 8 };
	void emitEvent (YEvent::EventReason reason, EventFlags flags = (EventFlags) 0);

	// signal registration; use "BlockEvents (this)" to temp block all signals
	friend struct BlockEvents;
	void connect (gpointer object, const char *name,
	              GCallback callback, gpointer data, bool after = true);
	void blockSignals();
	void unblockSignals();
	struct Signals;
	friend struct Signals;
	Signals *m_signals;

	void construct (YWidget *ywidget, YWidget *yparent,
	                GType type, const char *property_name, va_list args);

	// data
	GtkWidget *m_widget, *m_adj_size;  // associated GtkWidget, and adjustment for borders
	YWidget *m_ywidget;  // associated YWidget
};

struct BlockEvents
{
	BlockEvents (YGWidget *widget) : m_widget (widget)
	{ m_widget->blockSignals(); }
	~BlockEvents()
	{ m_widget->unblockSignals(); }

	private: YGWidget *m_widget;
};

/*
 * Macros to help implement proxies between common YWidget virtual
 * methods and the (multiply inherited) YGWidget base implementation
 * for GTK+.
 */
#define YGWIDGET_IMPL_COMMON(ParentClass)                       \
	virtual bool setKeyboardFocus() {                           \
		return doSetKeyboardFocus(); }                          \
	virtual void setEnabled (bool enabled) {                    \
		ParentClass::setEnabled (enabled);                      \
		doSetEnabled (enabled);                                 \
	}                                                           \
	virtual int  preferredWidth()  { return doPreferredSize (YD_HORIZ); } \
	virtual int  preferredHeight() { return doPreferredSize (YD_VERT); }  \
	virtual void setSize (int width, int height) { doSetSize (width, height); }

#define YGWIDGET_IMPL_USE_BOLD(ParentClass)                     \
    virtual void setUseBoldFont (bool useBold) {                \
    	ParentClass::setUseBoldFont (useBold);                  \
    	doSetUseBoldFont (useBold);                             \
    }

#define YGWIDGET_IMPL_CONTAINER(ParentClass)                    \
	YGWIDGET_IMPL_COMMON (ParentClass)                          \
	virtual void addChild (YWidget *ychild) {                   \
		ParentClass::addChild (ychild);                         \
		doAddChild (ychild, getContainer());                    \
	}                                                           \
	virtual void removeChild (YWidget *ychild) {                \
		ParentClass::removeChild (ychild);                      \
		doRemoveChild (ychild, getContainer());                 \
	}

/* This is a convenience class that allows for a label next to the
   intended widget. It should be used, in case you have the need for
   such, as it gives an uniform API. */
class YGLabeledWidget : public YGWidget
{
	public:
		YGLabeledWidget(YWidget *ywidget, YWidget *yparent,
		                const std::string &label_text, YUIDimension label_ori,
		                GType type, const char *property_name, ...);
		virtual ~YGLabeledWidget () {}

		virtual inline GtkWidget* getWidget() { return m_field; }

		void setLabelVisible (bool show);
		void setBuddy (GtkWidget *widget);
		virtual void doSetLabel (const std::string &label);

		YUIDimension orientation() { return m_orientation; }
		GtkWidget *getLabelWidget() { return m_label; }

	protected:
		GtkWidget *m_label, *m_field;
		YUIDimension m_orientation;
};

#define YGLABEL_WIDGET_IMPL(ParentClass)                   \
	YGWIDGET_IMPL_COMMON (ParentClass)                     \
	virtual void setLabel (const std::string &label) {     \
		ParentClass::setLabel (label);                     \
		doSetLabel (label);                                \
	}

/* This is a convenience class for widgets that need scrollbars. */
class YGScrolledWidget : public YGLabeledWidget
{
	public:
		YGScrolledWidget(YWidget *ywidget, YWidget *yparent,
		                 GType type, const char *property_name, ...);
		// if you want a label, use:
		YGScrolledWidget(YWidget *ywidget, YWidget *yparent,
		                 const std::string &label_text, YUIDimension label_ori,
		                 GType type, const char *property_name, ...);
		virtual ~YGScrolledWidget () {}

		virtual inline GtkWidget *getWidget() { return m_widget; }

		// you should use this method, not gtk_scrolled_window_set...
		void setPolicy (GtkPolicyType hpolicy, GtkPolicyType vpolicy);

	protected:
		void construct(GType type, const char *property_name, va_list args);
		GtkWidget *m_widget;
};

#endif /*YGWIDGET_H*/

