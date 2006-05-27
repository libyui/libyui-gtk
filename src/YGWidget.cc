#include <stdarg.h>
#include <YGWidget.h>

void
YGWidget::construct (YWidget *y_widget, YGWidget *parent,
		     bool show, GType type,
		     const char *property_name, va_list args)
{
	m_alloc.x = m_alloc.y = m_alloc.width = m_alloc.height = 0;

	m_widget = GTK_WIDGET (g_object_new_valist (type, property_name, args));

	y_widget->setWidgetRep ((void *)this);
	fprintf (stderr, "Set YWidget %p rep to %p\n", y_widget, this);
	
	GtkFixed *fixed;
	if (!parent || !(fixed = parent->getFixed()))
		g_warning ("No parent for new widget");
	else
		gtk_fixed_put (fixed, m_widget, 0, 0);
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
	GtkWidget *widget = m_widget;
	GtkWidget *child;

	// Some widgets are special cases where the true parent
	// is in fact a child of a compound widget
	if (GTK_IS_FRAME (widget) &&
		(child = gtk_bin_get_child (GTK_BIN (widget))))
		return GTK_FIXED (child);

	while (widget && !GTK_IS_FIXED (widget))
		widget = widget->parent;

	if (widget)
		return GTK_FIXED (widget);

	// Desparate last measures FIXME: necessary ?
	YGWidget *dialog;
	if (!(dialog = get (m_y_widget->yDialog())))
		return NULL;
	return GTK_FIXED (gtk_bin_get_child (GTK_BIN (dialog->getWidget())));
}

YGWidget *
YGWidget::get (YWidget *y_widget)
{
	YGWidget *ptr;
	if (!y_widget || !y_widget->widgetRep()) {
		fprintf (stderr, "Y_Widget %p : rep %p\n",
			 y_widget, y_widget ? y_widget->widgetRep() : NULL);
		return NULL;
	}
	ptr = static_cast<YGWidget *>(y_widget->widgetRep());
	g_assert (GTK_IS_WIDGET (ptr->m_widget));
	return ptr;
}

void
YGWidget::doSetSize (long newWidth, long newHeight)
{
 	gtk_widget_set_size_request (getWidget(), newWidth, newHeight );
}

void
YGWidget::doMoveChild( YWidget *child, long newx, long newy )
{
	gtk_fixed_move (getFixed(), YGWidget::get (child)->getWidget(), newx, newy);
}

long
YGWidget::getNiceSize (YUIDimension dim)
{
	long ret;
	GtkWidget *widget;
	GtkRequisition req;

	widget = getWidget();
#define SIZING_DEBUG
#ifdef SIZING_DEBUG
	fprintf (stderr, "Get nice size request for:\n");
	dumpWidgetTree (widget);
#endif

	gtk_widget_ensure_style (widget);      
	g_signal_emit_by_name (widget, "size_request", &req);

	// Since there are no tweaks for widget separation etc.
	// we get to do that ourselves
	if (dim == YD_HORIZ)
		ret = req.width;
	else 
		ret = req.height;

	fprintf (stderr, "NiceSize for '%s' %s %ld\n",
			 getWidgetName(), dim == YD_HORIZ ? "width" : "height",
			 ret);

	return ret;
}

char *YGWidget::mapKBAccel(const char *src)
{
    char *result = g_strdup (src);
	for (char *p = result; *p; p++) {
	   // FIXME '&' escaping ?
	    if (*p == '&') 
		    *p = '_';
	}
	return result;
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
