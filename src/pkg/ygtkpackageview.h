/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* A view for Ypp's query results.
*/

#ifndef YGTK_PACKAGE_VIEW_H
#define YGTK_PACKAGE_VIEW_H

#include "yzyppwrapper.h"
#include <gtk/gtkscrolledwindow.h>

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

enum Property { NAME_PROP, SUMMARY_PROP, NAME_SUMMARY_PROP, VERSION_PROP,
	AVAILABLE_VERSION_PROP, INSTALLED_VERSION_PROP,
	REPOSITORY_PROP, SUPPORT_PROP, SIZE_PROP,
	INSTALLED_CHECK_PROP, STATUS_ICON_PROP, TOTAL_PROPS
};

struct YGtkPackageView
{  // use ygtk_package_view_new() to instance the object
	GtkScrolledWindow parent;

	void setList (Ypp::PkgList list, const char *applyAllLabel);
	void packList (const char *header, Ypp::PkgList list, const char *applyAllLabel);
	void clear();

	void appendTextColumn (const char *header, int property, bool visible, int size, bool identAuto = false);
	void appendCheckColumn (int property);
	void appendButtonColumn (const char *header, int property);
	void appendIconColumn (const char *header, int property);
	void appendEmptyColumn (int size);

	void setRulesHint (bool hint);
	enum Action { NONE_ACTION, INSTALL_ACTION, REMOVE_ACTION, UNDO_ACTION, TOGGLE_ACTION };
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

const char *getRepositoryStockIcon (const std::string &url);

#endif /*YGTK_PACKAGE_VIEW_H*/

