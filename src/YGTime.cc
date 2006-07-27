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
	  YGLabeledWidget (this, parent, label, YD_VERT, true,
	                   YGTK_TYPE_FIELD_ENTRY, NULL)
	{
		IMPL
		guint fields_width[2] = { 2, 2 };
		ygtk_field_entry_set(YGTK_FIELD_ENTRY (getWidget()), 2, fields_width,
		                     ':', YGTK_FIELD_ALIGN_LEFT);
		setNewTime (time);

		g_signal_connect (G_OBJECT (getWidget()), "field-text-changed",
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

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_KEYBOARD_FOCUS

	// YGLabelWidget
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN(YTime)

	// callbacks
	static void value_changed_cb (YGtkFieldEntry *entry, gint field_nb,
	                              YGTime *pThis)
	{ IMPL; pThis->emitEvent (YEvent::ValueChanged); }
};

YWidget *
YGUI::createTime (YWidget *parent, YWidgetOpt &opt,
                  const YCPString &label, const YCPString &time)
{
	IMPL
	return new YGTime (opt, YGWidget::get (parent), label, time);
}

#include "YDate.h"
#include "ygtkspinbox.h"

#if 0
#include "ygtkmenubutton.h"

static const char *calendar_xpm[] =
{
	"18 18 191 2",
	"  	c None", ". 	c #9F9FA5", "+ 	c #545361", "@ 	c #1A191E", "# 	c #9D9D9D",
	"$ 	c #85848C", "% 	c #7F7F92", "& 	c #57567F", "* 	c #726FB2", "= 	c #413E63",
	"- 	c #828282", "; 	c #969696", "> 	c #818181", ", 	c #64637C", "' 	c #6D6B9C",
	") 	c #7A7AC2", "! 	c #8888E0", "~ 	c #9393F5", "{ 	c #8684DA", "] 	c #4E4D5B",
	"^ 	c #A8A8A8", "/ 	c #888891", "( 	c #5E5C74", "_ 	c #4F4F76", ": 	c #8380CE",
	"< 	c #908FED", "[ 	c #9999FA", "} 	c #9C9CF5", "| 	c #AAAAE8", "1 	c #6C6A80",
	"2 	c #8B8B8B", "3 	c #7B7985", "4 	c #7C7C99", "5 	c #656497", "6 	c #7876BE",
	"7 	c #8888E3", "8 	c #9796F9", "9 	c #9999FF", "0 	c #9F9FE9", "a 	c #A1A1E1",
	"b 	c #9B9AD1", "c 	c #DADAE6", "d 	c #C0C0C4", "e 	c #676767", "f 	c #6A688A",
	"g 	c #7573AD", "h 	c #7F7FCC", "i 	c #8A8AE5", "j 	c #9494F6", "k 	c #9C9CF3",
	"l 	c #A3A3DC", "m 	c #CDCDE7", "n 	c #CECEE2", "o 	c #AEADC0", "p 	c #FAFAFA",
	"q 	c #EAEAEA", "r 	c #808080", "s 	c #6A6A6A", "t 	c #8886D8", "u 	c #9392F3",
	"v 	c #9595F8", "w 	c #9C9CF2", "x 	c #B0B0E6", "y 	c #B3B3DA", "z 	c #A5A4B9",
	"A 	c #F8F8FB", "B 	c #F4F4F6", "C 	c #C2C2C4", "D 	c #F2F2F2", "E 	c #D4D3DA",
	"F 	c #CECECE", "G 	c #373737", "H 	c #9898FC", "I 	c #A0A0E5", "J 	c #A2A2DC",
	"K 	c #9A9ACB", "L 	c #E1E1E7", "M 	c #E2E2E5", "N 	c #B5B3C0", "O 	c #FFFFFF",
	"P 	c #E5E5E5", "Q 	c #D3D3D3", "R 	c #CBCBCB", "S 	c #D0CEDD", "T 	c #FBFBFB",
	"U 	c #8A8A8A", "V 	c #6D6D6D", "W 	c #9D9DF2", "X 	c #A4A4D7", "Y 	c #D3D3E6",
	"Z 	c #D4D4E1", "` 	c #B0AFBD", " .	c #FEFEFE", "..	c #E9E9E9", "+.	c #A4A4A4",
	"@.	c #BFBFBF", "#.	c #BDBACF", "$.	c #F1F1F1", "%.	c #D8D8D8", "&.	c #D5D4DB",
	"*.	c #E4E4E4", "=.	c #E3E3E3", "-.	c #767676", ";.	c #A0A0A0", ">.	c #B6B6D8",
	",.	c #A7A5B4", "'.	c #C6C6C6", ").	c #E7E7E7", "!.	c #B6B5BD", "~.	c #A7A7A7",
	"{.	c #8D8D8D", "].	c #C2C0D5", "^.	c #F5F5F5", "/.	c #E6E6E6", "(.	c #BFBFC6",
	"_.	c #D2D1D7", ":.	c #B4B4B4", "<.	c #747474", "[.	c #B4B2C0", "}.	c #E2E2E4",
	"|.	c #D4D4D4", "1.	c #AFAFAF", "2.	c #A8A7B4", "3.	c #CCCCCC", "4.	c #9F9F9F",
	"5.	c #BABAC0", "6.	c #DFDEE4", "7.	c #CDCDD3", "8.	c #DBDAE1", "9.	c #636363",
	"0.	c #EDEDEF", "a.	c #C2C2C2", "b.	c #D7D7D7", "c.	c #C0BDD1", "d.	c #ACABB3",
	"e.	c #B8B8B9", "f.	c #CFCFCF", "g.	c #BBBAC0", "h.	c #D9D8DF", "i.	c #E8E8E8",
	"j.	c #A5A5A5", "k.	c #5F5F5F", "l.	c #D2D1D9", "m.	c #DCDCDC", "n.	c #BEBEBE",
	"o.	c #C6C4D9", "p.	c #F2F2F4", "q.	c #ADADB3", "r.	c #BDBDC3", "s.	c #E2E2E2",
	"t.	c #D9D9D9", "u.	c #DFDFDF", "v.	c #303030", "w.	c #D0CEDA", "x.	c #F9F9F9",
	"y.	c #D0D0D0", "z.	c #CDCCD3", "A.	c #DEDDE3", "B.	c #D0D0D5", "C.	c #D9D8DE",
	"D.	c #E0E0E0", "E.	c #D5D5D5", "F.	c #8F8F8F", "G.	c #A2A2A2", "H.	c #999999",
	"I.	c #D0CFD6", "J.	c #DFDFE1", "K.	c #EBEBEB", "L.	c #CFCED3", "M.	c #DAD9DF",
	"N.	c #C8C8C8", "O.	c #DEDEDE", "P.	c #C0C0C0", "Q.	c #C3C3C3", "R.	c #8E8E8E",
	"S.	c #C1C0C8", "T.	c #D4D4DB", "U.	c #D6D6D6", "V.	c #C3C2D0", "W.	c #D1D1D1",
	"X.	c #DADADA", "Y.	c #DDDDDD", "Z.	c #737373", "`.	c #898989", " +	c #D3D3D6",
	".+	c #DDDCE0", "++	c #C1BFD1", "@+	c #EEEEEE", "#+	c #777777", "$+	c #909090",
	"%+	c #929292",
	"                                    ",
	"                  . + @             ",
	"            # $ % & * = -           ",
	"        ; > , ' ) ! ~ { ]           ",
	"    ^ / ( _ : < ~ [ } | 1 -         ",
	"2 3 4 5 6 7 8 9 0 a b c d e         ",
	"f g h i j 9 k l m n o p q r s       ",
	"t u v [ w x y z A B C D E F G       ",
	"H 9 I J K L M N O P Q R S T U V     ",
	"W X Y Z `  ...+.@.#.$.%.&.*.=.-.;.  ",
	">.,.O p '.).!.~.{.].^./.(._. .:.<.  ",
	"=.[.T }.|.1.2.3.4.5.6.P 7.8.).R 9.  ",
	"0.a.b.c.$.@.d.e.f.g.h.T =.3.*.i.j.k.",
	"l.m.n.o.p.).q.r.p s.t.).=.R u.=.3.v.",
	"w.x.y.z.A...B.C./.u.f.D.).E.F.G.H.2 ",
	"I.J.K.L.M.B =.N.i.K.O.P.Q.-.R.#     ",
	"S.T.x.s.U.V.s.W.X.Y.a.Z.`.>         ",
	" +.+b.U.'.++@+@+#+$+%+# ^           "
};
#endif

class YGDate : public YDate, public YGLabeledWidget
{
	GtkWidget *spin_box, *calendar;

public:
	YGDate (const YWidgetOpt &opt, YGWidget *parent,
	        const YCPString &label, const YCPString &date)
	: YDate (opt, label),
	  YGLabeledWidget (this, parent, label, YD_VERT, true, GTK_TYPE_HBOX, NULL)
	{
		IMPL
		spin_box = ygtk_spin_box_new();
		ygtk_spin_box_add_field (getSpin(), 1000, 9000);
		ygtk_spin_box_add_field (getSpin(), 1, 12);
		ygtk_spin_box_add_field (getSpin(), 1, 31);
		ygtk_spin_box_set_separator_character (getSpin(), '-');

#if 0
		GtkWidget *menu_button = ygtk_menu_button_new();

		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_xpm_data (calendar_xpm);
		ygtk_menu_button_set_icon (YGTK_MENU_BUTTON (menu_button), pixbuf);

		// TODO: needs to be translatable
		ygtk_menu_button_set_label (YGTK_MENU_BUTTON (menu_button), "Choose");

		calendar = gtk_calendar_new();
		GtkWidget *menu = gtk_menu_new();
//		gtk_menu_shell_append (GTK_MENU_SHELL (menu), calendar);
//		ygtk_menu_button_set_menu (YGTK_MENU_BUTTON (menu_button), GTK_MENU (menu));
gtk_widget_show_all (menu);
gtk_widget_show (calendar);

GtkWidget* menu_item = gtk_menu_item_new();
gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), calendar);

		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);


		ygtk_menu_button_set_menu (YGTK_MENU_BUTTON (menu_button), GTK_MENU (menu));
#endif
		gtk_container_add (GTK_CONTAINER (getWidget()), spin_box);
#if 0
		gtk_container_add (GTK_CONTAINER (getWidget()), menu_button);
		gtk_box_set_child_packing (GTK_BOX (getWidget()), menu_button,
		                           FALSE, FALSE, 0, GTK_PACK_START);
#endif
		gtk_widget_show (spin_box);

		setNewDate (date);

		g_signal_connect (G_OBJECT (spin_box), "value-changed",
		                  G_CALLBACK (spin_changed_cb), this);
#if 0
		g_signal_connect (G_OBJECT (calendar), "day-selected",
		                  G_CALLBACK (calendar_changed_cb), this);
#endif
	}

	virtual ~YGDate() {}

	GtkCalendar *getCalendar()
	{ return GTK_CALENDAR (calendar); }

	YGtkSpinBox *getSpin()
	{ return YGTK_SPIN_BOX (spin_box); }

	// YDate
	virtual void setNewDate (const YCPString &date)
	{
		IMPL
		int day = 0, month = 0, year = 0;
		sscanf (date->value_cstr(), "%04d-%02d-%02d", &year, &month, &day);
#if 0
		gtk_calendar_select_month (getCalendar(), month, year);
		gtk_calendar_select_day (getCalendar(), day);
#endif
		ygtk_spin_box_set_value (getSpin(), 0, year);
		ygtk_spin_box_set_value (getSpin(), 1, month);
		ygtk_spin_box_set_value (getSpin(), 2, day);
	}

	virtual YCPString getDate()
	{
		IMPL
		gchar *time = g_strdup_printf ("%04d-%02d-%02d",
		                               ygtk_spin_box_get_value (getSpin(), 0),
		                               ygtk_spin_box_get_value (getSpin(), 1),
		                               ygtk_spin_box_get_value (getSpin(), 2));
		YCPString str = YCPString (time);
		g_free (time);
		return str;
	}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_KEYBOARD_FOCUS

	// YGLabelWidget
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN(YDate)

	// callbacks
	static void spin_changed_cb (YGtkSpinBox *spinner, YGDate *pThis)
	{
		IMPL
#if 0
		gtk_calendar_select_month (pThis->getCalendar(),
				ygtk_spin_box_get_value (pThis->getSpin(), 1),
				ygtk_spin_box_get_value (pThis->getSpin(), 0));
		gtk_calendar_select_day (pThis->getCalendar(),
				ygtk_spin_box_get_value (pThis->getSpin(), 2));
#endif
		pThis->emitEvent (YEvent::ValueChanged);
	}
#if 0
	static void calendar_changed_cb (GtkCalendar *calendar, YGDate *pThis)
	{
		IMPL
		guint day, month, year;
		gtk_calendar_get_date (calendar, &year, &month, &day);

		ygtk_spin_box_set_value (pThis->getSpin(), 0, year);
		ygtk_spin_box_set_value (pThis->getSpin(), 1, month);
		ygtk_spin_box_set_value (pThis->getSpin(), 2, day);

		pThis->emitEvent (YEvent::ValueChanged);
	}
#endif
};

YWidget *
YGUI::createDate (YWidget *parent, YWidgetOpt &opt,
                  const YCPString &label, const YCPString &date)
{
	IMPL
	return new YGDate (opt, YGWidget::get (parent), label, date);
}
