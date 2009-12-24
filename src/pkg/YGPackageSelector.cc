/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/*
  Textdomain "yast2-gtk"
 */

#define YUILogComponent "gtk"
#include "config.h"
#include <YPackageSelector.h>
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
#include "ygtktooltip.h"
#include "ygtkpackageview.h"
#include "ygtkdetailview.h"
#include "ygtkdiskview.h"
#include "ygtknotebook.h"

// experiments:
extern bool search_entry_side, search_entry_top, dynamic_sidebar,
	status_side, status_tabs, status_tabs_as_actions, undo_side, undo_tab,
	undo_old_style, status_col, action_col, action_col_as_button,
	action_col_as_check, version_col, single_line_rows, details_start_hide,
	toolbar_top, toolbar_yast;

//** UI components -- split up for re-usability, but mostly for readability

class UndoView
{
Ypp::PkgList m_changes;

public:
//	GtkWidget *getWidget() { return m_vbox; }

	UndoView()
	{
		Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
		query->setToModify (true);
		m_changes = Ypp::PkgQuery (Ypp::Package::PACKAGE_TYPE, query);
#if 0
		GtkWidget *view = createView (listener);

		m_vbox = gtk_vbox_new (FALSE, 6);
		gtk_container_set_border_width (GTK_CONTAINER (m_vbox), 6);
		gtk_box_pack_start (GTK_BOX (m_vbox), view, TRUE, TRUE, 0);
#if 0
// FIXME: temporarily disable uncouple button
// issues: * too visible (also the labeling/icon are a bit off)
//         * undo window should not overlap (or at least placed at the side on start)
		GtkWidget *uncouple_button = gtk_button_new_with_label (_("Uncouple"));
		GtkWidget *icon = gtk_image_new_from_stock (GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image (GTK_BUTTON (uncouple_button), icon);
		GtkWidget *uncouple_align = gtk_alignment_new (1, .5, 0, 1);
		gtk_container_add (GTK_CONTAINER (uncouple_align), uncouple_button);
		gtk_widget_set_tooltip_text (uncouple_button, _("Open in new window"));
		g_signal_connect (G_OBJECT (uncouple_button), "clicked",
		                  G_CALLBACK (uncouple_clicked_cb), this);
		gtk_box_pack_start (GTK_BOX (m_vbox), uncouple_align, FALSE, TRUE, 0);
#endif
		gtk_widget_show_all (m_vbox);
		g_object_ref_sink (m_vbox);
#endif
	}

#if 0
	~UndoView()
	{ g_object_unref (m_vbox); }
#endif

	GtkWidget *createView (YGtkPackageView::Listener *listener, bool horizontal)
	{
		GtkWidget *main_box = horizontal ? gtk_hbox_new (TRUE, 6) : gtk_vbox_new (TRUE, 6);
		for (int i = 0; i < 3; i++) {
			const char *str = 0, *stock = 0;
			Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
			const char *prop;
			switch (i) {
				case 0:
					str = _("To install:");
					stock = GTK_STOCK_ADD;
					query->setToInstall (true);
					query->setIsInstalled (false);
					prop = "to-install";
					break;
				case 1:
					str = _("To upgrade:");
					stock = GTK_STOCK_GO_UP;
					query->setToInstall (true);
					query->setIsInstalled (true);
					prop = "to-upgrade";
					break;
				case 2:
					str = _("To remove:");
					stock = GTK_STOCK_REMOVE;
					query->setToRemove (true);
					prop = "to-remove";
					break;
				default: break;
			}
			Ypp::PkgQuery list (m_changes, query);

			GtkWidget *label_box, *icon, *label;
			label = gtk_label_new (str);
			gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
			icon = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_MENU);
			label_box = gtk_hbox_new (FALSE, 6);
			gtk_box_pack_start (GTK_BOX (label_box), icon, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (label_box), label, TRUE, TRUE, 0);
			YGtkPackageView *view = ygtk_package_view_new (FALSE);
			view->setVisible ("available-version", false);
/*			view->appendCheckColumn (checkCol);
			view->appendTextColumn (NULL, NAME_PROP, -1, true);*/
			view->setActivateAction (YGtkPackageView::UNDO_ACTION);
			view->setList (list, NULL);
			view->setListener (listener);
			gtk_scrolled_window_set_shadow_type (
				GTK_SCROLLED_WINDOW (view), GTK_SHADOW_IN);

			GtkWidget *box = gtk_vbox_new (FALSE, 6);
			gtk_box_pack_start (GTK_BOX (box), label_box, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (view), TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (main_box), box, TRUE, TRUE, 0);
		}
		return main_box;
	}
#if 0
private:
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

		GtkWidget *view = pThis->createView (NULL, true);
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
#endif
};

class ChangesPane : public Ypp::PkgList::Listener
{
	struct Entry {
		GtkWidget *m_box, *m_label, *m_button;
		GtkWidget *getWidget() { return m_box; }

		Entry (Ypp::Package *package)
		{
			m_label = gtk_label_new ("");
			gtk_misc_set_alignment (GTK_MISC (m_label), 0, 0.5);
			gtk_label_set_ellipsize (GTK_LABEL (m_label), PANGO_ELLIPSIZE_END);
			m_button = gtk_button_new();
			gtk_widget_set_tooltip_text (m_button, _("Undo"));
			GtkWidget *undo_image = gtk_image_new_from_stock (GTK_STOCK_UNDO, GTK_ICON_SIZE_MENU);
			gtk_button_set_image (GTK_BUTTON (m_button), undo_image);
			m_box = gtk_hbox_new (FALSE, 6);
			gtk_box_pack_start (GTK_BOX (m_box), m_label, TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (m_box), m_button, FALSE, FALSE, 0);
			gtk_widget_show_all (m_box);
			modified (package);
			g_signal_connect (G_OBJECT (m_button), "clicked",
			                  G_CALLBACK (undo_clicked_cb), package);
		}

		void modified (Ypp::Package *package)
		{
			const Ypp::Package::Version *version = 0;
			std::string text, action;
			if (package->toInstall (&version)) {
				if (package->isInstalled()) {
					if (version->cmp > 0)
						action = _("upgrade");
					else if (version->cmp < 0)
						action = _("downgrade");
					else
						action = _("re-install");
				}
				else if (package->type() == Ypp::Package::PATCH_TYPE)
					action = _("patch");
				else
					action = _("install");
			}
			else
				action = _("remove");
			text = action + " " + package->name();
			if (package->isAuto()) {
				text = "\t" + text;
				gtk_widget_hide (m_button);
			}
			else
				gtk_widget_show (m_button);
			gtk_label_set_text (GTK_LABEL (m_label), text.c_str());
			std::string tooltip = action + " " + package->name();
			if (version)
				tooltip += std::string (_("\nfrom")) + " <i>" + version->repo->name + "</i>";
			gtk_widget_set_tooltip_markup (m_label, tooltip.c_str());
		}

		static void undo_clicked_cb (GtkButton *button, Ypp::Package *package)
		{
			package->undo();
		}
	};

GtkWidget *m_box, *m_entries_box;
Ypp::PkgList m_pool;
GList *m_entries;

public:
	GtkWidget *getWidget()
	{ return m_box; }

	ChangesPane()
	: m_entries (NULL)
	{
		GtkWidget *heading = gtk_label_new (_("Changes:"));
		YGUtils::setWidgetFont (heading, PANGO_STYLE_NORMAL, PANGO_WEIGHT_BOLD, PANGO_SCALE_MEDIUM);
		gtk_misc_set_alignment (GTK_MISC (heading), 0, 0.5);
		m_entries_box = gtk_vbox_new (FALSE, 4);

		GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
		                                GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroll),
		                                       m_entries_box);
		GtkWidget *port = gtk_bin_get_child (GTK_BIN (scroll));
		gtk_viewport_set_shadow_type (GTK_VIEWPORT (port), GTK_SHADOW_NONE);

		GtkWidget *vbox = gtk_vbox_new (FALSE, 6);
		gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
		gtk_box_pack_start (GTK_BOX (vbox), heading, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);

		ygtk_wizard_set_information_expose_hook (vbox, &vbox->allocation);
		ygtk_wizard_set_information_expose_hook (m_entries_box, &m_entries_box->allocation);

		int width = YGUtils::getCharsWidth (vbox, 32);
		gtk_widget_set_size_request (vbox, width, -1);
		gtk_widget_show_all (vbox);

		m_box = gtk_handle_box_new();
		gtk_container_add (GTK_CONTAINER (m_box), vbox);
		gtk_handle_box_set_handle_position (GTK_HANDLE_BOX (m_box), GTK_POS_TOP);
		gtk_handle_box_set_snap_edge (GTK_HANDLE_BOX (m_box), GTK_POS_RIGHT);

		Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
		query->setToModify (true);
/*		if (pkg_selector->onlineUpdateMode())
			query->addType (Ypp::Package::PATCH_TYPE);*/
		m_pool = Ypp::PkgQuery (Ypp::Package::PACKAGE_TYPE, query);
		// initialize list -- there could already be packages modified
		for (int i = 0; i < m_pool.size(); i++)
			ChangesPane::entryInserted (m_pool, i, m_pool.get (i));
		m_pool.addListener (this);
	}

	~ChangesPane()
	{
		for (GList *i = m_entries; i; i = i->next)
			delete (Entry *) i->data;
		g_list_free (m_entries);
	}

	void startHack()  // call after init, after you did a show_all in the dialog
	{
		UpdateVisible();
		// ugly: signal modified for all entries to allow them to hide undo buttons
		GList *i;
		int index;
		for (index = 0, i = m_entries; index < m_pool.size() && i;
		     index++, i = i->next)
			((Entry *) i->data)->modified (m_pool.get (index));
	}

	void UpdateVisible()
	{
		if (details_start_hide)
			m_entries != NULL ? gtk_widget_show (m_box) : gtk_widget_hide (m_box);
	}

	virtual void entryInserted (const Ypp::PkgList list, int index, Ypp::Package *package)
	{
		Entry *entry = new Entry (package);
		gtk_box_pack_start (GTK_BOX (m_entries_box), entry->getWidget(), FALSE, TRUE, 0);
		m_entries = g_list_insert (m_entries, entry, index);
		UpdateVisible();
	}

	virtual void entryDeleted  (const Ypp::PkgList list, int index, Ypp::Package *package)
	{
		GList *i = g_list_nth (m_entries, index);
		Entry *entry = (Entry *) i->data;
		gtk_container_remove (GTK_CONTAINER (m_entries_box), entry->getWidget());
		delete entry;
		m_entries = g_list_delete_link (m_entries, i);
		UpdateVisible();
	}

	virtual void entryChanged  (const Ypp::PkgList list, int index, Ypp::Package *package)
	{
		Entry *entry = (Entry *) g_list_nth_data (m_entries, index);
		entry->modified (package);
	}
};

#if 0
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
	virtual ~QueryWidget() {}
};

class StoreView : public QueryWidget
{
protected:
	GtkWidget *m_view, *m_scroll, *m_box;
	enum Column { TEXT_COL, ICON_COL, ENABLED_COL, PTR_COL, TOOLTIP_COL, TOTAL_COLS };

	virtual void doBuild (GtkTreeStore *store) = 0;
	virtual void writeQuerySel (Ypp::PkgQuery::Query *query,
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
		writeQuerySel (query, ptrs);
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

	virtual void writeQuerySel (Ypp::PkgQuery::Query *query, const std::list <gpointer> &ptrs)
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
			box = gtk_hbox_new (FALSE, 6);
			GtkSettings *settings = gtk_settings_get_default();
			gboolean button_images;
			g_object_get (settings, "gtk-button-images", &button_images, NULL);
			if (button_images) {
				image = gtk_image_new_from_stock (GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
				gtk_box_pack_start (GTK_BOX (box), image, FALSE, TRUE, 0);
			}
			label = gtk_label_new (_("Edit..."));
			YGUtils::setWidgetFont (label, PANGO_STYLE_NORMAL, PANGO_WEIGHT_NORMAL, PANGO_SCALE_SMALL);
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

	virtual ~Repositories()
	{ Ypp::get()->setFavoriteRepository (NULL); }

protected:
	virtual void doBuild (GtkTreeStore *store)
	{
		GtkTreeIter iter;
		for (int i = 0; Ypp::get()->getRepository (i); i++) {
			const Ypp::Repository *repo = Ypp::get()->getRepository (i);
			gtk_tree_store_append (store, &iter, NULL);
			std::string text = repo->name, url (repo->url);
			url = YGUtils::escapeMarkup (url);
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
				ICON_COL, icon, ENABLED_COL, repo->enabled, PTR_COL, repo,
				TOOLTIP_COL, text.c_str(), -1);
		}
	}

	virtual void writeQuerySel (Ypp::PkgQuery::Query *query, const std::list <gpointer> &ptrs)
	{
		for (std::list <gpointer>::const_iterator it = ptrs.begin();
		     it != ptrs.end(); it++) {
			Ypp::Repository *repo = (Ypp::Repository *) *it;
			query->addRepository (repo);
		}
		gpointer ptr = ptrs.size() == 1 ? ptrs.front() : NULL;
		Ypp::get()->setFavoriteRepository ((Ypp::Repository *) ptr);
	}

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
		m_view->appendIconColumn (NULL, ICON_PROP);
		m_view->appendTextColumn (NULL, NAME_SUMMARY_PROP);
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
			gtk_image_new_from_stock (GTK_STOCK_INFO, GTK_ICON_SIZE_MENU), FALSE, TRUE, 0);
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
		g_signal_connect (G_OBJECT (m_name), "realize",
		                  G_CALLBACK (realize_cb), this);
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

	static void realize_cb (GtkWidget *widget, FindEntry *pThis)
	{ gtk_widget_grab_focus (widget); }
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
		GtkWidget *pkg_details_pane = gtk_vpaned_new();
		gtk_paned_pack1 (GTK_PANED (pkg_details_pane), m_notebook, TRUE, FALSE);
		gtk_paned_pack2 (GTK_PANED (pkg_details_pane), GTK_WIDGET (m_details), FALSE, TRUE);
		gtk_paned_set_position (GTK_PANED (pkg_details_pane), 600);

		m_combo = new FilterCombo (onlineUpdate, repoMgrEnabled);
		m_combo->setListener (this);
		m_combo->select (0);
		GtkWidget *hpane = gtk_hpaned_new();
		gtk_paned_pack1 (GTK_PANED (hpane), m_combo->getWidget(), FALSE, TRUE);
		gtk_paned_pack2 (GTK_PANED (hpane), pkg_details_pane, TRUE, FALSE);
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
		delete m_find;
	}

	bool confirmChanges()
	{
		if (m_onlineUpdate) return true;
		if (gtk_notebook_get_current_page (GTK_NOTEBOOK (m_notebook)) == 3) return true;

		GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
			"%s", _("Apply changes?"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
			_("Please review the changes to perfom."));
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
			GTK_STOCK_APPLY, GTK_RESPONSE_YES, NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
		gtk_window_set_default_size (GTK_WINDOW (dialog), 550, 500);
		gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);

		GtkWidget *view = m_undoView->createView (NULL);
		gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), view);
		gtk_widget_show_all (dialog);

		gint response = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return response == GTK_RESPONSE_YES;
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
		if (availablePackagesOnly()) {
			enablePage (m_onlineUpdate ? 1 : 2, false, 0);
			m_disabledTab = true;
		}
		else if (installedPackagesOnly()) {
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

	inline bool availablePackagesOnly()
	{ return m_combo->availablePackagesOnly(); }

	inline bool installedPackagesOnly()
	{
		if (m_find && m_find->installedPackagesOnly())
			return true;
		return m_combo->installedPackagesOnly();
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
				case 0: col = TO_INSTALL_PROP; break;
				case 1: col = TO_UPGRADE_PROP; break;
				case 2: col = TO_REMOVE_PROP; break;
			}
			YGtkPackageView *view = ygtk_package_view_new (FALSE);
			if (use_buttons) {
				//view->appendIconColumn (NULL, ICON_PROP);
				view->appendTextColumn (NULL, NAME_SUMMARY_PROP, 350);
				view->appendButtonColumn (NULL, col);
				view->setRulesHint (true);
			}
			else {
				if (m_onlineUpdate && nb > 0)
					view->appendEmptyColumn (25);
				else
					view->appendCheckColumn (col);
				view->appendTextColumn (_("Name"), NAME_PROP, 160);
				if (col == TO_UPGRADE_PROP) {
					view->appendTextColumn (_("Installed"), INSTALLED_VERSION_PROP, -1);
					view->appendTextColumn (_("Available"), AVAILABLE_VERSION_PROP, -1);
				}
				else
					view->appendTextColumn (_("Summary"), SUMMARY_PROP);
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
#endif

class ToolsBox
{
GtkWidget *m_box;

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

		m_box = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (m_box), button, FALSE, TRUE, 0);
		gtk_widget_show_all (m_box);
	}

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

static bool confirmCancel()
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

bool confirmApply()
{
	if (!Ypp::get()->isModified())
		return true;

	GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
		GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
		"%s", _("Apply changes?"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
		_("Please review the changes to perfom."));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
		GTK_STOCK_APPLY, GTK_RESPONSE_YES, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 550, 500);
	gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);

#if 0
	GtkWidget *view = m_undoView->createView (NULL);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), view);
#endif
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
		gtk_label_new ("TO DO\n\nTo download: xxx\nHard disk needed: xxx"));
	gtk_widget_show_all (dialog);

	gint response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	return response == GTK_RESPONSE_YES;
}

static bool confirmPkgs (const char *title, const char *message,
	GtkMessageType icon, const Ypp::PkgList list,
	const std::string &extraProp)
{
	GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
		GtkDialogFlags (0), icon, GTK_BUTTONS_NONE, "%s", title);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", message);
	gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
		GTK_STOCK_APPLY, GTK_RESPONSE_YES, NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
	gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (dialog), -1, 500);
	gtk_widget_show_all (dialog);

	YGtkPackageView *view = ygtk_package_view_new (TRUE);
	view->setVisible (extraProp, true);
	view->setList (list, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (view), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), GTK_WIDGET (view));

	bool confirm = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES);
	gtk_widget_destroy (dialog);
	return confirm;
}

static bool askConfirmUnsupported()
{
	Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
	query->setIsInstalled (false);
	query->setToModify (true);
	query->setIsSupported (false);

	Ypp::PkgQuery list (Ypp::Package::PACKAGE_TYPE, query);
	if (list.size() > 0)
		return confirmPkgs (_("Unsupported Packages"),	_("Please realize that the following "
			"software is either unsupported or requires an additional customer contract "
			"for support."), GTK_MESSAGE_WARNING, list, "support");
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
	gtk_window_set_default_size (GTK_WINDOW (dialog), 550, 500);
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
	gtk_window_set_default_size (GTK_WINDOW (dialog), -1, 500);
	gtk_widget_show_all (dialog);

	bool apply = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_APPLY);
	gtk_widget_destroy (dialog);
	return apply;
}

struct _QueryListener
{
	virtual void refresh() = 0;
};

struct _QueryWidget
{
	virtual GtkWidget *getWidget() = 0;

	virtual void updateType (Ypp::Package::Type type) = 0;
	virtual void updateList (Ypp::PkgList list) = 0;
	virtual bool begsUpdate() = 0;

	virtual bool writeQuery (Ypp::PkgQuery::Query *query) = 0;

	void setListener (_QueryListener *listener)
	{ m_listener = listener; }

//protected:
	_QueryWidget() : m_listener (NULL) {}
	virtual ~_QueryWidget() {}

	void notify() { if (m_listener) m_listener->refresh(); }

	void notifyDelay()
	{
		static guint timeout_id = 0;
		struct inner {
			static gboolean timeout_cb (gpointer data)
			{
				_QueryWidget *pThis = (_QueryWidget *) data;
				timeout_id = 0;
				pThis->notify();
				return FALSE;
			}
		};
		if (timeout_id) g_source_remove (timeout_id);
		timeout_id = g_timeout_add (500, inner::timeout_cb, this);
	}

	bool modified;  // flag for internal use

private:
	_QueryListener *m_listener;
};

class SearchEntry : public _QueryWidget
{
GtkWidget *m_widget, *m_entry, *m_combo;
bool m_clearIcon;

public:
	SearchEntry (bool combo_props) : _QueryWidget()
	{
		m_entry = gtk_entry_new();
		g_signal_connect (G_OBJECT (m_entry), "realize",
		                  G_CALLBACK (grab_focus_cb), NULL);
		gtk_widget_set_size_request (m_entry, 50, -1);
		GtkWidget *entry_hbox = gtk_hbox_new (FALSE, 2), *find_label;
		find_label = gtk_label_new_with_mnemonic (_("_Find:"));
		gtk_box_pack_start (GTK_BOX (entry_hbox), find_label, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (entry_hbox), m_entry, TRUE, TRUE, 0);
		gtk_label_set_mnemonic_widget (GTK_LABEL (find_label), m_entry);

		m_clearIcon = false;
		g_signal_connect (G_OBJECT (m_entry), "changed",
		                  G_CALLBACK (entry_changed_cb), this);
		g_signal_connect (G_OBJECT (m_entry), "icon-press",
		                  G_CALLBACK (icon_press_cb), this);

		m_combo = 0;
		if (combo_props) {
			m_combo = gtk_combo_box_new_text();
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_combo), _("Name & Summary"));
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_combo), _("Description"));
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_combo), _("File List"));
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_combo), _("Author"));
			gtk_combo_box_set_active (GTK_COMBO_BOX (m_combo), 0);
			g_signal_connect (G_OBJECT (m_combo), "changed",
				              G_CALLBACK (combo_changed_cb), this);

			GtkWidget *opt_hbox = gtk_hbox_new (FALSE, 2), *empty = gtk_event_box_new();
			gtk_box_pack_start (GTK_BOX (opt_hbox), empty, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (opt_hbox), gtk_label_new (_("by")), FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (opt_hbox), m_combo, TRUE, TRUE, 0);

			GtkSizeGroup *group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
			gtk_size_group_add_widget (group, find_label);
			gtk_size_group_add_widget (group, empty);
			g_object_unref (G_OBJECT (group));

			m_widget = gtk_vbox_new (FALSE, 4);
			gtk_box_pack_start (GTK_BOX (m_widget), entry_hbox, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (m_widget), opt_hbox, FALSE, TRUE, 0);
		}
		else
			m_widget = entry_hbox;
		gtk_widget_show_all (m_widget);
	}

	virtual GtkWidget *getWidget() { return m_widget; }

	virtual void updateType (Ypp::Package::Type type) {}
	virtual void updateList (Ypp::PkgList list) {}
	virtual bool begsUpdate() { return false; }

	virtual bool writeQuery (Ypp::PkgQuery::Query *query)
	{
		const gchar *name = gtk_entry_get_text (GTK_ENTRY (m_entry));
		if (*name) {
			int opt = 0;
			if (m_combo)
				opt = gtk_combo_box_get_active (GTK_COMBO_BOX (m_combo));
			bool whole_word = opt >= 2;
			query->addNames (name, ' ', opt == 0, opt == 0, opt == 1, opt == 2,
				opt == 3, whole_word);
			return true;
		}
		return false;
	}

	static void entry_changed_cb (GtkEditable *editable, SearchEntry *pThis)
	{
		pThis->notifyDelay();

		const gchar *name = gtk_entry_get_text (GTK_ENTRY (editable));
		bool icon = *name;
		if (pThis->m_clearIcon != icon) {
			gtk_entry_set_icon_from_stock (GTK_ENTRY (editable),
				GTK_ENTRY_ICON_SECONDARY, icon ? GTK_STOCK_CLEAR : NULL);
			gtk_entry_set_icon_activatable (GTK_ENTRY (editable),
				GTK_ENTRY_ICON_SECONDARY, icon);
			if (icon)
				gtk_entry_set_icon_tooltip_text (GTK_ENTRY (editable),
					GTK_ENTRY_ICON_SECONDARY, _("Clear"));
			pThis->m_clearIcon = icon;
		}
	}

	static void combo_changed_cb (GtkComboBox *combo, SearchEntry *pThis)
	{
		pThis->notify();
		gtk_widget_grab_focus (pThis->m_entry);
	}

	static void icon_press_cb (GtkEntry *entry, GtkEntryIconPosition pos,
	                            GdkEvent *event, SearchEntry *pThis)
	{
		gtk_entry_set_text (entry, "");
		gtk_widget_grab_focus (GTK_WIDGET (entry));
	}

	static void grab_focus_cb (GtkWidget *widget)
	{ gtk_widget_grab_focus (widget); }
};

static void selection_changed_cb (GtkTreeSelection *selection, _QueryWidget *pThis)
{
	if (gtk_tree_selection_get_selected (selection, NULL, NULL))
		pThis->notify();
}

static GtkWidget *tree_view_new()
{
	GtkWidget *view = gtk_tree_view_new();
	GtkTreeView *tview = GTK_TREE_VIEW (view);
	gtk_tree_view_set_headers_visible (tview, FALSE);
	gtk_tree_view_set_search_column (tview, 0);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (tview);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (
		NULL, renderer, "markup", 0, NULL);
	gtk_tree_view_append_column (tview, column);
	return view;
}

static GtkWidget *scrolled_window_new (GtkWidget *child)
{
	GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (scroll), child);
	return scroll;
}

static void list_store_set_text_count (GtkListStore *store, GtkTreeIter *iter,
	const char *text, int count)
{
	gchar *_text = g_strdup_printf ("%s <small>(%d)</small>", text, count);
	gtk_list_store_set (store, iter, 0, _text, 1, count > 0, -1);
	g_free (_text);
}

class CategoryView : public _QueryWidget
{
GtkWidget *m_widget, *m_view;
GtkTreeModel *m_model;
bool m_type2;

public:
	CategoryView() : _QueryWidget()
	{
		m_type2 = false;
		m_view = tree_view_new();
		GtkWidget *scroll = scrolled_window_new (m_view);
#if 0
		GtkWidget *combo = gtk_combo_box_new_text();
		gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Categories"));
		gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Repositories"));
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
		g_signal_connect (G_OBJECT (combo), "changed",
		                  G_CALLBACK (combo_changed_cb), this);
#endif
		GtkWidget *combo = gtk_label_new_with_mnemonic ("Ca_tegories:");
		gtk_misc_set_alignment (GTK_MISC (combo), 0, 0);
		gtk_label_set_mnemonic_widget (GTK_LABEL (combo), m_view);

		GtkWidget *vbox = gtk_vbox_new (FALSE, 4); //0);
		gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);
		m_widget = vbox;

		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (m_view));
		g_signal_connect (G_OBJECT (selection), "changed",
		                  G_CALLBACK (selection_changed_cb), this);
	}

	virtual GtkWidget *getWidget()
	{ return m_widget; }

	virtual void updateType (Ypp::Package::Type type)
	{
		m_type2 = type == Ypp::Package::PACKAGE_TYPE;

		GtkListStore *store;
		GtkTreeIter iter;
		store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_POINTER);
		m_model = GTK_TREE_MODEL (store);
		gtk_list_store_append (store, &iter);
		if (!dynamic_sidebar)
			gtk_list_store_set (store, &iter, 0, _("All packages"), -1);
		gtk_list_store_set (store, &iter, 1, TRUE, 2, NULL, -1);
		Ypp::Node *category;
		if (m_type2)
			category = Ypp::get()->getFirstCategory2 (type);
		else
			category = Ypp::get()->getFirstCategory (type);
		for (; category; category = category->next()) {
			gtk_list_store_append (store, &iter);
			if (!dynamic_sidebar)
				gtk_list_store_set (store, &iter, 0, category->name.c_str(), -1);
			gtk_list_store_set (store, &iter, 1, TRUE, 2, category, -1);
		}
		GtkTreeModel *filter_model = gtk_tree_model_filter_new (m_model, NULL);
		gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (filter_model), 1);
		gtk_tree_view_set_model (GTK_TREE_VIEW (m_view), filter_model);
		g_object_unref (G_OBJECT (store));
		g_object_unref (G_OBJECT (filter_model));

		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (m_view));
		g_signal_handlers_block_by_func (selection, (gpointer) selection_changed_cb, this);
		gtk_tree_model_get_iter_first (filter_model, &iter);
		gtk_tree_selection_select_iter (selection, &iter);
		g_signal_handlers_unblock_by_func (selection, (gpointer) selection_changed_cb, this);
	}

	virtual void updateList (Ypp::PkgList list)
	{
		GtkTreeModel *model = GTK_TREE_MODEL (m_model);
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter_first (model, &iter))
			do {
				Ypp::Node *category;
				gtk_tree_model_get (model, &iter, 2, &category, -1);

				GtkListStore *store = GTK_LIST_STORE (model);
				if (category) {
					int categoriesNb = 0;
					for (int i = 0; i < list.size(); i++) {
						Ypp::Package *pkg = list.get (i);
						Ypp::Node *pkg_category = m_type2 ? pkg->category2() : pkg->category();
						if (pkg_category == category)
							categoriesNb++;
					}
					list_store_set_text_count (store, &iter, category->name.c_str(), categoriesNb);
				}
				else
					list_store_set_text_count (store, &iter, _("All Categories"), list.size());
			} while (gtk_tree_model_iter_next (model, &iter));
	}

	virtual bool begsUpdate() { return dynamic_sidebar; }

	virtual bool writeQuery (Ypp::PkgQuery::Query *query)
	{
		GtkTreeModel *model;
		GtkTreeIter iter;
		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (m_view));
		if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
			Ypp::Node *category;
			gtk_tree_model_get (model, &iter, 2, &category, -1);
			if (category) {
				if (m_type2)
					query->addCategory2 (category);
				else
					query->addCategory (category);
				return true;
			}
		}
		return false;
	}

	static void combo_changed_cb (GtkComboBox *combo, CategoryView *pThis)
	{
	}
};

class StatusView : public _QueryWidget
{
GtkWidget *m_widget, *m_view;
GtkTreeModel *m_model;

public:
	StatusView() : _QueryWidget()
	{
		m_view = tree_view_new();

		GtkWidget *label = gtk_label_new_with_mnemonic ("_Statuses:");
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
		gtk_label_set_mnemonic_widget (GTK_LABEL (label), m_view);
		GtkWidget *scroll = scrolled_window_new (m_view);
		m_widget = gtk_vbox_new (FALSE, 4);
		gtk_box_pack_start (GTK_BOX (m_widget), label, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_widget), scroll, TRUE, TRUE, 0);

		GtkListStore *store;
		GtkTreeIter iter;
		store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_INT);
		m_model = GTK_TREE_MODEL (store);
		gtk_list_store_append (store, &iter);
		if (!dynamic_sidebar)
			gtk_list_store_set (store, &iter, 0, _("All packages"), -1);
		gtk_list_store_set (store, &iter, 1, TRUE, 2, 0, -1);
		gtk_list_store_append (store, &iter);
		if (!dynamic_sidebar)
			gtk_list_store_set (store, &iter, 0, _("Available"), -1);
		gtk_list_store_set (store, &iter, 1, TRUE, 2, 1, -1);
		gtk_list_store_append (store, &iter);
		if (!dynamic_sidebar)
			gtk_list_store_set (store, &iter, 0, _("Installed"), -1);
		gtk_list_store_set (store, &iter, 1, TRUE, 2, 2, -1);
		gtk_list_store_append (store, &iter);
		if (!dynamic_sidebar)
			gtk_list_store_set (store, &iter, 0, _("Upgrades"), -1);
		gtk_list_store_set (store, &iter, 1, TRUE, 2, 3, -1);
		gtk_list_store_append (store, &iter);
		if (!dynamic_sidebar)
			gtk_list_store_set (store, &iter, 0, _("Summary"), -1);
		gtk_list_store_set (store, &iter, 1, TRUE, 2, 4, -1);

		GtkTreeModel *filter_model = gtk_tree_model_filter_new (m_model, NULL);
		gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (filter_model), 1);
		gtk_tree_view_set_model (GTK_TREE_VIEW (m_view), filter_model);
		g_object_unref (G_OBJECT (store));
		g_object_unref (G_OBJECT (filter_model));

		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (m_view));
		gtk_tree_model_get_iter_first (filter_model, &iter);
		gtk_tree_selection_select_iter (selection, &iter);
		g_signal_connect (G_OBJECT (selection), "changed",
		                  G_CALLBACK (selection_changed_cb), this);
	}

	virtual GtkWidget *getWidget()
	{ return m_widget; }

	virtual void updateType (Ypp::Package::Type type) {}

	virtual void updateList (Ypp::PkgList list)
	{
		int installedNb = 0, upgradableNb = 0, notInstalledNb = 0, modifiedNb = 0;
		for (int i = 0; i < list.size(); i++) {
			Ypp::Package *pkg = list.get (i);
			if (pkg->isInstalled()) {
				installedNb++;
				if (pkg->hasUpgrade())
					upgradableNb++;
			}
			else
				notInstalledNb++;
			if (pkg->toModify())
				modifiedNb++;
		}

		GtkTreeModel *model = m_model;
		GtkTreeIter iter;
		GtkListStore *store = GTK_LIST_STORE (model);
		gtk_tree_model_iter_nth_child (model, &iter, NULL, 0);
		list_store_set_text_count (store, &iter, _("Any Status"), list.size());
		gtk_tree_model_iter_nth_child (model, &iter, NULL, 1);
		list_store_set_text_count (store, &iter, _("Not Installed"), notInstalledNb);
		gtk_tree_model_iter_nth_child (model, &iter, NULL, 2);
		list_store_set_text_count (store, &iter, _("Installed"), installedNb);
		gtk_tree_model_iter_nth_child (model, &iter, NULL, 3);
		list_store_set_text_count (store, &iter, _("Upgradable"), upgradableNb);
		gtk_tree_model_iter_nth_child (model, &iter, NULL, 4);
		list_store_set_text_count (store, &iter, _("Modified"), modifiedNb);
	}

	virtual bool begsUpdate() { return dynamic_sidebar; }

	virtual bool writeQuery (Ypp::PkgQuery::Query *query)
	{
		GtkTreeModel *model;
		GtkTreeIter iter;
		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (m_view));
		if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
			int status;
			gtk_tree_model_get (model, &iter, 2, &status, -1);
			switch (status) {
				case 0: default: break;
				case 1: query->setIsInstalled (false); break;
				case 2: query->setIsInstalled (true); break;
				case 3: query->setHasUpgrade (true); break;
				case 4: query->setToModify (true); break;
			}
			if (status > 0 && status <= 4)
				return true;
		}
		return false;
	}
};

class DetailBox
{
GtkWidget *m_widget, *m_text, *m_pkg_view;

public:
	GtkWidget *getWidget() { return m_widget; }

	DetailBox()
	{
		m_text = ygtk_rich_text_new();
		YGtkPackageView *view = ygtk_package_view_new (TRUE);
		m_pkg_view = GTK_WIDGET (view);
		//gtk_widget_set_size_request (m_pkg_view, -1, 200);

		GtkWidget *vbox = gtk_vbox_new (FALSE, 18);
		gtk_box_pack_start (GTK_BOX (vbox), m_text, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), m_pkg_view, TRUE, TRUE, 0);

		m_widget = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (m_widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (m_widget), vbox);

		g_signal_connect (G_OBJECT (vbox), "expose-event",
		                  G_CALLBACK (expose_cb), NULL);
	}

	void setPackage (Ypp::Package *package)
	{
		if (package) {
			std::string text = package->description (HTML_MARKUP);
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (m_text), text.c_str());
			gtk_widget_show (m_widget);
		}
		else
			ygtk_rich_text_set_plain_text (YGTK_RICH_TEXT (m_text), "");

		if (package && package->type() != Ypp::Package::PACKAGE_TYPE) {
			Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
			query->addCollection (package);
			Ypp::PkgQuery list (Ypp::Package::PACKAGE_TYPE, query);
			((YGtkPackageView *) m_pkg_view)->setList (list, NULL);
			gtk_widget_show (m_pkg_view);
		}
		else
			gtk_widget_hide (m_pkg_view);
	}

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

class Toolbar
{
GtkWidget *m_toolbar;
GtkWidget *m_install_button, *m_upgrade_button, *m_remove_button;
Ypp::PkgList m_packages;

public:
	GtkWidget *getWidget() { return m_toolbar; }

	Toolbar (bool yast_style)
	{
		if (yast_style) {
			m_toolbar = gtk_hbox_new (FALSE, 6);
			m_install_button = gtk_button_new_with_label (_("Install"));
			gtk_button_set_image (GTK_BUTTON (m_install_button),
				gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON));
			gtk_box_pack_start (GTK_BOX (m_toolbar), m_install_button, FALSE, TRUE, 0);
			m_upgrade_button = gtk_button_new_with_label (_("Upgrade"));
			gtk_button_set_image (GTK_BUTTON (m_upgrade_button),
				gtk_image_new_from_stock (GTK_STOCK_GO_UP, GTK_ICON_SIZE_BUTTON));
			gtk_box_pack_start (GTK_BOX (m_toolbar), m_upgrade_button, FALSE, TRUE, 0);
			m_remove_button = gtk_button_new_with_label (_("Remove"));
			gtk_button_set_image (GTK_BUTTON (m_remove_button),
				gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON));
			gtk_box_pack_start (GTK_BOX (m_toolbar), m_remove_button, FALSE, TRUE, 0);
		}
		else {
			m_toolbar = gtk_toolbar_new();
			m_install_button = GTK_WIDGET (gtk_tool_button_new (
				gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_LARGE_TOOLBAR), _("Install")));
			gtk_tool_item_set_is_important (GTK_TOOL_ITEM (m_install_button), TRUE);
			gtk_toolbar_insert (GTK_TOOLBAR (m_toolbar), GTK_TOOL_ITEM (m_install_button), -1);
			m_upgrade_button = GTK_WIDGET (gtk_tool_button_new (
				gtk_image_new_from_stock (GTK_STOCK_GO_UP, GTK_ICON_SIZE_LARGE_TOOLBAR), _("Upgrade")));
			gtk_tool_item_set_is_important (GTK_TOOL_ITEM (m_upgrade_button), TRUE);
			gtk_toolbar_insert (GTK_TOOLBAR (m_toolbar), GTK_TOOL_ITEM (m_upgrade_button), -1);
			m_remove_button = GTK_WIDGET (gtk_tool_button_new (
				gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_LARGE_TOOLBAR), _("Remove")));
			gtk_tool_item_set_is_important (GTK_TOOL_ITEM (m_remove_button), TRUE);
			gtk_toolbar_insert (GTK_TOOLBAR (m_toolbar), GTK_TOOL_ITEM (m_remove_button), -1);
		}

		g_signal_connect (G_OBJECT (m_install_button), "clicked",
			              G_CALLBACK (install_clicked_cb), this);
		g_signal_connect (G_OBJECT (m_upgrade_button), "clicked",
			              G_CALLBACK (upgrade_clicked_cb), this);
		g_signal_connect (G_OBJECT (m_remove_button), "clicked",
			              G_CALLBACK (remove_clicked_cb), this);

		setPackages (Ypp::PkgList());
	}

	void setPackages (Ypp::PkgList list)
	{
		m_packages = list;
		if (list.size() == 0) {
			gtk_widget_set_sensitive (m_install_button, FALSE);
			gtk_widget_set_sensitive (m_upgrade_button, FALSE);
			gtk_widget_set_sensitive (m_remove_button, FALSE);
		}
		else {
			bool installed = list.installed();
			gtk_widget_set_sensitive (m_install_button, !installed);
			gtk_widget_set_sensitive (m_remove_button, installed);
			gtk_widget_set_sensitive (m_upgrade_button, list.upgradable());
		}
	}

	static void install_clicked_cb (GtkWidget *button, Toolbar *pThis)
	{
		pThis->m_packages.install();
	}

	static void upgrade_clicked_cb (GtkWidget *button, Toolbar *pThis)
	{
		pThis->m_packages.install();
	}

	static void remove_clicked_cb (GtkWidget *button, Toolbar *pThis)
	{
		pThis->m_packages.remove();
	}
};

class UI : public YGtkPackageView::Listener, _QueryListener, Ypp::Disk::Listener
{
GtkWidget *m_widget, *m_disk_label;
YGtkPackageView *m_all_view, *m_installed_view, *m_available_view, *m_upgrades_view;
std::list <_QueryWidget *> m_query;
DetailBox *m_details;
DiskView *m_disk;
Ypp::Package::Type m_type;
Toolbar *m_toolbar;

public:
	GtkWidget *getWidget() { return m_widget; }

	UI (YPackageSelector *sel)
	{
		m_toolbar = 0;

		if (sel->onlineUpdateMode())
			m_type = Ypp::Package::PATCH_TYPE;
		else if (!sel->searchMode())
			m_type = Ypp::Package::PATTERN_TYPE;
		else
			m_type = Ypp::Package::PACKAGE_TYPE;

		m_all_view = m_installed_view = m_available_view = m_upgrades_view = NULL;

		_QueryWidget *search_entry = 0;
		if (search_entry_top)
			search_entry = new SearchEntry (false);
		m_query.push_back (search_entry);
		gtk_widget_set_size_request (search_entry->getWidget(), 160, -1);

		UndoView *undo_view = 0;
		if (undo_side || undo_tab)
			undo_view = new UndoView();

		GtkWidget *packages_box;
		if (status_tabs) {
			m_installed_view = ygtk_package_view_new (TRUE);
			m_installed_view->setListener (this);
			m_available_view = ygtk_package_view_new (TRUE);
			m_available_view->setListener (this);
			m_upgrades_view = ygtk_package_view_new (TRUE);
			m_upgrades_view->setListener (this);

			packages_box = ygtk_notebook_new();
			const char **labels;
			if (status_tabs_as_actions) {
				const char *t[] = { _("_Install"), _("_Upgrade"), _("_Remove"), _("Undo") };
				labels = t;
			}
			else {
				const char *t[] = { _("_Available"), _("_Upgrades"), _("_Installed"), _("Summary") };
				labels = t;
			}
			gtk_notebook_append_page (GTK_NOTEBOOK (packages_box),
				GTK_WIDGET (m_available_view), gtk_label_new_with_mnemonic (labels[0]));
			gtk_notebook_append_page (GTK_NOTEBOOK (packages_box),
				GTK_WIDGET (m_upgrades_view), gtk_label_new_with_mnemonic (labels[1]));
			gtk_notebook_append_page (GTK_NOTEBOOK (packages_box),
				GTK_WIDGET (m_installed_view), gtk_label_new_with_mnemonic (labels[2]));

			if (undo_tab) {
				GtkWidget *box = gtk_event_box_new();
				gtk_container_add (GTK_CONTAINER (box), undo_view->createView (this, true));
				gtk_container_set_border_width (GTK_CONTAINER (box), 6);
				gtk_notebook_append_page (GTK_NOTEBOOK (packages_box),
					box, gtk_label_new_with_mnemonic (labels[3]));
			}

			if (search_entry)
				// FIXME: only the entry itself is shown, without the "Find:" label
				ygtk_notebook_set_corner_widget (
					YGTK_NOTEBOOK (packages_box), search_entry->getWidget());
		}
		else {
			m_all_view = ygtk_package_view_new (TRUE);
			m_all_view->setListener (this);
			gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (m_all_view), GTK_SHADOW_IN);

			packages_box = gtk_vbox_new (FALSE, 4);
			GtkWidget *header_box = gtk_hbox_new (FALSE, 6);
			GtkWidget *header = gtk_label_new_with_mnemonic (_("_Listing:"));
			gtk_misc_set_alignment (GTK_MISC (header), 0, .5);
			gtk_label_set_mnemonic_widget (GTK_LABEL (header), GTK_WIDGET (m_all_view));
			gtk_box_pack_start (GTK_BOX (header_box), header, TRUE, TRUE, 0);
			if (search_entry)
				gtk_box_pack_start (GTK_BOX (header_box), search_entry->getWidget(), FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (packages_box), header_box, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (packages_box), GTK_WIDGET (m_all_view), TRUE, TRUE, 0);
		}

		GtkWidget *packages_button_box = gtk_vbox_new (FALSE, 4);
		gtk_box_pack_start (GTK_BOX (packages_button_box), packages_box, TRUE, TRUE, 0);
		if (!toolbar_top && toolbar_yast)
			gtk_box_pack_start (GTK_BOX (packages_button_box), (m_toolbar = new Toolbar (true))->getWidget(), FALSE, TRUE, 0);

		GtkWidget *packages_pane = gtk_hpaned_new();
		gtk_paned_pack1 (GTK_PANED (packages_pane), packages_button_box, TRUE, FALSE);
		GtkWidget *undo_widget;
		if (undo_side) {
			if (undo_old_style)
				undo_widget = (new ChangesPane())->getWidget();
			else
				undo_widget = undo_view->createView (this, false);
			gtk_paned_pack2 (GTK_PANED (packages_pane), undo_widget, FALSE, FALSE);
			gtk_paned_set_position (GTK_PANED (packages_pane), 350);
		}

		GtkWidget *view_pane = gtk_vpaned_new();
		gtk_paned_pack1 (GTK_PANED (view_pane), packages_pane, TRUE, FALSE);
		m_details = new DetailBox();
		gtk_paned_pack2 (GTK_PANED (view_pane), m_details->getWidget(), FALSE, TRUE);
		gtk_paned_set_position (GTK_PANED (view_pane), 500);

#if 0
		GtkWidget *combo = gtk_combo_box_new_text();
		if (sel->onlineUpdateMode()) {
			gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Patch"));
			gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
		}
		else {
			gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Package"));
			gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Pattern"));
			gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Language"));
			int i = sel->searchMode() ? 0 : 1;
			gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
		}
		g_signal_connect (G_OBJECT (combo), "changed",
		                  G_CALLBACK (type_changed_cb), this);

		m_disk = new DiskView();
		m_disk_label = gtk_label_new ("");

		GtkWidget *type_hbox = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (type_hbox), m_disk_label, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (type_hbox), m_disk->getWidget(), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (type_hbox), gtk_event_box_new(), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (type_hbox), gtk_label_new (_("Type:")), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (type_hbox), combo, FALSE, TRUE, 0);

		GtkWidget *view_vbox = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (view_vbox), view_pane, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (view_vbox), type_hbox, FALSE, TRUE, 0);
#endif
		GtkWidget *side_vbox = gtk_vbox_new (FALSE, 12);
		if (search_entry_side) {
			_QueryWidget *search_entry = new SearchEntry (true);
			m_query.push_back (search_entry);
			gtk_box_pack_start (GTK_BOX (side_vbox), search_entry->getWidget(), FALSE, TRUE, 0);
		}

		_QueryWidget *categories = new CategoryView();
		m_query.push_back (categories);

		GtkWidget *cat_pane = gtk_vpaned_new();
		gtk_paned_pack1 (GTK_PANED (cat_pane), categories->getWidget(), TRUE, FALSE);
		if (status_side) {
			_QueryWidget *statuses = new StatusView();
			m_query.push_back (statuses);
			gtk_paned_pack2 (GTK_PANED (cat_pane), statuses->getWidget(), TRUE, FALSE);
		}
		gtk_box_pack_start (GTK_BOX (side_vbox), cat_pane, TRUE, TRUE, 0);

		GtkWidget *side_pane = gtk_hpaned_new();
		gtk_paned_pack1 (GTK_PANED (side_pane), side_vbox, FALSE, TRUE);
		gtk_paned_pack2 (GTK_PANED (side_pane), view_pane, TRUE, FALSE);
		gtk_paned_set_position (GTK_PANED (side_pane), 150);

		for (std::list <_QueryWidget *>::const_iterator it = m_query.begin();
		     it != m_query.end(); it++) {
			(*it)->updateType (m_type);
			(*it)->setListener (this);
		}
		refresh();
		updateDisk();
		Ypp::get()->getDisk()->addListener (this);

		if (toolbar_top) {
			m_toolbar = new Toolbar (false);
			m_widget = gtk_vbox_new (FALSE, 6);
			gtk_box_pack_start (GTK_BOX (m_widget), m_toolbar->getWidget(), FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (m_widget), side_pane, TRUE, TRUE, 0);
		}
		else
			m_widget = side_pane;

		gtk_widget_show_all (m_widget);

		if (details_start_hide) {
			gtk_widget_hide (m_details->getWidget());
			if (undo_old_style)
				gtk_widget_hide (undo_widget);
		}
		else
			m_details->setPackage (NULL);
	}

	~UI()
	{
		for (std::list <_QueryWidget *>::const_iterator it = m_query.begin();
		     it != m_query.end(); it++)
			delete *it;
		delete m_disk;
	}

private:
	void updateWidget (_QueryWidget *widget)
	{
		Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
		for (std::list <_QueryWidget *>::const_iterator it = m_query.begin();
		     it != m_query.end(); it++) {
			if (*it == widget)
				continue;
			(*it)->writeQuery (query);
		}

		Ypp::PkgQuery list (m_type, query);
		widget->updateList (list);
	}

	virtual void refresh()
	{
		Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
		for (std::list <_QueryWidget *>::const_iterator it = m_query.begin();
		     it != m_query.end(); it++)
			(*it)->modified = (*it)->writeQuery (query);

		Ypp::PkgQuery list (m_type, query);
		if (m_all_view)
			m_all_view->setList (list, NULL);

		if (m_installed_view) {
			Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
			query->setIsInstalled (true);
			Ypp::PkgQuery _list (list, query);
			m_installed_view->setList (_list, NULL);
		}
		if (m_available_view) {
			Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
			query->setIsInstalled (false);
			Ypp::PkgQuery _list (list, query);
			m_available_view->setList (_list, NULL);
		}
		if (m_upgrades_view) {
			Ypp::PkgQuery::Query *query = new Ypp::PkgQuery::Query();
			query->setHasUpgrade (true);
			Ypp::PkgQuery _list (list, query);
			m_upgrades_view->setList (_list, NULL);
		}

		for (std::list <_QueryWidget *>::const_iterator it = m_query.begin();
		     it != m_query.end(); it++)
			if ((*it)->begsUpdate()) {
				if ((*it)->modified)
					updateWidget (*it);
				else
					(*it)->updateList (list);
			}
	}

	virtual void packagesSelected (Ypp::PkgList packages)
	{
		m_details->setPackage (packages.size() ? packages.get (0) : NULL);
		m_toolbar->setPackages (packages);
	}

	virtual void updateDisk()
	{
#if 0
		Ypp::Disk *disk = Ypp::get()->getDisk();
		const Ypp::Disk::Partition *part = 0;
		for (int i = 0; disk->getPartition (i); i++) {
			const Ypp::Disk::Partition *p = disk->getPartition (i);
			if (p->path == "/usr" || p->path == "/usr/") {
				part = p;
				break;
			}
			if (p->path == "/")
				part = p;
		}
		if (part) {
			gchar *text;
			if (part->delta)
				text = g_strdup_printf ("Free disk space: %s (%s%s)", part->free_str.c_str(),
					part->delta > 0 ? "+" : "", part->delta_str.c_str());
			else
				text = g_strdup_printf ("Free disk space: %s", part->free_str.c_str());
			gtk_label_set_text (GTK_LABEL (m_disk_label), text);
			g_free (text);
		}
		else
			gtk_label_set_text (GTK_LABEL (m_disk_label), "");
#endif
	}

	static void type_changed_cb (GtkComboBox *combo, UI *pThis)
	{
		pThis->m_type = (Ypp::Package::Type) gtk_combo_box_get_active (combo);
		for (std::list <_QueryWidget *>::const_iterator it = pThis->m_query.begin();
		     it != pThis->m_query.end(); it++)
			(*it)->updateType (pThis->m_type);
		pThis->refresh();
	}
};

#include "pkg-selector-help.h"

class YGPackageSelector : public YPackageSelector, public YGWidget, public Ypp::Interface
{
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
		int width, height;
		YGUI::ui()->pkgSelectorSize (&width, &height);
		dialog->setMinSize (width, height);

		YGtkWizard *wizard = YGTK_WIZARD (getWidget());
		ygtk_wizard_set_header_icon (wizard,
			THEMEDIR "/icons/22x22/apps/yast-software.png");
		const char *title = onlineUpdate ? _("Online Update") : _("Software Manager");
		ygtk_wizard_set_header_text (wizard, title);
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

#if 0
		m_tools = new ToolsBox();
		ygtk_wizard_set_extra_button (YGTK_WIZARD (getWidget()), m_tools->getWidget());
#endif

		m_progressbar = gtk_progress_bar_new();
		GtkWidget *empty = gtk_event_box_new();
		gtk_widget_set_size_request (empty, -1, 36);
		gtk_widget_show (empty);

		GtkWidget *vbox = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_end (GTK_BOX (vbox), empty, FALSE, TRUE, 0);
		gtk_box_pack_end (GTK_BOX (vbox), m_progressbar, FALSE, TRUE, 0);
		gtk_widget_show (vbox);
		ygtk_wizard_set_child (YGTK_WIZARD (wizard), vbox);
		Ypp::get()->setInterface (this);

/*		m_notebook = new QueryNotebook (onlineUpdate, repoMgrEnabled());
		gtk_box_pack_start (GTK_BOX (vbox), m_notebook->getWidget(), TRUE, TRUE, 0);
*/
		UI *ui = new UI (this);
		gtk_box_pack_start (GTK_BOX (vbox), ui->getWidget(), TRUE, TRUE, 0);
		gtk_widget_hide (empty);

		const char **help = onlineUpdate ? patch_help : pkg_help;
		std::string str;
		str.reserve (6144);
		for (int i = 0; help[i]; i++)
			str.append (help [i]);
		ygtk_wizard_set_help_text (wizard, str.c_str());
		dialog->setTitle (title);
		m_button_timeout_id = g_timeout_add (7500, wizard_enable_button_cb, this);
	}

	virtual ~YGPackageSelector()
	{
		if (m_button_timeout_id)
			g_source_remove (m_button_timeout_id);
//		delete m_notebook;
#if 0
		delete m_tools;
#endif
		Ypp::finish();
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

    static bool confirm_cb (void *pThis)
    { return confirmCancel(); }

	static void wizard_action_cb (YGtkWizard *wizard, gpointer id,
	                              gint id_type, YGPackageSelector *pThis)
	{
		const gchar *action = (gchar *) id;
		yuiMilestone() << "Closing PackageSelector with '" << action << "'\n";
		if (!strcmp (action, "accept")) {
			if (!pThis->onlineUpdateMode()) {
				if (pThis->confirmUnsupported())
					if (!askConfirmUnsupported())
						return;
			}
			if (confirmApply())
				YGUI::ui()->sendEvent (new YMenuEvent ("accept"));
		}
		else if (!strcmp (action, "cancel"))
			if (confirmCancel())
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
			  "repository");
	}

	virtual void loading (float progress)
	{
		if (progress == 1) {
			if (GTK_WIDGET_VISIBLE (m_progressbar)) {
				gtk_widget_hide (m_progressbar);
				while (g_main_context_iteration (NULL, FALSE)) ;
				//gtk_widget_set_sensitive (getWidget(), TRUE);
			}
			YGUI::ui()->normalCursor();
		}
		else {
			if (progress == 0)
				YGUI::ui()->busyCursor();
			else {  // progress=0 may be to trigger cursor only
				//gtk_widget_set_sensitive (getWidget(), FALSE);
				gtk_widget_show (m_progressbar);
				gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (m_progressbar), progress);
			}
			while (g_main_context_iteration (NULL, FALSE)) ;
		}
	}

	YGWIDGET_IMPL_COMMON (YPackageSelector)
};

#include "pkg/YGPackageSelectorPluginImpl.h"

YPackageSelector *
YGPackageSelectorPluginImpl::createPackageSelector (YWidget *parent, long modeFlags)
{
	modeFlags |= YPkg_SearchMode;
	return new YGPackageSelector (parent, modeFlags);
}

YWidget *
YGPackageSelectorPluginImpl::createPatternSelector (YWidget *parent, long modeFlags)
{
	modeFlags &= YPkg_SearchMode;
	return new YGPackageSelector (parent, modeFlags);
}

YWidget *
YGPackageSelectorPluginImpl::createSimplePatchSelector (YWidget *parent, long modeFlags)
{
	modeFlags |= YPkg_OnlineUpdateMode;
	return new YGPackageSelector (parent, modeFlags);
}

