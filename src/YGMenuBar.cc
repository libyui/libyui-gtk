/*
  Copyright 2020 by Angelo Naselli <anaselli at linux dot it>

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) version 3.0 of the License. This library
  is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
  License for more details. You should have received a copy of the GNU
  Lesser General Public License along with this library; if not, write
  to the Free Software Foundation, Inc., 51 Franklin Street, Fifth
  Floor, Boston, MA 02110-1301 USA
*/


/*-/

  File:         YGMenuBar.cc

  Author:       Angelo Naselli <anaselli@linux.it>

/-*/

#define YUILogComponent "gtk"

#include <yui/YUILog.h>
#include <YGUI.h>
#include <yui/gtk/YGUtils.h>
#include <yui/gtk/YGWidget.h>
#include <YSelectionWidget.h>
#include <yui/gtk/YGSelectionStore.h>
#include <yui/gtk/ygtktreeview.h>
#include <yui/gtk/ygtkratiobox.h>
#include <string.h>
#include <yui/YMenuItem.h>
#include <YTable.h>

#include <yui/gtk/YGDialog.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkmenubar.h>
#include <gtk/gtk.h>
#include "YGMenuBar.h"
#include <boost/filesystem.hpp>

#define YGTK_VBOX_NEW(arg) gtk_box_new(GTK_ORIENTATION_VERTICAL,arg)
#define YGTK_HBOX_NEW(arg) gtk_box_new(GTK_ORIENTATION_HORIZONTAL,arg)

typedef std::map<YItem*, GtkWidget*> MenuEntryMap;
typedef std::pair<YItem*, GtkWidget*> MenuEntryPair;

typedef std::map<YItem*, gulong> MenuCBMap;
typedef std::pair<YItem*, gulong> MenuCBPair;


struct YGMenuBar::Private
{
  GtkWidget *menubar;

  MenuEntryMap menu_entry;
  MenuCBMap menu_cb;

  std::vector<GtkWidget*> menu_widgets;
};

YGMenuBar::YGMenuBar(YWidget* parent)
  :  YMenuBar(NULL),
  YGWidget(this, parent, gtk_menu_bar_get_type(), NULL),
  d(new Private)

{
  YUI_CHECK_NEW ( d );
  d->menubar = getWidget();
}



static void selected_menuitem(GtkMenuItem *, YItem *item)
{
  YGUI::ui()->sendEvent (new YMenuEvent (item));
}


void YGMenuBar::rebuildMenuTree()
{
  // old menu cleanup
  for (MenuCBMap::iterator it=d->menu_cb.begin(); it!=d->menu_cb.end(); ++it)
  {
    auto search = d->menu_entry.find( it->first );
    if (search != d->menu_entry.end())
      g_signal_handler_disconnect (search->second, it->second);
  }

  for (GtkWidget *m: d->menu_widgets)
  {
    gtk_widget_destroy(m);
  }

  d->menu_widgets.clear();
  d->menu_cb.clear();
  d->menu_entry.clear();

  for ( YItemIterator it = itemsBegin(); it != itemsEnd(); ++it )
  {
    YMenuItem * item = dynamic_cast<YMenuItem *>( *it );
    YUI_CHECK_PTR( item );

    if ( ! item->isMenu() )
        YUI_THROW( YUIException( "YQMenuBar: Only menus allowed on toplevel." ) );

    yuiDebug() << item->label() << std::endl;
    GtkWidget *menu = gtk_menu_new();
    std::string lbl = YGUtils::mapKBAccel (item->label());
    GtkWidget *menu_entry = gtk_menu_item_new_with_mnemonic(lbl.c_str());

    d->menu_widgets.push_back(menu_entry);

    d->menu_entry.insert(MenuEntryPair(*it, menu_entry));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_entry), menu);
    gtk_widget_show(menu_entry);
    gtk_menu_shell_append (GTK_MENU_SHELL (d->menubar), menu_entry);

    yuiDebug() << item->label() << " enabled: " << item->isEnabled() << " visible:" << item->isVisible() << std::endl;
    gtk_widget_set_sensitive(menu_entry, item->isEnabled() ? gtk_true() : gtk_false());

    if (item->hasChildren())
      doCreateMenu(menu, item->childrenBegin(), item->childrenEnd());

    if (!item->isVisible())
      gtk_widget_hide(menu_entry);
  }
}

void YGMenuBar::doCreateMenu (GtkWidget *menu, YItemIterator begin, YItemIterator end)
{
  for (YItemIterator it = begin; it != end; it++)
  {
    YMenuItem * item = dynamic_cast<YMenuItem *>( *it );
    YUI_CHECK_PTR( item );

    if (item->isSeparator())
    {
      GtkWidget *sep = gtk_separator_menu_item_new();
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep);
      gtk_widget_show(sep);
    }
    else
    {
      GtkWidget *entry;
      YItem * yitem = *it;
      std::string action_name = YGUtils::mapKBAccel (yitem->label());
      if (yitem->hasIconName())
      {
        // if extension is present we consider a full path name, theme icons don't have extensions
        GtkWidget *icon;
        if (boost::filesystem::path(yitem->iconName()).has_extension())
        {
          icon = gtk_image_new_from_file(yitem->iconName().c_str());
        }
        else
        {
          GtkIconTheme * theme = gtk_icon_theme_get_default();
          std::string ico = boost::filesystem::path(yitem->iconName()).stem().c_str();
          if (gtk_icon_theme_has_icon (theme, ico.c_str()))
          {
            icon = gtk_image_new_from_icon_name (ico.c_str(), GTK_ICON_SIZE_MENU);
          }
          else
          {
            // last chance, just to add an icon
            icon = gtk_image_new_from_file(yitem->iconName().c_str());
          }
        }

        GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
        GtkWidget *label = gtk_label_new (action_name.c_str());
        entry = gtk_menu_item_new ();

        gtk_container_add (GTK_CONTAINER (box), icon);
        gtk_container_add (GTK_CONTAINER (box), label);

        gtk_label_set_use_underline (GTK_LABEL (label), TRUE);
        gtk_label_set_xalign (GTK_LABEL (label), 0.0);

        gtk_container_add (GTK_CONTAINER (entry), box);
      }
      else
      {
        entry = gtk_menu_item_new_with_mnemonic (action_name.c_str());
      }
      d->menu_entry.insert(MenuEntryPair(*it, entry));

      yuiDebug() << item->label() << " enabled: " << item->isEnabled() << " visible:" << item->isVisible() << std::endl;
      gtk_widget_set_sensitive(entry, item->isEnabled() ? gtk_true() : gtk_false());
      if (item->hasChildren())
      {
        GtkWidget *submenu = gtk_menu_new();

        gtk_menu_item_set_submenu(GTK_MENU_ITEM(entry), submenu);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), entry);
        gtk_widget_show_all(entry);

        doCreateMenu (submenu, (*it)->childrenBegin(), (*it)->childrenEnd());
      }
      else
      {
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), entry);
        gtk_widget_show_all (entry);

        gulong id = g_signal_connect (G_OBJECT (entry), "activate",
                          G_CALLBACK (selected_menuitem), *it);

        d->menu_cb.insert(MenuCBPair(*it, id));
      }
      if (!item->isVisible())
        gtk_widget_hide(entry);
    }
  }
}

void YGMenuBar::setItemEnabled( YMenuItem * item, bool enabled )
{
  YMenuWidget::setItemEnabled(item, enabled);

  auto search = d->menu_entry.find( item );
  if (search != d->menu_entry.end())
  {
    GtkWidget * menu_entry = search->second;
    gtk_widget_set_sensitive(menu_entry, enabled ? gtk_true() : gtk_false());
  }
  else
  {
    yuiError() << item->label() << " not found" << std::endl;
  }
}


void YGMenuBar::setItemVisible( YMenuItem * item, bool visible )
{
  YMenuWidget::setItemVisible(item, visible);

  auto search = d->menu_entry.find( item );
  if (search != d->menu_entry.end())
  {
    GtkWidget * menu_entry = search->second;
    gtk_widget_set_visible(menu_entry, visible ? gtk_true() : gtk_false());
  }
  else
  {
    yuiError() << item->label() << " not found" << std::endl;
  }
}

void YGMenuBar::deleteAllItems()
{
  for (MenuCBMap::iterator it=d->menu_cb.begin(); it!=d->menu_cb.end(); ++it)
  {
    auto search = d->menu_entry.find( it->first );
    if (search != d->menu_entry.end())
      g_signal_handler_disconnect (search->second, it->second);
  }

  for (GtkWidget *m: d->menu_widgets)
  {
    gtk_widget_destroy(m);
  }

  d->menu_widgets.clear();
  d->menu_cb.clear();
  d->menu_entry.clear();

  YMenuWidget::deleteAllItems();
}

void YGMenuBar::activateItem( YMenuItem * item )
{
  auto search = d->menu_entry.find( item );
  if (search != d->menu_entry.end())
  {
    selected_menuitem(NULL, item);
  }
  else
  {
    yuiError() << item->label() << " not found" << std::endl;
  }
}

YGMenuBar::~YGMenuBar()
{
  d->menu_cb.clear();
  d->menu_entry.clear();

  delete d;
}

YMenuBar *YGWidgetFactory::createMenuBar ( YWidget * parent )
{
  return new YGMenuBar(parent);
}

