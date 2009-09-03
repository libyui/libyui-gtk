/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* A view for Zypp wrapper query results.
*/

#ifndef YGTK_ZYPP_WRAPPER_H
#define YGTK_ZYPP_WRAPPER_H

#include "yzyppwrapper.h"

namespace ZyppModel {
	enum Columns {
		// pixbuf
		ICON_COLUMN,
		// text
		NAME_COLUMN, SUMMARY_COLUMN, NAME_SUMMARY_COLUMN, REPOSITORY_COLUMN, SUPPORT_COLUMN,
		INSTALLED_VERSION_COLUMN, AVAILABLE_VERSION_COLUMN,
		// checks
		TO_INSTALL_COLUMN, TO_UPGRADE_COLUMN, NOT_TO_REMOVE_COLUMN, TO_MODIFY_COLUMN,
		// internal
		STYLE_COLUMN, WEIGHT_COLUMN, SENSITIVE_COLUMN, CHECK_VISIBLE_COLUMN,
		FOREGROUND_COLUMN, BACKGROUND_COLUMN,
		// misc
		PTR_COLUMN, TOTAL_COLUMNS
	};
};

GtkTreeModel *ygtk_zypp_model_new();
void ygtk_zypp_model_append (GtkTreeModel *model,
	const char *header, Ypp::PkgList list, const char *applyAllLabel);

struct PackagesView
{
	PackagesView (bool descriptiveTooltip);
	~PackagesView();
	GtkWidget *getWidget();

	void setRows (Ypp::PkgList list, const char *applyAllLabel);  // tiny bit optimized
	// pack multiple lists:
	void appendRows (const char *header, Ypp::PkgList list, const char *applyAllLabel);
	void clear();

	void appendCheckColumn (int col);
	void appendIconColumn (const char *header, int col);
	void appendTextColumn (const char *header, int col, int size = -1);
	// (set all column headers to NULL in order to hide them.)

	void setFrame (bool show);

	struct Listener {
		virtual void packagesSelected (Ypp::PkgList packages) = 0;
	};
	void setListener (Listener *listener);
	Ypp::PkgList getSelected();

	struct Impl;
	Impl *impl;
};

struct PackageDetails
{
	PackageDetails (bool onlineUpdate);
	~PackageDetails();
	GtkWidget *getWidget();

	void setPackages (Ypp::PkgList packages);
	void setPackage (Ypp::Package *package);  // convenience

	struct Impl;
	Impl *impl;
};

// calls Ypp::finish() for cleaning, and it cleans our stuff as well
void ygtk_zypp_finish (void);

#endif /*YGTK_ZYPP_WRAPPER_H*/

