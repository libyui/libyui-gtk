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

	m_widget = GTK_WIDGET (g_object_new_valist (type, property_name, args));
	g_object_ref (G_OBJECT (m_widget));
	gtk_object_sink (GTK_OBJECT (m_widget));

fprintf (stderr, "setWidgetRep\n");
	y_widget->setWidgetRep ((void *)this);
#ifdef IMPL_DEBUG
	fprintf (stderr, "Set YWidget %p rep to %p\n", y_widget, this);
#endif

	GtkFixed *fixed;
fprintf (stderr, "asking for fixed\n");
	if (!parent || !(fixed = parent->getFixed()))
		g_warning ("No parent for new widget");
	else
		gtk_fixed_put (fixed, m_widget, 0, 0);
fprintf (stderr, "showing up\n");
	if (show)
		gtk_widget_show_all (m_widget);
}

// Widget constructor
YGWidget::YGWidget(YWidget *y_widget, YGWidget *parent,
		   bool show, GtkType type,
		   const char *property_name, ...) :
	m_y_widget (y_widget)
{
	va_list args;
	va_start (args, property_name);

	construct (y_widget, parent, show, type,
			   property_name, args);

	va_end (args);
}

YGWidget::~YGWidget()
{
	IMPL;
	gtk_widget_destroy (m_widget);
	g_object_unref (G_OBJECT (m_widget));
}

// Container constructor
YGWidget::YGWidget(YWidget *y_container, YGWidget *parent,
		   const char *property_name, ...) :
	m_y_widget (y_container)
{
	va_list args;
	va_start (args, property_name);

	construct (y_container, parent, true, GTK_TYPE_FIXED,
		   property_name, args);

	va_end (args);
}

GtkFixed *
YGWidget::getFixed()
{
	// In containers, the widget is the GtkFixed itself
	if (GTK_IS_FIXED (m_widget))
		return GTK_FIXED (m_widget);

	YWidget *y_parent;
	YGWidget *parent;

	if (!(y_parent = m_y_widget->yParent()) ||
	    !(parent = (YGWidget*) y_parent->widgetRep())) {
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
fprintf (stderr, "setting size %ld, %ld to %s\n", width, height, m_y_widget->widgetClass()); 
	gtk_widget_set_size_request (getWidget(), width, height);
fprintf (stderr, "done\n");
}

void
YGWidget::doMoveChild (YWidget *child, long x, long y)
{
	GtkFixed *fixed = YGWidget::getFixed();
	GtkWidget *widget = YGWidget::get (child)->getWidget();

	if (!GTK_IS_WIDGET (widget))
		g_error ("doMoveChild() failed -- widget %s isn't associated to a GtkWidget",
		         m_y_widget->widgetClass());
	else if (!GTK_IS_FIXED (fixed))
		g_error ("doMoveChild() failed -- no associated GtkFixed to widget %s",
		         m_y_widget->widgetClass());

	else
		gtk_fixed_move (YGWidget::getFixed(), widget, x, y);
}

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
		ret = req.width;
	else 
		ret = req.height;

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
	return getWidget()->style->xthickness;
}

int YGWidget::ythickness()
{
	return getWidget()->style->ythickness;
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
	setBuddy (m_field);
	doSetLabel (label_text);

	// Set the container and show widgets
	gtk_container_add (GTK_CONTAINER (m_widget), m_label);
	gtk_container_add (GTK_CONTAINER (m_widget), m_field);
	gtk_box_set_child_packing (GTK_BOX (m_widget), m_label,
	                      FALSE, FALSE, 4, GTK_PACK_START);
	if(show) {
		gtk_widget_show (m_label);
		gtk_widget_show (m_field);
	}
}

void
YGLabeledWidget::setLabelVisible(bool show)
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
YGLabeledWidget::doSetLabel (const YCPString & label)
{
	string str = YGUtils::mapKBAccel (label->value_cstr());
	gtk_label_set_text (GTK_LABEL (m_label), str.c_str());
	gtk_label_set_use_underline (GTK_LABEL (m_label), str.compare(label->value_cstr()));
}

/* YGScrolledWidget follows */

YGScrolledWidget::YGScrolledWidget (
		YWidget *y_widget, YGWidget *parent,
		bool show, GType type, const char *property_name, ...)
: YGLabeledWidget (y_widget, parent,
                   YCPString ("<no label>"), YD_VERT,
                   show, GTK_TYPE_SCROLLED_WINDOW, NULL)
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
                   GTK_TYPE_SCROLLED_WINDOW, NULL)
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

	gtk_scrolled_window_set_policy (
		GTK_SCROLLED_WINDOW (YGLabeledWidget::getWidget()),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (YGLabeledWidget::getWidget()), m_widget);
	gtk_widget_show (m_widget);
}
