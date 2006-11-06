//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"

#include "YSplit.h"
#include "ygtkratiobox.h"

// GtkBox-like container (actually, more like our YGtkRatioBox)
class YGSplit : public YSplit, public YGWidget
{
public:
	YGSplit (const YWidgetOpt &opt, YGWidget *parent, YUIDimension dim)
	: YSplit (opt, dim),
	  YGWidget (this, parent, true,
	            dim == YD_HORIZ ? YGTK_TYPE_RATIO_HBOX : YGTK_TYPE_RATIO_VBOX, NULL)
	{
		setBorder (0);
	}

	virtual void childAdded (YWidget *ychild)
	{
		YUIDimension this_dir, opposite_dir;
		this_dir = primaryDimension();
		opposite_dir = secondaryDimension();

		YGWidget *ygchild = YGWidget::get (ychild);
		GtkWidget *child = ygchild->getLayout();

		float ratio = ychild->stretchable (this_dir) ? 1 : 0;
		if (ychild->hasWeight (dimension()))
			ratio = ychild->weight (dimension());

printf ("split child (%s) padding: %d\n", ygchild->getWidgetName(), ygchild->getBorder());
		ygtk_ratio_box_pack (YGTK_RATIO_BOX (getWidget()), child, ratio, TRUE, 0);
	}

	virtual void childRemoved (YWidget *ychild)
	{
		GtkWidget *child = YGWidget::get (ychild)->getLayout();
		gtk_container_remove (GTK_CONTAINER (getWidget()), child);
	}

	virtual void moveChild (YWidget *, long, long) {};  // ignore
};

YContainerWidget *
YGUI::createSplit (YWidget *parent, YWidgetOpt &opt, YUIDimension dimension)
{
printf ("using ygsplit\n");
	IMPL
	return new YGSplit (opt, YGWidget::get (parent), dimension);
}

#include "YAlignment.h"

// GtkAlignment-like container , but we make it of type GtkEventBox because
// we already install widgets on GtkAlignments, so this is not much more
// than a proxy.
class YGAlignment : public YAlignment, public YGWidget
{
	GdkPixbuf *m_background_pixbuf;

public:
	YGAlignment (const YWidgetOpt &opt, YGWidget *parent,
	             YAlignmentType halign, YAlignmentType valign)
	: YAlignment (opt, halign, valign),
	  YGWidget (this, parent, true, GTK_TYPE_EVENT_BOX, NULL)
	{
		setBorder (0);
		m_background_pixbuf = 0;
	}

	virtual ~YGAlignment()
	{
		if (m_background_pixbuf)
			g_object_unref (G_OBJECT (m_background_pixbuf));
	}

	virtual void childAdded (YWidget *ychild)
	{
		YGWidget *child = YGWidget::get (ychild);
		gtk_container_add (GTK_CONTAINER (getWidget()), child->getLayout());

		if (align [YD_HORIZ] != YAlignUnchanged)
			child->setStretchable (YD_HORIZ, false);
		if (align [YD_VERT] != YAlignUnchanged)
			child->setStretchable (YD_VERT, false);
		child->setAlignment (align [YD_HORIZ], align [YD_VERT]);
		child->setPadding (topMargin(), bottomMargin(), leftMargin(), rightMargin());
		child->setMinSize (minWidth(), minHeight());
	}
	YGWIDGET_IMPL_CHILD_REMOVED (m_widget)

	virtual void setBackgroundPixmap (string filename)
	{
		if (!hasChildren()) {
			y2warning ("setting a background pixmap on a YAlignment with no children");
			return;
		}

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

	static gboolean expose_event_cb (GtkWidget *widget, GdkEventExpose *event,
	                                 YGAlignment *pThis)
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
		return FALSE;
	}

	virtual void moveChild (YWidget *, long, long) {};  // ignore
};

YContainerWidget *
YGUI::createAlignment (YWidget *parent, YWidgetOpt &opt,
                       YAlignmentType halign, YAlignmentType valign)
{
printf ("using ygalignment\n");
	IMPL
	return new YGAlignment (opt, YGWidget::get (parent), halign, valign);
}

#include "YEmpty.h"

// Just an empty space.
class YGEmpty : public YEmpty, public YGWidget
{
public:
	YGEmpty (const YWidgetOpt &opt, YGWidget *parent)
	: YEmpty (opt),
	  YGWidget (this, parent, true, GTK_TYPE_EVENT_BOX, NULL)
	{
		setBorder (0);
	}
};

YWidget *
YGUI::createEmpty (YWidget *parent, YWidgetOpt &opt)
{
printf ("using ygempty\n");
	IMPL
	return new YGEmpty (opt, YGWidget::get (parent));
}

#include "YSpacing.h"

// Empty space, with a fixed size.
class YGSpacing : public YSpacing, public YGWidget
{
public:
	YGSpacing (const YWidgetOpt &opt, YGWidget *parent,
	                      float size, bool horizontal, bool vertical)
	: YSpacing (opt, size, horizontal, vertical),
	  YGWidget (this, parent, true, GTK_TYPE_EVENT_BOX, NULL)
	{
		setBorder (0);
		gtk_widget_set_size_request (getWidget(), width(), height());
	}
};

YWidget *
YGUI::createSpacing (YWidget *parent, YWidgetOpt & opt, float size,
                     bool horiz, bool vert)
{
printf ("using ygspacing\n");
	IMPL
	return new YGSpacing (opt, YGWidget::get (parent), size, horiz, vert);
}

#include "YReplacePoint.h"

// an empty space that will get replaced
class YGReplacePoint : public YReplacePoint, public YGWidget
{
public:
	YGReplacePoint (const YWidgetOpt &opt, YGWidget *parent)
	: YReplacePoint (opt),
	  YGWidget (this, parent, true, GTK_TYPE_EVENT_BOX, NULL)
	{
		setBorder (0);
	}

	YGWIDGET_IMPL_CHILD_ADDED (m_widget)
	YGWIDGET_IMPL_CHILD_REMOVED (m_widget)
};

YContainerWidget *
YGUI::createReplacePoint( YWidget *parent, YWidgetOpt & opt )
{
printf ("using ygreplacepoint\n");
	IMPL
	return new YGReplacePoint (opt, YGWidget::get (parent));
}

#include "YSquash.h"

// A-like YAlignment, YSquash messes around child settings.
// In this case, it can remove the stretchable attribute.
class YGSquash : public YSquash, public YGWidget
{
public:
	YGSquash (const YWidgetOpt &opt, YGWidget *parent,
	          bool hsquash, bool vsquash)
	: YSquash (opt, hsquash, vsquash),
	  YGWidget (this, parent, true, GTK_TYPE_EVENT_BOX, NULL)
	{
		setBorder (0);
	}

	virtual void childAdded (YWidget *ychild)
	{
		YGWidget *child = YGWidget::get (ychild);
		gtk_container_add (GTK_CONTAINER (getWidget()), child->getLayout());

		if (ychild->stretchable (YD_HORIZ))
			child->setStretchable (YD_HORIZ, !squash [YD_HORIZ]);
		if (ychild->stretchable (YD_VERT))
			child->setStretchable (YD_VERT, !squash [YD_VERT]);
	}
	YGWIDGET_IMPL_CHILD_REMOVED(m_widget)
};

YContainerWidget *
YGUI::createSquash (YWidget *parent, YWidgetOpt &opt,
                    bool hsquash, bool vsquash)
{
printf ("using ygsquash\n");
	IMPL
	return new YGSquash (opt, YGWidget::get (parent), hsquash, vsquash);
}
