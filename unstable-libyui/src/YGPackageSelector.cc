/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>

#include "YGUI.h"
#include "YGUtils.h"
#include "YGi18n.h"
#include "YGDialog.h"

#if 1
#include "ygtkwizard.h"
#include "ygtkfindentry.h"
#include "ygtkhtmlwrap.h"
#include "ygtkzyppwrapper.h"

// utilities
static GtkWidget *createImageFromXPM (const char **xpm)
{
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_xpm_data (xpm);
	GtkWidget *image = gtk_image_new_from_pixbuf (pixbuf);
	gtk_widget_show (image);
	g_object_unref (pixbuf);
	return image;
}

#define FILEMANAGER_EXEC "/usr/bin/nautilus"
inline void FILEMANAGER_LAUNCH (const char *path)
{ system ((std::string (FILEMANAGER_EXEC) + " -n --no-desktop " + path + " &").c_str()); }

static void busyCursor()
{
	YGUI::ui()->busyCursor();
	// so that the cursor is actually set...
	while (g_main_context_iteration (NULL, FALSE)) ;
}
static void normalCursor()
{ YGUI::ui()->normalCursor(); }

#include "icons/pkg-list-mode.xpm"
#include "icons/pkg-tiles-mode.xpm"

class PackagesView
{
public:
	struct Listener {
		virtual void packagesSelected (const std::list <Ypp::Package *> &selection) = 0;
	};
	void setListener (Listener *listener)
	{ m_listener = listener; }

private:
Listener *m_listener;

	void packagesSelected (const std::list <Ypp::Package *> &selection)
	{
		if (m_listener) {
			busyCursor();
			m_listener->packagesSelected (selection);
			normalCursor();
		}
	}

	struct View {
		PackagesView *m_parent;
		GtkWidget *m_widget;
		View (PackagesView *parent) : m_parent (parent)
		{}
		virtual void setModel (GtkTreeModel *model) = 0;

		void selectedPaths (GtkTreeModel *model, GList *paths)
		{
			std::list <Ypp::Package *> packages;
			for (GList *i = paths; i; i = i->next) {
				Ypp::Package *package;
				GtkTreePath *path = (GtkTreePath *) i->data;
				GtkTreeIter iter;
				gtk_tree_model_get_iter (model, &iter, path);
				gtk_tree_model_get (model, &iter, YGtkZyppModel::PTR_COLUMN, &package, -1);
				gtk_tree_path_free (path);

				packages.push_back (package);
			}
			g_list_free (paths);
			m_parent->packagesSelected (packages);
		}
	};
	struct ListView : public View
	{
		ListView (PackagesView *parent)
		: View (parent)
		{
			GtkTreeView *view = GTK_TREE_VIEW (m_widget = gtk_tree_view_new());
			gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
			gtk_tree_view_set_search_column (GTK_TREE_VIEW (view), 1);
			GtkTreeViewColumn *column;
			GtkCellRenderer *renderer;
			renderer = gtk_cell_renderer_pixbuf_new();
			column = gtk_tree_view_column_new_with_attributes ("", renderer,
				"pixbuf", YGtkZyppModel::ICON_COLUMN, NULL);
			gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
			renderer = gtk_cell_renderer_text_new();
			g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
			column = gtk_tree_view_column_new_with_attributes ("", renderer,
				"markup", YGtkZyppModel::NAME_DESCRIPTION_COLUMN, NULL);
			gtk_tree_view_column_set_expand (column, TRUE);
			gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
			renderer = gtk_cell_renderer_pixbuf_new();
			column = gtk_tree_view_column_new_with_attributes ("", renderer,
				"pixbuf", YGtkZyppModel::SPECIAL_ICON_COLUMN, NULL);
			gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

			GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
			gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
			g_signal_connect (G_OBJECT (selection), "changed",
				                    G_CALLBACK (packages_selected_cb), this);
			gtk_widget_show (m_widget);
		}

		virtual void setModel (GtkTreeModel *model)
		{
			gtk_tree_view_set_model (GTK_TREE_VIEW (m_widget), model);
			if (GTK_WIDGET_REALIZED (m_widget))
				gtk_tree_view_scroll_to_point (GTK_TREE_VIEW (m_widget), 0, 0);
		}

		static void packages_selected_cb (GtkTreeSelection *selection, View *pThis)
		{
			GtkTreeModel *model;
			GList *paths = gtk_tree_selection_get_selected_rows (selection, &model);
			pThis->selectedPaths (model, paths);
		}
	};
	struct IconView : public View
	{
		IconView (PackagesView *parent)
		: View (parent)
		{
			GtkIconView *view = GTK_ICON_VIEW (m_widget = gtk_icon_view_new());
			gtk_icon_view_set_text_column (view, YGtkZyppModel::NAME_COLUMN);
			gtk_icon_view_set_pixbuf_column (view, YGtkZyppModel::ICON_COLUMN);
			gtk_icon_view_set_selection_mode (view, GTK_SELECTION_MULTIPLE);
			g_signal_connect (G_OBJECT (m_widget), "selection-changed",
				                    G_CALLBACK (packages_selected_cb), this);
			gtk_widget_show (m_widget);
		}

		virtual void setModel (GtkTreeModel *model)
		{
			gtk_icon_view_set_model (GTK_ICON_VIEW (m_widget), model);
			if (GTK_WIDGET_REALIZED (m_widget)) {
				GtkTreePath *path = gtk_tree_path_new_first();
				gtk_icon_view_scroll_to_path (GTK_ICON_VIEW (m_widget),
				                              path, FALSE, 0, 0);
				gtk_tree_path_free (path);
			}
		}

		static void packages_selected_cb (GtkIconView *view, View *pThis)
		{
			GtkTreeModel *model = gtk_icon_view_get_model (view);
			GList *paths = gtk_icon_view_get_selected_items (view);
			pThis->selectedPaths (model, paths);
		}
	};

GtkWidget *m_box, *m_bin, *m_list_button, *m_icon_button;
GtkTreeModel *m_model;
View *m_view;

public:
	GtkWidget *getWidget()
	{ return m_box; }

	PackagesView() : m_listener (NULL), m_model (NULL), m_view (NULL)
	{
		m_bin = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (m_bin),
		                                     GTK_SHADOW_IN);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (m_bin),
		                                GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

		GtkWidget *buttons = gtk_vbox_new (FALSE, 0), *image;
		image = createImageFromXPM (pkg_list_mode_xpm);
		m_list_button = gtk_toggle_button_new();
		gtk_button_set_image (GTK_BUTTON (m_list_button), image);
		image = createImageFromXPM (pkg_tiles_mode_xpm);
		m_icon_button = gtk_toggle_button_new();
		gtk_button_set_image (GTK_BUTTON (m_icon_button), image);
		gtk_box_pack_start (GTK_BOX (buttons), gtk_label_new(""), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (buttons), m_list_button, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (buttons), m_icon_button, FALSE, FALSE, 0);
		g_signal_connect (G_OBJECT (m_list_button), "toggled",
		                  G_CALLBACK (mode_toggled_cb), this);
		g_signal_connect (G_OBJECT (m_icon_button), "toggled",
		                  G_CALLBACK (mode_toggled_cb), this);

		m_box = gtk_hbox_new (FALSE, 2);
		gtk_box_pack_start (GTK_BOX (m_box), m_bin, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_box), buttons, FALSE, TRUE, 0);

		setMode (LIST_MODE);
	}

	~PackagesView()
	{
		if (m_model)
			g_object_unref (G_OBJECT (m_model));
		delete m_view;
	}

	enum ViewMode {
		LIST_MODE, ICON_MODE
	};
	void setMode (ViewMode mode)
	{
		if (GTK_WIDGET_REALIZED (m_bin))
			busyCursor();
		g_signal_handlers_block_by_func (m_list_button, (gpointer) mode_toggled_cb, this);
		g_signal_handlers_block_by_func (m_icon_button, (gpointer) mode_toggled_cb, this);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (m_list_button), mode == LIST_MODE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (m_icon_button), mode == ICON_MODE);

		g_signal_handlers_unblock_by_func (m_list_button, (gpointer) mode_toggled_cb, this);
		g_signal_handlers_unblock_by_func (m_icon_button, (gpointer) mode_toggled_cb, this);

		if (m_view)
			gtk_container_remove (GTK_CONTAINER (m_bin), m_view->m_widget);
		delete m_view;
		if (mode == LIST_MODE)
			m_view = new ListView (this);
		else
			m_view = new IconView (this);
fprintf (stderr, "adding new view to container\n");
		gtk_container_add (GTK_CONTAINER (m_bin), m_view->m_widget);
		if (m_model)
			m_view->setModel (m_model);
		packagesSelected (std::list <Ypp::Package *> ());
		normalCursor();
	}

	void query (Ypp::Query *query)
	{
		if (GTK_WIDGET_REALIZED (m_bin))
			busyCursor();

		if (m_model)
			g_object_unref (G_OBJECT (m_model));

		YGtkZyppModel *zmodel = ygtk_zypp_model_new (query);
		m_model = GTK_TREE_MODEL (zmodel);
		if (m_view) {
			m_view->setModel (m_model);
			packagesSelected (std::list <Ypp::Package *> ());
		}
		normalCursor();
	}

private:
	static void mode_toggled_cb (GtkToggleButton *toggle, PackagesView *pThis)
	{
		bool active = gtk_toggle_button_get_active (toggle);
		if (!active) {
			// don't let the button be un-pressed
			g_signal_handlers_block_by_func (toggle, (gpointer) mode_toggled_cb, pThis);
			gtk_toggle_button_set_active (toggle, TRUE);
			g_signal_handlers_unblock_by_func (toggle, (gpointer) mode_toggled_cb, pThis);
			return;
		}
		ViewMode mode = GTK_WIDGET (toggle) == pThis->m_list_button ? LIST_MODE : ICON_MODE;
		pThis->setMode (mode);
	}
};

// TEMP: a window of modified packages
class TrashWindow : PackagesView::Listener
{
GtkWidget *m_window, *m_undo_button;
PackagesView *m_view;

public:
	TrashWindow()
	{
		m_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title (GTK_WINDOW (m_window), "Changes");
		gtk_window_set_default_size (GTK_WINDOW (m_window), 200, 250);

		m_view = new PackagesView();
		Ypp::Query *query = new Ypp::Query (Ypp::Package::PACKAGE_TYPE);
		query->setIsModified (true);
		m_view->query (query);
		m_view->setListener (this);

		GtkWidget *vbox = gtk_vbox_new (FALSE, 6), *undo_align;
		m_undo_button = gtk_button_new_from_stock (GTK_STOCK_UNDO);
		gtk_widget_set_sensitive (m_undo_button, FALSE);
		g_signal_connect (G_OBJECT (m_undo_button), "clicked",
		                  G_CALLBACK (undo_clicked_cb), this);
		undo_align = gtk_alignment_new (1, 0, 0, 0);
		gtk_container_add (GTK_CONTAINER (undo_align), m_undo_button);

		gtk_box_pack_start (GTK_BOX (vbox), m_view->getWidget(), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), undo_align, FALSE, FALSE, 0);
		gtk_container_add (GTK_CONTAINER (m_window), vbox);
		gtk_widget_show_all (m_window);
	}

	~TrashWindow()
	{
		delete m_view;
		gtk_widget_destroy (m_window);
	}

private:
std::list <Ypp::Package *> m_selection;

	virtual void packagesSelected (const std::list <Ypp::Package *> &selection)
	{
		gtk_widget_set_sensitive (m_undo_button, !selection.empty());
		m_selection = selection;
	}

	static void undo_clicked_cb (GtkButton *button, TrashWindow *pThis)
	{
		for (std::list <Ypp::Package *>::iterator it = pThis->m_selection.begin();
		     it != pThis->m_selection.end(); it++)
			(*it)->undo();
	}
};

#include "icons/cat-development.xpm"
#include "icons/cat-documentation.xpm"
#include "icons/cat-emulators.xpm"
#include "icons/cat-games.xpm"
#include "icons/cat-hardware.xpm"
#include "icons/cat-network.xpm"
#include "icons/cat-multimedia.xpm"
#include "icons/cat-office.xpm"
#include "icons/cat-system.xpm"
#include "icons/cat-utilities.xpm"

class Filters
{
public:
	struct Listener {
		virtual void doQuery (Ypp::Query *query) = 0;
	};
	void setListener (Listener *listener)
	{ m_listener = listener; signalChanged(); }

private:
	GtkWidget *m_widget, *m_name, *m_status, *m_categories, *m_repos, *m_type,
	          *m_categories_expander, *m_repos_expander;
	Listener *m_listener;
	guint timeout_id;
	int repoToggled;  // how many repos are toggled? if 0, it can speed up query
	int packageType;

public:
	GtkWidget *getWidget()
	{ return m_widget; }

	Filters()
	: m_listener (NULL), timeout_id (0), repoToggled (0), packageType (-1)
	{
		GtkWidget *vbox = gtk_vbox_new (FALSE, 4);
		gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
		GtkSizeGroup *size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

		m_name = ygtk_find_entry_new();
		g_signal_connect_after (G_OBJECT (m_name), "changed",
		                        G_CALLBACK (entry_changed_cb), this);
		gtk_box_pack_start (GTK_BOX (vbox),
			labelWidget (_("Name: "), m_name, size_group), FALSE, TRUE, 0);

		m_status = gtk_combo_box_new_text();
		gtk_combo_box_append_text (GTK_COMBO_BOX (m_status), _("All"));
		gtk_combo_box_append_text (GTK_COMBO_BOX (m_status), _("Installed"));
		gtk_combo_box_append_text (GTK_COMBO_BOX (m_status), _("Upgradable"));
		gtk_combo_box_append_text (GTK_COMBO_BOX (m_status), _("Available"));
		gtk_combo_box_append_text (GTK_COMBO_BOX (m_status), _("Modified"));
		gtk_combo_box_set_active (GTK_COMBO_BOX (m_status), 0);
		g_signal_connect_after (G_OBJECT (m_status), "changed",
		                        G_CALLBACK (combo_changed_cb), this);
		gtk_box_pack_start (GTK_BOX (vbox),
			labelWidget (_("Status: "), m_status, size_group), FALSE, TRUE, 0);

		gtk_box_pack_start (GTK_BOX (vbox), buildCategories(), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), buildRepos(), FALSE, TRUE, 0);

		m_type = gtk_combo_box_new_text();
		gtk_combo_box_append_text (GTK_COMBO_BOX (m_type), _("Packages"));
		gtk_combo_box_append_text (GTK_COMBO_BOX (m_type), _("Patterns"));
		gtk_combo_box_append_text (GTK_COMBO_BOX (m_type), _("Languages"));
		gtk_combo_box_set_active (GTK_COMBO_BOX (m_type), 0);
		g_signal_connect_after (G_OBJECT (m_type), "changed",
		                        G_CALLBACK (combo_changed_cb), this);
		gtk_box_pack_start (GTK_BOX (vbox), gtk_label_new (""), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (vbox),
			labelWidget (_("Type: "), m_type, size_group), FALSE, TRUE, 0);

		m_widget = gtk_notebook_new();
		gtk_notebook_append_page (GTK_NOTEBOOK (m_widget), vbox,
		                          gtk_label_new (_("Filters")));
		g_object_unref (G_OBJECT (size_group));
	}

	~Filters()
	{
		if (timeout_id) g_source_remove (timeout_id);
	}

private:
	static void entry_changed_cb (GtkEditable *editable, Filters *pThis)
	{ pThis->signalChangedDelay(); }
	static void combo_changed_cb (GtkComboBox *combo, Filters *pThis)
	{ pThis->signalChanged(); }

	void signalChanged()
	{
		if (!m_listener) return;

		Ypp::Package::Type type;
		switch (gtk_combo_box_get_active (GTK_COMBO_BOX (m_type)))
		{
			case 0: default: type = Ypp::Package::PACKAGE_TYPE; break;
			case 1: type = Ypp::Package::PATTERN_TYPE; break;
			case 2: type = Ypp::Package::LANGUAGE_TYPE; break;
				
		}

		Ypp::Query *query = new Ypp::Query (type);

		const char *name = gtk_entry_get_text (GTK_ENTRY (m_name));
		if (*name)
			query->setName (std::string (name));
		switch (gtk_combo_box_get_active (GTK_COMBO_BOX (m_status)))
		{
			case 1: query->setIsInstalled (true); break;
			case 2: query->setHasUpgrade (true); break;
			case 3: query->setIsInstalled (false); break;
			case 4: query->setIsModified (true); break;
			case 0: default: break;
				
		}

		{
			GtkTreeSelection *selection = gtk_tree_view_get_selection (
				GTK_TREE_VIEW (m_categories));
			GtkTreeModel *model;
			GtkTreeIter iter;
			if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
				Ypp::Node *category;
				gtk_tree_model_get (model, &iter, 1, &category, -1);
				if (category)
					query->setCategory (category);
			}
		}

		if (repoToggled) {
			std::list <int> reposQuery;
			GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (m_repos));
			GtkTreeIter iter;
			if (gtk_tree_model_get_iter_first (model, &iter)) {
				do {
					gboolean enabled;
					gint repo;
					gtk_tree_model_get (model, &iter, 0, &enabled, 2, &repo, -1);
					if (enabled)
						reposQuery.push_back (repo);
				} while (gtk_tree_model_iter_next (model, &iter));
			}
			query->setRepositories (reposQuery);
		}

		m_listener->doQuery (query);
		updateCategories (type);
	}

	void signalChangedDelay()
	{
		struct inner {
			static gboolean timeout_cb (gpointer data)
			{
				Filters *pThis = (Filters *) data;
				pThis->timeout_id = 0;
				pThis->signalChanged();
				return FALSE;
			}
		};
		if (timeout_id) g_source_remove (timeout_id);
		timeout_id = g_timeout_add (500, inner::timeout_cb, this);
	}

	void updateCategories (Ypp::Package::Type type)
	{
		if (packageType == type)
			return;
		packageType = type;

		GtkTreeModel *model = NULL;

		if (type == Ypp::Package::PACKAGE_TYPE) {
			struct inner {
				static void populate (GtkTreeStore *store, GtkTreeIter *parent,
				                      Ypp::Node *category)
				{
					if (!category)
						return;
					GtkTreeIter iter;
					gtk_tree_store_append (store, &iter, parent);
					const std::string &name = category->name;
					gtk_tree_store_set (store, &iter, 0, name.c_str(), 1, category, -1);
					if (!parent) {
						const char **icon = 0;
						if (name == "Development")
							icon = cat_development_xpm;
						else if (name == "Documentation")
							icon = cat_documentation_xpm;
						else if (name == "Emulators")
							icon = cat_emulators_xpm;
						else if (name == "Games")
							icon = cat_games_xpm;
						else if (name == "Hardware")
							icon = cat_hardware_xpm;
						else if (name == "Multimedia")
							icon = cat_multimedia_xpm;
						else if (name == "Network")
							icon = cat_network_xpm;
						else if (name == "Office")
							icon = cat_office_xpm;
						else if (name == "System")
							icon = cat_system_xpm;
						else if (name == "Utilities")
							icon = cat_utilities_xpm;
						if (icon) {
							GdkPixbuf *pixbuf = gdk_pixbuf_new_from_xpm_data (icon);
							gtk_tree_store_set (store, &iter, 2, pixbuf, -1);
						}
					}
					populate (store, &iter, category->child());
					populate (store, parent, category->next());
				}
			};

			GtkTreeStore *store = gtk_tree_store_new (3, G_TYPE_STRING, G_TYPE_POINTER,
			                                          GDK_TYPE_PIXBUF);
			model = GTK_TREE_MODEL (store);

			GtkTreeIter iter;
			gtk_tree_store_append (store, &iter, NULL);
			gtk_tree_store_set (store, &iter, 0, _("All"), 1, NULL, -1);

			inner::populate (store, NULL, Ypp::get()->getFirstCategory (type));

		}
		else if (type == Ypp::Package::PATTERN_TYPE) {
			GtkListStore *store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_POINTER,
			                                          GDK_TYPE_PIXBUF);
			model = GTK_TREE_MODEL (store);

			GtkTreeIter iter;
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, _("All"), 1, NULL, -1);

			for (Ypp::Node *i = Ypp::get()->getFirstCategory (type); i; i = i->next()) {
				gtk_list_store_append (store, &iter);
				gtk_list_store_set (store, &iter, 0, i->name.c_str(), 1, i, -1);
			}
		}

		GtkTreeSelection *selection = gtk_tree_view_get_selection (
			GTK_TREE_VIEW (m_categories));
		g_signal_handlers_block_by_func (selection,
			(gpointer) categories_selection_cb, this);

		gtk_tree_view_set_model (GTK_TREE_VIEW (m_categories), model);
		if (model) {
			g_object_unref (G_OBJECT (model));
			gtk_widget_show (m_categories_expander);

			/* we use gtk_tree_view_set_cursor(), rather than gtk_tree_selection_select_iter()
			   because that one is buggy in that when the user first interacts with the treeview,
			   a change signal is sent, even if he was just expanding one node... */
			GtkTreePath *path = gtk_tree_path_new_first();
			gtk_tree_view_set_cursor (GTK_TREE_VIEW (m_categories), path, NULL, FALSE);
			gtk_tree_path_free (path);
		}
		else
			gtk_widget_hide (m_categories_expander);

		g_signal_handlers_unblock_by_func (selection,
			(gpointer) categories_selection_cb, this);
	}

	GtkWidget *buildCategories()
	{
		GtkWidget *scroll;
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;

		m_categories = gtk_tree_view_new();
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (m_categories), FALSE);
		gtk_tree_view_set_search_column (GTK_TREE_VIEW (m_categories), 0);
		renderer = gtk_cell_renderer_pixbuf_new();
		column = gtk_tree_view_column_new_with_attributes ("",
			renderer, "pixbuf", 2, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (m_categories), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes ("",
			renderer, "text", 0, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (m_categories), column);

		GtkTreeSelection *selection = gtk_tree_view_get_selection (
			GTK_TREE_VIEW (m_categories));
		gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
		g_signal_connect (G_OBJECT (selection), "changed",
		                  G_CALLBACK (categories_selection_cb), this);

		scroll = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
		                                     GTK_SHADOW_IN);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
		                                GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_widget_set_size_request (scroll, -1, 200);
		gtk_container_add (GTK_CONTAINER (scroll), m_categories);

		m_categories_expander = gtk_expander_new (_("Categories"));
		gtk_container_add (GTK_CONTAINER (m_categories_expander), scroll);
		gtk_expander_set_expanded (GTK_EXPANDER (m_categories_expander), TRUE);
		return m_categories_expander;
	}

	GtkWidget *buildRepos()
	{
		GtkWidget *scroll;
		GtkListStore *store;
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;

		// 0 - enabled, 1 - name, 2 - ptr
		store = gtk_list_store_new (3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_INT);
		for (int i = 0; Ypp::get()->getRepository (i); i++) {
			const Ypp::Repository *repo = Ypp::get()->getRepository (i);
			GtkTreeIter iter;
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, TRUE, 1, repo->name.c_str(), 2, i, -1);
		}
		m_repos = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (m_repos), FALSE);
		gtk_tree_view_set_search_column (GTK_TREE_VIEW (m_repos), 1);
		gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (m_repos)),
		                             GTK_SELECTION_NONE);
		g_object_unref (G_OBJECT (store));
		renderer = gtk_cell_renderer_toggle_new();
		column = gtk_tree_view_column_new_with_attributes ("", renderer, "active", 0, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (m_repos), column);
		g_signal_connect (G_OBJECT (renderer), "toggled",
		                  G_CALLBACK (repo_toggled_cb), this);
		g_signal_connect (G_OBJECT (m_repos), "row-activated",
		                  G_CALLBACK (repo_clicked_cb), this);
		renderer = gtk_cell_renderer_text_new();
		g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
		column = gtk_tree_view_column_new_with_attributes ("",
			renderer, "text", 1, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (m_repos), column);

		scroll = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
		                                     GTK_SHADOW_IN);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
		                                GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_widget_set_size_request (scroll, -1, 80);
		gtk_container_add (GTK_CONTAINER (scroll), m_repos);

		m_repos_expander = gtk_expander_new (_("Repositories"));
		gtk_container_add (GTK_CONTAINER (m_repos_expander), scroll);
		return m_repos_expander;
	}

	static void categories_selection_cb (GtkTreeSelection *selection, Filters *pThis)
	{
fprintf (stderr, "CATEGORIES SELECTED\n");
		pThis->signalChanged();
	}

	void toggle_repo (GtkTreePath *path)
	{
		IMPL
		GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (m_repos));
		GtkListStore *store = GTK_LIST_STORE (model);
		GtkTreeIter iter;
		gtk_tree_model_get_iter (model, &iter, path);

		gboolean enabled;
		gtk_tree_model_get (model, &iter, 0, &enabled, -1);
		gtk_list_store_set (store, &iter, 0, !enabled, -1);
		if (enabled) repoToggled--; else repoToggled++;
fprintf (stderr, "repoToggled count: %d\n", repoToggled);
		signalChanged();
	}

	static void repo_toggled_cb (GtkCellRendererToggle *renderer,
	                             gchar *path_str, Filters *pThis)
	{
		IMPL
		GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
		pThis->toggle_repo (path);
		gtk_tree_path_free (path);
	}

	static void repo_clicked_cb (GtkTreeView *view, GtkTreePath *path,
	                             GtkTreeViewColumn *column, Filters *pThis)
	{
		IMPL
		pThis->toggle_repo (path);
	}

	// utility
	static GtkWidget *labelWidget (const char *label_str, GtkWidget *widget,
	                               GtkSizeGroup *group)
	{
		GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
		GtkWidget *label = gtk_label_new_with_mnemonic (label_str);
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
		gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
		gtk_size_group_add_widget (group, label);
		return hbox;
	}
};

#include "icons/pkg-locked.xpm"
#include "icons/pkg-unlocked.xpm"

class PackageControl
{
GtkWidget *m_vbox, *m_status, *m_lock_button, *m_locked_image, *m_unlocked_image,
          *m_installed_box, *m_available_box, *m_remove_button, *m_install_button,
          *m_undo_button, *m_versions_combo, *m_version_repo;
GtkTreeModel *m_versions_model;

public:
std::list <Ypp::Package *> m_packages;

	GtkWidget *getWidget()
	{ return m_vbox; }

	PackageControl()
	{
		GtkSizeGroup *size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
		m_vbox = gtk_vbox_new (FALSE, 6);
		GtkWidget *hbox, *label;

		m_lock_button = gtk_toggle_button_new();
		g_signal_connect (G_OBJECT (m_lock_button), "toggled",
		                  G_CALLBACK (locked_toggled_cb), this);
		m_undo_button = gtk_button_new_from_stock (GTK_STOCK_UNDO);
		g_signal_connect (G_OBJECT (m_undo_button), "clicked",
		                  G_CALLBACK (undo_clicked_cb), this);

		hbox = gtk_hbox_new (FALSE, 6);
		label = gtk_label_new (_("Status:"));
		YGUtils::setWidgetFont (label, PANGO_WEIGHT_BOLD, PANGO_SCALE_MEDIUM);
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
		gtk_size_group_add_widget (size_group, label);
		m_status = gtk_label_new ("-");
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), m_status, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), m_undo_button, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new(0), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), m_lock_button, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_vbox), hbox, TRUE, TRUE, 0);

		m_locked_image = createImageFromXPM (pkg_locked_xpm);
		m_unlocked_image = createImageFromXPM (pkg_unlocked_xpm);
		g_object_ref_sink (G_OBJECT (m_locked_image));
		g_object_ref_sink (G_OBJECT (m_unlocked_image));

		m_remove_button = createButton (_("_Remove"), GTK_STOCK_DELETE);
		g_signal_connect (G_OBJECT (m_remove_button), "clicked",
		                  G_CALLBACK (remove_clicked_cb), this);
		m_install_button = createButton ("", GTK_STOCK_SAVE);
		g_signal_connect (G_OBJECT (m_install_button), "clicked",
		                  G_CALLBACK (install_clicked_cb), this);

		m_installed_box = gtk_hbox_new (FALSE, 6);
		label = gtk_label_new (_("Installed: "));
		YGUtils::setWidgetFont (label, PANGO_WEIGHT_BOLD, PANGO_SCALE_MEDIUM);
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
		gtk_size_group_add_widget (size_group, label);
		gtk_box_pack_start (GTK_BOX (m_installed_box), label, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_installed_box), m_remove_button, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_vbox), m_installed_box, TRUE, TRUE, 0);

		m_versions_model = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT));
		m_versions_combo = gtk_combo_box_new_with_model (m_versions_model);
		g_signal_connect (G_OBJECT (m_versions_combo), "changed",
		                  G_CALLBACK (version_changed_cb), this);

		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (m_versions_combo), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (m_versions_combo),
		                                renderer, "text", 0, NULL);

		m_available_box = gtk_hbox_new (FALSE, 6);
		label = gtk_label_new (_("Available: "));
		YGUtils::setWidgetFont (label, PANGO_WEIGHT_BOLD, PANGO_SCALE_MEDIUM);
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
		gtk_size_group_add_widget (size_group, label);
		gtk_box_pack_start (GTK_BOX (m_available_box), label, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_available_box), m_versions_combo, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_available_box), m_install_button, FALSE, TRUE, 0);
		label = gtk_label_new (_("(Repository:"));
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
		m_version_repo = gtk_label_new ("");
		gtk_misc_set_alignment (GTK_MISC (m_version_repo), 0, 0.5);
		gtk_label_set_ellipsize (GTK_LABEL (m_version_repo), PANGO_ELLIPSIZE_END);
		gtk_box_pack_start (GTK_BOX (m_available_box), label, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_available_box), m_version_repo, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_vbox), m_available_box, TRUE, TRUE, 0);

		g_object_unref (G_OBJECT (size_group));
	}

	~PackageControl()
	{
		gtk_widget_destroy (m_locked_image);
		g_object_unref (G_OBJECT (m_locked_image));
		gtk_widget_destroy (m_unlocked_image);
		g_object_unref (G_OBJECT (m_unlocked_image));
	}

	void setPackages (const std::list <Ypp::Package *> &packages)
	{
fprintf (stderr, "packages selected (%d)\n", packages.size());
		// FIXME: we'll probably want to re-work the controls interface
		// for now, let's just handle none-one-multiple selections separatadely
		m_packages = packages;
		if (packages.size() == 1) {
			Ypp::Package *package = packages.front();
			// status label
			std::string status;
			if (package->isInstalled())
				status = _("Installed ") + package->getInstalledVersion()->number;
			else
				status = _("Not installed");
			if (package->toInstall()) {
				int nb;
				const Ypp::Package::Version *version;
				package->toInstall (&nb);
				version = package->getAvailableVersion (nb);
				status += _(" (to install");
				if (version)
					status += " " + version->number;
				status += ")";
			}
			else if (package->toRemove())
				status += _(" (to remove)");
			gtk_label_set_text (GTK_LABEL (m_status), status.c_str());
			if (package->isModified())
				gtk_widget_show (m_undo_button);
			else
				gtk_widget_hide (m_undo_button);

			// install version
			if (package->isInstalled())
				gtk_widget_show (m_installed_box);
			else
				gtk_widget_hide (m_installed_box);

			// available versions
			gtk_widget_show (m_versions_combo);
			if (package->getAvailableVersion (0)) {
				gtk_list_store_clear (GTK_LIST_STORE (m_versions_model));
				for (int i = 0; package->getAvailableVersion (i); i++) {
					const char *version = package->getAvailableVersion (i)->number.c_str();
fprintf (stderr, "adding version: %s\n", version);
					GtkTreeIter iter;
					gtk_list_store_append (GTK_LIST_STORE (m_versions_model), &iter);
					gtk_list_store_set (GTK_LIST_STORE (m_versions_model), &iter,
					                    0, version, 1, i, -1);
				}
				gtk_combo_box_set_active (GTK_COMBO_BOX (m_versions_combo), 0);
				gtk_widget_show (m_available_box);
			}
			else
				gtk_widget_hide (m_available_box);

			// is locked
			gtk_widget_show (m_lock_button);
			bool locked = package->isLocked();
			g_signal_handlers_block_by_func (m_lock_button, (gpointer) locked_toggled_cb, this);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (m_lock_button), locked);
			g_signal_handlers_unblock_by_func (m_lock_button, (gpointer) locked_toggled_cb, this);
			gtk_button_set_image (GTK_BUTTON (m_lock_button),
				locked ? m_locked_image : m_unlocked_image);
			gtk_widget_set_sensitive (m_install_button, !locked);
			gtk_widget_set_sensitive (m_remove_button, !locked);

			gtk_widget_show (m_vbox);
		}
		else if (packages.size()) {
			bool allInstalled = true, allNotInstalled = true, allUpgradable = true,
			     allModified = true, allLocked = true, allUnlocked = true;
			for (std::list <Ypp::Package *>::const_iterator it = packages.begin();
			     it != packages.end(); it++) {
				if (!(*it)->isInstalled()) {
					allInstalled = false;
					allUpgradable = false;
				 }
				 else {
				 	allNotInstalled = false;
				 	const Ypp::Package::Version *version = (*it)->getAvailableVersion(0);
				 	if (!version || version->cmp <= 0)
				 		allUpgradable = false;
				 }
				if (!(*it)->isModified())
					allModified = false;
				if ((*it)->isLocked())
					allUnlocked = false;
				else
					allLocked = false;
			}

			std::string status;
			if (allInstalled)
				status = _("Installed");
			else if (allNotInstalled)
				status = _("Not installed");
			else
				status = _("--");

			if (allModified) {
				status += _(" (modified)");
				gtk_widget_show (m_undo_button);
			}
			else
				gtk_widget_hide (m_undo_button);
			gtk_label_set_text (GTK_LABEL (m_status), status.c_str());

			// install version
			if (allInstalled)
				gtk_widget_show (m_installed_box);
			else
				gtk_widget_hide (m_installed_box);

			// available versions
			gtk_widget_hide (m_versions_combo);
			if (allNotInstalled || allUpgradable) {
				gtk_widget_show (m_available_box);
				const char *installLabel = _("Install");
				if (allUpgradable)
					installLabel = _("Upgrade");
				gtk_button_set_label (GTK_BUTTON (m_install_button), installLabel);
			}
			else
				gtk_widget_hide (m_available_box);

			// is locked
			if (allLocked || allUnlocked) {
				gtk_widget_show (m_lock_button);
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (m_lock_button), allLocked);
				gtk_button_set_image (GTK_BUTTON (m_lock_button),
					allLocked ? m_locked_image : m_unlocked_image);
				gtk_widget_set_sensitive (m_install_button, !allLocked);
				gtk_widget_set_sensitive (m_remove_button, !allLocked);
			}
			else {
				gtk_widget_hide (m_lock_button);
				gtk_widget_set_sensitive (m_install_button, TRUE);
				gtk_widget_set_sensitive (m_remove_button, TRUE);
			}
			gtk_widget_show (m_vbox);
		}
		else
			gtk_widget_hide (m_vbox);

#if 0
		m_package = package;
		if (package) {
			if (package->isInstalled())	{
				gtk_widget_show (m_remove_button);
				gtk_widget_hide (m_install_button);
			}
			else {
				gtk_widget_show (m_install_button);
				gtk_widget_hide (m_remove_button);
			}
			if (package && package->hasUpgrade())
				gtk_widget_show (m_upgrade_button);
			else
				gtk_widget_hide (m_upgrade_button);
		}
		else {
			gtk_widget_hide (m_install_button);
			gtk_widget_hide (m_remove_button);
			gtk_widget_hide (m_upgrade_button);
		}
#endif
	}

private:
	static void install_clicked_cb (GtkButton *button, PackageControl *pThis)
	{
		busyCursor();
		Ypp::get()->startTransactions();
		for (std::list <Ypp::Package *>::iterator it = pThis->m_packages.begin();
		     it != pThis->m_packages.end(); it++) {
			int version;
			if (GTK_WIDGET_VISIBLE (pThis->m_versions_combo))
				version = gtk_combo_box_get_active (GTK_COMBO_BOX (
					pThis->m_versions_combo));
			else
				version = 0;  // i.e. most recent (on multi-packages)
			(*it)->install (version);
		}
		Ypp::get()->finishTransactions();
		normalCursor();
	}

	static void remove_clicked_cb (GtkButton *button, PackageControl *pThis)
	{
		busyCursor();
		Ypp::get()->startTransactions();
		for (std::list <Ypp::Package *>::iterator it = pThis->m_packages.begin();
		     it != pThis->m_packages.end(); it++)
			(*it)->remove();
		Ypp::get()->finishTransactions();
		normalCursor();
	}

	static void undo_clicked_cb (GtkButton *button, PackageControl *pThis)
	{
		busyCursor();
		Ypp::get()->startTransactions();
		for (std::list <Ypp::Package *>::iterator it = pThis->m_packages.begin();
		     it != pThis->m_packages.end(); it++)
			(*it)->undo();
		Ypp::get()->finishTransactions();
		normalCursor();
	}

	static void locked_toggled_cb (GtkToggleButton *button, PackageControl *pThis)
	{
		busyCursor();
		Ypp::get()->startTransactions();
		for (std::list <Ypp::Package *>::iterator it = pThis->m_packages.begin();
		     it != pThis->m_packages.end(); it++)
			(*it)->lock (gtk_toggle_button_get_active (button));
		Ypp::get()->finishTransactions();
		normalCursor();
	}

	static void version_changed_cb (GtkComboBox *combo, PackageControl *pThis)
	{
		fprintf (stderr, "version changed\n");
		if (!pThis->m_packages.empty()) {
			Ypp::Package *package = pThis->m_packages.front();
			int nb;
			GtkTreeIter iter;
			if (gtk_combo_box_get_active_iter (combo, &iter)) {
				gtk_tree_model_get (pThis->m_versions_model, &iter, 1, &nb, -1);
				const Ypp::Package::Version *version;
				fprintf (stderr, "get available version %d\n", nb);
				version = package->getAvailableVersion (nb);
				g_assert (version != NULL);

				const Ypp::Repository *repo = Ypp::get()->getRepository (version->repo);
				if (repo) {
					std::string repo_str = repo->name + ")";
					gtk_label_set_text (GTK_LABEL (pThis->m_version_repo), repo_str.c_str());
				}
				else
					gtk_label_set_text (GTK_LABEL (pThis->m_version_repo),
						"TODO: hide this for collections)");

				const char *installLabel = _("Install");
				if (package->isInstalled()) {
					if (version->cmp > 0)
						installLabel = _("Upgrade");
					else if (version->cmp == 0)
						installLabel = _("Re-install");
					else //if (version->cmp < 0)
						installLabel = _("Downgrade");
				}
				gtk_button_set_label (GTK_BUTTON (pThis->m_install_button), installLabel);
			}
		}
	}

	// utility
	static GtkWidget *createButton (const char *label_str, const gchar *stock_id)
	{
		GtkWidget *button, *image;
		button = gtk_button_new_with_mnemonic (label_str);
		if (stock_id) {
			image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
			gtk_button_set_image (GTK_BUTTON (button), image);
		}
		return button;
	}
};

class PackageInfo
{
GtkWidget *m_widget, *m_description, *m_filelist, *m_changelog, *m_authors;

public:
	GtkWidget *getWidget()
	{ return m_widget; }

	PackageInfo()
	{
		m_widget = gtk_notebook_new();
		addPage (_("Description"), createHtmlWidget (&m_description));
		addPage (_("File List"), createHtmlWidget (&m_filelist));
		addPage (_("ChangeLog"), createHtmlWidget (&m_changelog));
		addPage (_("Authors"), createHtmlWidget (&m_authors));
		gtk_widget_set_size_request (m_widget, -1, 150);
		ygtk_html_wrap_connect_link_clicked (m_filelist,
			G_CALLBACK (path_pressed_cb), NULL);
	}

	void setPackage (Ypp::Package *package)
	{
		if (package) {
			setText (m_description, package->description());
			setText (m_filelist, package->filelist());
			setText (m_changelog, package->changelog());
			setText (m_authors, package->authors());
			if (!GTK_WIDGET_VISIBLE (m_widget)) {
				gtk_notebook_set_current_page (GTK_NOTEBOOK (m_widget), 0);
				gtk_widget_show (m_widget);
			}
		}
		else
			gtk_widget_hide (m_widget);
	}

private:
	static void path_pressed_cb (GtkWidget *text, const gchar *link)
	{ FILEMANAGER_LAUNCH (link); }

	// utilities:
	static GtkWidget *createHtmlWidget (GtkWidget **html_widget)
	{
		*html_widget = ygtk_html_wrap_new();
		GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
		                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_container_add (GTK_CONTAINER (scroll), *html_widget);
		return scroll;
	}

	void addPage (const char *title, GtkWidget *content)
	{
		gtk_notebook_append_page (GTK_NOTEBOOK (m_widget), content, gtk_label_new (title));
	}

	void setText (GtkWidget *rtext, const std::string &text)
	{
		if (text.empty()) {
			const char *empty = _("<i>(only available for installed packages)</i>");
			ygtk_html_wrap_set_text (rtext, empty);
		}
		else
			ygtk_html_wrap_set_text (rtext, text.c_str());
		ygtk_html_wrap_scroll (rtext, TRUE);
	}
};

class DiskView : public Ypp::Disk::Listener
{
GtkWidget *m_widget;
bool m_hasWarn;
GtkTreeModel *m_model;

public:
	GtkWidget *getWidget()
	{ return m_widget; }

	DiskView()
	: m_hasWarn (false)
	{
		m_model = GTK_TREE_MODEL (gtk_list_store_new (
			// 0 - mount point, 1 - usage percent, 2 - usage string
			3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING));
		m_widget = createCombo (m_model);
		g_object_unref (G_OBJECT (m_model));

		Ypp::get()->getDisk()->setListener (this);
		update();
	}

private:
	#define MIN_FREE_MB_WARN (80*1024)
	virtual void update()
	{
		GtkListStore *store = GTK_LIST_STORE (m_model);
		gtk_list_store_clear (store);

		int warn_part = -1;
		Ypp::Disk *disk = Ypp::get()->getDisk();
		for (int i = 0; disk->getPartition (i); i++) {
			const Ypp::Disk::Partition *part = disk->getPartition (i);
			long usage = (part->used * 100) / (part->total + 1);
			std::string usage_str = part->used_str + " (of " + part->total_str + ")";
			GtkTreeIter iter;
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, part->path.c_str(), 1, usage,
			                    2, usage_str.c_str(), -1);
			if (warn_part < 0 && (part->total > 1024 && part->total - part->used < MIN_FREE_MB_WARN))
				warn_part = i;
		}
		if (warn_part >= 0) {
			warn();
			gtk_combo_box_set_active (GTK_COMBO_BOX (m_widget), warn_part);
		}
		else
			gtk_combo_box_set_active (GTK_COMBO_BOX (m_widget), 0);
	}

	void warn()
	{
		if (m_hasWarn) return;
		m_hasWarn = true;

		GtkWidget *dialog, *view;
		dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING,
			GTK_BUTTONS_OK, _("Disk Almost Full !"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
			"One of the partitions is reaching its limit of capacity. You may "
			"have to remove packages if you wish to install some.");

		view = createView (m_model);
		gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), view);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}

	// utilities
#if 1
	static GtkWidget *createView (GtkTreeModel *model)
	{
		GtkWidget *view = gtk_tree_view_new_with_model (model), *scroll;
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
		gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)),
		                             GTK_SELECTION_NONE);

		GtkTreeViewColumn *column;
		column = gtk_tree_view_column_new_with_attributes (_("Mount Point"),
			gtk_cell_renderer_text_new(), "text", 0, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
		column = gtk_tree_view_column_new_with_attributes (_("Usage"),
			gtk_cell_renderer_progress_new(), "value", 1, "text", 2, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

		scroll = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
		                                     GTK_SHADOW_IN);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
		                                GTK_POLICY_NEVER, GTK_POLICY_NEVER);
//		gtk_widget_set_size_request (scroll, -1, 50);
		gtk_container_add (GTK_CONTAINER (scroll), view);
		gtk_widget_show_all (scroll);
		return scroll;
	}
#endif
	static GtkWidget *createCombo (GtkTreeModel *model)
	{
		GtkWidget *combo = gtk_combo_box_new_with_model (model);
		GtkCellRenderer *renderer;
		renderer = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo),
		                                renderer, "text", 0, NULL);
		// let's put all columns to the same width, to make it look like a table
		g_object_set (G_OBJECT (renderer), "width-chars", 14, NULL);
		renderer = gtk_cell_renderer_progress_new();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo),
		                                renderer, "value", 1, "text", 2, NULL);
		return combo;
	}
};

class PackageSelector : public Filters::Listener, public PackagesView::Listener
{
PackagesView *m_packages;
Filters *m_filters;
PackageInfo *m_info;
PackageControl *m_control;
DiskView *m_disk;
GtkWidget *m_box;
TrashWindow *m_trashWin;

public:
	GtkWidget *getWidget()
	{ return m_box; }

	PackageSelector()
	{
		m_packages = new PackagesView();
		m_packages->setListener (this);

		m_control  = new PackageControl();
		m_info = new PackageInfo();

		GtkWidget *vbox = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (vbox), m_packages->getWidget(), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), m_control->getWidget(), FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), m_info->getWidget(), FALSE, TRUE, 0);

		m_filters = new Filters();
		m_filters->setListener (this);
		m_disk = new DiskView();

		GtkWidget *left_box = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (left_box), m_filters->getWidget(), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (left_box), m_disk->getWidget(), FALSE, TRUE, 0);

		m_box = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (m_box), left_box, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_box), vbox, TRUE, TRUE, 0);
		gtk_widget_show_all (m_box);

		m_control->setPackages (std::list <Ypp::Package*> ());
		m_info->setPackage (NULL);

		m_trashWin = new TrashWindow();
	}

	~PackageSelector()
	{
		delete m_packages;
		delete m_control;
		delete m_info;
		delete m_filters;
		delete m_disk;
		delete m_trashWin;
	}

	virtual void doQuery (Ypp::Query *query)
	{
		m_packages->query (query);
	}

	virtual void packagesSelected (const std::list <Ypp::Package *> &packages)
	{
		m_control->setPackages (packages);
		if (packages.size() == 1)
			m_info->setPackage (packages.front());
		else
			m_info->setPackage (NULL);
	}

	void packageModified (Ypp::Package *package)
	{
		// GTK+ doesn't fire selection change when a selected row changes, so we need
		// to re-load PackageControl in that occasions.
		std::list <Ypp::Package *>::iterator it;
		for (it = m_control->m_packages.begin(); it != m_control->m_packages.end(); it++)
			if (*it == package)
				break;
		if (it != m_control->m_packages.end())
			m_control->setPackages (m_control->m_packages);
	}
};

#include "YPackageSelector.h"

class YGPackageSelector : public YPackageSelector, public YGWidget, public Ypp::Interface
{
PackageSelector *m_package_selector;

public:
	YGPackageSelector (YWidget *parent, long mode)
		: YPackageSelector (NULL, mode),
		  YGWidget (this, parent, true, YGTK_TYPE_WIZARD, NULL)
	{
fprintf (stderr, "YGPackageSelector()\n");
		setBorder (0);
        YGDialog *dialog = YGDialog::currentDialog();
        dialog->setCloseCallback (confirm_cb, this);

        GtkWindow *window = YGDialog::currentWindow();
		gtk_window_resize (window, GTK_WIDGET (window)->allocation.width,
		                   MAX (580, GTK_WIDGET (window)->allocation.height));

fprintf (stderr, "wizard setup\n");
		YGtkWizard *wizard = YGTK_WIZARD (getWidget());
		ygtk_wizard_set_header_icon (wizard, window,
			THEMEDIR "/icons/32x32/apps/yast-software.png");
		ygtk_wizard_set_header_text (wizard, window, _("Package Selector"));
		ygtk_wizard_set_help_text (wizard,
			_("TO WRITE")
		);

fprintf (stderr, "buttons strings\n");
		ygtk_wizard_set_abort_button_label (wizard, _("_Cancel"));
		ygtk_wizard_set_abort_button_str_id (wizard, "cancel");
		ygtk_wizard_set_back_button_label (wizard, "");
		ygtk_wizard_set_next_button_label (wizard, _("_Accept"));
		ygtk_wizard_set_next_button_str_id (wizard, "accept");
		g_signal_connect (G_OBJECT (getWidget()), "action-triggered",
		                  G_CALLBACK (wizard_action_cb), this);

fprintf (stderr, "create package selector\n");
		m_package_selector = new PackageSelector();
		gtk_container_add (GTK_CONTAINER (wizard), m_package_selector->getWidget());

fprintf (stderr, "ypp::set interface()\n");
		Ypp::get()->setInterface (this);
fprintf (stderr, "done()\n");
	}

	virtual ~YGPackageSelector()
	{
		delete m_package_selector;
		ygtk_zypp_model_finish();
	}

protected:
	static void wizard_action_cb (YGtkWizard *wizard, gpointer id,
	                              gint id_type, YGPackageSelector *pThis)
	{
		IMPL
fprintf (stderr, "wizard action\n");
		const gchar *action = (gchar *) id;
fprintf (stderr, "action: %s\n", action);

		if (!strcmp (action, "accept")) {
			y2milestone ("Closing PackageSelector with 'accept'");
			YGUI::ui()->sendEvent (new YMenuEvent (YCPSymbol ("accept")));
		}
		else if (!strcmp (action, "cancel")) {
			y2milestone ("Closing PackageSelector with 'cancel'");
			if (pThis->confirmExit())
				YGUI::ui()->sendEvent (new YCancelEvent());
		}
	}

    bool confirmExit()
    {
    	if (!Ypp::get()->isModified())
    		return true;

        GtkWidget *dialog;
		dialog = gtk_message_dialog_new
            (YGDialog::currentWindow(),
			 GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING,
             GTK_BUTTONS_NONE, _("Changes not saved!"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
		                                          _("Quit anyway?"));
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
		                        GTK_STOCK_QUIT, GTK_RESPONSE_YES, NULL);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

        bool ok = gtk_dialog_run (GTK_DIALOG (dialog)) ==
                                   GTK_RESPONSE_YES;
		gtk_widget_destroy (dialog);
        return ok;
    }
    static bool confirm_cb (void *pThis)
    { return ((YGPackageSelector *)pThis)->confirmExit(); }

	virtual bool acceptLicense (Ypp::Package *package, const std::string &license)
	{
		std::string title = package->name() + _(" License Agreement");
		GtkWidget *dialog = gtk_dialog_new_with_buttons (title.c_str(),
			YGDialog::currentWindow(), GTK_DIALOG_NO_SEPARATOR,
			_("_Reject"), GTK_RESPONSE_REJECT, _("_Accept"), GTK_RESPONSE_ACCEPT, NULL);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

		GtkWidget *license_view, *license_window;

		license_view = ygtk_html_wrap_new();
		ygtk_html_wrap_set_text (license_view, license.c_str());

		license_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (license_window),
			                            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type
			(GTK_SCROLLED_WINDOW (license_window), GTK_SHADOW_IN);
		gtk_container_add (GTK_CONTAINER (license_window), license_view);

		GtkBox *vbox = GTK_BOX (GTK_DIALOG(dialog)->vbox);
		gtk_box_pack_start (vbox, license_window, TRUE, TRUE, 6);

		gtk_window_set_default_size (GTK_WINDOW (dialog), 500, 400);
		gtk_widget_show_all (dialog);

		gint ret = gtk_dialog_run (GTK_DIALOG (dialog));
		bool confirmed = (ret == GTK_RESPONSE_ACCEPT);

		gtk_widget_destroy (dialog);
		return confirmed;
	}

	virtual bool resolveProblems (std::list <Ypp::Problem *> problems)
	{
fprintf (stderr, "resolving %d problems\n", problems.size());
		// we can't use ordinary radio buttons, as gtk+ enforces that in a group
		// one must be selected...

		enum ColumnAlias {
			SHOW_TOGGLE_COL, ACTIVE_TOGGLE_COL, TEXT_COL, WEIGHT_TEXT_COL,
			APPLY_PTR_COL, TOOLTIP_TEXT_COL
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

				// disable all the other radios on the group, setting current
				gtk_tree_model_get_iter (model, &iter, path);
				if (gtk_tree_model_iter_parent (model, &parent, &iter)) {
					gtk_tree_model_iter_children (model, &iter, &parent);
					do {
						gtk_tree_store_set (store, &iter, ACTIVE_TOGGLE_COL, FALSE, -1);
						bool *apply;
						gtk_tree_model_get (model, &iter, APPLY_PTR_COL, &apply, -1);
						*apply = false;
					} while (gtk_tree_model_iter_next (model, &iter));
				}

				enabled = !enabled;
				if (apply)
					*apply = enabled;
				gtk_tree_model_get_iter (model, &iter, path);
				gtk_tree_store_set (store, &iter, ACTIVE_TOGGLE_COL, enabled, -1);
			}
#if 0
			static void renderer_toggled_cb (GtkCellRenderer *render, gchar *path_str,
			                                 GtkTreeModel *model)
			{
				GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
				solution_toggled (model, path);
				gtk_tree_path_free (path);
			}
#endif
			static void cursor_changed_cb (GtkTreeView *view, GtkTreeModel *model)
			{
				GtkTreePath *path;
				gtk_tree_view_get_cursor (view, &path, NULL);
				solution_toggled (model, path);
				gtk_tree_path_free (path);
			}
			static gboolean query_tooltip_cb (GtkWidget *view, gint x, gint y,
				gboolean keyboard_mode, GtkTooltip *tooltip, gpointer data)
			{
				GtkTreeModel *model;
				GtkTreePath *path;
				GtkTreeIter iter;
				if (gtk_tree_view_get_tooltip_context (GTK_TREE_VIEW (view),
				        &x, &y, keyboard_mode, &model, &path, &iter)) {
					gchar *tooltip_str;
					gtk_tree_model_get (model, &iter, TOOLTIP_TEXT_COL,
					                    &tooltip_str, -1);
					gtk_tree_view_set_tooltip_row (GTK_TREE_VIEW (view), tooltip, path);
					gtk_tree_path_free (path);
					if (tooltip_str) {
						gtk_tooltip_set_text (tooltip, tooltip_str);
						g_free (tooltip_str);
						return TRUE;
					}
				}
				return FALSE;


/*
				GtkTreePath *path;
				if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (view), x, y,
				                                   &path, NULL, NULL, NULL)) {
					GtkTreeIter iter;
					gchar *tooltip_text;
					gtk_tree_model_get_iter (model, &iter, path);
					gtk_tree_path_free (path);
					gtk_tree_model_get (model, &iter,
					                    TOOLTIP_TEXT_COL, &tooltip_text, -1);
					// some strings are just "   ", so we need to check that
					// now, 
					bool empty = true;
					for (int i = 0; tooltip_text[i] && empty; i++)
						if (tooltip_text[i] != ' ')
							empty = false;
					if (empty)
						gtk_tooltip_set_text (tooltip, "(no details)");
					else
						gtk_tooltip_set_text (tooltip, tooltip_text);
					g_free (tooltip_text);
					return TRUE;
				}
				return FALSE;
*/
			}
		};

		// model
		GtkTreeStore *store = gtk_tree_store_new (6, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
			G_TYPE_STRING, G_TYPE_INT, G_TYPE_POINTER, G_TYPE_STRING);
		for (std::list <Ypp::Problem *>::iterator it = problems.begin();
		     it != problems.end(); it++) {
			GtkTreeIter problem_iter;
			gtk_tree_store_append (store, &problem_iter, NULL);
			gtk_tree_store_set (store, &problem_iter, SHOW_TOGGLE_COL, FALSE,
				TEXT_COL, (*it)->description.c_str(),
				WEIGHT_TEXT_COL, PANGO_WEIGHT_BOLD, APPLY_PTR_COL, NULL,
				TOOLTIP_TEXT_COL, (*it)->details.c_str(), -1);

			for (int i = 0; (*it)->getSolution (i); i++) {
				Ypp::Problem::Solution *solution = (*it)->getSolution (i);
				GtkTreeIter solution_iter;
				const gchar *tooltip_text = solution->details.c_str();
				if (solution->details.empty())
					tooltip_text = NULL;
				gtk_tree_store_append (store, &solution_iter, &problem_iter);
				gtk_tree_store_set (store, &solution_iter, SHOW_TOGGLE_COL, TRUE,
					ACTIVE_TOGGLE_COL, FALSE, TEXT_COL, solution->description.c_str(),
					APPLY_PTR_COL, &solution->apply,
					TOOLTIP_TEXT_COL, tooltip_text, -1);
			}
		}

		// interface
		GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
			GtkDialogFlags (0), GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
			_("There are some conflicts on the transaction that must be "
			  "solved manually."));
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
/*		g_signal_connect (G_OBJECT (renderer), "toggled",
			G_CALLBACK (inner::renderer_toggled_cb), store);*/
		// we should not connect the actual toggle button, as we toggle on row press
		g_signal_connect (G_OBJECT (view), "cursor-changed",
			G_CALLBACK (inner::cursor_changed_cb), store);
		column = gtk_tree_view_column_new_with_attributes ("", renderer,
			"visible", SHOW_TOGGLE_COL, "active", ACTIVE_TOGGLE_COL, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes ("", renderer,
			"text", TEXT_COL, "weight", WEIGHT_TEXT_COL, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
		gtk_tree_view_expand_all (GTK_TREE_VIEW (view));
		gtk_widget_set_has_tooltip (view, TRUE);
		g_signal_connect (G_OBJECT (view), "query-tooltip",
			G_CALLBACK (inner::query_tooltip_cb), store);

		GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
			GTK_SHADOW_IN);
		gtk_container_add (GTK_CONTAINER (scroll), view);
		gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), scroll);

#if 0
		// version that uses viewport, labels and radio buttons -- unfortunately,
		// gtk+ enforces that one radio button per group to be selected -- we don't
		// want that as the user may decide to do nothing for a certain action.
		// We also don't want to add a "Do nothing" radio, as that would imply would
		// dismiss the conflict. No, we just want to let the user try to resolve current
		// conflicts...
		struct inner {
			static void solution_toggled (GtkToggleButton *toggle, bool *apply)
			{ *apply = gtk_toggle_button_get_active (toggle); }
		};

		GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Problem Resolver"),
			YGUI::ui()->currentWindow(), GtkDialogFlags (0),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_APPLY, GTK_RESPONSE_APPLY, NULL);

		GtkWidget *view_port = gtk_viewport_new (NULL, NULL);
		gtk_viewport_set_shadow_type (GTK_VIEWPORT (view_port), GTK_SHADOW_NONE);
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), view_port,
		                    TRUE, TRUE, 0); 
		GtkWidget *main_box = gtk_vbox_new (FALSE, 6);
		gtk_container_add (GTK_CONTAINER (view_port), main_box);

fprintf (stderr, "populating main box\n");
		for (std::list <Ypp::Problem *>::iterator it = problems.begin();
		     it != problems.end(); it++) {
			GtkWidget *label, *box;
fprintf (stderr, "creating label: %s\n", (*it)->description.c_str());
			label = gtk_label_new ((*it)->description.c_str());
			YGUtils::setWidgetFont (label, PANGO_WEIGHT_BOLD, PANGO_SCALE_MEDIUM);
fprintf (stderr, "setting tooltyp: %s\n", (*it)->details.c_str());
			if (!(*it)->details.empty())
				gtk_widget_set_tooltip_text (label, (*it)->details.c_str());

			GSList *group = NULL;
			box = gtk_vbox_new (FALSE, 6);
			for (int i = 0; (*it)->getSolution (i); i++) {
				Ypp::Problem::Solution *solution = (*it)->getSolution (i);
fprintf (stderr, "creating radio: %s\n", solution->description.c_str());
				GtkWidget *radio = gtk_radio_button_new_with_label (
					group, solution->description.c_str());
fprintf (stderr, "setting tooltip: %s\n", (*it)->details.c_str());
				if (!solution->details.empty())
					gtk_widget_set_tooltip_text (radio, solution->details.c_str());
				group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));
				gtk_box_pack_start (GTK_BOX (box), radio, FALSE, TRUE, 0);
				g_signal_connect (G_OBJECT (radio), "toggled",
					G_CALLBACK (inner::solution_toggled), &solution->apply);
			}

			gtk_box_pack_start (GTK_BOX (main_box), label, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (main_box), box, FALSE, TRUE, 0);
		}
#endif

		gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
		gtk_window_set_default_size (GTK_WINDOW (dialog), -1, 480);
		gtk_widget_show_all (dialog);

		bool apply = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_APPLY);
		gtk_widget_destroy (dialog);
		return apply;
	}

	virtual void packageModified (Ypp::Package *package)
	{
		m_package_selector->packageModified (package);
	}

	YGWIDGET_IMPL_COMMON
};

#else
#include "YPackageSelector.h"
class YGPackageSelector : public YPackageSelector, public YGWidget
{
public:
	YGPackageSelector (YWidget *parent, long mode)
		: YPackageSelector (NULL, mode),
		  YGWidget (this, parent, true, GTK_TYPE_EVENT_BOX, NULL)
	{
	}
	YGWIDGET_IMPL_COMMON
};
#endif

YPackageSelector *YGWidgetFactory::createPackageSelector (YWidget *parent, long mode)
{
/*	if (opt.youMode.value())
		return new YGPatchSelector (opt, YGWidget::get (parent));
	else*/
		return new YGPackageSelector (parent, mode);
}

