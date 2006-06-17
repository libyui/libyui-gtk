#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YEvent.h"
#include "YLabel.h"
#include "YGWidget.h"

#define TO16bits(clr) ((clr * 65536) / 256)
GdkColor fromYColor (const YColor &ycolor)
{
	GdkColor color = { 0, TO16bits (ycolor.red), TO16bits (ycolor.green),
	                      TO16bits (ycolor.blue) };
	return color;
}

class YGLabel : public YLabel, public YGWidget
{
	GtkWidget *m_widget;
	int m_margin;

public:
	YGLabel (const YWidgetOpt &opt, YGWidget *parent, YCPString text,
	         int margin = 0, const YColor *fgColor = 0, const YColor *bgColor = 0)
	: YLabel (opt, text)
	, YGWidget (this, parent, true, GTK_TYPE_EVENT_BOX, NULL)
	{
		if (opt.isOutputField.value()) {
			m_widget = gtk_entry_new();
			gtk_editable_set_editable (GTK_EDITABLE (m_widget), FALSE);
		}
		else
			m_widget = gtk_label_new ("");

		gtk_container_add (GTK_CONTAINER (YGWidget::getWidget()), getWidget());
		// NOTE: we can't just use gtk_container_set_border_width() because the color
		// would not expand to the borders
		m_margin = margin;
		gtk_widget_show (m_widget);

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

		if (fgColor) {
			GdkColor color = fromYColor(*fgColor);
			gtk_widget_modify_fg (getWidget(), GTK_STATE_NORMAL, &color);
		}
		if (bgColor) {
			GdkColor color = fromYColor(*bgColor);
			gtk_widget_modify_bg (YGWidget::getWidget(), GTK_STATE_NORMAL, &color);
		}

		setLabel (text);
	}

	virtual GtkWidget* getWidget()
	{ return m_widget; }

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
	virtual long nicesize (YUIDimension dim)
	{
		IMPL
		return getNiceSize (dim) + m_margin;
	}
};

YWidget *
YGUI::createLabel (YWidget *parent, YWidgetOpt &opt,
                   const YCPString &text)
{
	return new YGLabel (opt, YGWidget::get (parent), text);
}

YWidget *
YGUI::createColoredLabel (YWidget *parent, YWidgetOpt &opt,
                          YCPString label, YColor fgColor,
                          YColor bgColor, int margin)
{
	return new YGLabel (opt, YGWidget::get (parent), label,
	                           margin, &fgColor, &bgColor);
}
