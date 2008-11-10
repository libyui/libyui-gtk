/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "YGUtils.h"
#include "ygtkfieldentry.h"

#include "YInputField.h"

class YGInputField : public YInputField, public YGLabeledWidget
{
public:
	YGInputField (YWidget *parent, const string &label, bool passwordMode)
	: YInputField (NULL, label, passwordMode),
	  YGLabeledWidget (this, parent, label, YD_HORIZ,
	                   YGTK_TYPE_FIELD_ENTRY, NULL)
	{
		gtk_widget_set_size_request (getWidget(), 0, -1);  // let min size, set width
		YGtkFieldEntry *field = YGTK_FIELD_ENTRY (getWidget());
		ygtk_field_entry_add_field (field, 0);

		GtkEntry *entry = ygtk_field_entry_get_field_widget (field, 0);
		gtk_entry_set_activates_default (entry, TRUE);
		if (passwordMode)
			gtk_entry_set_visibility (entry, FALSE);

		connect (getWidget(), "field-entry-changed",
		         G_CALLBACK (value_changed_cb), this);
	}

	// YInputField
	virtual string value()
	{
		YGtkFieldEntry *field = YGTK_FIELD_ENTRY (getWidget());
		return ygtk_field_entry_get_field_text (field, 0);
	}

	virtual void setValue (const string &text)
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

	virtual void setValidChars (const string &validChars)
	{
		YInputField::setValidChars (validChars);
		updateProps();
	}

	static void value_changed_cb (YGtkFieldEntry *entry, gint field_nb, YGInputField *pThis)
	{ pThis->emitEvent (YEvent::ValueChanged); }

	virtual bool doSetKeyboardFocus()
    {
		YGtkFieldEntry *field = YGTK_FIELD_ENTRY (getWidget());
        return ygtk_field_entry_set_focus (field);
    }

	YGWIDGET_IMPL_COMMON
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YInputField)

	virtual unsigned int getMinSize (YUIDimension dim)
	{
		return dim == YD_HORIZ ? (shrinkable() ? 30 : 200) : 0;
	}
};

YInputField *YGWidgetFactory::createInputField (YWidget *parent, const string &label,
                                                bool passwordMode)
{
	return new YGInputField (parent, label, passwordMode);
}

#include "YTimeField.h"

class YGTimeField : public YTimeField, public YGLabeledWidget
{
public:
	YGTimeField (YWidget *parent, const string &label)
	: YTimeField (NULL, label),
	  YGLabeledWidget (this, parent, label, YD_HORIZ,
	                   YGTK_TYPE_FIELD_ENTRY, NULL)
	{
		IMPL
		YGtkFieldEntry *field = YGTK_FIELD_ENTRY (getWidget());
		ygtk_field_entry_add_field (field, ':');
		ygtk_field_entry_add_field (field, ':');
		ygtk_field_entry_setup_field (field, 0, 2, "0123456789");
		ygtk_field_entry_setup_field (field, 1, 2, "0123456789");

		connect (getWidget(), "field-entry-changed",
		         G_CALLBACK (value_changed_cb), this);
	}

	// YTimeField
	virtual void setValue (const string &time)
	{
		BlockEvents block (this);
		if (time.empty()) return;
		char hours[3], mins[3];
		sscanf (time.c_str(), "%2s:%2s", hours, mins);

		YGtkFieldEntry *entry = YGTK_FIELD_ENTRY (getWidget());
		ygtk_field_entry_set_field_text (entry, 0, hours);
		ygtk_field_entry_set_field_text (entry, 1, mins);
	}

	virtual string value()
	{
		IMPL
		const gchar *hours, *mins;
		YGtkFieldEntry *entry = YGTK_FIELD_ENTRY (getWidget());
		hours = ygtk_field_entry_get_field_text (entry, 0);
		mins  = ygtk_field_entry_get_field_text (entry, 1);

		gchar *time = g_strdup_printf ("%02d:%02d:00", atoi (hours), atoi (mins));
		string str (time);
		g_free (time);
		return str;
	}

	// callbacks
	static void value_changed_cb (YGtkFieldEntry *entry, gint field_nb,
	                              YGTimeField *pThis)
	{ pThis->emitEvent (YEvent::ValueChanged); }

	YGWIDGET_IMPL_COMMON
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YTimeField)
};

YTimeField *YGOptionalWidgetFactory::createTimeField (YWidget *parent, const string &label)
{
	IMPL
	return new YGTimeField (parent, label);
}

#include "YDateField.h"
#include "ygtkmenubutton.h"

class YGDateField : public YDateField, public YGLabeledWidget
{
GtkWidget *m_calendar, *m_popup_calendar;

public:
	YGDateField (YWidget *parent, const string &label)
	: YDateField (NULL, label),
	  YGLabeledWidget (this, parent, label, YD_HORIZ, YGTK_TYPE_FIELD_ENTRY, NULL)
	{
		IMPL
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

		connect (getWidget(), "field-entry-changed",
		         G_CALLBACK (value_changed_cb), this);
		connect (m_calendar, "day-selected",
		         G_CALLBACK (calendar_changed_cb), this);
		connect (m_calendar, "day-selected-double-click",
		         G_CALLBACK (double_click_cb), popup);
	}

	inline GtkCalendar *getCalendar()
	{ return GTK_CALENDAR (m_calendar); }
	inline YGtkFieldEntry *getField()
	{ return YGTK_FIELD_ENTRY (getWidget()); }

	// YDateField
	virtual void setValue (const string &date)
	{
		BlockEvents block (this);
		if (date.empty()) return;
		char year[5], month[3], day[3];
		sscanf (date.c_str(), "%4s-%2s-%2s", year, month, day);

		gtk_calendar_select_month (getCalendar(), atoi (month)-1, atoi (year));
		gtk_calendar_select_day (getCalendar(), atoi (day));

		ygtk_field_entry_set_field_text (getField(), 0, year);
		ygtk_field_entry_set_field_text (getField(), 1, month);
		ygtk_field_entry_set_field_text (getField(), 2, day);
	}

	virtual string value()
	{
		IMPL
		const gchar *year, *month, *day;
		year  = ygtk_field_entry_get_field_text (getField(), 0);
		month = ygtk_field_entry_get_field_text (getField(), 1);
		day   = ygtk_field_entry_get_field_text (getField(), 2);

		gchar *time = g_strdup_printf ("%04d-%02d-%02d", atoi (year),
		                               atoi (month), atoi (day));
		string str (time);
		g_free (time);
		return str;
	}

	// callbacks
	static void value_changed_cb (YGtkFieldEntry *entry, gint field_nb,
	                              YGDateField *pThis)
	{
		int year, month, day;
		year  = atoi (ygtk_field_entry_get_field_text (pThis->getField(), 0));
		month = atoi (ygtk_field_entry_get_field_text (pThis->getField(), 1));
		day   = atoi (ygtk_field_entry_get_field_text (pThis->getField(), 2));

		if (day < 1 || day > 31 || month < 1 || month > 12)
			return;	// avoid GtkCalendar warnings

		g_signal_handlers_block_by_func (pThis->getCalendar(),
		                                 (gpointer) calendar_changed_cb, pThis);

		gtk_calendar_select_month (pThis->getCalendar(), month-1, year);
		gtk_calendar_select_day (pThis->getCalendar(), day);

		g_signal_handlers_unblock_by_func (pThis->getCalendar(),
		                                   (gpointer) calendar_changed_cb, pThis);

		pThis->emitEvent (YEvent::ValueChanged);
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

	YGWIDGET_IMPL_COMMON
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YDateField)
};

YDateField *YGOptionalWidgetFactory::createDateField (YWidget *parent, const string &label)
{
	IMPL
	return new YGDateField (parent, label);
}

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
		IMPL
		setStretchable (YD_HORIZ, true);
		setStretchable (YD_VERT,  true);
		ygtk_time_zone_picker_set_map (YGTK_TIME_ZONE_PICKER (getWidget()),
			pixmap.c_str(), convert_code_to_name, (gpointer) &timezones);

		connect (getWidget(), "zone-clicked",
		         G_CALLBACK (zone_clicked_cb), this);
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

	YGWIDGET_IMPL_COMMON
};

YTimezoneSelector *YGOptionalWidgetFactory::createTimezoneSelector (YWidget *parent,
	const std::string &pixmap, const std::map <std::string, std::string> &timezones)
{
	return new YGTimezoneSelector (parent, pixmap, timezones);
}

