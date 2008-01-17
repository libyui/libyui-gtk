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
#include "ygtkscrolledwindow.h"
#include "ygtktogglebutton.h"
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

#define FILEMANAGER_EXEC "/usr/bin/nautilus -n --no-desktop"
inline void FILEMANAGER_LAUNCH (const char *path)
{ system ((std::string (FILEMANAGER_EXEC) + " " + path + " &").c_str()); }

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
			gtk_tree_view_set_search_column (GTK_TREE_VIEW (view), YGtkZyppModel::NAME_COLUMN);
			GtkTreeViewColumn *column;
			GtkCellRenderer *renderer;
			renderer = gtk_cell_renderer_pixbuf_new();
			column = gtk_tree_view_column_new_with_attributes ("", renderer,
				"pixbuf", YGtkZyppModel::ICON_COLUMN, NULL);
			gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
			gtk_tree_view_column_set_fixed_width (column, 38);
			gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
			renderer = gtk_cell_renderer_text_new();
			g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
			column = gtk_tree_view_column_new_with_attributes ("", renderer,
				"markup", YGtkZyppModel::NAME_DESCRIPTION_COLUMN, NULL);
			gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
			gtk_tree_view_column_set_fixed_width (column, 50 /* it will expand */);
			gtk_tree_view_column_set_expand (column, TRUE);
			gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
			gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (view), TRUE);

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

GtkWidget *m_bin;
GtkTreeModel *m_model;
View *m_view;

public:
	GtkWidget *getWidget()
	{ return m_bin; }

	PackagesView() : m_listener (NULL), m_model (NULL), m_view (NULL)
	{
		GtkWidget *buttons = gtk_vbox_new (FALSE, 0), *button;
		button = create_toggle_button (pkg_list_mode_xpm, "List view", NULL);
		gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, TRUE, 0);
		button = create_toggle_button (pkg_tiles_mode_xpm, "Tiles view", button);
		gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, TRUE, 0);
		gtk_widget_show_all (buttons);

		m_bin = ygtk_scrolled_window_new();
		ygtk_scrolled_window_set_corner_widget (YGTK_SCROLLED_WINDOW (m_bin), buttons);
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

		if (m_view)
			gtk_container_remove (GTK_CONTAINER (m_bin), m_view->m_widget);
		delete m_view;
		if (mode == LIST_MODE)
			m_view = new ListView (this);
		else
			m_view = new IconView (this);
		gtk_container_add (GTK_CONTAINER (m_bin), m_view->m_widget);
		if (m_model)
			m_view->setModel (m_model);

		packagesSelected (std::list <Ypp::Package *> ());
		normalCursor();
	}

	void setQuery (Ypp::Query *query)
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
	GtkWidget *create_toggle_button (const char **xpm, const char *tooltip, GtkWidget *member)
	{
		GSList *group = NULL;
		if (member)
			group = ygtk_toggle_button_get_group (YGTK_TOGGLE_BUTTON (member));
		GtkWidget *button = ygtk_toggle_button_new (group);
		gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
		gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
		if (!member)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

		// make it tiny
		GtkRcStyle *rcstyle = gtk_rc_style_new();
		rcstyle->xthickness = rcstyle->ythickness = 0;
		gtk_widget_modify_style (button, rcstyle);
		gtk_rc_style_unref (rcstyle),

		gtk_widget_set_tooltip_text (button, tooltip);
		g_signal_connect (G_OBJECT (button), "toggle-changed",
		                  G_CALLBACK (mode_toggled_cb), this);

		GtkWidget *image = createImageFromXPM (xpm);
		gtk_container_add (GTK_CONTAINER (button), image);
		return button;
	}

	static void mode_toggled_cb (GtkToggleButton *toggle, gint nb, PackagesView *pThis)
	{
		ViewMode mode = (nb == 0) ? LIST_MODE : ICON_MODE;
		pThis->setMode (mode);
	}
};

class ChangesPane : public Ypp::Pool::Listener
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
			gtk_widget_set_size_request (m_box, 140, -1);
			gtk_widget_show_all (m_box);
			modified (package);
			g_signal_connect (G_OBJECT (m_label), "style-set",
			                  G_CALLBACK (style_set_cb), NULL);
			g_signal_connect (G_OBJECT (m_button), "clicked",
			                  G_CALLBACK (undo_clicked_cb), package);
		}

		void modified (Ypp::Package *package)
		{
			std::string text;
			if (package->toInstall()) {
				if (package->isInstalled())
					text = "upgrade";
				else {
					if (package->type() == Ypp::Package::PATCH_TYPE)
						text = "patch";
					else
						text = "install";
				}
			}
			else
				text = "remove";
			text += " " + package->name();
			if (package->isAuto()) {
				text = "\t" + text;
				gtk_widget_hide (m_button);
			}
			else
				gtk_widget_show (m_button);
			gtk_label_set_text (GTK_LABEL (m_label), text.c_str());
		}

		static void undo_clicked_cb (GtkButton *button, Ypp::Package *package)
		{
			package->undo();
		}

		static void style_set_cb (GtkWidget *widget, GtkStyle *prev_style)
		{
			static bool set_style = false;
			if (set_style)
				return;
			set_style = true;
			GdkColor *color = &widget->style->fg [GTK_STATE_SELECTED];
			gtk_widget_modify_fg (widget, GTK_STATE_NORMAL, color);
			set_style = false;	
		}
	};

GtkWidget *m_box, *m_entries_box, *m_container;
Ypp::Pool *m_pool;
GList *m_entries;

public:
	GtkWidget *getWidget()
	{ return m_box; }

	ChangesPane (bool update_mode)
	: m_container (NULL), m_entries (NULL)
	{
		GtkWidget *heading = gtk_label_new (_("Changes:"));
		YGUtils::setWidgetFont (heading, PANGO_WEIGHT_ULTRABOLD, PANGO_SCALE_LARGE);
		gtk_misc_set_alignment (GTK_MISC (heading), 0, 0.5);
		g_signal_connect (G_OBJECT (heading), "style-set",
		                  G_CALLBACK (Entry::style_set_cb), NULL);
		m_entries_box = gtk_vbox_new (FALSE, 4);

		GtkWidget *port = gtk_viewport_new (NULL, NULL);
		gtk_viewport_set_shadow_type (GTK_VIEWPORT (port), GTK_SHADOW_NONE);
		gtk_container_add (GTK_CONTAINER (port), m_entries_box);

		GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
		                                GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_container_add (GTK_CONTAINER (scroll), port);

		GtkWidget *vbox = gtk_vbox_new (FALSE, 6);
		gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
		gtk_box_pack_start (GTK_BOX (vbox), heading, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);

		m_box = gtk_event_box_new();
		gtk_container_add (GTK_CONTAINER (m_box), vbox);
		g_signal_connect_after (G_OBJECT (m_box), "style-set",
		                        G_CALLBACK (style_set_cb), NULL);
		g_signal_connect_after (G_OBJECT (scroll), "style-set",
		                        G_CALLBACK (style_set_cb), NULL);
		g_signal_connect_after (G_OBJECT (port), "style-set",
		                        G_CALLBACK (style_set_cb), NULL);

		Ypp::Query *query = new Ypp::Query();
		query->setIsModified (true);
		if (update_mode) {
			query->addType (Ypp::Package::PATCH_TYPE);
			m_pool = new Ypp::Pool (query);
		}
		else
			m_pool = new Ypp::Pool (query, true);
		// initialize list -- there could already be packages modified
		for (Ypp::Pool::Iter it = m_pool->getFirst(); it; it = m_pool->getNext (it))
			ChangesPane::entryInserted (it, m_pool->get (it));
		m_pool->setListener (this);
	}

	~ChangesPane()
	{
		delete m_pool;
		for (GList *i = m_entries; i; i = i->next)
			delete (Entry *) i->data;
		g_list_free (m_entries);
	}

	void setContainer (GtkWidget *container)
	{
		m_container = container;
		if (!m_entries)
			gtk_widget_hide (m_container);
		// ugly: signal modified for all entries to allow them to hide undo buttons
		GList *i;
		Ypp::Pool::Iter it;
		for (it = m_pool->getFirst(), i = m_entries; it && i;
		     it = m_pool->getNext (it), i = i->next)
			((Entry *) i->data)->modified (m_pool->get (it));
	}

	virtual void entryInserted (Ypp::Pool::Iter iter, Ypp::Package *package)
	{
		Entry *entry = new Entry (package);
		gtk_box_pack_start (GTK_BOX (m_entries_box), entry->getWidget(), FALSE, TRUE, 0);
		int index = m_pool->getIndex (iter);
		m_entries = g_list_insert (m_entries, entry, index);
		if (m_container)
			gtk_widget_show (m_container);
	}

	virtual void entryDeleted  (Ypp::Pool::Iter iter, Ypp::Package *package)
	{
		int index = m_pool->getIndex (iter);
		GList *i = g_list_nth (m_entries, index);
		Entry *entry = (Entry *) i->data;
		gtk_container_remove (GTK_CONTAINER (m_entries_box), entry->getWidget());
		delete entry;
		m_entries = g_list_delete_link (m_entries, i);
		if (!m_entries)
			gtk_widget_hide (m_container);
	}

	virtual void entryChanged  (Ypp::Pool::Iter iter, Ypp::Package *package)
	{
		int index = m_pool->getIndex (iter);
		Entry *entry = (Entry *) g_list_nth_data (m_entries, index);
		entry->modified (package);
	}

	static void style_set_cb (GtkWidget *widget, GtkStyle *prev_style)
	{
		static bool set_style = false;
		if (set_style)
			return;
		set_style = true;
		GdkColor *color = &widget->style->bg [GTK_STATE_SELECTED];
		gtk_widget_modify_bg (widget, GTK_STATE_NORMAL, color);
		set_style = false;	
	}
};

// Maps icons to top package groups
struct CategoriesIconMap {
	const char *category, *icon;
};
static const CategoriesIconMap catIconMap[] = {
	{ "Amusements",    "package_games"           },
	{ "Games",         "package_games"           },
	{ "Development",   "package_development"     },
	{ "Libraries",     "package_development"     },
	{ "Documentation", "package_documentation"   },
	{ "Hardware",      "package_settings_peripherals" },
	{ "Applications",  "package_applications"    },
	{ "Productivity",  "package_applications"    },
	{ "System",        "package_system"          },
	{ "X11",           "package_system"          },
	{ "Multimedia",    "package_multimedia"      },
	{ "Video",         "package_multimedia"      },
	{ "Office",        "package_office_documentviewer" },
};
#define CAT_SIZE (sizeof (catIconMap)/sizeof (CategoriesIconMap))

#include "icons/pkg-installed.xpm"
#include "icons/pkg-installed-upgradable.xpm"
#include "icons/pkg-available.xpm"

class Filters
{
	class Collections
	{
		struct View
		{
			virtual GtkWidget *getWidget() = 0;
			virtual void writeQuery (Ypp::Query *query) = 0;

			Filters *m_filters;
			View (Filters *filters)
			: m_filters (filters)
			{}
		};

		struct Categories : public View
		{
			GtkWidget *m_scroll, *m_view;
		public:
			virtual GtkWidget *getWidget()
			{ return m_scroll; }

			Categories (Filters *filters, Ypp::Package::Type type)
			: View (filters)
			{
				GtkTreeViewColumn *column;
				GtkCellRenderer *renderer;

				m_view = gtk_tree_view_new();
				gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (m_view), FALSE);
				gtk_tree_view_set_search_column (GTK_TREE_VIEW (m_view), 0);
				renderer = gtk_cell_renderer_pixbuf_new();
				column = gtk_tree_view_column_new_with_attributes ("",
					renderer, "pixbuf", 2, NULL);
				gtk_tree_view_column_set_expand (column, FALSE);
				gtk_tree_view_append_column (GTK_TREE_VIEW (m_view), column);
				renderer = gtk_cell_renderer_text_new();
				column = gtk_tree_view_column_new_with_attributes ("",
					renderer, "text", 0, NULL);
				gtk_tree_view_column_set_expand (column, TRUE);
				gtk_tree_view_append_column (GTK_TREE_VIEW (m_view), column);
				if (type == Ypp::Package::PATCH_TYPE)
					gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (m_view), FALSE);
				else
					gtk_tree_view_set_expander_column (GTK_TREE_VIEW (m_view), column);

				GtkTreeSelection *selection = gtk_tree_view_get_selection (
					GTK_TREE_VIEW (m_view));
				gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
				g_signal_connect (G_OBJECT (selection), "changed",
						          G_CALLBACK (selection_cb), this);

				m_scroll = gtk_scrolled_window_new (NULL, NULL);
				gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (m_scroll),
						                             GTK_SHADOW_IN);
				gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (m_scroll),
						                        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
				gtk_widget_set_size_request (m_scroll, -1, 200);
				gtk_container_add (GTK_CONTAINER (m_scroll), m_view);

				build (type);
			}

			void build (Ypp::Package::Type type)
			{
				GtkTreeModel *model = NULL;

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
							const gchar *icon = 0;
							for (unsigned int i = 0; i < CAT_SIZE; i++)
								if (name == catIconMap[i].category) {
									icon = catIconMap[i].icon;
									break;
								}
							if (icon) {
								GtkIconTheme *icons = gtk_icon_theme_get_default();
								GdkPixbuf *pixbuf;
								pixbuf = gtk_icon_theme_load_icon (icons, icon, 22,
									GtkIconLookupFlags (0), NULL);
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

				GtkTreeSelection *selection = gtk_tree_view_get_selection (
					GTK_TREE_VIEW (m_view));
				g_signal_handlers_block_by_func (selection, (gpointer) selection_cb, this);

				gtk_tree_view_set_model (GTK_TREE_VIEW (m_view), model);
				if (model) {
					g_object_unref (G_OBJECT (model));

					/* we use gtk_tree_view_set_cursor(), rather than gtk_tree_selection_select_iter()
					   because that one is buggy in that when the user first interacts with the treeview,
					   a change signal is sent, even if he was just expanding one node... */
					GtkTreePath *path = gtk_tree_path_new_first();
					gtk_tree_view_set_cursor (GTK_TREE_VIEW (m_view), path, NULL, FALSE);
					gtk_tree_path_free (path);
				}

				g_signal_handlers_unblock_by_func (selection, (gpointer) selection_cb, this);
			}

			Ypp::Node *getActive()
			{
				GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (m_view));
				GtkTreeModel *model;
				GtkTreeIter iter;
				Ypp::Node *category = 0;
				if (gtk_tree_selection_get_selected (selection, &model, &iter))
					gtk_tree_model_get (model, &iter, 1, &category, -1);
				return category;
			}

			static void selection_cb (GtkTreeSelection *selection, View *pThis)
			{
				pThis->m_filters->signalChanged();
			}

			virtual void writeQuery (Ypp::Query *query)
			{
				Ypp::Node *node = getActive();
				if (node)
					query->addCategory (node);
			}
		};

		struct Pool : public View, public PackagesView::Listener
		{
			PackagesView *m_view;
			GtkWidget *m_box, *m_buttons_box;
		public:
			virtual GtkWidget *getWidget()
			{ return m_box; }

			Pool (Filters *filters, Ypp::Package::Type type)
			: View (filters)
			{
				m_view = new PackagesView();
				Ypp::Query *query = new Ypp::Query();
				query->addType (type); 
				m_view->setQuery (query);
				m_view->setListener (this);

				m_buttons_box = gtk_hbox_new (TRUE, 2);
				GtkWidget *image, *button;
				button = gtk_button_new_with_label (_("Install All"));
				image = gtk_image_new_from_stock (GTK_STOCK_SAVE, GTK_ICON_SIZE_BUTTON);
				gtk_button_set_image (GTK_BUTTON (button), image);
				g_signal_connect (G_OBJECT (button), "clicked",
				                  G_CALLBACK (install_cb), this);
				gtk_box_pack_start (GTK_BOX (m_buttons_box), button, TRUE, TRUE, 0);
				button = gtk_button_new_with_label (_("Remove All"));
				image = gtk_image_new_from_stock (GTK_STOCK_DELETE, GTK_ICON_SIZE_BUTTON);
				gtk_button_set_image (GTK_BUTTON (button), image);
				g_signal_connect (G_OBJECT (button), "clicked",
				                  G_CALLBACK (remove_cb), this);
				gtk_box_pack_start (GTK_BOX (m_buttons_box), button, TRUE, TRUE, 0);
				gtk_widget_set_sensitive (m_buttons_box, FALSE);

				m_box = gtk_vbox_new (FALSE, 4);
				gtk_box_pack_start (GTK_BOX (m_box), m_view->getWidget(), TRUE, TRUE, 0);
				gtk_box_pack_start (GTK_BOX (m_box), m_buttons_box, FALSE, TRUE, 0);
			}

			std::list <Ypp::Package *> m_selected;

			virtual void packagesSelected (const std::list <Ypp::Package *> &selection)
			{
				gtk_widget_set_sensitive (m_buttons_box, !selection.empty());
				m_selected = selection;
				m_filters->signalChanged();
			}

			virtual void writeQuery (Ypp::Query *query)
			{
				for (std::list <Ypp::Package *>::const_iterator it = m_selected.begin();
				     it != m_selected.end(); it++)
					query->addCollection (*it);
			}

			void doAll (bool install /*or remove*/)
			{
				// we just need to mark the collections themselves
				for (std::list <Ypp::Package *>::iterator it = m_selected.begin();
				     it != m_selected.end(); it++)
					install ? (*it)->install(0) : (*it)->remove();
			}

			static void install_cb (GtkButton *button, Pool *pThis)
			{ pThis->doAll (true); }
			static void remove_cb (GtkButton *button, Pool *pThis)
			{ pThis->doAll (false); }
		};

		View *m_view;
		GtkWidget *m_bin;
		Filters *m_filters;

		public:
		Collections (Filters *filters)
		: m_view (NULL), m_filters (filters)
		{
			m_bin = gtk_event_box_new();
		}

		~Collections()
		{
			delete m_view;
		}

		GtkWidget *getWidget()
		{ return m_bin; }

		void setType (Ypp::Package::Type type)
		{
			if (m_view)
				gtk_container_remove (GTK_CONTAINER (m_bin), m_view->getWidget());
			delete m_view;

			switch (type)
			{
				case Ypp::Package::PACKAGE_TYPE:
				case Ypp::Package::PATCH_TYPE:
					m_view = new Categories (m_filters, type);
					break;
				case Ypp::Package::PATTERN_TYPE:
				case Ypp::Package::LANGUAGE_TYPE:
					m_view = new Pool (m_filters, type);
					break;
				default:
					m_view = NULL;
					break;
			}

			if (m_view) {
				gtk_widget_show_all (m_view->getWidget());
				gtk_container_add (GTK_CONTAINER (m_bin), m_view->getWidget());
			}
		}

		void writeQuery (Ypp::Query *query)
		{
			if (m_view)
				m_view->writeQuery (query);
		}
	};

	class StatusButtons
	{
	public:
		enum Status { AVAILABLE, UPGRADE, INSTALLED, ALL };

	private:
		GtkWidget *m_box;
		Filters *m_filters;
		Status m_selected;
		bool m_updateMode;

	public:
		GtkWidget *getWidget()
		{ return m_box; }

		StatusButtons (Filters *filters, bool updateMode)
		: m_filters (filters), m_selected (AVAILABLE), m_updateMode (updateMode)
		{
			m_box = gtk_hbox_new (FALSE, 6);

			GtkWidget *button;
			GSList *group;
			button = createButton ("Available", pkg_available_xpm, NULL);
			group = ygtk_toggle_button_get_group (YGTK_TOGGLE_BUTTON (button));
			gtk_box_pack_start (GTK_BOX (m_box), button, TRUE, TRUE, 0);
			if (!updateMode) {
				button = createButton ("Upgrades", pkg_installed_upgradable_xpm, group);
				gtk_box_pack_start (GTK_BOX (m_box), button, TRUE, TRUE, 0);
			}
			button = createButton ("Installed", pkg_installed_xpm, group);
			gtk_box_pack_start (GTK_BOX (m_box), button, TRUE, TRUE, 0);
			button = createButton ("All", 0, group);
			gtk_box_pack_start (GTK_BOX (m_box), button, FALSE, TRUE, 0);
		}

		Status getActive()
		{
			return m_selected;
		}

		GtkWidget *createButton (const char *label, const char **xpm, GSList *group)
		{
			GtkWidget *button = ygtk_toggle_button_new (group);
			GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
			if (xpm)
				gtk_box_pack_start (GTK_BOX (hbox), createImageFromXPM (xpm), FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (label), TRUE, TRUE, 0);
			gtk_container_add (GTK_CONTAINER (button), hbox);
			if (!group)
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
			g_signal_connect (G_OBJECT (button), "toggle-changed",
			                  G_CALLBACK (status_toggled_cb), this);
			return button;
		}

		static void status_toggled_cb (GtkToggleButton *toggle, gint nb, StatusButtons *pThis)
		{
			pThis->m_selected = (Status) nb;
			if (pThis->m_updateMode && nb >= 1)
				pThis->m_selected = (Status) (nb+1);
			pThis->m_filters->signalChanged();
		}
	};

public:
	struct Listener {
		virtual void doQuery (Ypp::Query *query) = 0;
	};
	void setListener (Listener *listener)
	{ m_listener = listener; signalChanged(); }

private:
	Collections *m_collection;
	StatusButtons *m_statuses;
	GtkWidget *m_name, *m_repos, *m_type;
	Listener *m_listener;
	guint timeout_id;
	int m_selectedType;
	bool m_updateMode;

public:
	GtkWidget *getCollectionWidget() { return m_collection->getWidget(); }
	GtkWidget *getStatusesWidget()   { return m_statuses->getWidget(); }
	GtkWidget *getNameWidget()       { return m_name; }
	GtkWidget *getReposWidget()      { return m_repos; }
	GtkWidget *getTypeWidget()       { return m_type; }

	Filters (bool update_mode)
	: m_listener (NULL), timeout_id (0), m_selectedType (-1), m_updateMode (update_mode)
	{
		m_collection = new Collections (this);
		m_statuses = new StatusButtons (this, update_mode);

		m_name = ygtk_find_entry_new();
		gtk_widget_set_tooltip_markup (m_name,
			_("<b>Package search:</b> Use spaces to separate your keywords. They "
			"will be matched against RPM <i>name</i>, <i>summary</i> and "
			"<i>provides</i> attributes.\n(e.g.: \"yast dhcp\" will return yast's "
			"dhcpd tool)"));
		g_signal_connect (G_OBJECT (m_name), "changed",
		                  G_CALLBACK (entry_changed_cb), this);

		m_repos = gtk_combo_box_new_text();
		gtk_widget_set_tooltip_markup (m_repos,
			_("<b>Package repositories:</b> Limits the query to one repository.\n"
			"Repositories may be added or managed through YaST control center."));
		gtk_combo_box_append_text (GTK_COMBO_BOX (m_repos), _("All Repositories"));
		for (int i = 0; Ypp::get()->getRepository (i); i++) {
			const Ypp::Repository *repo = Ypp::get()->getRepository (i);
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_repos), repo->name.c_str());
		}
		gtk_combo_box_set_active (GTK_COMBO_BOX (m_repos), 0);
		g_signal_connect (G_OBJECT (m_repos), "changed",
		                  G_CALLBACK (combo_changed_cb), this);

		m_type = gtk_combo_box_new_text();
		if (update_mode)
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_type), _("Patches"));
		else {
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_type), _("Categories"));
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_type), _("Patterns"));
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_type), _("Languages"));
		}
		gtk_combo_box_set_active (GTK_COMBO_BOX (m_type), 0);
		g_signal_connect_after (G_OBJECT (m_type), "changed",
		                        G_CALLBACK (combo_changed_cb), this);
	}

	~Filters()
	{
		if (timeout_id)
			g_source_remove (timeout_id);
	}

public:
	int selectedRepo()
	{
		int repo = gtk_combo_box_get_active (GTK_COMBO_BOX (m_repos))-1;
		return repo;
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
		if (m_updateMode)
			type = Ypp::Package::PATCH_TYPE;
		else
			type = (Ypp::Package::Type) gtk_combo_box_get_active (GTK_COMBO_BOX (m_type));

		// adjust interface
		if (type != m_selectedType) {
			m_collection->setType (type);
			m_selectedType = type;
		}

		// create query
		Ypp::Query *query = new Ypp::Query();
		if (type == Ypp::Package::PATCH_TYPE)
			query->addType (Ypp::Package::PATCH_TYPE);
		else
			query->addType (Ypp::Package::PACKAGE_TYPE);

		const char *name = gtk_entry_get_text (GTK_ENTRY (m_name));
		if (*name)
			query->addNames (name, ' ');

		switch (m_statuses->getActive())
		{
			case StatusButtons::AVAILABLE:
				if (m_updateMode)
					// special pane for patches upgrades makes little sense, so
					// we instead list them together with availables
					query->setHasUpgrade (true);
				query->setIsInstalled (false);
				break;
			case StatusButtons::UPGRADE:
				query->setHasUpgrade (true);
				break;
			case StatusButtons::INSTALLED:
				query->setIsInstalled (true);
				break;
			case StatusButtons::ALL:
				break;
		}

		m_collection->writeQuery (query);

		if (selectedRepo() >= 0)
			query->addRepository (selectedRepo());

		m_listener->doQuery (query);
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
		timeout_id = g_timeout_add (250, inner::timeout_cb, this);
	}
};

#include "icons/pkg-locked.xpm"
#include "icons/pkg-unlocked.xpm"

class PackageControl
{
GtkWidget *m_widget, *m_install_button, *m_remove_button, *m_installed_version,
          *m_available_versions, *m_undo_button, *m_lock_button, *m_locked_image,
          *m_unlocked_image, *m_package_image;

public:
std::list <Ypp::Package *> m_packages;  // we keep a copy to test against modified...
Filters *m_filters;  // used to filter repo versions...

	GtkWidget *getWidget()
	{ return m_widget; }

	PackageControl (Filters *filters)
	: m_filters (filters)
	{
		// installed
		m_install_button = createButton ("", GTK_STOCK_SAVE);
		g_signal_connect (G_OBJECT (m_install_button), "clicked",
		                  G_CALLBACK (install_clicked_cb), this);

		m_installed_version = gtk_label_new ("");
		gtk_misc_set_alignment (GTK_MISC (m_installed_version), 0, 0.5);

		// available
		m_remove_button = createButton (_("_Remove"), GTK_STOCK_DELETE);
		g_signal_connect (G_OBJECT (m_remove_button), "clicked",
		                  G_CALLBACK (remove_clicked_cb), this);

		m_available_versions = gtk_combo_box_new_text();
		g_signal_connect (G_OBJECT (m_available_versions), "changed",
		                  G_CALLBACK (version_changed_cb), this);

		// lock
		m_lock_button = gtk_toggle_button_new();
		gtk_widget_set_tooltip_markup (m_lock_button,
			_("<b>Package lock:</b> prevents the package status from being modified by "
			"the solver (that is, it won't honour dependencies or collections ties.)"));
		g_signal_connect (G_OBJECT (m_lock_button), "toggled",
		                  G_CALLBACK (locked_toggled_cb), this);
		m_locked_image = createImageFromXPM (pkg_locked_xpm);
		m_unlocked_image = createImageFromXPM (pkg_unlocked_xpm);
		g_object_ref_sink (G_OBJECT (m_locked_image));
		g_object_ref_sink (G_OBJECT (m_unlocked_image));

		m_package_image = gtk_image_new();

		m_undo_button = gtk_button_new_from_stock (GTK_STOCK_UNDO);
		g_signal_connect (G_OBJECT (m_undo_button), "clicked",
		                  G_CALLBACK (undo_clicked_cb), this);

		// layout
		GtkWidget *table;
		table = gtk_table_new (3, 2, FALSE);
		gtk_table_set_row_spacings (GTK_TABLE (table), 6);
		gtk_table_set_col_spacings (GTK_TABLE (table), 6);
		// versions
		gtk_table_attach (GTK_TABLE (table), m_installed_version, 1, 2, 0, 1,
		                  GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach (GTK_TABLE (table), m_available_versions, 1, 2, 1, 2,
		                  GTK_FILL, GTK_FILL, 0, 0);
		// buttons
		gtk_table_attach (GTK_TABLE (table), m_remove_button, 2, 3, 0, 1,
		                  GTK_FILL, GtkAttachOptions (0), 0, 0);
		gtk_table_attach (GTK_TABLE (table), m_install_button, 2, 3, 1, 2,
		                  GTK_FILL, GtkAttachOptions (0), 0, 0);
		// labels
		GtkWidget *label;
		label = gtk_label_new (_("Installed: "));
		YGUtils::setWidgetFont (label, PANGO_WEIGHT_BOLD, PANGO_SCALE_MEDIUM);
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
		gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
		label = gtk_label_new (_("Available: "));
		YGUtils::setWidgetFont (label, PANGO_WEIGHT_BOLD, PANGO_SCALE_MEDIUM);
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
		gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

		GtkWidget *lock_align = gtk_alignment_new (0.5, 0.5, 0, 0);
		gtk_container_add (GTK_CONTAINER (lock_align), m_lock_button);

		GtkWidget *undo_align = gtk_alignment_new (0.5, 0.5, 0, 0);
		gtk_container_add (GTK_CONTAINER (undo_align), m_undo_button);

		GtkWidget *hbox = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), undo_align, FALSE, TRUE, 12);
		gtk_box_pack_start (GTK_BOX (hbox), lock_align, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new(""), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), m_package_image, FALSE, TRUE, 0);

		m_widget = gtk_alignment_new (0, 0.5, 1, 0);
		gtk_container_set_border_width (GTK_CONTAINER (m_widget), 6);
		gtk_container_add (GTK_CONTAINER (m_widget), hbox);
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
		m_packages = packages;
		if (packages.empty())
			return;
		Ypp::Package *single_package = packages.size() == 1 ? packages.front() : NULL;

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
			if ((*it)->isModified()) {
				// if modified, can't be locked or unlocked
				allLocked = allUnlocked = false;
			}
			else
				allModified = false;
			if ((*it)->isLocked())
				allUnlocked = false;
			else
				allLocked = false;
		}

		if (allInstalled) {
			gtk_widget_show (m_remove_button);
			if (single_package)
				gtk_label_set_text (GTK_LABEL (m_installed_version),
				                    single_package->getInstalledVersion()->number.c_str());
			else
				gtk_label_set_text (GTK_LABEL (m_installed_version), "(several)");
		}
		else {
			gtk_widget_hide (m_remove_button);
			gtk_label_set_text (GTK_LABEL (m_installed_version), "--");
		}

		GtkTreeModel *model = gtk_combo_box_get_model (
			GTK_COMBO_BOX (m_available_versions));
		gtk_list_store_clear (GTK_LIST_STORE (model));
		gtk_widget_set_sensitive (m_available_versions, FALSE);
		gtk_widget_show (m_install_button);
		if (single_package) {
			if (single_package->getAvailableVersion (0)) {
				gtk_widget_set_sensitive (m_available_versions, TRUE);
				gtk_widget_show (m_install_button);
				int selectedRepo = m_filters->selectedRepo();
				for (int i = 0; single_package->getAvailableVersion (i); i++) {
					const Ypp::Package::Version *version = single_package->getAvailableVersion (i);
					if (selectedRepo >= 0 && version->repo != selectedRepo)
						continue;
					string text = version->number + "\n";
					text += "(" + Ypp::get()->getRepository (version->repo)->name + ")";
					gtk_combo_box_append_text (GTK_COMBO_BOX (m_available_versions), text.c_str());
				}
				gtk_combo_box_set_active (GTK_COMBO_BOX (m_available_versions), 0);
			}
			else
				gtk_widget_hide (m_install_button);
		}
		else {
			if (allUpgradable) {
				gtk_combo_box_append_text (GTK_COMBO_BOX (m_available_versions), "(upgrades)");
				gtk_combo_box_set_active (GTK_COMBO_BOX (m_available_versions), 0);
				gtk_button_set_label (GTK_BUTTON (m_install_button), _("Upgrade"));
			}
			else if (allNotInstalled) {
				gtk_combo_box_append_text (GTK_COMBO_BOX (m_available_versions), "(several)");
				gtk_combo_box_set_active (GTK_COMBO_BOX (m_available_versions), 0);
				gtk_button_set_label (GTK_BUTTON (m_install_button), _("Install"));
			}
			else
				gtk_widget_hide (m_install_button);
		}

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

		if (allModified)
			gtk_widget_show (m_undo_button);
		else
			gtk_widget_hide (m_undo_button);

		gtk_image_clear (GTK_IMAGE (m_package_image));
		if (single_package) {
			GtkIconTheme *icons = gtk_icon_theme_get_default();
			GdkPixbuf *pixbuf = gtk_icon_theme_load_icon (icons,
				single_package->name().c_str(), 32, GtkIconLookupFlags (0), NULL);
			if (pixbuf) {
				gtk_image_set_from_pixbuf (GTK_IMAGE (m_package_image), pixbuf);
				g_object_unref (G_OBJECT (pixbuf));
			}
		}
	}

private:
	static void install_clicked_cb (GtkButton *button, PackageControl *pThis)
	{
		busyCursor();
		Ypp::get()->startTransactions();
		for (std::list <Ypp::Package *>::iterator it = pThis->m_packages.begin();
		     it != pThis->m_packages.end(); it++) {
			int version;
			if (GTK_WIDGET_VISIBLE (pThis->m_available_versions))
				version = gtk_combo_box_get_active (GTK_COMBO_BOX (
					pThis->m_available_versions));
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

	static void locked_toggled_cb (GtkToggleButton *button, PackageControl *pThis)
	{
		bool lock = gtk_toggle_button_get_active (button);
		busyCursor();
		Ypp::get()->startTransactions();
		for (std::list <Ypp::Package *>::iterator it = pThis->m_packages.begin();
		     it != pThis->m_packages.end(); it++)
			(*it)->lock (lock);
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

	static void version_changed_cb (GtkComboBox *combo, PackageControl *pThis)
	{
		if (pThis->m_packages.size() == 1) {
			Ypp::Package *package = pThis->m_packages.front();
			int nb = gtk_combo_box_get_active (GTK_COMBO_BOX (pThis->m_available_versions));
			if (nb == -1) return;

			const Ypp::Package::Version *version;
			version = package->getAvailableVersion (nb);
			assert (version != NULL);

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

class PackageDetails
{
GtkWidget *m_widget, *m_description, *m_filelist, *m_changelog, *m_authors;
PackageControl *m_control;

public:
	GtkWidget *getWidget()
	{ return m_widget; }

	PackageDetails (Filters *filters, bool update_mode)
	{
		m_control = new PackageControl (filters);
		m_widget = gtk_notebook_new();
		gtk_notebook_set_tab_pos (GTK_NOTEBOOK (m_widget), GTK_POS_BOTTOM);
		addPage (_("Status"), m_control->getWidget());
		addPage (_("Description"), createHtmlWidget (&m_description));
		if (update_mode)
			m_filelist = m_changelog = m_authors = NULL;
		else {
			addPage (_("File List"), createHtmlWidget (&m_filelist));
			addPage (_("ChangeLog"), createHtmlWidget (&m_changelog));
			addPage (_("Authors"), createHtmlWidget (&m_authors));
			ygtk_html_wrap_connect_link_clicked (m_filelist,
				G_CALLBACK (path_pressed_cb), NULL);
		}
	}

	void setPackages (std::list <Ypp::Package *> packages)
	{
		if (packages.empty()) {
			gtk_widget_hide (m_widget);
		}
		else {
			gtk_widget_show (m_widget);
			m_control->setPackages (packages);
			Ypp::Package *package = packages.front();
			if (package) {
				setText (m_description, package->description(), false);
				setText (m_filelist, package->filelist(), true);
				setText (m_changelog, package->changelog(), true);
				setText (m_authors, package->authors(), false);
				if (!GTK_WIDGET_VISIBLE (m_widget)) {
					gtk_notebook_set_current_page (GTK_NOTEBOOK (m_widget), 0);
					gtk_widget_show (m_widget);
				}
			}
		}
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

	void setText (GtkWidget *rtext, const std::string &text, bool only_availables)
	{
		if (!rtext) return;
		const char *str = text.c_str();
		if (text.empty()) {
			if (only_availables)
				str = _("<i>(only available for installed packages)</i>");
			else
				str = "--";
		}
		ygtk_html_wrap_set_text (rtext, str);
		ygtk_html_wrap_scroll (rtext, TRUE);
	}
};

#if 0
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
#else
// disabled
struct DiskView
{
GtkWidget *m_empty;
GtkWidget *getWidget() { return m_empty; }
DiskView() { m_empty = gtk_event_box_new(); }
};
#endif

class PackageSelector : public Filters::Listener, public PackagesView::Listener
{
PackagesView *m_packages;
Filters *m_filters;
PackageDetails *m_details;
DiskView *m_disk;
GtkWidget *m_box;
ChangesPane *m_changes;

public:
	GtkWidget *getWidget()
	{ return m_box; }

	PackageSelector (bool update_mode)
	{
		m_packages = new PackagesView();
		m_filters = new Filters (update_mode);
		m_details = new PackageDetails (m_filters, update_mode);
		m_disk = new DiskView();
		m_changes = new ChangesPane (update_mode);
		m_packages->setListener (this);
		m_filters->setListener (this);

		GtkWidget *filter_box = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (filter_box), gtk_label_new (_("Filters:")), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (filter_box), m_filters->getNameWidget(), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (filter_box), m_filters->getReposWidget(), FALSE, TRUE, 0);

		GtkWidget *categories_box = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (categories_box), m_filters->getCollectionWidget(),
		                    TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (categories_box), m_filters->getTypeWidget(), FALSE, TRUE, 0);

		GtkWidget *packages_box = gtk_hpaned_new();
		gtk_paned_pack1 (GTK_PANED (packages_box), categories_box, TRUE, TRUE);
		gtk_paned_pack2 (GTK_PANED (packages_box), m_packages->getWidget(), TRUE, FALSE);
		gtk_paned_set_position (GTK_PANED (packages_box), 180);

		GtkWidget *details_box = gtk_vpaned_new();
		gtk_paned_pack1 (GTK_PANED (details_box), packages_box, TRUE, FALSE);
		gtk_paned_pack2 (GTK_PANED (details_box), m_details->getWidget(), TRUE, FALSE);
		gtk_paned_set_position (GTK_PANED (details_box), 30000 /* minimal size */);

		GtkWidget *vbox = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (vbox), m_filters->getStatusesWidget(), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), filter_box, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), details_box, TRUE, TRUE, 0);
		gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

		GtkWidget *changes_box = gtk_vbox_new (FALSE, 0);
		gtk_box_pack_start (GTK_BOX (changes_box), m_changes->getWidget(), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (changes_box), m_disk->getWidget(), FALSE, TRUE, 0);

		m_box = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (m_box), vbox, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_box), changes_box, FALSE, TRUE, 0);

		gtk_widget_show_all (m_box);
		m_changes->setContainer (changes_box);
		m_details->setPackages (std::list <Ypp::Package *> ());
	}

	~PackageSelector()
	{
		delete m_packages;
		delete m_details;
		delete m_filters;
		delete m_disk;
		delete m_changes;
	}

	virtual void doQuery (Ypp::Query *query)
	{
		m_packages->setQuery (query);
	}

	virtual void packagesSelected (const std::list <Ypp::Package *> &packages)
	{
		m_details->setPackages (packages);
	}

	void packageModified (Ypp::Package *package)
	{
		m_details->packageModified (package);
	}
};

#include "YPackageSelector.h"
#include "pkg-selector-help.h"

class YGPackageSelector : public YPackageSelector, public YGWidget, public Ypp::Interface
{
PackageSelector *m_package_selector;

public:
	YGPackageSelector (YWidget *parent, long mode)
		: YPackageSelector (NULL, mode),
		  YGWidget (this, parent, true, YGTK_TYPE_WIZARD, NULL)
	{
		setBorder (0);
		YGTK_WIZARD (getWidget())->child_border_width = 0;

        YGDialog *dialog = YGDialog::currentDialog();
        dialog->setCloseCallback (confirm_cb, this);

        GtkWindow *window = YGDialog::currentWindow();
		gtk_window_resize (window,
			MAX (700, GTK_WIDGET (window)->allocation.width),
			MAX (680, GTK_WIDGET (window)->allocation.height));

		YGtkWizard *wizard = YGTK_WIZARD (getWidget());
		ygtk_wizard_set_header_icon (wizard, window,
			THEMEDIR "/icons/22x22/apps/yast-software.png");
		ygtk_wizard_set_header_text (wizard, window,
			onlineUpdateMode() ? _("Patch Selector") : _("Package Selector"));
		ygtk_wizard_set_help_text (wizard, onlineUpdateMode() ? patch_help : pkg_help);

		ygtk_wizard_set_abort_button_label (wizard, _("_Cancel"));
		ygtk_wizard_set_abort_button_str_id (wizard, "cancel");
		ygtk_wizard_set_back_button_label (wizard, "");
		ygtk_wizard_set_next_button_label (wizard, _("_Apply"));
		ygtk_wizard_set_next_button_str_id (wizard, "accept");
		g_signal_connect (G_OBJECT (getWidget()), "action-triggered",
		                  G_CALLBACK (wizard_action_cb), this);

		m_package_selector = new PackageSelector (onlineUpdateMode());
		gtk_container_add (GTK_CONTAINER (wizard), m_package_selector->getWidget());

		Ypp::get()->setInterface (this);
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
		const gchar *action = (gchar *) id;

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
			}
			static void truncate (std::string &str, unsigned int size)
			{
				if (str.size() > size) {
					str.erase (size-3);
					str += "...";
				}
			}
		};

		// model
		GtkTreeStore *store = gtk_tree_store_new (6, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
			G_TYPE_STRING, G_TYPE_INT, G_TYPE_POINTER, G_TYPE_STRING);
		for (std::list <Ypp::Problem *>::iterator it = problems.begin();
		     it != problems.end(); it++) {
			inner::truncate ((*it)->details, 800);
			GtkTreeIter problem_iter;
			gtk_tree_store_append (store, &problem_iter, NULL);
			gtk_tree_store_set (store, &problem_iter, SHOW_TOGGLE_COL, FALSE,
				TEXT_COL, (*it)->description.c_str(),
				WEIGHT_TEXT_COL, PANGO_WEIGHT_BOLD, APPLY_PTR_COL, NULL,
				TOOLTIP_TEXT_COL, (*it)->details.c_str(), -1);

			for (int i = 0; (*it)->getSolution (i); i++) {
				Ypp::Problem::Solution *solution = (*it)->getSolution (i);
				inner::truncate (solution->details, 800);
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
		// we should not connect the actual toggle button, as we toggle on row press
		g_signal_connect (G_OBJECT (view), "cursor-changed",
			G_CALLBACK (inner::cursor_changed_cb), store);
		column = gtk_tree_view_column_new_with_attributes ("", renderer,
			"visible", SHOW_TOGGLE_COL, "active", ACTIVE_TOGGLE_COL, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
		renderer = gtk_cell_renderer_text_new();
		g_object_set (G_OBJECT (renderer), "wrap-width", 400, NULL);
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
	return new YGPackageSelector (parent, mode);
}

