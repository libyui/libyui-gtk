/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YPackageSelector is implemented as a singleton, allowing
   intercommunication of the widgets that compound it.
*/

#ifndef YGTK_PACKAGE_SELECTOR_H
#define YGTK_PACKAGE_SELECTOR_H

#include <YPackageSelector.h>
#include "YGWidget.h"
#include "yzyppwrapper.h"

struct YGtkPkgUndoList;
struct YGtkPkgSearchEntry;

class YGPackageSelector : public YPackageSelector, YGWidget
{
public:
	YGPackageSelector (YWidget *parent, long mode);
	virtual ~YGPackageSelector();

	static YGPackageSelector *get() { return singleton; }

	void apply();
	void cancel();

	void showFilterWidget (const char *filter);
	void searchFor (Ypp::PoolQuery::StringAttribute attrb, const std::string &text);
	void showSelectableDetails (Ypp::Selectable &sel);
	void popupChanges();
	void filterPkgSuffix (const std::string &suffix, bool enable);

	YGtkPkgUndoList *undoList();
	YGtkPkgSearchEntry *getSearchEntry();

	// let the UI function while processing something; returns 'true' if the
	// function was called again while the UI was 'breathing' -- in which case
	// you probably want to return.
	bool yield();

	YGWIDGET_IMPL_COMMON (YPackageSelector)

	struct Impl;
	Impl *impl;
private:
	static YGPackageSelector *singleton;
};

#endif

