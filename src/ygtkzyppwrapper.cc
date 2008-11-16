/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkZyppWrapper, gtk+ hooks for zypp wrapper */
// check the header file for information about these hooks

#include <config.h>
#include <gtk/gtk.h>
#include "ygtkzyppwrapper.h"
#include "YGUtils.h"

static GdkPixbuf *loadPixbuf (const char *icon)
{ return YGUtils::loadPixbuf (std::string (DATADIR) + "/" + icon); }

// bridge as we don't want to mix c++ class polymorphism and gobject
static void ygtk_zypp_model_entry_changed (YGtkZyppModel *model, Ypp::Pool::Iter iter);
static void ygtk_zypp_model_entry_inserted (YGtkZyppModel *model, Ypp::Pool::Iter iter);
static void ygtk_zypp_model_entry_deleted (YGtkZyppModel *model, Ypp::Pool::Iter iter);
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
	          *to_install, *to_install_upgrade, *to_remove, *to_auto_install,
	          *to_auto_remove;
	PackageIcons() {
		installed = loadPixbuf ("pkg-installed.png");
		installed_upgradable = loadPixbuf ("pkg-installed-upgradable.png");
		installed_locked = loadPixbuf ("pkg-installed-locked.png");
		installed_upgradable_locked = loadPixbuf ("pkg-installed-upgradable-locked.png");
		available = loadPixbuf ("pkg-available.png");
		available_locked = loadPixbuf ("pkg-available-locked.png");
		to_install = loadPixbuf ("pkg-install.png");
		to_install_upgrade = loadPixbuf ("pkg-upgrade.png");
		to_remove = loadPixbuf ("pkg-remove.png");
		to_auto_install = loadPixbuf ("pkg-install-auto.png");
		to_auto_remove = loadPixbuf ("pkg-remove-auto.png");
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
		g_object_unref (G_OBJECT (to_install_upgrade));
		g_object_unref (G_OBJECT (to_auto_install));
		g_object_unref (G_OBJECT (to_auto_remove));
	}
};

static PackageIcons *icons = NULL;
void ygtk_zypp_model_finish (void)
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

static GtkTreeModelFlags ygtk_zypp_model_get_flags (GtkTreeModel *model)
{
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (model);
	int flags = GTK_TREE_MODEL_ITERS_PERSIST;
	if (zmodel->pool->isPlainList())
		flags |= (GtkTreeModelFlags) GTK_TREE_MODEL_LIST_ONLY;
	return (GtkTreeModelFlags) flags;
}

static gboolean ygtk_zypp_model_get_iter (GtkTreeModel *model, GtkTreeIter *iter,
                                          GtkTreePath  *path)
{
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (model);
	gint *indices = gtk_tree_path_get_indices (path);
	gint depth = gtk_tree_path_get_depth (path);

	Ypp::Pool::Path zpath;	
	for (gint i = 0; i < depth; i++)
		zpath.push_back (indices[i]);

	iter->user_data = (gpointer) zmodel->pool->fromPath (zpath);
	return iter->user_data != NULL;
}

static GtkTreePath *ygtk_zypp_model_get_path (GtkTreeModel *model, GtkTreeIter *iter)
{
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (model);
	Ypp::Pool::Path zpath = zmodel->pool->toPath (iter->user_data);

	GtkTreePath *path = gtk_tree_path_new();
	for (Ypp::Pool::Path::const_iterator it = zpath.begin(); it != zpath.end(); it++)
		gtk_tree_path_append_index (path, *it);
	return path;
}

static gboolean ygtk_zypp_model_iter_next (GtkTreeModel *model, GtkTreeIter *iter)
{
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (model);
	iter->user_data = zmodel->pool->getNext (iter->user_data);
	return iter->user_data != NULL;
}

static gboolean ygtk_zypp_model_iter_parent (GtkTreeModel *model, GtkTreeIter *iter,
                                             GtkTreeIter  *child)
{
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (model);
	iter->user_data = zmodel->pool->getParent (child->user_data);
	return iter->user_data != NULL;
}

static gboolean ygtk_zypp_model_iter_has_child (GtkTreeModel *model, GtkTreeIter *iter)
{
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (model);
	return zmodel->pool->getChild (iter->user_data) != NULL;
}

static gint ygtk_zypp_model_iter_n_children (GtkTreeModel *model, GtkTreeIter *iter)
{
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (model);
	Ypp::Pool::Iter ziter;
	if (iter == NULL)  /* root */
		ziter = zmodel->pool->getFirst();
	else {
		ziter = (Ypp::Pool::Iter) iter->user_data;
		ziter = zmodel->pool->getChild (ziter);
	}
	if (!ziter)
		return 0;
	int children = 1;
	while ((ziter = zmodel->pool->getNext (ziter)))
		children++;
	return children;
}

static gboolean ygtk_zypp_model_iter_nth_child (GtkTreeModel *model, GtkTreeIter *iter,
                                                GtkTreeIter  *parent, gint        n)
{
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (model);
	Ypp::Pool::Iter ziter;
	if (parent == NULL)  /* root */
		ziter = zmodel->pool->getFirst();
	else
		ziter = zmodel->pool->getChild (parent->user_data);
	if (!ziter)
		return FALSE;
	for (int c = 0; c < n; c++)
		ziter = zmodel->pool->getNext (ziter);
	iter->user_data = ziter;
	return iter->user_data != NULL;
}

static gboolean ygtk_zypp_model_iter_children (GtkTreeModel *model, GtkTreeIter *iter,
                                               GtkTreeIter  *parent)
{
	return ygtk_zypp_model_iter_nth_child (model, iter, parent, 0);
}

void ygtk_zypp_model_entry_changed (YGtkZyppModel *model, Ypp::Pool::Iter ziter)
{
	GtkTreeIter iter;
	iter.user_data = ziter;
	GtkTreePath *path = ygtk_zypp_model_get_path (GTK_TREE_MODEL (model), &iter);
	gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);
	gtk_tree_path_free (path);
}

void ygtk_zypp_model_entry_inserted (YGtkZyppModel *model, Ypp::Pool::Iter it)
{
	GtkTreeIter iter;
	iter.user_data = it;
	GtkTreePath *path = ygtk_zypp_model_get_path (GTK_TREE_MODEL (model), &iter);
	gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);
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
{ return YGtkZyppModel::TOTAL_COLUMNS; }

static GType ygtk_zypp_model_get_column_type (GtkTreeModel *tree_model, gint column)
{
	switch (column) {
		case YGtkZyppModel::ICON_COLUMN:
			return GDK_TYPE_PIXBUF;
		case YGtkZyppModel::NAME_COLUMN:
		case YGtkZyppModel::NAME_DESCRIPTION_COLUMN:
			return G_TYPE_STRING;
		case YGtkZyppModel::PTR_COLUMN:
			return G_TYPE_POINTER;
	}
	return 0;
}

static void ygtk_zypp_model_get_value (GtkTreeModel *model, GtkTreeIter *iter,
                                       gint column, GValue *value)
{
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (model);
	Ypp::Pool::Iter pool_iter = iter->user_data;
	Ypp::Package *package = zmodel->pool->get (pool_iter);

	g_value_init (value, ygtk_zypp_model_get_column_type (model, column));

	if (!package) {
		if (column == YGtkZyppModel::NAME_COLUMN ||
		    column == YGtkZyppModel::NAME_DESCRIPTION_COLUMN) {
			std::string name = zmodel->pool->getName (iter->user_data);
			if (column == YGtkZyppModel::NAME_DESCRIPTION_COLUMN)
				name = "<b>" + name + "</b>";
			g_value_set_string (value, g_strdup (name.c_str()));
		}
		return;
	}

	switch (column)
	{
		case YGtkZyppModel::ICON_COLUMN:
		{
			GdkPixbuf *pixbuf = 0;
			switch (package->type()) {
				case Ypp::Package::PATTERN_TYPE: {
					std::string filename (package->icon());
					GtkIconTheme *icons = gtk_icon_theme_get_default();
					pixbuf = gtk_icon_theme_load_icon (icons,
						filename.c_str(), 32, GtkIconLookupFlags (0), NULL);
					break;
				}
				default:
					break;
			}
			if (pixbuf) {
				if (!package->isInstalled()) {
					GdkPixbuf *_pixbuf = pixbuf;
					pixbuf = YGUtils::setOpacity (_pixbuf, 50, true);
					g_object_unref (_pixbuf);
				}
			}
			else {
				bool locked = package->isLocked();
				bool auto_ = package->isAuto();
				if (package->toInstall()) {
					if (auto_)
						pixbuf = icons->to_auto_install;
					else {
						pixbuf = icons->to_install;
						if (package->isInstalled())
							pixbuf = icons->to_install_upgrade;
					}
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
			}
			g_value_set_object (value, (GObject *) pixbuf);
			break;
		}
		case YGtkZyppModel::NAME_COLUMN:
		{
			std::string name = YGUtils::truncate (package->name(), 15, 1);
			g_value_set_string (value, g_strdup (name.c_str()));
			break;
		}
		case YGtkZyppModel::NAME_DESCRIPTION_COLUMN:
		{
			bool highlight = zmodel->pool->highlight (pool_iter);
			std::string str = package->name();
/*			if (highlight)
				str = "<span color=\"red\">" + str + "</span>";*/
			std::string summary = package->summary();
			if (!summary.empty()) {
				YGUtils::escapeMarkup (summary);
				str += "\n<small>" + summary + "</small>";
			}
			if (highlight)
				str = "<b>" + str + "</b>";
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
		case YGtkZyppModel::TOTAL_COLUMNS:
			break;
	}
}

YGtkZyppModel *ygtk_zypp_model_new (Ypp::Pool *pool)
{
	YGtkZyppModel *zmodel = (YGtkZyppModel *) g_object_new (YGTK_TYPE_ZYPP_MODEL, NULL);
	zmodel->notify = new YGtkZyppModel::PoolNotify();
	zmodel->notify->model = zmodel;
	zmodel->pool = pool;
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

