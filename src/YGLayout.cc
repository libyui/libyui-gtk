//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGLayout.h"

// Look for YGLayout.h for the actual implementation
// Since these classes are so thin and are only included
// in one point, there isn't anything to win by splitting them.

YContainerWidget *
YGUI::createSplit (YWidget *parent, YWidgetOpt &opt,
                   YUIDimension dimension)
{
	IMPL
	return new YGSplit (opt, YGWidget::get (parent), dimension);
}

YWidget *
YGUI::createEmpty (YWidget *parent, YWidgetOpt &opt)
{
	IMPL
	return new YGEmpty (opt, YGWidget::get (parent));
}

YWidget *
YGUI::createSpacing (YWidget *parent, YWidgetOpt & opt, float size,
                     bool horizontal, bool vertical)
{
	IMPL
	return new YGSpacing (opt, YGWidget::get (parent),
	                      size, horizontal, vertical);
}

YContainerWidget *
YGUI::createReplacePoint( YWidget *parent, YWidgetOpt & opt )
{
	IMPL
	return new YGReplacePoint (opt, YGWidget::get (parent));
}

YContainerWidget *
YGUI::createSquash (YWidget *parent, YWidgetOpt &opt,
                    bool hsquash, bool vsquash)
{
	IMPL
	return new YGSquash (opt, YGWidget::get (parent), hsquash, vsquash);
}

YContainerWidget *
YGUI::createAlignment (YWidget *parent, YWidgetOpt &opt,
                       YAlignmentType halign, YAlignmentType valign)
{
	IMPL
	return new YGAlignment (opt, YGWidget::get (parent),
	                        halign, valign);
}

YGAlignment::YGAlignment (const YWidgetOpt &opt, YGWidget *parent,
                          YAlignmentType halign, YAlignmentType valign)
	: YAlignment (opt, halign, valign),
	  YGWidget (this, parent, true, GTK_TYPE_EVENT_BOX, NULL)
{
	m_background_pixbuf = 0;
	setBorder (0);

	m_fixed = gtk_fixed_new();
	gtk_container_add (GTK_CONTAINER (getWidget()), m_fixed);
	gtk_widget_show (m_fixed);
}

YGAlignment::~YGAlignment()
{
	if (m_background_pixbuf)
		g_object_unref (G_OBJECT (m_background_pixbuf));
}

void YGAlignment::setBackgroundPixmap (string filename)
{
	// YAlignment will prepend a path to the image
	YAlignment::setBackgroundPixmap (filename);
	filename = YAlignment::backgroundPixmap();

	if (m_background_pixbuf)
		g_object_unref (G_OBJECT (m_background_pixbuf));

	if (filename.empty()) {
		m_background_pixbuf = 0;
		g_signal_handlers_disconnect_by_func (G_OBJECT (getWidget()),
		                                      (void*) expose_event_cb, this);
	}
	else {
		GError *error = 0;
		m_background_pixbuf = gdk_pixbuf_new_from_file (filename.c_str(), &error);
		if (!m_background_pixbuf)
			g_warning ("Setting YAlignment background - couldn't load image '%s' - %s",
			           filename.c_str(), error->message);
		else
			g_signal_connect (G_OBJECT (getWidget()), "expose-event",
			                  G_CALLBACK (expose_event_cb), this);
	}
}

gboolean YGAlignment::expose_event_cb (GtkWidget *widget,
                GdkEventExpose *event, YGAlignment *pThis)
{
	int x, y, width, height;
	x = widget->allocation.x;
	y = widget->allocation.y;
	width = widget->allocation.width;
	height = widget->allocation.height;

	cairo_t *cr = gdk_cairo_create (widget->window);

	gdk_cairo_set_source_pixbuf (cr, pThis->m_background_pixbuf, 0, 0);
	cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);

	cairo_rectangle (cr, x, y, width, height);
	cairo_fill (cr);

	cairo_destroy (cr);

	GtkContainer *container = GTK_CONTAINER (widget);
	gtk_container_propagate_expose (container, pThis->m_fixed, event);
	return TRUE;
}

GtkFixed *YGAlignment::getFixed()
{
	return GTK_FIXED (m_fixed);
}
