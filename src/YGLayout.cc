/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>
#include "YGUI.h"
#include "YGWidget.h"
#include "YGUtils.h"

#include "YLayoutBox.h"
#include "ygtkratiobox.h"
#include "YSpacing.h"

// GtkBox-like container (actually, more like our YGtkRatioBox)
class YGLayoutBox : public YLayoutBox, public YGWidget
{
	// This group is meant to set all YGLabeledWidget with horizontal label
	// to share the same width (if they belong to the same YSplit), so they
	// look right
	GtkSizeGroup *m_labels_group;

public:
	YGLayoutBox (YWidget *parent, YUIDimension dim)
	: YLayoutBox (NULL, dim),
	  YGWidget (this, parent, true,
	            dim == YD_HORIZ ? YGTK_TYPE_RATIO_HBOX : YGTK_TYPE_RATIO_VBOX, NULL)
	{
		setBorder (0);
		m_labels_group = NULL;
	}

	~YGLayoutBox()
	{
		if (m_labels_group)
			g_object_unref (G_OBJECT (m_labels_group));
	}

	virtual string getDebugLabel() const
	{ return primary() == YD_HORIZ ? "horizontal" : "vertical"; }

	virtual void doAddChild (YWidget *ychild, GtkWidget *container)
	{
		IMPL
		YGWidget::doAddChild (ychild, container);

		// set labels of YGLabeledWidgets to the same width
		// we have to do quite some work due to over-clutter on YCP progs
		while (ychild) {
			if (ychild->hasChildren()) {  // container
				// try to see if there is a YGLabeledWidget at start
				// (and ignore YSpacings)
				YWidget *container = ychild;
				for (YWidgetListConstIterator it = container->childrenBegin();
					 it != container->childrenEnd(); it++) {
					ychild = *it;
					if (!dynamic_cast <YSpacing *> (ychild))
						break;
				}
			}
			else {
				YGWidget *ygchild = YGWidget::get (ychild);
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

	YGWIDGET_IMPL_CHILD_ADDED (getWidget())
	YGWIDGET_IMPL_CHILD_REMOVED (getWidget())

	virtual void sync_stretchable (YWidget *ychild)
	{
		IMPL
		YGtkRatioBox *box = YGTK_RATIO_BOX (getWidget());
		YGWidget *child = YGWidget::get (ychild);

		YUIDimension dim = primary();
		bool horiz_fill = ychild->stretchable (YD_HORIZ) || ychild->hasWeight (YD_HORIZ);
		bool vert_fill  = ychild->stretchable (YD_VERT) || ychild->hasWeight (YD_VERT);

		ygtk_ratio_box_set_child_packing (box, child->getLayout(), ychild->weight (dim),
		                                  horiz_fill, vert_fill, 0);
		ygtk_ratio_box_set_child_expand (box, child->getLayout(),
			ychild->stretchable (dim), isLayoutStretch (ychild, dim));
		YGWidget::sync_stretchable();
	}

	virtual void moveChild (YWidget *, int, int) {}  // ignore
};

YLayoutBox *YGWidgetFactory::createLayoutBox (YWidget *parent, YUIDimension dimension)
{
	IMPL
	return new YGLayoutBox (parent, dimension);
}

#include "YAlignment.h"

class YGAlignment : public YAlignment, public YGWidget
{
	GdkPixbuf *m_background_pixbuf;

public:
	YGAlignment (YWidget *parent, YAlignmentType halign, YAlignmentType valign)
	: YAlignment (NULL, halign, valign),
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

	virtual void doAddChild (YWidget *ychild, GtkWidget *container)
	{
		YGWidget::doAddChild (ychild, container);

		/* The padding is used for stuff like making YCP progs nicer for
		   yast-qt wizard, so it hurts us -- it's disabled. */
		//child->setPadding (topMargin(), bottomMargin(),
		//                   leftMargin(), rightMargin());
		setMinSize (minWidth(), minHeight());
	}
	YGWIDGET_IMPL_CHILD_ADDED (m_widget)
	YGWIDGET_IMPL_CHILD_REMOVED (m_widget)

	virtual void sync_stretchable (YWidget *child)
	{
		IMPL
		setAlignment (alignment (YD_HORIZ), alignment (YD_VERT));
		YGWidget::sync_stretchable();
	}

	void setAlignment (YAlignmentType halign, YAlignmentType valign)
	{
		struct inner {
			// helper -- converts YWidget YAlignmentType to Gtk's align float
			static float yToGtkAlign (YAlignmentType align)
			{
				switch (align) {
					case YAlignBegin:  return 0.0;
					default:
					case YAlignCenter: return 0.5;
					case YAlignEnd:    return 1.0;
				}
			}
		};

		float xalign, yalign, xscale, yscale;
		xalign = inner::yToGtkAlign (halign);
		yalign = inner::yToGtkAlign (valign);
		xscale = (halign == YAlignUnchanged) ? 1 : 0;
		yscale = (valign == YAlignUnchanged) ? 1 : 0;
		if (hasChildren()) {
			// special case: child has stretch opt
			if (firstChild()->stretchable (YD_HORIZ))
				xscale = 1;
			if (firstChild()->stretchable (YD_VERT))
				yscale = 1;
		}
		gtk_alignment_set (GTK_ALIGNMENT (getWidget()), xalign, yalign, xscale, yscale);
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

	virtual void moveChild (YWidget *, int, int) {};  // ignore

	virtual string getDebugLabel() const
	{
		struct inner {
			static const char *alignLabel (YAlignmentType align)
			{
				switch (align) {
					case YAlignUnchanged:
						return "unchanged";
					case YAlignBegin:
						return "begin";
					case YAlignEnd:
						return "end";
					case YAlignCenter:
						return "center";
				}
				return ""; /*not run*/
			}
		};

		string str;
		str += inner::alignLabel (alignment (YD_HORIZ));
		str += " x ";
		str += inner::alignLabel (alignment (YD_VERT));
		return str;
	}
};

YAlignment *YGWidgetFactory::createAlignment (YWidget *parent, YAlignmentType halign,
                                              YAlignmentType valign)
{
	IMPL
	return new YGAlignment (parent, halign, valign);
}

#include "YEmpty.h"

// Just an empty space.
class YGEmpty : public YEmpty, public YGWidget
{
public:
	YGEmpty (YWidget *parent)
	: YEmpty (NULL),
	  YGWidget (this, parent, true, GTK_TYPE_EVENT_BOX, NULL)
	{
		setBorder (0);
	}

	YGWIDGET_IMPL_COMMON
};

YEmpty *YGWidgetFactory::createEmpty (YWidget *parent)
{
	IMPL
	return new YGEmpty (parent);
}

#include "YSpacing.h"

// Empty space, with a fixed size.
class YGSpacing : public YSpacing, public YGWidget
{
public:
	YGSpacing (YWidget *parent, YUIDimension dim, bool stretchable, YLayoutSize_t size)
	: YSpacing (NULL, dim, stretchable, size),
	  YGWidget (this, parent, true, GTK_TYPE_EVENT_BOX, NULL)
	{
		setBorder (0);
		int width = YSpacing::size (YD_HORIZ), height = YSpacing::size (YD_VERT);
		gtk_widget_set_size_request (getWidget(), width, height);
	}

	YGWIDGET_IMPL_COMMON
};

YSpacing *YGWidgetFactory::createSpacing (YWidget *parent, YUIDimension dim,
                                          bool stretchable, YLayoutSize_t size)
{
	return new YGSpacing (parent, dim, stretchable, size);
}

#include "YReplacePoint.h"

//TEMP:
#include "YPushButton.h"

// an empty space that will get replaced
class YGReplacePoint : public YReplacePoint, public YGWidget
{
public:
	YGReplacePoint (YWidget *parent)
	: YReplacePoint (NULL),
	  YGWidget (this, parent, true, GTK_TYPE_EVENT_BOX, NULL)
	{
		setBorder (0);
	}

	YGWIDGET_IMPL_CHILD_ADDED (getWidget())
	YGWIDGET_IMPL_CHILD_REMOVED (getWidget())
};

YReplacePoint *YGWidgetFactory::createReplacePoint (YWidget *parent)
{
	IMPL
	return new YGReplacePoint (parent);
}

#include "YSquash.h"

// A-like YAlignment, YSquash messes around child settings.
// In this case, it can remove the stretchable attribute.
class YGSquash : public YSquash, public YGWidget
{
public:
	YGSquash (YWidget *parent, bool hsquash, bool vsquash)
	: YSquash (NULL, hsquash, vsquash),
	  YGWidget (this, parent, true, GTK_TYPE_EVENT_BOX, NULL)
	{
		setBorder (0);
	}

	YGWIDGET_IMPL_CHILD_ADDED (getWidget())
	YGWIDGET_IMPL_CHILD_REMOVED (getWidget())
};

YSquash *YGWidgetFactory::createSquash (YWidget *parent, bool hsquash, bool vsquash)
{
	IMPL
	return new YGSquash (parent, hsquash, vsquash);
}

