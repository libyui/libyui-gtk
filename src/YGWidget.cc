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

YGWidget::YGWidget(YWidget *ywidget, YWidget *yparent, bool show,
                   GtkType type, const char *property_name, ...)
	: m_ywidget (ywidget)
{
	va_list args;
	va_start (args, property_name);
	construct (ywidget, yparent, show, type, property_name, args);
	va_end (args);
}

void YGWidget::construct (YWidget *ywidget, YWidget *yparent, bool _show,
                          GType type, const char *property_name, va_list args)
{
	m_widget = GTK_WIDGET (g_object_new_valist (type, property_name, args));

	if (type == GTK_TYPE_WINDOW)
		m_adj_size = m_widget;
	else {
		m_adj_size = ygtk_adj_size_new();
		g_object_ref_sink (G_OBJECT (m_adj_size));
		gtk_widget_show (m_adj_size);
		gtk_container_add (GTK_CONTAINER (m_adj_size), m_widget);
	}

	// Split by two so that with another widget it will have full border...
	setBorder (DEFAULT_BORDER / 2);
	if (_show)
		show();

	ywidget->setWidgetRep ((void *) this);
	if (yparent) {
		ywidget->setParent (yparent);
		yparent->addChild (ywidget);
	}
}

YGWidget::~YGWidget()
{
	IMPL
	if (YGUI::ui()->eventPendingFor (m_ywidget))
		YGUI::ui()->m_event_handler.consumePendingEvent();
	// remove children if container?
#if 0
	struct inner {
		static void foreach_child_cb (GtkWidget *child, GtkContainer *container)
		{ gtk_container_remove (container, child); }
	};
	if (GTK_IS_CONTAINER (m_widget))
		gtk_container_foreach (GTK_CONTAINER (m_widget),
			(GtkCallback) inner::foreach_child_cb, m_widget);
#endif
	gtk_widget_destroy (m_adj_size);
	g_object_unref (G_OBJECT (m_adj_size));
}

void YGWidget::show()
{
	gtk_widget_show (m_widget);
}

YGWidget *YGWidget::get (YWidget *ywidget)
{
	//g_assert (ywidget->widgetRep() != NULL);
	return (YGWidget *) ywidget->widgetRep();
}

bool YGWidget::doSetKeyboardFocus()
{
	gtk_widget_grab_focus (GTK_WIDGET (getWidget()));
	return gtk_widget_is_focus (GTK_WIDGET (getWidget()));
}

void YGWidget::doSetEnabled (bool enabled)
{
	gtk_widget_set_sensitive (getLayout(), enabled);
}

void YGWidget::doSetUseBoldFont (bool useBold)
{
   	PangoWeight weight = useBold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL;
	YGUtils::setWidgetFont (getWidget(), weight, PANGO_SCALE_MEDIUM);
}

void YGWidget::doAddChild (YWidget *ychild, GtkWidget *container)
{
	GtkWidget *child = YGWidget::get (ychild)->getLayout();
	gtk_container_add (GTK_CONTAINER (container), child);
}

void YGWidget::doRemoveChild (YWidget *ychild, GtkWidget *container)
{
	/* Note: removeChild() is generally a result of a widget being removed as it
	   will remove itself from the parent. But YGWidget deconstructor would run
	   before the YWidget one, as that's the order we have been using, so we
	   can't use it, we can't retrieve the GTK widget then. However, this is a
	   non-issue, as ~YGWidget will destroy the widget, and GTK will remove it
	   from the parent. */
	if (!ychild->beingDestroyed()) {
		GtkWidget *child = YGWidget::get (ychild)->getLayout();
		gtk_container_remove (GTK_CONTAINER (container), child);
	}
}

int YGWidget::getPreferredSize (YUIDimension dimension)
{
	// We might want to try to do some caching here..
	GtkRequisition req;
	gtk_widget_size_request (m_adj_size, &req);
	return dimension == YD_HORIZ ? req.width : req.height;
}

#include "ygtkfixed.h"

void YGWidget::doSetSize (int width, int height)
{
	GtkWidget *parent = 0;
	if (m_ywidget->parent())
		parent = YGWidget::get (m_ywidget->parent())->getWidget();

	if (parent && YGTK_IS_FIXED (parent))
		ygtk_fixed_set_child_size (YGTK_FIXED (parent), m_adj_size, width, height);
}

void YGWidget::emitEvent(YEvent::EventReason reason, bool if_notify,
                         bool if_not_pending, bool immediate)
{
	struct inner
	{
		static gboolean dispatchEvent (gpointer data)
		{
			YWidgetEvent *event = (YWidgetEvent *) data;
			if (!YGUI::ui()->eventPendingFor (event->widget()))
				YGUI::ui()->sendEvent (event);
			return FALSE;
		}
	};

	if (!if_notify || m_ywidget->notify())
	{
		if (!immediate)
			g_timeout_add (250, inner::dispatchEvent, new YWidgetEvent (m_ywidget, reason));
		else if (!if_not_pending || !YGUI::ui()->eventPendingFor (m_ywidget))
		{
			if (immediate)
				YGUI::ui()->sendEvent (new YWidgetEvent (m_ywidget, reason));
		}
	}
}

void YGWidget::setBorder (unsigned int border)
{
	IMPL
	gtk_container_set_border_width (GTK_CONTAINER (m_adj_size), border);
}

void YGWidget::setMinSize (unsigned int width, unsigned int height)
{
	IMPL
	ygtk_adj_size_set_min (YGTK_ADJ_SIZE (m_adj_size), width, height);
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

/* YGLabeledWidget follows */

YGLabeledWidget::YGLabeledWidget (YWidget *ywidget, YWidget *parent,
                                  const std::string &label_text, YUIDimension label_ori,
                                  bool show, GType type,
                                  const char *property_name, ...)
	: YGWidget (ywidget, parent, show,
//	            label_ori == YD_VERT ? GTK_TYPE_VBOX : GTK_TYPE_HBOX,
	            GTK_TYPE_VBOX,
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
/*	if (label_ori == YD_HORIZ)
		gtk_label_set_line_wrap (GTK_LABEL (m_label), TRUE);*/
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

void YGLabeledWidget::doSetLabel (const std::string &label)
{
	string str = YGUtils::mapKBAccel (label);
	if (str.empty())
		gtk_widget_hide (m_label);
	else {
		gtk_widget_show (m_label);

		// add a ':' at the end
		int last = str.length()-1;
		if (str [last] != ':' && str [last] != '.' && str [last] != ' ')
			str += ':';

/*		// set it as upper case
		unsigned int first = (str [0] == '_') ? 1 : 0;
		if (str [first] >= 'a' && str [first] <= 'z')
			str [first] += 'A' - 'a';
*/
		gtk_label_set_text (GTK_LABEL (m_label), str.c_str());
		gtk_label_set_use_underline (GTK_LABEL (m_label), TRUE);
	}
}

/* YGScrolledWidget follows */
#define MAX_SCROLL_WIDTH 120

YGScrolledWidget::YGScrolledWidget (YWidget *ywidget, YWidget *parent,
                                    bool show, GType type,
                                    const char *property_name, ...)
	: YGLabeledWidget (ywidget, parent, string(), YD_VERT, show,
	                   GTK_TYPE_SCROLLED_WINDOW, "shadow-type", GTK_SHADOW_IN, NULL)
{
	va_list args;
	va_start (args, property_name);
	construct(type, property_name, args);
	va_end (args);
	setLabelVisible (false);
}

YGScrolledWidget::YGScrolledWidget (YWidget *ywidget, YWidget *parent,
                                    const std::string &label_text, YUIDimension label_ori,
                                    bool show, GType type,
                                    const char *property_name, ...)
	: YGLabeledWidget (ywidget, parent, label_text, label_ori, show,
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

	setPolicy (GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (YGLabeledWidget::getWidget()), m_widget);
	gtk_widget_show (m_widget);
}

void YGScrolledWidget::setPolicy (GtkPolicyType hpolicy, GtkPolicyType vpolicy)
{
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (YGLabeledWidget::getWidget()),
	                                hpolicy, vpolicy);
}

