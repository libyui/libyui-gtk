/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#define YUILogComponent "gtk"
#include <yui/Libyui_config.h>
#include "YGUI.h"
#include "YGUtils.h"
#include "YGWidget.h"
#include "YMenuButton.h"
#include "ygtkmenubutton.h"

static void selected_item_cb (GtkMenuItem *menuitem, YItem *item)
{
	// HACK: gtk_menu_get_active() doesn't work properly
	GtkWidget *menu = gtk_widget_get_ancestor (GTK_WIDGET (menuitem), GTK_TYPE_MENU);
	g_object_set_data (G_OBJECT (menu), "active", menuitem);

	YGUI::ui()->sendEvent (new YMenuEvent (item));
}

static void doCreateMenu (GtkWidget *parent, YItemIterator begin, YItemIterator end)
{
	for (YItemIterator it = begin; it != end; it++) {
		GtkWidget *entry;
		std::string str = YGUtils::mapKBAccel ((*it)->label());

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
	YGMenuButton (YWidget *parent, const std::string &label)
	:  YMenuButton (NULL, label),
	   YGWidget (this, parent, YGTK_TYPE_MENU_BUTTON, NULL)
	{ setLabel (label); }

	// YMenuButton
	virtual void setLabel (const std::string &label)
	{
		std::string str = YGUtils::mapKBAccel (label.c_str());
		ygtk_menu_button_set_label (YGTK_MENU_BUTTON (getWidget()), str.c_str());
		YMenuButton::setLabel (label);
	}

	virtual void rebuildMenuTree()
	{
		GtkWidget *menu = gtk_menu_new();
		doCreateMenu (menu, itemsBegin(), itemsEnd());
		gtk_widget_show_all (menu);
		ygtk_menu_button_set_popup (YGTK_MENU_BUTTON (getWidget()), menu);
	}

        // NOTE copy of Qt one
        void activateItem( YMenuItem * item )
        {
                if ( item )
                    YGUI::ui()->sendEvent( new YMenuEvent( item ) );
        }


	YGWIDGET_IMPL_COMMON (YMenuButton)
};

YMenuButton *YGWidgetFactory::createMenuButton (YWidget *parent, const std::string &label)
{ return new YGMenuButton (parent, label); }

#include <YContextMenu.h>

class YGContextMenu : public YContextMenu, public YGWidget
{
	int m_deactivateTimeout;

public:
	YGContextMenu()
	:  YContextMenu(),
	   YGWidget (this, NULL, GTK_TYPE_MENU, NULL)
	{
		// "cancel" signal doesnt seem to work properly
		m_deactivateTimeout = 0;
		connect (getWidget(), "deactivate", G_CALLBACK (deactivate_cb), this);
	}

	// YContextMenu
	virtual void rebuildMenuTree()
	{
		GtkWidget *menu = getWidget();
		doCreateMenu (menu, itemsBegin(), itemsEnd());
		gtk_widget_show_all (menu);

#		if GTK_CHECK_VERSION (3, 22, 0)
		gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
#		else
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time());
#		endif
	}

	// callbacks
	static void deactivate_cb (GtkMenuShell *menu, YGContextMenu *pThis)
	{  // ugly: we need to make sure a selection was made before this callback called
		// we'll use a timeout because deactivate seems to be called more than once
		if(pThis->m_deactivateTimeout == 0)
			pThis->m_deactivateTimeout = g_timeout_add_full (G_PRIORITY_LOW, 50, cancel_cb, pThis, NULL);
		//g_idle_add_full (G_PRIORITY_LOW, cancel_cb, pThis, NULL);
	}

	static gboolean cancel_cb (gpointer data)
	{
		YGContextMenu *pThis = (YGContextMenu *) data;
		if (!g_object_get_data (G_OBJECT (pThis->getWidget()), "active"))
			YGUI::ui()->sendEvent (new YCancelEvent());
		delete pThis;
		return FALSE;
	}

	YGWIDGET_IMPL_COMMON (YContextMenu)
};

bool YGApplication::openContextMenu (const YItemCollection &itemCollection)
{
	YGContextMenu *menu = new YGContextMenu();
	menu->addItems (itemCollection);
	return true;
}

