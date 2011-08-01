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

//#define HAS_VESTIGIAL_DIALOG

struct YGtkPkgUndoList;
struct YGtkPkgSearchEntry;
struct YGtkPkgHistoryDialog;
struct YGtkPkgVestigialDialog;

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
	void popupChanges();
	void filterPkgSuffix (const std::string &suffix, bool enable_filter);
	void showRepoManager();

	void showHistoryDialog();
#ifdef HAS_VESTIGIAL_DIALOG
	void showVestigialDialog();
#endif

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

	YGtkPkgHistoryDialog *m_historyDialog;
#ifdef HAS_VESTIGIAL_DIALOG
	YGtkPkgVestigialDialog *m_vestigialDialog;
#endif
};

#endif

