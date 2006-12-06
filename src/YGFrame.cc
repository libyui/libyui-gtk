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
	{
		// install children on a GtkAlignment, so we can set some identation
		GtkWidget *alignment = gtk_alignment_new (0, 0, 1, 1);
		gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 15, 0);
		gtk_widget_show (alignment);

		GtkWidget *child = YGWidget::get (ychild)->getLayout();
		gtk_container_add (GTK_CONTAINER (alignment), child);
		gtk_container_add (GTK_CONTAINER (getWidget()), alignment);
		stretch_safe = true;
		sync_stretchable();
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
