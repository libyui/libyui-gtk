/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#define YUILogComponent "gtk"
#include "YGUI.h"
#include "YGUtils.h"
#include "YGWidget.h"
#include "YLabel.h"

#define AUTO_WRAP_WIDTH         100
#define AUTO_WRAP_HEIGHT        20

class YGLabel : public YLabel, public YGWidget
{
public:
	YGLabel (YWidget *parent, const std::string &text, bool heading, bool outputField)
    : YLabel (NULL, text, heading, outputField),
      YGWidget (this, parent, GTK_TYPE_LABEL, NULL), _layoutPass1Width(0), _layoutPass1Height(0)
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

 		if (!isOutputField()) {
 			bool selectable = i != std::string::npos && i != label.size()-1;
 			gtk_label_set_selectable (GTK_LABEL (getWidget()), selectable);
 		}

	}

  void setAutoWrap( bool autoWrap )
  {
      yuiDebug() <<  endl;
      YLabel::setAutoWrap( autoWrap );
      gtk_label_set_line_wrap (GTK_LABEL (getWidget()), gboolean(autoWrap));
      gtk_label_set_single_line_mode (GTK_LABEL (getWidget()), gboolean(!autoWrap));
  }

	//YGWIDGET_IMPL_COMMON (YLabel)
    virtual bool setKeyboardFocus() {
        return doSetKeyboardFocus();
    }

    virtual void setEnabled (bool enabled)
    {   YLabel::setEnabled (enabled);
        doSetEnabled (enabled);
    }

    virtual int preferredWidth()
    {

       // return 100;
      int width;

      if ( autoWrap() )
      {
          int lpass = layoutPass();
          //yuiDebug() << "autowrap " << autoWrap() << " layoutpass " << lpass << endl;
          if ( lpass != 1 )
          {
            width = _layoutPass1Width + 2; // some pixel more for border
          }
          else
          {
              width = doPreferredSize (YD_HORIZ);
          }
      }
      else  // ! autoWrap()
        width = doPreferredSize (YD_HORIZ);


      return (width > AUTO_WRAP_WIDTH ? width : AUTO_WRAP_WIDTH);
    }

    virtual int preferredHeight()
    {
      int height;

      if ( autoWrap() )
      {
          if ( layoutPass() != 1 )
          {
              height = _layoutPass1Height + 4; // some pixel more for border
          }
          else
          {
              height = doPreferredSize (YD_VERT);
          }
      }
      else  // ! autoWrap()
      {
          height = doPreferredSize (YD_VERT);
      }

      return (height > AUTO_WRAP_HEIGHT ? height : AUTO_WRAP_HEIGHT);

    }

    virtual void setSize (int width, int height)
    {
      if ( autoWrap() )
      {
          int lpass = layoutPass();
          //yuiDebug() << "layoutpass " << lpass << endl;
          if (lpass == 1 || _layoutPass1Width <= 0)
          {
            _layoutPass1Width = width;
            gint minimum_height, natural_height;

            gtk_widget_get_preferred_height_for_width
                               (getWidget(),
                                _layoutPass1Width,
                                &minimum_height,
                                &natural_height);
            _layoutPass1Height = natural_height;
          }
      }
      doSetSize (width, height);
    }

	YGWIDGET_IMPL_USE_BOLD (YLabel)
protected:
    int _layoutPass1Width;
    int _layoutPass1Height;
};

YLabel *YGWidgetFactory::createLabel (YWidget *parent,
	const std::string &text, bool heading, bool outputField)
{ return new YGLabel (parent, text, heading, outputField); }

