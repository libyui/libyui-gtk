#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YGWidget.h"

#define TO16bits(clr) (clr << 8)
GdkColor fromYColor (const YColor &ycolor)
{
	GdkColor color = { 0, TO16bits (ycolor.red), TO16bits (ycolor.green),
	                   TO16bits (ycolor.blue) };
	return color;
}

class YGGenericLabel : public YGWidget
{
protected:
	GtkWidget *m_widget;
	int m_vert_margin, m_hori_margin;

public:
	YGGenericLabel (YWidget *y_widget, YGWidget *parent, const YWidgetOpt &opt,
	                YCPString text, int hor_margin = 0, int ver_margin = 0,
	                const YColor *fgColor = 0, const YColor *bgColor = 0)
	: YGWidget (y_widget, parent, true, GTK_TYPE_EVENT_BOX, NULL)
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
		m_hori_margin = hor_margin;
		m_vert_margin = ver_margin;
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

		doSetLabel (text);
	}

	virtual ~YGGenericLabel() {}

	virtual GtkWidget* getWidget()
	{ return m_widget; }

	void doSetLabel (const YCPString &label)
	{
		if (GTK_IS_LABEL (getWidget()))
			gtk_label_set_label (GTK_LABEL (getWidget()), label->value_cstr());
		else
			gtk_entry_set_text (GTK_ENTRY (getWidget()), label->value_cstr());
	}

	virtual long getNiceSize (YUIDimension dim)
	{
		IMPL
		return YGWidget::getNiceSize (dim) +
		           (dim  == YD_HORIZ ? m_hori_margin : m_vert_margin);
	}
};

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
	YGWIDGET_IMPL_NICESIZE
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
	, YGGenericLabel (this, parent, opt, text, margin, margin, &fgColor, &bgColor)
	{ }

	// YGWidget
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_NICESIZE
	// YGLabeledWidget
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YColoredLabel)
	// YGGenericLabel
	YG_GENERIC_LABEL_IMPL_KEYBOARD_FOCUS
};

YWidget *
YGUI::createColoredLabel (YWidget *parent, YWidgetOpt &opt,
                          YCPString label, YColor fgColor,
                          YColor bgColor, int margin)
{
	return new YGColoredLabel (opt, YGWidget::get (parent), label,
	                                     fgColor, bgColor, margin);
}

#include "YBarGraph.h"

// FIXME: this is broken

class YGBarGraph : public YBarGraph, public YGWidget
{
	vector <YGGenericLabel*> labels;

public:
	YGBarGraph (const YWidgetOpt &opt, YGWidget *parent)
	: YBarGraph (opt)
	, YGWidget (this, parent, true, GTK_TYPE_HBOX, NULL)
	{ }

	// YBarGraph
	virtual void doUpdate()
	{
		for (unsigned int i = 0; i < labels.size(); i++) {
			gtk_container_remove (GTK_CONTAINER (getWidget()), labels[i]->getWidget());
			delete labels[i];
		}
		labels.clear();

		for (int i = 0; i < segments(); i++) {
			YGGenericLabel *glabel;
			// TODO: add colors
			glabel = new YGGenericLabel (this, this, YWidgetOpt(), label(i), 0, value(i));
			labels.push_back (glabel);
			gtk_container_add (GTK_CONTAINER (getWidget()), glabel->getWidget());
		}
	}

	// YWidget
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_NICESIZE
};

YWidget *
YGUI::createBarGraph (YWidget *parent, YWidgetOpt &opt)
{
	return new YGBarGraph (opt, YGWidget::get (parent));
}
