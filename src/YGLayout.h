#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YSplit.h"
#include "YEmpty.h"
#include "YSpacing.h"
#include "YSquash.h"
#include "YReplacePoint.h"
#include "YGWidget.h"

#define YGWIDGET_DEBUG_NICESIZE_CHAIN(ParentClass) \
	virtual long nicesize (YUIDimension dim) \
	{ \
		long ret = ParentClass::nicesize (dim); \
		fprintf (stderr, "NiceSize for '%s' %s %ld\n", \
			 getWidgetName(), dim == YD_HORIZ ? "width" : "height", ret); \
		return ret; \
	}

#define YGWIDGET_CONTAINER_IMPL(ParentClass) \
	YGWIDGET_IMPL_SET_ENABLING \
	YGWIDGET_DEBUG_NICESIZE_CHAIN(ParentClass) \
	YGWIDGET_IMPL_SET_SIZE_CHAIN(ParentClass)  \
	virtual bool setKeyboardFocus() IMPL_RET(false) \
	virtual void startMultipleChanges() IMPL \
	virtual void doneMultipleChanges() IMPL \
	virtual void saveUserInput (YMacroRecorder *macroRecorder) IMPL

// YSplit

class YGSplit : public YSplit, public YGWidget
{
public:
	YGSplit (const YWidgetOpt &opt, YGWidget *parent, YUIDimension dimension)
	: YSplit( opt, dimension ),
	  YGWidget( this, parent )
	{}
	// YSplit
	YGWIDGET_IMPL_MOVE_CHILD
	// YWidget
	YGWIDGET_CONTAINER_IMPL (YSplit)
	virtual GtkFixed *getFixed()
		{ return GTK_FIXED (getWidget()); }
};

// YEmpty

class YGEmpty : public YEmpty, public YGWidget
{
public:
	YGEmpty (const YWidgetOpt &opt, YGWidget *parent)
	: YEmpty( opt ),
	  YGWidget( this, parent )
	{}
	YGWIDGET_CONTAINER_IMPL (YEmpty)
	virtual GtkFixed *getFixed()
		{ return GTK_FIXED (getWidget()); }
};

// YSpacing

class YGSpacing : public YSpacing, public YGWidget
{
public:
	YGSpacing (const YWidgetOpt &opt, YGWidget *parent,
	           float size, bool horizontal, bool vertical)
	: YSpacing (opt, size, horizontal, vertical),
	  YGWidget(this, parent)
	{}
	YGWIDGET_CONTAINER_IMPL (YSpacing)
	virtual GtkFixed *getFixed()
		{ return GTK_FIXED (getWidget()); }
};

// YReplacePoint

class YGReplacePoint : public YReplacePoint, public YGWidget
{
public:
	YGReplacePoint (const YWidgetOpt &opt, YGWidget *parent)
	: YReplacePoint (opt),
	  YGWidget( this, parent ) {}
	// YContainerWidget
	virtual void childAdded( YWidget *child )
	{
		YGWidget::get(child)->show();
	}
	// YWidget
	YGWIDGET_CONTAINER_IMPL (YReplacePoint)
	virtual GtkFixed *getFixed()
		{ return GTK_FIXED (getWidget()); }
};

// YSquash

class YGSquash : public YSquash, public YGWidget
{
public:
	YGSquash (const YWidgetOpt &opt, YGWidget *parent,
	          bool hsquash, bool vsquash)
	: YSquash (opt, hsquash, vsquash),
	  YGWidget( this, parent ) {}
	YGWIDGET_CONTAINER_IMPL (YSquash)
	virtual GtkFixed *getFixed()
		{ return GTK_FIXED (getWidget()); }
};

// YAlignment

class YGAlignment : public YAlignment, public YGWidget
{
	GtkWidget *m_fixed;
	GdkPixbuf *m_background_pixbuf;

public:
	YGAlignment (const YWidgetOpt &opt, YGWidget *parent,
	             YAlignmentType halign, YAlignmentType valign);

	virtual void setBackgroundPixmap (string image);
	virtual GtkFixed *getFixed();

	static gboolean expose_event_cb (GtkWidget *widget, GdkEventExpose *event,
	                                 YGAlignment *pThis);
	// YAlignment
	YGWIDGET_IMPL_MOVE_CHILD
	// YWidget
	YGWIDGET_CONTAINER_IMPL (YAlignment)
};
