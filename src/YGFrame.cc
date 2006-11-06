//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "YGUtils.h"
#include "YFrame.h"

#define CHILDREN_IDENTATION 15

// Instead of tradicional looking frames, we use Gnome convention
// for the frame's look. That is, don't draw a frame, use bold
// header and pad children.

class YGFrame : public YFrame, public YGWidget
{
	GtkRequisition m_label_req;

public:
	YGFrame (const YWidgetOpt &opt,
		 YGWidget         *parent,
		 const YCPString & label) :
		YFrame (opt, label),
		YGWidget (this, parent, true,
		          GTK_TYPE_FRAME, "shadow-type", GTK_SHADOW_NONE, NULL)
	{
		IMPL;
		m_label_req.width = m_label_req.height = 0;

//		gtk_widget_show_all (getWidget());
		setLabel (label);
	}

	virtual ~YGFrame() {}

	// YFrame
	virtual void setLabel (const YCPString &label)
	{
		string str = "<b>" + label->value() + "</b>";
		// elminate '&' that some YCP applications seem to set and have
		// no use for a GtkFrame -- they actually interfer with the markup
		string::size_type i = 0;
		while ((i = str.find ("&", i)) != string::npos)
			str.erase (i, 1);

		gtk_frame_set_label (GTK_FRAME (getWidget()), str.c_str());
		YFrame::setLabel (label);

		GtkWidget *frame_label = gtk_frame_get_label_widget (GTK_FRAME (getWidget()));
		gtk_label_set_use_markup (GTK_LABEL (frame_label), TRUE);
	}

	YGWIDGET_IMPL_COMMON
	virtual void childAdded (YWidget *ychild)
	{  // add children with some identation
		YGWidget *ygchild = YGWidget::get (ychild);
		ygchild->setPadding (0, 0, 15, 0);
		ygchild->setBorder (0);

		GtkWidget *child = ygchild->getLayout();
		gtk_container_add (GTK_CONTAINER (getWidget()), child);
	}
	YGWIDGET_IMPL_CHILD_REMOVED (getWidget())
};

YContainerWidget *
YGUI::createFrame (YWidget *parent, YWidgetOpt &opt,
                   const YCPString &label)
{
	IMPL;
	return new YGFrame (opt, YGWidget::get (parent), label);
}
