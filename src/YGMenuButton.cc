/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YGWidget.h"
#include "YMenuButton.h"
#include "ygtkmenubutton.h"

class YGMenuButton : public YMenuButton, public YGWidget
{
public:
	YGMenuButton (YWidget *parent, const string &label)
	:  YMenuButton (NULL, label),
	   YGWidget (this, parent, YGTK_TYPE_MENU_BUTTON, NULL)
	{
		IMPL
		string str = YGUtils::mapKBAccel (label.c_str());
		ygtk_menu_button_set_label (YGTK_MENU_BUTTON (getWidget()), str.c_str());
	}

	// YMenuButton
	virtual void rebuildMenuTree()
	{
		GtkWidget *menu = doCreateMenu (itemsBegin(), itemsEnd());
		gtk_widget_show_all (menu);
		ygtk_menu_button_set_popup (YGTK_MENU_BUTTON (getWidget()), menu);
	}

	static GtkWidget* doCreateMenu (YItemIterator begin, YItemIterator end)
	{
		GtkWidget *menu = gtk_menu_new();
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

			gtk_menu_shell_append (GTK_MENU_SHELL (menu), entry);

			if ((*it)->hasChildren())
				gtk_menu_item_set_submenu (GTK_MENU_ITEM (entry),
					doCreateMenu ((*it)->childrenBegin(), (*it)->childrenEnd()));
			else
				g_signal_connect (G_OBJECT (entry), "activate",
				                  G_CALLBACK (selected_item_cb), *it);
		}

		return menu;
	}

	static void selected_item_cb (GtkMenuItem *menuitem, YItem *item)
	{
		YGUI::ui()->sendEvent (new YMenuEvent (item));
	}

	YGWIDGET_IMPL_COMMON
};

YMenuButton *YGWidgetFactory::createMenuButton (YWidget *parent, const string &label)
{
	return new YGMenuButton (parent, label);
}

