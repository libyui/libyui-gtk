/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Text box information on a given package.
*/

#ifndef YGTK_DETAIL_VIEW_H
#define YGTK_DETAIL_VIEW_H

#include "yzyppwrapper.h"
#include <gtk/gtkscrolledwindow.h>

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

	void setPackage (Ypp::Package *package);
	void setPackages (Ypp::PkgList packages);

	struct Impl;
	Impl *impl;
};

struct YGtkDetailViewClass
{
	GtkScrolledWindowClass parent_class;
};

GtkWidget* ygtk_detail_view_new (gboolean onlineUpdate);
GType ygtk_detail_view_get_type (void) G_GNUC_CONST;

#endif /*YGTK_DETAIL_VIEW_H*/

