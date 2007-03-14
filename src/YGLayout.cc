//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "YGUtils.h"

#include "YSplit.h"
#include "ygtkratiobox.h"
#include "YSpacing.h"

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
	            dim == YD_HORIZ ? YGTK_TYPE_RATIO_HBOX
	                            : YGTK_TYPE_RATIO_VBOX, NULL)
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
		YGWidget *ygchild = YGWidget::get (ychild);

		gtk_container_add (GTK_CONTAINER (getWidget()), ygchild->getLayout());
		sync_stretchable (ychild);

		// set labels of YGLabeledWidgets to the same width
		// we have to do quite some work due to over-clutter on YCP progs
		while (ychild) {
			if (ychild->isContainer()) {
				// try to see if there is a YGLabeledWidget at start
				// (and ignore YSpacings)
				YContainerWidget *container = (YContainerWidget *) ychild;
				if (container->numChildren())
					for (int i = 0; i < container->numChildren(); i++) {
						ychild = container->child (i);
						if (!dynamic_cast <YSpacing *> (ychild))
							break;
					}
				else  // no kids
					break;
			}
			else {
				ygchild = YGWidget::get (ychild);
				YGLabeledWidget *labeled_child = dynamic_cast <YGLabeledWidget *> (ygchild);
				if (labeled_child && labeled_child->orientation() == YD_HORIZ) {
					if (!m_labels_group)
						m_labels_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
					gtk_size_group_add_widget (m_labels_group,
					          labeled_child->getLabelWidget());
				}
				break;
			}
		}
	}
	YGWIDGET_IMPL_CHILD_REMOVED (getWidget())

	virtual void sync_stretchable (YWidget *ychild)
	{
		IMPL
		YGtkRatioBox *box = YGTK_RATIO_BOX (getWidget());
		YGWidget *child = YGWidget::get (ychild);

		YUIDimension dim = dimension();
		bool horiz_fill = child->isStretchable (YD_HORIZ) ||
		                 ychild->hasWeight (YD_HORIZ);
		bool vert_fill  = child->isStretchable (YD_VERT) ||
		                 ychild->hasWeight (YD_VERT);

		ygtk_ratio_box_set_child_packing (box, child->getLayout(), ychild->weight (dim),
		                                  horiz_fill, vert_fill, 0, GTK_PACK_START);
		ygtk_ratio_box_set_child_expand (box, child->getLayout(),
		                                 child->isStretchable (dim));
		YGWidget::sync_stretchable();
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

class YGAlignment : public YAlignment, public YGWidget
{
	GdkPixbuf *m_background_pixbuf;

public:
	YGAlignment (const YWidgetOpt &opt, YGWidget *parent,
	             YAlignmentType halign, YAlignmentType valign)
	: YAlignment (opt, halign, valign),
	  YGWidget (this, parent, true, GTK_TYPE_ALIGNMENT, NULL)
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

		setAlignment (align [YD_HORIZ], align [YD_VERT]);
		/* The padding is used for stuff like making YCP progs nicer for
		   yast-qt wizard, so it hurts us -- it's disabled. */
		//child->setPadding (topMargin(), bottomMargin(),
		//                   leftMargin(), rightMargin());
		setMinSize (minWidth(), minHeight());
		sync_stretchable();
	}
	YGWIDGET_IMPL_CHILD_REMOVED (m_widget)

	void setAlignment (YAlignmentType halign, YAlignmentType valign)
	{
		GValue zero = YGUtils::floatToGValue (0.0);
		if (halign != YAlignUnchanged) {
			GValue xalign = YGUtils::floatToGValue (yToGtkAlign (halign));
			g_object_set_property (G_OBJECT (getWidget()), "xalign", &xalign);
			g_object_set_property (G_OBJECT (getWidget()), "xscale", &zero);
		}
		if (valign != YAlignUnchanged) {
			GValue yalign = YGUtils::floatToGValue (yToGtkAlign (valign));
			g_object_set_property (G_OBJECT (getWidget()), "yalign", &yalign);
			g_object_set_property (G_OBJECT (getWidget()), "yscale", &zero);
		}
	}

	void setPadding (int top, int bottom, int left, int right)
	{
		gtk_alignment_set_padding (GTK_ALIGNMENT (getWidget()),
		                           top, bottom, left, right);
	}

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

	YGWIDGET_IMPL_CHILD_ADDED (getWidget())
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
