//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YGWidget.h"

#include "YLabel.h"

class YGLabel : public YLabel, public YGWidget
{
public:
	YGLabel (const YWidgetOpt &opt, YGWidget *parent, YCPString text)
	: YLabel (opt, text),
	  YGWidget (this, parent, true, opt.isOutputField.value() ? GTK_TYPE_ENTRY
	                                : GTK_TYPE_LABEL, NULL)
	{
		if (opt.isOutputField.value()) {
			gtk_editable_set_editable (GTK_EDITABLE (getWidget()), FALSE);
			// not editable GtkEntrys don't really look like it, so...
			gtk_widget_modify_base (getWidget(), GTK_STATE_NORMAL,
			                        &getWidget()->style->base [GTK_STATE_INSENSITIVE]);
		}

		if (opt.boldFont.value())
			YGUtils::setWidgetFont (getWidget(), PANGO_WEIGHT_BOLD, PANGO_SCALE_MEDIUM);
		if (opt.isHeading.value())
			YGUtils::setWidgetFont (getWidget(), PANGO_WEIGHT_ULTRABOLD,
			                        PANGO_SCALE_XX_LARGE);

		setLabel (text);
	}

	// YGWidget
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
//	YGWIDGET_IMPL_NICESIZE

virtual long nicesize (YUIDimension dim)
{
long size = getNiceSize (dim);
printf ("%s nice size for label %s: %ld\n",
	dim == YD_HORIZ ? "horizontal" : "vertical", debugLabel().c_str(), size);
return size;
}


	virtual bool setKeyboardFocus()
	{
		if (GTK_IS_ENTRY (getWidget())) {
			gtk_widget_grab_focus (GTK_WIDGET(getWidget()));
			return gtk_widget_is_focus (GTK_WIDGET(getWidget()));
			}
		else  // GtkLabel
			return false;
	}

	virtual void setLabel (const YCPString &label)
	{
		if (GTK_IS_ENTRY (getWidget()))
			gtk_entry_set_text (GTK_ENTRY (getWidget()), label->value_cstr());
		else  // GtkLabel
			gtk_label_set_label (GTK_LABEL (getWidget()), label->value_cstr());

		YLabel::setLabel (label);
	}
};

YWidget *
YGUI::createLabel (YWidget *parent, YWidgetOpt &opt,
                   const YCPString &text)
{
	return new YGLabel (opt, YGWidget::get (parent), text);
}

#include "YColoredLabel.h"

class YGColoredLabel : public YColoredLabel, public YGWidget
{
	GtkWidget *m_label;
	int m_border;

public:
	YGColoredLabel (const YWidgetOpt &opt, YGWidget *parent, YCPString text,
	                const YColor &fgColor, const YColor &bgColor, int margin)
	: YColoredLabel (opt, text),
	  YGWidget (this, parent, true, GTK_TYPE_EVENT_BOX, NULL)
	{
		IMPL
		m_label = gtk_label_new (NULL);
		gtk_widget_show (m_label);
		gtk_container_add (GTK_CONTAINER (getWidget()), m_label);

		// NOTE: we can't just use gtk_container_set_border_width() because the color
		// would not expand to the borders
		m_border = margin;

		if (opt.boldFont.value())
			YGUtils::setWidgetFont (m_label, PANGO_WEIGHT_BOLD, PANGO_SCALE_MEDIUM);
		if (opt.isHeading.value())
			YGUtils::setWidgetFont (m_label, PANGO_WEIGHT_ULTRABOLD, PANGO_SCALE_XX_LARGE);

		setForegroundColor (fgColor);
		setBackgroundColor (bgColor);

		setLabel (text);
	}

	virtual ~YGColoredLabel() {}

	static GdkColor fromYColor (const YColor &ycolor)
	{
		GdkColor color = { 0, ycolor.red << 8, ycolor.green << 8, ycolor.blue << 8 };
		return color;
	}

	void setForegroundColor (const YColor &ycolor)
	{
		GdkColor gcolor = fromYColor (ycolor);
		gtk_widget_modify_fg (m_label, GTK_STATE_NORMAL, &gcolor);
	}

	void setBackgroundColor (const YColor &ycolor)
	{
		GdkColor gcolor = fromYColor (ycolor);
		gtk_widget_modify_bg (getWidget(), GTK_STATE_NORMAL, &gcolor);
	}


	// YGWidget
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING

	virtual long nicesize (YUIDimension dim)
	{ return getNiceSize (dim) + (m_border * 2); }

	virtual void setLabel (const YCPString &label)
	{ gtk_label_set_label (GTK_LABEL (m_label), label->value_cstr()); }
};

YWidget *
YGUI::createColoredLabel (YWidget *parent, YWidgetOpt &opt,
                          YCPString label, YColor fgColor,
                          YColor bgColor, int margin)
{
	return new YGColoredLabel (opt, YGWidget::get (parent), label,
	                           fgColor, bgColor, margin);
}
