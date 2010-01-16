/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkZyppView, Zypp GtkTreeView implementation */
// check the header file for information about this widget

/*
  Textdomain "yast2-gtk"
 */
#define YUILogComponent "gtk"
#include "config.h"
#include "ygtkpackageview.h"
#include "ygtktreeview.h"
#include "ygtktreemodel.h"
#include "ygtkcellrendererbutton.h"
#include "YGi18n.h"
#include "YGUtils.h"
#include <gtk/gtk.h>
#include <string.h>

extern bool status_col, action_col, action_col_as_button, action_col_as_check,
	action_col_label, version_col, colorful_rows, italicize_changed_row,
	golden_changed_row, single_line_rows;

//** Icons resources

struct PackageIcons {
	GdkPixbuf *installed, *installed_upgradable, *installed_locked,
	          *installed_upgradable_locked, *available, *available_locked,
	          *to_install, *to_install_upgrade, *to_remove, *to_auto_install,
	          *to_auto_remove;

	static PackageIcons *get()
	{  // let the system reclaim the mem after sw_single finishes
		static PackageIcons *icons = NULL;
		if (!icons)
			icons = new PackageIcons();
		return icons;
	}

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

	static GdkPixbuf *loadPixbuf (const char *icon)
	{ return YGUtils::loadPixbuf (std::string (DATADIR) + "/" + icon); }

	GdkPixbuf *getPixbuf (Ypp::Package *package)
	{
		bool locked = package->isLocked();
		bool auto_ = package->isAuto();
		if (package->toInstall()) {
			if (auto_)
				return to_auto_install;
			else {
				if (package->isInstalled())
					return to_install_upgrade;
				return to_install;
			}
		}
		else if (package->toRemove()) {
			if (auto_)
				return to_auto_remove;
			else
				return to_remove;
		}
		else if (package->hasUpgrade()) {
			if (locked)
				return installed_upgradable_locked;
			else
				return installed_upgradable;
		}
		else if (package->isInstalled()) {
			if (locked)
				return installed_locked;
			return installed;
		}
		if (locked)
			return available_locked;
		return available;
	}
};

//** Model

enum Property {
	// pixbuf
	ICON_PROP,
	// text
	NAME_PROP, SUMMARY_PROP, NAME_SUMMARY_PROP, REPOSITORY_PROP, SUPPORT_PROP,
	SIZE_PROP, INSTALLED_VERSION_PROP, AVAILABLE_VERSION_PROP,
	// checks
	TO_INSTALL_PROP, TO_UPGRADE_PROP, TO_REMOVE_PROP, TO_MODIFY_PROP,
	IS_INSTALLED_PROP,
	// internal
	STYLE_PROP, WEIGHT_PROP, SENSITIVE_PROP, CHECK_VISIBLE_PROP,
	FOREGROUND_PROP, VERSION_FOREGROUND_PROP, BACKGROUND_PROP, XPAD_PROP,
	ACTION_LABEL_PROP, ACTION_STOCK_PROP,
	// misc
	PTR_PROP, TOTAL_PROPS
};

static GType _columnType (int col)
{
	switch (col) {
		case ICON_PROP:
			return GDK_TYPE_PIXBUF;
		case NAME_PROP:
		case SUMMARY_PROP:
		case NAME_SUMMARY_PROP:
		case REPOSITORY_PROP:
		case SUPPORT_PROP:
		case SIZE_PROP:
		case INSTALLED_VERSION_PROP:
		case AVAILABLE_VERSION_PROP:
		case FOREGROUND_PROP:
		case VERSION_FOREGROUND_PROP:
		case BACKGROUND_PROP:
		case ACTION_LABEL_PROP:
		case ACTION_STOCK_PROP:
			return G_TYPE_STRING;
		case TO_INSTALL_PROP:
		case TO_UPGRADE_PROP:
		case TO_REMOVE_PROP:
		case TO_MODIFY_PROP:
		case IS_INSTALLED_PROP:
		case SENSITIVE_PROP:
		case CHECK_VISIBLE_PROP:
			return G_TYPE_BOOLEAN;
		case STYLE_PROP:
		case WEIGHT_PROP:
		case XPAD_PROP:
			return G_TYPE_INT;
		case PTR_PROP:
			return G_TYPE_POINTER;
	}
	return 0;
}

static void _getValueDefault (int col, GValue *value)
{
	switch (col) {
		case ICON_PROP:
			g_value_set_object (value, NULL);
			break;
		case NAME_PROP:
		case SUMMARY_PROP:
		case NAME_SUMMARY_PROP:
		case REPOSITORY_PROP:
		case SUPPORT_PROP:
		case SIZE_PROP:
		case INSTALLED_VERSION_PROP:
		case AVAILABLE_VERSION_PROP:
		case ACTION_LABEL_PROP:
		case ACTION_STOCK_PROP:
			g_value_set_string (value, g_strdup (""));
			break;
		case FOREGROUND_PROP:
		case VERSION_FOREGROUND_PROP:
		case BACKGROUND_PROP:
			g_value_set_string (value, NULL);
			break;
		case TO_INSTALL_PROP:
		case TO_UPGRADE_PROP:
		case TO_REMOVE_PROP:
		case TO_MODIFY_PROP:
		case IS_INSTALLED_PROP:
			g_value_set_boolean (value, FALSE);
			break;
		case STYLE_PROP:
			g_value_set_int (value, PANGO_STYLE_NORMAL);
			break;
		case WEIGHT_PROP:
			g_value_set_int (value, PANGO_WEIGHT_NORMAL);
			break;
		case XPAD_PROP:
			g_value_set_int (value, 0);
			break;
		case SENSITIVE_PROP:
			g_value_set_boolean (value, TRUE);
			break;
		case CHECK_VISIBLE_PROP:
			g_value_set_boolean (value, TRUE);
			break;
		case PTR_PROP:
			g_value_set_pointer (value, NULL);
			break;
	}
}

struct YGtkZyppModel : public YGtkTreeModel, Ypp::PkgList::Listener
{
	YGtkZyppModel()
	{}

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
	{ return TOTAL_PROPS; }

	virtual bool showEmptyEntry() const
	{ return true; }

	virtual GType columnType (int col) const
	{ return _columnType (col); }

	virtual void getValue (int row, int col, GValue *value)
	{
		if (row == -1) {
			switch (col) {
				case NAME_PROP:
					g_value_set_string (value, g_strdup (_("(No entries.)")));
					break;
				case STYLE_PROP:
					g_value_set_int (value, PANGO_STYLE_ITALIC);
					break;
				case CHECK_VISIBLE_PROP:
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
				case NAME_PROP:
					g_value_set_string (value, g_strdup (segment->header.c_str()));
					break;
				case NAME_SUMMARY_PROP: {
					std::string header (segment->header);
					header = "<big>" + header + "</big>";
					g_value_set_string (value, g_strdup (header.c_str()));
					break;
				}
				case WEIGHT_PROP:
					g_value_set_int (value, PANGO_WEIGHT_BOLD);
					break;
				case CHECK_VISIBLE_PROP:
					g_value_set_boolean (value, FALSE);
					break;
				case FOREGROUND_PROP:
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
				case NAME_PROP:
				case NAME_SUMMARY_PROP:
					g_value_set_string (value, g_strdup (segment->applyAll.c_str()));
					break;
				case BACKGROUND_PROP: {
#if 0
				case FOREGROUND_PROP:
					GtkStyle *style = gtk_widget_get_default_style();
					GdkColor *color = &style->bg [GTK_STATE_SELECTED];
					if (col == FOREGROUND_PROP)
						color = &style->fg [GTK_STATE_NORMAL];
					gchar *str = gdk_color_to_string (color);  // old: "lightblue"
					g_value_set_string (value, str);
#endif
					g_value_set_string (value, g_strdup ("lightblue"));
					break;
				}
				case FOREGROUND_PROP:
					g_value_set_string (value, g_strdup ("black"));
					break;
				case TO_INSTALL_PROP:
				case TO_UPGRADE_PROP:
				case TO_REMOVE_PROP:
				case TO_MODIFY_PROP: {
					bool modified = segment->list.modified();
					g_value_set_boolean (value, modified);
					break;
				}
				case ACTION_LABEL_PROP:
					g_value_set_string (value, g_strdup (_("All")));
					break;
				case ACTION_STOCK_PROP:
					g_value_set_string (value, g_strdup (GTK_STOCK_SELECT_ALL));
					break;
				default:
					_getValueDefault (col, value);
					break;
			}
			return;
		}

		Ypp::Package *package = segment->list.get (index);
		switch (col) {
			case ICON_PROP: {
				GdkPixbuf *pixbuf = 0;
				if (package->type() == Ypp::Package::PATTERN_TYPE) {
					std::string filename (package->icon());
					GtkIconTheme *icons = gtk_icon_theme_get_default();
					pixbuf = gtk_icon_theme_load_icon (icons,
						filename.c_str(), 32, GtkIconLookupFlags (0), NULL);
					if (!package->isInstalled()) {
						GdkPixbuf *_pixbuf = pixbuf;
						pixbuf = YGUtils::setOpacity (_pixbuf, 50, true);
						g_object_unref (_pixbuf);
					}
				}
				if (!pixbuf)
					pixbuf = PackageIcons::get()->getPixbuf (package);
				g_value_set_object (value, (GObject *) pixbuf);
				break;
			}
			case NAME_PROP: {
				std::string str (package->name());
				g_value_set_string (value, g_strdup (str.c_str()));
				break;
			}
			case SUMMARY_PROP: {
				std::string str (package->summary());
				g_value_set_string (value, g_strdup (str.c_str()));
				break;
			}
			case NAME_SUMMARY_PROP: {
				std::string str = package->name();
				std::string summary = package->summary();
				if (!summary.empty()) {
					summary = YGUtils::escapeMarkup (summary);
					str += "\n<small>" + summary + "</small>";
				}
				g_value_set_string (value, g_strdup (str.c_str()));
				break;
			}
			case REPOSITORY_PROP: {
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
			case SUPPORT_PROP:
				g_value_set_string (value, g_strdup (package->support().c_str()));
				break;
			case SIZE_PROP:
				g_value_set_string (value, g_strdup (package->size().c_str()));
				break;
			case INSTALLED_VERSION_PROP: {
				const Ypp::Package::Version *version = package->getInstalledVersion();
				if (version)
					g_value_set_string (value, g_strdup (version->number.c_str()));
				break;
			}
			case AVAILABLE_VERSION_PROP: {
				const Ypp::Package::Version *version = package->getAvailableVersion (0);
				if (version)
					g_value_set_string (value, g_strdup (version->number.c_str()));
				break;
			}
			case TO_INSTALL_PROP:
				g_value_set_boolean (value, package->toInstall());
				break;
			case TO_UPGRADE_PROP: {
				const Ypp::Package::Version *version = 0;
				if (package->toInstall (&version))
					g_value_set_boolean (value, version->cmp > 0);
				break;
			}
			case TO_REMOVE_PROP:
				g_value_set_boolean (value, package->toRemove());
				break;
			case TO_MODIFY_PROP:
				g_value_set_boolean (value, package->toModify());
				break;
			case IS_INSTALLED_PROP:  // if is-installed at the end
				bool installed;
				if (package->toInstall())
					installed = true;
				else if (package->toRemove())
					installed = false;
				else
					installed = package->isInstalled();
				g_value_set_boolean (value, installed);
				break;
			case SENSITIVE_PROP: {
				bool sensitive = !package->isLocked();
				g_value_set_boolean (value, sensitive);
				break;
			}
			case STYLE_PROP: {
				if (italicize_changed_row) {
					PangoStyle style = PANGO_STYLE_NORMAL;
					if (package->toModify())
						style = PANGO_STYLE_ITALIC;
					g_value_set_int (value, style);
				}
				else
					_getValueDefault (col, value);
				break;
			}
			case VERSION_FOREGROUND_PROP:
			case FOREGROUND_PROP: {
				const char *color = 0;
				if (colorful_rows) {
					if (!package->isInstalled())
						color = "darkgray";
					if (col == VERSION_FOREGROUND_PROP)
						if (package->hasUpgrade())
							color = "blue";
				}
				g_value_set_string (value, color);
				break;
			}
			case BACKGROUND_PROP: {
				const char *color = 0;
				if (golden_changed_row)
					if (package->toModify())
						color = "yellow";
				g_value_set_string (value, color);
				break;
			}
			case WEIGHT_PROP: {
				bool highlight = segment->list.highlight (package);
				int weight = highlight ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL;
				g_value_set_int (value, weight);
				break;
			}
			case XPAD_PROP: {
				int xpad = package->isAuto() ? 15 : 0;
				g_value_set_int (value, xpad);
				break;
			}
			case ACTION_LABEL_PROP: {
				const char *label;
				if (package->toModify())
					label = _("Undo");
				else if (package->isInstalled())
					label = _("Remove");
				else
					label = _("Add");
				g_value_set_string (value, g_strdup (label));
				break;
			}
			case ACTION_STOCK_PROP: {
				const char *stock;
				if (package->toModify())
					stock = GTK_STOCK_UNDO;
				else if (package->isInstalled())
					stock = GTK_STOCK_REMOVE;
				else
					stock = GTK_STOCK_ADD;
				g_value_set_string (value, g_strdup (stock));
				break;
			}
			case PTR_PROP:
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

static GtkTreeModel *ygtk_zypp_model_new()
{ return ygtk_tree_model_new (new YGtkZyppModel()); }

static void ygtk_zypp_model_append (GtkTreeModel *model,
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
	virtual int columnsNb() const { return TOTAL_PROPS; }
	virtual GType columnType (int col) const { return _columnType (col); }

	virtual void getValue (int row, int col, GValue *value)
	{
		switch (col) {
			case NAME_PROP:
				g_value_set_string (value, g_strdup (_("Query...")));
				break;
			case STYLE_PROP:
				g_value_set_int (value, PANGO_STYLE_ITALIC);
				break;
			case CHECK_VISIBLE_PROP:
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

struct Column {
	const char *header;
	std::string prop;
	bool visibleDefault, allowHide, sortable;
};
static const Column columns[] = {
	{ "Name", "name", true, false, true },
	{ "Version", "available-version", true, true, false },
	{ "Repository", "repository", false, true, false },
	{ "Support", "support", false, true, false },
	{ "Size", "size", false, true, true },
};
static int columns_size = sizeof (columns) / sizeof (Column);

static Property translateProperty (const std::string &prop)
{
	if (prop == "name")
		return single_line_rows ? NAME_PROP : NAME_SUMMARY_PROP;
	if (prop == "available-version")
		return AVAILABLE_VERSION_PROP;
	if (prop == "repository")
		return REPOSITORY_PROP;
	if (prop == "support")
		return SUPPORT_PROP;
	if (prop == "size")
		return SIZE_PROP;
	if (prop == "to-install")
		return TO_INSTALL_PROP;
	if (prop == "is-installed")
		return IS_INSTALLED_PROP;
	return (Property) 0;
}

struct YGtkPackageView::Impl
{
	Impl (GtkWidget *scroll, bool descriptiveTooltip)
	: m_listener (NULL), m_popup_hack (NULL), m_descriptiveTooltip (descriptiveTooltip),
	  m_model (NULL), m_modelId (0), m_activate_action (NONE_ACTION)
	{
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

		GtkTreeView *view = GTK_TREE_VIEW (m_view = ygtk_tree_view_new());
		gtk_tree_view_set_search_column (view, NAME_PROP);
		gtk_tree_view_set_fixed_height_mode (view, TRUE);
		gtk_tree_view_set_headers_visible (view, FALSE);
		gtk_tree_view_set_enable_tree_lines (view, TRUE);

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

		if (action_col) {
			if (action_col_as_button)
				appendButtonColumn (NULL, "to-install");
			else //if (action_col_as_check)
				appendCheckColumn ("is-installed"); //("to-install");
		}
		if (status_col)
			appendIconColumn (NULL, "icon");

		for (int i = 0; i < columns_size; i++) {
			const Column *column = &columns[i];
			bool visible = column->visibleDefault;
			if (column->prop == "available-version")
				visible = version_col;
			appendTextColumn (column->header, column->prop,
				visible, column->sortable);
		}
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
	Ypp::PkgList m_list;
	guint m_modelId;
	Action m_activate_action;

	// methods
	void setList (Ypp::PkgList list, const char *applyAllLabel)
	{
		m_list = list;
		setListImpl (list, applyAllLabel);
	}

	void setListImpl (Ypp::PkgList list, const char *applyAllLabel)
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

	void packList (const char *header, Ypp::PkgList list, const char *applyAllLabel)
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
			gtk_tree_model_get (model, &iter, PTR_PROP, &package, -1);
			gtk_tree_path_free (path);
			if (package)
				packages.append (package);
		}
		g_list_free (paths);
		return packages;
	}

	GtkTreeViewColumn *getColumn (const std::string &property)
	{
		GList *columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (m_view));
		for (GList *i = columns; i; i = i->next) {
			gchar *col_prop = (gchar *) g_object_get_data (G_OBJECT (i->data), "property");
			if (col_prop && property == col_prop)
				return (GtkTreeViewColumn *) i->data;
		}
		return NULL;
	}

	std::string getColumnProp (GtkTreeViewColumn *column)
	{
		gchar *prop = (gchar *) g_object_get_data (G_OBJECT (column), "property");
		return prop;
	}

	void setVisible (const std::string &property, bool visible)
	{ gtk_tree_view_column_set_visible (getColumn (property), visible); }

	bool isVisible (const std::string &property)
	{ return gtk_tree_view_column_get_visible (getColumn (property)); }

	void appendIconColumn (const char *header, const std::string &prop)
	{
		int col = ICON_PROP;  // 'prop' is ignored
		GtkTreeView *view = GTK_TREE_VIEW (m_view);
		if (header)
			gtk_tree_view_set_headers_visible (view, TRUE);
		GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
			int height = MAX (34, YGUtils::getCharsHeight (m_view, 2));
			gtk_cell_renderer_set_fixed_size (renderer, -1, height);
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (
			header, renderer, "pixbuf", col,
			"cell-background", BACKGROUND_PROP, NULL);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_fixed_width (column, 38);
		gtk_tree_view_append_column (view, column);
	}

	static Action columnAction (int col)
	{
		switch (col) {
			case TO_INSTALL_PROP:
			case TO_UPGRADE_PROP:
			default:
				return INSTALL_ACTION;
			case TO_REMOVE_PROP:
				return REMOVE_ACTION;
			case TO_MODIFY_PROP:
				return UNDO_ACTION;
			case IS_INSTALLED_PROP:
				return TOGGLE_ACTION;
		}
	}

	void appendCheckColumn (const std::string &prop)
	{
		int modelCol = translateProperty (prop);

		GtkTreeView *view = GTK_TREE_VIEW (m_view);
		GtkCellRenderer *renderer = gtk_cell_renderer_toggle_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (NULL,
			renderer, "active", modelCol, "visible", CHECK_VISIBLE_PROP,
			"sensitive", SENSITIVE_PROP,
			"cell-background", BACKGROUND_PROP, NULL);
		g_signal_connect (G_OBJECT (renderer), "toggled",
			              G_CALLBACK (renderer_toggled_cb), this);

		// it seems like GtkCellRendererToggle has no width at start, so fixed doesn't work
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_fixed_width (column, 25);
		gtk_tree_view_append_column (view, column);

		Action action = columnAction (modelCol);
		g_object_set_data (G_OBJECT (renderer), "action", GINT_TO_POINTER (action));
		if (m_activate_action == NONE_ACTION)
			m_activate_action = action;
		g_object_set_data (G_OBJECT (column), "status-tooltip", GINT_TO_POINTER (1));
	}

	void appendButtonColumn (const char *header, const std::string &prop)
	{
		int labelCol = ACTION_LABEL_PROP, stockCol = ACTION_STOCK_PROP;

		GtkTreeView *view = GTK_TREE_VIEW (m_view);
		GtkCellRenderer *renderer = ygtk_cell_renderer_button_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (
			header, renderer,// "active", modelCol,
			"visible", CHECK_VISIBLE_PROP,
			"sensitive", SENSITIVE_PROP,
			"cell-background", BACKGROUND_PROP,
			NULL);
		if (action_col_label)
			gtk_tree_view_column_add_attribute (column, renderer, "text", labelCol);
		gboolean button_images;
		g_object_get (G_OBJECT (gtk_settings_get_default()), "gtk-button-images", &button_images, NULL);
		bool show_icon = !action_col_label || button_images;
		if (show_icon)
			gtk_tree_view_column_add_attribute (column, renderer, "stock-id", stockCol);
		g_signal_connect (G_OBJECT (renderer), "toggled",
			              G_CALLBACK (renderer_toggled_cb), this);

		PangoRectangle rect;
		int width = 0;
		if (action_col_label) {
			const char *text[] = { _("Add"), _("Remove") };
			for (int i = 0; i < 2; i++) {
				PangoLayout *layout = gtk_widget_create_pango_layout (m_view, text[i]);
				pango_layout_get_pixel_extents (layout, NULL, &rect);
				width = MAX (width, rect.width);
				g_object_unref (G_OBJECT (layout));
			}
		}
		width += 16;
		if (show_icon) {
			int icon_width;
			gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (m_view),
				GTK_ICON_SIZE_MENU, &icon_width, NULL);
			width += icon_width;
		}

		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_fixed_width (column, width);
		gtk_tree_view_append_column (view, column);

		Action action = columnAction (TO_INSTALL_PROP);
		g_object_set_data (G_OBJECT (renderer), "action", GINT_TO_POINTER (action));
		if (m_activate_action == NONE_ACTION)
			m_activate_action = action;
		g_object_set_data (G_OBJECT (column), "status-tooltip", GINT_TO_POINTER (1));
	}

	void appendTextColumn (const char *header, const std::string &prop, bool visible, bool sortable, bool identAuto = false)
	{
		int col = translateProperty (prop);
		int size = 100;
		if (col == NAME_PROP || col == NAME_SUMMARY_PROP)
			size = -1;

		GtkTreeView *view = GTK_TREE_VIEW (m_view);
		if (header)
			gtk_tree_view_set_headers_visible (view, TRUE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		PangoEllipsizeMode ellipsize = PANGO_ELLIPSIZE_END;
		if (size >= 0 && col != NAME_SUMMARY_PROP)
			ellipsize = PANGO_ELLIPSIZE_MIDDLE;
		g_object_set (G_OBJECT (renderer), "ellipsize", ellipsize, NULL);
/*		gboolean reverse = gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL;
		if (reverse) {  // work-around: Pango ignored alignment flag on RTL
			gtk_widget_set_direction (m_view, GTK_TEXT_DIR_LTR);
			g_object_set (renderer, "alignment", PANGO_ALIGN_RIGHT, NULL);
		}*/
		const char *colType = col == NAME_SUMMARY_PROP ? "markup" : "text";
		GtkTreeViewColumn *column;
		column = gtk_tree_view_column_new_with_attributes (
			header, renderer, colType, col,
			"sensitive", SENSITIVE_PROP,
			"style", STYLE_PROP,
			"weight", WEIGHT_PROP,
			"cell-background", BACKGROUND_PROP,
			NULL);
		int foregroundCol = FOREGROUND_PROP;
		if (col == AVAILABLE_VERSION_PROP)
			foregroundCol = VERSION_FOREGROUND_PROP;
		gtk_tree_view_column_add_attribute (column, renderer, "foreground", foregroundCol);
		g_object_set_data_full (G_OBJECT (column), "property", g_strdup (prop.c_str()), g_free);
		if (identAuto)
			gtk_tree_view_column_add_attribute (column, renderer, "xpad", XPAD_PROP);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_resizable (column, TRUE);
//		gtk_tree_view_column_set_reorderable (column, TRUE);
		if (size >= 0)
			gtk_tree_view_column_set_fixed_width (column, size);
		else
			gtk_tree_view_column_set_expand (column, TRUE);
		gtk_tree_view_column_set_visible (column, visible);
		gtk_tree_view_column_set_clickable (column, sortable);
		if (sortable)
			g_signal_connect (G_OBJECT (column), "clicked",
			                  G_CALLBACK (column_clicked_cb), this);
		if (col == NAME_PROP || col == NAME_SUMMARY_PROP) {
			gtk_tree_view_column_set_sort_indicator (column, TRUE);
			gtk_tree_view_column_set_sort_order (column, GTK_SORT_ASCENDING);
		}
//		gtk_tree_view_insert_column (view, column, reverse ? 0 : -1);
		gtk_tree_view_append_column (view, column);
	}

	void appendEmptyColumn (int size)
	{
		GtkTreeView *view = GTK_TREE_VIEW (m_view);
		GtkTreeViewColumn *column = gtk_tree_view_column_new ();
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_fixed_width (column, size);
		gtk_tree_view_append_column (view, column);
	}

	void removeColumn (const char *header)
	{ gtk_tree_view_remove_column (GTK_TREE_VIEW (m_view), getColumn (header)); }

	bool hasColumn (const char *header)
	{ return getColumn (header); }

	void setRulesHint (bool hint)
	{ gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (m_view), TRUE); }

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
			static void show_column_cb (GtkCheckMenuItem *item, Impl *pThis)
			{
				GList *siblings = gtk_container_get_children (GTK_CONTAINER (
					gtk_widget_get_parent (GTK_WIDGET (item)))), *i;
				for (i = siblings; i; i = i->next)
					if (i->data == item)
						break;
				int index = g_list_position (siblings, i) + 1;
				g_list_free (siblings);
				bool visible = gtk_check_menu_item_get_active (item);
				GtkTreeViewColumn *column = gtk_tree_view_get_column (
					GTK_TREE_VIEW (pThis->m_view), index);
				gtk_tree_view_column_set_visible (column, visible);
			}
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
		inner::appendItem (menu, NULL, NULL, GTK_STOCK_SELECT_ALL,
		                   true, inner::select_all_cb, this);

		GtkWidget *item = gtk_menu_item_new_with_mnemonic (_("_Show Column"));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		GtkWidget *submenu = gtk_menu_new();
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
		for (int i = 0; i < columns_size; i++) {
			const Column *column = &columns[i];
			if (column->header) {
				GtkWidget *item = gtk_check_menu_item_new_with_label (column->header);
				gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item);
				gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), isVisible (column->prop));
				if (column->allowHide)
					g_signal_connect (G_OBJECT (item), "toggled",
						G_CALLBACK (inner::show_column_cb), this);
				else
					gtk_widget_set_sensitive (item, FALSE);
			}
		}

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
		gtk_tree_model_get (model, &iter, PTR_PROP, &package, -1);
		return package != NULL;
	}

	static void apply (Ypp::Package *package, Action action, bool enable)
	{
		if (package->toModify())
			package->undo();
		else
//		if (enable)
			switch (action) {
				case INSTALL_ACTION: package->install (0); break;
				case REMOVE_ACTION: package->remove(); break;
				case UNDO_ACTION: package->undo(); break;
				case TOGGLE_ACTION:
					if (package->isInstalled())
						package->remove();
					else
						package->install(0);
					break;
				case NONE_ACTION: break;
			}
/*		else
			package->undo();*/
	}

	static gboolean apply_iter_cb (GtkTreeModel *model,
		GtkTreePath *path, GtkTreeIter *iter, gpointer action)
	{
		Ypp::Package *package;
		gboolean enable;
		enable = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (model), "enable"));
		gtk_tree_model_get (model, iter, PTR_PROP, &package, -1);
		if (package)
			apply (package, (Action) GPOINTER_TO_INT (action), enable);
		return FALSE;
	}

	static void renderer_toggled_cb (GtkCellRenderer *renderer, gchar *path_str,
			                         Impl *pThis)
	{
		Ypp::Package *package = 0;
		GtkTreeView *view = GTK_TREE_VIEW (pThis->m_view);
		GtkTreeModel *model = gtk_tree_view_get_model (view);
		GtkTreeIter iter;
		gtk_tree_model_get_iter_from_string (model, &iter, path_str);
		gtk_tree_model_get (model, &iter, PTR_PROP, &package, -1);

		gboolean active;
		if (GTK_IS_CELL_RENDERER_TOGGLE (renderer))
			active = gtk_cell_renderer_toggle_get_active (GTK_CELL_RENDERER_TOGGLE (renderer));
		else
			active = ygtk_cell_renderer_button_get_active (YGTK_CELL_RENDERER_BUTTON (renderer));

		Action action = (Action) GPOINTER_TO_INT (g_object_get_data (G_OBJECT (renderer), "action"));
		if (package)
			apply (package, action, !active);
		else {
			if (!gtk_tree_model_iter_next (model, &iter)) {  // on apply-all
				g_object_set_data (G_OBJECT (model), "enable", GINT_TO_POINTER (!active));
				Ypp::get()->startTransactions();
				gtk_tree_model_foreach (model, apply_iter_cb, GINT_TO_POINTER (action));
				Ypp::get()->finishTransactions();
			}
		}
	}

	static void package_activated_cb (GtkTreeView *view, GtkTreePath *path,
	                                  GtkTreeViewColumn *column, Impl *pThis)
	{
		Ypp::PkgList packages = pThis->getSelected();
		switch (pThis->m_activate_action) {
			case INSTALL_ACTION: packages.install(); break;
			case REMOVE_ACTION: packages.remove(); break;
			case UNDO_ACTION: packages.undo(); break;
			case TOGGLE_ACTION:
				if (packages.installed())
					packages.remove();
				else
					packages.install();
				break;
			case NONE_ACTION: break;
		}
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
			gtk_tree_model_get (model, &iter, PTR_PROP, &package, -1);
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
				if (g_object_get_data (G_OBJECT (column), "status-tooltip")) {
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
					gtk_tree_model_get (model, &iter, ICON_PROP, &pixbuf, -1);
				if (pixbuf) {
					gtk_tooltip_set_icon (tooltip, pixbuf);
					g_object_unref (G_OBJECT (pixbuf));
				}
			}
			return TRUE;
		}
		return FALSE;
	}

	static void column_clicked_cb (GtkTreeViewColumn *column, Impl *pThis)
	{
fprintf (stderr, "column clicked\n");
		GList *columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (pThis->m_view));
		for (GList *i = columns; i; i = i->next) {
			GtkTreeViewColumn *col = (GtkTreeViewColumn *) i->data;
			if (col != column)
				gtk_tree_view_column_set_sort_indicator (col, FALSE);
		}

		GtkSortType sort;
		if (gtk_tree_view_column_get_sort_indicator (column)) {
			GtkSortType sort = gtk_tree_view_column_get_sort_order (column);
			if (sort == GTK_SORT_ASCENDING)
				sort = GTK_SORT_DESCENDING;
			else
				sort = GTK_SORT_ASCENDING;
		}
		else
			sort = GTK_SORT_ASCENDING;
		gtk_tree_view_column_set_sort_indicator (column, TRUE);
		gtk_tree_view_column_set_sort_order (column, sort);
fprintf (stderr, "column indicator: %d\n", gtk_tree_view_column_get_sort_indicator (column));
		std::string prop = pThis->getColumnProp (column);
		Ypp::PkgList l (pThis->m_list);
		if (prop != "name")
			l = Ypp::PkgSort (pThis->m_list, prop.c_str(), sort == GTK_SORT_ASCENDING);
		pThis->setListImpl (l, NULL);
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

void YGtkPackageView::packList (const char *header, Ypp::PkgList list, const char *applyAllLabel)
{ impl->packList (header, list, applyAllLabel); }

void YGtkPackageView::clear()
{ impl->clear(); }

void YGtkPackageView::setVisible (const std::string &property, bool visible)
{ impl->setVisible (property, visible); }

bool YGtkPackageView::isVisible (const std::string &property)
{ return impl->isVisible (property); }

void YGtkPackageView::setRulesHint (bool hint)
{ impl->setRulesHint (hint); }

void YGtkPackageView::setActivateAction (Action action)
{ impl->m_activate_action = action; }

void YGtkPackageView::setListener (Listener *listener)
{ impl->m_listener = listener; }

Ypp::PkgList YGtkPackageView::getSelected()
{ return impl->getSelected(); }

