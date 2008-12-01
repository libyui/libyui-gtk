/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include "YGUI.h"
#include "YGWidget.h"
#include "YGUtils.h"

/* Our layout stuff is a hybrid between GTK and libyui. We use libyui
   for YLayoutBox and a couple of other widgets, but we do things GTK
   friendly, so for single-child containers like GtkNotebook we don't
   have to do any work. */

#include "ygtkfixed.h"

static void doMoveChild (GtkWidget *fixed, YWidget *ychild, int x, int y)
{
	GtkWidget *child = YGWidget::get (ychild)->getLayout();
	ygtk_fixed_set_child_pos (YGTK_FIXED (fixed), child, x, y);
}

#define YGLAYOUT_INIT                                          \
	ygtk_fixed_setup (YGTK_FIXED (getWidget()), preferred_size_cb, set_size_cb, this);
#define YGLAYOUT_PREFERRED_SIZE_IMPL(ParentClass) \
	static void preferred_size_cb (YGtkFixed *fixed, gint *width, gint *height, \
	                               gpointer pThis) { \
		*width = ((ParentClass *) pThis)->ParentClass::preferredWidth(); \
		*height = ((ParentClass *) pThis)->ParentClass::preferredHeight(); \
	}
#define YGLAYOUT_SET_SIZE_IMPL(ParentClass) \
	static void set_size_cb (YGtkFixed *fixed, gint width, gint height, \
	                         gpointer pThis) { \
		((ParentClass *) pThis)->ParentClass::setSize (width, height); \
	} \
	virtual void setSize (int width, int height) { doSetSize (width, height); } \
	virtual void moveChild (YWidget *ychild, int x, int y)      \
	{ doMoveChild (getWidget(), ychild, x, y); }                \
	virtual void setEnabled (bool enabled) {                    \
		ParentClass::setEnabled (enabled);                      \
		doSetEnabled (enabled);                                 \
	}

#include "YPushButton.h"
#include "YMenuButton.h"

class ButtonHeightGroup
{
GtkSizeGroup *group;

public:
	ButtonHeightGroup() { group = NULL; }

	void addWidget (YWidget *ywidget)
	{
		if (dynamic_cast <YPushButton *> (ywidget) ||
		    dynamic_cast <YMenuButton *> (ywidget)) {
			bool create_group = !group;
			if (create_group)
				group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
			gtk_size_group_add_widget (group, YGWidget::get (ywidget)->getLayout());
			if (create_group)
				g_object_unref (G_OBJECT (group));
		}
	}
};

#include "YLayoutBox.h"

class YGLayoutBox : public YLayoutBox, public YGWidget
{
// set all buttons in a HBox the same height (some may have icons)
ButtonHeightGroup group;

public:
	YGLayoutBox (YWidget *parent, YUIDimension dim)
	: YLayoutBox (NULL, dim),
	  YGWidget (this, parent, YGTK_TYPE_FIXED, NULL)
	{
		setBorder (0);
		YGLAYOUT_INIT
	}

	virtual void addChild (YWidget *ychild)
	{
		YWidget::addChild (ychild);
		doAddChild (ychild, getWidget());
		if (primary() == YD_HORIZ)
			group.addWidget (ychild);
	}
	YGWIDGET_IMPL_CHILD_REMOVED (getWidget())
	YGLAYOUT_PREFERRED_SIZE_IMPL (YLayoutBox)
	YGLAYOUT_SET_SIZE_IMPL (YLayoutBox)
};

YLayoutBox *YGWidgetFactory::createLayoutBox (YWidget *parent, YUIDimension dimension)
{
	IMPL
	return new YGLayoutBox (parent, dimension);
}

#if YAST2_VERSION >= 2017006
#include "YButtonBox.h"

class YGButtonBox : public YButtonBox, public YGWidget
{
ButtonHeightGroup group;

public:
	YGButtonBox (YWidget *parent)
	: YButtonBox (NULL),
	  YGWidget (this, parent, YGTK_TYPE_FIXED, NULL)
	{
		setBorder (0);
		// YUI system variable test for layout policy doesn't work flawlessly
		setLayoutPolicy (gnomeLayoutPolicy());
		YGLAYOUT_INIT
	}

	virtual void addChild (YWidget *ychild)
	{
		YWidget::addChild (ychild);
		doAddChild (ychild, getWidget());
		group.addWidget (ychild);
	}
	YGWIDGET_IMPL_CHILD_REMOVED (getWidget())
	YGLAYOUT_PREFERRED_SIZE_IMPL (YButtonBox)
	YGLAYOUT_SET_SIZE_IMPL (YButtonBox)
};

YButtonBox *YGWidgetFactory::createButtonBox (YWidget *parent)
{
	IMPL
	return new YGButtonBox (parent);
}

#endif

#include "YAlignment.h"

class YGAlignment : public YAlignment, public YGWidget
{
	GdkPixbuf *m_background_pixbuf;

public:
	YGAlignment (YWidget *parent, YAlignmentType halign, YAlignmentType valign)
	: YAlignment (NULL, halign, valign),
	  YGWidget (this, parent, YGTK_TYPE_FIXED, NULL)
	{
		setBorder (0);
		m_background_pixbuf = 0;
		YGLAYOUT_INIT
	}

	virtual ~YGAlignment()
	{
		if (m_background_pixbuf)
			g_object_unref (G_OBJECT (m_background_pixbuf));
	}

	YGWIDGET_IMPL_CHILD_ADDED (m_widget)
	YGWIDGET_IMPL_CHILD_REMOVED (m_widget)
	YGLAYOUT_PREFERRED_SIZE_IMPL (YAlignment)
	YGLAYOUT_SET_SIZE_IMPL (YAlignment)

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
	  YGWidget (this, parent, GTK_TYPE_EVENT_BOX, NULL)
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
	  YGWidget (this, parent, YGTK_TYPE_FIXED, NULL)
	{
		setBorder (0);
		YGLAYOUT_INIT
	}

	YGWIDGET_IMPL_COMMON
	YGLAYOUT_PREFERRED_SIZE_IMPL (YSpacing)
	static void set_size_cb (YGtkFixed *fixed, gint width, gint height, 
	                         gpointer pThis) {}
};

YSpacing *YGWidgetFactory::createSpacing (YWidget *parent, YUIDimension dim,
                                          bool stretchable, YLayoutSize_t size)
{
	return new YGSpacing (parent, dim, stretchable, size);
}

#include "YReplacePoint.h"

// an empty space that will get replaced
class YGReplacePoint : public YReplacePoint, public YGWidget
{
public:
	YGReplacePoint (YWidget *parent)
	: YReplacePoint (NULL),
	  YGWidget (this, parent, GTK_TYPE_EVENT_BOX, NULL)
	{
		setBorder (0);
	}

	YGWIDGET_IMPL_COMMON
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
	  YGWidget (this, parent, GTK_TYPE_EVENT_BOX, NULL)
	{
		setBorder (0);
	}

	YGWIDGET_IMPL_COMMON
	YGWIDGET_IMPL_CHILD_ADDED (getWidget())
	YGWIDGET_IMPL_CHILD_REMOVED (getWidget())
};

YSquash *YGWidgetFactory::createSquash (YWidget *parent, bool hsquash, bool vsquash)
{
	IMPL
	return new YGSquash (parent, hsquash, vsquash);
}

