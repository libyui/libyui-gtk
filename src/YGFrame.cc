#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "YFrame.h"

class YGFrame : public YFrame, public YGWidget
{
	GtkRequisition m_label_req;
public:
	YGFrame (const YWidgetOpt &opt,
		 YGWidget         *parent,
		 const YCPString & label) :
		YFrame (opt, label),
		YGWidget (this, parent, true,
			  GTK_TYPE_FRAME, NULL)
	{
		IMPL;
		m_label_req.width = m_label_req.height = 0;
		gtk_container_add (GTK_CONTAINER (getWidget()), gtk_fixed_new());
		gtk_widget_show_all (getWidget());
		setLabel (label);
	}

	virtual ~YGFrame() {}

	// YFrame
	virtual void setLabel (const YCPString &label)
	{
		gtk_frame_set_label (GTK_FRAME (getWidget()), label->value_cstr());
		YFrame::setLabel (label);
	}

	// YGWidget
	GtkFixed *getFixed()
	{
		return GTK_FIXED (gtk_bin_get_child (GTK_BIN (getWidget())));
	}

	// YWidget
	YGWIDGET_IMPL_SET_ENABLING

	virtual void setSize (long newWidth, long newHeight)
	{
		doSetSize (newWidth, newHeight);

		long newChildWidth  = max (0L, newWidth);
		long newChildHeight = max (0L, newHeight - m_label_req.height);

		if (numChildren() > 0) {
			int border = GTK_CONTAINER (getWidget())->border_width;
			YContainerWidget::child(0)->setSize (newChildWidth - 2*xthickness() - border,
			                                     newChildHeight - 2*ythickness() - border);
		}
	}

	virtual long nicesize (YUIDimension dim)
	{
		IMPL

		long niceSize = numChildren() > 0 ? YContainerWidget::child(0)->nicesize( dim ) : 0;
		GtkFrame *frame = GTK_FRAME (getWidget());
  
		if (frame->label_widget && GTK_WIDGET_VISIBLE (frame->label_widget)) {
			gtk_widget_size_request (frame->label_widget, &m_label_req);
			m_label_req.width += 6;
			m_label_req.height = MAX (0, m_label_req.height -
			                             GTK_WIDGET (frame)->style->ythickness);
			if (dim == YD_HORIZ)
				niceSize = MAX (niceSize, m_label_req.width);
			else
				niceSize += m_label_req.height;
		}

		niceSize += GTK_CONTAINER (frame)->border_width;
		niceSize += thickness (dim) * 2;
			  
		return niceSize;
	}
};

YContainerWidget *
YGUI::createFrame (YWidget *parent, YWidgetOpt &opt,
                   const YCPString &label)
{
	IMPL;
	return new YGFrame (opt, YGWidget::get (parent), label);
}
