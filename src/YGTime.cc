//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "ygtkfieldentry.h"

#include "YTime.h"

class YGTime : public YTime, public YGLabeledWidget
{
public:
	YGTime (const YWidgetOpt &opt, YGWidget *parent,
	        const YCPString &label, const YCPString &time)
	: YTime (opt, label),
	  YGLabeledWidget (this, parent, label, YD_HORIZ, true,
	                   YGTK_TYPE_FIELD_ENTRY, NULL)
	{
		IMPL
		ygtk_field_entry_add_field (YGTK_FIELD_ENTRY (getWidget()), ':',
		                            2, "0123456789");
		ygtk_field_entry_add_field (YGTK_FIELD_ENTRY (getWidget()), ':',
		                            2, "0123456789");
		setNewTime (time);

		gtk_widget_show_all (getWidget());
		g_signal_connect (G_OBJECT (getWidget()), "field-entry-changed",
		                  G_CALLBACK (value_changed_cb), this);
	}

	virtual ~YGTime() {}

	// YTime
	virtual void setNewTime (const YCPString &time)
	{
		IMPL
		char hours[3], mins[3];
		sscanf (time->value_cstr(), "%2s:%2s", hours, mins);

		YGtkFieldEntry *entry = YGTK_FIELD_ENTRY (getWidget());
		ygtk_field_entry_set_field_text (entry, 0, hours);
		ygtk_field_entry_set_field_text (entry, 1, mins);
	}

	virtual YCPString getTime()
	{
		IMPL
		const gchar *hours, *mins;
		YGtkFieldEntry *entry = YGTK_FIELD_ENTRY (getWidget());
		hours = ygtk_field_entry_get_field_text (entry, 0);
		mins  = ygtk_field_entry_get_field_text (entry, 1);

		gchar *time = g_strdup_printf ("%02d:%02d:00", atoi (hours), atoi (mins));
		YCPString str = YCPString (time);
		g_free (time);
		return str;
	}

	// callbacks
	static void value_changed_cb (YGtkFieldEntry *entry, gint field_nb,
	                              YGTime *pThis)
	{ IMPL; pThis->emitEvent (YEvent::ValueChanged); }

	YGWIDGET_IMPL_COMMON
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN(YTime)
};

YWidget *
YGUI::createTime (YWidget *parent, YWidgetOpt &opt,
                  const YCPString &label, const YCPString &time)
{
	IMPL
	return new YGTime (opt, YGWidget::get (parent), label, time);
}

#include "YDate.h"
#include "ygtkmenubutton.h"

class YGDate : public YDate, public YGLabeledWidget
{
	GtkWidget *m_entry, *m_calendar, *m_popup_calendar;

public:
	YGDate (const YWidgetOpt &opt, YGWidget *parent,
	        const YCPString &label, const YCPString &date)
	: YDate (opt, label),
	  YGLabeledWidget (this, parent, label, YD_HORIZ, true, GTK_TYPE_HBOX, NULL)
	{
		IMPL
		m_entry = ygtk_field_entry_new();
		ygtk_field_entry_add_field (YGTK_FIELD_ENTRY (m_entry), '-', 4, "0123456789");
		ygtk_field_entry_add_field (YGTK_FIELD_ENTRY (m_entry), '-', 2, "0123456789");
		ygtk_field_entry_add_field (YGTK_FIELD_ENTRY (m_entry), '-', 2, "0123456789");

		GtkWidget *menu_button = ygtk_menu_button_new();
		m_calendar = gtk_calendar_new();
		gtk_widget_show (m_calendar);
		GtkWidget *popup = ygtk_popup_window_new (m_calendar);
		ygtk_menu_button_set_popup (YGTK_MENU_BUTTON (menu_button), popup);

		gtk_box_pack_start (GTK_BOX (getWidget()), m_entry, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (getWidget()), menu_button, FALSE, TRUE, 6);
		gtk_widget_show_all (getWidget());

		setNewDate (date);
		g_signal_connect (G_OBJECT (m_entry), "field-entry-changed",
		                  G_CALLBACK (value_changed_cb), this);

		g_signal_connect (G_OBJECT (m_calendar), "day-selected",
		                  G_CALLBACK (calendar_changed_cb), this);
		g_signal_connect (G_OBJECT (m_calendar), "day-selected-double-click",
		                  G_CALLBACK (double_click_cb), popup);
	}

	virtual ~YGDate() {}

	GtkCalendar *getCalendar()
	{ return GTK_CALENDAR (m_calendar); }

	YGtkFieldEntry *getEntry()
	{ return YGTK_FIELD_ENTRY (m_entry); }

	// YDate
	virtual void setNewDate (const YCPString &date)
	{
		IMPL
		char year[5], month[3], day[3];
		sscanf (date->value_cstr(), "%4s-%2s-%2s", year, month, day);

		gtk_calendar_select_month (getCalendar(), atoi (month)-1, atoi (year));
		gtk_calendar_select_day (getCalendar(), atoi (day));

		YGtkFieldEntry *entry = getEntry();
		ygtk_field_entry_set_field_text (entry, 0, year);
		ygtk_field_entry_set_field_text (entry, 1, month);
		ygtk_field_entry_set_field_text (entry, 2, day);
	}

	virtual YCPString getDate()
	{
		IMPL
		const gchar *year, *month, *day;
		YGtkFieldEntry *entry = getEntry();
		year  = ygtk_field_entry_get_field_text (entry, 0);
		month = ygtk_field_entry_get_field_text (entry, 1);
		day   = ygtk_field_entry_get_field_text (entry, 2);

		gchar *time = g_strdup_printf ("%04d-%02d-%02d", atoi (year),
		                               atoi (month), atoi (day));
		YCPString str = YCPString (time);
		g_free (time);
		return str;
	}

	// callbacks
	static void value_changed_cb (YGtkFieldEntry *entry, gint field_nb,
	                              YGDate *pThis)
	{
		IMPL
		int year, month, day;
		year  = atoi (ygtk_field_entry_get_field_text (pThis->getEntry(), 0));
		month = atoi (ygtk_field_entry_get_field_text (pThis->getEntry(), 1));
		day   = atoi (ygtk_field_entry_get_field_text (pThis->getEntry(), 2));

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

	static void calendar_changed_cb (GtkCalendar *calendar, YGDate *pThis)
	{
		IMPL
		guint year, month, day;
		gtk_calendar_get_date (calendar, &year, &month, &day);
		month += 1;  // GTK calendar months go from 0 to 11

		gchar *year_str, *month_str, *day_str;
		year_str = g_strdup_printf  ("%d", year);
		month_str = g_strdup_printf ("%d", month);
		day_str = g_strdup_printf   ("%d", day);

		g_signal_handlers_block_by_func (pThis->getEntry(),
		                                 (gpointer) value_changed_cb, pThis);

		YGtkFieldEntry *entry = pThis->getEntry();
		ygtk_field_entry_set_field_text (entry, 0, year_str);
		ygtk_field_entry_set_field_text (entry, 1, month_str);
		ygtk_field_entry_set_field_text (entry, 2, day_str);

		g_signal_handlers_unblock_by_func (pThis->getEntry(),
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
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN(YDate)
};

YWidget *
YGUI::createDate (YWidget *parent, YWidgetOpt &opt,
                  const YCPString &label, const YCPString &date)
{
	IMPL
	return new YGDate (opt, YGWidget::get (parent), label, date);
}
