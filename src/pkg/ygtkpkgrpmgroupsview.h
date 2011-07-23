/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Good-old RPM-groups tree-view.
*/

#ifndef YGTK_PKG_RPM_GROUP_VIEW_H
#define YGTK_PKG_RPM_GROUP_VIEW_H

#include "ygtkpkgquerywidget.h"
#include <gtk/gtk.h>

struct YGtkPkgRpmGroupsView : public YGtkPkgQueryWidget
{
	YGtkPkgRpmGroupsView();
	virtual ~YGtkPkgRpmGroupsView();
	virtual GtkWidget *getWidget();

	virtual bool begsUpdate() { return false; }
	virtual void updateList (Ypp::List list) {}

	virtual void clearSelection();
	virtual bool writeQuery (Ypp::PoolQuery &query);

	struct Impl;
	Impl *impl;
};

#endif

