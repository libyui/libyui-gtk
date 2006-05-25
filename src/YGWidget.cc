#include <stdarg.h>
#include <YGWidget.h>

// Switch for new layout code
#undef YGWIDGET_NEW_LAYOUT

const GType ContainerType = GTK_TYPE_FIXED;

static GQuark allow_size_allocate = 0;

static void
clobbered_size_allocate (GtkWidget     *widget,
						 GtkAllocation *allocation)
{
	GType parent;
	GtkWidgetClass *parent_klass;

	parent = g_type_parent (G_OBJECT_TYPE (widget));
	parent_klass = GTK_WIDGET_CLASS (g_type_class_peek (parent));

	if (g_object_get_qdata (G_OBJECT (widget), allow_size_allocate))
		parent_klass->size_allocate (widget, allocation);
	else
		g_message ("clobbered_size_allocate");
}

/*
 * This generates a gtk-sub-class if necessary with the
 * size allocation logic eviscerated.
 */
static GtkType
dynamic_subtype (GtkType master_type)
{
	GtkType newtype;

	g_return_val_if_fail (g_type_is_a (master_type, GTK_TYPE_WIDGET), master_type);

	if (!allow_size_allocate)
		allow_size_allocate = g_quark_from_static_string ("allow-size-allocate");

	gchar *subtype = g_strconcat (g_type_name (master_type),
								  "ClobberedSize", NULL);
	if ((newtype = g_type_from_name (subtype)) == G_TYPE_INVALID)
	{
		GTypeQuery query;
		g_type_query (master_type, &query);
        GTypeInfo new_info = { 0, };
		new_info.class_size = query.class_size;
		new_info.instance_size = query.instance_size;
        newtype = g_type_register_static(master_type, subtype,
										 &new_info, (GTypeFlags)0);
		GtkWidgetClass *klass = GTK_WIDGET_CLASS (g_type_class_ref (newtype));
		klass->size_allocate = clobbered_size_allocate;
	}
	return newtype;
}

void
YGWidget::construct (YWidget *y_widget, YGWidget *parent,
					 bool show, GType type,
					 const char *property_name, va_list args)
{
	m_alloc.x = m_alloc.y = m_alloc.width = m_alloc.height = 0;

#ifdef YGWIDGET_NEW_LAYOUT
	fprintf (stderr, "dynamic sub-type!\n");
	if (!show)
			g_assert (g_type_is_a (type, GTK_TYPE_WINDOW));
	else
			type = dynamic_subtype (type);
#endif

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
do_size_allocate (GtkWidget *widget, GtkAllocation *alloc)
{
	GtkAllocation alloc_copy = *alloc;
	g_object_set_qdata (G_OBJECT (widget), allow_size_allocate,
						GUINT_TO_POINTER (1));
	fprintf (stderr, "do size allocate (%p) %d,%d %d, %d\n",
			 widget, alloc_copy.x, alloc_copy.y, alloc_copy.width, alloc_copy.height);

	if (widget->parent)
	{ /* no window widgets */
		alloc_copy.x += widget->parent->allocation.x;
		alloc_copy.y += widget->parent->allocation.y;
	}
	gtk_widget_size_allocate (widget, &alloc_copy);
	g_object_set_qdata (G_OBJECT (widget), allow_size_allocate, NULL);
}

void
YGWidget::doSetSize (long newWidth, long newHeight)
{
#ifdef YGWIDGET_NEW_LAYOUT
	m_alloc.width = newWidth;
	m_alloc.height = newHeight;
	do_size_allocate (m_widget, &m_alloc);
#else
	gtk_widget_set_size_request (getWidget(), newWidth, newHeight );
#endif
}

void
YGWidget::moveWidget (long newx, long newy)
{
	m_alloc.x = newx;
	m_alloc.y = newy;
	do_size_allocate (m_widget, &m_alloc);
}

void
YGWidget::doMoveChild( YWidget *child, long newx, long newy )
{
#ifdef YGWIDGET_NEW_LAYOUT
	YGWidget::get (child)->moveWidget (newx, newy);
#else
	gtk_fixed_move (getFixed(), YGWidget::get (child)->getWidget(), newx, newy);
#endif
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
