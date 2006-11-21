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
	// This group is meant to set all YGLabeledWidget with horizontal label
	// to share the same width (if they belong to the same YSplit), so they
	// look right
	GtkSizeGroup *m_labels_group;

public:
	YGSplit (const YWidgetOpt &opt, YGWidget *parent, YUIDimension dim)
	: YSplit (opt, dim),
	  YGWidget (this, parent, true,
	            dim == YD_HORIZ ? YGTK_TYPE_RATIO_HBOX : YGTK_TYPE_RATIO_VBOX, NULL)
	{
		setBorder (0);
		m_labels_group = NULL;
	}

	~YGSplit()
	{
		if (m_labels_group)
			g_object_unref (G_OBJECT (m_labels_group));
	}

	virtual void childAdded (YWidget *ychild)
	{
		IMPL
		YUIDimension this_dir, opposite_dir;
		this_dir = primaryDimension();
		opposite_dir = secondaryDimension();

		YGWidget *ygchild = YGWidget::get (ychild);
		GtkWidget *child = ygchild->getLayout();

		YGtkRatioBox *box = YGTK_RATIO_BOX (getWidget());
		ygtk_ratio_box_pack_start (box, child, ychild->weight (this_dir), TRUE, 0);
		if (ychild->hasWeight (this_dir))
			ygchild->setStretchable (this_dir, true);
		setChildStretchable (ychild);
#ifdef IMPL_DEBUG
		fprintf (stderr, "added child: %s - stretch: %d - weight: %ld\n", ygchild->getWidgetName(),
		         ychild->stretchable (this_dir), ychild->weight (this_dir));
#endif
		// stretchable property changes at every addchild for containers
//		YGWidget::setStretchable (this_dir, YWidget::stretchable (this_dir));
//		YGWidget::setStretchable (opposite_dir, YWidget::stretchable (opposite_dir));
		if (!YWidget::stretchable (opposite_dir))
			YGWidget::setAlignment (YAlignCenter, YAlignCenter);
printf ("split will be set stretchable: %d x %d\n", YWidget::stretchable (this_dir), YWidget::stretchable (opposite_dir));
		// set labels of YGLabeledWidgets to the same width
		YGLabeledWidget *labeled_child = dynamic_cast <YGLabeledWidget *> (ygchild);
		if (labeled_child && labeled_child->orientation() == YD_HORIZ) {
			if (!m_labels_group)
				m_labels_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
			gtk_size_group_add_widget (m_labels_group, labeled_child->getLabelWidget());
		}
	}
	YGWIDGET_IMPL_CHILD_REMOVED (getWidget())

	void setChildStretchable (YWidget *ychild)
	{
		YGtkRatioBox *box = YGTK_RATIO_BOX (getWidget());
		YUIDimension dim = dimension();
		YGWidget *child = YGWidget::get (ychild);

		if (ychild->hasWeight (dim))
			ygtk_ratio_box_set_child_ratio (box, child->getLayout(), ychild->weight (dim));
		else if (child->isStretchable (dim))
			ygtk_ratio_box_set_child_expand (YGTK_RATIO_BOX (getWidget()),
		                                   child->getLayout(), TRUE);
		else
			ygtk_ratio_box_set_child_expand (YGTK_RATIO_BOX (getWidget()),
		                                   child->getLayout(), FALSE);
	}

	virtual void moveChild (YWidget *, long, long) {};  // ignore
};

YContainerWidget *
YGUI::createSplit (YWidget *parent, YWidgetOpt &opt, YUIDimension dimension)
{
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

		child->setAlignment (align [YD_HORIZ], align [YD_VERT]);
		/* The padding is set to make things nicer for yast-qt wizard, but it doesn't for us
		   -- just ignore it. */
		//child->setPadding (topMargin(), bottomMargin(), leftMargin(), rightMargin());
		child->setMinSize (minWidth(), minHeight());

		// don't let children fill up given space if alignment is set
		if (align [YD_HORIZ] != YAlignUnchanged)
			child->setStretchable (YD_HORIZ, false);
		if (align [YD_VERT] != YAlignUnchanged)
			child->setStretchable (YD_VERT, false);
	}
	YGWIDGET_IMPL_CHILD_REMOVED (m_widget)

	virtual void setBackgroundPixmap (string filename)
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

	static gboolean expose_event_cb (GtkWidget *widget, GdkEventExpose *event,
	                                 YGAlignment *pThis)
	{
		GtkAllocation *alloc = &widget->allocation;
		cairo_t *cr = gdk_cairo_create (widget->window);

		gdk_cairo_set_source_pixbuf (cr, pThis->m_background_pixbuf, 0, 0);
		cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);

		cairo_rectangle (cr, alloc->x, alloc->y, alloc->width, alloc->height);
		cairo_fill (cr);

		cairo_destroy (cr);

		gtk_container_propagate_expose (GTK_CONTAINER (widget),
		                                GTK_BIN (widget)->child, event);
		return TRUE;
	}

	virtual void moveChild (YWidget *, long, long) {};  // ignore
};

YContainerWidget *
YGUI::createAlignment (YWidget *parent, YWidgetOpt &opt,
                       YAlignmentType halign, YAlignmentType valign)
{
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

	virtual void childAdded (YWidget *ychild)
	{
		GtkWidget *child_widget = YGWidget::get (ychild)->getLayout();
		gtk_container_add (GTK_CONTAINER (getWidget()), child_widget);

		// notify parent container of stretchable changes
		YWidget *parent, *child;
		for (parent = yParent(), child = this; parent;
		     child = parent, parent = parent->yParent()) {
			YSplit *split = dynamic_cast <YSplit *> (parent);
			if (split && split->hasChildren())
				((YGSplit *) split)->setChildStretchable (child);
		}
	}

	YGWIDGET_IMPL_CHILD_REMOVED (getWidget())
};

YContainerWidget *
YGUI::createReplacePoint( YWidget *parent, YWidgetOpt & opt )
{
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

	YGWIDGET_IMPL_CHILD_ADDED (getWidget())
	YGWIDGET_IMPL_CHILD_REMOVED (getWidget())
};

YContainerWidget *
YGUI::createSquash (YWidget *parent, YWidgetOpt &opt,
                    bool hsquash, bool vsquash)
{
	IMPL
	return new YGSquash (opt, YGWidget::get (parent), hsquash, vsquash);
}
