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

	virtual GtkWidget *getWidget() { return m_widget; }
	// containers should use this call rather than getWidget()
	GtkWidget *getLayout() { return m_adj_size; }

	// Get the YGWidget associated with a YWidget
	static YGWidget *get (YWidget *y_widget);

	// debug
	const char *getWidgetName() const { return m_ywidget->widgetClass(); }
	virtual string getDebugLabel() const  // let YWidget::debugLabel() be overloaded
	{	// container debug labels are worse than useless
		if (m_ywidget->hasChildren()) return string();
		return m_ywidget->debugLabel(); }

	// for YWidget
	virtual bool doSetKeyboardFocus();
	virtual void doSetEnabled (bool enabled);
	virtual void doAddChild (YWidget *child, GtkWidget *container);
	virtual void doRemoveChild (YWidget *child, GtkWidget *container);
	void doSetUseBoldFont (bool useBold);

	// Event handling
	enum EventFlags {
		DELAY_EVENT = 2, IGNORE_NOTIFY_EVENT = 4, IF_NOT_PENDING_EVENT = 8,
		NORMAL_EVENT = 0 };
	void emitEvent (YEvent::EventReason reason, EventFlags flags = NORMAL_EVENT);

	// Signal registration (use "BlockEvents (this)" when to block these)
	void connect (GObject *object, const char *name,
	               GCallback callback, gpointer data);
	inline void connect (GtkWidget *widget, const char *name,
	                      GCallback callback, gpointer data)
	{ connect (G_OBJECT (widget), name, callback, data); }
	void connect_after (GObject *object, const char *name,
	                     GCallback callback, gpointer data);
	inline void connect_after (GtkWidget *widget, const char *name,
	                             GCallback callback, gpointer data)
	{ connect_after (G_OBJECT (widget), name, callback, data); }

	// Aesthetics
	void setBorder (unsigned int border);  // in pixels
	virtual unsigned int getMinSize (YUIDimension dim) { return 0; }

protected:
	// layout
	virtual int getPreferredSize (YUIDimension dimension);
	void doSetSize (int width, int height);

	GtkWidget *m_widget;  // associated GtkWidget -- use getWidget()
	GtkWidget *m_adj_size;  // installed on m_widget, allows better size constrains
	YWidget *m_ywidget;  // associated YWidget

	friend struct BlockEvents;
	void connectSignal (GObject *object, const char *name,
	                     GCallback callback, gpointer data, bool after);
	void blockSignals();
	void unblockSignals();
	struct Signals;
	friend struct Signals;
	Signals *m_signals;

	void construct (YWidget *ywidget, YWidget *yparent,
	                GType type, const char *property_name, va_list args);
};

struct BlockEvents
{
BlockEvents (YGWidget *widget)
	: m_widget (widget)
	{ m_widget->blockSignals(); }
~BlockEvents()
	{ m_widget->unblockSignals(); }
private:
	YGWidget *m_widget;
};

/*
 * Macros to help implement proxies between common YWidget virtual
 * methods and the (multiply inherited) YGWidget base implementation
 * for GTK+.
 */
#define YGWIDGET_IMPL_COMMON                                    \
	virtual bool setKeyboardFocus() {                           \
		return doSetKeyboardFocus(); }                          \
	virtual void setEnabled (bool enabled) {                    \
		YWidget::setEnabled (enabled);                          \
		doSetEnabled (enabled);                                 \
	}                                                           \
	virtual int  preferredWidth()  { return getPreferredSize (YD_HORIZ); } \
	virtual int  preferredHeight() { return getPreferredSize (YD_VERT); }  \
	virtual void setSize (int width, int height) { doSetSize (width, height); }

#define YGWIDGET_IMPL_USE_BOLD(ParentClass)                     \
    virtual void setUseBoldFont (bool useBold) {                \
    	ParentClass::setUseBoldFont (useBold);                  \
    	doSetUseBoldFont (useBold);                             \
    }

// for containers
#define YGWIDGET_IMPL_CHILD_ADDED(ParentClass, container)       \
	virtual void addChild (YWidget *ychild) {                   \
		ParentClass::addChild (ychild);                         \
		doAddChild (ychild, container);                         \
	}
#define YGWIDGET_IMPL_CHILD_REMOVED(ParentClass, container)     \
	virtual void removeChild (YWidget *ychild) {                \
		ParentClass::removeChild (ychild);                      \
		doRemoveChild (ychild, container);                      \
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

		virtual GtkWidget* getWidget() { return m_field; }

		void setLabelVisible (bool show);
		void setBuddy (GtkWidget *widget);
		virtual void doSetLabel (const std::string &label);

		YUIDimension orientation() { return m_orientation; }
		GtkWidget *getLabelWidget() { return m_label; }

	protected:
		GtkWidget *m_label, *m_field;
		YUIDimension m_orientation;
};

#define YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN(ParentClass)   \
	virtual void setLabel (const std::string &label) {     \
		IMPL                                               \
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

		virtual GtkWidget *getWidget() { return m_widget; }

		// you should use this method, not gtk_scrolled_window_set...
		void setPolicy (GtkPolicyType hpolicy, GtkPolicyType vpolicy);

	protected:
		void construct(GType type, const char *property_name, va_list args);
		GtkWidget *m_widget;
};

#endif /*YGWIDGET_H*/

