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
		IMPL
		if (opt.isOutputField.value()) {
			gtk_editable_set_editable (GTK_EDITABLE (getWidget()), FALSE);
			// not editable GtkEntrys don't really look like it, so...
			gtk_widget_modify_base (getWidget(), GTK_STATE_NORMAL,
			                        &getWidget()->style->base [GTK_STATE_INSENSITIVE]);
		}
		else
			gtk_misc_set_alignment (GTK_MISC (getWidget()), 0.0, 0.5);

		if (opt.boldFont.value())
			YGUtils::setWidgetFont (getWidget(), PANGO_WEIGHT_BOLD, PANGO_SCALE_MEDIUM);
		if (opt.isHeading.value())
			YGUtils::setWidgetFont (getWidget(), PANGO_WEIGHT_ULTRABOLD, PANGO_SCALE_XX_LARGE);
		setLabel (text);
	}

	virtual void setLabel (const YCPString &label)
	{
		if (GTK_IS_LABEL (getWidget()))
			gtk_label_set_label (GTK_LABEL (getWidget()), label->value_cstr());
		else
			gtk_entry_set_text (GTK_ENTRY (getWidget()), label->value_cstr());
		YLabel::setLabel (label);

		// Some YCP progs have crap like no labeled labels cluttering the layout
		label->value().empty() ? gtk_widget_hide (getWidget())
		                       : gtk_widget_show (getWidget());
	}

	// YWidget
	virtual bool doSetKeyboardFocus()
	{
		if (GTK_IS_ENTRY (getWidget())) {
			gtk_widget_grab_focus (GTK_WIDGET(getWidget()));
			return gtk_widget_is_focus (GTK_WIDGET(getWidget()));
			}
		else  // GtkLabel
			return false;
	}

	YGWIDGET_IMPL_COMMON
};

YWidget *
YGUI::createLabel (YWidget *parent, YWidgetOpt &opt,
                   const YCPString &text)
{
	return new YGLabel (opt, YGWidget::get (parent), text);
}

#include "YColoredLabel.h"
#include "ygtkbargraph.h"

class YGColoredLabel : public YColoredLabel, public YGWidget
{
public:
	YGColoredLabel (const YWidgetOpt &opt, YGWidget *parent, YCPString text,
	                const YColor &fgColor, const YColor &bgColor, int margin)
	: YColoredLabel (opt, text),
	  YGWidget (this, parent, true, YGTK_TYPE_COLORED_LABEL, NULL)
	{
		IMPL
		gtk_misc_set_alignment (GTK_MISC (getWidget()), 0.0, 0.5);
		gtk_misc_set_padding (GTK_MISC (getWidget()), margin, margin);

		if (opt.boldFont.value())
			YGUtils::setWidgetFont (getWidget(), PANGO_WEIGHT_BOLD, PANGO_SCALE_MEDIUM);
		if (opt.isHeading.value())
			YGUtils::setWidgetFont (getWidget(), PANGO_WEIGHT_ULTRABOLD, PANGO_SCALE_XX_LARGE);
		setLabel (text);

		YGtkColoredLabel *color_label = YGTK_COLORED_LABEL (getWidget());
		ygtk_colored_label_set_shadow_type (color_label, GTK_SHADOW_OUT);
		ygtk_colored_label_set_foreground (color_label, fgColor.red, fgColor.green, fgColor.blue);
		ygtk_colored_label_set_background (color_label, bgColor.red, bgColor.green, bgColor.blue);
	}

	virtual void setLabel (const YCPString &label)
	{
		gtk_label_set_label (GTK_LABEL (getWidget()), label->value_cstr());
		YColoredLabel::setLabel (label);
	}

	YGWIDGET_IMPL_COMMON
};

YWidget *
YGUI::createColoredLabel (YWidget *parent, YWidgetOpt &opt, YCPString label,
                          YColor fgColor, YColor bgColor, int margin)
{
	return new YGColoredLabel (opt, YGWidget::get (parent), label,
	                             fgColor, bgColor, margin);
}
