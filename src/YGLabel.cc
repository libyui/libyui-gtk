#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YGWidget.h"

GdkColor fromYColor (const YColor &ycolor)
{
	GdkColor color = { 0, ycolor.red << 8, ycolor.green << 8, ycolor.blue << 8 };
	return color;
}

class YGGenericLabel : public YGWidget
{
protected:
	GtkWidget *m_inner_widget;
	int m_border;

public:
	YGGenericLabel (YWidget *y_widget, YGWidget *parent, const YWidgetOpt &opt,
	                YCPString text, int border = 0,
	                const YColor *fgColor = 0, const YColor *bgColor = 0)
	: YGWidget (y_widget, parent, true, GTK_TYPE_EVENT_BOX, NULL)
	{
		if (opt.isOutputField.value()) {
			m_inner_widget = gtk_entry_new();
			gtk_editable_set_editable (GTK_EDITABLE (m_inner_widget), FALSE);
		}
		else
			m_inner_widget = gtk_label_new ("");

		gtk_container_add (GTK_CONTAINER (YGWidget::getWidget()), getWidget());
		// NOTE: we can't just use gtk_container_set_border_width() because the color
		// would not expand to the borders
		m_border = border;
		gtk_widget_show (m_inner_widget);

		PangoFontDescription* font = pango_font_description_new();
		if (opt.boldFont.value())
			pango_font_description_set_weight (font, PANGO_WEIGHT_BOLD);
		/* This isn't a documented attribute, but seems to be used: */
		if (opt.isHeading.value()) {
			pango_font_description_set_weight (font, PANGO_WEIGHT_HEAVY);
			int size = pango_font_description_get_size (getWidget()->style->font_desc);
			pango_font_description_set_size   (font, (int)(size * PANGO_SCALE_XX_LARGE));
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

		doSetLabel (text);
	}

	virtual ~YGGenericLabel() {}

	virtual GtkWidget* getWidget()
	{ return m_inner_widget; }

	void doSetLabel (const YCPString &label)
	{
		if (GTK_IS_LABEL (getWidget()))
			gtk_label_set_label (GTK_LABEL (getWidget()), label->value_cstr());
		else
			gtk_entry_set_text (GTK_ENTRY (getWidget()), label->value_cstr());
	}
};

#define YG_GENERIC_LABEL_IMPL_NICESIZE \
	virtual long nicesize (YUIDimension dim) { \
		return getNiceSize (dim) + 2*m_border; \
	}
#define YG_GENERIC_LABEL_IMPL_KEYBOARD_FOCUS \
	virtual bool setKeyboardFocus() {          \
		if (GTK_IS_ENTRY (getWidget())) {        \
			gtk_widget_grab_focus (GTK_WIDGET(getWidget()));      \
			return gtk_widget_is_focus (GTK_WIDGET(getWidget())); \
			}                                      \
		return false;                            \
	}

#include "YLabel.h"

class YGLabel : public YLabel, public YGGenericLabel
{
public:
	YGLabel (const YWidgetOpt &opt, YGWidget *parent, YCPString text)
	: YLabel (opt, text)
	, YGGenericLabel (this, parent, opt, text)
	{ }

	// YGWidget
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	YG_GENERIC_LABEL_IMPL_NICESIZE
	// YGLabeledWidget
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YLabel)
	// YGGenericLabel
	YG_GENERIC_LABEL_IMPL_KEYBOARD_FOCUS
};

YWidget *
YGUI::createLabel (YWidget *parent, YWidgetOpt &opt,
                   const YCPString &text)
{
	return new YGLabel (opt, YGWidget::get (parent), text);
}

#include "YColoredLabel.h"

class YGColoredLabel : public YColoredLabel, public YGGenericLabel
{
public:
	YGColoredLabel (const YWidgetOpt &opt, YGWidget *parent, YCPString text,
	                const YColor &fgColor, const YColor &bgColor, int margin)
	: YColoredLabel (opt, text)
	, YGGenericLabel (this, parent, opt, text, margin, &fgColor, &bgColor)
	{ }

	// YGWidget
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	YG_GENERIC_LABEL_IMPL_NICESIZE
	// YGLabeledWidget
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YColoredLabel)
};

YWidget *
YGUI::createColoredLabel (YWidget *parent, YWidgetOpt &opt,
                          YCPString label, YColor fgColor,
                          YColor bgColor, int margin)
{
	return new YGColoredLabel (opt, YGWidget::get (parent), label,
	                           fgColor, bgColor, margin);
}
