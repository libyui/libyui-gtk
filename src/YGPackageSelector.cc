/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/*
  Textdomain "yast2-gtk"
 */

#define YUILogComponent "gtk"
#include <config.h>
#include <string.h>
#include "YGUI.h"
#include "YGUtils.h"
#include "YGi18n.h"
#include "YGDialog.h"

#include "ygtkwizard.h"
#include "ygtkfindentry.h"
#include "ygtkmenubutton.h"
#include "ygtkhtmlwrap.h"
#include "ygtkrichtext.h"
#include "ygtktreeview.h"
#include "ygtkzyppview.h"
#include "ygtktooltip.h"

// experiments:
static bool show_find_pane = false, use_buttons = false, show_novelty_filter = false;
bool YGUI::pkgSelectorParse (const char *arg)
{
	if (!strcmp (arg, "find-pane"))
		show_find_pane = true;
	else if (!strcmp (arg, "buttons"))
		use_buttons = true;
	else if (!strcmp (arg, "novelty-filter"))
		show_novelty_filter = true;
	else return false;
	return true;
}

//** UI components -- split up for re-usability, but mostly for readability

class FlexPane
{
GtkWidget *m_bin, *m_child1, *m_child2;
bool m_isVertical, m_resize1, m_resize2, m_shrink1, m_shrink2;
int m_vpos, m_hpos;

public:
	GtkWidget *getWidget() { return m_bin; }

	FlexPane() : m_child1 (NULL), m_isVertical (true), m_vpos (0), m_hpos (0)
	{
		m_bin = gtk_event_box_new();
		gtk_widget_show (m_bin);
		g_signal_connect_after (G_OBJECT (m_bin), "size-allocate",
		                        G_CALLBACK (size_allocate_cb), this);
	}

	~FlexPane()
	{
		g_object_unref (G_OBJECT (m_child1));
		g_object_unref (G_OBJECT (m_child2));
	}

	void pack (GtkWidget *child, bool resize, bool shrink)
	{
		g_object_ref_sink (G_OBJECT (child));
		if (m_child1) {
			m_child2 = child; m_resize2 = resize; m_shrink2 = shrink;
		}
		else {
			m_child1 = child; m_resize1 = resize; m_shrink1 = shrink;
		}
	}

	void setPosition (int vpos, int hpos)
	{ m_vpos = vpos; m_hpos = hpos; }

private:
	static bool isVertical (int width)
	{ return width < 1000; }  // heuristic

	void setup()
	{
		bool isVer = isVertical (m_bin->allocation.width);
		GtkWidget *oldPane = GTK_BIN (m_bin)->child;
		if (!oldPane || (m_isVertical != isVer)) {
			m_isVertical = isVer;
			if (oldPane) {
				gtk_container_remove (GTK_CONTAINER (oldPane), m_child1);
				gtk_container_remove (GTK_CONTAINER (oldPane), m_child2);
				gtk_container_remove (GTK_CONTAINER (m_bin), oldPane);
			}

			GtkWidget *pane = isVer ? gtk_vpaned_new() : gtk_hpaned_new();
			gtk_paned_pack1 (GTK_PANED (pane), m_child1, m_resize1, m_shrink1);
			gtk_paned_pack2 (GTK_PANED (pane), m_child2, m_resize2, m_shrink2);
			if (m_hpos) {
				int size = isVer ? m_bin->allocation.height : m_bin->allocation.width;
				int pos = isVer ? m_vpos : m_hpos;
				pos = pos < 0 ? size + pos : pos;
				gtk_paned_set_position (GTK_PANED (pane), pos);
			}

			gtk_container_add (GTK_CONTAINER (m_bin), pane);
			gtk_widget_show (pane);
		}
	}

	static void size_allocate_cb (GtkWidget *bin, GtkAllocation *alloc, FlexPane *pThis)
	{ pThis->setup(); }
};

class UndoView
{
GtkWidget *m_vbox;
Ypp::PkgList m_changes;

public:
	GtkWidget *getWidget() { return m_vbox; }

	UndoView (YGtkPackageView::Listener *listener)
	{
		Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
		query->setToModify (true);
		m_changes = Ypp::PkgQuery (Ypp::Package::PACKAGE_TYPE, query);

		GtkWidget *view = createView (m_changes, listener);

		GtkWidget *uncouple_button = gtk_button_new_with_label (_("Uncouple"));
		GtkWidget *icon = gtk_image_new_from_stock (GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image (GTK_BUTTON (uncouple_button), icon);
		GtkWidget *uncouple_align = gtk_alignment_new (1, .5, 0, 1);
		gtk_container_add (GTK_CONTAINER (uncouple_align), uncouple_button);
		gtk_widget_set_tooltip_text (uncouple_button, _("Open in new window"));
		g_signal_connect (G_OBJECT (uncouple_button), "clicked",
		                  G_CALLBACK (uncouple_clicked_cb), this);

		m_vbox = gtk_vbox_new (FALSE, 6);
		gtk_container_set_border_width (GTK_CONTAINER (m_vbox), 6);
		gtk_box_pack_start (GTK_BOX (m_vbox), view, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_vbox), uncouple_align, FALSE, TRUE, 0);
		gtk_widget_show_all (m_vbox);
		g_object_ref_sink (m_vbox);
	}

	~UndoView()
	{ g_object_unref (m_vbox); }

private:
	static GtkWidget *createView (const Ypp::PkgList changes, YGtkPackageView::Listener *listener)
	{
		GtkWidget *hbox = gtk_hbox_new (TRUE, 6);
		for (int i = 0; i < 3; i++) {
			const char *str = 0, *stock = 0;
			Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
			int checkCol = 0;
			switch (i) {
				case 0:
					str = _("To install:");
					stock = GTK_STOCK_ADD;
					query->setToInstall (true);
					query->setIsInstalled (false);
					checkCol = ZyppModel::TO_INSTALL_COLUMN;
					break;
				case 1:
					str = _("To upgrade:");
					stock = GTK_STOCK_GO_UP;
					query->setToInstall (true);
					query->setIsInstalled (true);
					checkCol = ZyppModel::TO_UPGRADE_COLUMN;
					break;
				case 2:
					str = _("To remove:");
					stock = GTK_STOCK_REMOVE;
					query->setToRemove (true);
					checkCol = ZyppModel::TO_REMOVE_COLUMN;
					break;
				default: break;
			}
			Ypp::PkgQuery list (changes, query);

			GtkWidget *label_box, *icon, *label;
			label = gtk_label_new (str);
			gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
			icon = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_MENU);
			label_box = gtk_hbox_new (FALSE, 6);
			gtk_box_pack_start (GTK_BOX (label_box), icon, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (label_box), label, TRUE, TRUE, 0);
			YGtkPackageView *view = ygtk_package_view_new (FALSE);
			view->appendCheckColumn (checkCol);
			view->appendTextColumn (NULL, ZyppModel::NAME_COLUMN, -1, true);
			view->setList (list, NULL);
			view->setListener (listener);
			gtk_scrolled_window_set_shadow_type (
				GTK_SCROLLED_WINDOW (view), GTK_SHADOW_IN);

			GtkWidget *box = gtk_vbox_new (FALSE, 6);
			gtk_box_pack_start (GTK_BOX (box), label_box, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (view), TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (hbox), box, TRUE, TRUE, 0);
		}
		return hbox;
	}

	static void uncouple_clicked_cb (GtkButton *button, UndoView *pThis)
	{
		gtk_widget_hide (pThis->m_vbox);
		GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Undo History"),
			YGDialog::currentWindow(), GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
		gtk_window_set_default_size (GTK_WINDOW (dialog), 500, 450);
		gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
		g_signal_connect (G_OBJECT (dialog), "delete-event",
		                  G_CALLBACK (uncouple_delete_event_cb), pThis);
		g_signal_connect (G_OBJECT (dialog), "response",
		                  G_CALLBACK (close_response_cb), pThis);

		GtkWidget *view = createView (pThis->m_changes, NULL);
		gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), view);
		gtk_widget_show_all (dialog);
	}

	static gboolean uncouple_delete_event_cb (
		GtkWidget *widget, GdkEvent *event, UndoView *pThis)
	{
		gtk_widget_show (pThis->m_vbox);
		return FALSE;
	}

	static void close_response_cb (GtkDialog *dialog, gint response, UndoView *pThis)
	{
		gtk_widget_show (pThis->m_vbox);
		gtk_widget_destroy (GTK_WIDGET (dialog));
	}
};

struct QueryListener {
	virtual void queryNotify() = 0;
	virtual void queryNotifyDelay() = 0;
};

class QueryWidget
{
protected:
	QueryListener *m_listener;
	void notify() { m_listener->queryNotify(); }
	void notifyDelay() { m_listener->queryNotifyDelay(); }

public:
	void setListener (QueryListener *listener) { m_listener = listener; }
	virtual GtkWidget *getWidget() = 0;
	virtual void writeQuery (Ypp::PkgQuery::Query *query) = 0;

	virtual bool availablePackagesOnly() { return false; }
	virtual bool installedPackagesOnly() { return false; }
};

class StoreView : public QueryWidget
{
protected:
	GtkWidget *m_view, *m_scroll, *m_box;
	enum Column { TEXT_COL, ICON_COL, ENABLED_COL, PTR_COL, TOOLTIP_COL, TOTAL_COLS };

	virtual void doBuild (GtkTreeStore *store) = 0;
	virtual void writeQuery (Ypp::PkgQuery::Query *query,
	                           const std::list <gpointer> &ptr) = 0;

	StoreView()
	: QueryWidget()
	{
		m_scroll = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (m_scroll),
				                             GTK_SHADOW_IN);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (m_scroll),
				                        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		m_view = NULL;
		m_box = gtk_vbox_new (FALSE, 2);
		gtk_box_pack_start (GTK_BOX (m_box), m_scroll, TRUE, TRUE, 0);

		// parent constructor should call build()
	}

public:
	virtual GtkWidget *getWidget() { return m_box; }

	virtual void writeQuery (Ypp::PkgQuery::Query *query)
	{
		GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (m_view));
		GtkTreeModel *model;
		GList *selected = gtk_tree_selection_get_selected_rows (selection, &model);
		std::list <gpointer> ptrs;
		for (GList *i = selected; i; i = i->next) {
			GtkTreePath *path = (GtkTreePath *) i->data;
			GtkTreeIter iter;
			gtk_tree_model_get_iter (model, &iter, path);
			gpointer ptr;
			gtk_tree_model_get (model, &iter, PTR_COL, &ptr, -1);
			if (ptr)
				ptrs.push_back (ptr);
			gtk_tree_path_free (path);
		}
		g_list_free (selected);
		writeQuery (query, ptrs);
	}

protected:
	void build (bool tree_mode, bool with_icons, bool multi_selection,
	            bool do_tooltip)
	{
		if (m_view)
			gtk_container_remove (GTK_CONTAINER (m_scroll), m_view);

		m_view = ygtk_tree_view_new();
		GtkTreeView *view = GTK_TREE_VIEW (m_view);
		gtk_tree_view_set_headers_visible (view, FALSE);
		gtk_tree_view_set_search_column (view, TEXT_COL);
		if (do_tooltip)
			gtk_tree_view_set_tooltip_column (view, TEXT_COL);
		gtk_tree_view_set_show_expanders (view, tree_mode);
		gtk_tree_view_set_tooltip_column (view, TOOLTIP_COL);

		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;
		if (with_icons) {
			renderer = gtk_cell_renderer_pixbuf_new();
			column = gtk_tree_view_column_new_with_attributes ("",
				renderer, "icon-name", ICON_COL, "sensitive", ENABLED_COL, NULL);
			gtk_tree_view_append_column (view, column);
		}
		renderer = gtk_cell_renderer_text_new();
		g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
		column = gtk_tree_view_column_new_with_attributes ("",
			renderer, "markup", TEXT_COL, "sensitive", ENABLED_COL, NULL);
		gtk_tree_view_append_column (view, column);

		GtkTreeStore *store = gtk_tree_store_new (TOTAL_COLS,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_POINTER, G_TYPE_STRING);
		GtkTreeModel *model = GTK_TREE_MODEL (store);
		gtk_tree_view_set_model (view, model);
		g_object_unref (G_OBJECT (model));

		GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
		gtk_tree_selection_set_mode (selection,
			multi_selection ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_BROWSE);
		g_signal_connect (G_OBJECT (selection), "changed",
				          G_CALLBACK (selection_cb), this);
		gtk_tree_selection_set_select_function (selection, can_select_cb, this, NULL);

		block();
		GtkTreeIter iter;
		gtk_tree_store_append (store, &iter, NULL);
		gtk_tree_store_set (store, &iter, TEXT_COL, _("All"), ENABLED_COL, TRUE,
		                    TOOLTIP_COL, _("No filter"), -1);
		doBuild (store);

		selectFirstItem();
		unblock();

		gtk_container_add (GTK_CONTAINER (m_scroll), m_view);
		gtk_widget_show (m_view);
	}

private:
	void block()
	{
		GtkTreeView *view = GTK_TREE_VIEW (m_view);
		GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
		g_signal_handlers_block_by_func (selection, (gpointer) selection_cb, this);
	}
	void unblock()
	{
		GtkTreeView *view = GTK_TREE_VIEW (m_view);
		GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
		g_signal_handlers_unblock_by_func (selection, (gpointer) selection_cb, this);
	}

	void selectFirstItem()
	{
		/* we use gtk_tree_view_set_cursor(), rather than gtk_tree_selection_select_iter()
		   because that one is buggy in that when the user first interacts with the
		   treeview, a change signal is sent, even if he was just expanding one node... */
		block();
		GtkTreePath *path = gtk_tree_path_new_first();
		gtk_tree_view_set_cursor (GTK_TREE_VIEW (m_view), path, NULL, FALSE);
		gtk_tree_path_free (path);
		unblock();
	}

	static void selection_cb (GtkTreeSelection *selection, StoreView *pThis)
	{
		pThis->notify();
		// if item unselected, make sure "All" is
		if (gtk_tree_selection_count_selected_rows (selection) == 0)
			pThis->selectFirstItem();
	}

	static gboolean can_select_cb (GtkTreeSelection *selection, GtkTreeModel *model,
	                               GtkTreePath *path, gboolean currently_selected,
	                               gpointer pData)
	{
		gboolean ret;
		GtkTreeIter iter;
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter, ENABLED_COL, &ret, -1);
		return ret;
	}
};

struct Categories : public StoreView
{
	bool m_rpmGroups, m_onlineUpdate;

public:
	Categories (bool onlineUpdate)
	: StoreView(), m_rpmGroups (false), m_onlineUpdate (onlineUpdate)
	{
		if (!onlineUpdate) {
			GtkWidget *check = gtk_check_button_new_with_label (_("Detailed"));
			YGUtils::setWidgetFont (GTK_BIN (check)->child,
				PANGO_STYLE_NORMAL, PANGO_WEIGHT_NORMAL, PANGO_SCALE_SMALL);
			gtk_widget_set_tooltip_text (check,
				_("Group by the PackageKit-based filter or straight from the actual "
				"RPM information."));
			g_signal_connect (G_OBJECT (check), "toggled",
				              G_CALLBACK (rpm_groups_toggled_cb), this);
			gtk_box_pack_start (GTK_BOX (m_box), check, FALSE, TRUE, 0);
		}
		build (m_rpmGroups, !m_rpmGroups, false, false);
	}

protected:
	virtual void doBuild (GtkTreeStore *store)
	{
		struct inner {
			static void populate (GtkTreeStore *store, GtkTreeIter *parent,
						          Ypp::Node *category, Categories *pThis)
			{
				if (!category)
					return;
				GtkTreeIter iter;
				gtk_tree_store_append (store, &iter, parent);
				gtk_tree_store_set (store, &iter, TEXT_COL, category->name.c_str(),
					ICON_COL, category->icon,  PTR_COL, category, ENABLED_COL, TRUE, -1);
				populate (store, &iter, category->child(), pThis);
				populate (store, parent, category->next(), pThis);
			}
		};

		Ypp::Node *first_category;
		Ypp::Package::Type type = m_onlineUpdate ?
			Ypp::Package::PATCH_TYPE : Ypp::Package::PACKAGE_TYPE;
		if (!m_rpmGroups && !m_onlineUpdate)
			first_category = Ypp::get()->getFirstCategory2 (type);
		else
			first_category = Ypp::get()->getFirstCategory (type);
		inner::populate (store, NULL, first_category, this);
		if (!m_rpmGroups && !m_onlineUpdate) {
			GtkTreeView *view = GTK_TREE_VIEW (m_view);
			GtkTreeIter iter;
			gtk_tree_store_append (store, &iter, NULL);
			gtk_tree_view_set_row_separator_func (view,
				is_tree_model_iter_separator_cb, NULL, NULL);

			gtk_tree_store_append (store, &iter, NULL);
			gtk_tree_store_set (store, &iter, TEXT_COL, _("Recommended"),
				ICON_COL, GTK_STOCK_ABOUT,  PTR_COL, GINT_TO_POINTER (1),
				ENABLED_COL, TRUE, TOOLTIP_COL, _("Recommended by an installed package"), -1);
			gtk_tree_store_append (store, &iter, NULL);
			gtk_tree_store_set (store, &iter, TEXT_COL, _("Suggested"),
				ICON_COL, GTK_STOCK_ABOUT,  PTR_COL, GINT_TO_POINTER (2),
				ENABLED_COL, TRUE, TOOLTIP_COL, _("Suggested by an installed package"), -1);
			gtk_tree_store_append (store, &iter, NULL);
			gtk_tree_store_set (store, &iter, TEXT_COL, _("Fresh"),
				ICON_COL, GTK_STOCK_NEW,  PTR_COL, GINT_TO_POINTER (3),
				ENABLED_COL, TRUE, TOOLTIP_COL, _("Uploaded in the last week"), -1);
		}
	}

	virtual void writeQuery (Ypp::PkgQuery::Query *query, const std::list <gpointer> &ptrs)
	{
		gpointer ptr = ptrs.empty() ? NULL : ptrs.front();
		int nptr = GPOINTER_TO_INT (ptr);
		if (nptr == 1)
			query->setIsRecommended (true);
		else if (nptr == 2)
			query->setIsSuggested (true);
		else if (nptr == 3)
			query->setBuildAge (7);
		else if (ptr) {
			Ypp::Node *node = (Ypp::Node *) ptr;
			if (m_rpmGroups || m_onlineUpdate)
				query->addCategory (node);
			else
				query->addCategory2 (node);
		}
	}

	static void rpm_groups_toggled_cb (GtkToggleButton *button, Categories *pThis)
	{
		pThis->m_rpmGroups = gtk_toggle_button_get_active (button);
		pThis->build (pThis->m_rpmGroups, !pThis->m_rpmGroups, false, false);
		pThis->notify();
	}

	static gboolean is_tree_model_iter_separator_cb (
		GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
	{
		gint col = data ? GPOINTER_TO_INT (data) : 0;
		gpointer ptr;
		gtk_tree_model_get (model, iter, col, &ptr, -1);
		return ptr == NULL;
	}
};

struct Repositories : public StoreView
{
public:
	Repositories (bool repoMgrEnabled)
	: StoreView()
	{
		if (repoMgrEnabled) {
			GtkWidget *align, *button, *box, *image, *label;
			image = gtk_image_new_from_stock (GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
			label = gtk_label_new (_("Edit..."));
			YGUtils::setWidgetFont (label, PANGO_STYLE_NORMAL, PANGO_WEIGHT_NORMAL, PANGO_SCALE_SMALL);
			box = gtk_hbox_new (FALSE, 6);
			gtk_box_pack_start (GTK_BOX (box), image, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
			button = gtk_button_new();
			gtk_container_add (GTK_CONTAINER (button), box);
			gtk_widget_set_tooltip_text (button, _("Access the repositories manager tool."));
			align = gtk_alignment_new (0, 0, 0, 1);
			gtk_container_add (GTK_CONTAINER (align), button);
			g_signal_connect (G_OBJECT (button), "clicked",
				              G_CALLBACK (setup_button_clicked_cb), this);
			gtk_box_pack_start (GTK_BOX (m_box), align, FALSE, TRUE, 0);
		}
		build (false, true, true, true);
	}

	~Repositories()
	{ Ypp::get()->setFavoriteRepository (NULL); }

protected:
	virtual void doBuild (GtkTreeStore *store)
	{
		GtkTreeIter iter;
		for (int i = 0; Ypp::get()->getRepository (i); i++) {
			const Ypp::Repository *repo = Ypp::get()->getRepository (i);
			gtk_tree_store_append (store, &iter, NULL);
			std::string text = repo->name, url (repo->url);
			YGUtils::escapeMarkup (url);
			text += "\n<small>" + url + "</small>";
			const gchar *icon;
			if (repo->url.empty())
				icon = GTK_STOCK_MISSING_IMAGE;
			else if (repo->url.compare (0, 2, "cd", 2) == 0 ||
			         repo->url.compare (0, 3, "dvd", 3) == 0)
				icon = GTK_STOCK_CDROM;
			else if (repo->url.compare (0, 3, "iso", 3) == 0)
				icon = GTK_STOCK_FILE;
			else
				icon = GTK_STOCK_NETWORK;
			gtk_tree_store_set (store, &iter, TEXT_COL, text.c_str(),
				ICON_COL, icon, ENABLED_COL, repo->enabled, PTR_COL, repo, -1);
		}
	}

	virtual void writeQuery (Ypp::PkgQuery::Query *query, const std::list <gpointer> &ptrs)
	{
		for (std::list <gpointer>::const_iterator it = ptrs.begin();
		     it != ptrs.end(); it++) {
			Ypp::Repository *repo = (Ypp::Repository *) *it;
			query->addRepository (repo);
		}
		gpointer ptr = ptrs.size() == 1 ? ptrs.front() : NULL;
		Ypp::get()->setFavoriteRepository ((Ypp::Repository *) ptr);
	}

	virtual bool availablePackagesOnly() { return true; }

	static void setup_button_clicked_cb (GtkButton *button, Repositories *pThis)
	{ YGUI::ui()->sendEvent (new YMenuEvent ("repo_mgr")); }
};

struct Collection : public QueryWidget, YGtkPackageView::Listener
{
private:
	YGtkPackageView *m_view;
	GtkWidget *m_buttons_box, *m_box;

public:
	virtual GtkWidget *getWidget() { return m_box; }

	Collection (Ypp::Package::Type type)
	: QueryWidget()
	{
		m_view = ygtk_package_view_new (TRUE);
		m_view->appendIconColumn (NULL, ZyppModel::ICON_COLUMN);
		m_view->appendTextColumn (NULL, ZyppModel::NAME_SUMMARY_COLUMN);
		if (type == Ypp::Package::LANGUAGE_TYPE)
			m_view->setList (Ypp::PkgQuery (type, NULL), NULL);
		else
			populateView (m_view, type);
		m_view->setListener (this);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (m_view), GTK_SHADOW_IN);

		// control buttons
		m_buttons_box = gtk_alignment_new (0, 0, 0, 0);
		GtkWidget *image, *button;
		button = gtk_button_new_with_label (_("Install All"));
		image = gtk_image_new_from_stock (GTK_STOCK_SAVE, GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image (GTK_BUTTON (button), image);
		g_signal_connect (G_OBJECT (button), "clicked",
		                  G_CALLBACK (install_cb), this);
		gtk_container_add (GTK_CONTAINER (m_buttons_box), button);
		gtk_widget_set_sensitive (m_buttons_box, FALSE);

		// layout
		m_box = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (m_box), GTK_WIDGET (m_view), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_box), m_buttons_box, FALSE, TRUE, 0);
	}

	static void populateView (YGtkPackageView *view, Ypp::Package::Type type)
	{  // list 1D categories intertwined with its constituent packages
		Ypp::Node *category = Ypp::get()->getFirstCategory (type);
		for (; category; category = category->next()) {
			Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
			query->addCategory (category);
			view->appendList (category->name.c_str(), Ypp::PkgQuery (type, query), NULL);
		}
	}

	virtual void packagesSelected (Ypp::PkgList selection)
	{
		gtk_widget_set_sensitive (m_buttons_box, selection.notInstalled());
		notify();
	}

	virtual void writeQuery (Ypp::PkgQuery::Query *query)
	{
		Ypp::PkgList selected = m_view->getSelected();
		for (int i = 0; selected.get (i); i++)
			query->addCollection (selected.get (i));
		if (selected.size() == 0)
			query->setClear();
	}

	void doAll (bool install /*or remove*/)
	{  // we just need to mark the collections themselves
		Ypp::PkgList selected = m_view->getSelected();
		install ? selected.install() : selected.remove();
	}

	static void install_cb (GtkButton *button, Collection *pThis)
	{ pThis->doAll (true); }
	static void remove_cb (GtkButton *button, Collection *pThis)
	{ pThis->doAll (false); }
};

static const char *find_entry_tooltip =
	_("<b>Package search:</b> Use spaces to separate your keywords. They "
	"will be matched against RPM <i>name</i> and <i>summary</i> attributes. "
	"Other criteria attributes are available by pressing the search icon.\n"
	"(usage example: \"yast dhcp\" will return yast's dhcpd tool)");
static const char *find_entry_novelty_tooltip =
	_("Number of days since the package was built by the repository.");
static const char *find_entry_file_tooltip =
	_("Note: Only applicable to installed packages.");
static std::string findPatternTooltip (const std::string &name)
{
	Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
	query->addNames (name, ' ', true, false, false, false, false, true);
	query->setIsInstalled (false);
	Ypp::PkgQuery pool (Ypp::Package::PATTERN_TYPE, query);
	if (pool.size() > 0) {
		std::string _name = pool.get (0)->name();
		std::string text = _("Patterns are available that can "
			"assist you in the installment of");
		text += " <i>" + _name + "</i>.";
		return text;
	}
	return "";
}

class FindPane : public QueryWidget
{
GtkWidget *m_box, *m_name, *m_radio[5], *m_info_box, *m_info_label;
bool m_onlineUpdate;

public:
	virtual GtkWidget *getWidget() { return m_box; }

	FindPane (bool onlineUpdate)
	: m_onlineUpdate (onlineUpdate)
	{
		GtkWidget *name_box = gtk_hbox_new (FALSE, 6), *button;
		m_name = ygtk_find_entry_new();
		gtk_widget_set_tooltip_markup (m_name, find_entry_tooltip);
		g_signal_connect (G_OBJECT (m_name), "changed",
		                  G_CALLBACK (name_changed_cb), this);
		button = gtk_button_new_with_label ("");
		GtkWidget *image = gtk_image_new_from_stock (GTK_STOCK_OK, GTK_ICON_SIZE_MENU);
		gtk_button_set_image (GTK_BUTTON (button), image);
		gtk_widget_set_tooltip_text (button, _("Search!"));
		gtk_box_pack_start (GTK_BOX (name_box), m_name, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (name_box), button, FALSE, TRUE, 0);

		for (int i = 0; i < 5; i++) m_radio[i] = 0;
		GtkWidget *radio_box = gtk_vbox_new (FALSE, 0);
		m_radio[0] = gtk_radio_button_new_with_label_from_widget (NULL, _("Name & summary"));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (m_radio[0]), TRUE);
		GtkRadioButton *radiob = GTK_RADIO_BUTTON (m_radio[0]);
		m_radio[1] = gtk_radio_button_new_with_label_from_widget (radiob, _("Description"));
		if (!onlineUpdate) {
			m_radio[2] = gtk_radio_button_new_with_label_from_widget (radiob, _("File name"));
			gtk_widget_set_tooltip_text (m_radio[2], find_entry_file_tooltip);
			m_radio[3] = gtk_radio_button_new_with_label_from_widget (radiob, _("Author"));
			if (show_novelty_filter) {
				m_radio[4] = gtk_radio_button_new_with_label_from_widget (radiob, _("Novelty (in days)"));
				g_signal_connect (G_OBJECT (m_radio[4]), "toggled", G_CALLBACK (novelty_toggled_cb), this);
				gtk_widget_set_tooltip_markup (m_radio[4], find_entry_novelty_tooltip);
			}
		}
		for (int i = 0; i < 5; i++)
			if (m_radio [i])
				gtk_box_pack_start (GTK_BOX (radio_box), m_radio[i], FALSE, TRUE, 0);

		GtkWidget *radio_frame = gtk_frame_new (_("Search in:"));
		YGUtils::setWidgetFont (gtk_frame_get_label_widget (GTK_FRAME (radio_frame)),
			PANGO_STYLE_NORMAL, PANGO_WEIGHT_BOLD, PANGO_SCALE_MEDIUM);
		gtk_frame_set_shadow_type (GTK_FRAME (radio_frame), GTK_SHADOW_NONE);
		GtkWidget *align = gtk_alignment_new (0, 0, 1, 1);
		gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 15, 0);
		gtk_container_add (GTK_CONTAINER (align), radio_box);
		gtk_container_add (GTK_CONTAINER (radio_frame), align);

		m_info_box = gtk_hbox_new (FALSE, 6);
		m_info_label = gtk_label_new ("");
		gtk_label_set_line_wrap (GTK_LABEL (m_info_label), TRUE);
		gtk_box_pack_start (GTK_BOX (m_info_box),
			gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_MENU), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_info_box), m_info_label, TRUE, TRUE, 0);

		GtkWidget *separator1 = gtk_event_box_new();
		gtk_widget_set_size_request (separator1, -1, 12);
		GtkWidget *separator2 = gtk_event_box_new();
		gtk_widget_set_size_request (separator2, -1, 15);
		GtkWidget *separator3 = gtk_event_box_new();
		gtk_widget_set_size_request (separator3, -1, 50);

		GtkWidget *box = gtk_vbox_new (FALSE, 0);
		gtk_box_pack_start (GTK_BOX (box), separator1, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (box), name_box, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (box), separator2, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (box), radio_frame, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (box), separator3, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (box), m_info_box, FALSE, TRUE, 0);
		m_box = box;

		g_signal_connect (G_OBJECT (m_name), "activate", G_CALLBACK (button_clicked_cb), this);
		g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (button_clicked_cb), this);
	}

	virtual void writeQuery (Ypp::PkgQuery::Query *query)
	{
		gtk_widget_hide (m_info_box);
		const char *name = gtk_entry_get_text (GTK_ENTRY (m_name));
		if (*name) {
			int item;
			for (item = 0; item < 5; item++)
				if (m_radio[item])
				    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (m_radio[item])))
						break;
			if (item >= 5) ;
			else if (item == 4) {  // novelty
				int days = atoi (name);
				query->setBuildAge (days);
			}
			else {
				bool use_name, use_summary, use_description, use_filelist, use_authors;
				use_name = use_summary = use_description = use_filelist = use_authors = false;
				switch (item) {
					case 0:  // name & summary
					default:
						use_name = use_summary = true;
						break;
					case 1:  // description
						use_name = use_summary = use_description = true;
						break;
					case 2:  // file
						use_filelist = true;
						break;
					case 3:  // author
						use_authors = true;
						break;
				}
				query->addNames (name, ' ', use_name, use_summary, use_description,
					             use_filelist, use_authors);
			}
			if (item == 0 && !m_onlineUpdate) {  // tip: user may be unaware of the patterns
				std::string text = findPatternTooltip (name);
				if (!text.empty()) {
					gtk_label_set_markup (GTK_LABEL (m_info_label), text.c_str());
					// wrapping must be performed manually in GTK
					int width;
					gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, NULL);
					width = getWidget()->allocation.width - width - 6;
					gtk_widget_set_size_request (m_info_label, width, -1);
					gtk_widget_show_all (m_info_box);
				}
			}
		}
	}

	virtual bool installedPackagesOnly()
	{ return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (m_radio[2])); }

	const char *searchName()
	{
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (m_radio[0]))) {
			const char *name = gtk_entry_get_text (GTK_ENTRY (m_name));
			return *name ? name : NULL;
		}
		return NULL;
	}

private:
	static void novelty_toggled_cb (GtkToggleButton *button, FindPane *pThis)
	{
		// novelty is weird; so show usage case
		const gchar *text = "";
		if (gtk_toggle_button_get_active (button))
			text = "7";
		g_signal_handlers_block_by_func (pThis->m_name, (gpointer) name_changed_cb, pThis);
		gtk_entry_set_text (GTK_ENTRY (pThis->m_name), text);
		g_signal_handlers_unblock_by_func (pThis->m_name, (gpointer) name_changed_cb, pThis);
		gtk_editable_set_position (GTK_EDITABLE (pThis->m_name), -1);
		ygtk_find_entry_set_state (YGTK_FIND_ENTRY (pThis->m_name), TRUE);
	}

	static void name_changed_cb (YGtkFindEntry *entry, FindPane *pThis)
	{
		const gchar *text = gtk_entry_get_text (GTK_ENTRY (entry));
		// novelty allows only numbers
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pThis->m_radio[4]))) {
			gboolean correct = TRUE;
			for (const gchar *i = text; *i; i++)
				if (!g_ascii_isdigit (*i)) {
					correct = FALSE;
					break;
				}
			ygtk_find_entry_set_state (entry, correct);
		}
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pThis->m_radio[0])))
			pThis->notifyDelay();
		else if (*text == '\0')
			pThis->notify();
	}

	static void button_clicked_cb (GtkWidget *widget, FindPane *pThis)
	{ pThis->notify(); }
};

class FindEntry : public QueryWidget
{
GtkWidget *m_name;
bool m_onlineUpdate;
GtkWidget *m_combo;

public:
	GtkWidget *getWidget() { return m_name; }

	FindEntry (bool onlineUpdate, GtkWidget *combo) : QueryWidget()
	, m_onlineUpdate (onlineUpdate), m_combo (combo)
	{
		m_name = ygtk_find_entry_new();
		gtk_widget_set_size_request (m_name, 140, -1);
		gtk_widget_set_tooltip_markup (m_name, find_entry_tooltip);
		ygtk_find_entry_insert_item (YGTK_FIND_ENTRY (m_name),
			_("Filter by name & summary"), GTK_STOCK_FIND, NULL);
		ygtk_find_entry_insert_item (YGTK_FIND_ENTRY (m_name),
			_("Filter by description"), GTK_STOCK_EDIT, NULL);
		if (!onlineUpdate) {
			ygtk_find_entry_insert_item (YGTK_FIND_ENTRY (m_name),
				_("Filter by file"), GTK_STOCK_OPEN, find_entry_file_tooltip);
			ygtk_find_entry_insert_item (YGTK_FIND_ENTRY (m_name),
				_("Filter by author"), GTK_STOCK_ABOUT, NULL);
			if (show_novelty_filter)
				ygtk_find_entry_insert_item (YGTK_FIND_ENTRY (m_name),
					_("Filter by novelty (in days)"), GTK_STOCK_NEW, find_entry_novelty_tooltip);
		}
		g_signal_connect (G_OBJECT (m_name), "changed",
		                  G_CALLBACK (name_changed_cb), this);
		g_signal_connect (G_OBJECT (m_name), "menu-item-selected",
		                  G_CALLBACK (name_item_changed_cb), this);
		gtk_widget_show (m_name);
	}

	void clear()
	{
		g_signal_handlers_block_by_func (m_name, (gpointer) name_changed_cb, this);
		g_signal_handlers_block_by_func (m_name, (gpointer) name_item_changed_cb, this);
		gtk_entry_set_text (GTK_ENTRY (m_name), "");
		ygtk_find_entry_select_item (YGTK_FIND_ENTRY (m_name), 0);
		g_signal_handlers_unblock_by_func (m_name, (gpointer) name_changed_cb, this);
		g_signal_handlers_unblock_by_func (m_name, (gpointer) name_item_changed_cb, this);
	}

	const char *searchName()
	{
		YGtkFindEntry *entry = YGTK_FIND_ENTRY (m_name);
		if (ygtk_find_entry_get_selected_item (entry) == 0) {
			const char *name = gtk_entry_get_text (GTK_ENTRY (m_name));
			return *name ? name : NULL;
		}
		return NULL;
	}

	virtual void writeQuery (Ypp::PkgQuery::Query *query)
	{
		const char *name = gtk_entry_get_text (GTK_ENTRY (m_name));
		if (*name) {
			int item = ygtk_find_entry_get_selected_item (YGTK_FIND_ENTRY (m_name));
			if (item >= 5) ;
			else if (item == 4) {  // novelty
				int days = atoi (name);
				query->setBuildAge (days);
			}
			else {
				bool use_name, use_summary, use_description, use_filelist, use_authors;
				use_name = use_summary = use_description = use_filelist = use_authors = false;
				switch (item) {
					case 0:  // name & summary
					default:
						use_name = use_summary = true;
						break;
					case 1:  // description
						use_name = use_summary = use_description = true;
						break;
					case 2:  // file
						use_filelist = true;
						break;
					case 3:  // author
						use_authors = true;
						break;
				}
				query->addNames (name, ' ', use_name, use_summary, use_description,
					             use_filelist, use_authors);
			}

			if (item == 0 && !m_onlineUpdate) {  // tip: user may be unaware of the patterns
				std::string text = findPatternTooltip (name);
				if (!text.empty())
					ygtk_tooltip_show_at_widget (m_combo, YGTK_POINTER_UP_LEFT,
						text.c_str(), GTK_STOCK_DIALOG_INFO);
			}
		}
	}

	virtual bool installedPackagesOnly()
	{ return ygtk_find_entry_get_selected_item (YGTK_FIND_ENTRY (m_name)) == 2; }

private:
	static void name_changed_cb (YGtkFindEntry *entry, FindEntry *pThis)
	{
		gint nb = ygtk_find_entry_get_selected_item (entry);
		if (nb == 4) {  // novelty only allows numbers
			const gchar *text = gtk_entry_get_text (GTK_ENTRY (entry));
			gboolean correct = TRUE;
			for (const gchar *i = text; *i; i++)
				if (!g_ascii_isdigit (*i)) {
					correct = FALSE;
					break;
				}
			ygtk_find_entry_set_state (entry, correct);
		}
		pThis->notifyDelay();
	}

	static void name_item_changed_cb (YGtkFindEntry *entry, gint nb, FindEntry *pThis)
	{
		const gchar *text = "";
		if (nb == 4) text = "7";  // novelty is weird; show usage case
		g_signal_handlers_block_by_func (entry, (gpointer) name_changed_cb, pThis);
		gtk_entry_set_text (GTK_ENTRY (entry), text);
		g_signal_handlers_unblock_by_func (entry, (gpointer) name_changed_cb, pThis);
		gtk_editable_set_position (GTK_EDITABLE (entry), -1);
		ygtk_find_entry_set_state (entry, TRUE);
		pThis->notify();
	}
};

class FilterCombo : public QueryWidget
{
GtkWidget *m_box, *m_combo, *m_bin;
bool m_onlineUpdate, m_repoMgrEnabled;
QueryWidget *m_queryWidget;

public:
	virtual GtkWidget *getWidget() { return m_box; }
	GtkWidget *getComboBox() { return m_combo; }

	FilterCombo (bool onlineUpdate, bool repoMgrEnabled)
	: QueryWidget(), m_onlineUpdate (onlineUpdate), m_repoMgrEnabled (repoMgrEnabled),
	  m_queryWidget (NULL)
	{
		m_combo = gtk_combo_box_new_text();
		if (onlineUpdate)
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_combo), _("Severity"));
		else {
			if (show_find_pane)
				gtk_combo_box_append_text (GTK_COMBO_BOX (m_combo), _("Search"));
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_combo), _("Groups"));
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_combo), _("Patterns"));
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_combo), _("Languages"));
			gtk_widget_set_tooltip_markup (m_combo,
				_("Packages can be organized in:\n"
/*
				"<b>Search:</b> find a given package by name, summary or some "
				"other attribute.\n"
*/
				"<b>Groups:</b> simple categorization of packages by purpose.\n"
				"<b>Patterns:</b> assists in installing all packages necessary "
				"for several working environments.\n"
				"<b>Languages:</b> adds another language to the system.\n"
				"<b>Repositories:</b> catalogues what the several configured "
				"repositories have available."));
		}
		gtk_combo_box_append_text (GTK_COMBO_BOX (m_combo), _("Repositories"));
		gtk_combo_box_set_active (GTK_COMBO_BOX (m_combo), 0);
		g_signal_connect (G_OBJECT (m_combo), "changed",
		                  G_CALLBACK (combo_changed_cb), this);

		m_bin = gtk_event_box_new();
		m_box = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (m_box), m_combo, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_box), m_bin, TRUE, TRUE, 0);
		select (0);
	};

	~FilterCombo()
	{ delete m_queryWidget; }

	virtual void writeQuery (Ypp::PkgQuery::Query *query)
	{ m_queryWidget->writeQuery (query); }

	virtual bool availablePackagesOnly()
	{ return m_queryWidget->availablePackagesOnly(); }
	virtual bool installedPackagesOnly()
	{ return m_queryWidget->installedPackagesOnly(); }

	const char *searchName()
	{
		if (gtk_combo_box_get_active (GTK_COMBO_BOX (m_combo)) == 0)
			return ((FindPane *) m_queryWidget)->searchName();
		return NULL;
	}

	void select (int nb)
	{
		delete m_queryWidget;
		if (m_onlineUpdate) {
			switch (nb) {
				case 0: m_queryWidget = new Categories (m_onlineUpdate); break;
				case 1: m_queryWidget = new Repositories (m_repoMgrEnabled); break;
				default: break;
			}
		}
		else {
			if (!show_find_pane) nb++;
			switch (nb) {
				case 0: m_queryWidget = new FindPane (m_onlineUpdate); break;
				case 1: m_queryWidget = new Categories (m_onlineUpdate); break;
				case 2: m_queryWidget = new Collection (Ypp::Package::PATTERN_TYPE); break;
				case 3: m_queryWidget = new Collection (Ypp::Package::LANGUAGE_TYPE); break;
				case 4: m_queryWidget = new Repositories (m_repoMgrEnabled); break;
				default: break;
			}
		}
		m_queryWidget->setListener (m_listener);
		setChild (m_queryWidget->getWidget());
	}

private:
	void setChild (GtkWidget *widget)
	{
		if (GTK_BIN (m_bin)->child)
			gtk_container_remove (GTK_CONTAINER (m_bin), GTK_BIN (m_bin)->child);
		gtk_container_add (GTK_CONTAINER (m_bin), widget);
		gtk_widget_show_all (m_bin);
	}

	static void combo_changed_cb (GtkComboBox *combo, FilterCombo *pThis)
	{
		pThis->select (gtk_combo_box_get_active (combo));
		pThis->notify();
	}
};

#include "ygtknotebook.h"

class QueryNotebook : public QueryListener, YGtkPackageView::Listener
{
GtkWidget *m_widget, *m_notebook;
bool m_onlineUpdate;
FilterCombo *m_combo;
YGtkDetailView *m_details;
FlexPane *m_pane;
GtkWidget *m_oldPage;
guint m_timeout_id;
bool m_disabledTab, m_highlightTab;
UndoView *m_undoView;
Ypp::PkgList m_packages, m_pool;
FindEntry *m_find;

public:
	GtkWidget *getWidget() { return m_widget; }

	QueryNotebook (bool onlineUpdate, bool repoMgrEnabled)
	: m_onlineUpdate (onlineUpdate), m_timeout_id (0), m_disabledTab (true), m_highlightTab (false),
	  m_undoView (NULL)
	{
		m_notebook = ygtk_notebook_new();
		gtk_widget_show (m_notebook);
		appendPage (0, _("_Install"), GTK_STOCK_ADD);
		if (!onlineUpdate)
			appendPage (1, _("_Upgrade"), GTK_STOCK_GO_UP);
		if (!onlineUpdate)
			appendPage (2, _("_Remove"), GTK_STOCK_REMOVE);
		else
			appendPage (2, _("Installed"), GTK_STOCK_HARDDISK);
		if (!onlineUpdate)
			appendPage (3, _("_Undo"), GTK_STOCK_UNDO);

		m_details = YGTK_DETAIL_VIEW (ygtk_detail_view_new (onlineUpdate));
		m_pane = new FlexPane();
		m_pane->pack (m_notebook, true, false);
		m_pane->pack (GTK_WIDGET (m_details), false, false);
		m_pane->setPosition (-175, -500);

		m_combo = new FilterCombo (onlineUpdate, repoMgrEnabled);
		m_combo->setListener (this);
		m_combo->select (0);
		GtkWidget *hpane = gtk_hpaned_new();
		gtk_paned_pack1 (GTK_PANED (hpane), m_combo->getWidget(), FALSE, TRUE);
		gtk_paned_pack2 (GTK_PANED (hpane), m_pane->getWidget(), TRUE, FALSE);
		gtk_paned_set_position (GTK_PANED (hpane), 170);

		m_widget = hpane;
		gtk_widget_show_all (m_widget);
		gtk_widget_hide (GTK_WIDGET (m_details));

		m_find = NULL;
		if (!show_find_pane) {
			m_find = new FindEntry (onlineUpdate, m_combo->getComboBox());
			ygtk_notebook_set_corner_widget (YGTK_NOTEBOOK (m_notebook), m_find->getWidget());
			m_find->setListener (this);
		}

		Ypp::Package::Type type = Ypp::Package::PACKAGE_TYPE;
		if (m_onlineUpdate)
			type = Ypp::Package::PATCH_TYPE;
		m_packages = Ypp::get()->getPackages (type);
		queryNotify();
	}

	~QueryNotebook()
	{
		if (m_timeout_id)
			g_source_remove (m_timeout_id);
		delete m_undoView;
		delete m_combo;
		delete m_pane;
		delete m_find;
	}

	void setUndoPage()
	{
		if (!m_onlineUpdate)
			gtk_notebook_set_current_page (GTK_NOTEBOOK (m_notebook), 3);
	}

private:
	const char *getTooltip (int page)
	{
		if (m_onlineUpdate && page > 0)
			page++;
		switch (page) {
			case 0: return _("Available for install");
			case 1: return _("Upgrades");
			case 2: return m_onlineUpdate ? _("Installed patches") : _("Installed packages");
			case 3: return _("Undo history");
			default: break;
		}
		return NULL;
	}

	// callbacks
	virtual void queryNotify()
	{
		if (m_disabledTab) {  // limit users to the tabs whose query is applicable
			m_disabledTab = false;
			for (int i = 0; i < 4; i++)
				enablePage (i, true);
		}
		if (m_combo->availablePackagesOnly()) {
			enablePage (1, false);
			if (!m_onlineUpdate)
				enablePage (2, false, 0);
			m_disabledTab = true;
		}
		else if (m_combo->installedPackagesOnly()) {
			int new_page = m_onlineUpdate ? 1 : 2;
			enablePage (0, false, new_page);
			m_disabledTab = true;
		}

		GtkNotebook *notebook = GTK_NOTEBOOK (m_notebook);
		for (int i = 0; i < 3; i++) {
			GtkWidget *page = gtk_notebook_get_nth_page (notebook, i);
			if (page) YGTK_PACKAGE_VIEW (page)->clear();
		}

		Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
		if (!show_find_pane)
			m_find->writeQuery (query);
		m_combo->writeQuery (query);
		m_pool = Ypp::PkgQuery (m_packages, query);

		for (int i = 0; i < 3; i++) {
			GtkWidget *page = gtk_notebook_get_nth_page (notebook, i);
			if (page) {
				YGtkPackageView *view = YGTK_PACKAGE_VIEW (page);
				Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
				int n = i;
				if (m_onlineUpdate && n > 0) n++;
				switch (n) {
					case 0:  // available
						if (m_onlineUpdate)
							// special pane for patches upgrades makes little sense, so
							// we instead list them together with availables
							query->setHasUpgrade (true);
						query->setIsInstalled (false);
						break;
					case 1:  // upgrades
						query->setHasUpgrade (true);
						break;
					case 2:  // installed
						query->setIsInstalled (true);
						break;
					default: break;
				}
				Ypp::PkgQuery list (m_pool, query);
				if (!m_onlineUpdate) {
					const char *applyAll = NULL;
					if (!m_onlineUpdate && n == 1)
						applyAll = _("Upgrade All");
					view->setList (list, applyAll);
				}
				else {
					for (int i = 0; i < 6; i++) {
						Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
						query->setSeverity (i);
						Ypp::PkgQuery severity_list (list, query);
						std::string str (Ypp::Package::severityStr (i));
						view->appendList (str.c_str(), severity_list, NULL);
					}
				}
			}
		}

		if (!m_onlineUpdate) {
			// set tab label bold if there's a package there with the name
			if (m_highlightTab) {
				for (int i = 0; i < 3; i++)
					highlightPage (i, false);
				m_highlightTab = false;
			}
			const char *name;
			if (show_find_pane)
				name = m_combo->searchName();
			else
				name = m_find->searchName();
			if (name) {
				Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
				query->addNames (name, 0, true, false, false, false, false, true, true);
				Ypp::PkgQuery list (Ypp::Package::PACKAGE_TYPE, query);
				if (list.size()) {
					m_highlightTab = true;
					Ypp::Package *pkg = list.get (0);
					if (!pkg->isInstalled())
						highlightPage (0, true);
					else {
						highlightPage (2, true);
						if (pkg->hasUpgrade())
							highlightPage (1, true);
					}
				}
			}
		}
	}

	virtual void queryNotifyDelay()
	{
		struct inner {
			static gboolean timeout_cb (gpointer data)
			{
				QueryNotebook *pThis = (QueryNotebook *) data;
				pThis->m_timeout_id = 0;
				pThis->queryNotify();
				return FALSE;
			}
		};
		if (m_timeout_id) g_source_remove (m_timeout_id);
		m_timeout_id = g_timeout_add (500, inner::timeout_cb, this);
	}

	virtual void packagesSelected (Ypp::PkgList packages)
	{
		m_details->setPackages (packages);
		if (packages.size() > 0)
			gtk_widget_show (GTK_WIDGET (m_details));
	}

	// utilities
	void appendPage (int nb, const char *text, const char *stock)
	{
		GtkWidget *hbox = gtk_hbox_new (FALSE, 6), *label, *icon;
		if (stock) {
			icon = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_MENU);
			gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);
		}
		label = gtk_label_new (text);
		gtk_label_set_use_underline (GTK_LABEL (label), TRUE);
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
		g_object_set_data (G_OBJECT (hbox), "label", label);
		gtk_widget_show_all (hbox);

		GtkWidget *page;
		if (nb < 3) {
			int col;
			switch (nb) {
				default:
				case 0: col = ZyppModel::TO_INSTALL_COLUMN; break;
				case 1: col = ZyppModel::TO_UPGRADE_COLUMN; break;
				case 2: col = ZyppModel::TO_REMOVE_COLUMN; break;
			}
			YGtkPackageView *view = ygtk_package_view_new (FALSE);
			if (use_buttons) {
				//view->appendIconColumn (NULL, ZyppModel::ICON_COLUMN);
				view->appendTextColumn (NULL, ZyppModel::NAME_SUMMARY_COLUMN, 350);
				view->appendButtonColumn (NULL, col);
			}
			else {
				if (!m_onlineUpdate || nb == 0)
					view->appendCheckColumn (col);
				int nameSize = (col == ZyppModel::TO_UPGRADE_COLUMN) ? -1 : 150;
				view->appendTextColumn (_("Name"), ZyppModel::NAME_COLUMN, nameSize);
				if (col == ZyppModel::TO_UPGRADE_COLUMN) {
					view->appendTextColumn (_("Installed"), ZyppModel::INSTALLED_VERSION_COLUMN, 150);
					view->appendTextColumn (_("Available"), ZyppModel::AVAILABLE_VERSION_COLUMN, 150);
				}
				else
					view->appendTextColumn (_("Summary"), ZyppModel::SUMMARY_COLUMN);
			}
			view->setListener (this);
			page = GTK_WIDGET (view);
		}
		else {
			m_undoView = new UndoView (this);
			page = m_undoView->getWidget();
		}
		gtk_notebook_append_page (GTK_NOTEBOOK (m_notebook), page, hbox);
	}

	void enablePage (int page_nb, bool enabled, int new_page_nb = -1)
	{
		GtkNotebook *notebook = GTK_NOTEBOOK (m_notebook);
		GtkWidget *page = gtk_notebook_get_nth_page (notebook, page_nb);
		if (page) {
			GtkWidget *label = gtk_notebook_get_tab_label (notebook, page);
			gtk_widget_set_sensitive (label, enabled);
			const char *tooltip = getTooltip (page_nb);
			if (!enabled)
				tooltip = _("Query only applicable to available packages.");
			gtk_widget_set_tooltip_text (label, tooltip);
			if (!enabled && new_page_nb >= 0) {
				GtkWidget *selected = gtk_notebook_get_nth_page (notebook,
					gtk_notebook_get_current_page (notebook));
				if (selected == page)
					gtk_notebook_set_current_page (notebook, new_page_nb);
			}
		}
	}

	void highlightPage (int page_nb, bool highlight)
	{
		GtkNotebook *notebook = GTK_NOTEBOOK (m_notebook);
		GtkWidget *label = gtk_notebook_get_tab_label (notebook,
			gtk_notebook_get_nth_page (notebook, page_nb));
		label = (GtkWidget *) g_object_get_data (G_OBJECT (label), "label");
		YGUtils::setWidgetFont (label, PANGO_STYLE_NORMAL,
			highlight ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL, PANGO_SCALE_MEDIUM);
	}
};

class DiskView : public Ypp::Disk::Listener
{
	static GtkWidget *DiskList_new (GtkTreeModel *model, bool framed)
	{
		bool foreign_model = model == NULL;
		if (model == NULL) {
			GtkListStore *store = gtk_list_store_new (6,
				// 0 - mount point, 1 - usage percent, 2 - usage string,
				// (highlight warns) 3 - font weight, 4 - font color string,
				// 5 - delta description
				G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING,
				G_TYPE_STRING);
			model = GTK_TREE_MODEL (store);
		}

		GtkWidget *view = gtk_tree_view_new_with_model (model), *scroll = view;
		if (foreign_model)
			g_object_unref (G_OBJECT (model));
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
		gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)),
			                         GTK_SELECTION_NONE);

		GtkTreeViewColumn *column;
		column = gtk_tree_view_column_new_with_attributes (_("Mount Point"),
			gtk_cell_renderer_text_new(), "text", 0, "weight", 3, "foreground", 4, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
		column = gtk_tree_view_column_new_with_attributes (_("Usage"),
			gtk_cell_renderer_progress_new(), "value", 1, "text", 2, NULL);
		gtk_tree_view_column_set_min_width (column, 180);  // SIZE_REQUEST
		gtk_tree_view_column_set_expand (column, TRUE);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		g_object_set (G_OBJECT (renderer), "alignment", PANGO_ALIGN_RIGHT,
		              "style", PANGO_STYLE_ITALIC, NULL);
		column = gtk_tree_view_column_new_with_attributes ("Delta",
			renderer, "text", 5, NULL);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

		if (framed) {
			scroll = gtk_scrolled_window_new (NULL, NULL);
			gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
				                                 GTK_SHADOW_IN);
			gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					                        GTK_POLICY_NEVER, GTK_POLICY_NEVER);
			gtk_container_add (GTK_CONTAINER (scroll), view);
		}
		else
			g_object_set (view, "can-focus", FALSE, NULL);
		gtk_widget_show_all (scroll);
		return scroll;
	}

	static void DiskList_setup (GtkWidget *widget, int i, const std::string &path,
		int usage_percent, int delta_percent, const std::string &description,
		const std::string &changed, bool is_full)
	{
		GtkTreeView *view = GTK_TREE_VIEW (widget);
		GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (view));
		if (i == 0)
			gtk_list_store_clear (store);
		GtkTreeIter iter;
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, path.c_str(), 1, usage_percent,
			                2, description.c_str(), 3, PANGO_WEIGHT_NORMAL, 4, NULL,
			                5, changed.c_str(), -1);
		if (is_full)
			gtk_list_store_set (store, &iter, 3, PANGO_WEIGHT_BOLD, 4, "red", -1);
	}

GdkPixbuf *m_diskPixbuf, *m_diskFullPixbuf;
bool m_hasWarn;
GtkWidget *m_button, *m_view;

public:
	GtkWidget *getWidget()
	{ return m_button; }

	DiskView ()
	: m_hasWarn (false)
	{
		m_button = ygtk_menu_button_new();
		gtk_widget_set_tooltip_text (m_button, _("Disk usage"));
		gtk_container_add (GTK_CONTAINER (m_button), gtk_image_new_from_pixbuf (NULL));

		m_view = DiskList_new (NULL, false);
		ygtk_menu_button_set_popup_align (YGTK_MENU_BUTTON (m_button), m_view, 0.0, 1.0);

		m_diskPixbuf = YGUtils::loadPixbuf (std::string (DATADIR) + "/harddisk.png");
		m_diskFullPixbuf = YGUtils::loadPixbuf (std::string (DATADIR) + "/harddisk-full.png");

		Ypp::get()->getDisk()->setListener (this);
		update();
	}

	~DiskView()
	{
		g_object_unref (m_diskPixbuf);
		g_object_unref (m_diskFullPixbuf);
	}

private:
	virtual void update()
	{
		#define MIN_PERCENT_WARN 90
		#define MIN_FREE_MB_WARN (500*1024)
		bool warn = false;
		Ypp::Disk *disk = Ypp::get()->getDisk();
		for (int i = 0; disk->getPartition (i); i++) {
			const Ypp::Disk::Partition *part = disk->getPartition (i);

			int usage_percent = (part->used * 100) / (part->total + 1);
			int delta_percent = (part->delta * 100) / (part->total + 1);

			bool full = usage_percent > MIN_PERCENT_WARN &&
			            (part->total - part->used) < MIN_FREE_MB_WARN;
			if (full) warn = true;
			std::string description (part->used_str + _(" of ") + part->total_str);
			std::string delta_str;
			if (part->delta) {
				delta_str = (part->delta > 0 ? "(+" : "(");
				delta_str += part->delta_str + ")";
			}
			DiskList_setup (m_view, i, part->path, usage_percent, delta_percent,
			                description, delta_str, full);
		}
		GdkPixbuf *pixbuf = m_diskPixbuf;
		if (warn) {
			pixbuf = m_diskFullPixbuf;
			popupWarning();
		}
		gtk_image_set_from_pixbuf (GTK_IMAGE (GTK_BIN (m_button)->child), pixbuf);
	}

	void popupWarning()
	{
		if (m_hasWarn) return;
		m_hasWarn = true;
		if (!GTK_WIDGET_REALIZED (getWidget())) return;

		GtkWidget *dialog, *view;
		dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING,
			GTK_BUTTONS_OK, "%s", _("Disk Almost Full !"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s",
			_("One of the partitions is reaching its limit of capacity. You may "
			"have to remove packages if you wish to install some."));

		GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (m_view));
		view = DiskList_new (model, true);
		gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), view);
		g_signal_connect (G_OBJECT (dialog), "response",
		                  G_CALLBACK (gtk_widget_destroy), this);
		gtk_widget_show (dialog);
	}
};

class ToolsBox
{
GtkWidget *m_box;
DiskView *m_disk;

public:
	GtkWidget *getWidget() { return m_box; }

	ToolsBox()
	{
		GtkWidget *button, *popup, *item;
		button = ygtk_menu_button_new_with_label (_("Tools"));
		popup = gtk_menu_new();

		item = gtk_menu_item_new_with_label (_("Import List..."));
		gtk_menu_shell_append (GTK_MENU_SHELL (popup), item);
		g_signal_connect (G_OBJECT (item), "activate",
		                  G_CALLBACK (import_file_cb), this);
		item = gtk_menu_item_new_with_label (_("Export List..."));
		gtk_menu_shell_append (GTK_MENU_SHELL (popup), item);
		g_signal_connect (G_OBJECT (item), "activate",
		                  G_CALLBACK (export_file_cb), this);
		gtk_menu_shell_append (GTK_MENU_SHELL (popup), gtk_separator_menu_item_new());
		item = gtk_menu_item_new_with_label (_("Generate Dependency Testcase..."));
		gtk_menu_shell_append (GTK_MENU_SHELL (popup), item);
		g_signal_connect (G_OBJECT (item), "activate",
		                  G_CALLBACK (create_solver_testcase_cb), this);

		ygtk_menu_button_set_popup (YGTK_MENU_BUTTON (button), popup);
		gtk_widget_show_all (popup);

		m_disk = new DiskView();

		m_box = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (m_box), button, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_box), m_disk->getWidget(), FALSE, TRUE, 0);
		gtk_widget_show_all (m_box);
	}

	~ToolsBox()
	{ delete m_disk; }

private:
	static void errorMsg (const std::string &header, const std::string &message)
	{
		GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
			"%s", header.c_str());
		gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
			"%s", message.c_str());
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}

	static void import_file_cb (GtkMenuItem *item, ToolsBox *pThis)
	{
		GtkWidget *dialog;
		dialog = gtk_file_chooser_dialog_new (_("Import Package List"),
			YGDialog::currentWindow(), GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

		int ret = gtk_dialog_run (GTK_DIALOG (dialog));
		if (ret == GTK_RESPONSE_ACCEPT) {
			char *filename;
			filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
			if (!Ypp::get()->importList (filename)) {
				std::string error = _("Couldn't load package list from: ");
				error += filename;
				errorMsg (_("Import Failed"), error);
			}
			g_free (filename);
		}
		gtk_widget_destroy (dialog);
	}

	static void export_file_cb (GtkMenuItem *item, ToolsBox *pThis)
	{
		GtkWidget *dialog;
		dialog = gtk_file_chooser_dialog_new (_("Export Package List"),
			YGDialog::currentWindow(), GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

		int ret = gtk_dialog_run (GTK_DIALOG (dialog));
		if (ret == GTK_RESPONSE_ACCEPT) {
			char *filename;
			filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
			if (!Ypp::get()->exportList (filename)) {
				std::string error = _("Couldn't save package list to: ");
				error += filename;
				errorMsg (_("Export Failed"), error);
			}
			g_free (filename);
		}
		gtk_widget_destroy (dialog);
	}

	static void create_solver_testcase_cb (GtkMenuItem *item)
	{
		const char *dirname = "/var/log/YaST2/solverTestcase";
		std::string msg = _("Use this to generate extensive logs to help tracking down "
		                  "bugs in the dependency resolver.\nThe logs will be stored in "
		                  "directory: ");
		msg += dirname;

		GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK_CANCEL,
			"%s", _("Create Dependency Resolver Test Case"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", msg.c_str());
		int ret = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		if (ret == GTK_RESPONSE_OK) {
		    if (Ypp::get()->createSolverTestcase (dirname)) {
				GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
					GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO, "%s", _("Success"));
				msg = _("Dependency resolver test case written to");
				msg += " <tt>";
				msg += dirname;
				msg += "</tt>\n";
				msg += _("Prepare <tt>y2logs.tgz tar</tt> archive to attach to Bugzilla?");
				gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
                                                                                    "%s", msg.c_str());
				ret = gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				if (ret == GTK_RESPONSE_YES)
					YGUI::ui()->askSaveLogs();
		    }
		    else {
		    	msg = _("Failed to create dependency resolver test case.\n"
					"Please check disk space and permissions for");
				msg += " <tt>";
				msg += dirname;
				msg += "</tt>";
				errorMsg ("Error", msg.c_str());
		    }
		}
	}
};

//** Dialogs

static bool confirmExit()
{
	if (!Ypp::get()->isModified())
		return true;

    GtkWidget *dialog;
	dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
		 GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
		 "%s", _("Changes not saved!"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s",
		_("Quit anyway?"));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
	                        GTK_STOCK_QUIT, GTK_RESPONSE_YES, NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

    bool ok = gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES;
	gtk_widget_destroy (dialog);
    return ok;
}

static bool confirmPkgs (const char *title, const char *message,
	GtkMessageType icon, const Ypp::PkgList list, const char *extraColTitle, int extraCol)
{
	GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
		GtkDialogFlags (0), icon, GTK_BUTTONS_NONE, "%s", title);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", message);
	gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_NO, GTK_RESPONSE_NO,
		GTK_STOCK_YES, GTK_RESPONSE_YES, NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
	gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (dialog), -1, 480);
	gtk_widget_show_all (dialog);

	YGtkPackageView *view = ygtk_package_view_new (TRUE);
	view->appendCheckColumn (ZyppModel::TO_INSTALL_COLUMN);
	view->appendTextColumn (_("Name"), ZyppModel::NAME_COLUMN, 150);
	view->appendTextColumn (extraColTitle, extraCol);
	view->setList (list, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (view), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), GTK_WIDGET (view));

	bool confirm = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES);
	gtk_widget_destroy (dialog);
	delete view;
	return confirm;
}

static bool askConfirmUnsupported()
{
	Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
	query->setIsInstalled (false);
	query->setToModify (true);
	query->setIsUnsupported (true);

	Ypp::PkgQuery list (Ypp::Package::PACKAGE_TYPE, query);
	if (list.size() > 0)
		return confirmPkgs (_("Unsupported Packages"),	_("Please realize that the following "
			"software is either unsupported or requires an additional customer contract "
			"for support."), GTK_MESSAGE_WARNING, list,
			_("Supportability"), ZyppModel::SUPPORT_COLUMN);
	return true;
}

static bool acceptText (Ypp::Package *package, const std::string &title,
	const std::string &open, const std::string &text, bool question)
{
	GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
		(GtkDialogFlags) 0, question ? GTK_MESSAGE_QUESTION : GTK_MESSAGE_INFO,
		question ? GTK_BUTTONS_YES_NO : GTK_BUTTONS_OK,
		"%s %s", package->name().c_str(), title.c_str());
	if (!open.empty())
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
			"%s", open.c_str());
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

	GtkWidget *view = ygtk_html_wrap_new(), *scroll;
	ygtk_html_wrap_set_text (view, text.c_str(), FALSE);

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
		                            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type
		(GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (scroll), view);

	GtkBox *vbox = GTK_BOX (GTK_DIALOG(dialog)->vbox);
	gtk_box_pack_start (vbox, scroll, TRUE, TRUE, 6);

	gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 550, 450);
	gtk_widget_show_all (dialog);

	gint ret = gtk_dialog_run (GTK_DIALOG (dialog));
	bool confirmed = (ret == GTK_RESPONSE_YES);
	gtk_widget_destroy (dialog);
	return confirmed;
}

static bool resolveProblems (const std::list <Ypp::Problem *> &problems)
{
	// we can't use ordinary radio buttons, as gtk+ enforces that in a group
	// one must be selected...
	#define DETAILS_PAD  25
	enum ColumnAlias {
		SHOW_TOGGLE_COL, ACTIVE_TOGGLE_COL, TEXT_COL, WEIGHT_TEXT_COL,
		TEXT_PAD_COL, APPLY_PTR_COL, TOTAL_COLS
	};

	struct inner {
		static void solution_toggled (GtkTreeModel *model, GtkTreePath *path)
		{
			GtkTreeStore *store = GTK_TREE_STORE (model);
			GtkTreeIter iter, parent;

			gboolean enabled;
			bool *apply;
			gtk_tree_model_get_iter (model, &iter, path);
			gtk_tree_model_get (model, &iter, ACTIVE_TOGGLE_COL, &enabled,
			                    APPLY_PTR_COL, &apply, -1);
			if (!apply)
				return;

			// disable all the other radios on the group, setting current
			gtk_tree_model_get_iter (model, &iter, path);
			if (gtk_tree_model_iter_parent (model, &parent, &iter)) {
				gtk_tree_model_iter_children (model, &iter, &parent);
				do {
					gtk_tree_store_set (store, &iter, ACTIVE_TOGGLE_COL, FALSE, -1);
					bool *apply;
					gtk_tree_model_get (model, &iter, APPLY_PTR_COL, &apply, -1);
					if (apply) *apply = false;
				} while (gtk_tree_model_iter_next (model, &iter));
			}

			enabled = !enabled;
			*apply = enabled;
			gtk_tree_model_get_iter (model, &iter, path);
			gtk_tree_store_set (store, &iter, ACTIVE_TOGGLE_COL, enabled, -1);
		}
		static void cursor_changed_cb (GtkTreeView *view, GtkTreeModel *model)
		{
			GtkTreePath *path;
			gtk_tree_view_get_cursor (view, &path, NULL);
			solution_toggled (model, path);
			gtk_tree_path_free (path);
		}
	};

	// model
	GtkTreeStore *store = gtk_tree_store_new (TOTAL_COLS,
		G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_INT,
		G_TYPE_INT, G_TYPE_POINTER);
	for (std::list <Ypp::Problem *>::const_iterator it = problems.begin();
	     it != problems.end(); it++) {
		GtkTreeIter problem_iter;
		gtk_tree_store_append (store, &problem_iter, NULL);
		gtk_tree_store_set (store, &problem_iter, SHOW_TOGGLE_COL, FALSE,
			TEXT_COL, (*it)->description.c_str(), WEIGHT_TEXT_COL, PANGO_WEIGHT_BOLD, -1);
		if (!(*it)->details.empty()) {
			GtkTreeIter details_iter;
			gtk_tree_store_append (store, &details_iter, &problem_iter);
			gtk_tree_store_set (store, &details_iter, SHOW_TOGGLE_COL, FALSE,
				TEXT_COL, (*it)->details.c_str(), TEXT_PAD_COL, DETAILS_PAD, -1);
		}

		for (int i = 0; (*it)->getSolution (i); i++) {
			Ypp::Problem::Solution *solution = (*it)->getSolution (i);
			GtkTreeIter solution_iter;
			gtk_tree_store_append (store, &solution_iter, &problem_iter);
			gtk_tree_store_set (store, &solution_iter, SHOW_TOGGLE_COL, TRUE,
				WEIGHT_TEXT_COL, PANGO_WEIGHT_NORMAL,
				ACTIVE_TOGGLE_COL, FALSE, TEXT_COL, solution->description.c_str(),
				APPLY_PTR_COL, &solution->apply, -1);
			if (!solution->details.empty()) {
				gtk_tree_store_append (store, &solution_iter, &problem_iter);
				gtk_tree_store_set (store, &solution_iter, SHOW_TOGGLE_COL, FALSE,
					WEIGHT_TEXT_COL, PANGO_WEIGHT_NORMAL,
					TEXT_COL, solution->details.c_str(), TEXT_PAD_COL, DETAILS_PAD, -1);
			}
		}
	}

	// interface
	GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
		GtkDialogFlags (0), GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, "%s",
		_("There are some conflicts on the transaction that must be solved manually."));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_APPLY, GTK_RESPONSE_APPLY, NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_APPLY);

	GtkWidget *view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	g_object_unref (G_OBJECT (store));
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (
		GTK_TREE_VIEW (view)), GTK_SELECTION_NONE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (view), TEXT_COL);
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	renderer = gtk_cell_renderer_toggle_new();
	gtk_cell_renderer_toggle_set_radio (
		GTK_CELL_RENDERER_TOGGLE (renderer), TRUE);
	// we should not connect the actual toggle button, as we toggle on row press
	g_signal_connect (G_OBJECT (view), "cursor-changed",
		G_CALLBACK (inner::cursor_changed_cb), store);
	column = gtk_tree_view_column_new_with_attributes ("", renderer,
		"visible", SHOW_TOGGLE_COL, "active", ACTIVE_TOGGLE_COL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (G_OBJECT (renderer), "wrap-width", 400, NULL);
	column = gtk_tree_view_column_new_with_attributes ("", renderer,
		"text", TEXT_COL, "weight", WEIGHT_TEXT_COL, "xpad", TEXT_PAD_COL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
	gtk_tree_view_expand_all (GTK_TREE_VIEW (view));
	gtk_widget_set_has_tooltip (view, TRUE);

	GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
		GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (scroll), view);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), scroll);

	gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (dialog), -1, 480);
	gtk_widget_show_all (dialog);

	bool apply = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_APPLY);
	gtk_widget_destroy (dialog);
	return apply;
}

#include <YPackageSelector.h>
#include "pkg-selector-help.h"

class YGPackageSelector : public YPackageSelector, public YGWidget, public Ypp::Interface
{
QueryNotebook *m_notebook;
ToolsBox *m_tools;
GtkWidget *m_progressbar;
guint m_button_timeout_id;

public:
	YGPackageSelector (YWidget *parent, long mode)
	: YPackageSelector (NULL, mode)
	, YGWidget (this, parent, YGTK_TYPE_WIZARD, NULL)
	{
		setBorder (0);
		bool onlineUpdate = onlineUpdateMode();
		YGDialog *dialog = YGDialog::currentDialog();
		dialog->setCloseCallback (confirm_cb, this);
		dialog->setMinSize (700, 800);  // enlarge

		YGtkWizard *wizard = YGTK_WIZARD (getWidget());
		ygtk_wizard_set_header_icon (wizard,
			THEMEDIR "/icons/22x22/apps/yast-software.png");
		const char *title = onlineUpdate ? _("Online Update") : _("Software Manager");
		ygtk_wizard_set_header_text (wizard, title);
		dialog->setTitle (title);
		ygtk_wizard_set_help_text (wizard, _("Please wait..."));

		ygtk_wizard_set_button_label (wizard,  wizard->abort_button,
		                              _("_Cancel"), GTK_STOCK_CANCEL);
		ygtk_wizard_set_button_str_id (wizard, wizard->abort_button, "cancel");
		ygtk_wizard_set_button_label (wizard,  wizard->back_button, NULL, NULL);
		ygtk_wizard_set_button_label (wizard,  wizard->next_button,
		                              _("A_pply"), GTK_STOCK_APPLY);
		ygtk_wizard_set_button_str_id (wizard, wizard->next_button, "accept");
		g_signal_connect (G_OBJECT (wizard), "action-triggered",
		                  G_CALLBACK (wizard_action_cb), this);
		ygtk_wizard_enable_button (wizard, wizard->next_button, FALSE);

		m_tools = new ToolsBox();
		ygtk_wizard_set_extra_button (YGTK_WIZARD (getWidget()), m_tools->getWidget());

		m_progressbar = gtk_progress_bar_new();
		GtkWidget *align = gtk_alignment_new (0, .95, 1, 0);
		gtk_container_add (GTK_CONTAINER (align), m_progressbar);

		GtkWidget *vbox = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_end (GTK_BOX (vbox), align, FALSE, TRUE, 0);
		gtk_widget_show (align);
		gtk_widget_show (vbox);
		ygtk_wizard_set_child (YGTK_WIZARD (wizard), vbox);
		Ypp::get()->setInterface (this);

		m_notebook = new QueryNotebook (onlineUpdate, repoMgrEnabled());
		gtk_box_pack_start (GTK_BOX (vbox), m_notebook->getWidget(), TRUE, TRUE, 0);

		const char **help = onlineUpdate ? patch_help : pkg_help;
		std::string str;
		str.reserve (6144);
		for (int i = 0; help[i]; i++)
			str.append (help [i]);
		ygtk_wizard_set_help_text (wizard, str.c_str());
		m_button_timeout_id = g_timeout_add (7500, wizard_enable_button_cb, this);
	}

	virtual ~YGPackageSelector()
	{
		if (m_button_timeout_id)
			g_source_remove (m_button_timeout_id);
		delete m_notebook;
		delete m_tools;
		ygtk_zypp_finish();
	}

	// (Y)Gtk callbacks
	static gboolean wizard_enable_button_cb (gpointer data)
	{
		YGPackageSelector *pThis = (YGPackageSelector *) data;
		pThis->m_button_timeout_id = 0;
		YGtkWizard *wizard = YGTK_WIZARD (pThis->getWidget());
		ygtk_wizard_enable_button (wizard, wizard->next_button, TRUE);
		return FALSE;
	}

	static gboolean wizard_confirm_button_cb (gpointer data)
	{
		YGPackageSelector *pThis = (YGPackageSelector *) data;
		pThis->m_button_timeout_id = 0;
		YGtkWizard *wizard = YGTK_WIZARD (pThis->getWidget());
		ygtk_wizard_set_button_label (wizard,  wizard->next_button,
					                  _("A_pply"), GTK_STOCK_APPLY);
		return FALSE;
	}

    static bool confirm_cb (void *pThis)
    { return confirmExit(); }

	static void wizard_action_cb (YGtkWizard *wizard, gpointer id,
	                              gint id_type, YGPackageSelector *pThis)
	{
		IMPL
		const gchar *action = (gchar *) id;
		yuiMilestone() << "Closing PackageSelector with '" << action << "'\n";
		if (!strcmp (action, "accept")) {
			if (!pThis->onlineUpdateMode()) {
				if (pThis->confirmUnsupported())
					if (!askConfirmUnsupported())
						return;
				if (Ypp::get()->isModified() && !pThis->m_button_timeout_id) {
					pThis->m_notebook->setUndoPage();
					YGtkWizard *wizard = YGTK_WIZARD (pThis->getWidget());
					ygtk_wizard_set_button_label (wizard,  wizard->next_button,
								                  _("_Sure?"), GTK_STOCK_OK);
					pThis->m_button_timeout_id =
						g_timeout_add (10*1000, wizard_confirm_button_cb, pThis);
					return;
				}
			}
			YGUI::ui()->sendEvent (new YMenuEvent ("accept"));
		}
		else if (!strcmp (action, "cancel"))
			if (confirmExit())
				YGUI::ui()->sendEvent (new YCancelEvent());
	}

	// Ypp callbacks
	virtual bool acceptLicense (Ypp::Package *package, const std::string &license)
	{
		return acceptText (package, _("License Agreement"),
			_("Do you accept the terms of this license?"), license, true);
	}

	virtual void notifyMessage (Ypp::Package *package, const std::string &msg)
	{ acceptText (package, _("Notification"), "", msg, false); }

	virtual bool resolveProblems (const std::list <Ypp::Problem *> &problems)
	{ return ::resolveProblems (problems); }

	virtual bool allowRestrictedRepo (const Ypp::PkgList list)
	{
		return confirmPkgs (_("Dependencies from Filtered Repositories"),
			_("The following packages have necessary dependencies that are not provided "
			  "by the filtered repository. Install them?"), GTK_MESSAGE_WARNING, list,
			  _("Repository"), ZyppModel::REPOSITORY_COLUMN);
	}

	virtual void loading (float progress)
	{
		if (progress == 1) {
			if (GTK_WIDGET_VISIBLE (m_progressbar)) {
				gtk_widget_hide (m_progressbar);
				while (g_main_context_iteration (NULL, FALSE)) ;
			}
			YGUI::ui()->normalCursor();
		}
		else {
			if (progress == 0)
				YGUI::ui()->busyCursor();
			else {  // progress=0 may be to trigger cursor only
				gtk_widget_show (m_progressbar);
				gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (m_progressbar), progress);
			}
			while (g_main_context_iteration (NULL, FALSE)) ;
		}
	}

	YGWIDGET_IMPL_COMMON (YPackageSelector)
};

YPackageSelector *YGWidgetFactory::createPackageSelector (YWidget *parent, long mode)
{ return new YGPackageSelector (parent, mode); }

