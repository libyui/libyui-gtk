/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* A view for Zypp wrapper query results.
*/

#ifndef YGTK_ZYPP_WRAPPER_H
#define YGTK_ZYPP_WRAPPER_H

#include "yzyppwrapper.h"
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtktreemodel.h>

namespace ZyppModel {
	enum Columns {
		// pixbuf
		ICON_COLUMN,
		// text
		NAME_COLUMN, SUMMARY_COLUMN, NAME_SUMMARY_COLUMN, REPOSITORY_COLUMN, SUPPORT_COLUMN,
		INSTALLED_VERSION_COLUMN, AVAILABLE_VERSION_COLUMN,
		// checks
		TO_INSTALL_COLUMN, TO_UPGRADE_COLUMN, TO_REMOVE_COLUMN, TO_MODIFY_COLUMN,
		// internal
		STYLE_COLUMN, WEIGHT_COLUMN, SENSITIVE_COLUMN, CHECK_VISIBLE_COLUMN,
		FOREGROUND_COLUMN, BACKGROUND_COLUMN, XPAD_COLUMN,
		INSTALL_LABEL_COLUMN, REMOVE_LABEL_COLUMN, INSTALL_STOCK_COLUMN, REMOVE_STOCK_COLUMN,
		// misc
		PTR_COLUMN, TOTAL_COLUMNS
	};
};

GtkTreeModel *ygtk_zypp_model_new();
void ygtk_zypp_model_append (GtkTreeModel *model,
	const char *header, Ypp::PkgList list, const char *applyAllLabel);

// calls Ypp::finish() for cleaning, and it cleans our icon resources as well
void ygtk_zypp_finish (void);

#define YGTK_TYPE_PACKAGE_VIEW            (ygtk_package_view_get_type ())
#define YGTK_PACKAGE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	YGTK_TYPE_PACKAGE_VIEW, YGtkPackageView))
#define YGTK_PACKAGE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
	YGTK_TYPE_PACKAGE_VIEW, YGtkPackageViewClass))
#define YGTK_IS_PACKAGE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                              YGTK_TYPE_PACKAGE_VIEW))
#define YGTK_IS_PACKAGE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                              YGTK_TYPE_PACKAGE_VIEW))
#define YGTK_PACKAGE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
	YGTK_TYPE_PACKAGE_VIEW, YGtkPackageViewClass))

struct YGtkPackageView
{  // use ygtk_package_view_new() to instance the object
	GtkScrolledWindow parent;

	void setList (Ypp::PkgList list, const char *applyAllLabel);  // tiny bit optimized
	// pack multiple lists:
	void appendList (const char *header, Ypp::PkgList list, const char *applyAllLabel);
	void clear();

	void appendCheckColumn (int col);
	void appendButtonColumn (const char *header, int col);
	void appendIconColumn (const char *header, int col);
	void appendTextColumn (const char *header, int col, int size = -1, bool identAuto = false);
	void appendEmptyColumn (int size);
	// (set all column headers to NULL in order to hide them.)
	void setRulesHint (bool hint);
	enum Action { NONE_ACTION, INSTALL_ACTION, REMOVE_ACTION, UNDO_ACTION };
	void setActivateAction (Action action);

	struct Listener {
		virtual void packagesSelected (Ypp::PkgList packages) = 0;
	};
	void setListener (Listener *listener);
	Ypp::PkgList getSelected();

	struct Impl;
	Impl *impl;
};

struct YGtkPackageViewClass
{
	GtkScrolledWindowClass parent_class;
};

YGtkPackageView* ygtk_package_view_new (gboolean descriptiveTooltip);
GType ygtk_package_view_get_type (void) G_GNUC_CONST;

#define YGTK_TYPE_DETAIL_VIEW            (ygtk_detail_view_get_type ())
#define YGTK_DETAIL_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	YGTK_TYPE_DETAIL_VIEW, YGtkDetailView))
#define YGTK_DETAIL_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
	YGTK_TYPE_DETAIL_VIEW, YGtkDetailViewClass))
#define YGTK_IS_DETAIL_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                              YGTK_TYPE_DETAIL_VIEW))
#define YGTK_IS_DETAIL_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                              YGTK_TYPE_DETAIL_VIEW))
#define YGTK_DETAIL_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
	YGTK_TYPE_DETAIL_VIEW, YGtkDetailViewClass))

struct YGtkDetailView
{  // use ygtk_detail_view_new() to instance the object
	GtkScrolledWindow parent;

	void setPackages (Ypp::PkgList packages);
	void setPackage (Ypp::Package *package);  // convenience

	struct Impl;
	Impl *impl;
};

struct YGtkDetailViewClass
{
	GtkScrolledWindowClass parent_class;
};

GtkWidget* ygtk_detail_view_new (gboolean onlineUpdate);
GType ygtk_detail_view_get_type (void) G_GNUC_CONST;

#endif /*YGTK_ZYPP_WRAPPER_H*/

