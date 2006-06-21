#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YGUtils.h"
#include "YGWidget.h"
#include "YMenuButton.h"

class YGMenuButton : public YMenuButton, public YGWidget
{
GtkWidget *m_menu, *m_label;

public:
	YGMenuButton (const YWidgetOpt &opt,
	              YGWidget         *parent,
	              YCPString         label)
	:  YMenuButton (opt, label),
	   YGWidget (this, parent, true, GTK_TYPE_TOGGLE_BUTTON, NULL)
	{
		IMPL;
		g_signal_connect (G_OBJECT (getWidget()), "toggled",
		                  G_CALLBACK (toggled_cb), this);

		// Add a separator and an arrow to the button to indicate there is a menu
		GtkWidget *hbox, *separator, *arrow;
		hbox = gtk_hbox_new (FALSE, 0);
		m_label = gtk_label_new ("");
		separator = gtk_vseparator_new ();
		arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);

		// put arrow in different end according to the language phrase direction
		if (gtk_widget_get_direction (getWidget()) == GTK_TEXT_DIR_LTR && false) {
			gtk_container_add (GTK_CONTAINER (hbox), m_label);
			gtk_container_add (GTK_CONTAINER (hbox), separator);
			gtk_container_add (GTK_CONTAINER (hbox), arrow);
		}
		else {
			gtk_container_add (GTK_CONTAINER (hbox), arrow);
			gtk_container_add (GTK_CONTAINER (hbox), separator);
			gtk_container_add (GTK_CONTAINER (hbox), m_label);
		}
		gtk_box_set_child_packing (GTK_BOX (hbox), separator,
		                      FALSE, FALSE, 5, GTK_PACK_START);
		gtk_box_set_child_packing (GTK_BOX (hbox), arrow,
		                      FALSE, FALSE, 5, GTK_PACK_START);
		gtk_container_add (GTK_CONTAINER (getWidget()), hbox);
		gtk_widget_show_all (getWidget());
	
		setLabel (label);
		m_menu = NULL;
	}

	virtual ~YGMenuButton() {}

	// YMenuButton
	virtual void setLabel (const YCPString &label)
	{
		IMPL;
		string str = YGUtils::mapKBAccel (label->value_cstr());
		gtk_label_set_text_with_mnemonic (GTK_LABEL (m_label), str.c_str());
		YMenuButton::setLabel (label);
	}

	GtkMenu* getMenu()
	{ return GTK_MENU (m_menu); }

	// YMenuButton
	virtual void createMenu()
	{
		if (m_menu) {
			gtk_widget_destroy (m_menu);
			g_object_unref (m_menu);
		}
		m_menu = doCreateMenu (getToplevelMenu()->itemList());

		g_signal_connect (G_OBJECT (m_menu), "hide", G_CALLBACK (menu_hiden_cb),
		                  GTK_TOGGLE_BUTTON (getWidget()));
	}

	static GtkWidget* doCreateMenu (YMenuItemList &items)
	{
		GtkWidget *menu = gtk_menu_new();
		for (YMenuItemListIterator it = items.begin(); it != items.end(); it++) {
			GtkWidget *entry;
			string str = YGUtils::mapKBAccel ((*it)->getLabel()->value_cstr());
			entry = gtk_menu_item_new_with_mnemonic (str.c_str());

			gtk_menu_shell_append (GTK_MENU_SHELL (menu), entry);

			if ((*it)->hasChildren())
				gtk_menu_item_set_submenu (GTK_MENU_ITEM (entry),
				             doCreateMenu ((*it)->itemList()));
			else
				g_signal_connect (G_OBJECT (entry), "activate",
				                  G_CALLBACK (selected_item_cb), *it);
		}

		gtk_widget_show_all (menu);
		return menu;
	}

	static void popup_menu_cb (YGMenuButton *pThis)
	{
		IMPL
		GtkMenu *menu = pThis->getMenu();
		if (menu) {
			gtk_menu_popup (menu, NULL, NULL,
			                get_menu_pos, (YGWidget*) pThis, 0,
			                gtk_get_current_event_time());
			gtk_widget_set_size_request (GTK_WIDGET (menu),
			                             pThis->getWidget()->allocation.width, -1);
		}
	}

	static void toggled_cb (GtkWidget *button, YGMenuButton* pThis)
	{
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
			popup_menu_cb (pThis);
	}

	static void selected_item_cb (GtkMenuItem *menuitem, YMenuItem *yitem)
	{
		YGUI::ui()->sendEvent (new YMenuEvent (yitem->getId()));
	}

	static void menu_hiden_cb (GtkWidget *widget, GtkToggleButton *button)
	{
		gtk_toggle_button_set_active (button, FALSE);
	}

	static void get_menu_pos (GtkMenu *menu, gint *x, gint *y,
	                         gboolean *push_in, gpointer data)
	{
		// Based on code from GtkComboBox
		GtkWidget *widget = ((YGWidget*) data)->getWidget();
		gint sx, sy;
		GtkRequisition req;

		gdk_window_get_origin (widget->window, &sx, &sy);

		if (GTK_WIDGET_NO_WINDOW (widget)) {
			sx += widget->allocation.x;
			sy += widget->allocation.y;
		}

		gtk_widget_size_request (GTK_WIDGET (menu), &req);

		if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
			*x = sx;
		else
			*x = sx + widget->allocation.width - req.width;
		*y = sy;

		GdkRectangle monitor;
		GdkScreen *screen = gtk_widget_get_screen (widget);
		int monitor_num = gdk_screen_get_monitor_at_window (screen, widget->window);
		gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

		if (*x < monitor.x)
			*x = monitor.x;
		else if (*x + req.width > monitor.x + monitor.width)
			*x = monitor.x + monitor.width - req.width;

		if (monitor.y + monitor.height - *y - widget->allocation.height >= req.height)
			*y += widget->allocation.height;
		else if (*y - monitor.y >= req.height)
			*y -= req.height;
		else if (monitor.y + monitor.height - *y - widget->allocation.height > *y - monitor.y)
			*y += widget->allocation.height;
		else
			*y -= req.height;

		*push_in = FALSE;
	}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
};

YWidget *
YGUI::createMenuButton (YWidget *parent, YWidgetOpt &opt,
                        const YCPString &label)
{
	return new YGMenuButton (opt, YGWidget::get (parent), label);
}
