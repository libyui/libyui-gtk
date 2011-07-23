/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Show zypp::Product list.
*/

#ifndef YGTK_PKG_PRODUCT_DIALOG_H
#define YGTK_PKG_PRODUCT_DIALOG_H

#include <gtk/gtk.h>

struct YGtkPkgProductDialog
{
	YGtkPkgProductDialog();
	~YGtkPkgProductDialog();

	void popup();

private:
	GtkWidget *m_dialog;
};

#endif

