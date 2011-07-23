/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Status bar, showing the changes to apply.
*/

#ifndef YGTK_PKG_STATUS_BAR_H
#define YGTK_PKG_STATUS_BAR_H

#include <gtk/gtk.h>

struct YGtkPkgUndoList;

struct YGtkPkgStatusBar
{
	YGtkPkgStatusBar (YGtkPkgUndoList *undo);
	~YGtkPkgStatusBar();

	GtkWidget *getWidget();

	struct Impl;
	Impl *impl;
};

#endif

