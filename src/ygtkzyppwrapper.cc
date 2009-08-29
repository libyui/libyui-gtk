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
static void ygtk_zypp_model_entry_changed (YGtkZyppModel *model, Ypp::PkgList::Iter iter);
static void ygtk_zypp_model_entry_inserted (YGtkZyppModel *model, Ypp::PkgList::Iter iter);
static void ygtk_zypp_model_entry_deleted (YGtkZyppModel *model, Ypp::PkgList::Iter iter);
struct YGtkZyppModel::PkgListNotify : public Ypp::PkgList::Listener {
YGtkZyppModel *model;
	virtual void entryChanged  (Ypp::PkgList::Iter iter, Ypp::Package *package)
	{ ygtk_zypp_model_entry_changed (model, iter); }
	virtual void entryInserted (Ypp::PkgList::Iter iter, Ypp::Package *package)
	{ ygtk_zypp_model_entry_inserted (model, iter); }
	virtual void entryDeleted  (Ypp::PkgList::Iter iter, Ypp::Package *package)
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
	delete zmodel->list;
	zmodel->list = NULL;
	G_OBJECT_CLASS (ygtk_zypp_model_parent_class)->finalize (object);
}

static GtkTreeModelFlags ygtk_zypp_model_get_flags (GtkTreeModel *model)
{ return (GtkTreeModelFlags) (GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY); }

static gboolean ygtk_zypp_model_get_iter (GtkTreeModel *model, GtkTreeIter *iter,
                                          GtkTreePath  *path)
{  // from Path to Iter
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (model);
	gint indice = gtk_tree_path_get_indices (path)[0];

	Ypp::PkgList::Iter ziter = zmodel->list->first();
	for (int r = 0; r < indice; r++)
		ziter = zmodel->list->next (ziter);

	iter->user_data = (gpointer) ziter;
	return iter->user_data != NULL;
}

static GtkTreePath *ygtk_zypp_model_get_path (GtkTreeModel *model, GtkTreeIter *iter)
{  // from Iter to Path
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (model);

	Ypp::PkgList::Iter ziter = (Ypp::PkgList::Iter) iter->user_data;
	GtkTreePath *path = gtk_tree_path_new();
	int index = 0;
	for (Ypp::PkgList::Iter it = zmodel->list->first(); it && it != ziter;
	     it = zmodel->list->next (it))
		index++;
	gtk_tree_path_append_index (path, index);
	return path;
}

static gboolean ygtk_zypp_model_iter_next (GtkTreeModel *model, GtkTreeIter *iter)
{
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (model);
	iter->user_data = zmodel->list->next (iter->user_data);
	return iter->user_data != NULL;
}

static gboolean ygtk_zypp_model_iter_parent (GtkTreeModel *, GtkTreeIter *, GtkTreeIter *)
{ return FALSE; }

static gboolean ygtk_zypp_model_iter_has_child (GtkTreeModel *, GtkTreeIter *)
{ return FALSE; }

static gint ygtk_zypp_model_iter_n_children (GtkTreeModel *model, GtkTreeIter *iter)
{ return 0; }

static gboolean ygtk_zypp_model_iter_nth_child (GtkTreeModel *model, GtkTreeIter *iter,
                                                GtkTreeIter  *parent, gint        n)
{
	if (parent) return FALSE;
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (model);
	Ypp::PkgList::Iter ziter = zmodel->list->first();
	for (int c = 0; c < n; c++)
		ziter = zmodel->list->next (ziter);
	iter->user_data = ziter;
	return iter->user_data != NULL;
}

static gboolean ygtk_zypp_model_iter_children (
	GtkTreeModel *model, GtkTreeIter *iter, GtkTreeIter  *parent)
{ return ygtk_zypp_model_iter_nth_child (model, iter, parent, 0); }

void ygtk_zypp_model_entry_changed (YGtkZyppModel *model, Ypp::PkgList::Iter ziter)
{
	GtkTreeIter iter;
	iter.user_data = ziter;
	GtkTreePath *path = ygtk_zypp_model_get_path (GTK_TREE_MODEL (model), &iter);
	gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);
	gtk_tree_path_free (path);
}

void ygtk_zypp_model_entry_inserted (YGtkZyppModel *model, Ypp::PkgList::Iter it)
{
	GtkTreeIter iter;
	iter.user_data = it;
	GtkTreePath *path = ygtk_zypp_model_get_path (GTK_TREE_MODEL (model), &iter);
	gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);
	gtk_tree_path_free (path);
}

void ygtk_zypp_model_entry_deleted (YGtkZyppModel *model, Ypp::PkgList::Iter it)
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
		case YGtkZyppModel::SUMMARY_COLUMN:
		case YGtkZyppModel::NAME_SUMMARY_COLUMN:
		case YGtkZyppModel::REPOSITORY_COLUMN:
		case YGtkZyppModel::SUPPORT_COLUMN:
		case YGtkZyppModel::INSTALLED_VERSION_COLUMN:
		case YGtkZyppModel::AVAILABLE_VERSION_COLUMN:
			return G_TYPE_STRING;
		case YGtkZyppModel::TO_INSTALL_COLUMN:
		case YGtkZyppModel::TO_UPGRADE_COLUMN:
		case YGtkZyppModel::NOT_TO_REMOVE_COLUMN:
		case YGtkZyppModel::TO_MODIFY_COLUMN:
		case YGtkZyppModel::IS_UNLOCKED_COLUMN:
			return G_TYPE_BOOLEAN;
		case YGtkZyppModel::IS_MODIFIED_ITALIC_COLUMN:
			return G_TYPE_INT;
		case YGtkZyppModel::PTR_COLUMN:
			return G_TYPE_POINTER;
	}
	return 0;
}

static void ygtk_zypp_model_get_value (GtkTreeModel *model, GtkTreeIter *iter,
                                       gint column, GValue *value)
{
	YGtkZyppModel *zmodel = YGTK_ZYPP_MODEL (model);
	Ypp::PkgList::Iter ziter = iter->user_data;
	Ypp::Package *package = zmodel->list->get (ziter);

	g_value_init (value, ygtk_zypp_model_get_column_type (model, column));

/*  // FIXME:
	if (!package) {
		if (column == YGtkZyppModel::NAME_COLUMN ||
		    column == YGtkZyppModel::NAME_SUMMARY_COLUMN) {
			std::string name = zmodel->list->getName (iter->user_data);
			if (column == YGtkZyppModel::NAME_SUMMARY_COLUMN)
				name = "<big><b><span color=\"darkgray\">" + name + "</span></b></big>";
			g_value_set_string (value, g_strdup (name.c_str()));
		}
		return;
	}
*/

	switch (column) {
		case YGtkZyppModel::ICON_COLUMN: {
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
		case YGtkZyppModel::NAME_COLUMN: {
			bool highlight = zmodel->list->highlight (ziter);
			std::string str (package->name());
			if (highlight)
				str = "<b>" + str + "</b>";
			g_value_set_string (value, g_strdup (str.c_str()));
			break;
		}
		case YGtkZyppModel::SUMMARY_COLUMN: {
			std::string str (package->summary());
			g_value_set_string (value, g_strdup (str.c_str()));
			break;
		}
		case YGtkZyppModel::NAME_SUMMARY_COLUMN: {
			bool highlight = zmodel->list->highlight (ziter);
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
		case YGtkZyppModel::REPOSITORY_COLUMN: {
			const Ypp::Package::Version *version = 0;
			if (package->toInstall (&version)) ;
			if (!version)
				version = package->getInstalledVersion();
			std::string repo;
			if (version && version->repo)
				repo = version->repo->name;
			g_value_set_string (value, g_strdup (repo.c_str()));
			break;
		}
		case YGtkZyppModel::SUPPORT_COLUMN:
			g_value_set_string (value, g_strdup (package->support (true).c_str()));
			break;
		case YGtkZyppModel::INSTALLED_VERSION_COLUMN: {
			const Ypp::Package::Version *version = package->getInstalledVersion();
			std::string ver = version ? version->number : "";
			g_value_set_string (value, g_strdup (ver.c_str()));
			break;
		}
		case YGtkZyppModel::AVAILABLE_VERSION_COLUMN: {
			const Ypp::Package::Version *version = package->getAvailableVersion (0);
			std::string ver = version ? version->number : "";
			g_value_set_string (value, g_strdup (ver.c_str()));
			break;
		}
		case YGtkZyppModel::TO_INSTALL_COLUMN:
			g_value_set_boolean (value, package->toInstall());
			break;
		case YGtkZyppModel::TO_UPGRADE_COLUMN: {
			const Ypp::Package::Version *version = 0;
			if (package->toInstall (&version))
				g_value_set_boolean (value, version->cmp > 0);
			break;
		}
		case YGtkZyppModel::NOT_TO_REMOVE_COLUMN:
			g_value_set_boolean (value, !package->toRemove());
			break;
		case YGtkZyppModel::TO_MODIFY_COLUMN:
			g_value_set_boolean (value, package->toModify());
			break;
		case YGtkZyppModel::IS_UNLOCKED_COLUMN:
			g_value_set_boolean (value, !package->isLocked());
			break;
		case YGtkZyppModel::IS_MODIFIED_ITALIC_COLUMN: {
			int modified = package->toModify() ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL;
			g_value_set_int (value, modified);
			break;
		}
		case YGtkZyppModel::PTR_COLUMN:
			g_value_set_pointer (value, (void *) package);
			break;
		case YGtkZyppModel::TOTAL_COLUMNS:
			break;
	}
}

YGtkZyppModel *ygtk_zypp_model_new (const Ypp::PkgList list)
{
	YGtkZyppModel *zmodel = (YGtkZyppModel *) g_object_new (YGTK_TYPE_ZYPP_MODEL, NULL);
	zmodel->notify = new YGtkZyppModel::PkgListNotify();
	zmodel->notify->model = zmodel;
	zmodel->list = new Ypp::PkgList (list);
	zmodel->list->setListener (zmodel->notify);
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

