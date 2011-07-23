/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* A dialog to help the user track no longer needed dependencies.
*/

#ifndef YGTK_PKG_VESTIGIAL_DIALOG_H
#define YGTK_PKG_VESTIGIAL_DIALOG_H

#include <gtk/gtk.h>

struct YGtkPkgVestigialDialog
{
	YGtkPkgVestigialDialog();
	~YGtkPkgVestigialDialog();

	void popup();

	struct Impl;
	Impl *impl;
};

// you do not want to use this class directly:
// use YGPackageSelector::get()->showVestigialDialog()

#endif

