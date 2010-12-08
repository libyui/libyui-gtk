/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* YGtkPkgFilterView, several zypp attribute query criteria implementations */
// check the header file for information about this widget

/*
  Textdomain "gtk"
 */

#include "YGi18n.h"
#include "YGUI.h"
#include "config.h"
#include "YGUtils.h"
#include "ygtkpkgfilterview.h"
#include "ygtkpkglistview.h"
#include "YGPackageSelector.h"
#include <gtk/gtk.h>
#include "ygtktreeview.h"
#include "ygtkcellrenderertext.h"

// List store abstract

struct YGtkPkgFilterModel::Impl {
	GtkTreeModel *filter, *model;

	int convertlIterToStoreRow (GtkTreeIter *iter)
	{
		GtkTreeIter _iter;
		gtk_tree_model_filter_convert_iter_to_child_iter (
			GTK_TREE_MODEL_FILTER (filter), &_iter, iter);

		GtkTreePath *path = gtk_tree_model_get_path (model, &_iter);
		int row = gtk_tree_path_get_indices (path)[0];
		gtk_tree_path_free (path);
		return row;
	}
};

YGtkPkgFilterModel::YGtkPkgFilterModel()
: impl (new Impl())
{
	GtkListStore *store = gtk_list_store_new (TOTAL_COLUMNS, GDK_TYPE_PIXBUF,
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_INT,
		G_TYPE_POINTER);
	impl->model = GTK_TREE_MODEL (store);

	impl->filter = gtk_tree_model_filter_new (impl->model, NULL);
	gtk_tree_model_filter_set_visible_column (
		GTK_TREE_MODEL_FILTER (impl->filter), VISIBLE_COLUMN);
	g_object_unref (G_OBJECT (impl->model));
}

YGtkPkgFilterModel::~YGtkPkgFilterModel()
{ g_object_unref (G_OBJECT (impl->filter)); }

GtkTreeModel *YGtkPkgFilterModel::getModel()
{ return impl->filter; }

struct UpdateData {
	YGtkPkgFilterModel *pThis;
	Ypp::List *list;
	UpdateData (YGtkPkgFilterModel *pThis, Ypp::List *list)
	: pThis (pThis), list (list) {}
};

static gboolean update_list_cb (GtkTreeModel *model,
	GtkTreePath *path, GtkTreeIter *iter, gpointer _data)
{
	gchar *text;
	UpdateData *data = (UpdateData *) _data;
	gpointer mdata;
	gtk_tree_model_get (model, iter, YGtkPkgFilterModel::TEXT_COLUMN, &text,
		YGtkPkgFilterModel::DATA_COLUMN, &mdata, -1);

	bool separator = !(*text);
	g_free (text);
	if (separator) return FALSE;

	int row = gtk_tree_path_get_indices (path)[0];
	if (row == 0 && data->pThis->firstRowIsAll())
		data->pThis->setRowCount (0, data->list->size());
	else
		data->pThis->updateRow (*data->list, row, mdata);
	return FALSE;
}

void YGtkPkgFilterModel::updateList (Ypp::List list)
{
	if (!begsUpdate()) return;
	UpdateData data (this, &list);
	gtk_tree_model_foreach (impl->model, update_list_cb, &data);
}

bool YGtkPkgFilterModel::writeQuery (Ypp::PoolQuery &query, GtkTreeIter *iter)
{
	gpointer data;
	gtk_tree_model_get (impl->filter, iter, DATA_COLUMN, &data, -1);

	int row = impl->convertlIterToStoreRow (iter);
	if (row == 0 && firstRowIsAll())
		return false;
	return writeRowQuery (query, row, data);
}

GtkWidget *YGtkPkgFilterModel::createToolbox (GtkTreeIter *iter)
{
	int row = impl->convertlIterToStoreRow (iter);
	return createToolboxRow (row);
}

void YGtkPkgFilterModel::addRow (const char *icon, const char *text,
	bool enabled, gpointer data, bool defaultVisible)
{
	// we use cell-render-pixbuf's "pixbuf" rather than "icon-name" so we
	// can use a fixed size
	GdkPixbuf *pixbuf = 0;
	if (icon) {
		if (!strcmp (icon, "empty")) {
			pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 32, 32);
			gdk_pixbuf_fill (pixbuf, 0xffffff00);
		}
		else
			pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(),
				icon, 32, GtkIconLookupFlags (0), NULL);
	}

	int weight = PANGO_WEIGHT_NORMAL;
	if (firstRowIsAll() && gtk_tree_model_iter_n_children (impl->model, NULL) == 0)
		weight = PANGO_WEIGHT_BOLD;

	GtkTreeIter iter;
	GtkListStore *store = GTK_LIST_STORE (impl->model);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, ICON_COLUMN, pixbuf, TEXT_COLUMN, text,
		COUNT_NUMBER_COLUMN, "", ENABLED_COLUMN, enabled, VISIBLE_COLUMN, defaultVisible,
		DATA_COLUMN, data, WEIGHT_COLUMN, weight, -1);

	if (pixbuf) g_object_unref (pixbuf);
}

void YGtkPkgFilterModel::addSeparator()
{ addRow (NULL, "", true, NULL); }

void YGtkPkgFilterModel::setRowCount (int row, int count)
{
	GtkListStore *store = GTK_LIST_STORE (impl->model);
	GtkTreeIter iter;
	gtk_tree_model_iter_nth_child (impl->model, &iter, NULL, row);

	gchar *str = g_strdup_printf ("%d", count);
	gtk_list_store_set (store, &iter, COUNT_NUMBER_COLUMN, str,
		VISIBLE_COLUMN, row == 0 || count > 0, -1);
	g_free (str);
}

// Status

struct YGtkPkgStatusModel::Impl : public Ypp::SelListener {
	Impl (YGtkPkgStatusModel *parent)
	: parent (parent), list (0)
	{ Ypp::addSelListener (this); }

	~Impl()
	{ Ypp::removeSelListener (this); }

	virtual void selectableModified()
	{  // make sure to update the "Modified" row on packages change
		parent->updateRow (list, modifiedRow(), NULL);
	}

	YGtkPkgStatusModel *parent;

	static int modifiedRow()
	{ return YGPackageSelector::get()->onlineUpdateMode() ? 2 : 5; }

	Ypp::List list;
};

static Ypp::StatusMatch::Status rowToStatus (int row)
{
	if (YGPackageSelector::get()->onlineUpdateMode())
		switch (row) {
			case 0: return Ypp::StatusMatch::NOT_INSTALLED;
			case 1: return Ypp::StatusMatch::IS_INSTALLED;
			case 2: return Ypp::StatusMatch::TO_MODIFY;
		}
	else
		switch (row) {
			case 1: return Ypp::StatusMatch::NOT_INSTALLED;
			case 2: return Ypp::StatusMatch::IS_INSTALLED;
			case 3: return Ypp::StatusMatch::HAS_UPGRADE;
			case 4: return Ypp::StatusMatch::IS_LOCKED;
			case 5: return Ypp::StatusMatch::TO_MODIFY;
		}
	return (Ypp::StatusMatch::Status) 0;
}

YGtkPkgStatusModel::YGtkPkgStatusModel()
: impl (new Impl (this))
{
	if (YGPackageSelector::get()->onlineUpdateMode()) {
		// Translators: this refers to the package status
		addRow (NULL, _("Available"), true, 0);
		// Translators: this refers to the package status
		addRow (NULL, _("Installed"), true, 0);
		// Translators: this refers to the package status
		addRow (NULL, _("Modified"), true, 0, false);
	}
	else {
		// Translators: "Any status" may be translated as "All statuses" (whichever's smaller)
		addRow (NULL, _("All packages"), true, 0);
		// Translators: this refers to the package status
		addRow (NULL, _("Not installed"), true, 0);
		addRow (NULL, _("Installed"), true, 0);
		// Translators: refers to package status: may be translated as "Upgrade"
		addRow (NULL, _("Upgrades"), true, 0, false);
		// Translators: this refers to the package status
		addRow (NULL, _("Locked"), true, 0, false);
		addRow (NULL, _("Modified"), true, 0, false);
	}
}

YGtkPkgStatusModel::~YGtkPkgStatusModel()
{ delete impl; }

bool YGtkPkgStatusModel::firstRowIsAll()
{ return !YGPackageSelector::get()->onlineUpdateMode(); }

void YGtkPkgStatusModel::updateRow (Ypp::List list, int row, gpointer data)
{
	impl->list = list;
	Ypp::StatusMatch match (rowToStatus (row));
	setRowCount (row, list.count (&match));
}

bool YGtkPkgStatusModel::writeRowQuery (Ypp::PoolQuery &query, int row, gpointer data)
{
	query.addCriteria (new Ypp::StatusMatch (rowToStatus (row)));
	return true;
}

static void upgrade_patches_clicked_cb (GtkButton *button, YGtkPkgStatusModel *pThis)
{
	Ypp::startTransactions();
	for (int i = 0; i < pThis->impl->list.size(); i++) {
		Ypp::Selectable sel = pThis->impl->list.get (i);
		Ypp::Package pkg (sel);
		if (sel.hasUpgrade() && pkg.isCandidatePatch())
			sel.install();
	}
	Ypp::finishTransactions();
}

static void upgrade_all_clicked_cb (GtkButton *button, YGtkPkgStatusModel *pThis)
{
	Ypp::startTransactions();
	for (int i = 0; i < pThis->impl->list.size(); i++) {
		Ypp::Selectable sel = pThis->impl->list.get (i);
		if (sel.hasUpgrade())
			sel.install();
	}
	Ypp::finishTransactions();
}

struct PkgHasPatchMatch : public Ypp::Match {
	virtual bool match (Ypp::Selectable &sel)
	{
		Ypp::Package pkg (sel);
		return pkg.isCandidatePatch();
	}
};

GtkWidget *YGtkPkgStatusModel::createToolboxRow (int row)
{  // "Upgrade All" button
	if (row == 3 && !YGPackageSelector::get()->onlineUpdateMode()) {
		PkgHasPatchMatch match;
		bool hasPatches = impl->list.count (&match);

		GtkWidget *hbox = gtk_hbox_new (FALSE, 6), *button, *icon;

		// Translators: this is shown next to the "Upgrade All" button
		button = gtk_button_new_with_label (_("Only Upgrade Patches"));
		gtk_widget_set_sensitive (button, hasPatches);
		g_signal_connect (G_OBJECT (button), "clicked",
		                  G_CALLBACK (upgrade_patches_clicked_cb), this);
		gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);

		button = gtk_button_new_with_label (_("Upgrade All"));
		icon = gtk_image_new_from_stock (GTK_STOCK_GO_UP, GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image (GTK_BUTTON (button), icon);
		g_signal_connect (G_OBJECT (button), "clicked",
		                  G_CALLBACK (upgrade_all_clicked_cb), this);
		gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);

		gtk_widget_show_all (hbox);
		return hbox;
	}
	return NULL;
}

// PackageKit-based Group

struct PKGroup {
	const char *name, *icon;
	YPkgGroupEnum id;

	PKGroup (YPkgGroupEnum id) : id (id) {
		name = zypp_tag_group_enum_to_localised_text (id);
		icon = zypp_tag_enum_to_icon (id);
	}
	bool operator < (const PKGroup &other) const
	{ return strcmp (this->name, other.name) < 0; }
};

YGtkPkgPKGroupModel::YGtkPkgPKGroupModel()
{
	std::set <PKGroup> groups;
	for (int i = 0; i < YPKG_GROUP_UNKNOWN; i++)
		groups.insert (PKGroup ((YPkgGroupEnum) i));

	addRow ("empty", _("All packages"), true, 0);
	for (std::set <PKGroup>::const_iterator it = groups.begin();
	     it != groups.end(); it++)
		addRow (it->icon, it->name, true, GINT_TO_POINTER (((int)it->id)+1));

	for (int i = YPKG_GROUP_UNKNOWN; i < YPKG_GROUP_TOTAL; i++) {
#if ZYPP_VERSION >= 6030004
		if (i == YPKG_GROUP_MULTIVERSION && zypp::sat::Pool::instance().multiversionEmpty())
#else
		if (i == YPKG_GROUP_MULTIVERSION)
#endif
			continue;
		if (i == YPKG_GROUP_ORPHANED) continue;
		std::string name = zypp_tag_group_enum_to_localised_text ((YPkgGroupEnum) i);
		const char *icon = zypp_tag_enum_to_icon ((YPkgGroupEnum) i);
		if (i == YPKG_GROUP_RECENT)
			// Translators: when "upload" isn't easy to translate, translate this as "7 days old"
			name += std::string ("\n<small>") + _("(uploaded last 7 days)") + "</small>";
		addRow (icon, name.c_str(), true, GINT_TO_POINTER (i+1));
		if (i == YPKG_GROUP_UNKNOWN)
			addSeparator();
	}
}

void YGtkPkgPKGroupModel::updateRow (Ypp::List list, int row, gpointer data)
{
	YPkgGroupEnum id = (YPkgGroupEnum) (GPOINTER_TO_INT (data)-1);
	Ypp::PKGroupMatch match (id);
	setRowCount (row, list.count (&match));
}

bool YGtkPkgPKGroupModel::writeRowQuery (Ypp::PoolQuery &query, int row, gpointer data)
{
	YPkgGroupEnum id = (YPkgGroupEnum) (GPOINTER_TO_INT (data)-1);
	query.addCriteria (new Ypp::PKGroupMatch (id));
	return true;
}

// Repository

struct YGtkPkgRepositoryModel::Impl {
	// cannot store as point at the list-store
	std::vector <Ypp::Repository> repos;
	Ypp::Repository *selected;

	Impl() : selected (NULL)
	{ repos.reserve (zyppPool().knownRepositoriesSize()); }
};

YGtkPkgRepositoryModel::YGtkPkgRepositoryModel()
: impl (new Impl())
{
	addRow (NULL, _("All repositories"), true, 0);

	for (zypp::ResPoolProxy::repository_iterator it = zyppPool().knownRepositoriesBegin();
	     it != zyppPool().knownRepositoriesEnd(); it++) {
		Ypp::Repository repo (*it);
		const char *icon = getRepositoryStockIcon (repo);
		std::string label (getRepositoryLabel (repo));
		addRow (icon, label.c_str(), true, GINT_TO_POINTER (1));
		impl->repos.push_back (repo);
	}

	zypp::RepoManager manager;
	std::list <zypp::RepoInfo> known_repos = manager.knownRepositories();
	for (std::list <zypp::RepoInfo>::const_iterator it = known_repos.begin();
	     it != known_repos.end(); it++) {
		Ypp::Repository repo (*it);
		if (!repo.enabled()) {
			const char *icon = getRepositoryStockIcon (repo);
			std::string label (getRepositoryLabel (repo));
			addRow (icon, label.c_str(), false, GINT_TO_POINTER (0));
		}
	}

	addRow (GTK_STOCK_MISSING_IMAGE, _("None"), true, GINT_TO_POINTER (2));
}

YGtkPkgRepositoryModel::~YGtkPkgRepositoryModel()
{ delete impl; }

void YGtkPkgRepositoryModel::updateRow (Ypp::List list, int row, gpointer data)
{
	if (GPOINTER_TO_INT (data) == 1) {  // normal repo
		Ypp::Repository &repo = impl->repos[row-1];
		bool isSystem = repo.isSystem();
		int count = 0;
		for (int i = 0; i < list.size(); i++) {
			Ypp::Selectable sel = list.get (i);
			if (isSystem) {
				if (sel.isInstalled())
					count++;
			}
			else for (int i = 0; i < sel.totalVersions(); i++) {
				Ypp::Version version = sel.version (i);
				if (!version.isInstalled()) {
					Ypp::Repository pkg_repo = version.repository();
					if (repo == pkg_repo) {
						count++;
						break;
					}
				}
			}
		}
		setRowCount (row, count);
	}
	else if (GPOINTER_TO_INT (data) == 2) {  // 'none'
		Ypp::PKGroupMatch match (YPKG_GROUP_ORPHANED);
		setRowCount (row, list.count (&match));
	}
}

bool YGtkPkgRepositoryModel::writeRowQuery (Ypp::PoolQuery &query, int row, gpointer data)
{
	impl->selected = 0;
	if (GPOINTER_TO_INT (data) == 1) {
		Ypp::Repository &repo = impl->repos[row-1];
		query.addRepository (repo);
		impl->selected = &repo;
	}
	else if (GPOINTER_TO_INT (data) == 2)
		query.addCriteria (new Ypp::PKGroupMatch (YPKG_GROUP_ORPHANED));
	return true;
}

static void sync_toolbox_buttons (Ypp::Repository *repo, GtkWidget *box)
{
	GtkWidget *button, *undo;
	GList *children = gtk_container_get_children (GTK_CONTAINER (box));
	button = (GtkWidget *) g_list_nth_data (children, 0);
	undo = (GtkWidget *) g_list_nth_data (children, 1);
	g_list_free (children);

	if (zypp::getZYpp()->resolver()->upgradingRepo (repo->zyppRepo())) {
		gtk_widget_set_sensitive (button, FALSE);
		gtk_widget_show (undo);
	}
	else {
		gtk_widget_set_sensitive (button, TRUE);
		gtk_widget_hide (undo);
	}
}

static void switch_repo_status (Ypp::Repository *repo)
{
	ZyppRepository zrepo = repo->zyppRepo();
	if (zypp::getZYpp()->resolver()->upgradingRepo (zrepo))
		zypp::getZYpp()->resolver()->removeUpgradeRepo (zrepo);
	else
		zypp::getZYpp()->resolver()->addUpgradeRepo (zrepo);
}

static void switch_clicked_cb (GtkButton *button, YGtkPkgRepositoryModel *pThis)
{
	Ypp::Repository *repo = pThis->impl->selected;
	switch_repo_status (repo);
	if (!Ypp::runSolver())  // on solver cancel -- switch back
		switch_repo_status (repo);
	sync_toolbox_buttons (repo, gtk_widget_get_parent (GTK_WIDGET (button)));
}

GtkWidget *YGtkPkgRepositoryModel::createToolboxRow (int row)
{
	if (row > 0 && impl->selected) {
		if (impl->selected->isOutdated()) {
			GtkWidget *label = gtk_label_new (
				_("Repository not refreshed in a long time."));
			gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
			YGUtils::setWidgetFont (label,
				PANGO_STYLE_ITALIC, PANGO_WEIGHT_NORMAL, PANGO_SCALE_MEDIUM);
			GtkWidget *icon = gtk_image_new_from_stock (
				GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_BUTTON);
			GtkWidget *hbox = gtk_hbox_new (FALSE, 4);
			gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
			gtk_widget_show_all (hbox);
			return hbox;
		}
		else if (!impl->selected->isSystem()) {
			GtkWidget *hbox = gtk_hbox_new (FALSE, 6), *button;
			button = gtk_button_new_from_stock (GTK_STOCK_UNDO);
			g_signal_connect (G_OBJECT (button), "clicked",
				              G_CALLBACK (switch_clicked_cb), this);
			gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, TRUE, 0);
			button = gtk_button_new_with_label (
				_("Switch installed packages to the versions in this repository"));
			gtk_button_set_image (GTK_BUTTON (button),
				gtk_image_new_from_stock (GTK_STOCK_REFRESH, GTK_ICON_SIZE_BUTTON));
			g_signal_connect (G_OBJECT (button), "clicked",
				              G_CALLBACK (switch_clicked_cb), this);
			gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, TRUE, 0);
			gtk_widget_show_all (hbox);
			sync_toolbox_buttons (impl->selected, hbox);
			return hbox;
		}
	}
	return NULL;
}

static void edit_clicked_cb (GtkWidget *button, YGtkPkgRepositoryModel *pThis)
{ YGPackageSelector::get()->showRepoManager(); }

GtkWidget *YGtkPkgRepositoryModel::createInternalToolbox()
{
	if (!YGPackageSelector::get()->repoMgrEnabled()) return NULL;
	GtkWidget *button = gtk_button_new_with_label (_("Edit Repositories"));
	GtkWidget *icon = gtk_image_new_from_stock (GTK_STOCK_EDIT, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image (GTK_BUTTON (button), icon);
	g_signal_connect (G_OBJECT (button), "clicked",
	                  G_CALLBACK (edit_clicked_cb), this);

	GtkWidget *align = gtk_alignment_new (0, 0, 0, 1);
	gtk_container_add (GTK_CONTAINER (align), button);
	return align;
}

GtkWidget *YGtkPkgRepositoryModel::createInternalPopup()
{
	if (!YGPackageSelector::get()->repoMgrEnabled()) return NULL;
	GtkWidget *menu = gtk_menu_new();
	GtkWidget *item = gtk_image_menu_item_new_with_mnemonic (_("Edit Repositories"));
	GtkWidget *icon = gtk_image_new_from_stock (GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), icon);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (G_OBJECT (item), "activate",
	                  G_CALLBACK (edit_clicked_cb), this);
	return menu;
}

// Support

YGtkPkgSupportModel::YGtkPkgSupportModel()
{
	addRow (NULL, _("All packages"), true, 0);
	for (int i = 0; i < Ypp::Package::supportTotal(); i++)
		addRow (NULL, Ypp::Package::supportSummary (i).c_str(), true, 0, false);
}

void YGtkPkgSupportModel::updateRow (Ypp::List list, int row, gpointer data)
{
	Ypp::SupportMatch match (row-1);
	setRowCount (row, list.count (&match));
}

bool YGtkPkgSupportModel::writeRowQuery (Ypp::PoolQuery &query, int row, gpointer data)
{
	query.addCriteria (new Ypp::SupportMatch (row-1));
	return true;
}

// Priority

YGtkPkgPriorityModel::YGtkPkgPriorityModel()
{
		// Translators: "Any priority" may be translated as "All priorities" (whichever's smaller)
	addRow (NULL, _("All patches"), true, 0);
	for (int i = 0; i < Ypp::Patch::priorityTotal(); i++)
		addRow (NULL, Ypp::Patch::prioritySummary (i), true, 0, false);
}

void YGtkPkgPriorityModel::updateRow (Ypp::List list, int row, gpointer data)
{
	Ypp::PriorityMatch match (row-1);
	setRowCount (row, list.count (&match));
}

bool YGtkPkgPriorityModel::writeRowQuery (Ypp::PoolQuery &query, int row, gpointer data)
{
	query.addCriteria (new Ypp::PriorityMatch (row-1));
	return true;
}

// View widget

struct YGtkPkgFilterView::Impl {
	GtkWidget *box, *scroll, *view;
	YGtkPkgFilterModel *model;

	Impl (YGtkPkgFilterModel *model) : model (model) {}
	~Impl() { delete model; }
};

static gboolean tree_selection_possible_cb (GtkTreeSelection *selection,
	GtkTreeModel *model, GtkTreePath *path, gboolean is_selected, gpointer data)
{
	GtkTreeIter iter;
	gtk_tree_model_get_iter (model, &iter, path);
	int col = GPOINTER_TO_INT (data);
	gboolean val;
	gtk_tree_model_get (model, &iter, col, &val, -1);
	return val;
}

static void selection_changed_cb (GtkTreeSelection *selection, YGtkPkgFilterView *pThis)
{
	if (gtk_tree_selection_get_selected (selection, NULL, NULL))
		pThis->notify();
}

static void right_click_cb (YGtkTreeView *view, gboolean outreach, YGtkPkgFilterView *pThis)
{
	GtkWidget *menu = pThis->impl->model->createInternalPopup();
	if (menu) ygtk_tree_view_popup_menu (view, menu);
}

YGtkPkgFilterView::YGtkPkgFilterView (YGtkPkgFilterModel *model)
: YGtkPkgQueryWidget(), impl (new Impl (model))
{
	bool updates = model->begsUpdate();
	impl->view = ygtk_tree_view_new (NULL);
	gtk_tree_view_set_model (GTK_TREE_VIEW (impl->view), model->getModel());
	GtkTreeView *view = GTK_TREE_VIEW (impl->view);
	gtk_tree_view_set_headers_visible (view, FALSE);
	gtk_tree_view_set_search_column (view, YGtkPkgFilterModel::TEXT_COLUMN);
	if (updates)
		gtk_tree_view_set_tooltip_column (view, YGtkPkgFilterModel::TEXT_COLUMN);
	gtk_tree_view_set_enable_tree_lines (view, TRUE);
	gtk_tree_view_set_row_separator_func (view, YGUtils::empty_row_is_separator_cb,
		GINT_TO_POINTER (YGtkPkgFilterModel::TEXT_COLUMN), NULL);
	gtk_tree_view_expand_all (view);
	g_signal_connect (G_OBJECT (view), "right-click",
		              G_CALLBACK (right_click_cb), this);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	if (model->hasIconCol()) {
		renderer = gtk_cell_renderer_pixbuf_new();
		column = gtk_tree_view_column_new_with_attributes (NULL,
			renderer, "pixbuf", YGtkPkgFilterModel::ICON_COLUMN,
			"sensitive", YGtkPkgFilterModel::ENABLED_COLUMN, NULL);
		ygtk_tree_view_append_column (YGTK_TREE_VIEW (view), column);
	}
	renderer = ygtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		NULL, renderer, "markup", YGtkPkgFilterModel::TEXT_COLUMN,
		"sensitive", YGtkPkgFilterModel::ENABLED_COLUMN,
		"weight", YGtkPkgFilterModel::WEIGHT_COLUMN, NULL);
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_tree_view_column_set_expand (column, TRUE);
	ygtk_tree_view_append_column (YGTK_TREE_VIEW (view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		NULL, renderer, "text", YGtkPkgFilterModel::COUNT_NUMBER_COLUMN,
		"sensitive", YGtkPkgFilterModel::ENABLED_COLUMN,
		"weight", YGtkPkgFilterModel::WEIGHT_COLUMN, NULL);
	g_object_set (G_OBJECT (renderer), "xalign", 1.0,
		"scale", PANGO_SCALE_SMALL, "foreground", "#8c8c8c", NULL);
	ygtk_tree_view_append_column (YGTK_TREE_VIEW (view), column);

	GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
	gtk_tree_selection_set_select_function (selection, tree_selection_possible_cb,
		GINT_TO_POINTER (YGtkPkgFilterModel::ENABLED_COLUMN), NULL);
	g_signal_connect (G_OBJECT (selection), "changed",
	                  G_CALLBACK (selection_changed_cb), this);
	clearSelection();

	impl->scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (impl->scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (
		GTK_SCROLLED_WINDOW (impl->scroll), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (impl->scroll), impl->view);

	GtkWidget *toolbox = model->createInternalToolbox();
	if (toolbox) {
		impl->box = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (impl->box), impl->scroll, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (impl->box), toolbox, FALSE, TRUE, 0);
	}
	else
		impl->box = impl->scroll;
	gtk_widget_show_all (impl->box);
}

YGtkPkgFilterView::~YGtkPkgFilterView()
{ delete impl; }

GtkWidget *YGtkPkgFilterView::getWidget()
{ return impl->box; }

bool YGtkPkgFilterView::begsUpdate()
{ return impl->model->begsUpdate(); }

void YGtkPkgFilterView::updateList (Ypp::List list)
{
	impl->model->updateList (list);

	// always ensure some row is selected
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (impl->view));
	GtkTreeModel *model;
	if (!gtk_tree_selection_get_selected (selection, &model, NULL)) {
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter_first (model, &iter)) {
			g_signal_handlers_block_by_func (selection, (gpointer) selection_changed_cb, this);
			gtk_tree_selection_select_iter (selection, &iter);
			g_signal_handlers_unblock_by_func (selection, (gpointer) selection_changed_cb, this);
		}
	}
}

void YGtkPkgFilterView::clearSelection()
{ select (0); /* select 1st row, so some row is selected */ }

bool YGtkPkgFilterView::writeQuery (Ypp::PoolQuery &query)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (impl->view));
	if (gtk_tree_selection_get_selected (selection, &model, &iter))
		return impl->model->writeQuery (query, &iter);
	return false;
}

GtkWidget *YGtkPkgFilterView::createToolbox()
{
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (impl->view));
	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
		return impl->model->createToolbox (&iter);
	return NULL;
}

void YGtkPkgFilterView::select (int row)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (impl->view));
	g_signal_handlers_block_by_func (selection, (gpointer) selection_changed_cb, this);
	GtkTreeIter iter;
	if (row >= 0) {
		gtk_tree_model_iter_nth_child (impl->model->getModel(), &iter, NULL, row);
		gtk_tree_selection_select_iter (selection, &iter);
	}
	else
		gtk_tree_selection_unselect_all (selection);
	g_signal_handlers_unblock_by_func (selection, (gpointer) selection_changed_cb, this);
}

