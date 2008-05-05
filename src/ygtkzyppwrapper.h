/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Contains ZyppWrapper query results in a GtkTreeModel.
*/

#ifndef YGTK_ZYPP_WRAPPER_H
#define YGTK_ZYPP_WRAPPER_H

#include "yzyppwrapper.h"
#include <gtk/gtktreemodel.h>

#define YGTK_TYPE_ZYPP_MODEL	        (ygtk_zypp_model_get_type ())
#define YGTK_ZYPP_MODEL(obj)	        (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                         YGTK_TYPE_ZYPP_MODEL, YGtkZyppModel))
#define YGTK_ZYPP_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                         YGTK_TYPE_ZYPP_MODEL, YGtkZyppModelClass))
#define YGTK_IS_ZYPP_MODEL(obj)	        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), YGTK_TYPE_ZYPP_MODEL))
#define YGTK_IS_ZYPP_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), YGTK_TYPE_ZYPP_MODEL))
#define YGTK_ZYPP_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                         YGTK_TYPE_ZYPP_MODEL, YGtkZyppModelClass))

struct YGtkZyppModel
{
	GObject parent;

	enum Columns {
		ICON_COLUMN, NAME_COLUMN, NAME_DESCRIPTION_COLUMN,
		PATTERN_DESCRIPTION_COLUMN, PTR_COLUMN, TOTAL_COLUMNS
	};

	Ypp::Pool *pool;

	struct PoolNotify;
	PoolNotify *notify;
};

struct YGtkZyppModelClass
{
	GObjectClass parent_class;
};

YGtkZyppModel *ygtk_zypp_model_new (Ypp::Pool *pool);
GType ygtk_zypp_model_get_type (void) G_GNUC_CONST;

// besides calling Ypp for cleaning, it also cleans the icons of this hook
void ygtk_zypp_model_finish (void);

#endif /*YGTK_ZYPP_WRAPPER_H*/

