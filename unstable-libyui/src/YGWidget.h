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
	YGWidget(YWidget *ywidget, YWidget *yparent, bool show,
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
	virtual void doSetEnabling (bool enabled);

	// Event handling
	void emitEvent(YEvent::EventReason reason, bool if_notify = true,
	               bool if_not_pending = false, bool immediate = true);

	// Aesthetics
	void setBorder (unsigned int border);  // in pixels
	void setMinSize (unsigned int min_width, unsigned int min_height);
	void setMinSizeInChars (unsigned int min_width, unsigned int min_height);

	// whenever the stretchable property may change (eg. when adding a child
	// for a container), call this function to make sure it is honored.
	virtual void sync_stretchable (YWidget *child = 0);

protected:
	void show();

	GtkWidget *m_widget;  // associated GtkWidget -- use getWidget()
	GtkWidget *m_adj_size;  // installed on m_widget, allows better size constrains
	YWidget *m_ywidget;  // associated YWidget

	void construct (YWidget *ywidget, YWidget *yparent,
	                bool show, GType type,
	                const char *property_name, va_list args);
};

/*
 * Macros to help implement proxies between common YWidget virtual
 * methods and the (multiply inherited) YGWidget base implementation
 * for GTK+.
 */
#define YGWIDGET_IMPL_COMMON                                    \
	virtual bool setKeyboardFocus()                             \
	    { return doSetKeyboardFocus(); }                        \
	virtual void setEnabling (bool enabled)                     \
	    { doSetEnabling (enabled); }                            \
	virtual int  preferredWidth()  { return 0; }                \
	virtual int  preferredHeight() { return 0; }                \
	virtual void moveChild (YWidget *child, int x, int y) {}    \
	virtual void setSize (int width, int height) {}

#define YGWIDGET_IMPL_USE_BOLD(ParentClass)                     \
    virtual void setUseBoldFont (bool useBold) {                \
    	PangoWeight weight = useBold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL; \
		YGUtils::setWidgetFont (getWidget(), weight, PANGO_SCALE_MEDIUM); \
    	ParentClass::setUseBoldFont (useBold);                  \
    }

// for containers
#define YGWIDGET_IMPL_CHILD_ADDED(container)                    \
	virtual void addChild (YWidget *ychild) {                   \
		YWidget::addChild (ychild);                             \
		GtkWidget *child = YGWidget::get (ychild)->getLayout(); \
		gtk_container_add (GTK_CONTAINER (container), child);   \
		sync_stretchable();                                     \
	}
#define YGWIDGET_IMPL_CHILD_REMOVED(container)                   \
	virtual void removeChild (YWidget *ychild) {                 \
		YWidget::removeChild (ychild);                           \
		GtkWidget *child = YGWidget::get (ychild)->getLayout();  \
		gtk_container_remove (GTK_CONTAINER (container), child); \
	}

/* This is a convenience class that allows for a label next to the
   intended widget. It should be used, in case you have the need for
   such, as it gives an uniform API. */
class YGLabeledWidget : public YGWidget
{
	public:
		YGLabeledWidget(YWidget *ywidget, YWidget *yparent,
		                const std::string &label_text, YUIDimension label_ori,
		                bool show, GType type, const char *property_name, ...);
		virtual ~YGLabeledWidget () {}

		virtual GtkWidget* getWidget() { return m_field; }

		void setLabelVisible(bool show);
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
		doSetLabel (label);                                \
		ParentClass::setLabel (label);                     \
	}

/* This is a convenience class for widgets that need scrollbars. */
class YGScrolledWidget : public YGLabeledWidget
{
	public:
		YGScrolledWidget(YWidget *ywidget, YWidget *yparent,
		                 bool show, GType type, const char *property_name, ...);
		// if you want a label, use:
		YGScrolledWidget(YWidget *ywidget, YWidget *yparent,
		                 const std::string &label_text, YUIDimension label_ori,
		                 bool show, GType type, const char *property_name, ...);
		virtual ~YGScrolledWidget () {}

		virtual GtkWidget *getWidget() { return m_widget; }

		// you should use this method, not gtk_scrolled_window_set...
		void setPolicy (GtkPolicyType hpolicy, GtkPolicyType vpolicy);

	protected:
		void construct(GType type, const char *property_name, va_list args);
		GtkWidget *m_widget;
};

#endif /*YGWIDGET_H*/

