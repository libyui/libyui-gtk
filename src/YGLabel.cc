/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#define YUILogComponent "gtk"
#include "YGUI.h"
#include "YGUtils.h"
#include "YGWidget.h"
#include "YLabel.h"

class YGLabel : public YLabel, public YGWidget
{
public:
	YGLabel (YWidget *parent, const std::string &text, bool heading, bool outputField)
    : YLabel (NULL, text, heading, outputField),
      YGWidget (this, parent, GTK_TYPE_LABEL, NULL)
	{
#		if GTK_CHECK_VERSION (3, 14, 0)
		gtk_widget_set_halign (getWidget(), GTK_ALIGN_START);
		gtk_widget_set_valign (getWidget(), GTK_ALIGN_CENTER);
#		else
		gtk_misc_set_alignment (GTK_MISC (getWidget()), 0.0, 0.5);
#		endif

		if (outputField) {
			gtk_label_set_selectable (GTK_LABEL (getWidget()), TRUE);
			gtk_label_set_single_line_mode (GTK_LABEL (getWidget()), TRUE);
			YGUtils::setWidgetFont (getWidget(), PANGO_STYLE_ITALIC, PANGO_WEIGHT_NORMAL,
				                    PANGO_SCALE_MEDIUM);
		}
		if (heading)
			YGUtils::setWidgetFont (getWidget(), PANGO_STYLE_NORMAL, PANGO_WEIGHT_BOLD,
				                    PANGO_SCALE_LARGE);
		setLabel (text);
	}

	virtual void setText (const std::string &label)
	{
		YLabel::setText (label);
		gtk_label_set_label (GTK_LABEL (getWidget()), label.c_str());
		std::string::size_type i = label.find ('\n', 0);
		if (isOutputField()) {  // must not have a breakline
			if (i != std::string::npos) {
				std::string l (label, 0, i);
				gtk_label_set_label (GTK_LABEL (getWidget()), l.c_str());
			}
		}
		else {
			bool selectable = i != std::string::npos && i != label.size()-1;
			gtk_label_set_selectable (GTK_LABEL (getWidget()), selectable);
		}
	}

	YGWIDGET_IMPL_COMMON (YLabel)
	YGWIDGET_IMPL_USE_BOLD (YLabel)
};

YLabel *YGWidgetFactory::createLabel (YWidget *parent,
	const std::string &text, bool heading, bool outputField)
{ return new YGLabel (parent, text, heading, outputField); }

