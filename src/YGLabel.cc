//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YGWidget.h"

class YGGenericLabel : public YGWidget
{
public:
	YGGenericLabel (YWidget *y_widget, YGWidget *parent, const YWidgetOpt &opt,
	                YCPString text, const YColor *fgColor = 0, const YColor *bgColor = 0,
	                int margin = 0)
	: YGWidget (y_widget, parent, true, opt.isOutputField.value() ? GTK_TYPE_ENTRY
	                                    : GTK_TYPE_LABEL, NULL)
	{
		IMPL
		if (opt.isOutputField.value()) {
			gtk_editable_set_editable (GTK_EDITABLE (getWidget()), FALSE);
			// not editable GtkEntrys don't really look like it, so...
			gtk_widget_modify_base (getWidget(), GTK_STATE_NORMAL,
			                        &getWidget()->style->base [GTK_STATE_INSENSITIVE]);
		}
		else {
			gtk_misc_set_alignment (GTK_MISC (getWidget()), 0.0, 0.5);
			gtk_misc_set_padding (GTK_MISC (getWidget()), margin, margin);
		}

		if (opt.boldFont.value())
			YGUtils::setWidgetFont (getWidget(), PANGO_WEIGHT_BOLD, PANGO_SCALE_MEDIUM);
		if (opt.isHeading.value())
			YGUtils::setWidgetFont (getWidget(), PANGO_WEIGHT_ULTRABOLD, PANGO_SCALE_XX_LARGE);
		doSetLabel (text);

		if (fgColor)
			setForegroundColor (*fgColor);
		if (bgColor) {
			setBackgroundColor (*bgColor);
			g_signal_connect (G_OBJECT (getWidget()), "expose-event",
			                  G_CALLBACK (expose_event), NULL);
		}
	}

	void doSetLabel (const YCPString &label)
	{
		if (GTK_IS_LABEL (getWidget()))
			gtk_label_set_label (GTK_LABEL (getWidget()), label->value_cstr());
		else
			gtk_entry_set_text (GTK_ENTRY (getWidget()), label->value_cstr());
	}

	static GdkColor fromYColor (const YColor &ycolor)
	{
		GdkColor color = { 0, ycolor.red << 8, ycolor.green << 8, ycolor.blue << 8 };
		return color;
	}

	void setForegroundColor (const YColor &ycolor)
	{
		GdkColor color = fromYColor (ycolor);
		gtk_widget_modify_fg (getWidget(), GTK_STATE_NORMAL, &color);
	}

	void setBackgroundColor (const YColor &ycolor)
	{
		GdkColor color = fromYColor (ycolor);
		gtk_widget_modify_bg (getWidget(), GTK_STATE_NORMAL, &color);
	}

	// YWidget
	bool doSetKeyboardFocus()
	{
		if (GTK_IS_ENTRY (getWidget())) {
			gtk_widget_grab_focus (GTK_WIDGET(getWidget()));
			return gtk_widget_is_focus (GTK_WIDGET(getWidget()));
			}
		else  // GtkLabel
			return false;
	}

	// callbacks
	static gboolean expose_event (GtkWidget *widget, GdkEventExpose *event)
	{
		GtkStyle *style = gtk_widget_get_style (widget);
		GtkAllocation *alloc = &widget->allocation;
		gtk_paint_box (style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_OUT, &event->area,
		               widget, NULL, alloc->x, alloc->y, alloc->width, alloc->height);
		return FALSE;
	}
};

#define YG_GENERIC_LABEL_IMPL_KEYBOARD_FOCUS   \
		virtual bool setKeyboardFocus() {          \
			return doSetKeyboardFocus();             \
		}

#include "YLabel.h"

class YGLabel : public YLabel, public YGGenericLabel
{
public:
	YGLabel (const YWidgetOpt &opt, YGWidget *parent, YCPString text)
	: YLabel (opt, text), YGGenericLabel (this, parent, opt, text)
	{ }

	// YGWidget
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_ENABLING
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
	, YGGenericLabel (this, parent, opt, text, &fgColor, &bgColor, margin)
	{ }

	// YGWidget
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_ENABLING
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
