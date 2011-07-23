/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Show /var/log/zypp/history.
   Uses zypp::parser::HistoryLogReader.
*/

#ifndef YGTK_PKG_HISTORY_DIALOG_H
#define YGTK_PKG_HISTORY_DIALOG_H

#include <gtk/gtk.h>

struct YGtkPkgHistoryDialog
{
	YGtkPkgHistoryDialog();
	~YGtkPkgHistoryDialog();

	void popup();

private:
	GtkWidget *m_dialog;
};

// you do not want to use this class directly:
// use YGPackageSelector::get()->showHistoryDialog()

#endif

