/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Pattern and language list boxes.
*/

#ifndef YGTK_PKG_PATTERN_LIST_H
#define YGTK_PKG_PATTERN_LIST_H

#include "ygtkpkgquerywidget.h"
#include <gtk/gtk.h>

struct YGtkPkgPatternView : public YGtkPkgQueryWidget
{
	YGtkPkgPatternView (Ypp::Selectable::Type type);
	virtual ~YGtkPkgPatternView();
	virtual GtkWidget *getWidget();

	virtual bool begsUpdate() { return false; }
	virtual void updateList (Ypp::List list) {}

	virtual void clearSelection();
	virtual bool writeQuery (Ypp::PoolQuery &query);

	struct Impl;
	Impl *impl;
};

bool isPatternsPoolEmpty();

#endif

