/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <stdarg.h>
#include "YGWidget.h"
#include "YGUtils.h"
#include "ygtkratiobox.h"

// default widgets border -- may be overlapped with a setBorder(..)
#define DEFAULT_BORDER       6
#define LABEL_WIDGET_SPACING 4

/* YGWidget follows */

YGWidget::YGWidget(YWidget *y_widget, YGWidget *parent, bool show,
                   GtkType type, const char *property_name, ...)
	: m_y_widget (y_widget)
{
	va_list args;
	va_start (args, property_name);
	construct (y_widget, parent, show, type, property_name, args);
	va_end (args);
}

void YGWidget::construct (YWidget *y_widget, YGWidget *parent, bool _show,
                          GType type, const char *property_name, va_list args)
{
	m_widget = GTK_WIDGET (g_object_new_valist (type, property_name, args));

	if (type == GTK_TYPE_WINDOW)
		m_min_size = m_widget;
	else {
		m_min_size = ygtk_min_size_new (0, 0);
		g_object_ref (G_OBJECT (m_min_size));
		gtk_object_sink (GTK_OBJECT (m_min_size));
		gtk_widget_show (m_min_size);
		gtk_container_add (GTK_CONTAINER (m_min_size), m_widget);
	}

	y_widget->setWidgetRep ((void *) this);
	if (parent)
		y_widget->setParent (parent->m_y_widget);

#ifdef IMPL_DEBUG
	fprintf (stderr, "Set YWidget %p rep to %p\n", y_widget, this);
#endif

	// Split by two so that with another widget it will have full border...
	setBorder (DEFAULT_BORDER / 2);

	if (_show)
		show();
}

YGWidget::~YGWidget()
{
	IMPL
	gtk_widget_destroy (m_min_size);
	g_object_unref (G_OBJECT (m_min_size));
}

void YGWidget::show()
{
	gtk_widget_show (m_widget);
}

YGWidget *YGWidget::get (YWidget *y_widget)
{
	if (!y_widget || !y_widget->widgetRep()) {
#ifdef IMPL_DEBUG
		if (y_widget)
			fprintf (stderr, "Widget '%s' (label: '%s') not supported\n",
			         y_widget->widgetClass(), y_widget->debugLabel().c_str());
		else
			fprintf (stderr, "YGWidget::get() on null\n");
#endif
		return NULL;
	}
	return (YGWidget *) (y_widget->widgetRep());
}

bool YGWidget::doSetKeyboardFocus()
{
	IMPL
	gtk_widget_grab_focus (GTK_WIDGET (getWidget()));
	return gtk_widget_is_focus (GTK_WIDGET (getWidget()));
}

void YGWidget::doSetEnabling (bool enabled)
{
	gtk_widget_set_sensitive (getWidget(), enabled);
}

void YGWidget::emitEvent (YEvent::EventReason reason, bool if_notify,
                          bool if_not_pending)
{
	if ((!if_notify      || m_y_widget->getNotify()) &&
	    (!if_not_pending || !YGUI::ui()->eventPendingFor(m_y_widget)))
		YGUI::ui()->sendEvent (new YWidgetEvent (m_y_widget, reason));
}

void YGWidget::setBorder (unsigned int border)
{
	IMPL
	gtk_container_set_border_width (GTK_CONTAINER (m_min_size), border);
}

void YGWidget::setMinSize (unsigned int width, unsigned int height)
{
	IMPL
	if (width)
		ygtk_min_size_set_width (YGTK_MIN_SIZE (m_min_size), width);
	if (height)
		ygtk_min_size_set_height (YGTK_MIN_SIZE (m_min_size), height);
}

void YGWidget::setMinSizeInChars (unsigned int width, unsigned int height)
{
	IMPL
	if (width)
		width = YGUtils::getCharsWidth (getWidget(), width);
	if (height)
		height = YGUtils::getCharsHeight (getWidget(), height);
	setMinSize (width, height);
}

void YGWidget::sync_stretchable (YWidget *child)
{
	IMPL
	YWidget *parent = m_y_widget->yParent();
	if (parent)
		// tell parent to sync too!
		YGWidget::get (parent)->sync_stretchable (m_y_widget);
}

/* Checks everywhere in a container to see if there are children (so
   he is completely initialized) so that we may ask him for stretchable()
   because some YContainerWidgets crash when they don't have children. */
#include "YSplit.h"
static bool safe_stretchable (YWidget *widget)
{
	YContainerWidget *container = dynamic_cast <YContainerWidget *> (widget);
	if (container) {
		YSplit *split = dynamic_cast <YSplit *> (widget);
		// in the case of YSplit its safe to ask for stretchability with no kids
		if (split) {
			if (!split->hasChildren())
				return true;
		}
		else
			if (!container->hasChildren())
				return false;

		for (int i = 0; i < container->numChildren(); i++)
			if (!safe_stretchable (container->child (i)))
				return false;
	}
	return true;
}

bool YGWidget::isStretchable (YUIDimension dim)
{
	if (safe_stretchable (m_y_widget))
		return m_y_widget->stretchable (dim);
	return false;
}

/* YGLabeledWidget follows */

YGLabeledWidget::YGLabeledWidget (YWidget *y_widget, YGWidget *parent,
                                  YCPString label_text, YUIDimension label_ori,
                                  bool show, GType type,
                                  const char *property_name, ...)
	: YGWidget (y_widget, parent, show,
	            label_ori == YD_VERT ? GTK_TYPE_VBOX : GTK_TYPE_HBOX,
	            "spacing", LABEL_WIDGET_SPACING, NULL)
{
	// Create the field widget
	va_list args;
	va_start (args, property_name);
	m_field = GTK_WIDGET (g_object_new_valist (type, property_name, args));
	va_end (args);

	// Create the label
	m_label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (m_label), 0.0, 0.5);
	if (label_ori == YD_HORIZ)
		gtk_label_set_line_wrap (GTK_LABEL (m_label), TRUE);
	if(show) {
		gtk_widget_show (m_label);
		gtk_widget_show (m_field);
	}

	setBuddy (m_field);
	doSetLabel (label_text);

	// Set the container and show widgets
	gtk_box_pack_start (GTK_BOX (m_widget), m_label, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (m_widget), m_field, TRUE, TRUE, 0);
	m_orientation = label_ori;
}

void YGLabeledWidget::setLabelVisible (bool show)
{
	if (show)
		gtk_widget_show (m_label);
	else
		gtk_widget_hide (m_label);
}

void YGLabeledWidget::setBuddy (GtkWidget *widget)
{
	gtk_label_set_mnemonic_widget (GTK_LABEL (m_label), widget);
}

void YGLabeledWidget::doSetLabel (const YCPString &label)
{
	string str = YGUtils::mapKBAccel (label->value_cstr());
	if (str.empty()) {
		gtk_widget_hide (m_label);
	}
	else {
		// add a ':' at the end
		int last = str.length()-1;
		if (str [last] != ':' && str [last] != '.' && str [last] != ' ')
			str += ':';

		// set it as upper case
		unsigned int first = (str [0] == '_') ? 1 : 0;
		if (str [first] >= 'a' && str [first] <= 'z')
			str [first] += 'A' - 'a';

		gtk_label_set_text (GTK_LABEL (m_label), str.c_str());
		gtk_label_set_use_underline (GTK_LABEL (m_label), TRUE);
	}
}

/* YGScrolledWidget follows */
#define MAX_SCROLL_SIZE 120

YGScrolledWidget::YGScrolledWidget (YWidget *y_widget, YGWidget *parent,
                                    bool show, GType type,
                                    const char *property_name, ...)
	: YGLabeledWidget (y_widget, parent, YCPString ("<no label>"), YD_VERT, show,
	                   YGTK_TYPE_SCROLLED_WINDOW, "shadow-type", GTK_SHADOW_IN, NULL)
{
	va_list args;
	va_start (args, property_name);
	construct(type, property_name, args);
	va_end (args);

	setLabelVisible (false);
}

YGScrolledWidget::YGScrolledWidget (YWidget *y_widget, YGWidget *parent,
                                    YCPString label_text, YUIDimension label_ori,
                                    bool show, GType type,
                                    const char *property_name, ...)
	: YGLabeledWidget (y_widget, parent, label_text, label_ori, show,
	                   YGTK_TYPE_SCROLLED_WINDOW, "shadow-type", GTK_SHADOW_IN, NULL)
{
	va_list args;
	va_start (args, property_name);
	construct(type, property_name, args);
	va_end (args);
}

void YGScrolledWidget::construct (GType type, const char *property_name,
                                  va_list args)
{
	m_widget = GTK_WIDGET (g_object_new_valist (type, property_name, args));
	setBuddy (m_widget);

	setPolicy (GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (YGLabeledWidget::getWidget()), m_widget);
	gtk_widget_show (m_widget);
}

void YGScrolledWidget::setPolicy (GtkPolicyType hpolicy, GtkPolicyType vpolicy)
{
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (YGLabeledWidget::getWidget()),
	                                hpolicy, vpolicy);
	ygtk_scrolled_window_set_auto_policy (YGTK_SCROLLED_WINDOW (YGLabeledWidget::getWidget()),
		hpolicy == GTK_POLICY_AUTOMATIC ? MAX_SCROLL_SIZE : 0,
		vpolicy == GTK_POLICY_AUTOMATIC ? MAX_SCROLL_SIZE : 0);
}
