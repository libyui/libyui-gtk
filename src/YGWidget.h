#ifndef YGWIDGET_H
#define YGWIDGET_H

#include <gtk/gtk.h>
#include <stdarg.h>
#include "YGUI.h"
#include "YEvent.h"

class YGWidget
{
public:
	YGWidget(YWidget *y_container, YGWidget *parent,
			 const char *property_name = NULL, ...);
	YGWidget(YWidget *y_widget, YGWidget *parent,
			 bool show, GType type,
			 const char *property_name, ...);
	virtual ~YGWidget();

	GtkWidget *getWidget() { return m_widget; }
	const char *getWidgetName() { return m_y_widget->widgetClass(); }
	virtual GtkFixed *getFixed();

	long getNiceSize (YUIDimension dim);
	void show ();
	void doSetSize (long newWidth, long newHeight);
	void doMoveChild (YWidget *child, long newx, long newy);
	int xthickness();
	int ythickness();
	int thickness (YUIDimension dim) { return dim == YD_HORIZ ? xthickness() : ythickness(); }

	/* Get the YGWidget associated with this YWidget */
	static YGWidget *get (YWidget *y_widget);

	// Event handling
	void emitEvent(YEvent::EventReason reason,
	               bool if_notify = true, bool if_not_pending = false);

protected:
	/* The associated GTK+ widget. */
	GtkWidget *m_widget;

	// no RTTI for dynamic_cast ?
	YWidget   *m_y_widget;
	GtkAllocation m_alloc;
	void construct (YWidget *y_widget, YGWidget *parent,
	                bool show, GType type,
	                const char *property_name, va_list args);
};

/*
 * Macros to help implement proxies between common YWidget virtual
 * methods and the (multiply inherited) YGWidget base implementation
 * for GTK+.
 */

#define YGWIDGET_IMPL_NICESIZE \
		virtual long nicesize (YUIDimension dim) \
			{ IMPL; return getNiceSize (dim); }
#define YGWIDGET_IMPL_SET_ENABLING \
		virtual void setEnabling (bool enabled) \
			{ IMPL; gtk_widget_set_sensitive (getWidget(), enabled); }
#define YGWIDGET_IMPL_SET_SIZE \
		virtual void setSize (long newWidth, long newHeight) { \
			fprintf (stderr, "%s:%s -G%p-Y%p- %ld, %ld\n", G_STRLOC, G_STRFUNC, \
			         m_widget, m_y_widget, newWidth, newHeight); \
			doSetSize (newWidth, newHeight); \
		}
#define YGWIDGET_IMPL_SET_SIZE_CHAIN(ParentClass) \
		virtual void setSize (long newWidth, long newHeight) { \
			fprintf (stderr, "%s:%s -G%p-Y%p- %ld, %ld + chain\n", G_STRLOC, G_STRFUNC, \
			         m_widget, m_y_widget, newWidth, newHeight); \
			doSetSize (newWidth, newHeight); \
			ParentClass::setSize (newWidth, newHeight); \
		}
#define YGWIDGET_IMPL_MOVE_CHILD \
		virtual void moveChild (YWidget *child, long newx, long newy) {       \
			fprintf (stderr, "%s:%s -G%p-Y%p- %ld, %ld\n", G_STRLOC, G_STRFUNC, \
			         child ? get(child)->getWidget():NULL, child, newx, newy);  \
			doMoveChild (child, newx, newy); \
		}
#define YGWIDGET_IMPL_KEYBOARD_FOCUS \
		virtual bool setKeyboardFocus() { \
			gtk_widget_grab_focus (GTK_WIDGET(getWidget())); \
			return gtk_widget_is_focus (GTK_WIDGET(getWidget())); \
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
		fprintf (stderr, "%s:%s -G%p-Y%p- '%s' + chain\n", G_STRLOC, G_STRFUNC, \
		         m_widget, m_y_widget, label->value_cstr());                    \
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
