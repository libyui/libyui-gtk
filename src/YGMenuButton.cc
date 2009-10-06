/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#define YUILogComponent "gtk"
#include "config.h"
#include "YGUI.h"
#include "YGUtils.h"
#include "YGWidget.h"
#include "YMenuButton.h"
#include "ygtkmenubutton.h"

static void selected_item_cb (GtkMenuItem *menuitem, YItem *item)
{
	YGUI::ui()->sendEvent (new YMenuEvent (item));
}

static void doCreateMenu (GtkWidget *parent, YItemIterator begin, YItemIterator end)
{
	for (YItemIterator it = begin; it != end; it++) {
		GtkWidget *entry, *image = 0;
		string str = YGUtils::mapKBAccel ((*it)->label());

		if ((*it)->hasIconName()) {
			GdkPixbuf *pixbuf = YGUtils::loadPixbuf ((*it)->iconName());
			if (pixbuf) {
				image = gtk_image_new_from_pixbuf (pixbuf);
				g_object_unref (G_OBJECT (pixbuf));
			}
		}

		if (image) {
			entry = gtk_image_menu_item_new_with_mnemonic (str.c_str());
			gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (entry), image);
		}
		else
			entry = gtk_menu_item_new_with_mnemonic (str.c_str());

		gtk_menu_shell_append (GTK_MENU_SHELL (parent), entry);

		if ((*it)->hasChildren()) {
			GtkWidget *submenu = gtk_menu_new();
			doCreateMenu (submenu, (*it)->childrenBegin(), (*it)->childrenEnd());
			gtk_menu_item_set_submenu (GTK_MENU_ITEM (entry), submenu);
		}
		else
			g_signal_connect (G_OBJECT (entry), "activate",
			                  G_CALLBACK (selected_item_cb), *it);
	}
}

class YGMenuButton : public YMenuButton, public YGWidget
{
public:
	YGMenuButton (YWidget *parent, const string &label)
	:  YMenuButton (NULL, label),
	   YGWidget (this, parent, YGTK_TYPE_MENU_BUTTON, NULL)
	{
		string str = YGUtils::mapKBAccel (label.c_str());
		ygtk_menu_button_set_label (YGTK_MENU_BUTTON (getWidget()), str.c_str());
	}

	// YMenuButton
	virtual void rebuildMenuTree()
	{
		GtkWidget *menu = gtk_menu_new();
		doCreateMenu (menu, itemsBegin(), itemsEnd());
		gtk_widget_show_all (menu);
		ygtk_menu_button_set_popup (YGTK_MENU_BUTTON (getWidget()), menu);
	}

	YGWIDGET_IMPL_COMMON (YMenuButton)
};

YMenuButton *YGWidgetFactory::createMenuButton (YWidget *parent, const string &label)
{ return new YGMenuButton (parent, label); }

#if YAST2_VERSION > 2018003
#include <YContextMenu.h>

class YGContextMenu : public YContextMenu, public YGWidget
{
public:
	YGContextMenu()
	:  YContextMenu(),
	   YGWidget (this, NULL, GTK_TYPE_MENU, NULL)
	{
		connect (getWidget(), "cancel", G_CALLBACK (cancel_cb), this);
	}

	// YContextMenu
	virtual void rebuildMenuTree()
	{
		GtkWidget *menu = getWidget();
		doCreateMenu (menu, itemsBegin(), itemsEnd());
		gtk_widget_show_all (menu);
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time());
	}

	// callbacks
	static void cancel_cb (GtkMenuShell *menu, YGContextMenu *pThis)
	{ YGUI::ui()->sendEvent (new YCancelEvent()); }

	YGWIDGET_IMPL_COMMON (YContextMenu)
};

bool YGApplication::openContextMenu (const YItemCollection &itemCollection)
{
	YGContextMenu *menu = new YGContextMenu();
	menu->addItems (itemCollection);
	return true;
}
#endif

