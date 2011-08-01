/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Zypp search query interface..
*/

#ifndef YGTK_PKG_SEARCH_ENTRY_H
#define YGTK_PKG_SEARCH_ENTRY_H

#include "ygtkpkgquerywidget.h"
#include "ygtkpkglistview.h"

struct YGtkPkgSearchEntry : public YGtkPkgQueryWidget
{
	YGtkPkgSearchEntry();
	virtual ~YGtkPkgSearchEntry();

	virtual GtkWidget *getWidget();

	virtual bool begsUpdate() { return false; }
	virtual void updateList (Ypp::List list) {}

	virtual void clearSelection();
	virtual bool writeQuery (Ypp::PoolQuery &query);

	virtual GtkWidget *createToolbox();

	void setText (Ypp::PoolQuery::StringAttribute attribute, const std::string &text);
	void setActivateWidget (GtkWidget *widget);

	Ypp::PoolQuery::StringAttribute getAttribute();
	std::list <std::string> getText();
	std::string getTextStr();

	struct Impl;
	Impl *impl;
};

#endif

