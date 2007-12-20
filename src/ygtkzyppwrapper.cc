/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkZyppWrapper, gtk+ hooks for zypp wrapper */
// check the header file for information about these hooks

#include <config.h>
#include "ygtkzyppwrapper.h"
#include "YGUtils.h"

#include "icons/pkg-installed.xpm"
#include "icons/pkg-installed-upgradable.xpm"
#include "icons/pkg-installed-upgradable-locked.xpm"
#include "icons/pkg-installed-locked.xpm"
#include "icons/pkg-available.xpm"
#include "icons/pkg-available-locked.xpm"
#include "icons/pkg-install.xpm"
#include "icons/pkg-remove.xpm"
#include "icons/pkg-install-auto.xpm"
#include "icons/pkg-remove-auto.xpm"
#include "icons/pkg-3D.xpm"

// bridge as we don't want to mix c++ class polymorphism and gobject
struct YGtkZyppModel::PoolNotify : public Ypp::Pool::Listener {
YGtkZyppModel *model;
	virtual void entryChanged  (Ypp::Pool::Iter iter, Ypp::Package *package)
	{ ygtk_zypp_model_entry_changed (model, iter); }
	virtual void entryInserted (Ypp::Pool::Iter iter, Ypp::Package *package)
	{ ygtk_zypp_model_entry_inserted (model, iter); }
	virtual void entryDeleted  (Ypp::Pool::Iter iter, Ypp::Package *package)
	{ ygtk_zypp_model_entry_deleted (model, iter); }
};

// Icons resources
struct PackageIcons {
	GdkPixbuf *installed, *installed_upgradable, *installed_locked,
	          *installed_upgradable_locked, *available, *available_locked,
	          *to_install, *to_remove, *to_auto_install, *to_auto_remove,
	          *is_3D;
	PackageIcons() {
		installed = gdk_pixbuf_new_from_xpm_data (pkg_installed_xpm);
		installed_upgradable =
			gdk_pixbuf_new_from_xpm_data (pkg_installed_upgradable_xpm);
		installed_locked = gdk_pixbuf_new_from_xpm_data (pkg_installed_locked_xpm);
		installed_upgradable_locked =
			gdk_pixbuf_new_from_xpm_data (pkg_installed_upgradable_locked_xpm);
		available = gdk_pixbuf_new_from_xpm_data (pkg_available_xpm);
		available_locked = gdk_pixbuf_new_from_xpm_data (pkg_available_locked_xpm);
		to_install = gdk_pixbuf_new_from_xpm_data (pkg_install_xpm);
		to_remove = gdk_pixbuf_new_from_xpm_data (pkg_remove_xpm);
		to_auto_install = gdk_pixbuf_new_from_xpm_data (pkg_install_auto_xpm);
		to_auto_remove = gdk_pixbuf_new_from_xpm_data (pkg_remove_auto_xpm);
		is_3D = gdk_pixbuf_new_from_xpm_data (pkg_3D_xpm);
	}
	~PackageIcons() {
		g_object_unref (G_OBJECT (installed));
		g_object_unref (G_OBJECT (installed_upgradable));
		g_object_unref (G_OBJECT (installed_upgradable_locked));
		g_object_unref (G_OBJECT (installed_locked));
		g_object_unref (G_OBJECT (available));
		g_object_unref (G_OBJECT (available_locked));
		g_object_unref (G_OBJECT (to_install));
		g_object_unref (G_OBJECT (to_remove));
		g_object_unref (G_OBJECT (to_auto_install));
		g_object_unref (G_OBJECT (to_auto_remove));
		g_object_unref (G_OBJECT (is_3D));
	}
};

static PackageIcons *icons = NULL;
void ygtk_zypp_model_finish()
{
	delete icons; icons = NULL;
	Ypp::finish();
}

static void ygtk_zypp_model_tree_model_init (GtkTreeModelIface *iface);

G_DEFINE_TYPE_WITH_CODE (YGtkZyppModel, ygtk_zypp_model, G_TYPE_OBJECT,
	G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL, ygtk_zypp_model_tree_model_init))

static void ygtk_zypp_model_init (YGtkZyppModel *zmodel)
{
	if (!icons)
		icons = new PackageIcons();
}

static void ygtk_zypp_model_finalize (GObject *object)
{
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (object);
	delete zmodel->notify;
	zmodel->notify = NULL;
	delete zmodel->pool;
	zmodel->pool = NULL;
	G_OBJECT_CLASS (ygtk_zypp_model_parent_class)->finalize (object);
}

static GtkTreeModelFlags ygtk_zypp_model_get_flags (GtkTreeModel *)
{
  return GtkTreeModelFlags (GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY);
}

static gboolean ygtk_zypp_model_get_iter (GtkTreeModel *model, GtkTreeIter *iter,
                                          GtkTreePath  *path)
{
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (model);
	gint row = gtk_tree_path_get_indices (path)[0];

	iter->user_data = (gpointer) zmodel->pool->getIter (row);
	return iter->user_data != NULL;
}

static GtkTreePath *ygtk_zypp_model_get_path (GtkTreeModel *model, GtkTreeIter *iter)
{
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (model);
	int row = zmodel->pool->getIndex (iter->user_data);
fprintf (stderr, "get index: %d\n", row);

	GtkTreePath *path = gtk_tree_path_new();
	gtk_tree_path_append_index (path, row);
	return path;
}

static gboolean ygtk_zypp_model_iter_next (GtkTreeModel *model, GtkTreeIter *iter)
{
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (model);
	iter->user_data = zmodel->pool->getNext (iter->user_data);
	return iter->user_data != NULL;
}

void ygtk_zypp_model_entry_changed (YGtkZyppModel *model, Ypp::Pool::Iter it)
{
	GtkTreeIter iter;
	iter.user_data = it;
	GtkTreePath *path = ygtk_zypp_model_get_path (GTK_TREE_MODEL (model), &iter);
	gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);
	gtk_tree_path_free (path);
}

void ygtk_zypp_model_entry_inserted (YGtkZyppModel *model, Ypp::Pool::Iter it)
{
fprintf (stderr, "inserted row\n");
	GtkTreeIter iter;
	iter.user_data = it;
	GtkTreePath *path = ygtk_zypp_model_get_path (GTK_TREE_MODEL (model), &iter);
fprintf (stderr, "signal\n");
	gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);
fprintf (stderr, "done\n");
	gtk_tree_path_free (path);
}

void ygtk_zypp_model_entry_deleted (YGtkZyppModel *model, Ypp::Pool::Iter it)
{
	GtkTreeIter iter;
	iter.user_data = it;
	GtkTreePath *path = ygtk_zypp_model_get_path (GTK_TREE_MODEL (model), &iter);
	gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);
	gtk_tree_path_free (path);
}

static gint ygtk_zypp_model_get_n_columns (GtkTreeModel *model)
{
	return YGtkZyppModel::TOTAL_COLUMNS;
}

static GType ygtk_zypp_model_get_column_type (GtkTreeModel *tree_model, gint column)
{
	switch (column)
	{
		case YGtkZyppModel::ICON_COLUMN:
			return GDK_TYPE_PIXBUF;
		case YGtkZyppModel::NAME_COLUMN:
		case YGtkZyppModel::NAME_DESCRIPTION_COLUMN:
			return G_TYPE_STRING;
		case YGtkZyppModel::PTR_COLUMN:
			return G_TYPE_POINTER;
		case YGtkZyppModel::SPECIAL_ICON_COLUMN:
			return GDK_TYPE_PIXBUF;
	}
	return 0;
}

static void ygtk_zypp_model_get_value (GtkTreeModel *model, GtkTreeIter *iter,
                                       gint column, GValue *value)
{
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (model);
	Ypp::Package *package = zmodel->pool->get (iter->user_data);

	g_value_init (value, ygtk_zypp_model_get_column_type (model, column));
	switch (column)
	{
		case YGtkZyppModel::ICON_COLUMN:
		{
			GdkPixbuf *pixbuf;
			bool locked = package->isLocked();
			bool auto_ = package->isAuto();
			if (package->toInstall()) {
				if (auto_)
					pixbuf = icons->to_auto_install;
				else
					pixbuf = icons->to_install;
			}
			else if (package->toRemove()) {
				if (auto_)
					pixbuf = icons->to_auto_remove;
				else
					pixbuf = icons->to_remove;
			}
			else if (package->hasUpgrade()) {
				if (locked)
					pixbuf = icons->installed_upgradable_locked;
				else
					pixbuf = icons->installed_upgradable;
			}
			else if (package->isInstalled()) {
				if (locked)
					pixbuf = icons->installed_locked;
				else
					pixbuf = icons->installed;
			}
			else {
				if (locked)
					pixbuf = icons->available_locked;
				else
					pixbuf = icons->available;
			}
			g_value_set_object (value, (GObject *) pixbuf);
			break;
		}
		case YGtkZyppModel::NAME_COLUMN:
		{
			std::string name = package->name();
			const unsigned int max_width = 15;
			if (name.size() > max_width) {
				name.erase (max_width-3);
				name += "...";
			}
			g_value_set_string (value, g_strdup (name.c_str()));
			break;
		}
		case YGtkZyppModel::NAME_DESCRIPTION_COLUMN:
		{
			std::string str = package->name();
			std::string summary = package->summary();
			if (!summary.empty()) {
				YGUtils::escapeMarkup (summary);
				str += "\n<small>" + summary + "</small>";
			}
			g_value_set_string (value, g_strdup (str.c_str()));
			break;
		}
		case YGtkZyppModel::PTR_COLUMN:
		{
			void *ptr;
			ptr = (void *) package;
			g_value_set_pointer (value, ptr);
			break;
		}
		case YGtkZyppModel::SPECIAL_ICON_COLUMN:
		{
			if (package->is3D())
				g_value_set_object (value, (GObject *) icons->is_3D);
			else
				g_value_set_object (value, NULL);
			break;
		}
		default:
			g_warning ("YGtkZyppModel column %d doesn't exist.", column);
			break;
	}
}

static gboolean ygtk_zypp_model_iter_children (GtkTreeModel *model, GtkTreeIter *iter,
                                              GtkTreeIter  *parent)
{
	return FALSE;
}

static gboolean ygtk_zypp_model_iter_has_child (GtkTreeModel *model, GtkTreeIter *iter)
{
  return FALSE;
}

static gint ygtk_zypp_model_iter_n_children (GtkTreeModel *model, GtkTreeIter *iter)
{
	return 0;
}

static gboolean ygtk_zypp_model_iter_nth_child (GtkTreeModel *model, GtkTreeIter *iter,
                                                GtkTreeIter  *parent, gint        n)
{
	return FALSE;
}

static gboolean ygtk_zypp_model_iter_parent (GtkTreeModel *model, GtkTreeIter *iter,
                                             GtkTreeIter  *child)
{
  return FALSE;
}

YGtkZyppModel *ygtk_zypp_model_new (Ypp::Query *query)
{
	YGtkZyppModel *zmodel = (YGtkZyppModel *) g_object_new (YGTK_TYPE_ZYPP_MODEL, NULL);
	zmodel->notify = new YGtkZyppModel::PoolNotify();
	zmodel->notify->model = zmodel;
	zmodel->pool = new Ypp::Pool (query);
	zmodel->pool->setListener (zmodel->notify);
	return zmodel;
}

static void ygtk_zypp_model_class_init (YGtkZyppModelClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = ygtk_zypp_model_finalize;
}

static void ygtk_zypp_model_tree_model_init (GtkTreeModelIface *iface)
{
	iface->get_flags = ygtk_zypp_model_get_flags;
	iface->get_n_columns = ygtk_zypp_model_get_n_columns;
	iface->get_column_type = ygtk_zypp_model_get_column_type;
	iface->get_iter = ygtk_zypp_model_get_iter;
	iface->get_path = ygtk_zypp_model_get_path;
	iface->get_value = ygtk_zypp_model_get_value;
	iface->iter_next = ygtk_zypp_model_iter_next;
	iface->iter_children = ygtk_zypp_model_iter_children;
	iface->iter_has_child = ygtk_zypp_model_iter_has_child;
	iface->iter_n_children = ygtk_zypp_model_iter_n_children;
	iface->iter_nth_child = ygtk_zypp_model_iter_nth_child;
	iface->iter_parent = ygtk_zypp_model_iter_parent;
}

