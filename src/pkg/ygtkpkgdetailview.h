/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Text box information on a given selectable.
*/

#ifndef YGTK_PKG_DETAIL_VIEW_H
#define YGTK_PKG_DETAIL_VIEW_H

#include <gtk/gtk.h>
#include "yzyppwrapper.h"

struct YGtkPkgDetailView
{
	YGtkPkgDetailView();
	~YGtkPkgDetailView();

	GtkWidget *getWidget();

	void setSelectable (Ypp::Selectable &sel);
	void setList (Ypp::List list);

	struct Impl;
	Impl *impl;
};

#endif

