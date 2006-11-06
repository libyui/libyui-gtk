//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <stdarg.h>
#include "YGWidget.h"
#include "YGUtils.h"
#include "ygtkratiobox.h"

// default widgets border -- may be overlapped with a setBorder(..)
#define DEFAULT_BORDER 6
#define LABEL_WIDGET_SPACING 4

/* YGWidget follows */

YGWidget::YGWidget(YWidget *y_widget, YGWidget *parent, bool show,
                   GtkType type, const char *property_name, ...) :
	m_y_widget (y_widget)
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
		m_alignment = gtk_alignment_new (0.5, 0, 1, 1);
		g_object_ref (G_OBJECT (m_alignment));
		gtk_object_sink (GTK_OBJECT (m_alignment));
		gtk_widget_show (m_alignment);

		m_min_size = ygtk_min_size_new (0, 0);
		gtk_widget_show (m_min_size);
		gtk_container_add (GTK_CONTAINER (m_alignment), m_min_size);
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

	// We wait for realize to ask some layout attributes because some are
	// not yet initialized in some cases (like stretchable for YWidget contaienrs)
	g_signal_connect (G_OBJECT (m_widget), "realize", G_CALLBACK (realize_cb), this);

	if (_show)
		show();
}

YGWidget::~YGWidget()
{
	IMPL
	gtk_widget_destroy (m_alignment);
	g_object_unref (G_OBJECT (m_alignment));
}

void YGWidget::realize_cb (GtkWidget *widget, YGWidget *pThis)
{
	YWidget *ywidget = pThis->m_y_widget;
	pThis->setStretchable (YD_HORIZ, ywidget->stretchable (YD_HORIZ) ||
	                                 ywidget->hasWeight (YD_HORIZ));
	pThis->setStretchable (YD_VERT, ywidget->stretchable (YD_VERT) ||
	                                ywidget->hasWeight (YD_VERT));
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

void YGWidget::setMinSize (int width, int height)
{
	if (width)  width = YGUtils::getCharsWidth (getWidget(), width);
	if (height) height = YGUtils::getCharsHeight (getWidget(), height);

	ygtk_min_size_set (YGTK_MIN_SIZE (m_min_size), width, height);
}

static GValue floatToGValue (float num)  // helper
{
	GValue value = { 0 };
	g_value_init (&value, G_TYPE_FLOAT);
	g_value_set_float (&value, num);
	return value;
}

void YGWidget::setStretchable (YUIDimension dim, bool stretch)
{
	m_y_widget->setStretchable (dim, stretch);

	GValue scale = floatToGValue (stretch ? 1.0 : 0.0);
	if (dim == YD_HORIZ)
		g_object_set_property (G_OBJECT (m_alignment), "xscale", &scale);
	else // YD_VERT
		g_object_set_property (G_OBJECT (m_alignment), "yscale", &scale);
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
	if(show) {
		gtk_widget_show (m_label);
		gtk_widget_show (m_field);
	}
	setBuddy (m_field);
	doSetLabel (label_text);

	// Set the container and show widgets
	gtk_box_pack_start (GTK_BOX (m_widget), m_label, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (m_widget), m_field, TRUE, TRUE, 0);
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
	if (str.empty())
		gtk_widget_hide (m_label);
	else {
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
