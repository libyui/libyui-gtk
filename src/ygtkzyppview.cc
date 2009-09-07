/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkZyppView, Zypp GtkTreeView implementation */
// check the header file for information about this widget

/*
  Textdomain "yast2-gtk"
 */
#define YUILogComponent "gtk"
#include <config.h>
#include <gtk/gtk.h>
#include "ygtkzyppview.h"
#include "ygtktreeview.h"
#include "ygtktreemodel.h"
#include "ygtkrichtext.h"
#include "YGi18n.h"
#include "YGUtils.h"
#include "YGDialog.h"
#include <stdlib.h>
#include <string.h>
#include <list>

//** Utilities

#define GNOME_OPEN_PATH "/usr/bin/gnome-open"
inline bool CAN_OPEN_URIS()
{ return g_file_test (GNOME_OPEN_PATH, G_FILE_TEST_IS_EXECUTABLE); }
inline void OPEN_URI (const char *uri)
{ system ((std::string (GNOME_OPEN_PATH " ") + uri + " &").c_str()); }

static GdkPixbuf *loadPixbuf (const char *icon)
{ return YGUtils::loadPixbuf (std::string (DATADIR) + "/" + icon); }

//** Icons resources

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
void ygtk_zypp_finish (void)
{
	delete icons; icons = NULL;
	Ypp::finish();
}

//** Model

static GType _columnType (int col)
{
	switch (col) {
		case ZyppModel::ICON_COLUMN:
			return GDK_TYPE_PIXBUF;
		case ZyppModel::NAME_COLUMN:
		case ZyppModel::SUMMARY_COLUMN:
		case ZyppModel::NAME_SUMMARY_COLUMN:
		case ZyppModel::REPOSITORY_COLUMN:
		case ZyppModel::SUPPORT_COLUMN:
		case ZyppModel::INSTALLED_VERSION_COLUMN:
		case ZyppModel::AVAILABLE_VERSION_COLUMN:
		case ZyppModel::FOREGROUND_COLUMN:
		case ZyppModel::BACKGROUND_COLUMN:
			return G_TYPE_STRING;
		case ZyppModel::TO_INSTALL_COLUMN:
		case ZyppModel::TO_UPGRADE_COLUMN:
		case ZyppModel::TO_REMOVE_COLUMN:
		case ZyppModel::TO_MODIFY_COLUMN:
		case ZyppModel::SENSITIVE_COLUMN:
		case ZyppModel::CHECK_VISIBLE_COLUMN:
			return G_TYPE_BOOLEAN;
		case ZyppModel::STYLE_COLUMN:
		case ZyppModel::WEIGHT_COLUMN:
		case ZyppModel::XPAD_COLUMN:
			return G_TYPE_INT;
		case ZyppModel::PTR_COLUMN:
			return G_TYPE_POINTER;
	}
	return 0;
}

static void _getValueDefault (int col, GValue *value)
{
	switch (col) {
		case ZyppModel::ICON_COLUMN:
			g_value_set_object (value, NULL);
			break;
		case ZyppModel::NAME_COLUMN:
		case ZyppModel::SUMMARY_COLUMN:
		case ZyppModel::NAME_SUMMARY_COLUMN:
		case ZyppModel::REPOSITORY_COLUMN:
		case ZyppModel::SUPPORT_COLUMN:
		case ZyppModel::INSTALLED_VERSION_COLUMN:
		case ZyppModel::AVAILABLE_VERSION_COLUMN:
			g_value_set_string (value, g_strdup (""));
			break;
		case ZyppModel::FOREGROUND_COLUMN:
		case ZyppModel::BACKGROUND_COLUMN:
			g_value_set_string (value, NULL);
			break;
		case ZyppModel::TO_INSTALL_COLUMN:
		case ZyppModel::TO_UPGRADE_COLUMN:
		case ZyppModel::TO_REMOVE_COLUMN:
		case ZyppModel::TO_MODIFY_COLUMN:
			g_value_set_boolean (value, FALSE);
			break;
		case ZyppModel::STYLE_COLUMN:
			g_value_set_int (value, PANGO_STYLE_NORMAL);
			break;
		case ZyppModel::WEIGHT_COLUMN:
			g_value_set_int (value, PANGO_WEIGHT_NORMAL);
			break;
		case ZyppModel::XPAD_COLUMN:
			g_value_set_int (value, 0);
			break;
		case ZyppModel::SENSITIVE_COLUMN:
			g_value_set_boolean (value, TRUE);
			break;
		case ZyppModel::CHECK_VISIBLE_COLUMN:
			g_value_set_boolean (value, TRUE);
			break;
		case ZyppModel::PTR_COLUMN:
			g_value_set_pointer (value, NULL);
			break;
		case ZyppModel::TOTAL_COLUMNS: break;
	}
}

struct YGtkZyppModel : public YGtkTreeModel, Ypp::PkgList::Listener
{
	YGtkZyppModel()
	{
		if (!icons)
			icons = new PackageIcons();
	}

	void append (const std::string &header, Ypp::PkgList list, const std::string &applyAllLabel)
	{
		block.segments.push_back (Block::Segment (&block, header, list, applyAllLabel));
		list.addListener (this);
	}

protected:
	struct Block {
		struct Segment {
			Segment (Block *block, const std::string &header, Ypp::PkgList list, const std::string &applyAllLabel)
			: block (block), header (header), applyAll (applyAllLabel), list (list)
			{}

			int size()
			{
				int size = list.size();
				if (size == 0) return 0;
				return size + (header.empty() ? 0 : 1) + (applyAll.empty() ? 0 : 1);
			}

			void signalChanged (int index, YGtkTreeModel::Listener *listener)
			{
				int row = getRow() + index + (header.empty() ? 0 : 1);
				listener->rowChanged (row);
			}

			void signalInserted (int index, YGtkTreeModel::Listener *listener)
			{
				int row = getRow() + index + (header.empty() ? 0 : 1);
				if (list.size() == 1) {
					if (!header.empty())
						listener->rowInserted (row-1);
					if (!applyAll.empty())
						listener->rowInserted (row+1);
				}
				listener->rowInserted (row);
			}

			void signalDelete (int index, YGtkTreeModel::Listener *listener)
			{
				int row = getRow() + index + (header.empty() ? 0 : 1);
				if (list.size() == 1) {
					if (!header.empty())
						listener->rowDeleted (row-1);
					if (!applyAll.empty())
						listener->rowDeleted (row+1);
				}
				listener->rowDeleted (row);
			}

			int getRow()
			{
				int row = 0;
				for (std::list <Segment>::iterator it = block->segments.begin();
				     it != block->segments.end(); it++) {
					if (&(*it) == this)
						break;
					row += it->size();
				}
				return row;
			}

			Block *block;
			std::string header, applyAll;
			Ypp::PkgList list;
		};

		std::list <Segment> segments;

		Segment *get (const Ypp::PkgList *list)
		{
			for (std::list <Segment>::iterator it = segments.begin(); it != segments.end(); it++)
				if (it->list == *list)
					return &(*it);
			return NULL;
		}

		bool get (int row, Segment **segment, int *index)
		{
			int size = 0;
			for (std::list <Segment>::iterator it = segments.begin(); it != segments.end(); it++) {
				int _size = it->size();
				if (size+_size > row) {
					*segment = &(*it);
					*index = row - size - (it->header.empty() ? 0 : 1);
					if (!it->applyAll.empty() && *index == _size-1)
						*index = -2;
					return true;
				}
				size += _size;
			}
			return false;
		}

		int sumSize()
		{
			int size = 0;
			for (std::list <Segment>::iterator it = segments.begin(); it != segments.end(); it++)
				size += it->size();
			return size;
		}
	};

	Block block;

	virtual int rowsNb()
	{ return block.sumSize(); }

	virtual int columnsNb() const
	{ return ZyppModel::TOTAL_COLUMNS; }

	virtual bool showEmptyEntry() const
	{ return true; }

	virtual GType columnType (int col) const
	{ return _columnType (col); }

	virtual void getValue (int row, int col, GValue *value)
	{
		if (row == -1) {
			switch (col) {
				case ZyppModel::NAME_COLUMN:
					g_value_set_string (value, g_strdup (_("(No entries.)")));
					break;
				case ZyppModel::STYLE_COLUMN:
					g_value_set_int (value, PANGO_STYLE_ITALIC);
					break;
				case ZyppModel::CHECK_VISIBLE_COLUMN:
					g_value_set_boolean (value, FALSE);
					break;
				default:
					_getValueDefault (col, value);
					break;
			}
			return;
		}

		Block::Segment *segment = 0;
		int index = 0;
		block.get (row, &segment, &index);

		if (index == -1) {  // header
			switch (col) {
				case ZyppModel::NAME_COLUMN:
					g_value_set_string (value, g_strdup (segment->header.c_str()));
					break;
				case ZyppModel::NAME_SUMMARY_COLUMN: {
					std::string header (segment->header);
					header = "<big>" + header + "</big>";
					g_value_set_string (value, g_strdup (header.c_str()));
					break;
				}
				case ZyppModel::WEIGHT_COLUMN:
					g_value_set_int (value, PANGO_WEIGHT_BOLD);
					break;
				case ZyppModel::CHECK_VISIBLE_COLUMN:
					g_value_set_boolean (value, FALSE);
					break;
				case ZyppModel::FOREGROUND_COLUMN:
					g_value_set_string (value, g_strdup ("darkgray"));
					break;
				default:
					_getValueDefault (col, value);
					break;
			}
			return;
		}
		if (index == -2) {  // apply all label
			switch (col) {
				case ZyppModel::NAME_COLUMN:
					g_value_set_string (value, g_strdup (segment->applyAll.c_str()));
					break;
				case ZyppModel::BACKGROUND_COLUMN: {
#if 0
				case ZyppModel::FOREGROUND_COLUMN:
					GtkStyle *style = gtk_widget_get_default_style();
					GdkColor *color = &style->bg [GTK_STATE_SELECTED];
					if (col == ZyppModel::FOREGROUND_COLUMN)
						color = &style->fg [GTK_STATE_NORMAL];
					gchar *str = gdk_color_to_string (color);  // old: "lightblue"
					g_value_set_string (value, str);
#endif
					g_value_set_string (value, g_strdup ("lightblue"));
					break;
				}
				case ZyppModel::FOREGROUND_COLUMN:
					g_value_set_string (value, g_strdup ("black"));
					break;
				case ZyppModel::TO_INSTALL_COLUMN:
				case ZyppModel::TO_UPGRADE_COLUMN:
				case ZyppModel::TO_REMOVE_COLUMN:
				case ZyppModel::TO_MODIFY_COLUMN: {
					bool modified = segment->list.modified();
					g_value_set_boolean (value, modified);
					break;
				}
				default:
					_getValueDefault (col, value);
					break;
			}
			return;
		}

		Ypp::Package *package = segment->list.get (index);
		switch (col) {
			case ZyppModel::ICON_COLUMN: {
				if (!icons)
					icons = new PackageIcons();

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
			case ZyppModel::NAME_COLUMN: {
				std::string str (package->name());
				g_value_set_string (value, g_strdup (str.c_str()));
				break;
			}
			case ZyppModel::SUMMARY_COLUMN: {
				std::string str (package->summary());
				g_value_set_string (value, g_strdup (str.c_str()));
				break;
			}
			case ZyppModel::NAME_SUMMARY_COLUMN: {
				std::string str = package->name();
				std::string summary = package->summary();
				if (!summary.empty()) {
					YGUtils::escapeMarkup (summary);
					str += "\n<small>" + summary + "</small>";
				}
				g_value_set_string (value, g_strdup (str.c_str()));
				break;
			}
			case ZyppModel::REPOSITORY_COLUMN: {
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
			case ZyppModel::SUPPORT_COLUMN:
				g_value_set_string (value, g_strdup (package->support().c_str()));
				break;
			case ZyppModel::INSTALLED_VERSION_COLUMN: {
				const Ypp::Package::Version *version = package->getInstalledVersion();
				g_value_set_string (value, g_strdup (version->number.c_str()));
				break;
			}
			case ZyppModel::AVAILABLE_VERSION_COLUMN: {
				const Ypp::Package::Version *version = package->getAvailableVersion (0);
				g_value_set_string (value, g_strdup (version->number.c_str()));
				break;
			}
			case ZyppModel::TO_INSTALL_COLUMN:
				g_value_set_boolean (value, package->toInstall());
				break;
			case ZyppModel::TO_UPGRADE_COLUMN: {
				const Ypp::Package::Version *version = 0;
				if (package->toInstall (&version))
					g_value_set_boolean (value, version->cmp > 0);
				break;
			}
			case ZyppModel::TO_REMOVE_COLUMN:
				g_value_set_boolean (value, package->toRemove());
				break;
			case ZyppModel::TO_MODIFY_COLUMN:
				g_value_set_boolean (value, package->toModify());
				break;
			case ZyppModel::SENSITIVE_COLUMN:
				g_value_set_boolean (value, !package->isLocked());
				break;
/*
			case ZyppModel::STYLE_COLUMN: {
				PangoStyle style = PANGO_STYLE_NORMAL;
				if (package->isAuto())
					style = PANGO_STYLE_ITALIC;
				g_value_set_int (value, style);
				break;
			}
*/
			case ZyppModel::WEIGHT_COLUMN: {
				bool highlight = segment->list.highlight (package);
				int weight = highlight ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL;
				g_value_set_int (value, weight);
				break;
			}
			case ZyppModel::XPAD_COLUMN: {
				int xpad = package->isAuto() ? 10 : 0;
				g_value_set_int (value, xpad);
				break;
			}
			case ZyppModel::PTR_COLUMN:
				g_value_set_pointer (value, (void *) package);
				break;
			default:
				_getValueDefault (col, value);
				break;
		}
	}

	virtual void entryChanged  (const Ypp::PkgList list, int index, Ypp::Package *package)
	{
		Block::Segment *seg = block.get (&list);
		seg->signalChanged (index, listener);
	}

	virtual void entryInserted (const Ypp::PkgList list, int index, Ypp::Package *package)
	{
		Block::Segment *seg = block.get (&list);
		seg->signalInserted (index, listener);
	}

	virtual void entryDeleted  (const Ypp::PkgList list, int index, Ypp::Package *package)
	{
		Block::Segment *seg = block.get (&list);
		seg->signalDelete (index, listener);
	}
};

GtkTreeModel *ygtk_zypp_model_new()
{ return ygtk_tree_model_new (new YGtkZyppModel()); }

void ygtk_zypp_model_append (GtkTreeModel *model,
	const std::string &header, Ypp::PkgList list, const std::string &applyAllLabel)
{
	YGtkZyppModel *zmodel = (YGtkZyppModel *) ygtk_tree_model_get_model (model);
	zmodel->append (header, list, applyAllLabel);
}

// model to show "loading..." on query
struct EmptyModel : public YGtkTreeModel
{
	virtual bool showEmptyEntry() const { return false; }
	virtual int rowsNb() { return 1; }
	virtual int columnsNb() const { return ZyppModel::TOTAL_COLUMNS; }
	virtual GType columnType (int col) const { return _columnType (col); }

	virtual void getValue (int row, int col, GValue *value)
	{
		switch (col) {
			case ZyppModel::NAME_COLUMN:
				g_value_set_string (value, g_strdup (_("Query...")));
				break;
			case ZyppModel::STYLE_COLUMN:
				g_value_set_int (value, PANGO_STYLE_ITALIC);
				break;
			case ZyppModel::CHECK_VISIBLE_COLUMN:
				g_value_set_boolean (value, FALSE);
				break;
			default:
				_getValueDefault (col, value);
				break;
		}
	}

	virtual void setListener (YGtkTreeModel::Listener *listener) {}
};

//** View

struct YGtkPackageView::Impl
{
	Impl (GtkWidget *scroll, bool descriptiveTooltip)
	: m_listener (NULL), m_popup_hack (NULL), m_descriptiveTooltip (descriptiveTooltip),
	  m_model (NULL), m_modelId (0)
	{
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

		GtkTreeView *view = GTK_TREE_VIEW (m_view = ygtk_tree_view_new());
		gtk_tree_view_set_search_column (view, ZyppModel::NAME_COLUMN);
		gtk_tree_view_set_fixed_height_mode (view, TRUE);
		gtk_tree_view_set_headers_visible (view, FALSE);

		GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
		gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
		g_signal_connect (G_OBJECT (selection), "changed",
		                  G_CALLBACK (packages_selected_cb), this);
		gtk_tree_selection_set_select_function (selection, can_select_row_cb,
		                                        this, NULL);

		g_signal_connect (G_OBJECT (m_view), "row-activated",
		                  G_CALLBACK (package_activated_cb), this);
		g_signal_connect (G_OBJECT (m_view), "right-click",
			              G_CALLBACK (popup_menu_cb), this);
		gtk_widget_set_has_tooltip (m_view, TRUE);
		g_signal_connect (G_OBJECT (m_view), "query-tooltip",
		                  G_CALLBACK (query_tooltip_cb), this);

		gtk_container_add (GTK_CONTAINER (scroll), m_view);
		gtk_widget_show_all (scroll);
		clear();
	}

	~Impl()
	{
		if (m_popup_hack) gtk_widget_destroy (m_popup_hack);
		if (m_model)
			g_object_unref (G_OBJECT (m_model));
	}

	// data
	YGtkPackageView::Listener *m_listener;
	GtkWidget *m_view, *m_popup_hack;
	bool m_descriptiveTooltip;
	GtkTreeModel *m_model;
	guint m_modelId;

	// methods
	void setList (Ypp::PkgList list, const char *applyAllLabel)
	{
		std::string _applyAllLabel = applyAllLabel ? applyAllLabel : "";
		if (m_model)
			g_object_unref (G_OBJECT (m_model));
		m_model = ygtk_zypp_model_new();
		ygtk_zypp_model_append (m_model, std::string (""), list, _applyAllLabel);
		GtkTreeView *view = GTK_TREE_VIEW (m_view);
		gtk_tree_view_set_model (view, m_model);
		if (GTK_WIDGET_REALIZED (view))
			gtk_tree_view_scroll_to_point (view, -1, 0);
	}

	void appendList (const char *header, Ypp::PkgList list, const char *applyAllLabel)
	{
		std::string _header = header ? header : "";
		std::string _applyAllLabel = applyAllLabel ? applyAllLabel : "";
		if (!m_model)
			m_model = ygtk_zypp_model_new();
		ygtk_zypp_model_append (m_model, _header, list, _applyAllLabel);
		if (!m_modelId)
			m_modelId = g_idle_add_full (G_PRIORITY_LOW, set_model_cb, this, NULL);
	}

	void clear()
	{
		if (m_model)
			g_object_unref (G_OBJECT (m_model));
		m_model = 0;
		static GtkTreeModel *empty = 0;
		if (!empty)
			empty = ygtk_tree_model_new (new EmptyModel());
		gtk_tree_view_set_model (GTK_TREE_VIEW (m_view), empty);
	}

	GList *getSelectedPaths (GtkTreeModel **model)
	{ return gtk_tree_selection_get_selected_rows (getTreeSelection(), model); }

	void selectAll()
	{ gtk_tree_selection_select_all (getTreeSelection()); }

	void unselectAll()
	{ gtk_tree_selection_unselect_all (getTreeSelection()); }

	int countSelected()
	{ return gtk_tree_selection_count_selected_rows (getTreeSelection()); }

	Ypp::PkgList getSelected()
	{
		GtkTreeModel *model;
		GList *paths = getSelectedPaths (&model);
		Ypp::PkgList packages;
		for (GList *i = paths; i; i = i->next) {
			Ypp::Package *package;
			GtkTreePath *path = (GtkTreePath *) i->data;
			GtkTreeIter iter;
			gtk_tree_model_get_iter (model, &iter, path);
			gtk_tree_model_get (model, &iter, ZyppModel::PTR_COLUMN, &package, -1);
			gtk_tree_path_free (path);
			if (package)
				packages.append (package);
		}
		g_list_free (paths);
		return packages;
	}

	void appendIconColumn (const char *header, int col)
	{
		GtkTreeView *view = GTK_TREE_VIEW (m_view);
		if (header)
			gtk_tree_view_set_headers_visible (view, TRUE);
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
			int height = MAX (34, YGUtils::getCharsHeight (m_view, 2));
			gtk_cell_renderer_set_fixed_size (renderer, -1, height);
		column = gtk_tree_view_column_new_with_attributes (
			header, renderer, "pixbuf", col,
			"cell-background", ZyppModel::BACKGROUND_COLUMN, NULL);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_fixed_width (column, 38);
		gtk_tree_view_append_column (view, column);
	}

	void appendCheckColumn (int modelCol)
	{
		GtkTreeView *view = GTK_TREE_VIEW (m_view);
		GtkCellRenderer *renderer = gtk_cell_renderer_toggle_new();
		GtkTreeViewColumn *column;
		column = gtk_tree_view_column_new_with_attributes (NULL,
			renderer, "active", modelCol, "visible", ZyppModel::CHECK_VISIBLE_COLUMN,
			"sensitive", ZyppModel::SENSITIVE_COLUMN,
			"cell-background", ZyppModel::BACKGROUND_COLUMN, NULL);
		g_object_set_data (G_OBJECT (renderer), "col", GINT_TO_POINTER (modelCol));
		g_signal_connect (G_OBJECT (renderer), "toggled",
			              G_CALLBACK (renderer_toggled_cb), this);
		// it seems like GtkCellRendererToggle has no width at start, so fixed doesn't work
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_fixed_width (column, 25);
		gtk_tree_view_append_column (view, column);
	}

	void appendTextColumn (const char *header, int col, int size, bool identAuto)
	{
		GtkTreeView *view = GTK_TREE_VIEW (m_view);
		if (header)
			gtk_tree_view_set_headers_visible (view, TRUE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		g_object_set (G_OBJECT (renderer), "ellipsize",
			size == -1 ? PANGO_ELLIPSIZE_END : PANGO_ELLIPSIZE_MIDDLE, NULL);
/*		gboolean reverse = gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL;
		if (reverse) {  // work-around: Pango ignored alignment flag on RTL
			gtk_widget_set_direction (m_view, GTK_TEXT_DIR_LTR);
			g_object_set (renderer, "alignment", PANGO_ALIGN_RIGHT, NULL);
		}*/
		GtkTreeViewColumn *column;
		column = gtk_tree_view_column_new_with_attributes (
			header, renderer, "markup", col,
			"sensitive", ZyppModel::SENSITIVE_COLUMN,
			"style", ZyppModel::STYLE_COLUMN,
			"weight", ZyppModel::WEIGHT_COLUMN,
			"foreground", ZyppModel::FOREGROUND_COLUMN,
			"cell-background", ZyppModel::BACKGROUND_COLUMN,
			NULL);
		if (identAuto)
			gtk_tree_view_column_add_attribute (column, renderer, "xpad", ZyppModel::XPAD_COLUMN);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_resizable (column, TRUE);
		if (size >= 0)
			gtk_tree_view_column_set_fixed_width (column, size);
		else
			gtk_tree_view_column_set_expand (column, TRUE);
//		gtk_tree_view_insert_column (view, column, reverse ? 0 : -1);
		gtk_tree_view_append_column (view, column);
	}

	GtkTreeSelection *getTreeSelection()
	{ return gtk_tree_view_get_selection (GTK_TREE_VIEW (m_view)); }

	void signalSelected()
	{ if (m_listener) m_listener->packagesSelected (getSelected()); }

	void signalPopup (int button, int event_time)
	{
		// GtkMenu emits "deactivate" before Items notifications, so there isn't
		// a better way to de-allocate the popup
		if (m_popup_hack) gtk_widget_destroy (m_popup_hack);
		GtkWidget *menu = m_popup_hack = gtk_menu_new();

		struct inner {
			static void appendItem (GtkWidget *menu, const char *label,
				const char *tooltip, const char *icon, bool sensitive,
				void (& callback) (GtkMenuItem *item, Impl *pThis), Impl *pThis)
			{
				GtkWidget *item;
				if (icon) {
					if (label) {
						item = gtk_image_menu_item_new_with_mnemonic (label);
						GtkWidget *image;
						if (*icon == 'g')
							image = gtk_image_new_from_stock (icon, GTK_ICON_SIZE_MENU);
						else {
							std::string filename = std::string (DATADIR) + "/" + icon;
							image = gtk_image_new_from_file (filename.c_str());
						}
						gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
					}
					else
						item = gtk_image_menu_item_new_from_stock (icon, NULL);
				}
				else
					item = gtk_menu_item_new_with_mnemonic (label);
				if (tooltip)
					gtk_widget_set_tooltip_markup (item, tooltip);
				if (!sensitive)
					gtk_widget_set_sensitive (item, FALSE);
				gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
				g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (callback), pThis);
			}
			static void install_cb (GtkMenuItem *item, Impl *pThis)
			{ pThis->getSelected().install(); }
			static void remove_cb (GtkMenuItem *item, Impl *pThis)
			{ pThis->getSelected().remove(); }
			static void undo_cb (GtkMenuItem *item, Impl *pThis)
			{ pThis->getSelected().undo(); }
			static void lock_cb (GtkMenuItem *item, Impl *pThis)
			{ pThis->getSelected().lock (true); }
			static void unlock_cb (GtkMenuItem *item, Impl *pThis)
			{ pThis->getSelected().lock (false); }
			static void select_all_cb (GtkMenuItem *item, Impl *pThis)
			{ pThis->selectAll(); }
		};

		Ypp::PkgList packages = getSelected();
		bool empty = true, canLock = packages.canLock(), unlocked = packages.unlocked();
		bool locked = !unlocked && canLock;
		if (packages.notInstalled())
			inner::appendItem (menu, _("_Install"), 0, GTK_STOCK_SAVE,
			                   !locked, inner::install_cb, this), empty = false;
		if (packages.upgradable())
			inner::appendItem (menu, _("_Upgrade"), 0, GTK_STOCK_GOTO_TOP,
			                   !locked, inner::install_cb, this), empty = false;
		if (packages.installed() && packages.canRemove())
			inner::appendItem (menu, _("_Remove"), 0, GTK_STOCK_DELETE,
			                   !locked, inner::remove_cb, this), empty = false;
		if (packages.modified())
			inner::appendItem (menu, _("_Undo"), 0, GTK_STOCK_UNDO,
			                   true, inner::undo_cb, this), empty = false;
		if (canLock) {
			static const char *lock_tooltip =
				"<b>Package lock:</b> prevents the package status from being modified by "
				"the solver (that is, it won't honour dependencies or collections ties.)";
			if (packages.locked())
				inner::appendItem (menu, _("_Unlock"), _(lock_tooltip), "pkg-unlocked.png",
				                   true, inner::unlock_cb, this), empty = false;
			if (unlocked)
				inner::appendItem (menu, _("_Lock"), _(lock_tooltip), "pkg-locked.png",
				                   true, inner::lock_cb, this), empty = false;
		}
		if (!empty)
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());
		inner::appendItem (menu, 0, 0, GTK_STOCK_SELECT_ALL,
		                   true, inner::select_all_cb, this);

		gtk_menu_attach_to_widget (GTK_MENU (menu), m_view, NULL);
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,  button, event_time);
		gtk_widget_show_all (menu);
	}

	// callbacks
	static gboolean set_model_cb (gpointer data)
	{
		Impl *pThis = (Impl *) data;
		GtkTreeView *view = GTK_TREE_VIEW (pThis->m_view);
		if (gtk_tree_view_get_model (view) == pThis->m_model)
			gtk_tree_view_set_model (view, NULL);
		gtk_tree_view_set_model (view, pThis->m_model);
		pThis->m_modelId = 0;
		return FALSE;
	}

	static void packages_selected_cb (GtkTreeSelection *selection, Impl *pThis)
	{ if (GTK_WIDGET_REALIZED (pThis->m_view)) pThis->signalSelected(); }

	static void popup_menu_cb (YGtkTreeView *view, gboolean outreach, Impl *pThis)
	{ if (!outreach) pThis->signalPopup(3, gtk_get_current_event_time()); }

	static gboolean can_select_row_cb (GtkTreeSelection *selection, GtkTreeModel *model,
		GtkTreePath *path, gboolean path_currently_selected, gpointer data)
	{
		void *package;
		GtkTreeIter iter;
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter, ZyppModel::PTR_COLUMN, &package, -1);
		return package != NULL;
	}

	static void apply (Ypp::Package *package, int col, bool enable)
	{
		if (enable)
			switch (col) {
				case ZyppModel::TO_INSTALL_COLUMN:
				case ZyppModel::TO_UPGRADE_COLUMN:
					package->install (0);
					break;
				case ZyppModel::TO_REMOVE_COLUMN:
					package->remove();
					break;
				default: break;
			}
		else
			package->undo();
	}

	static gboolean apply_iter_cb (GtkTreeModel *model,
		GtkTreePath *path, GtkTreeIter *iter, gpointer col)
	{
		Ypp::Package *package;
		gboolean enable;
		enable = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (model), "enable"));
		gtk_tree_model_get (model, iter, ZyppModel::PTR_COLUMN, &package, -1);
		if (package)
			apply (package, GPOINTER_TO_INT (col), enable);
		return FALSE;
	}

	static void renderer_toggled_cb (GtkCellRendererToggle *renderer, gchar *path_str,
			                         Impl *pThis)
	{
		Ypp::Package *package = 0;
		GtkTreeView *view = GTK_TREE_VIEW (pThis->m_view);
		GtkTreeModel *model = gtk_tree_view_get_model (view);
		GtkTreeIter iter;
		gtk_tree_model_get_iter_from_string (model, &iter, path_str);
		gtk_tree_model_get (model, &iter, ZyppModel::PTR_COLUMN, &package, -1);

		gboolean active = gtk_cell_renderer_toggle_get_active (renderer);
		int col = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (renderer), "col"));
		if (package)
			apply (package, col, !active);
		else {
			if (!gtk_tree_model_iter_next (model, &iter)) {  // on apply-all
				g_object_set_data (G_OBJECT (model), "enable", GINT_TO_POINTER (!active));
				Ypp::get()->startTransactions();
				gtk_tree_model_foreach (model, apply_iter_cb, GINT_TO_POINTER (col));
				Ypp::get()->finishTransactions();
			}
		}
	}

	static void package_activated_cb (GtkTreeView *view, GtkTreePath *path,
	                                  GtkTreeViewColumn *column, Impl *pThis)
	{
		Ypp::PkgList packages = pThis->getSelected();
		if (packages.notInstalled() || packages.upgradable())
			packages.install();
	}

	static gboolean query_tooltip_cb (GtkWidget *widget, gint x, gint y,
		gboolean keyboard_mode, GtkTooltip *tooltip, Impl *pThis)
	{
		GtkTreeView *view = GTK_TREE_VIEW (widget);
		GtkTreeModel *model;
		GtkTreePath *path;
		GtkTreeIter iter;
		if (gtk_tree_view_get_tooltip_context (view,
		        &x, &y, keyboard_mode, &model, &path, &iter)) {
			gtk_tree_view_set_tooltip_row (view, tooltip, path);
			gtk_tree_path_free (path);

			Ypp::Package *package = 0;
			gtk_tree_model_get (model, &iter, ZyppModel::PTR_COLUMN, &package, -1);
			if (!package) return FALSE;

			std::string text;
			text.reserve (64);
			if (!pThis->m_descriptiveTooltip) {
				GtkTreeViewColumn *column;
				int bx, by;
				gtk_tree_view_convert_widget_to_bin_window_coords (
					view, x, y, &bx, &by);
				gtk_tree_view_get_path_at_pos (
					view, x, y, NULL, &column, NULL, NULL);
				int status_col = 0;
				if (column == gtk_tree_view_get_column (view, status_col)) {
					if (package->toInstall()) {
						if (package->isInstalled())
							text = _("To re-install a different version");
						else
							text = _("To install");
					}
					else if (package->toRemove())
						text = _("To remove");
					else if (package->isInstalled()) {
						text = _("Installed");
						if (package->hasUpgrade())
							text += _(" (upgrade available)");
					}
					else
						text = _("Not installed");
					if (package->isAuto())
						text += _("\n<i>status changed by the dependency solver</i>");
				}
				else {
					if (package->isLocked())
						text = _("locked: right-click to unlock");
					else if (package->isAuto())
						text = _("auto: automatically selected due to dependencies");
				}
			}
			else {
				text = std::string ("<b>") + package->name() + "</b>\n";
				text += package->description (GTK_MARKUP);
			}
			if (text.empty())
				return FALSE;
			gtk_tooltip_set_markup (tooltip, text.c_str());

			if (pThis->m_descriptiveTooltip) {
				GdkPixbuf *pixbuf = 0;
				std::string filename (package->icon());
				if (!filename.empty())
					pixbuf = YGUtils::loadPixbuf (filename.c_str());
				if (!pixbuf)
					gtk_tree_model_get (model, &iter, ZyppModel::ICON_COLUMN, &pixbuf, -1);
				if (pixbuf) {
					gtk_tooltip_set_icon (tooltip, pixbuf);
					g_object_unref (G_OBJECT (pixbuf));
				}
			}
			return TRUE;
		}
		return FALSE;
	}
};

G_DEFINE_TYPE (YGtkPackageView, ygtk_package_view, GTK_TYPE_SCROLLED_WINDOW)

static void ygtk_package_view_init (YGtkPackageView *view)
{}

YGtkPackageView *ygtk_package_view_new (gboolean descriptiveTooltip)
{
	YGtkPackageView *view = (YGtkPackageView *) g_object_new (YGTK_TYPE_PACKAGE_VIEW, NULL);
	view->impl = new YGtkPackageView::Impl (GTK_WIDGET (view), descriptiveTooltip);
	return view;
}

static void ygtk_package_view_finalize (GObject *object)
{
	G_OBJECT_CLASS (ygtk_package_view_parent_class)->finalize (object);
	YGtkPackageView *view = YGTK_PACKAGE_VIEW (object);
	delete view->impl;
	view->impl = NULL;
}

static void ygtk_package_view_class_init (YGtkPackageViewClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = ygtk_package_view_finalize;
}

void YGtkPackageView::setList (Ypp::PkgList list, const char *applyAllLabel)
{ impl->setList (list, applyAllLabel); }

void YGtkPackageView::appendList (const char *header, Ypp::PkgList list, const char *applyAllLabel)
{ impl->appendList (header, list, applyAllLabel); }

void YGtkPackageView::clear()
{ impl->clear(); }

void YGtkPackageView::appendIconColumn (const char *header, int col)
{ impl->appendIconColumn (header, col); }

void YGtkPackageView::appendCheckColumn (int col)
{ impl->appendCheckColumn (col); }

void YGtkPackageView::appendTextColumn (const char *header, int col, int size, bool identAuto)
{ impl->appendTextColumn (header, col, size, identAuto); }

void YGtkPackageView::setListener (Listener *listener)
{ impl->m_listener = listener; }

Ypp::PkgList YGtkPackageView::getSelected()
{ return impl->getSelected(); }

//** Detail & control

class YGtkDetailView::Impl
{
private:
	struct Versions : public Ypp::PkgList::Listener {
		GtkWidget *m_box, *m_versions_box, *m_button, *m_undo_button;
		Ypp::PkgList m_packages;  // we keep a copy to test against modified...

		GtkWidget *getWidget() { return m_box; }

		Versions()
		{
			GtkWidget *label = gtk_label_new (_("Versions:"));
			YGUtils::setWidgetFont (label, PANGO_STYLE_NORMAL, PANGO_WEIGHT_BOLD, PANGO_SCALE_MEDIUM);
			gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

			m_versions_box = gtk_vbox_new (FALSE, 2);
			m_button = gtk_button_new_with_label ("");
			g_signal_connect (G_OBJECT (m_button), "clicked", G_CALLBACK (button_clicked_cb), this);
			m_undo_button = gtk_button_new_with_label ("");
			gtk_button_set_image (GTK_BUTTON (m_undo_button),
				gtk_image_new_from_stock (GTK_STOCK_UNDO, GTK_ICON_SIZE_BUTTON));
			gtk_widget_set_tooltip_text (m_undo_button, _("Undo"));
			g_signal_connect (G_OBJECT (m_undo_button), "clicked", G_CALLBACK (undo_clicked_cb), this);
			GtkWidget *button_box = gtk_hbox_new (FALSE, 6);
			gtk_box_pack_start (GTK_BOX (button_box), gtk_label_new(""), TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (button_box), m_button, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (button_box), m_undo_button, FALSE, TRUE, 0);

			m_box = gtk_vbox_new (FALSE, 6);
			gtk_box_pack_start (GTK_BOX (m_box), label, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (m_box), m_versions_box, TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (m_box), button_box, FALSE, TRUE, 0);
		}

		~Versions()
		{ m_packages.removeListener (this); }

		void setPackages (Ypp::PkgList packages)
		{
			m_packages.removeListener (this);
			m_packages = packages;
			m_packages.addListener (this);

			GList *children = gtk_container_get_children (GTK_CONTAINER (m_versions_box));
			for (GList *i = children; i; i = i->next)
				gtk_container_remove (GTK_CONTAINER (m_versions_box), (GtkWidget *) i->data);
			g_list_free (children);

			if (packages.size() == 0) {
				gtk_widget_hide (m_box);
				return;
			}
			gtk_widget_show (m_box);
			if (packages.size() == 1) {
				Ypp::Package *package = packages.get (0);

				int children = 0;
				GtkWidget *radio = 0;
				const Ypp::Package::Version *version;
				if ((version = package->getInstalledVersion())) {
					std::string _version = YGUtils::truncate (version->number, 20, 0);

					bool italic = package->toRemove();
					char *text = g_strdup_printf ("%s%s <small>(%s)</small>\n%s%s",
						italic ? "<i>" : "",
						_version.c_str(), version->arch.c_str(), _("Installed"),
						italic ? "</i>" : "");
					radio = gtk_radio_button_new_with_label (NULL, text);
					gtk_label_set_use_markup (GTK_LABEL (GTK_BIN (radio)->child), TRUE);
					if (version->number.size() > 20) {
						char *text = g_strdup_printf ("%s <small>(%s)</small>\n%s",
							version->number.c_str(), version->arch.c_str(), _("Installed"));
						gtk_widget_set_tooltip_markup (radio, text);
						g_free (text);
					}
					gtk_box_pack_start (GTK_BOX (m_versions_box), radio, FALSE, TRUE, 0);
					g_free (text);
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);
					g_object_set_data (G_OBJECT (radio), "version", (void *) version);
					g_signal_connect (G_OBJECT (radio), "toggled",
						              G_CALLBACK (version_toggled_cb), this);
					children++;
				}
				bool activeSet = package->toRemove();
				const Ypp::Package::Version *toInstall = 0;
				if (!package->toInstall (&toInstall))
					toInstall = 0;
				const Ypp::Repository *favoriteRepo = Ypp::get()->favoriteRepository();
				for (int i = 0; (version = package->getAvailableVersion (i)); i++) {
					std::string _version = YGUtils::truncate (version->number, 20, 0);
					std::string repo;
					if (version->repo)
						repo = YGUtils::truncate (version->repo->name, 22, 0);

					bool italic = toInstall == version;
					char *text = g_strdup_printf ("%s%s <small>(%s)</small>\n%s%s",
						italic ? "<i>" : "",
						_version.c_str(), version->arch.c_str(), repo.c_str(),
						italic ? "</i>" : "");
					radio = gtk_radio_button_new_with_label_from_widget (
						GTK_RADIO_BUTTON (radio), text);
					gtk_label_set_use_markup (GTK_LABEL (GTK_BIN (radio)->child), TRUE);
					g_free (text);
					if (version->number.size() > 20 ||
						(version->repo && version->repo->name.size() > 22)) {
						char *text = g_strdup_printf ("%s <small>(%s)</small>\n%s",
							version->number.c_str(), version->arch.c_str(),
							version->repo ? version->repo->name.c_str() : "");
						gtk_widget_set_tooltip_markup (radio, text);
						g_free (text);
					}
					gtk_box_pack_start (GTK_BOX (m_versions_box), radio, FALSE, TRUE, 0);
					if (!activeSet) {
						if (toInstall == version) {
							gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);
							activeSet = true;
						}
						else if (i == 0)
							gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);
						else if (version->repo == favoriteRepo) {
							gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);
							favoriteRepo = 0;  // select only the 1st hit
						}
					}
					g_object_set_data (G_OBJECT (radio), "version", (void *) version);
					g_signal_connect (G_OBJECT (radio), "toggled",
						              G_CALLBACK (version_toggled_cb), this);
					if ((children % 2) == 1)
						g_signal_connect (G_OBJECT (radio), "expose-event", G_CALLBACK (draw_gray_cb), this);
					children++;
				}
				gtk_widget_show_all (m_versions_box);
			}

			// is locked
			if (packages.locked() || packages.unlocked())
				gtk_widget_set_sensitive (m_button, !packages.locked());
			else
				gtk_widget_set_sensitive (m_button, TRUE);
			syncButton();
		}

	private:
		virtual void entryChanged  (const Ypp::PkgList list, int index, Ypp::Package *package)
		{ setPackages (m_packages); /* refresh */ }

		// won't happen:
		virtual void entryInserted (const Ypp::PkgList list, int index, Ypp::Package *package) {}
		virtual void entryDeleted  (const Ypp::PkgList list, int index, Ypp::Package *package) {}

		static gboolean draw_gray_cb (GtkWidget *widget, GdkEventExpose *event, Versions *pThis)
		{
			GtkAllocation *alloc = &widget->allocation;
			int x = alloc->x, y = alloc->y, w = alloc->width, h = alloc->height;

			cairo_t *cr = gdk_cairo_create (widget->window);
			cairo_rectangle (cr, x, y, w, h);
			cairo_set_source_rgb (cr, 245/255.0, 245/255.0, 245/255.0);
			cairo_fill (cr);
			cairo_destroy (cr);
			return FALSE;
		}

		const Ypp::Package::Version *getVersion()
		{
			GtkWidget *radio = 0;
			GList *children = gtk_container_get_children (GTK_CONTAINER (m_versions_box));
			for (GList *i = children; i; i = i->next)
				if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (i->data))) {
					radio = (GtkWidget *) i->data;
					break;
				}
			g_list_free (children);
			if (radio)
				return (Ypp::Package::Version *) g_object_get_data (G_OBJECT (radio), "version");
			return NULL;
		}

		void syncButton()
		{
			gtk_widget_hide (m_undo_button);
			const char *label = 0, *stock = 0;
			if (m_packages.size() == 1) {
				Ypp::Package *package = m_packages.get (0);
				const Ypp::Package::Version *version = getVersion();
				const Ypp::Package::Version *installed = package->getInstalledVersion();
				if (installed == version) {
					label = _("Remove");
					stock = GTK_STOCK_DELETE;
					if (package->toRemove())
						gtk_widget_show (m_undo_button);
				}
				else {
					if (installed) {
						if (version->cmp > 0) {
							label = _("Upgrade");
							stock = GTK_STOCK_GO_UP;
						}
						else if (version->cmp < 0) {
							label = _("Downgrade");
							stock = GTK_STOCK_GO_DOWN;
						}
						else {
							label = _("Re-install");
							stock = GTK_STOCK_REFRESH;
						}
					}
					else {
						label = _("Install");
						stock = GTK_STOCK_SAVE;
					}

					const Ypp::Package::Version *toInstall = 0;
					if (!package->toInstall (&toInstall))
						toInstall = 0;
					if (toInstall == version)
						gtk_widget_show (m_undo_button);
				}
			}
			else {
				if (m_packages.modified())
					gtk_widget_show (m_undo_button);
				if (m_packages.upgradable()) {
					label = _("Upgrade");
					stock = GTK_STOCK_GO_UP;
				}
				else if (m_packages.installed()) {
					label = _("Remove");
					stock = GTK_STOCK_DELETE;
				}
				else if (m_packages.notInstalled()) {
					label = _("Install");
					stock = GTK_STOCK_SAVE;
				}
				else if (m_packages.modified()) {
					label = _("Undo");
					stock = GTK_STOCK_UNDO;
					gtk_widget_hide (m_undo_button);
				}
			}
			if (label) {
				gtk_button_set_label (GTK_BUTTON (m_button), label);
				GtkWidget *image = gtk_image_new_from_stock (
					stock, GTK_ICON_SIZE_BUTTON);
				gtk_button_set_image (GTK_BUTTON (m_button), image);
				gtk_widget_show (m_button);
			}
			else
				gtk_widget_hide (m_button);
		}

		static void version_toggled_cb (GtkToggleButton *radio, Versions *pThis)
		{ if (gtk_toggle_button_get_active (radio)) pThis->syncButton(); }

		static void button_clicked_cb (GtkButton *button, Versions *pThis)
		{
			if (pThis->m_packages.size() == 1) {
				Ypp::Package *package = pThis->m_packages.get (0);
				const Ypp::Package::Version *version = pThis->getVersion();
				const Ypp::Package::Version *installed = package->getInstalledVersion();
				if (installed == version)
					pThis->m_packages.remove();
				else
					package->install (version);
			}
			else {
				if (pThis->m_packages.upgradable())
					pThis->m_packages.install();
				else if (pThis->m_packages.installed())
					pThis->m_packages.remove();
				else if (pThis->m_packages.notInstalled())
					pThis->m_packages.install();
				else if (pThis->m_packages.modified())
					pThis->m_packages.undo();
			}
		}

		static void undo_clicked_cb (GtkButton *button, Versions *pThis)
		{
			pThis->m_packages.undo();
		}
	};

GtkWidget *m_scroll, *m_icon, *m_icon_frame, *m_description, *m_filelist, *m_changelog,
	*m_authors, *m_support, *m_requires, *m_provides;
Versions *m_versions;
YGtkPackageView *m_contents;

public:
	Impl (GtkWidget *scroll, bool onlineUpdate)
	{
		m_scroll = scroll;
		m_versions = new Versions();
		GtkWidget *hbox = gtk_hbox_new (FALSE, 2);
		setupScrollAsWhite (scroll, hbox);

		GtkWidget *versions_box = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (versions_box),
			createIconWidget (&m_icon, &m_icon_frame), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (versions_box), m_versions->getWidget(), FALSE, TRUE, 0);

		GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
		m_description = ygtk_rich_text_new();
		g_signal_connect (G_OBJECT (m_description), "link-clicked",
		                  G_CALLBACK (link_pressed_cb), this);
		gtk_box_pack_start (GTK_BOX (vbox), m_description, FALSE, TRUE, 0);
		if (!onlineUpdate) {
			m_filelist = ygtk_rich_text_new();
			m_changelog = ygtk_rich_text_new();
			m_authors = ygtk_rich_text_new();
			m_support = ygtk_rich_text_new();
			m_requires = ygtk_rich_text_new();
			m_provides = ygtk_rich_text_new();
			GtkWidget *dependencies_box = gtk_hbox_new (TRUE, 0);
			gtk_box_pack_start (GTK_BOX (dependencies_box), m_requires, TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (dependencies_box), m_provides, TRUE, TRUE, 0);

			appendExpander (vbox, _("File List"), m_filelist);
			appendExpander (vbox, _("Changelog"), m_changelog);
			appendExpander (vbox, _("Authors"), m_authors);
			appendExpander (vbox, _("Dependencies"), dependencies_box);
			appendExpander (vbox, "", m_support);
			m_contents = NULL;
			if (CAN_OPEN_URIS())
				g_signal_connect (G_OBJECT (m_filelist), "link-clicked",
						          G_CALLBACK (link_pressed_cb), this);
		}
		else {
			m_filelist = m_changelog = m_authors = m_support = m_requires = m_provides = NULL;
			m_contents = ygtk_package_view_new (TRUE);
			m_contents->appendTextColumn (_("Name"), ZyppModel::NAME_COLUMN, 150);
			m_contents->appendTextColumn (_("Summary"), ZyppModel::SUMMARY_COLUMN);
			gtk_widget_set_size_request (GTK_WIDGET (m_contents), -1, 150);
			appendExpander (vbox, _("Applies to"), GTK_WIDGET (m_contents));
		}

		gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), versions_box, FALSE, TRUE, 0);
		gtk_widget_show_all (hbox);
	}

	~Impl()
	{ delete m_versions; }

	void setPackages (Ypp::PkgList packages)
	{
		gtk_widget_hide (m_icon_frame);
		if (packages.size() == 1) {
			Ypp::Package *package = packages.get (0);
			std::string description = "<b>" + package->name() + "</b><br>";
			description += package->description (HTML_MARKUP);
			setText (m_description, description);
			if (m_filelist)  setText (m_filelist, package->filelist (true));
			if (m_changelog) setText (m_changelog, package->changelog());
			if (m_authors)   setText (m_authors, package->authors (true));
			if (m_support) {
				GtkWidget *expander = gtk_widget_get_ancestor (m_support, GTK_TYPE_EXPANDER);
				std::string label ("<b>" + std::string (_("Supportability:")) + "</b> ");
				label += package->support();
				gtk_expander_set_label (GTK_EXPANDER (expander), label.c_str());
				setText (m_support, package->supportText (true));
			}
			if (m_requires && m_provides) {
				std::string requires_str = _("Requires:");
				std::string provides_str = _("Provides:");
				requires_str += "<br><blockquote>";
				requires_str += package->requires (true);
				YGUtils::replace (provides_str, "\n", 1, "<br>");
				requires_str += "</blockquote>";
				provides_str += "<br><blockquote>";
				provides_str += package->provides (true);
				YGUtils::replace (requires_str, "\n", 1, "<br>");
				provides_str += "</blockquote>";
				setText (m_requires, requires_str);
				setText (m_provides, provides_str);
			}

			if (m_contents) {  // patches -- "apply to"
				Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
				query->addCollection (package);
				m_contents->setList (Ypp::PkgQuery (Ypp::Package::PACKAGE_TYPE, query), 0);
			}

			gtk_image_clear (GTK_IMAGE (m_icon));
			GtkIconTheme *icons = gtk_icon_theme_get_default();
			GdkPixbuf *pixbuf = gtk_icon_theme_load_icon (icons,
				package->name().c_str(), 32, GtkIconLookupFlags (0), NULL);
			if (pixbuf) {
				gtk_image_set_from_pixbuf (GTK_IMAGE (m_icon), pixbuf);
				g_object_unref (G_OBJECT (pixbuf));
				gtk_widget_show (m_icon_frame);
			}
		}
		else {
			std::string description;
			if (packages.size() > 0) {
				description = "Selected:";
				description += "<ul>";
				for (int i = 0; packages.get (i); i++)
					description += "<li>" + packages.get (i)->name() + "</li>";
				description += "</ul>";
			}
			setText (m_description, description);
			if (m_filelist)  setText (m_filelist, "");
			if (m_changelog) setText (m_changelog, "");
			if (m_authors)   setText (m_authors, "");
			if (m_support)   setText (m_support, "");
			if (m_requires)  setText (m_requires, "");
			if (m_provides)  setText (m_provides, "");
			if (m_contents) {
				gtk_widget_hide (gtk_widget_get_ancestor (
					GTK_WIDGET (m_contents), GTK_TYPE_EXPANDER));
				m_contents->clear();
			}
		}

		m_versions->setPackages (packages);
		scrollTop();
	}

	void setPackage (Ypp::Package *package)
	{
		Ypp::PkgList packages;
		packages.append (package);
		setPackages (packages);
	}

private:
	void scrollTop()
	{
		GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW (m_scroll);
		GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment (scroll);
		YGUtils::scrollWidget (vadj, true);
	}

	// utilities:
	static GtkWidget *createIconWidget (GtkWidget **icon_widget, GtkWidget **icon_frame)
	{
		*icon_widget = gtk_image_new();

		GtkWidget *box, *align, *frame;
		box = gtk_event_box_new();
		gtk_container_add (GTK_CONTAINER (box), *icon_widget);
		gtk_container_set_border_width (GTK_CONTAINER (box), 2);

		frame = gtk_frame_new (NULL);
		gtk_container_add (GTK_CONTAINER (frame), box);

		align = gtk_alignment_new (0, 0, 0, 0);
		gtk_container_add (GTK_CONTAINER (align), frame);
		gtk_container_set_border_width (GTK_CONTAINER (align), 6);
		*icon_frame = align;
		return align;
	}

	static void setupScrollAsWhite (GtkWidget *scroll, GtkWidget *box)
	{
		struct inner {
			static gboolean expose_cb (GtkWidget *widget, GdkEventExpose *event)
			{
				cairo_t *cr = gdk_cairo_create (widget->window);
				GdkColor color = { 0, 255 << 8, 255 << 8, 255 << 8 };
				gdk_cairo_set_source_color (cr, &color);
				cairo_rectangle (cr, event->area.x, event->area.y,
						         event->area.width, event->area.height);
				cairo_fill (cr);
				cairo_destroy (cr);
				return FALSE;
			}
		};

		g_signal_connect (G_OBJECT (box), "expose-event",
			              G_CALLBACK (inner::expose_cb), NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroll), box);
	}

	static void appendExpander (GtkWidget *box, const char *label, GtkWidget *child)
	{
		std::string str = std::string ("<b>") + label + "</b>";
		GtkWidget *expander = gtk_expander_new (str.c_str());
		gtk_expander_set_use_markup (GTK_EXPANDER (expander), TRUE);
		gtk_container_add (GTK_CONTAINER (expander), child);
		gtk_box_pack_start (GTK_BOX (box), expander, FALSE, TRUE, 0);
	}

	static void setText (GtkWidget *text, const std::string &str)
	{
		ygtk_rich_text_set_text (YGTK_RICH_TEXT (text), str.c_str(), FALSE);
		GtkWidget *expander = gtk_widget_get_ancestor (text, GTK_TYPE_EXPANDER);
		if (expander)
			str.empty() ? gtk_widget_hide (expander) : gtk_widget_show (expander);
	}

	// callbacks
	static void link_pressed_cb (GtkWidget *text, const gchar *link, Impl *pThis)
	{
		if (!strncmp (link, "pkg://", 6)) {
			const gchar *pkg_name = link + 6;
			Ypp::Package *pkg = Ypp::get()->getPackages (Ypp::Package::PACKAGE_TYPE).find (pkg_name);
			if (pkg)
				pThis->setPackage (pkg);
			else {
				GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
					GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK, _("Package '%s' was not found."), pkg_name);
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
			}
		}
		else
			OPEN_URI (link);
	}
};

G_DEFINE_TYPE (YGtkDetailView, ygtk_detail_view, GTK_TYPE_SCROLLED_WINDOW)

static void ygtk_detail_view_init (YGtkDetailView *view)
{}

GtkWidget *ygtk_detail_view_new (gboolean onlineUpdate)
{
	YGtkDetailView *view = (YGtkDetailView *) g_object_new (YGTK_TYPE_DETAIL_VIEW, NULL);
	view->impl = new YGtkDetailView::Impl (GTK_WIDGET (view), onlineUpdate);
	return GTK_WIDGET (view);
}

static void ygtk_detail_view_finalize (GObject *object)
{
	G_OBJECT_CLASS (ygtk_detail_view_parent_class)->finalize (object);
	YGtkDetailView *view = YGTK_DETAIL_VIEW (object);
	delete view->impl;
	view->impl = NULL;
}

static void ygtk_detail_view_class_init (YGtkDetailViewClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = ygtk_detail_view_finalize;
}

void YGtkDetailView::setPackages (Ypp::PkgList packages)
{ impl->setPackages (packages); }

void YGtkDetailView::setPackage (Ypp::Package *package)
{ impl->setPackage (package); }

