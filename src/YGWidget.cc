//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <stdarg.h>
#include "YGWidget.h"
#include "YGUtils.h"

/* YGWidget follows */

void
YGWidget::construct (YWidget *y_widget, YGWidget *parent,
		     bool show, GType type,
		     const char *property_name, va_list args)
{
	m_alloc.x = m_alloc.y = m_alloc.width = m_alloc.height = 0;
	m_min_xsize = m_min_ysize = 0;

	m_widget = GTK_WIDGET (g_object_new_valist (type, property_name, args));
	g_object_ref (G_OBJECT (m_widget));
	gtk_object_sink (GTK_OBJECT (m_widget));

	y_widget->setWidgetRep ((void *) this);
#ifdef IMPL_DEBUG
	fprintf (stderr, "Set YWidget %p rep to %p\n", y_widget, this);
	fprintf (stderr, "border for %s: %d\n", y_widget->widgetClass(), m_border);
#endif

	if (parent) {
		y_widget->setParent (parent->m_y_widget);
		GtkFixed *fixed;

		if (!(fixed = parent->getFixed()))
			g_error ("YGWidget: widget %s doesn't have where to seat",
			         y_widget->widgetClass());
		else
			gtk_fixed_put (fixed, m_widget, m_border, m_border);
	}
	else
		m_border = 0;

	if (show)
		gtk_widget_show (m_widget);
}

// Widget constructor
YGWidget::YGWidget(YWidget *y_widget, YGWidget *parent,
		   bool show, GtkType type,
		   const char *property_name, ...) :
	m_y_widget (y_widget)
{
	m_border = 6;

	va_list args;
	va_start (args, property_name);

	construct (y_widget, parent, show, type,
			   property_name, args);

	va_end (args);
}

// Container constructor
YGWidget::YGWidget(YWidget *y_container, YGWidget *parent,
		   const char *property_name, ...) :
	m_y_widget (y_container)
{
	m_border = 0;

	va_list args;
	va_start (args, property_name);

	construct (y_container, parent, true, GTK_TYPE_FIXED,
		   property_name, args);

	va_end (args);
}

YGWidget::~YGWidget()
{
	IMPL
	gtk_widget_destroy (m_widget);
	g_object_unref (G_OBJECT (m_widget));
}

void YGWidget::setBorder (unsigned int border)
{
	// We need to move child to fit new border.
	// NOTE: setBorder() is meant to be called from the constructor
	// only, otherwise we would need to recalculate the all layout.
	m_border = border;
#ifdef IMPL_DEBUG
fprintf (stderr, "border for %s: %d (posteriori)\n", m_y_widget->widgetClass(), m_border);
#endif
	gtk_fixed_move (YGWidget::getFixed(), m_widget, m_border, m_border);
}

void YGWidget::setMinSize (unsigned int xsize, unsigned int ysize)
{
	if (xsize)
		m_min_xsize = YGUtils::getCharsWidth (getWidget(), xsize);
	if (ysize)
		m_min_ysize = YGUtils::getCharsHeight (getWidget(), ysize);
}

GtkFixed *
YGWidget::getFixed()
{
	// This is just a shortcut so that simple containers don't have to
	// override getFixed()
	if (GTK_IS_FIXED (m_widget))
		return GTK_FIXED (m_widget);

	YWidget *y_parent;
	YGWidget *parent;

	if (!(y_parent = m_y_widget->yParent()) ||
	    !(parent = get (y_parent))) {
		g_error ("getFixed() failed -- widget %s is orphan", m_y_widget->widgetClass());
		return NULL;
	}

	return parent->getFixed();
}

YGWidget *
YGWidget::get (YWidget *y_widget)
{
	if (!y_widget || !y_widget->widgetRep()) {
#ifdef IMPL_DEBUG
		fprintf (stderr, "Y_Widget %p : rep %p\n",
			 y_widget, y_widget ? y_widget->widgetRep() : NULL);
#endif
		return NULL;
	}
	return (YGWidget *) (y_widget->widgetRep());
}

void
YGWidget::doSetSize (long width, long height)
{
	// leave some blank space around...
	width  -= m_border * 2;
	height -= m_border * 2;
	gtk_widget_set_size_request (getWidget(), width, height);
}

void
YGWidget::doMoveChild (YWidget *child, long x, long y)
{
	GtkFixed *fixed = getFixed();
	YGWidget *yg_widget = YGWidget::get (child);
	GtkWidget *widget   = yg_widget->getWidget();
	int border = yg_widget->m_border;

	if (!GTK_IS_WIDGET (widget))
		g_error ("doMoveChild() failed -- widget %s isn't associated to a GtkWidget",
		         m_y_widget->widgetClass());
	else if (!GTK_IS_FIXED (fixed))
		g_error ("doMoveChild() failed -- no associated GtkFixed to widget %s",
		         m_y_widget->widgetClass());
	else
		gtk_fixed_move (fixed, widget, x + border, y + border);
}

// The slightly non-obvious thing here, is that by emitting the
// size_request signal ourselves, we can force the widget to re-
// calculate it's preferred size - without getting clobbered by
// the (now incorrect) 'gtk_widget_set_size_request' data on the
// widget.

long
YGWidget::getNiceSize (YUIDimension dim)
{
	long ret;
	GtkRequisition req;

#ifdef IMPL_DEBUG
	fprintf (stderr, "Get nice size request for:\n");
	dumpWidgetTree (m_widget);
#endif

	gtk_widget_ensure_style (m_widget);
	g_signal_emit_by_name (m_widget, "size_request", &req);

	// Since there are no tweaks for widget separation etc.
	// we get to do that ourselves
	if (dim == YD_HORIZ)
		ret = MAX (req.width, (signed) m_min_xsize);
	else
		ret = MAX (req.height, (signed) m_min_ysize);
	ret += m_border * 2;

#ifdef IMPL_DEBUG
	fprintf (stderr, "NiceSize for '%s' %s %ld\n",
	         getWidgetName(), dim == YD_HORIZ ? "width" : "height",
	         ret);
#endif

	return ret;
}

void YGWidget::show()
{
	gtk_widget_show (m_widget);
}

int YGWidget::xthickness()
{
	return getWidget()->style->xthickness + m_border;
}

int YGWidget::ythickness()
{
	return getWidget()->style->ythickness + m_border;
}

void YGWidget::emitEvent (YEvent::EventReason reason, bool if_notify, bool if_not_pending)
{
	if ((!if_notify      || m_y_widget->getNotify()) &&
	    (!if_not_pending || !YGUI::ui()->eventPendingFor(m_y_widget)))
		YGUI::ui()->sendEvent (new YWidgetEvent (m_y_widget, reason));
}

/* YGLabeledWidget follows */

YGLabeledWidget::YGLabeledWidget(
	     YWidget *y_widget,
	     YGWidget *parent,
	     YCPString label_text, YUIDimension label_ori,
	     bool show, GType type,
	     const char *property_name, ...)
	: YGWidget (y_widget, parent, show,
	    label_ori == YD_VERT ? GTK_TYPE_VBOX : GTK_TYPE_HBOX,
	    "spacing", 4, NULL)
{
	// Create the field widget
	va_list args;
	va_start (args, property_name);
	m_field = GTK_WIDGET (g_object_new_valist (type, property_name, args));
	va_end (args);

	// Create the label
	m_label = gtk_label_new ("");
	if(show) {
		gtk_widget_show (m_label);
		gtk_widget_show (m_field);
	}
	setBuddy (m_field);
	doSetLabel (label_text);

	// Set the container and show widgets
	gtk_container_add (GTK_CONTAINER (m_widget), m_label);
	gtk_container_add (GTK_CONTAINER (m_widget), m_field);
	gtk_box_set_child_packing (GTK_BOX (m_widget), m_label,
	                      FALSE, FALSE, 4, GTK_PACK_START);
}

void
YGLabeledWidget::setLabelVisible (bool show)
{
	if (show)
		gtk_widget_show (m_label);
	else
		gtk_widget_hide (m_label);
}

void
YGLabeledWidget::setBuddy (GtkWidget *widget)
{
	gtk_label_set_mnemonic_widget (GTK_LABEL (m_label), widget);
}

void
YGLabeledWidget::doSetLabel (const YCPString &label)
{
	string str = YGUtils::mapKBAccel (label->value_cstr());
	if (str.empty())
		gtk_widget_hide (m_label);
	else {
		gtk_label_set_text (GTK_LABEL (m_label), str.c_str());
		gtk_label_set_use_underline (GTK_LABEL (m_label), TRUE);
	}
}

/* YGScrolledWidget follows */

YGScrolledWidget::YGScrolledWidget (
		YWidget *y_widget, YGWidget *parent,
		bool show, GType type, const char *property_name, ...)
: YGLabeledWidget (y_widget, parent,
                   YCPString ("<no label>"), YD_VERT, show,
                   GTK_TYPE_SCROLLED_WINDOW, "shadow-type", GTK_SHADOW_IN, NULL)
{
	va_list args;
	va_start (args, property_name);
	construct(type, property_name, args);
	va_end (args);

	setLabelVisible (false);
}

YGScrolledWidget::YGScrolledWidget (
		YWidget *y_widget, YGWidget *parent,
		YCPString label_text, YUIDimension label_ori,
		bool show, GType type, const char *property_name, ...)
: YGLabeledWidget (y_widget, parent, label_text, label_ori, show,
                   GTK_TYPE_SCROLLED_WINDOW, "shadow-type", GTK_SHADOW_IN, NULL)
{
	va_list args;
	va_start (args, property_name);
	construct(type, property_name, args);
	va_end (args);
}

void YGScrolledWidget::construct
		(GType type, const char *property_name, va_list args)
{
	m_widget = GTK_WIDGET (g_object_new_valist (type, property_name, args));
	setBuddy (m_widget);

	gtk_scrolled_window_set_policy (
		GTK_SCROLLED_WINDOW (YGLabeledWidget::getWidget()),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (YGLabeledWidget::getWidget()), m_widget);
	gtk_widget_show (m_widget);
}
