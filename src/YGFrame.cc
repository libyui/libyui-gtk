/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include "YGUI.h"
#include "YGWidget.h"
#include "YGUtils.h"

// Instead of traditional looking frames, we use Gnome convention for the
// frame's look. That is: don't draw a frame, use bold header and pad the child.
#define CHILD_INDENTATION 15

class YGBaseFrame : public YGWidget
{
protected:
// a GtkAlignment to set some indentation on the child
GtkWidget *m_containee;

public:
	YGBaseFrame (YWidget *ywidget, YWidget *parent)
	: YGWidget (ywidget, parent, true, GTK_TYPE_FRAME,
	            "shadow-type", GTK_SHADOW_NONE, NULL)
	{
		IMPL
		m_containee = gtk_alignment_new (0, 0, 1, 1);
		gtk_alignment_set_padding (GTK_ALIGNMENT (m_containee), 0, 0, 15, 0);
		gtk_widget_show (m_containee);
		gtk_container_add (GTK_CONTAINER (getWidget()), m_containee);
	}
	virtual ~YGBaseFrame() {}
};

#include "YFrame.h"

class YGFrame : public YFrame, public YGBaseFrame
{
public:
	YGFrame (YWidget *parent, const string &label)
	: YFrame (NULL, label),
	  YGBaseFrame (this, parent)
	{
		GtkWidget *label_widget = gtk_label_new ("");
		YGUtils::setWidgetFont (GTK_WIDGET (label_widget), PANGO_WEIGHT_BOLD,
		                        PANGO_SCALE_MEDIUM);
		gtk_widget_show (label_widget);
		gtk_frame_set_label_widget (GTK_FRAME (getWidget()), label_widget);
		setLabel (label);
	}

	virtual ~YGFrame() {}

	// YFrame
	virtual void setLabel (const string &_str)
	{
		IMPL
		/* Get rid of mnemonics; makes no sense here. */
		size_t length = _str.length();
		string str;
		str.reserve (length);
		for (size_t i = 0; i < length; i++)
			if (_str[i] != '&')
				str += _str[i];

		GtkWidget *label = gtk_frame_get_label_widget (GTK_FRAME (getWidget()));
		gtk_label_set_text (GTK_LABEL (label), str.c_str());

		YFrame::setLabel (_str);
	}

	YGWIDGET_IMPL_COMMON
	YGWIDGET_IMPL_CHILD_ADDED (m_containee)
	YGWIDGET_IMPL_CHILD_REMOVED (getWidget())
};


YFrame *YGWidgetFactory::createFrame (YWidget *parent, const string &label)
{
	IMPL
	return new YGFrame (parent, label);
}

#include "YCheckBoxFrame.h"

class YGCheckBoxFrame : public YCheckBoxFrame, public YGBaseFrame
{
public:
	YGCheckBoxFrame (YWidget *parent, const string &label, bool checked)
	: YCheckBoxFrame (NULL, label, checked),
	  YGBaseFrame (this, parent)
	{
		IMPL
        GtkWidget *button = gtk_check_button_new_with_mnemonic("");
		YGUtils::setWidgetFont (GTK_WIDGET (button), PANGO_WEIGHT_BOLD,
		                        PANGO_SCALE_MEDIUM);
		gtk_widget_show_all (button);
		gtk_frame_set_label_widget (GTK_FRAME (getWidget()), button);

		setLabel (label);
        setValue (checked);
		g_signal_connect (G_OBJECT (button), "toggled",
                          G_CALLBACK (toggled_cb), this);
	}
	virtual ~YGCheckBoxFrame() {}

	// YCheckBoxFrame
	virtual void setLabel (const string &_str)
	{
        GtkWidget *button = gtk_frame_get_label_widget (GTK_FRAME (getWidget()));
        GtkLabel *label = GTK_LABEL (GTK_BIN (button)->child);

		string str (YGUtils::mapKBAccel (_str));
		gtk_label_set_text_with_mnemonic (label, str.c_str());
		YCheckBoxFrame::setLabel (_str);
	}

    bool value()
    {
        GtkWidget *button = gtk_frame_get_label_widget (GTK_FRAME (getWidget()));
        return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    }

    void setValue (bool value)
    {
        GtkWidget *button = gtk_frame_get_label_widget (GTK_FRAME (getWidget()));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), value);
    }

	virtual void setEnabling (bool enabled)
    {
        GtkWidget *frame = getWidget();
        if (enabled) {
            gtk_widget_set_sensitive (frame, TRUE);
            handleChildrenEnablement (value());
        }
        else {
            gtk_widget_set_sensitive (frame, FALSE);
			YWidget::setChildrenEnabled (false);
        }
    }

	YGWIDGET_IMPL_CHILD_ADDED (m_containee)
	YGWIDGET_IMPL_CHILD_REMOVED (getWidget())

private:
	static void toggled_cb (GtkWidget *widget, YGCheckBoxFrame *pThis)
    {
        pThis->setEnabling (true);
        if (pThis->notify())
            YGUI::ui()->sendEvent (new YWidgetEvent (pThis, YEvent::ValueChanged));
    }
};

YCheckBoxFrame *YGWidgetFactory::createCheckBoxFrame (YWidget *parent, const string &label,
                                                      bool checked)
{
	IMPL
	return new YGCheckBoxFrame (parent, label, checked);
}

