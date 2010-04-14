/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Show /var/log/zypp/history.
   Uses zypp::parser::HistoryLogReader.
*/

#ifndef YGTK_PKG_HISTORY_DIALOG_H
#define YGTK_PKG_HISTORY_DIALOG_H

#include <gtk/gtkwidget.h>

struct YGtkPkgHistoryDialog
{
	YGtkPkgHistoryDialog();
	~YGtkPkgHistoryDialog();

	void popup();

private:
	GtkWidget *m_dialog;
};

// use this method to create the dialog -- so it is cached
void popupHistoryDialog();

#endif

