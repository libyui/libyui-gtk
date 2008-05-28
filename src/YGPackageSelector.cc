/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/*
  Textdomain "yast2-gtk"
 */

#include <config.h>
#include <string.h>
#include "YGUI.h"
#include "YGUtils.h"
#include "YGi18n.h"
#include "YGDialog.h"

#if 1
#include "ygtkwizard.h"
#include "ygtkfindentry.h"
#include "ygtkmenubutton.h"
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

const char *lock_tooltip =
	"<b>Package lock:</b> prevents the package status from being modified by "
    "the solver (that is, it won't honour dependencies or collections ties.)";

struct PkgList
{
	bool empty() const
	{ return packages.empty(); }
	bool single() const
	{ return packages.size() == 1; }
	Ypp::Package *front() const
	{ return packages.front(); }

	bool installed() const
	{ init(); return _allInstalled;  }
	bool notInstalled() const
	{ init(); return _allNotInstalled; }
	bool upgradable() const
	{ init(); return _allUpgradable; }
	bool modified() const
	{ init(); return _allModified;   }
	bool locked() const
	{ init(); return _allLocked;     }
	bool unlocked() const
	{ init(); return _allUnlocked;   }

	void install()  // or upgrade
	{
		busyCursor();
		Ypp::get()->startTransactions();
		for (std::list <Ypp::Package *>::iterator it = packages.begin();
		     it != packages.end(); it++)
			(*it)->install (0);
		Ypp::get()->finishTransactions();
		normalCursor();
	}

	void remove()
	{
		busyCursor();
		Ypp::get()->startTransactions();
		for (std::list <Ypp::Package *>::iterator it = packages.begin();
		     it != packages.end(); it++)
			(*it)->remove();
		Ypp::get()->finishTransactions();
		normalCursor();
	}

	void lock (bool toLock)
	{
		busyCursor();
		Ypp::get()->startTransactions();
		for (std::list <Ypp::Package *>::iterator it = packages.begin();
		     it != packages.end(); it++)
			(*it)->lock (toLock);
		Ypp::get()->finishTransactions();
		normalCursor();
	}

	void undo()
	{
		busyCursor();
		Ypp::get()->startTransactions();
		for (std::list <Ypp::Package *>::iterator it = packages.begin();
		     it != packages.end(); it++)
			(*it)->undo();
		Ypp::get()->finishTransactions();
		normalCursor();
	}

	void push (Ypp::Package *package)
	{ packages.push_back (package); }

	bool contains (const Ypp::Package *package) const
	{
		const_iterator it;
		for (it = packages.begin(); it != packages.end(); it++)
			if (*it == package)
				return true;
		return false;
	}

	PkgList() : inited (0)
	{}

	void clearCache()
	{ inited = 0; }

	typedef std::list <Ypp::Package *>::const_iterator const_iterator;
	const_iterator begin() const
	{ return packages.begin(); }
	const_iterator end() const
	{ return packages.end(); }

private:
	void init() const
	{ PkgList *packages = const_cast <PkgList *> (this); packages->_init(); }
	void _init()
	{
		if (inited) return; inited = 1;
		if (packages.empty())
			_allInstalled = _allNotInstalled = _allUpgradable = _allModified =
				_allLocked = _allUnlocked = false;
		else {
			_allInstalled = _allNotInstalled = _allUpgradable = _allModified =
				_allLocked = _allUnlocked = true;
			for (std::list <Ypp::Package *>::const_iterator it = packages.begin();
				 it != packages.end(); it++) {
				if (!(*it)->isInstalled()) {
					_allInstalled = false;
					_allUpgradable = false;
				 }
				 else {
				 	_allNotInstalled = false;
				 	const Ypp::Package::Version *version = (*it)->getAvailableVersion(0);
				 	if (!version || version->cmp <= 0)
				 		_allUpgradable = false;
				 }
				if ((*it)->isModified()) {
					// if modified, can't be locked or unlocked
					_allLocked = _allUnlocked = false;
				}
				else
					_allModified = false;
				if ((*it)->isLocked())
					_allUnlocked = false;
				else
					_allLocked = false;
			}
		}
	}

	std::list <Ypp::Package *> packages;
	guint inited : 2, _allInstalled : 2, _allNotInstalled : 2, _allUpgradable : 2,
	      _allModified : 2, _allLocked : 2, _allUnlocked : 2;
};

#include "icons/pkg-list-mode.xpm"
#include "icons/pkg-tiles-mode.xpm"
#include "icons/pkg-locked.xpm"
#include "icons/pkg-unlocked.xpm"

class PackagesView
{
public:
	struct Listener {
		virtual void packagesSelected (const PkgList &packages) = 0;
	};
	void setListener (Listener *listener)
	{ m_listener = listener; }

private:
Listener *m_listener;

	inline bool inited()
	{ return GTK_WIDGET_REALIZED (m_bin); }
	void packagesSelected (const PkgList &packages)
	{
		if (m_listener && inited()) {
			busyCursor();
			m_listener->packagesSelected (packages);
			normalCursor();
		}
	}

	struct View
	{
		PackagesView *m_parent;
		GtkWidget *m_widget, *m_popup_hack;
		View (PackagesView *parent) : m_parent (parent), m_popup_hack (NULL)
		{}

		virtual ~View()
		{
			if (m_popup_hack) gtk_widget_destroy (m_popup_hack);
		}

		virtual void setModel (GtkTreeModel *model) = 0;
		virtual GList *getSelectedPaths (GtkTreeModel **model) = 0;
		virtual void selectAll() = 0;
		virtual void unselectAll() = 0;
		virtual void ensureVisible (GtkTreePath *path) = 0;

		virtual int countSelected()
		{
			int count = 0;
			GtkTreeModel *model;
			GList *paths = getSelectedPaths (&model);
			for (GList *i = paths; i; i = i->next) {
				GtkTreePath *path = (GtkTreePath *) i->data;
				gtk_tree_path_free (path);
				count++;
			}
			g_list_free (paths);
			return count;
		}

		PkgList getSelected()
		{
			GtkTreeModel *model;
			GList *paths = getSelectedPaths (&model);
			PkgList packages;
			for (GList *i = paths; i; i = i->next) {
				Ypp::Package *package;
				GtkTreePath *path = (GtkTreePath *) i->data;
				GtkTreeIter iter;
				gtk_tree_model_get_iter (model, &iter, path);
				gtk_tree_model_get (model, &iter, YGtkZyppModel::PTR_COLUMN, &package, -1);
				gtk_tree_path_free (path);

				if (package)
					packages.push (package);
			}
			g_list_free (paths);
			return packages;
		}

		void signalSelected()
		{
			PkgList packages = getSelected();
			m_parent->packagesSelected (packages);
		}

		void signalPopup (int button, int event_time)
		{
			// GtkMenu emits "deactivate" before Items notifications, so there isn't
			// a better way to de-allocate the popup
			if (m_popup_hack) gtk_widget_destroy (m_popup_hack);
			GtkWidget *menu = m_popup_hack = gtk_menu_new();

			struct inner {
				static void appendItem (GtkWidget *menu, const char *label,
					const char *tooltip, const char *stock_icon, const char **xpm_icon,
					bool sensitive,
					void (& callback) (GtkMenuItem *item, View *pThis), View *pThis)
				{
					GtkWidget *item;
					if (stock_icon || xpm_icon) {
						if (label) {
							item = gtk_image_menu_item_new_with_mnemonic (label);
							GtkWidget *icon;
							if (stock_icon)
								icon = gtk_image_new_from_stock (stock_icon, GTK_ICON_SIZE_MENU);
							else
								icon = createImageFromXPM (xpm_icon);
							gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), icon);
						}
						else
							item = gtk_image_menu_item_new_from_stock (stock_icon, NULL);
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
				static void install_cb (GtkMenuItem *item, View *pThis)
				{
					PkgList packages = pThis->getSelected();
					packages.install();
				}
				static void remove_cb (GtkMenuItem *item, View *pThis)
				{
					PkgList packages = pThis->getSelected();
					packages.remove();
				}
				static void undo_cb (GtkMenuItem *item, View *pThis)
				{
					PkgList packages = pThis->getSelected();
					packages.undo();
				}
				static void lock_cb (GtkMenuItem *item, View *pThis)
				{
					PkgList packages = pThis->getSelected();
					packages.lock (true);
				}
				static void unlock_cb (GtkMenuItem *item, View *pThis)
				{
					PkgList packages = pThis->getSelected();
					packages.lock (false);
				}
				static void select_all_cb (GtkMenuItem *item, View *pThis)
				{
					pThis->selectAll();
				}
			};

			PkgList packages = getSelected();
			bool empty = true, unlocked = packages.unlocked();
			if (packages.notInstalled())
				inner::appendItem (menu, _("_Install"), 0, GTK_STOCK_DELETE, 0,
				                   unlocked, inner::install_cb, this), empty = false;
			if (packages.upgradable())
				inner::appendItem (menu, _("_Upgrade"), 0, GTK_STOCK_GOTO_TOP, 0,
				                   unlocked, inner::install_cb, this), empty = false;
			if (packages.installed())
				inner::appendItem (menu, _("_Remove"), 0, GTK_STOCK_SAVE, 0,
				                   unlocked, inner::remove_cb, this), empty = false;
			if (packages.modified())
				inner::appendItem (menu, _("_Undo"), 0, GTK_STOCK_UNDO, 0,
				                   true, inner::undo_cb, this), empty = false;
			if (packages.locked())
				inner::appendItem (menu, _("_Unlock"), _(lock_tooltip), 0, pkg_unlocked_xpm,
				                   true, inner::unlock_cb, this), empty = false;
			if (unlocked)
				inner::appendItem (menu, _("_Lock"), _(lock_tooltip), 0, pkg_locked_xpm,
				                   true, inner::lock_cb, this), empty = false;
			if (!empty)
				gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());
			inner::appendItem (menu, 0, 0, GTK_STOCK_SELECT_ALL, 0,
			                   true, inner::select_all_cb, this);

			gtk_widget_show_all (menu);
			gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,  button, event_time);
		}

		static gboolean popup_key_cb (GtkWidget *widget, View *pThis)
		{ pThis->signalPopup (0, gtk_get_current_event_time()); return TRUE; }

		static void size_allocated_cb (GtkWidget *widget, GtkAllocation *alloc, View *pThis)
		{
			GList *paths = pThis->getSelectedPaths (NULL);
			if (paths && !paths->next /* single selection */)
				pThis->ensureVisible ((GtkTreePath *) paths->data);
			g_list_foreach (paths, (GFunc) gtk_tree_path_free, 0);
			g_list_free (paths);
		}
	};
	struct ListView : public View
	{
		bool m_isTree;
		ListView (bool isTree, bool showTooltips, PackagesView *parent)
		: View (parent), m_isTree (isTree)
		{
			GtkTreeView *view = GTK_TREE_VIEW (m_widget = gtk_tree_view_new());
			gtk_tree_view_set_headers_visible (view, FALSE);
			gtk_tree_view_set_search_column (view, YGtkZyppModel::NAME_COLUMN);
			GtkTreeViewColumn *column;
			GtkCellRenderer *renderer;
			renderer = gtk_cell_renderer_pixbuf_new();
			column = gtk_tree_view_column_new_with_attributes ("", renderer,
				"pixbuf", YGtkZyppModel::ICON_COLUMN, NULL);
			gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
			gtk_tree_view_column_set_fixed_width (column, 38);
			gtk_tree_view_append_column (view, column);
			renderer = gtk_cell_renderer_text_new();
			g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
			column = gtk_tree_view_column_new_with_attributes ("", renderer,
				"markup", YGtkZyppModel::NAME_DESCRIPTION_COLUMN, NULL);
			gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
			gtk_tree_view_column_set_fixed_width (column, 50 /* it will expand */);
			gtk_tree_view_column_set_expand (column, TRUE);
			gtk_tree_view_append_column (view, column);
			if (!isTree)
				gtk_tree_view_set_fixed_height_mode (view, TRUE);
			gtk_tree_view_set_show_expanders (view, FALSE);

			GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
			gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
			g_signal_connect (G_OBJECT (selection), "changed",
			                  G_CALLBACK (packages_selected_cb), this);
			gtk_tree_selection_set_select_function (selection, can_select_row_cb,
			                                        this, NULL);
			gtk_widget_show (m_widget);

			g_signal_connect (G_OBJECT (m_widget), "popup-menu",
			                  G_CALLBACK (popup_key_cb), this);
			g_signal_connect (G_OBJECT (m_widget), "button-press-event",
			                  G_CALLBACK (popup_button_cb), this);
			g_signal_connect_after (G_OBJECT (m_widget), "size-allocate",
			                        G_CALLBACK (size_allocated_cb), this);
			if (showTooltips)
				gtk_tree_view_set_tooltip_column (view, YGtkZyppModel::PATTERN_DESCRIPTION_COLUMN);
		}

		virtual void setModel (GtkTreeModel *model)
		{
			GtkTreeView *view = GTK_TREE_VIEW (m_widget);
			gtk_tree_view_set_model (view, model);
			if (m_isTree)
				gtk_tree_view_expand_all (view);
			if (GTK_WIDGET_REALIZED (m_widget))
				gtk_tree_view_scroll_to_point (view, 0, 0);
		}

		GtkTreeSelection *getTreeSelection()
		{ return gtk_tree_view_get_selection (GTK_TREE_VIEW (m_widget)); }

		virtual GList *getSelectedPaths (GtkTreeModel **model)
		{ return gtk_tree_selection_get_selected_rows (getTreeSelection(), model); }

		virtual void selectAll()
		{ gtk_tree_selection_select_all (getTreeSelection()); }

		virtual void unselectAll()
		{ gtk_tree_selection_unselect_all (getTreeSelection()); }

		virtual int countSelected()
		{ return gtk_tree_selection_count_selected_rows (getTreeSelection()); }

		static void packages_selected_cb (GtkTreeSelection *selection, View *pThis)
		{ pThis->signalSelected(); }

		static gboolean popup_button_cb (GtkWidget *widget, GdkEventButton *event, View *pThis)
		{
			// workaround (based on gedit): we want the tree view to receive this press in order
			// to select the row, but we can't use connect_after, so we throw a dummy mouse press
			if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
				static bool safeguard = false;
				if (safeguard) return false;
				safeguard = true;
				if (pThis->countSelected() <= 1) {  // if there is a selection, let it be
					event->button = 1;
					if (!gtk_widget_event (widget, (GdkEvent *) event))
						return FALSE;
				}
				pThis->signalPopup (3, event->time);
				safeguard = false;
				return TRUE;
			}
			return FALSE;
		}

		static gboolean can_select_row_cb (GtkTreeSelection *selection, GtkTreeModel *model,
			GtkTreePath *path, gboolean path_currently_selected, gpointer data)
		{
			void *package;
			GtkTreeIter iter;
			gtk_tree_model_get_iter (model, &iter, path);
			gtk_tree_model_get (model, &iter, YGtkZyppModel::PTR_COLUMN, &package, -1);
			return package != NULL;
		}

		virtual void ensureVisible (GtkTreePath *path)
		{
			GtkTreeView *view = GTK_TREE_VIEW (m_widget);
			gtk_tree_view_scroll_to_cell (view, path, NULL, FALSE, 0, 0);
		}

#if 0
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
#endif
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

			g_signal_connect (G_OBJECT (m_widget), "popup-menu",
			                  G_CALLBACK (popup_key_cb), this);
			g_signal_connect_after (G_OBJECT (m_widget), "button-press-event",
			                        G_CALLBACK (popup_button_after_cb), this);
			g_signal_connect_after (G_OBJECT (m_widget), "size-allocate",
			                        G_CALLBACK (size_allocated_cb), this);
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

		virtual GList *getSelectedPaths (GtkTreeModel **model)
		{
			GtkIconView *view = GTK_ICON_VIEW (m_widget);
			if (model)
				*model = gtk_icon_view_get_model (view);
			GList *paths = gtk_icon_view_get_selected_items (view);
			return paths;
		}

		virtual void selectAll()
		{ gtk_icon_view_select_all (GTK_ICON_VIEW (m_widget)); }
		virtual void unselectAll()
		{ gtk_icon_view_unselect_all (GTK_ICON_VIEW (m_widget)); }

		static void packages_selected_cb (GtkIconView *view, View *pThis)
		{ pThis->signalSelected(); }

		static gboolean popup_button_after_cb (GtkWidget *widget, GdkEventButton *event,
		                                       View *pThis)
		{
			if (event->type == GDK_BUTTON_PRESS && event->button == 3)
				pThis->signalPopup (3, event->time);
			return FALSE;
		}

		virtual void ensureVisible (GtkTreePath *path)
		{
			GtkIconView *view = GTK_ICON_VIEW (m_widget);
			gtk_icon_view_scroll_to_path (view, path, FALSE, 0, 0);
		}
	};

GtkWidget *m_bin;
GtkTreeModel *m_model;
View *m_view;
bool m_isTree;

public:
	GtkWidget *getWidget()
	{ return m_bin; }

	PackagesView (bool isTree)
	: m_listener (NULL), m_model (NULL), m_view (NULL), m_isTree (isTree)
	{
		m_bin = ygtk_scrolled_window_new();

		if (!isTree) {
			GtkWidget *buttons = gtk_vbox_new (FALSE, 0), *button;
			button = create_toggle_button (pkg_list_mode_xpm, "List view", NULL);
			gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, TRUE, 0);
			button = create_toggle_button (pkg_tiles_mode_xpm, "Tiles view", button);
			gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, TRUE, 0);
			gtk_widget_show_all (buttons);

			ygtk_scrolled_window_set_corner_widget (YGTK_SCROLLED_WINDOW (m_bin), buttons);
		}

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
		if (inited())
			busyCursor();

		if (m_view)
			gtk_container_remove (GTK_CONTAINER (m_bin), m_view->m_widget);
		delete m_view;
		if (mode == LIST_MODE)
			m_view = new ListView (m_isTree, m_isTree, this);
		else
			m_view = new IconView (this);
		gtk_container_add (GTK_CONTAINER (m_bin), m_view->m_widget);
		if (m_model)
			m_view->setModel (m_model);

		packagesSelected (PkgList());
		normalCursor();
	}

	void setPool (Ypp::Pool *pool)
	{
		if (GTK_WIDGET_REALIZED (m_bin))
			busyCursor();

		if (m_model)
			g_object_unref (G_OBJECT (m_model));

		YGtkZyppModel *zmodel = ygtk_zypp_model_new (pool);
		m_model = GTK_TREE_MODEL (zmodel);
		if (m_view) {
			m_view->setModel (m_model);
			packagesSelected (PkgList());
		}
		normalCursor();
	}

	void setQuery (Ypp::QueryPool::Query *query)
	{
		Ypp::QueryPool *pool = new Ypp::QueryPool (query);
		setPool (pool);
	}

	PkgList getSelected()
	{ return m_view->getSelected(); }

	void unselectAll()
	{ m_view->unselectAll(); }

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

		// set button tiny
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
			gtk_widget_show_all (m_box);
			modified (package);
			g_signal_connect (G_OBJECT (m_label), "style-set",
			                  G_CALLBACK (label_style_set_cb), NULL);
			g_signal_connect (G_OBJECT (m_box), "style-set",
			                  G_CALLBACK (box_style_set_cb), NULL);
			g_signal_connect (G_OBJECT (m_button), "clicked",
			                  G_CALLBACK (undo_clicked_cb), package);
		}

		void modified (Ypp::Package *package)
		{
			const Ypp::Package::Version *version = 0;
			std::string text;
			if (package->toInstall (&version)) {
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
			std::string tooltip = package->summary();
			if (version)
				tooltip += "\nfrom <i>" + version->repo->name + "</i>";
			gtk_widget_set_tooltip_markup (m_label, tooltip.c_str());
		}

		static void undo_clicked_cb (GtkButton *button, Ypp::Package *package)
		{
			package->undo();
		}

		static void label_style_set_cb (GtkWidget *widget, GtkStyle *prev_style)
		{
			static bool safeguard = false;
			if (safeguard) return;
			safeguard = true;
			GdkColor *color = &widget->style->fg [GTK_STATE_SELECTED];
			gtk_widget_modify_fg (widget, GTK_STATE_NORMAL, color);
			safeguard = false;	
		}
		static void box_style_set_cb (GtkWidget *widget, GtkStyle *prev_style)
		{
			int width = YGUtils::getCharsWidth (widget, 22);
			gtk_widget_set_size_request (widget, width, -1);
		}
	};

GtkWidget *m_box, *m_entries_box, *m_container;
Ypp::Pool *m_pool;
GList *m_entries;
YGtkWizard *m_wizard;

public:
	GtkWidget *getWidget()
	{ return m_box; }

	ChangesPane (YGtkWizard *wizard, bool update_mode)
	: m_container (NULL), m_entries (NULL), m_wizard (wizard)
	{
		GtkWidget *heading = gtk_label_new (_("Changes:"));
		YGUtils::setWidgetFont (heading, PANGO_WEIGHT_ULTRABOLD, PANGO_SCALE_LARGE);
		gtk_misc_set_alignment (GTK_MISC (heading), 0, 0.5);
		g_signal_connect (G_OBJECT (heading), "style-set",
		                  G_CALLBACK (Entry::label_style_set_cb), NULL);
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

		Ypp::QueryPool::Query *query = new Ypp::QueryPool::Query();
		query->setIsModified (true);
		if (update_mode) {
			query->addType (Ypp::Package::PATCH_TYPE);
			m_pool = new Ypp::QueryPool (query);
		}
		else
			m_pool = new Ypp::QueryPool (query, true);
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

	void UpdateVisible()
	{
		ygtk_wizard_enable_next_button (m_wizard, m_entries != NULL);
		if (m_container) {
			if (m_entries != NULL)
				gtk_widget_show (m_container);
			else
				gtk_widget_hide (m_container);
		}
	}

	void setContainer (GtkWidget *container)
	{
		m_container = container;
		UpdateVisible();
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
		int index = m_pool->toPath (iter).front();
		m_entries = g_list_insert (m_entries, entry, index);
		UpdateVisible();
	}

	virtual void entryDeleted  (Ypp::Pool::Iter iter, Ypp::Package *package)
	{
		int index = m_pool->toPath (iter).front();
		GList *i = g_list_nth (m_entries, index);
		Entry *entry = (Entry *) i->data;
		gtk_container_remove (GTK_CONTAINER (m_entries_box), entry->getWidget());
		delete entry;
		m_entries = g_list_delete_link (m_entries, i);
		UpdateVisible();
	}

	virtual void entryChanged  (Ypp::Pool::Iter iter, Ypp::Package *package)
	{
		int index = m_pool->toPath (iter).front();
		Entry *entry = (Entry *) g_list_nth_data (m_entries, index);
		entry->modified (package);
	}

	static void style_set_cb (GtkWidget *widget, GtkStyle *prev_style)
	{
		static bool safeguard = false;
		if (safeguard) return;
		safeguard = true;
		GdkColor *color = &widget->style->bg [GTK_STATE_SELECTED];
		gtk_widget_modify_bg (widget, GTK_STATE_NORMAL, color);
		safeguard = false;	
	}
};

#include "icons/pkg-installed.xpm"
#include "icons/pkg-installed-upgradable.xpm"
#include "icons/pkg-available.xpm"

class Collections
{
public:
	struct Listener {
		virtual void collectionChanged() = 0;
	};

private:
	struct View
	{
		virtual GtkWidget *getWidget() = 0;
		virtual void writeQuery (Ypp::QueryPool::Query *query) = 0;

		Collections::Listener *m_listener;
		View (Collections::Listener *listener)
		: m_listener (listener)
		{}
		virtual ~View() {}

		void signalChanged()
		{ m_listener->collectionChanged(); }
	};

	struct Categories : public View
	{
		GtkWidget *m_scroll, *m_view;
		bool m_alternative;  // use second categories...
	public:
		virtual GtkWidget *getWidget()
		{ return m_scroll; }

		Categories (Collections::Listener *listener, Ypp::Package::Type type, bool alternative)
		: View (listener), m_alternative (alternative)
		{
			GtkTreeViewColumn *column;
			GtkCellRenderer *renderer;

			m_view = gtk_tree_view_new();
			GtkTreeView *view = GTK_TREE_VIEW (m_view);
			gtk_tree_view_set_headers_visible (view, FALSE);
			gtk_tree_view_set_search_column (view, 0);
			renderer = gtk_cell_renderer_pixbuf_new();
			column = gtk_tree_view_column_new_with_attributes ("",
				renderer, "pixbuf", 2, NULL);
			gtk_tree_view_column_set_expand (column, FALSE);
			gtk_tree_view_append_column (view, column);
			renderer = gtk_cell_renderer_text_new();
			column = gtk_tree_view_column_new_with_attributes ("",
				renderer, "text", 0, NULL);
			gtk_tree_view_column_set_expand (column, TRUE);
			gtk_tree_view_append_column (view, column);
			if (type != Ypp::Package::PACKAGE_TYPE)
				gtk_tree_view_set_show_expanders (view, FALSE);
			else
				gtk_tree_view_set_expander_column (view, column);

			GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
			gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
			g_signal_connect (G_OBJECT (selection), "changed",
					          G_CALLBACK (selection_cb), this);

			m_scroll = gtk_scrolled_window_new (NULL, NULL);
			gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (m_scroll),
					                             GTK_SHADOW_IN);
			gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (m_scroll),
					                        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
			gtk_container_add (GTK_CONTAINER (m_scroll), m_view);

			build (type);
		}

		void build (Ypp::Package::Type type)
		{
			GtkTreeModel *model = NULL;

			struct inner {
				static void populate (GtkTreeStore *store, GtkTreeIter *parent,
							          Ypp::Node *category, Categories *pThis)
				{
					if (!category)
						return;
					GtkTreeIter iter;
					gtk_tree_store_append (store, &iter, parent);
					const std::string &name = category->name;
					const char *icon = category->icon;
					gtk_tree_store_set (store, &iter, 0, name.c_str(), 1, category, -1);
					if (icon) {
						GdkPixbuf *pixbuf = loadThemeIcon (icon);
						gtk_tree_store_set (store, &iter, 2, pixbuf, -1);
						if (pixbuf)
							g_object_unref (G_OBJECT (pixbuf));
					}
					populate (store, &iter, category->child(), pThis);
					populate (store, parent, category->next(), pThis);
				}
				static GdkPixbuf *loadThemeIcon (const char *icon)
				{
					GtkIconTheme *icons = gtk_icon_theme_get_default();
					GdkPixbuf *pixbuf = gtk_icon_theme_load_icon (icons, icon, 22,
						GtkIconLookupFlags (0), NULL);
					return pixbuf;
				}
			};

			GtkTreeStore *store = gtk_tree_store_new (3, G_TYPE_STRING, G_TYPE_POINTER,
						                              GDK_TYPE_PIXBUF);
			model = GTK_TREE_MODEL (store);

			GtkTreeView *view = GTK_TREE_VIEW (m_view);
			GtkTreeSelection *selection = gtk_tree_view_get_selection (view);

			g_signal_handlers_block_by_func (selection, (gpointer) selection_cb, this);

			GtkTreeIter iter;
			gtk_tree_store_append (store, &iter, NULL);
			gtk_tree_store_set (store, &iter, 0, _("All"), 1, NULL, -1);

			Ypp::Node *first_category;
			if (m_alternative)
				first_category = Ypp::get()->getFirstCategory2 (type);
			else
				first_category = Ypp::get()->getFirstCategory (type);
			inner::populate (store, NULL, first_category, this);
			if (m_alternative) {
				GdkPixbuf *pixbuf = inner::loadThemeIcon (GTK_STOCK_ABOUT);
				gtk_tree_store_append (store, &iter, NULL);
				gtk_tree_store_set (store, &iter, 0, _("Recommended"), 1, GINT_TO_POINTER (1),
					                2, pixbuf, -1);
				gtk_tree_store_append (store, &iter, NULL);
				gtk_tree_store_set (store, &iter, 0, _("Suggested"), 1, GINT_TO_POINTER (2),
					                2, pixbuf, -1);
				g_object_unref (G_OBJECT (pixbuf));
				gtk_tree_view_set_show_expanders (view, FALSE);
			}

			gtk_tree_view_set_model (view, model);
			g_object_unref (G_OBJECT (model));
			selectFirstItem();

			g_signal_handlers_unblock_by_func (selection, (gpointer) selection_cb, this);
		}

		void selectFirstItem()
		{
			GtkTreeView *view = GTK_TREE_VIEW (m_view);
			GtkTreeSelection *selection = gtk_tree_view_get_selection (view);

			g_signal_handlers_block_by_func (selection, (gpointer) selection_cb, this);

			/* we use gtk_tree_view_set_cursor(), rather than gtk_tree_selection_select_iter()
			   because that one is buggy in that when the user first interacts with the treeview,
			   a change signal is sent, even if he was just expanding one node... */
			GtkTreePath *path = gtk_tree_path_new_first();
			gtk_tree_view_set_cursor (view, path, NULL, FALSE);
			gtk_tree_path_free (path);

			g_signal_handlers_unblock_by_func (selection, (gpointer) selection_cb, this);
		}

		static void selection_cb (GtkTreeSelection *selection, Categories *pThis)
		{
			pThis->signalChanged();

			// if item unselected, make sure "All" is
			if (!gtk_tree_selection_get_selected (selection, NULL, NULL))
				pThis->selectFirstItem();
		}

		virtual void writeQuery (Ypp::QueryPool::Query *query)
		{
			Ypp::Node *node = 0;
			GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (m_view));
			GtkTreeModel *model;
			GtkTreeIter iter;
			if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
				gpointer ptr;
				gtk_tree_model_get (model, &iter, 1, &ptr, -1);
				if (GPOINTER_TO_INT (ptr) == 1)
					query->setIsRecommended (true);
				else if (GPOINTER_TO_INT (ptr) == 2)
					query->setIsSuggested (true);
				else
					node = (Ypp::Node *) ptr;
			}
			if (node) {
				if (m_alternative)
					query->addCategory2 (node);
				else
					query->addCategory (node);
			}
		}
	};

	struct Pool : public View, public PackagesView::Listener
	{
		PackagesView * m_view;
		GtkWidget *m_box, *m_buttons_box;
		Ypp::Package::Type m_type;

	public:
		virtual GtkWidget *getWidget()
		{ return m_box; }

		Pool (Collections::Listener *listener, Ypp::Package::Type type)
		: View (listener)
		{
			m_type = type;

			m_view = new PackagesView (true);
			m_view->setPool (new Ypp::TreePool (type));
			m_view->setListener (this);

			// control buttons
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

			// layout
			m_box = gtk_vbox_new (FALSE, 4);
			gtk_box_pack_start (GTK_BOX (m_box), m_view->getWidget(), TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (m_box), m_buttons_box, FALSE, TRUE, 0);
		}

		virtual ~Pool()
		{
			delete m_view;
		}

		virtual void packagesSelected (const PkgList &selection)
		{
			gtk_widget_set_sensitive (m_buttons_box, !selection.empty());
			signalChanged();
		}

		virtual void writeQuery (Ypp::QueryPool::Query *query)
		{
			PkgList selected = m_view->getSelected();
			for (PkgList::const_iterator it = selected.begin();
			     it != selected.end(); it++)
				query->addCollection (*it);
		}

		void doAll (bool install /*or remove*/)
		{
			// we just need to mark the collections themselves
			PkgList selected = m_view->getSelected();
			install ? selected.install() : selected.remove();
		}

		static void install_cb (GtkButton *button, Pool *pThis)
		{ pThis->doAll (true); }
		static void remove_cb (GtkButton *button, Pool *pThis)
		{ pThis->doAll (false); }
	};

	View *m_view;
	GtkWidget *m_bin;
	Collections::Listener *m_listener;

	public:
	Collections (Collections::Listener *listener)
	: m_view (NULL), m_listener (listener)
	{
		m_bin = gtk_event_box_new();
	}

	~Collections()
	{
		delete m_view;
	}

	GtkWidget *getWidget()
	{ return m_bin; }

	void setType (Ypp::Package::Type type, bool alternative = false)
	{
		if (m_view)
			gtk_container_remove (GTK_CONTAINER (m_bin), m_view->getWidget());
		delete m_view;

		switch (type)
		{
			case Ypp::Package::PACKAGE_TYPE:
			case Ypp::Package::PATCH_TYPE:
				m_view = new Categories (m_listener, type, alternative);
				break;
			case Ypp::Package::PATTERN_TYPE:
			//case Ypp::Package::LANGUAGE_TYPE:
				m_view = new Pool (m_listener, type);
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

	void writeQuery (Ypp::QueryPool::Query *query)
	{
		if (m_view)
			m_view->writeQuery (query);
	}
};

class Filters : public Collections::Listener
{
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
			button = createButton (_("Available"), pkg_available_xpm, NULL);
			group = ygtk_toggle_button_get_group (YGTK_TOGGLE_BUTTON (button));
			gtk_box_pack_start (GTK_BOX (m_box), button, TRUE, TRUE, 0);
			if (!updateMode) {
				button = createButton (_("Upgrades"), pkg_installed_upgradable_xpm, group);
				gtk_box_pack_start (GTK_BOX (m_box), button, TRUE, TRUE, 0);
			}
			button = createButton (_("Installed"), pkg_installed_xpm, group);
			gtk_box_pack_start (GTK_BOX (m_box), button, TRUE, TRUE, 0);
			button = createButton (_("All"), 0, group);
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
		virtual void doQuery (Ypp::QueryPool::Query *query) = 0;
	};
	void setListener (Listener *listener)
	{ m_listener = listener; signalChanged(); }

private:
	Collections *m_collection;
	StatusButtons *m_statuses;
	GtkWidget *m_name, *m_repos, *m_type;
	Listener *m_listener;
	guint timeout_id;
	bool m_updateMode, m_enableRepoMgr;

	enum PaneType { GROUPS_TYPE, CATEGORIES_TYPE, PATTERNS_TYPE };
	int m_selectedType;

public:
	GtkWidget *getCollectionWidget() { return m_collection->getWidget(); }
	GtkWidget *getStatusesWidget()   { return m_statuses->getWidget(); }
	GtkWidget *getNameWidget()       { return m_name; }
	GtkWidget *getReposWidget()      { return m_repos; }
	GtkWidget *getTypeWidget()       { return m_type; }

	Filters (bool updateMode, bool enableRepoMgr)
	: m_listener (NULL), timeout_id (0), m_updateMode (updateMode),
	  m_enableRepoMgr (enableRepoMgr), m_selectedType (-1)
	{
		m_collection = new Collections (this);
		m_statuses = new StatusButtons (this, updateMode);

		m_name = ygtk_find_entry_new();
		gtk_widget_set_tooltip_markup (m_name,
			_("<b>Package search:</b> Use spaces to separate your keywords. They "
			"will be matched against RPM <i>name</i>, <i>summary</i> and "
			"<i>provides</i> attributes.\n(e.g.: \"yast dhcp\" will return yast's "
			"dhcpd tool)"));
		g_signal_connect (G_OBJECT (m_name), "changed",
		                  G_CALLBACK (entry_changed_cb), this);

		m_repos = gtk_combo_box_new();
		gtk_widget_set_tooltip_markup (m_repos,
			_("<b>Package repositories:</b> Limits the query to one repository.\n"
			"You can add or remove them  through the YaST control center or by "
			"selecting the respective option."));
		GtkListStore *store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);
		gtk_combo_box_set_model (GTK_COMBO_BOX (m_repos), GTK_TREE_MODEL (store));
		g_object_unref (G_OBJECT (store));
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (m_repos), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (m_repos), renderer,
			"markup", 0, NULL);
		g_object_set (G_OBJECT (renderer), "width-chars", 25,
		              "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
		GtkTreeIter iter;
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, _("All Repositories"), 1, FALSE, -1);
		for (int i = 0; Ypp::get()->getRepository (i); i++) {
			const Ypp::Repository *repo = Ypp::get()->getRepository (i);
			gtk_list_store_append (store, &iter);
			std::string str = "  " + repo->name;
			gtk_list_store_set (store, &iter, 0, str.c_str(), 1, FALSE, -1);
		}
		if (enableRepoMgr) {
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, "-", 1, TRUE, -1);
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, _("Add or Remove..."), 1, FALSE, -1);
		}
		gtk_combo_box_set_active (GTK_COMBO_BOX (m_repos), 0);
		gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (m_repos),
			is_combo_entry_separator_cb, this, NULL);
		g_signal_connect_after (G_OBJECT (m_repos), "changed",
		                        G_CALLBACK (combo_changed_cb), this);

		m_type = gtk_combo_box_new_text();
		if (updateMode)
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_type), _("Patches"));
		else {
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_type), _("Groups"));
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_type), _("Categories"));
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_type), _("Patterns"));
#if 0
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_type), _("Languages"));
#endif
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
	const Ypp::Repository *selectedRepo()
	{
		GtkComboBox *combo = GTK_COMBO_BOX (m_repos);
		int repo = gtk_combo_box_get_active (combo);

		if (m_enableRepoMgr) {
			GtkTreeModel *model = gtk_combo_box_get_model (combo);
			int setup_id = gtk_tree_model_iter_n_children (model, NULL);
			if (repo == setup_id-1) {
				YGUI::ui()->sendEvent (new YMenuEvent ("repo_mgr"));
				repo = 0;
			}
		}

		const Ypp::Repository *ret = 0;
		if (repo > 0)
			ret = Ypp::get()->getRepository (repo-1);
		Ypp::get()->setFavoriteRepository (ret);
		return ret;
	}

private:
	static void entry_changed_cb (GtkEditable *editable, Filters *pThis)
	{ pThis->signalChangedDelay(); }
	static void combo_changed_cb (GtkComboBox *combo, Filters *pThis)
	{ pThis->signalChanged(); }
	virtual void collectionChanged()
	{ signalChanged(); }

	void signalChanged()
	{
		if (!m_listener) return;

		PaneType t = (PaneType) gtk_combo_box_get_active (GTK_COMBO_BOX (m_type));
		bool alternative = false;
		Ypp::Package::Type type;
		if (m_updateMode)
			type = Ypp::Package::PATCH_TYPE;
		else {
			if (t == PATTERNS_TYPE)
				type = Ypp::Package::PATTERN_TYPE;
			else {
				type = Ypp::Package::PACKAGE_TYPE;
				alternative = t == GROUPS_TYPE;
			}
		}

		// adjust interface
		if (t != m_selectedType) {
			m_collection->setType (type, alternative);
			m_selectedType = t;
		}

		// create query
		Ypp::QueryPool::Query *query = new Ypp::QueryPool::Query();
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

		if (selectedRepo())
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

	// callbacks
	static gboolean is_combo_entry_separator_cb (GtkTreeModel *model, GtkTreeIter *iter,
	                                             gpointer data)
	{
		gboolean ret;
		gtk_tree_model_get (model, iter, 1, &ret, -1);
		return ret;
	}
};

class PackageControl
{
GtkWidget *m_widget, *m_install_button, *m_remove_button, *m_installed_version,
          *m_available_versions, *m_installed_box, *m_available_box;
//#define SHOW_LOCK_UNDO_BUTTONS  // to specific -- show them only as popups
#ifdef SHOW_LOCK_UNDO_BUTTON
GtkWidget *m_undo_button, *m_lock_button, *m_locked_image, *m_unlocked_image;
#endif

public:
PkgList m_packages;  // we keep a copy to test against modified...
Filters *m_filters;  // used to filter repo versions...

	GtkWidget *getWidget()
	{ return m_widget; }

	PackageControl (Filters *filters)
	: m_filters (filters)
	{
		// installed
		m_remove_button = createButton (_("_Remove"), GTK_STOCK_DELETE);
		g_signal_connect (G_OBJECT (m_remove_button), "clicked",
		                  G_CALLBACK (remove_clicked_cb), this);

		m_installed_version = gtk_label_new ("");
		gtk_label_set_selectable (GTK_LABEL (m_installed_version), TRUE);
		gtk_misc_set_alignment (GTK_MISC (m_installed_version), 0, 0.5);

		m_installed_box = gtk_vbox_new (FALSE, 2);
		gtk_box_pack_start (GTK_BOX (m_installed_box), createBoldLabel (_("Installed:")),
		                    FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_installed_box), m_installed_version, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_installed_box), m_remove_button, FALSE, TRUE, 0);

		// available
		m_install_button = createButton ("", GTK_STOCK_SAVE);
		g_signal_connect (G_OBJECT (m_install_button), "clicked",
		                  G_CALLBACK (install_clicked_cb), this);

		m_available_versions = gtk_combo_box_new();
		GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
		gtk_combo_box_set_model (GTK_COMBO_BOX (m_available_versions),
		                         GTK_TREE_MODEL (store));
		g_object_unref (G_OBJECT (store));
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (m_available_versions), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (m_available_versions), renderer,
		                                "markup", 0, NULL);
		g_signal_connect (G_OBJECT (m_available_versions), "changed",
		                  G_CALLBACK (version_changed_cb), this);

		m_available_box = gtk_vbox_new (FALSE, 2);
		gtk_box_pack_start (GTK_BOX (m_available_box), createBoldLabel (_("Available:")),
		                    FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_available_box), m_available_versions, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_available_box), m_install_button, FALSE, TRUE, 0);

		m_widget = gtk_vbox_new (FALSE, 12);
		gtk_box_pack_start (GTK_BOX (m_widget), m_installed_box, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_widget), m_available_box, FALSE, TRUE, 0);

#ifdef SHOW_LOCK_UNDO_BUTTON
		m_lock_button = gtk_toggle_button_new();
		gtk_widget_set_tooltip_markup (m_lock_button, lock_tooltip);
		g_signal_connect (G_OBJECT (m_lock_button), "toggled",
		                  G_CALLBACK (locked_toggled_cb), this);
		m_locked_image = createImageFromXPM (pkg_locked_xpm);
		m_unlocked_image = createImageFromXPM (pkg_unlocked_xpm);
		g_object_ref_sink (G_OBJECT (m_locked_image));
		g_object_ref_sink (G_OBJECT (m_unlocked_image));

		m_undo_button = gtk_button_new_from_stock (GTK_STOCK_UNDO);
		g_signal_connect (G_OBJECT (m_undo_button), "clicked",
		                  G_CALLBACK (undo_clicked_cb), this);

		GtkWidget *lock_align = gtk_alignment_new (1.0, 0.5, 0, 0);
		gtk_container_add (GTK_CONTAINER (lock_align), m_lock_button);
		gtk_box_pack_start (GTK_BOX (m_widget), lock_align, FALSE, TRUE, 0);

		GtkWidget *undo_align = gtk_alignment_new (0.5, 0.5, 0, 0);
		gtk_container_add (GTK_CONTAINER (undo_align), m_undo_button);
		gtk_box_pack_start (GTK_BOX (m_widget), undo_align, FALSE, TRUE, 0);
#endif
	}

#ifdef SHOW_LOCK_UNDO_BUTTON
	~PackageControl()
	{
		gtk_widget_destroy (m_locked_image);
		g_object_unref (G_OBJECT (m_locked_image));
		gtk_widget_destroy (m_unlocked_image);
		g_object_unref (G_OBJECT (m_unlocked_image));
	}
#endif

	void setPackages (const PkgList &packages)
	{
		m_packages = packages;
		if (packages.empty())
			return;

		Ypp::Package *single_package = packages.single() ? packages.front() : NULL;

		if (packages.installed()) {
			gtk_widget_show (m_installed_box);
			if (single_package)
				gtk_label_set_text (GTK_LABEL (m_installed_version),
				                    single_package->getInstalledVersion()->number.c_str());
			else
				gtk_label_set_text (GTK_LABEL (m_installed_version), "(several)");
		}
		else {
			gtk_widget_hide (m_installed_box);
		}

		GtkTreeModel *model = gtk_combo_box_get_model (
			GTK_COMBO_BOX (m_available_versions));
		gtk_list_store_clear (GTK_LIST_STORE (model));
		gtk_widget_set_sensitive (m_available_versions, FALSE);
		gtk_widget_show (m_available_box);
		if (single_package) {
			if (single_package->getAvailableVersion (0)) {
				gtk_widget_set_sensitive (m_available_versions, TRUE);
				gtk_widget_show (m_available_box);
				const Ypp::Repository *selectedRepo = m_filters->selectedRepo();
				for (int i = 0; single_package->getAvailableVersion (i); i++) {
					const Ypp::Package::Version *version = single_package->getAvailableVersion (i);
					string text = version->number + "\n";
					string repo = YGUtils::truncate (version->repo->name,
						MAX (20, version->number.length()), 0);
					text += "<i>" + repo + "</i>";
					GtkTreeIter iter;
					gtk_list_store_append (GTK_LIST_STORE (model), &iter);
					gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, text.c_str(), -1);
					if (selectedRepo && version->repo == selectedRepo) {
						gtk_combo_box_set_active (GTK_COMBO_BOX (m_available_versions), i);
						gtk_widget_set_sensitive (m_available_versions, FALSE);
					}
					else if (i == 0)
						gtk_combo_box_set_active (GTK_COMBO_BOX (m_available_versions), 0);
				}
			}
			else
				gtk_widget_hide (m_available_box);
		}
		else {
			if (packages.upgradable()) {
				gtk_combo_box_append_text (GTK_COMBO_BOX (m_available_versions), "(upgrades)");
				gtk_combo_box_set_active (GTK_COMBO_BOX (m_available_versions), 0);
				gtk_button_set_label (GTK_BUTTON (m_install_button), _("Upgrade"));
			}
			else if (packages.notInstalled()) {
				gtk_combo_box_append_text (GTK_COMBO_BOX (m_available_versions), "(several)");
				gtk_combo_box_set_active (GTK_COMBO_BOX (m_available_versions), 0);
				gtk_button_set_label (GTK_BUTTON (m_install_button), _("Install"));
			}
			else
				gtk_widget_hide (m_available_box);
		}

		// is locked
		if (packages.locked() || packages.unlocked()) {
#ifdef SHOW_LOCK_UNDO_BUTTON
			gtk_widget_show (m_lock_button);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (m_lock_button), packages.locked());
			gtk_button_set_image (GTK_BUTTON (m_lock_button),
				packages.locked() ? m_locked_image : m_unlocked_image);
#endif
			gtk_widget_set_sensitive (m_install_button, !packages.locked());
			gtk_widget_set_sensitive (m_remove_button, !packages.locked());
		}
		else {
#ifdef SHOW_LOCK_UNDO_BUTTON
			gtk_widget_hide (m_lock_button);
#endif
			gtk_widget_set_sensitive (m_install_button, TRUE);
			gtk_widget_set_sensitive (m_remove_button, TRUE);
		}

#ifdef SHOW_LOCK_UNDO_BUTTON
		if (packages.modified())
			gtk_widget_show (m_undo_button);
		else
			gtk_widget_hide (m_undo_button);
#endif
	}

	void packageModified (Ypp::Package *package)
	{
		// GTK+ doesn't fire selection change when a selected row changes, so we need
		// to re-load PackageControl in that occasions.
		if (m_packages.contains (package)) {
			m_packages.clearCache();
			setPackages (m_packages);
		}
	}

private:
	static void install_clicked_cb (GtkButton *button, PackageControl *pThis)
	{
		if (pThis->m_packages.single()) {
			busyCursor();
			Ypp::Package *package = pThis->m_packages.front();
			int version = gtk_combo_box_get_active (GTK_COMBO_BOX (
					pThis->m_available_versions));
			package->install (package->getAvailableVersion (version));
			normalCursor();
		}
		else
			pThis->m_packages.install();
	}

	static void remove_clicked_cb (GtkButton *button, PackageControl *pThis)
	{
		pThis->m_packages.remove();
	}

#ifdef SHOW_LOCK_UNDO_BUTTON
	static void locked_toggled_cb (GtkToggleButton *button, PackageControl *pThis)
	{
		bool lock = gtk_toggle_button_get_active (button);
		pThis->m_packages.lock (lock);
	}

	static void undo_clicked_cb (GtkButton *button, PackageControl *pThis)
	{
		pThis->m_packages.undo();
	}
#endif

	static void version_changed_cb (GtkComboBox *combo, PackageControl *pThis)
	{
		if (pThis->m_packages.single()) {
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
		if (label_str)
			button = gtk_button_new_with_mnemonic (label_str);
		else
			button = gtk_button_new();
		if (stock_id) {
			image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
			gtk_button_set_image (GTK_BUTTON (button), image);
		}
		return button;
	}
	static GtkWidget *createBoldLabel (const char *text)
	{
		GtkWidget *label = gtk_label_new (text);
		YGUtils::setWidgetFont (label, PANGO_WEIGHT_BOLD, PANGO_SCALE_MEDIUM);
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
		return label;
	}
};

class PackageDetails
{
public:
	struct Listener {
		virtual void goToPackage (Ypp::Package *package) = 0;
	};

private:
	struct TextExpander {
		GtkWidget *expander, *text;

		TextExpander (const char *name)
		{
			text = ygtk_html_wrap_new();

			string str = string ("<b>") + name + "</b>";
			expander = gtk_expander_new (str.c_str());
			gtk_expander_set_use_markup (GTK_EXPANDER (expander), TRUE);
			gtk_container_add (GTK_CONTAINER (expander), text);
		}

		void setText (const std::string &str)
		{
			ygtk_html_wrap_set_text (text, str.c_str(), FALSE);
			str.empty() ? gtk_widget_hide (expander) : gtk_widget_show (expander);
		}
	};
	struct DepExpander {
		GtkWidget *expander, *table, *requires, *provides;

		DepExpander (const char *name)
		{
			requires = ygtk_html_wrap_new();
			provides = ygtk_html_wrap_new();

			table = gtk_hbox_new (FALSE, 0);
			gtk_box_pack_start (GTK_BOX (table), requires, TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (table), provides, TRUE, TRUE, 0);

			string str = string ("<b>") + name + "</b>";
			expander = gtk_expander_new (str.c_str());
			gtk_expander_set_use_markup (GTK_EXPANDER (expander), TRUE);
			gtk_container_add (GTK_CONTAINER (expander), table);
		}

		void setPackage (const Ypp::Package *package)
		{
			if (package) {
				std::string requires_str = _("Requires:");
				std::string provides_str = _("Provides:");
				requires_str += "<br><blockquote>";
				provides_str += "<br><blockquote>";
				requires_str += package->requires();
				provides_str += package->provides();
				YGUtils::replace (requires_str, "\n", 1, "<br>");
				YGUtils::replace (provides_str, "\n", 1, "<br>");
				requires_str += "</blockquote>";
				provides_str += "</blockquote>";

				ygtk_html_wrap_set_text (requires, requires_str.c_str(), FALSE);
				ygtk_html_wrap_set_text (provides, provides_str.c_str(), FALSE);
				gtk_widget_show (expander);
			}
			else
				gtk_widget_hide (expander);
		}
	};

GtkWidget *m_widget, *m_description, *m_icon, *m_icon_frame;
TextExpander *m_filelist, *m_changelog, *m_authors;
DepExpander *m_dependencies;
Listener *m_listener;

public:
	void setListener (Listener *listener)
	{ m_listener = listener; }

	GtkWidget *getWidget()
	{ return m_widget; }

	PackageDetails (bool update_mode)
	: m_listener (NULL)
	{
		GtkWidget *vbox;
		m_widget = createWhiteViewPort (&vbox);

		GtkWidget *hbox = gtk_hbox_new (FALSE, 6);
		m_description = ygtk_html_wrap_new();
		ygtk_html_wrap_connect_link_clicked (m_description,
			G_CALLBACK (description_pressed_cb), this);
		gtk_box_pack_start (GTK_BOX (hbox), m_description, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), createIconWidget (&m_icon, &m_icon_frame),
		                    FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

		if (!update_mode) {
			m_filelist = new TextExpander (_("File List"));
			m_changelog = new TextExpander (_("Changelog"));
			m_authors = new TextExpander (_("Authors"));
			m_dependencies = new DepExpander (_("Dependencies"));
			gtk_box_pack_start (GTK_BOX (vbox), m_filelist->expander, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (vbox), m_changelog->expander, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (vbox), m_authors->expander, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (vbox), m_dependencies->expander, FALSE, TRUE, 0);
			ygtk_html_wrap_connect_link_clicked (m_filelist->text,
				G_CALLBACK (path_pressed_cb), NULL);
		}
		else {
			m_filelist = m_changelog = m_authors = NULL;
			m_dependencies = NULL;
		}
	}

	~PackageDetails()
	{
		delete m_filelist;
		delete m_changelog;
		delete m_authors;
		delete m_dependencies;
	}

	void setPackages (const PkgList &packages)
	{
		if (packages.empty())
			return;
		Ypp::Package *package = packages.front();

		gtk_widget_hide (m_icon_frame);
		if (packages.single()) {
			string description = "<b>" + package->name() + "</b><br>";
			description += package->description();
			ygtk_html_wrap_set_text (m_description, description.c_str(), FALSE);
			if (m_filelist)  m_filelist->setText (package->filelist());
			if (m_changelog) m_changelog->setText (package->changelog());
			if (m_authors)   m_authors->setText (package->authors());
			if (m_dependencies) m_dependencies->setPackage (package);

			gtk_image_clear (GTK_IMAGE (m_icon));
			GtkIconTheme *icons = gtk_icon_theme_get_default();
			GdkPixbuf *pixbuf = gtk_icon_theme_load_icon (icons,
				package->name().c_str(), 32, GtkIconLookupFlags (0), NULL);
			if (pixbuf) {
				gtk_image_set_from_pixbuf (GTK_IMAGE (m_icon), pixbuf);
				g_object_unref (G_OBJECT (pixbuf));
				gtk_widget_show (m_icon_frame);
			}
			scrollTop();
		}
		else {
			string description = "Selected:";
			description += "<ul>";
			for (std::list <Ypp::Package *>::const_iterator it = packages.begin();
			     it != packages.end(); it++)
				description += "<li><b>" + (*it)->name() + "</b></li>";
			description += "</ul>";
			ygtk_html_wrap_set_text (m_description, description.c_str(), FALSE);
			if (m_filelist)  m_filelist->setText ("");
			if (m_changelog) m_changelog->setText ("");
			if (m_authors)   m_authors->setText ("");
			if (m_dependencies) m_dependencies->setPackage (NULL);
		}
	}

private:
	static void path_pressed_cb (GtkWidget *text, const gchar *link)
	{ FILEMANAGER_LAUNCH (link); }

	static void description_pressed_cb (GtkWidget *text, const gchar *link,
	                                    PackageDetails *pThis)
	{
		if (!strncmp (link, "pkg://", 6)) {
			const gchar *pkg_name = link + 6;
			yuiMilestone() << "Hyperlinking to package \"" << pkg_name << "\"" << endl;
			Ypp::Package *pkg = Ypp::get()->findPackage (Ypp::Package::PACKAGE_TYPE, pkg_name);
			if (pThis->m_listener && pkg)
				pThis->m_listener->goToPackage (pkg);
			else {
				GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
					GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK, _("Package '%s' was not found."), pkg_name);
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
			}
		}
		else
			yuiError() << "Protocol not supported - can't follow hyperlink \""
			           << link << "\"" << endl;
	}

	void scrollTop()
	{
		GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW (m_widget);
		YGUtils::scrollWidget (gtk_scrolled_window_get_vadjustment (scroll), true);
	}

	// utilities:
	static GtkWidget *createHtmlWidget (const char *name, GtkWidget **html_widget,
	                                    bool visible)
	{
		*html_widget = ygtk_html_wrap_new();

		GtkWidget *box = gtk_event_box_new();
		gtk_container_add (GTK_CONTAINER (box), *html_widget);
		gtk_container_set_border_width (GTK_CONTAINER (box), 4);

		string str = string ("<b>") + name + "</b>";
		GtkWidget *expander = gtk_expander_new (str.c_str());
		gtk_expander_set_use_markup (GTK_EXPANDER (expander), TRUE);
		gtk_container_add (GTK_CONTAINER (expander), box);
		if (visible)
			gtk_expander_set_expanded (GTK_EXPANDER (expander), TRUE);
		return expander;
	}

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

	static GtkWidget *createWhiteViewPort (GtkWidget **vbox)
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

		*vbox = gtk_vbox_new (FALSE, 0);
		g_signal_connect (G_OBJECT (*vbox), "expose-event",
			              G_CALLBACK (inner::expose_cb), NULL);

		GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroll), *vbox);
		return scroll;
	}
};

#include "icons/harddisk.xpm"
#include "icons/harddisk-full.xpm"

class DiskView : public Ypp::Disk::Listener
{
GtkWidget *m_widget;
GdkPixbuf *m_diskPixbuf, *m_diskFullPixbuf;
bool m_hasWarn;
GtkTreeModel *m_model;

public:
	GtkWidget *getWidget()
	{ return m_widget; }

	DiskView()
	: m_hasWarn (false)
	{
		m_model = GTK_TREE_MODEL (gtk_list_store_new (
			// 0 - mount point, 1 - usage percent, 2 - usage string,
			// (highlight warns) 3 - font weight, 4 - font color string
			5, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING));
		GtkListStore *store = GTK_LIST_STORE (m_model);
		Ypp::Disk *disk = Ypp::get()->getDisk();
		for (int i = 0; disk->getPartition (i); i++) {
			GtkTreeIter iter;
			gtk_list_store_append (store, &iter);
		}

		m_widget = createMenuButton (m_model);
		g_object_unref (G_OBJECT (m_model));

		m_diskPixbuf = gdk_pixbuf_new_from_xpm_data (harddisk_xpm);
		m_diskFullPixbuf = gdk_pixbuf_new_from_xpm_data (harddisk_full_xpm);

		Ypp::get()->getDisk()->setListener (this);
		update();
	}

	~DiskView()
	{
		g_object_unref (m_diskPixbuf);
		g_object_unref (m_diskFullPixbuf);
	}

private:
	#define MIN_PERCENT_WARN 90
	#define MIN_FREE_MB_WARN (500*1024)
	virtual void update()
	{
		bool warn = false;
		GtkListStore *store = GTK_LIST_STORE (m_model);
		Ypp::Disk *disk = Ypp::get()->getDisk();
		for (int i = 0; disk->getPartition (i); i++) {
			const Ypp::Disk::Partition *part = disk->getPartition (i);

			long usage = (part->used * 100) / (part->total + 1);
			bool full = usage > MIN_PERCENT_WARN && (part->total - part->used) < MIN_FREE_MB_WARN;
			std::string usage_str = part->used_str + " / " + part->total_str;
			if (full)
				warn = true;

			GtkTreeIter iter;
			g_assert (gtk_tree_model_iter_nth_child (m_model, &iter, NULL, i));
			gtk_list_store_set (store, &iter, 0, part->path.c_str(), 1, usage,
			                    2, usage_str.c_str(), 3, PANGO_WEIGHT_NORMAL, 4, NULL, -1);
			if (full)
				gtk_list_store_set (store, &iter, 3, PANGO_WEIGHT_BOLD, 4, "red", -1);
		}
		GdkPixbuf *pixbuf = m_diskPixbuf;
		if (warn) {
			pixbuf = m_diskFullPixbuf;
			warnDialog();
		}
		gtk_image_set_from_pixbuf (GTK_IMAGE (GTK_BIN (m_widget)->child), pixbuf);
	}

	void warnDialog()
	{
		if (m_hasWarn) return;
		m_hasWarn = true;
		if (!GTK_WIDGET_REALIZED (m_widget)) return;

		GtkWidget *dialog, *view;
		dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
                                                 GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING,
                                                 GTK_BUTTONS_OK, "%s", _("Disk Almost Full !"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s",
			_("One of the partitions is reaching its limit of capacity. You may "
			"have to remove packages if you wish to install some."));

		view = createView (m_model, true);
		gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), view);

		g_signal_connect (G_OBJECT (dialog), "response",
		                  G_CALLBACK (gtk_widget_destroy), this);
		gtk_widget_show (dialog);
	}

	// utilities
	static GtkWidget *createView (GtkTreeModel *model, bool shadow)
	{
		GtkWidget *view = gtk_tree_view_new_with_model (model), *scroll;
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
		gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)),
		                             GTK_SELECTION_NONE);

		GtkTreeViewColumn *column;
		column = gtk_tree_view_column_new_with_attributes (_("Mount Point"),
			gtk_cell_renderer_text_new(), "text", 0, "weight", 3, "foreground", 4, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
		column = gtk_tree_view_column_new_with_attributes (_("Usage"),
			gtk_cell_renderer_progress_new(), "value", 1, "text", 2, NULL);
		gtk_tree_view_column_set_min_width (column, 150);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
		gtk_widget_show (view);

		if (shadow) {
			scroll = gtk_scrolled_window_new (NULL, NULL);
			gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
			                                     GTK_SHADOW_IN);
			gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				                            GTK_POLICY_NEVER, GTK_POLICY_NEVER);
			gtk_container_add (GTK_CONTAINER (scroll), view);
			gtk_widget_show (scroll);
			return scroll;
		}
		g_object_set (view, "can-focus", FALSE, NULL);
		return view;
	}
#if 0
	static void packCellRenderers (GtkCellLayout *layout)
	{
		GtkCellRenderer *renderer;
		renderer = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start (layout, renderer, FALSE);
		gtk_cell_layout_set_attributes (layout, renderer, "text", 0, NULL);
		// let's put all columns to the same width, to make it look like a table
		g_object_set (G_OBJECT (renderer), "width-chars", 14, NULL);
		renderer = gtk_cell_renderer_progress_new();
		gtk_cell_layout_pack_start (layout, renderer, TRUE);
		gtk_cell_layout_set_attributes (layout, renderer, "value", 1, "text", 2, NULL);
		g_object_set (G_OBJECT (renderer), "width", 150, NULL);
	}
	static GtkWidget *createCombo (GtkTreeModel *model)
	{
		GtkWidget *combo = gtk_combo_box_new_with_model (model);
		packCellRenderers (GTK_CELL_LAYOUT (combo));
		return combo;
	}
#endif
	static GtkWidget *createMenuButton (GtkTreeModel *model)
	{
		GtkWidget *button = ygtk_menu_button_new(), *menu;
		gtk_widget_set_tooltip_text (button, _("Disk usage"));
		gtk_container_add (GTK_CONTAINER (button), gtk_image_new_from_pixbuf (NULL));

#if 1
		menu = createView (model, false);
#else
		menu = gtk_menu_new();
		Ypp::Disk *disk = Ypp::get()->getDisk();
		for (int i = 0; disk->getPartition (i); i++) {
			GtkWidget *cell = gtk_cell_view_new();
			GtkTreePath *path = gtk_tree_path_new_from_indices (i, -1);
			gtk_cell_view_set_model (GTK_CELL_VIEW (cell), model);
			gtk_cell_view_set_displayed_row (GTK_CELL_VIEW (cell), path);
			packCellRenderers (GTK_CELL_LAYOUT (cell));
			gtk_tree_path_free (path);

			GtkWidget *item = gtk_menu_item_new();
			gtk_container_add (GTK_CONTAINER (item), cell);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			gtk_widget_show_all (cell);
			gtk_widget_show_all (item);
		}
#endif
		ygtk_menu_button_set_popup_align (YGTK_MENU_BUTTON (button), menu, 0.5, 1.0);
		return button;
	}
};

class PackageSelector : public Filters::Listener, public PackagesView::Listener,
                        public PackageDetails::Listener
{
PackagesView *m_packages;
Filters *m_filters;
PackageControl *m_control;
PackageDetails *m_details;
DiskView *m_disk;
GtkWidget *m_box, *m_details_box;
ChangesPane *m_changes;

public:
	GtkWidget *getWidget()
	{ return m_box; }

	PackageSelector (YGtkWizard *wizard, bool updateMode, bool enableRepoMgr)
	{
		m_packages = new PackagesView (false);
		m_filters = new Filters (updateMode, enableRepoMgr);
		m_control = new PackageControl (m_filters);
		m_details = new PackageDetails (updateMode);
		m_disk = new DiskView();
		m_changes = new ChangesPane (wizard, updateMode);
		m_packages->setListener (this);
		m_filters->setListener (this);
		m_details->setListener (this);

		GtkWidget *filter_box = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (filter_box), gtk_label_new (_("Filters:")), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (filter_box), m_filters->getNameWidget(), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (filter_box), m_filters->getReposWidget(), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (filter_box), gtk_label_new(""), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (filter_box), m_disk->getWidget(), FALSE, TRUE, 0);

		GtkWidget *categories_box = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (categories_box), m_filters->getCollectionWidget(),
		                    TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (categories_box), m_filters->getTypeWidget(), FALSE, TRUE, 0);

		GtkWidget *packages_box = gtk_hpaned_new();
		gtk_paned_pack1 (GTK_PANED (packages_box), categories_box, TRUE, TRUE);
		gtk_paned_pack2 (GTK_PANED (packages_box), m_packages->getWidget(), TRUE, FALSE);
		gtk_paned_set_position (GTK_PANED (packages_box), 180);

		m_details_box = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (m_details_box), m_details->getWidget(), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_details_box), m_control->getWidget(), FALSE, TRUE, 0);

		GtkWidget *packages_pane = gtk_vpaned_new();
		gtk_paned_pack1 (GTK_PANED (packages_pane), packages_box, TRUE, FALSE);
		gtk_paned_pack2 (GTK_PANED (packages_pane), m_details_box, TRUE, FALSE);
		gtk_paned_set_position (GTK_PANED (packages_pane), 30000 /* minimal size */);

		GtkWidget *vbox = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (vbox), m_filters->getStatusesWidget(), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), filter_box, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), packages_pane, TRUE, TRUE, 0);
		gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

		GtkWidget *changes_box = gtk_vbox_new (FALSE, 0);
		gtk_box_pack_start (GTK_BOX (changes_box), m_changes->getWidget(), TRUE, TRUE, 0);

		m_box = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (m_box), vbox, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_box), changes_box, FALSE, TRUE, 0);

		gtk_widget_show_all (m_box);
		m_changes->setContainer (changes_box);
		packagesSelected (PkgList());
		gtk_widget_hide (m_details_box);
	}

	~PackageSelector()
	{
		delete m_packages;
		delete m_filters;
		delete m_details;
		delete m_disk;
		delete m_changes;
	}

	virtual void doQuery (Ypp::QueryPool::Query *query)
	{
		m_packages->setQuery (query);
	}

	virtual void packagesSelected (const PkgList &packages)
	{
		m_details->setPackages (packages);
		m_control->setPackages (packages);
		if (!packages.empty())
			gtk_widget_show (m_details_box);
	}

	virtual void goToPackage (Ypp::Package *package)
	{
		m_packages->unselectAll();
		PkgList list;
		list.push (package);
		m_details->setPackages (list);
		m_control->setPackages (list);
	}

	void packageModified (Ypp::Package *package)
	{
		m_control->packageModified (package);
	}
};

#include "YPackageSelector.h"
#include "pkg-selector-help.h"

class YGPackageSelector : public YPackageSelector, public YGWidget,
                          public Ypp::Interface, public Ypp::PkgListener
{
PackageSelector *m_package_selector;

public:
	YGPackageSelector (YWidget *parent, long mode)
		: YPackageSelector (NULL, mode),
		  YGWidget (this, parent, true, YGTK_TYPE_WIZARD, NULL)
	{
		setBorder (0);
		YGTK_WIZARD (getWidget())->child_border_width = 0;

		GtkWindow *window = YGDialog::currentWindow();
		int enlarge_width = MIN (0.95 * YUI::app()->displayWidth(), 650);
		int enlarge_height = MIN (0.95 * YUI::app()->displayHeight(), 600);
		gtk_window_resize (window,
			MAX (enlarge_width, GTK_WIDGET (window)->allocation.width),
			MAX (enlarge_height, GTK_WIDGET (window)->allocation.height));

		YGtkWizard *wizard = YGTK_WIZARD (getWidget());
		ygtk_wizard_set_header_icon (wizard,
			THEMEDIR "/icons/22x22/apps/yast-software.png");
		ygtk_wizard_set_header_text (wizard,
			onlineUpdateMode() ? _("Patch Selection") : _("Package Selection"));

		ygtk_wizard_set_abort_button_label (wizard, _("_Cancel"));
		ygtk_wizard_set_abort_button_str_id (wizard, "cancel");
		ygtk_wizard_set_back_button_label (wizard, "");
		ygtk_wizard_set_next_button_label (wizard, _("_Apply"));
		ygtk_wizard_set_next_button_str_id (wizard, "accept");
		ygtk_wizard_enable_next_button (wizard, FALSE);
		g_signal_connect (G_OBJECT (getWidget()), "action-triggered",
		                  G_CALLBACK (wizard_action_cb), this);

		busyCursor();
		ygtk_wizard_set_help_text (wizard, onlineUpdateMode() ? _(patch_help) : _(pkg_help));
		createToolsButton();

		YGDialog *dialog = YGDialog::currentDialog();
		dialog->setCloseCallback (confirm_cb, this);

		m_package_selector = new PackageSelector (wizard, onlineUpdateMode(), repoMgrEnabled());
		gtk_container_add (GTK_CONTAINER (wizard), m_package_selector->getWidget());

		Ypp::get()->setInterface (this);
		Ypp::get()->addPkgListener (this);
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
			yuiMilestone() << "Closing PackageSelector with 'accept'" << endl;
			YGUI::ui()->sendEvent (new YMenuEvent ("accept"));
		}
		else if (!strcmp (action, "cancel")) {
			yuiMilestone() << "Closing PackageSelector with 'cancel'" << endl;
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
             GTK_BUTTONS_NONE, "%s", _("Changes not saved!"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s",
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
		GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
			(GtkDialogFlags) 0, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
			"%s %s", package->name().c_str(), _("License Agreement"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
			"%s", _("Do you accept the terms of this license?"));
        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

		GtkWidget *license_view, *license_window;
		license_view = ygtk_html_wrap_new();
		ygtk_html_wrap_set_text (license_view, license.c_str(), FALSE);

		license_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (license_window),
			                            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type
			(GTK_SCROLLED_WINDOW (license_window), GTK_SHADOW_IN);
		gtk_container_add (GTK_CONTAINER (license_window), license_view);

		GtkBox *vbox = GTK_BOX (GTK_DIALOG(dialog)->vbox);
		gtk_box_pack_start (vbox, license_window, TRUE, TRUE, 6);

		gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
		gtk_window_set_default_size (GTK_WINDOW (dialog), 550, 450);
		gtk_widget_show_all (dialog);

		gint ret = gtk_dialog_run (GTK_DIALOG (dialog));
		bool confirmed = (ret == GTK_RESPONSE_YES);

		gtk_widget_destroy (dialog);
		return confirmed;
	}

	virtual bool resolveProblems (const std::list <Ypp::Problem *> &problems)
	{
		// we can't use ordinary radio buttons, as gtk+ enforces that in a group
		// one must be selected...

		#define DETAILS_PAD  25
		enum ColumnAlias {
			SHOW_TOGGLE_COL, ACTIVE_TOGGLE_COL, TEXT_COL, WEIGHT_TEXT_COL,
			TEXT_PAD_COL, APPLY_PTR_COL
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
		GtkTreeStore *store = gtk_tree_store_new (8, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
			G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_POINTER);
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
					ACTIVE_TOGGLE_COL, FALSE, TEXT_COL, solution->description.c_str(),
					APPLY_PTR_COL, &solution->apply, -1);
				if (!solution->details.empty()) {
					gtk_tree_store_append (store, &solution_iter, &problem_iter);
					gtk_tree_store_set (store, &solution_iter, SHOW_TOGGLE_COL, FALSE,
						TEXT_COL, solution->details.c_str(), TEXT_PAD_COL, DETAILS_PAD, -1);
				}
			}
		}

		// interface
		GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
                                                            GtkDialogFlags (0), GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, "%s",
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

	virtual bool allowRestrictedRepo (const std::list <std::pair <const Ypp::Package *, const Ypp::Repository *> > &pkgs)
	{
		std::string text;
		std::list < std::pair <const Ypp::Package *, const Ypp::Repository *> >::const_iterator it;
		for (it = pkgs.begin(); it != pkgs.end(); it++) {
			const Ypp::Package *pkg = it->first;
			const Ypp::Repository *repo = it->second;
			if (!text.empty())
				text += "\n\n";
			text += pkg->name() + "\n<i>" + repo->name + "</i>";
		}

		// interface
		GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
			GtkDialogFlags (0), GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, "%s",
			_("Dependencies from Filtered Repositories"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s",
			_("The following packages are necessary dependencies that are not provided "
			  "by the filtered repository. Install them?"));
		gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			GTK_STOCK_NO, GTK_RESPONSE_NO,
			GTK_STOCK_YES, GTK_RESPONSE_YES, NULL);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

		GtkWidget *label = gtk_label_new (text.c_str());
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
		gtk_label_set_selectable (GTK_LABEL (label), TRUE);
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0);

		GtkWidget *scroll = gtk_viewport_new (NULL, NULL);
		gtk_viewport_set_shadow_type (GTK_VIEWPORT (scroll), GTK_SHADOW_IN);
		gtk_container_add (GTK_CONTAINER (scroll), label);

		gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), scroll);

		gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
		gtk_window_set_default_size (GTK_WINDOW (dialog), -1, 480);
		gtk_widget_show_all (dialog);

		bool confirm = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES);
		gtk_widget_destroy (dialog);
		return confirm;
	}

	virtual void packageModified (Ypp::Package *package)
	{
		// FIXME: this is mostly a hack; the thing is that GtkTreeSelection doesn't
		// signal when a selected row changes values. Anyway, to be done differently.
		m_package_selector->packageModified (package);
	}

	YGWIDGET_IMPL_COMMON

	// Utilities
	void createToolsButton()
	{
		struct inner {
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
			static void import_file_cb (GtkMenuItem *item, YGPackageSelector *pThis)
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
			static void export_file_cb (GtkMenuItem *item, YGPackageSelector *pThis)
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
				gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                                                          "%s", msg.c_str());
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
				    	msg = _( "Failed to create dependency resolver test case.\n"
							"Please check disk space and permissions for");
						msg += " <tt>";
						msg += dirname;
						msg += "</tt>";
						errorMsg ("Error", msg.c_str());
				    }
				}
			}
		};

		GtkWidget *button, *popup, *item;
		button = ygtk_menu_button_new_with_label (_("Tools"));
		popup = gtk_menu_new();

		item = gtk_menu_item_new_with_label (_("Import List..."));
		gtk_menu_shell_append (GTK_MENU_SHELL (popup), item);
		g_signal_connect (G_OBJECT (item), "activate",
		                  G_CALLBACK (inner::import_file_cb), this);
		item = gtk_menu_item_new_with_label (_("Export List..."));
		gtk_menu_shell_append (GTK_MENU_SHELL (popup), item);
		g_signal_connect (G_OBJECT (item), "activate",
		                  G_CALLBACK (inner::export_file_cb), this);
		gtk_menu_shell_append (GTK_MENU_SHELL (popup), gtk_separator_menu_item_new());
		item = gtk_menu_item_new_with_label (_("Generate Dependency Testcase..."));
		gtk_menu_shell_append (GTK_MENU_SHELL (popup), item);
		g_signal_connect (G_OBJECT (item), "activate",
		                  G_CALLBACK (inner::create_solver_testcase_cb), this);

		ygtk_menu_button_set_popup (YGTK_MENU_BUTTON (button), popup);
		ygtk_wizard_set_extra_button (YGTK_WIZARD (getWidget()), button);
		gtk_widget_show (button);
		gtk_widget_show_all (popup);
	}
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

