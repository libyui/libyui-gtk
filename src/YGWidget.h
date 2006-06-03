#ifndef YGWIDGET_H
#define YGWIDGET_H

#include <gtk/gtk.h>
#include <stdarg.h>
#include <YGUI.h>

class YGWidget
{
public:
	YGWidget(YWidget *y_container, YGWidget *parent,
			 const char *property_name = NULL, ...);
	YGWidget(YWidget *y_widget, YGWidget *parent,
			 bool show, GType type,
			 const char *property_name, ...);

	GtkWidget *getWidget() { return m_widget; }
	const char *getWidgetName() { return m_y_widget->widgetClass(); }
	GtkFixed *getFixed();

	long getNiceSize (YUIDimension dim);
	void show ();
	void doSetSize (long newWidth, long newHeight);
	void doMoveChild (YWidget *child, long newx, long newy);
	int xthickness();
	int ythickness();
	int thickness (YUIDimension dim) { return dim == YD_HORIZ ? xthickness() : ythickness(); }

	/* Get the YGWidget associated with this YWidget */
	static YGWidget *get (YWidget *y_widget);
	static char *mapKBAccel(const char *src);

protected:
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
			{ LOC; return getNiceSize (dim); }
#define YGWIDGET_IMPL_SET_ENABLING \
		virtual void setEnabling (bool enabled) \
			{ LOC; gtk_widget_set_sensitive (getWidget(), enabled); }
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
		virtual void moveChild (YWidget *child, long newx, long newy) { \
			fprintf (stderr, "%s:%s -G%p-Y%p- %ld, %ld\n", G_STRLOC, G_STRFUNC, \
			         child ? get(child)->getWidget():NULL, child, newx, newy); \
			doMoveChild (child, newx, newy); \
		}
#define YGWIDGET_IMPL_KEYBOARD_FOCUS \
		virtual bool setKeyboardFocus() { \
			gtk_widget_grab_focus (GTK_WIDGET(getWidget())); \
			return gtk_widget_is_focus (GTK_WIDGET(getWidget())); \
			}

#endif // YGWIDGET_H
