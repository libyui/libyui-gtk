#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YEvent.h"
#include "YLabel.h"
#include "YGWidget.h"

class YGLabel : public YLabel, public YGWidget
{
public:
	YGLabel (const YWidgetOpt &opt, YGWidget *parent, YCPString text)
	: YLabel (opt, text)
	, YGWidget (this, parent, true,
	            opt.isOutputField.value() ? GTK_TYPE_ENTRY : GTK_TYPE_LABEL,
	            NULL)
	{
		if (opt.isOutputField.value())
			gtk_editable_set_editable (GTK_EDITABLE (getWidget()), FALSE);

		PangoFontDescription* font = pango_font_description_new();
		if (opt.boldFont.value())
			pango_font_description_set_weight (font, PANGO_WEIGHT_BOLD);
		/* This isn't a documented attribute, but seems to be used: */
		if (opt.isHeading.value()) {
			pango_font_description_set_weight (font, PANGO_WEIGHT_HEAVY);
			pango_font_description_set_size   (font, 16*PANGO_SCALE);
		}
		gtk_widget_modify_font (getWidget(), font);
		pango_font_description_free (font);

		setLabel (text);
	}

	// YLabel
	virtual void setLabel (const YCPString & label)
	{
		if (GTK_IS_LABEL (getWidget()))
			gtk_label_set_label (GTK_LABEL (getWidget()), label->value_cstr());
		else
			gtk_entry_set_text (GTK_ENTRY (getWidget()), label->value_cstr());
		YLabel::setLabel (label);
	}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	virtual bool setKeyboardFocus()
	{
		if (GTK_IS_ENTRY (getWidget())) {
			gtk_widget_grab_focus (GTK_WIDGET(getWidget()));
			return gtk_widget_is_focus (GTK_WIDGET(getWidget()));
			}
		return false;
	}
};

YWidget *
YGUI::createLabel (YWidget *parent, YWidgetOpt & opt,
                   const YCPString & text)
{
	return new YGLabel (opt, YGWidget::get (parent), text);
}
