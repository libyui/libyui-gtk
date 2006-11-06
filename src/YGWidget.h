#ifndef YGWIDGET_H
#define YGWIDGET_H

#include <gtk/gtk.h>
#include <stdarg.h>
#include "YGUI.h"
#include "YEvent.h"

class YGWidget
{
public:
	YGWidget(YWidget *y_widget, YGWidget *parent, bool show,
	         GType type, const char *property_name, ...);
	virtual ~YGWidget();

	virtual GtkWidget *getWidget() { return m_widget; }
	const char *getWidgetName() { return m_y_widget->widgetClass(); }
	void show();

	// containers should use this call rather than getWidget()
	GtkWidget *getLayout() { return m_alignment; }

	// Get the YGWidget associated with a YWidget
	static YGWidget *get (YWidget *y_widget);

	// for YWidget
	virtual bool doSetKeyboardFocus();
	virtual void doSetEnabling (bool enabled);

	// Event handling
	void emitEvent(YEvent::EventReason reason, bool if_notify = true,
	               bool if_not_pending = false);

	// Layout
	void setBorder (unsigned int border);  // in pixels
	unsigned int getBorder();

	void setMinSize (int min_width, int min_height);  // in characters
	void setStretchable (YUIDimension dim, bool stretch);
	void setAlignment (YAlignmentType halign, YAlignmentType valign);
	void setPadding (int top, int bottom, int left, int right);

protected:
	GtkWidget *m_widget;  // associated GtkWidget -- use getWidget()
	YWidget *m_y_widget;  // associated YWidget
	GtkWidget *m_alignment;  // associated GtkWidget where this widget is installed
	                         // so we may set border and other things
	GtkWidget *m_min_size;  // installed on m_widget, allows setting a min size

	void construct (YWidget *y_widget, YGWidget *parent,
	                bool show, GType type,
	                const char *property_name, va_list args);

	// callbacks
	GtkAllocation m_alloc;
	static void realize_cb (GtkWidget *widget, YGWidget *pThis);
	static void size_allocate_cb (GtkWidget *widget, GtkAllocation *allocation,
	                              YGWidget *pThis);

	// parameters to set the layout prettier
	int m_min_width, m_min_height;
};

/*
 * Macros to help implement proxies between common YWidget virtual
 * methods and the (multiply inherited) YGWidget base implementation
 * for GTK+.
 */
// FIXME: we should set parent to setstretchable
#define YGWIDGET_IMPL_COMMON                                   \
	virtual bool setKeyboardFocus()                              \
	    { return doSetKeyboardFocus(); }                         \
	virtual void setEnabling (bool enabled)                      \
	    { doSetEnabling (enabled); }                             \
	virtual long nicesize (YUIDimension dim) { return 0; }       \
	virtual void moveChild (YWidget *child, long x, long y) {}   \
	virtual void setSize (long width, long height) {}

// for containers
#define YGWIDGET_IMPL_CHILD_ADDED(container)                  \
	virtual void childAdded (YWidget *ychild) {                 \
	    GtkWidget *child = YGWidget::get (ychild)->getLayout(); \
	    gtk_container_add (GTK_CONTAINER (container), child);   \
	}
#define YGWIDGET_IMPL_CHILD_REMOVED(container)                 \
	virtual void childRemoved (YWidget *ychild) {                \
	    GtkWidget *child = YGWidget::get (ychild)->getLayout();  \
	    gtk_container_remove (GTK_CONTAINER (container), child); \
	}

/* This is a convenience class that allows for a label next to the
   intended widget. It should be used, in case you have the need for
   such, as it gives an uniform API.                               */

class YGLabeledWidget : public YGWidget
{
	public:
		YGLabeledWidget(YWidget *y_widget, YGWidget *parent,
		                YCPString label_text, YUIDimension label_ori,
		                bool show, GType type, const char *property_name, ...);
		virtual ~YGLabeledWidget () {}

		virtual GtkWidget* getWidget() { return m_field; }

		void setLabelVisible(bool show);
		void setBuddy (GtkWidget *widget);
		virtual void doSetLabel (const YCPString &label);
	protected:
		GtkWidget *m_label, *m_field;
};

#define YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN(ParentClass)                    \
	virtual void setLabel (const YCPString &label) {                          \
		IMPL                                                                    \
		doSetLabel (label);                                                     \
		ParentClass::setLabel (label);                                          \
	}

/* This is a convenience class for widgets that need scrollbars. */

class YGScrolledWidget : public YGLabeledWidget
{
	public:
		YGScrolledWidget(YWidget *y_widget, YGWidget *parent,
		                 bool show, GType type, const char *property_name, ...);
		// if you want a label, use:
		YGScrolledWidget(YWidget *y_widget, YGWidget *parent,
		                 YCPString label_text, YUIDimension label_ori,
		                 bool show, GType type, const char *property_name, ...);
		virtual ~YGScrolledWidget () {}

		virtual GtkWidget *getWidget() { return m_widget; }

	protected:
		void construct(GType type, const char *property_name, va_list args);
		GtkWidget *m_widget;
};

#endif // YGWIDGET_H
