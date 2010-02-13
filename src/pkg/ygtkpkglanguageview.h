/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Unlike the pattern view, this one is a simple widget derived from
   the packages one.
*/

#ifndef YGTK_PKG_LANGUAGE_LIST_H
#define YGTK_PKG_LANGUAGE_LIST_H

#include "ygtkpkglistview.h"
#include "ygtkpkgquerywidget.h"

struct YGtkPkgLanguageView : public YGtkPkgListView, YGtkPkgListView::Listener, YGtkPkgQueryWidget
{
	YGtkPkgLanguageView();
	virtual GtkWidget *getWidget();

	virtual bool begsUpdate() { return false; }
	virtual void updateList (Ypp::List list) {}

	virtual void clearSelection() {}
	virtual bool writeQuery (Ypp::PoolQuery &query);

	virtual void selectionChanged();

	struct Impl;
	Impl *impl;
};

#endif

