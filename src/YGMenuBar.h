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

  File:         YGMenuBar.h

  Author:       Angelo Naselli <anaselli@linux.it>

/-*/

#ifndef YGMenuBar_h
#define YGMenuBar_h


#include <yui/YMenuBar.h>
#include <yui/gtk/ygtktreeview.h>

#include <yui/gtk/YGSelectionStore.h>
#include <yui/gtk/YGWidget.h>

#include <gtk/gtk.h>


class YGMenuBar : public YMenuBar, public YGWidget
{
public:

  YGMenuBar ( YWidget *parent );

  virtual ~YGMenuBar ( );

  /**
    * Rebuild the displayed menu tree from the internally stored YMenuItems.
    *
    * Reimplemented from YMenuWidget.
    **/
  virtual void rebuildMenuTree();

  /**
    * Enable or disable an item.
    *
    * Reimplemented from YMenuWidget.
    **/
  virtual void setItemEnabled( YMenuItem * item, bool enabled );

  /**
    * Show or hide an item.
    *
    * Reimplemented from YMenuWidget.
    **/
  virtual void setItemVisible( YMenuItem * item, bool visible );

  /**
    * Activate the item selected in the tree. Can be used in tests to simulate
    * user input.
    **/
  virtual void activateItem( YMenuItem * item );

  /**
  * Delete all items.
  *
  * Reimplemented from YMenuWidget
  **/
  virtual void deleteAllItems();


  YGWIDGET_IMPL_COMMON (YMenuBar)

private:
  struct Private;
  Private *d;

  void doCreateMenu (GtkWidget *menu, YItemIterator begin, YItemIterator end);
};

#endif // YGMenuBar_h
