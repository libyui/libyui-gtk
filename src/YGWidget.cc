//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

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
		m_alignment = m_widget;
	else {
		m_alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
		g_object_ref (G_OBJECT (m_alignment));
		gtk_object_sink (GTK_OBJECT (m_alignment));
		gtk_widget_show (m_alignment);

		m_min_size = ygtk_min_size_new (0, 0);
		gtk_widget_show (m_min_size);
		gtk_container_add (GTK_CONTAINER (m_alignment), m_min_size);
		gtk_container_add (GTK_CONTAINER (m_min_size), m_widget);
	}

	y_widget->setWidgetRep ((void *) this);

	// Ordinary widgets stretchable attribute is fetched at constructor,
	// while container widget is fetched at every addChild() because
	// they generally depend on the child for their stretchable attribute.
	if (!y_widget->isContainer())
			sync_stretchable();

	// Just set parent after set stretch because we don't want it to notify
	// to its container parent, since it isn't yet added to it...
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
	gtk_widget_destroy (m_alignment);
	g_object_unref (G_OBJECT (m_alignment));
}

void YGWidget::show()
{
	gtk_widget_show (m_widget);
}

YGWidget *YGWidget::get (YWidget *y_widget)
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
	gtk_container_set_border_width (GTK_CONTAINER (m_alignment), border);
}

unsigned int YGWidget::getBorder()
{
	return gtk_container_get_border_width (GTK_CONTAINER (m_alignment));
}

void YGWidget::setMinSize (unsigned int width, unsigned int height)
{
	if (width)
		ygtk_min_size_set_width (YGTK_MIN_SIZE (m_min_size), width);
	if (height)
		ygtk_min_size_set_height (YGTK_MIN_SIZE (m_min_size), height);
}

void YGWidget::setMinSizeInChars (unsigned int width, unsigned int height)
{
	if (width)
		width = YGUtils::getCharsWidth (getWidget(), width);
	if (height)
		height = YGUtils::getCharsHeight (getWidget(), height);
	setMinSize (width, height);
}

static GValue floatToGValue (float num)  // helper
{
	GValue value = { 0 };
	g_value_init (&value, G_TYPE_FLOAT);
	g_value_set_float (&value, num);
	return value;
}

/* Just tells the widget to fill the space it is given or not. You should
   also ensure its container gives it space accordingly.
   NOTE: this property should only be set during the gui construction, since
   we don't report stretchable changes to its container. The only gui dynamic
   changes are done by YReplacePoint, so we do that work there. */
void YGWidget::setStretchable (YUIDimension dim, bool stretch)
{
	GValue scale = floatToGValue (stretch ? 1.0 : 0.0);
	g_object_set_property (G_OBJECT (m_alignment),
	                       (dim == YD_HORIZ) ? "xscale" : "yscale", &scale);
}

void YGWidget::sync_stretchable()
{
	setStretchable (YD_HORIZ, isStretchable (YD_HORIZ));
	setStretchable (YD_VERT, isStretchable (YD_VERT));
}

bool YGWidget::isStretchable (YUIDimension dim)
{
	YContainerWidget *container = dynamic_cast <YContainerWidget *> (m_y_widget);
	if (container && !container->hasChildren())
		return false;  // some YWidget containers would crash if they don't have kids
	return m_y_widget->stretchable (dim) || m_y_widget->hasWeight (dim);
}

// helper -- converts YWidget YAlignmentType to Gtk's align float
static float yToGtkAlign (YAlignmentType align)
{
	switch (align) {
		case YAlignBegin:  return 0.0;
		case YAlignCenter: return 0.5;
		case YAlignEnd:    return 1.0;
		default: return -1;
	}
}

void YGWidget::setAlignment (YAlignmentType halign, YAlignmentType valign)
{
	if (halign != YAlignUnchanged) {
		GValue xalign = floatToGValue (yToGtkAlign (halign));
		g_object_set_property (G_OBJECT (m_alignment), "xalign", &xalign);
	}
	if (valign != YAlignUnchanged) {
		GValue yalign = floatToGValue (yToGtkAlign (valign));
		g_object_set_property (G_OBJECT (m_alignment), "yalign", &yalign);
	}
}

void YGWidget::setPadding (int top, int bottom, int left, int right)
{
	gtk_alignment_set_padding (GTK_ALIGNMENT (m_alignment), top, bottom, left, right);
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
		// label tweaking
		if (str [str.length()-1] != ':' && str [str.length()-1] != '.')
			str += ':';
		unsigned int first_ch = (str [0] == '_') ? 1 : 0;
		if (str [first_ch] >= 'a' && str [first_ch] <= 'z')
			str [first_ch] += 'A' - 'a';

		gtk_label_set_text (GTK_LABEL (m_label), str.c_str());
		gtk_label_set_use_underline (GTK_LABEL (m_label), TRUE);
	}
}

/* YGScrolledWidget follows */

YGScrolledWidget::YGScrolledWidget (YWidget *y_widget, YGWidget *parent,
                                    bool show, GType type,
                                    const char *property_name, ...)
	: YGLabeledWidget (y_widget, parent, YCPString ("<no label>"), YD_VERT, show,
	                   GTK_TYPE_SCROLLED_WINDOW, "shadow-type", GTK_SHADOW_IN, NULL)
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
	                   GTK_TYPE_SCROLLED_WINDOW, "shadow-type", GTK_SHADOW_IN, NULL)
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

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (YGLabeledWidget::getWidget()),
	                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (YGLabeledWidget::getWidget()), m_widget);
	gtk_widget_show (m_widget);
}
