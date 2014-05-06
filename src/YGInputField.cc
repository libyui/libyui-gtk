/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#define YUILogComponent "gtk"
#include <yui/Libyui_config.h>
#include "YGUI.h"
#include "YGWidget.h"
#include "YGUtils.h"
#include "ygtkfieldentry.h"

#include <YInputField.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <sstream>

using namespace boost::gregorian;

class YGInputField : public YInputField, public YGLabeledWidget
{
public:
	YGInputField (YWidget *parent, const std::string &label, bool passwordMode)
	: YInputField (NULL, label, passwordMode),
	  YGLabeledWidget (this, parent, label, YD_HORIZ,
	                   YGTK_TYPE_FIELD_ENTRY, NULL)
	{
		gtk_widget_set_size_request (getWidget(), 0, -1);  // let min size, set width
		YGtkFieldEntry *field = YGTK_FIELD_ENTRY (getWidget());
		ygtk_field_entry_add_field (field, 0);

		GtkEntry *entry = ygtk_field_entry_get_field_widget (field, 0);
		gtk_entry_set_activates_default (entry, TRUE);
		gtk_entry_set_visibility (entry, !passwordMode);

		connect (getWidget(), "field-entry-changed", G_CALLBACK (value_changed_cb), this);
	}

	// YInputField
	virtual std::string value()
	{
		YGtkFieldEntry *field = YGTK_FIELD_ENTRY (getWidget());
		return ygtk_field_entry_get_field_text (field, 0);
	}

	virtual void setValue (const std::string &text)
	{
		BlockEvents block (this);
		YGtkFieldEntry *field = YGTK_FIELD_ENTRY (getWidget());
		ygtk_field_entry_set_field_text (field, 0, text.c_str());
	}

	void updateProps()
	{
		YGtkFieldEntry *field = YGTK_FIELD_ENTRY (getWidget());
		ygtk_field_entry_setup_field (field, 0, inputMaxLength(), validChars().c_str());
	}

	virtual void setInputMaxLength (int len)
	{
		YInputField::setInputMaxLength (len);
		updateProps();
	}

	virtual void setValidChars (const std::string &validChars)
	{
		YInputField::setValidChars (validChars);
		updateProps();
	}

	// callbacks
	static void value_changed_cb (YGtkFieldEntry *entry, gint field_nb, YGInputField *pThis)
	{ pThis->emitEvent (YEvent::ValueChanged); }

	// YGWidget
	virtual bool doSetKeyboardFocus()
    {
		YGtkFieldEntry *field = YGTK_FIELD_ENTRY (getWidget());
        return ygtk_field_entry_set_focus (field);
    }

	virtual unsigned int getMinSize (YUIDimension dim)
	{ return dim == YD_HORIZ ? (shrinkable() ? 30 : 200) : 0; }

	YGLABEL_WIDGET_IMPL (YInputField)
};

YInputField *YGWidgetFactory::createInputField (YWidget *parent, const std::string &label,
                                                bool passwordMode)
{
	return new YGInputField (parent, label, passwordMode);
}

#include "YTimeField.h"

class YGTimeField : public YTimeField, public YGLabeledWidget
{
	std::string old_time;
public:
	YGTimeField (YWidget *parent, const std::string &label)
	: YTimeField (NULL, label),
	  YGLabeledWidget (this, parent, label, YD_HORIZ,
	                   YGTK_TYPE_FIELD_ENTRY, NULL)
	{
		// Same default as Qt
		old_time = "00:00:00";
		YGtkFieldEntry *field = YGTK_FIELD_ENTRY (getWidget());
		ygtk_field_entry_add_field (field, ':');
		ygtk_field_entry_add_field (field, ':');
		ygtk_field_entry_add_field (field, ':');
		ygtk_field_entry_setup_field (field, 0, 2, "0123456789");
		ygtk_field_entry_setup_field (field, 1, 2, "0123456789");
		ygtk_field_entry_setup_field (field, 2, 2, "0123456789");
 
		setValue ( old_time );

		connect (getWidget(), "field-entry-changed", G_CALLBACK (value_changed_cb), this);
	}

	bool validTime(const std::string& input_time)
	{
		tm tm1;
		std::stringstream ss;
		ss << input_time;
		char c; 
          
		if (!(ss >> tm1.tm_hour))
			return false;
		ss >> c;
          
		if (!(ss >> tm1.tm_min))
			return false;
		ss >> c;
          
		if (!(ss >> tm1.tm_sec))
			return false;

		return (tm1.tm_hour<=23 && tm1.tm_min <= 59 && tm1.tm_sec <= 59);
	}

	// YTimeField
	virtual void setValue (const std::string &time)
	{
		BlockEvents block (this);
		if (time.empty()) return;
		if (!validTime(time)) return;
                
		char hours[3], mins[3], secs[3];
		sscanf (time.c_str(), "%2s:%2s:%2s", hours, mins, secs);

		YGtkFieldEntry *entry = YGTK_FIELD_ENTRY (getWidget());
		ygtk_field_entry_set_field_text (entry, 0, hours);
		ygtk_field_entry_set_field_text (entry, 1, mins);
		ygtk_field_entry_set_field_text (entry, 2, secs);
                
		old_time = time;
	}

	virtual std::string value()
	{
		const gchar *hours, *mins, *secs;
		YGtkFieldEntry *entry = YGTK_FIELD_ENTRY (getWidget());
		if (!gtk_entry_get_text_length (ygtk_field_entry_get_field_widget (entry, 0)) ||
		    !gtk_entry_get_text_length (ygtk_field_entry_get_field_widget (entry, 1)) ||
		    !gtk_entry_get_text_length (ygtk_field_entry_get_field_widget (entry, 2)))
		return old_time;
            
		hours = ygtk_field_entry_get_field_text (entry, 0);
		mins  = ygtk_field_entry_get_field_text (entry, 1);
		secs  = ygtk_field_entry_get_field_text (entry, 2);

		gchar *time = g_strdup_printf ("%02d:%02d:%02d", atoi (hours), atoi (mins), atoi (secs));
		std::string str (time);
		g_free (time);
		return str;
	}

	// callbacks
	static void value_changed_cb (YGtkFieldEntry *entry, gint field_nb,
	                              YGTimeField *pThis)
	{ 
		if (!gtk_entry_get_text_length (ygtk_field_entry_get_field_widget (entry, 0)) ||
		    !gtk_entry_get_text_length (ygtk_field_entry_get_field_widget (entry, 1)) ||
		    !gtk_entry_get_text_length (ygtk_field_entry_get_field_widget (entry, 2)))
		return;
                
		if (pThis->validTime(pThis->value()))
		{            
			pThis->old_time = pThis->value();
			pThis->emitEvent (YEvent::ValueChanged); 
		}
		else
		{
			pThis->setValue(pThis->old_time);
		}
	}

	YGLABEL_WIDGET_IMPL (YTimeField)
};

YTimeField *YGOptionalWidgetFactory::createTimeField (YWidget *parent, const std::string &label)
{ return new YGTimeField (parent, label); }

#include "YDateField.h"
#include "ygtkmenubutton.h"

class YGDateField : public YDateField, public YGLabeledWidget
{
GtkWidget *m_calendar, *m_popup_calendar;
std::string old_date;

public:
	YGDateField (YWidget *parent, const std::string &label)
	: YDateField (NULL, label),
	  YGLabeledWidget (this, parent, label, YD_HORIZ, YGTK_TYPE_FIELD_ENTRY, NULL)
	{
		// Same value as QT default
		old_date = "2000-01-01";
		ygtk_field_entry_add_field (getField(), '-');
		ygtk_field_entry_add_field (getField(), '-');
		ygtk_field_entry_add_field (getField(), '-');
		ygtk_field_entry_setup_field (getField(), 0, 4, "0123456789");
		ygtk_field_entry_setup_field (getField(), 1, 2, "0123456789");
		ygtk_field_entry_setup_field (getField(), 2, 2, "0123456789");

		m_calendar = gtk_calendar_new();
		gtk_widget_show (m_calendar);
		GtkWidget *popup = ygtk_popup_window_new (m_calendar);

		GtkWidget *menu_button = ygtk_menu_button_new_with_label ("");
		ygtk_menu_button_set_popup (YGTK_MENU_BUTTON (menu_button), popup);
		gtk_widget_show (menu_button);
		gtk_box_pack_start (GTK_BOX (getWidget()), menu_button, FALSE, TRUE, 6);
                
		setValue(old_date);

		connect (getWidget(), "field-entry-changed", G_CALLBACK (value_changed_cb), this);
		connect (m_calendar, "day-selected", G_CALLBACK (calendar_changed_cb), this);
		g_signal_connect (G_OBJECT (m_calendar), "day-selected-double-click",
		                  G_CALLBACK (double_click_cb), popup);
	}

	inline GtkCalendar *getCalendar()
	{ return GTK_CALENDAR (m_calendar); }
	inline YGtkFieldEntry *getField()
	{ return YGTK_FIELD_ENTRY (getWidget()); }

	bool validDate(const std::string& input_date)
	{
		std::wstringstream ss;
		wdate_input_facet * fac = new wdate_input_facet(L"%Y-%m-%d");
		ss.imbue(std::locale(std::locale::classic(), fac));

		date d;
		ss << input_date.c_str();
		ss >> d;
		return d != date();
	}
        
	// YDateField
	virtual void setValue (const std::string &date)
	{
		BlockEvents block (this);
		if (date.empty()) return;
		if (!validDate(date)) return;

		char year[5], month[3], day[3];
		sscanf (date.c_str(), "%4s-%2s-%2s", year, month, day);

		gtk_calendar_select_month (getCalendar(), atoi (month)-1, atoi (year));
		gtk_calendar_select_day (getCalendar(), atoi (day));

		ygtk_field_entry_set_field_text (getField(), 0, year);
		ygtk_field_entry_set_field_text (getField(), 1, month);
		ygtk_field_entry_set_field_text (getField(), 2, day);
		old_date = date;
	}

	virtual std::string value()
	{
		if (gtk_entry_get_text_length (ygtk_field_entry_get_field_widget (getField(), 0)) < 4 ||
		    !gtk_entry_get_text_length (ygtk_field_entry_get_field_widget (getField(), 1)) ||
		    !gtk_entry_get_text_length (ygtk_field_entry_get_field_widget (getField(), 2)))
			return old_date;

		const gchar *year, *month, *day;
		year  = ygtk_field_entry_get_field_text (getField(), 0);
		month = ygtk_field_entry_get_field_text (getField(), 1);
		day   = ygtk_field_entry_get_field_text (getField(), 2);

		gchar *time = g_strdup_printf ("%04d-%02d-%02d", atoi (year),
		                               atoi (month), atoi (day));
		std::string str (time);
		g_free (time);
		return str;
	}

	// callbacks
	static void value_changed_cb (YGtkFieldEntry *entry, gint field_nb,
	                              YGDateField *pThis)
	{
		if (gtk_entry_get_text_length (ygtk_field_entry_get_field_widget (entry, 0)) < 4 ||
		    !gtk_entry_get_text_length (ygtk_field_entry_get_field_widget (entry, 1)) ||
		    !gtk_entry_get_text_length (ygtk_field_entry_get_field_widget (entry, 2)))
			return;

		std::string new_date = pThis->value();
		bool changed = false;

		if (pThis->validDate(new_date))
		{
			changed = true;
			pThis->old_date = new_date;
		}
		else
		{
			pThis->setValue(pThis->old_date);
		}

		if (changed)
		{               
			int year, month, day;
			year  = atoi (ygtk_field_entry_get_field_text (pThis->getField(), 0));
 			month = atoi (ygtk_field_entry_get_field_text (pThis->getField(), 1));
			day   = atoi (ygtk_field_entry_get_field_text (pThis->getField(), 2));
                
			g_signal_handlers_block_by_func (pThis->getCalendar(),
						(gpointer) calendar_changed_cb, pThis);
			gtk_calendar_select_month (pThis->getCalendar(), month-1, year);
			gtk_calendar_select_day (pThis->getCalendar(), day);
			g_signal_handlers_unblock_by_func (pThis->getCalendar(),
						(gpointer) calendar_changed_cb, pThis);

			pThis->emitEvent (YEvent::ValueChanged);
		}
	}

	static void calendar_changed_cb (GtkCalendar *calendar, YGDateField *pThis)
	{
		guint year, month, day;
		gtk_calendar_get_date (calendar, &year, &month, &day);
		month += 1;  // GTK calendar months go from 0 to 11

		gchar *year_str, *month_str, *day_str;
		year_str = g_strdup_printf  ("%d", year);
		month_str = g_strdup_printf ("%d", month);
		day_str = g_strdup_printf   ("%d", day);

		g_signal_handlers_block_by_func (pThis->getField(),
		                                 (gpointer) value_changed_cb, pThis);

		YGtkFieldEntry *entry = pThis->getField();
		ygtk_field_entry_set_field_text (entry, 0, year_str);
		ygtk_field_entry_set_field_text (entry, 1, month_str);
		ygtk_field_entry_set_field_text (entry, 2, day_str);

		pThis->old_date = pThis->value();
		g_signal_handlers_unblock_by_func (pThis->getField(),
		                                   (gpointer) value_changed_cb, pThis);

		g_free (year_str);
		g_free (month_str);
		g_free (day_str);

		pThis->emitEvent (YEvent::ValueChanged);
	}

	static void double_click_cb (GtkCalendar *calendar, YGtkPopupWindow *popup)
	{
		// close popup
		gtk_widget_hide (GTK_WIDGET (popup));
	}

	YGLABEL_WIDGET_IMPL (YDateField)
};

YDateField *YGOptionalWidgetFactory::createDateField (YWidget *parent, const std::string &label)
{ return new YGDateField (parent, label); }

#include "YTimezoneSelector.h"
#include "ygtktimezonepicker.h"

class YGTimezoneSelector : public YTimezoneSelector, public YGWidget
{
public:
	YGTimezoneSelector (YWidget *parent, const std::string &pixmap,
		const std::map <std::string, std::string> &timezones)
	: YTimezoneSelector (NULL, pixmap, timezones),
	  YGWidget (this, parent, YGTK_TYPE_TIME_ZONE_PICKER, NULL)
	{
		setStretchable (YD_HORIZ, true);
		setStretchable (YD_VERT,  true);
		ygtk_time_zone_picker_set_map (YGTK_TIME_ZONE_PICKER (getWidget()),
			pixmap.c_str(), convert_code_to_name, (gpointer) &timezones);

		connect (getWidget(), "zone-clicked", G_CALLBACK (zone_clicked_cb), this);
	}

	// YTimezoneSelector
	virtual std::string currentZone() const
	{
    	YGTimezoneSelector *pThis = const_cast <YGTimezoneSelector *> (this);
		const gchar *zone = ygtk_time_zone_picker_get_current_zone (
			YGTK_TIME_ZONE_PICKER (pThis->getWidget()));
    	if (zone)
    		return zone;
    	return std::string();
    }

	virtual void setCurrentZone (const std::string &zone, bool zoom)
	{
		BlockEvents block (this);
		ygtk_time_zone_picker_set_current_zone (YGTK_TIME_ZONE_PICKER (getWidget()),
		                                        zone.c_str(), zoom);
	}

	// YGtkTimeZonePicker
	static const gchar *convert_code_to_name (const gchar *code, gpointer pData)
	{
		const std::map <std::string, std::string> *timezones =
			(std::map <std::string, std::string> *) pData;
		std::map <std::string, std::string>::const_iterator name =
			timezones->find (code);
		if (name  == timezones->end())
			return NULL;
		return name->second.c_str();
	}

	// callbacks
	static void zone_clicked_cb (YGtkTimeZonePicker *picker, const gchar *zone,
	                             YGTimezoneSelector *pThis)
	{ pThis->emitEvent (YEvent::ValueChanged); }

	YGWIDGET_IMPL_COMMON (YTimezoneSelector)
};

YTimezoneSelector *YGOptionalWidgetFactory::createTimezoneSelector (YWidget *parent,
	const std::string &pixmap, const std::map <std::string, std::string> &timezones)
{ return new YGTimezoneSelector (parent, pixmap, timezones); }

