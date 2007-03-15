//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "YGUtils.h"
#include "YFrame.h"

#define CHILDREN_IDENTATION 15

// Instead of traditional looking frames, we use Gnome convention
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
        str = YGUtils::mapKBAccel(str.c_str());
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

#if YAST2_VERSION > 2014004 || YAST2_VERSION > 2013032
#include "YCheckBoxFrame.h"

class YGCheckBoxFrame : public YCheckBoxFrame, public YGWidget
{
	GtkRequisition m_label_req;
    GtkLabel      *m_label;

	static void toggled_cb (GtkWidget *widget, YGCheckBoxFrame *pThis)
    {
        pThis->setEnabling (true);
        if (pThis->getNotify())
            YGUI::ui()->sendEvent( new YWidgetEvent( pThis, YEvent::ValueChanged ) );
    }

public:
	YGCheckBoxFrame (const YWidgetOpt &opt,
                     YGWidget         *parent,
                     const YCPString  &label,
                     bool              checked) :
		YCheckBoxFrame (opt, label),
		YGWidget (this, parent, true,
		          GTK_TYPE_FRAME, "shadow-type", GTK_SHADOW_NONE, NULL)
	{
		IMPL;
		m_label_req.width = m_label_req.height = 0;
        GtkWidget *button = gtk_check_button_new_with_label("");
		g_signal_connect (G_OBJECT (button), "toggled",
                          G_CALLBACK (toggled_cb), this);

        m_label = GTK_LABEL(GTK_BIN(button)->child);
        gtk_frame_set_label_widget (GTK_FRAME (getWidget()), button);
		gtk_widget_show_all (button);
		setLabel (label);
        setValue( checked );
	}

	virtual ~YGCheckBoxFrame() {}

	// YFrame
	virtual void setLabel (const YCPString &label)
	{
		string str = "<b>" + label->value() + "</b>";
        str = YGUtils::mapKBAccel(str.c_str());
        gtk_label_set_text_with_mnemonic (m_label, str.c_str());
		gtk_label_set_use_markup (GTK_LABEL (m_label), TRUE);
		YCheckBoxFrame::setLabel (label);
	}

    bool getValue()
    {
        GtkWidget *button = gtk_frame_get_label_widget (GTK_FRAME (getWidget()));
        return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    }

    void setValue (bool newValue)
    {
        GtkWidget *button = gtk_frame_get_label_widget (GTK_FRAME (getWidget()));
        return gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), newValue);
    }

	virtual void setEnabling (bool enabled)
    {
        GtkWidget *frame = getWidget();
        gtk_widget_set_sensitive (frame, TRUE);
        handleChildrenEnablement (getValue());
    }

	virtual void childAdded (YWidget *ychild)
	{
		// install children on a GtkAlignment, so we can set some identation
		GtkWidget *alignment = gtk_alignment_new (0, 0, 1, 1);
		gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 15, 0);
		gtk_widget_show (alignment);

		GtkWidget *child = YGWidget::get (ychild)->getLayout();
		gtk_container_add (GTK_CONTAINER (alignment), child);
		gtk_container_add (GTK_CONTAINER (getWidget()), alignment);
		sync_stretchable();
	}
	YGWIDGET_IMPL_CHILD_REMOVED (getWidget())
};

YContainerWidget *
YGUI::createCheckBoxFrame (YWidget *parent, YWidgetOpt &opt,
                           const YCPString &label, bool checked)
{
	IMPL;
	return new YGCheckBoxFrame (opt, YGWidget::get (parent), label, checked);
}

#endif /*YAST2_VERSION*/
