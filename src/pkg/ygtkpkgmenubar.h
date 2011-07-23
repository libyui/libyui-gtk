/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* A menu bar identical to that from yast2-qt.
*/

#ifndef YGTK_PKG_MENU_BAR_H
#define YGTK_PKG_MENU_BAR_H

#include <gtk/gtk.h>

struct YGtkPkgMenuBar
{
	YGtkPkgMenuBar();

	GtkWidget *getWidget() { return m_menu; }

	private:
		GtkWidget *m_menu;
};

#endif

