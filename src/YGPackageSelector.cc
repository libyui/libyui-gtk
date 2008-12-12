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
#include "YPackageSelector.h"

// singleton like Ypp -- used for the flags
static YPackageSelector *pkg_selector = 0;

#if 1  // set 0 to disable this widget
#include "ygtkwizard.h"
#include "ygtkfindentry.h"
#include "ygtkmenubutton.h"
#include "ygtkscrolledwindow.h"
#include "ygtktogglebutton.h"
#include "ygtkhtmlwrap.h"
#include "ygtkrichtext.h"
#include "ygtkhandlebox.h"
#include "ygtktooltip.h"
#include "ygtkzyppwrapper.h"

// utilities
static GtkWidget *image_new_from_file (const char *filename)
{
	std::string file = std::string (DATADIR) + "/" + filename;
	return gtk_image_new_from_file (file.c_str());
}

#define GNOME_OPEN_PATH "/usr/bin/gnome-open"
inline bool CAN_OPEN_URIS()
{ return g_file_test (GNOME_OPEN_PATH, G_FILE_TEST_IS_EXECUTABLE); }
inline void OPEN_URI (const char *uri)
{ system ((std::string (GNOME_OPEN_PATH " ") + uri + " &").c_str()); }

static int busyI = 0;
static void busyCursor()
{
	if (!busyI)
		YGUI::ui()->busyCursor();
	// ensure the cursor is actually set and update the UI...
	while (g_main_context_iteration (NULL, FALSE)) ;
	busyI++;
}
static void normalCursor()
{
	busyI = MAX (0, busyI-1);
	if (!busyI)
		YGUI::ui()->normalCursor();
}

static gboolean is_tree_model_iter_separator_cb (GtkTreeModel *model, GtkTreeIter *iter,
                                                 gpointer data)
{
	gint col = data ? GPOINTER_TO_INT (data) : 0;
	gpointer ptr;
	gtk_tree_model_get (model, iter, col, &ptr, -1);
	return ptr == NULL;
}

static void ensure_visible_size_allocate_cb (GtkWidget *widget, GtkAllocation *alloc)
{
	GList *paths;
	if (GTK_IS_TREE_VIEW (widget)) {
		GtkTreeView *view = GTK_TREE_VIEW (widget);
		GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
		paths = gtk_tree_selection_get_selected_rows (selection, NULL);
		if (paths && !paths->next) {
			GtkTreePath *path = (GtkTreePath *) paths->data;
			gtk_tree_view_scroll_to_cell (view, path, NULL, FALSE, 0, 0);
		}
	}
	else { // if (GTK_IS_ICON_VIEW (widget))
		GtkIconView *view = GTK_ICON_VIEW (widget);
		paths = gtk_icon_view_get_selected_items (view);
		if (paths && !paths->next) {
			GtkTreePath *path = (GtkTreePath *) paths->data;
			gtk_icon_view_scroll_to_path (view, path, FALSE, 0, 0);
		}
	}
	g_list_foreach (paths, (GFunc) gtk_tree_path_free, 0);
	g_list_free (paths);
}

static void ensure_view_visible_hook (GtkWidget *widget /* tree view or icon view */)
{
	g_signal_connect_after (G_OBJECT (widget), "size-allocate",
	                        G_CALLBACK (ensure_visible_size_allocate_cb), NULL);
}

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
	bool canLock() const
	{ init(); return _allCanLock;   }

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
				_allLocked = _allUnlocked = _allCanLock = false;
		else {
			_allInstalled = _allNotInstalled = _allUpgradable = _allModified =
				_allLocked = _allUnlocked = _allCanLock = true;
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
				if ((*it)->toModify()) {
					// if modified, can't be locked or unlocked
					_allLocked = _allUnlocked = false;
				}
				else
					_allModified = false;
				if ((*it)->isLocked())
					_allUnlocked = false;
				else
					_allLocked = false;
				if (!(*it)->canLock())
					_allCanLock = false;
			}
		}
	}

	std::list <Ypp::Package *> packages;
	guint inited : 2, _allInstalled : 2, _allNotInstalled : 2, _allUpgradable : 2,
	      _allModified : 2, _allLocked : 2, _allUnlocked : 2, _allCanLock : 2;
};

// UI controls

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

	void packagesSelected (const PkgList &packages)
	{
		if (m_listener && GTK_WIDGET_REALIZED (m_bin)) {
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
					const char *tooltip, const char *icon, bool sensitive,
					void (& callback) (GtkMenuItem *item, View *pThis), View *pThis)
				{
					GtkWidget *item;
					if (icon) {
						if (label) {
							item = gtk_image_menu_item_new_with_mnemonic (label);
							GtkWidget *image;
							if (*icon == 'g')
								image = gtk_image_new_from_stock (icon, GTK_ICON_SIZE_MENU);
							else
								image = image_new_from_file (icon);
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
			bool empty = true, canLock = packages.canLock(), unlocked = packages.unlocked();
			bool locked = !unlocked && canLock;
			if (packages.notInstalled())
				inner::appendItem (menu, _("_Install"), 0, GTK_STOCK_SAVE,
				                   !locked, inner::install_cb, this), empty = false;
			if (packages.upgradable())
				inner::appendItem (menu, _("_Upgrade"), 0, GTK_STOCK_GOTO_TOP,
				                   !locked, inner::install_cb, this), empty = false;
			if (packages.installed())
				inner::appendItem (menu, _("_Remove"), 0, GTK_STOCK_DELETE,
				                   !locked, inner::remove_cb, this), empty = false;
			if (packages.modified())
				inner::appendItem (menu, _("_Undo"), 0, GTK_STOCK_UNDO,
				                   true, inner::undo_cb, this), empty = false;
			if (canLock) {
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

			gtk_menu_attach_to_widget (GTK_MENU (menu), m_widget, NULL);
			gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,  button, event_time);
			gtk_widget_show_all (menu);
		}

		static gboolean popup_key_cb (GtkWidget *widget, View *pThis)
		{ pThis->signalPopup (0, gtk_get_current_event_time()); return TRUE; }
	};
	struct ListView : public View
	{
		bool m_isTree, m_descriptiveTooltip;
		ListView (bool isTree, bool descriptiveTooltip, bool editable, PackagesView *parent)
		: View (parent), m_isTree (isTree), m_descriptiveTooltip (descriptiveTooltip)
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
			if (isTree) {
				int height = MAX (34, YGUtils::getCharsHeight (m_widget, 2));
				gtk_cell_renderer_set_fixed_size (renderer, -1, height);
			}
			g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
			gboolean reverse = gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL;
			if (reverse) {  // work-around: Pango ignored alignment flag on RTL
				gtk_widget_set_direction (m_widget, GTK_TEXT_DIR_LTR);
				g_object_set (renderer, "alignment", PANGO_ALIGN_RIGHT, NULL);
			}
			column = gtk_tree_view_column_new_with_attributes ("", renderer,
				"markup", YGtkZyppModel::NAME_DESCRIPTION_COLUMN, NULL);
			gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
			gtk_tree_view_column_set_fixed_width (column, 50 /* it will expand */);
			gtk_tree_view_column_set_expand (column, TRUE);
			gtk_tree_view_insert_column (view, column, reverse ? 0 : -1);
			gtk_tree_view_set_fixed_height_mode (view, TRUE);
			gtk_tree_view_set_show_expanders (view, FALSE);  /* would conflict with icons */

			GtkTreeSelection *selection = gtk_tree_view_get_selection (view);
			gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
			g_signal_connect (G_OBJECT (selection), "changed",
			                  G_CALLBACK (packages_selected_cb), this);
			gtk_tree_selection_set_select_function (selection, can_select_row_cb,
			                                        this, NULL);
			gtk_widget_show (m_widget);

			if (editable) {
				g_signal_connect (G_OBJECT (m_widget), "row-activated",
				                  G_CALLBACK (package_activated_cb), this);
				g_signal_connect (G_OBJECT (m_widget), "popup-menu",
					              G_CALLBACK (popup_key_cb), this);
				g_signal_connect (G_OBJECT (m_widget), "button-press-event",
					              G_CALLBACK (popup_button_cb), this);
			}
			gtk_widget_set_has_tooltip (m_widget, TRUE);
			g_signal_connect (G_OBJECT (m_widget), "query-tooltip",
			                  G_CALLBACK (query_tooltip_cb), this);
			ensure_view_visible_hook (m_widget);
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

		static void package_activated_cb (GtkTreeView *view, GtkTreePath *path,
		                                  GtkTreeViewColumn *column, View *pThis)
		{
			PkgList packages = pThis->getSelected();
			if (packages.notInstalled() || packages.upgradable())
				packages.install();
		}

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

		static gboolean query_tooltip_cb (GtkWidget *widget, gint x, gint y,
			gboolean keyboard_mode, GtkTooltip *tooltip, ListView *pThis)
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
				gtk_tree_model_get (model, &iter, YGtkZyppModel::PTR_COLUMN, &package, -1);
				if (!package) return FALSE;

				std::string text;
				if (!pThis->m_descriptiveTooltip) {
					GtkTreeViewColumn *column;
					int bx, by;
					gtk_tree_view_convert_widget_to_bin_window_coords (
						view, x, y, &bx, &by);
					gtk_tree_view_get_path_at_pos (
						view, x, y, NULL, &column, NULL, NULL);
					int status_col =
						gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL ? 1 : 0;
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
						if (package->isLocked())
							text += _("\n<i>locked: right-click to unlock</i>");
					}
				}
				else {
					text = std::string ("<b>") + package->name() + "</b>\n";
					text += package->description (GTK_MARKUP);
				}
				if (text.empty())
					return FALSE;
				gtk_tooltip_set_markup (tooltip, text.c_str());

				GdkPixbuf *pixbuf = 0;
				switch (package->type())
				{
					case Ypp::Package::LANGUAGE_TYPE: {
						std::string filename (package->icon());
						if (!filename.empty())
							pixbuf = YGUtils::loadPixbuf (filename.c_str());
						break;
					}
					default:
						break;
				}
				if (!pixbuf)
					gtk_tree_model_get (model, &iter,
						YGtkZyppModel::ICON_COLUMN, &pixbuf, -1);
				if (pixbuf) {
					gtk_tooltip_set_icon (tooltip, pixbuf);
					g_object_unref (G_OBJECT (pixbuf));
				}
				return TRUE;
			}
			return FALSE;
		}
	};
	struct IconView : public View
	{
		IconView (bool editable, PackagesView *parent)
		: View (parent)
		{
			GtkIconView *view = GTK_ICON_VIEW (m_widget = gtk_icon_view_new());
			gtk_icon_view_set_text_column (view, YGtkZyppModel::NAME_COLUMN);
			gtk_icon_view_set_pixbuf_column (view, YGtkZyppModel::ICON_COLUMN);
			gtk_icon_view_set_selection_mode (view, GTK_SELECTION_MULTIPLE);
			g_signal_connect (G_OBJECT (m_widget), "selection-changed",
				                    G_CALLBACK (packages_selected_cb), this);
			gtk_widget_show (m_widget);

			if (editable) {
				g_signal_connect (G_OBJECT (m_widget), "popup-menu",
					              G_CALLBACK (popup_key_cb), this);
				g_signal_connect_after (G_OBJECT (m_widget), "button-press-event",
					                    G_CALLBACK (popup_button_after_cb), this);
			}
			ensure_view_visible_hook (m_widget);
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
	};

GtkWidget *m_bin;
GtkTreeModel *m_model;
View *m_view;
bool m_isTree, m_descriptiveTooltip;

public:
	GtkWidget *getWidget()
	{ return m_bin; }

	PackagesView (bool useScrollWindow, bool isTree, bool descriptiveTooltip, bool enableTilesMode, bool editable)
	: m_listener (NULL), m_model (NULL), m_view (NULL), m_isTree (isTree), m_descriptiveTooltip (descriptiveTooltip)
	{
		if (useScrollWindow) {
			m_bin = ygtk_scrolled_window_new();
			if (enableTilesMode) {
				GtkWidget *buttons = gtk_vbox_new (FALSE, 0), *button;
				button = create_toggle_button ("pkg-list-mode.xpm", _("View as list"), NULL);
				gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, TRUE, 0);
				button = create_toggle_button ("pkg-tiles-mode.xpm", _("View as grid"), button);
				gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, TRUE, 0);
				gtk_widget_show_all (buttons);

				ygtk_scrolled_window_set_corner_widget (YGTK_SCROLLED_WINDOW (m_bin), buttons);
			}
		}
		else
			m_bin = gtk_event_box_new();
		setMode (LIST_MODE, editable);
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
	void setMode (ViewMode mode, bool editable)
	{
		busyCursor();
		if (m_view)
			gtk_container_remove (GTK_CONTAINER (m_bin), m_view->m_widget);
		delete m_view;
		if (mode == LIST_MODE)
			m_view = new ListView (m_isTree, m_descriptiveTooltip, editable, this);
		else // if (mode == ICON_MODE)
			m_view = new IconView (editable, this);
		gtk_container_add (GTK_CONTAINER (m_bin), m_view->m_widget);
		if (m_model)
			m_view->setModel (m_model);

		packagesSelected (PkgList());
		gtk_widget_show_all (m_bin);
		normalCursor();
	}

	void setPool (Ypp::Pool *pool)
	{
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
	GtkWidget *create_toggle_button (const char *icon, const char *tooltip, GtkWidget *member)
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

		GtkWidget *image = image_new_from_file (icon);
		gtk_container_add (GTK_CONTAINER (button), image);
		return button;
	}

	static void mode_toggled_cb (GtkToggleButton *toggle, gint nb, PackagesView *pThis)
	{ pThis->setMode ((ViewMode) nb, true); }
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
Ypp::Pool *m_pool;
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

		m_box = ygtk_handle_box_new();
		gtk_container_add (GTK_CONTAINER (m_box), vbox);
		gtk_handle_box_set_handle_position (GTK_HANDLE_BOX (m_box), GTK_POS_TOP);
		gtk_handle_box_set_snap_edge (GTK_HANDLE_BOX (m_box), GTK_POS_RIGHT);

		Ypp::QueryPool::Query *query = new Ypp::QueryPool::Query();
		query->setToModify (true);
		if (pkg_selector->updateMode())
			query->addType (Ypp::Package::PATCH_TYPE);
		m_pool = new Ypp::QueryPool (query);
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

	void startHack()  // call after init, after you did a show_all in the dialog
	{
		UpdateVisible();
		// ugly: signal modified for all entries to allow them to hide undo buttons
		GList *i;
		Ypp::Pool::Iter it;
		for (it = m_pool->getFirst(), i = m_entries; it && i;
		     it = m_pool->getNext (it), i = i->next)
			((Entry *) i->data)->modified (m_pool->get (it));
	}

	void UpdateVisible()
	{
		m_entries != NULL ? gtk_widget_show (m_box) : gtk_widget_hide (m_box);
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
};

class Collections
{
public:
	struct Listener {
		virtual void collectionChanged (bool immediate) = 0;
	};

private:
	struct View
	{
		virtual void writeQuery (Ypp::QueryPool::Query *query) = 0;

		Collections::Listener *m_listener;
		View (Collections::Listener *listener)
		: m_listener (listener)
		{
			m_box = gtk_vbox_new (FALSE, 2);
		}
		virtual ~View() {}

		GtkWidget *getWidget()
		{ return m_box; }

		void signalChanged()      { m_listener->collectionChanged (true); }
		void signalChangedDelay() { m_listener->collectionChanged (false); }

	protected:
		GtkWidget *m_box;
	};

	struct StoreView : public View
	{
	protected:
		GtkWidget *m_view, *m_scroll;
		enum Column { TEXT_COL, ICON_COL, ENABLED_COL, PTR_COL, TOTAL_COLS };

		virtual void doBuild (GtkTreeStore *store) = 0;
		virtual void writeQuery (Ypp::QueryPool::Query *query,
		                           const std::list <gpointer> &ptr) = 0;

	public:
		StoreView (Collections::Listener *listener)
		: View (listener)
		{
			m_scroll = gtk_scrolled_window_new (NULL, NULL);
			gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (m_scroll),
					                             GTK_SHADOW_IN);
			gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (m_scroll),
					                        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
			m_view = NULL;

			gtk_box_pack_start (GTK_BOX (m_box), m_scroll, TRUE, TRUE, 0);

			// parent constructor should call build()
		}

		void build (bool tree_mode, bool with_icons, bool multi_selection,
		            bool do_tooltip)
		{
			if (m_view)
				gtk_container_remove (GTK_CONTAINER (m_scroll), m_view);

			m_view = gtk_tree_view_new();
			GtkTreeView *view = GTK_TREE_VIEW (m_view);
			gtk_tree_view_set_headers_visible (view, FALSE);
			gtk_tree_view_set_search_column (view, TEXT_COL);
			if (do_tooltip)
				gtk_tree_view_set_tooltip_column (view, TEXT_COL);
			gtk_tree_view_set_show_expanders (view, tree_mode);

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
				G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_POINTER);
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
			gtk_tree_store_set (store, &iter, TEXT_COL, _("All"), ENABLED_COL, TRUE, -1);
			doBuild (store);

			selectFirstItem();
			unblock();

			gtk_container_add (GTK_CONTAINER (m_scroll), m_view);
			gtk_widget_show (m_view);
			if (!tree_mode)
				ensure_view_visible_hook (m_view);
		}

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
			pThis->signalChanged();
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

		virtual void writeQuery (Ypp::QueryPool::Query *query)
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
	};

	struct Categories : public StoreView
	{
		bool m_rpmGroups;
	public:
		Categories (Collections::Listener *listener)
		: StoreView (listener), m_rpmGroups (false)
		{
			if (!pkg_selector->updateMode()) {
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
			Ypp::Package::Type type = pkg_selector->updateMode() ?
				Ypp::Package::PATCH_TYPE : Ypp::Package::PACKAGE_TYPE;
			if (!m_rpmGroups && !pkg_selector->updateMode())
				first_category = Ypp::get()->getFirstCategory2 (type);
			else
				first_category = Ypp::get()->getFirstCategory (type);
			inner::populate (store, NULL, first_category, this);
			if (!m_rpmGroups && !pkg_selector->updateMode()) {
				GtkTreeView *view = GTK_TREE_VIEW (m_view);
				GtkTreeIter iter;
				gtk_tree_store_append (store, &iter, NULL);
				gtk_tree_view_set_row_separator_func (view,
					is_tree_model_iter_separator_cb, NULL, NULL);

				gtk_tree_store_append (store, &iter, NULL);
				gtk_tree_store_set (store, &iter, TEXT_COL, _("Recommended"),
					ICON_COL, GTK_STOCK_ABOUT,  PTR_COL, GINT_TO_POINTER (1),
					ENABLED_COL, TRUE, -1);
				gtk_tree_store_append (store, &iter, NULL);
				gtk_tree_store_set (store, &iter, TEXT_COL, _("Suggested"),
					ICON_COL, GTK_STOCK_ABOUT,  PTR_COL, GINT_TO_POINTER (2),
					ENABLED_COL, TRUE, -1);
			}
		}

		virtual void writeQuery (Ypp::QueryPool::Query *query,
		                           const std::list <gpointer> &ptrs)
		{
			gpointer ptr = ptrs.empty() ? NULL : ptrs.front();
			int nptr = GPOINTER_TO_INT (ptr);
			if (nptr == 1)
				query->setIsRecommended (true);
			else if (nptr == 2)
				query->setIsSuggested (true);
			else if (ptr) {
				Ypp::Node *node = (Ypp::Node *) ptr;
				if (m_rpmGroups || pkg_selector->updateMode())
					query->addCategory (node);
				else
					query->addCategory2 (node);
			}
		}

		static void rpm_groups_toggled_cb (GtkToggleButton *button, Categories *pThis)
		{
			pThis->m_rpmGroups = gtk_toggle_button_get_active (button);
			pThis->build (pThis->m_rpmGroups, !pThis->m_rpmGroups, false, false);
			pThis->signalChanged();
		}
	};

	struct Repositories : public StoreView
	{
	public:
		Repositories (Collections::Listener *listener)
		: StoreView (listener)
		{
			if (pkg_selector->repoMgrEnabled()) {
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

		virtual ~Repositories()
		{
			Ypp::get()->setFavoriteRepository (NULL);
		}

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

		virtual void writeQuery (Ypp::QueryPool::Query *query,
		                         const std::list <gpointer> &ptrs)
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
		{
			YGUI::ui()->sendEvent (new YMenuEvent ("repo_mgr"));
		}
	};

	struct Pool : public View, public PackagesView::Listener
	{
	private:
		PackagesView * m_view;
		GtkWidget *m_buttons_box;

	public:
		Pool (Collections::Listener *listener, Ypp::Package::Type type)
		: View (listener)
		{
			m_view = new PackagesView (true, true, true, false, true);
			m_view->setPool (new Ypp::TreePool (type));
			m_view->setListener (this);

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
			gtk_box_pack_start (GTK_BOX (m_box), m_view->getWidget(), TRUE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (m_box), m_buttons_box, FALSE, TRUE, 0);
		}

		virtual ~Pool()
		{
			delete m_view;
		}

		virtual void packagesSelected (const PkgList &selection)
		{
			gtk_widget_set_sensitive (m_buttons_box, selection.notInstalled());
			signalChanged();
		}

		virtual void writeQuery (Ypp::QueryPool::Query *query)
		{
			PkgList selected = m_view->getSelected();
			for (PkgList::const_iterator it = selected.begin();
			     it != selected.end(); it++)
				query->addCollection (*it);
			if (selected.empty())
				query->setClear();
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
	enum Type { GROUPS_TYPE, PATTERNS_TYPE, LANGUAGES_TYPE, REPOS_TYPE };

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

	void setType (Type type)
	{
		if (m_view)
			gtk_container_remove (GTK_CONTAINER (m_bin), m_view->getWidget());
		delete m_view;

		switch (type)
		{
			case GROUPS_TYPE:
				m_view = new Categories (m_listener);
				break;
			case PATTERNS_TYPE:
				m_view = new Pool (m_listener, Ypp::Package::PATTERN_TYPE);
				break;
			case LANGUAGES_TYPE:
				m_view = new Pool (m_listener, Ypp::Package::LANGUAGE_TYPE);
				break;
			default:
				m_view = new Repositories (m_listener);
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

	public:
		GtkWidget *getWidget()
		{ return m_box; }

		StatusButtons (Filters *filters)
		: m_filters (filters), m_selected (AVAILABLE)
		{
			m_box = gtk_hbox_new (FALSE, 6);

			GtkWidget *button;
			GSList *group;
			button = createButton (_("_Available"), "pkg-available.png", NULL);
			group = ygtk_toggle_button_get_group (YGTK_TOGGLE_BUTTON (button));
			gtk_box_pack_start (GTK_BOX (m_box), button, TRUE, TRUE, 0);
			if (!pkg_selector->updateMode()) {
				button = createButton (_("_Upgrades"), "pkg-installed-upgradable.png", group);
				gtk_box_pack_start (GTK_BOX (m_box), button, TRUE, TRUE, 0);
			}
			button = createButton (_("_Installed"), "pkg-installed.png", group);
			gtk_box_pack_start (GTK_BOX (m_box), button, TRUE, TRUE, 0);
			button = createButton (_("A_ll"), 0, group);
			gtk_box_pack_start (GTK_BOX (m_box), button, FALSE, TRUE, 0);
		}

		Status getActive()
		{
			return m_selected;
		}

		GtkWidget *getSpecificWidget (Status status)
		{
			GtkWidget *button;
			GList *children = gtk_container_get_children (GTK_CONTAINER (m_box));
			button = (GtkWidget *) g_list_nth_data (children, (int) status);
			g_list_free (children);
			return button;
		}

		GtkWidget *createButton (const char *label, const char *icon, GSList *group)
		{
			GtkWidget *button = ygtk_toggle_button_new (group);
#if 0
			gtk_button_set_label (GTK_BUTTON (button), label);
			gtk_button_set_use_underline (GTK_BUTTON (button), TRUE);
			if (icon)
				gtk_button_set_image (GTK_BUTTON (button), image_new_from_file (icon));
#else
			GtkWidget *box = gtk_hbox_new (FALSE, 6);
			if (icon)
				gtk_box_pack_start (GTK_BOX (box), image_new_from_file (icon),
					FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (box), gtk_label_new_with_mnemonic (label),
				TRUE, TRUE, 0);
			gtk_container_add (GTK_CONTAINER (button), box);
#endif
			if (!group)
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
			gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
			g_signal_connect (G_OBJECT (button), "toggle-changed",
			                  G_CALLBACK (status_toggled_cb), this);
			return button;
		}

		static void status_toggled_cb (GtkToggleButton *toggle, gint nb, StatusButtons *pThis)
		{
			pThis->m_selected = (Status) nb;
			if (pkg_selector->updateMode() && nb >= 1)
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
	GtkWidget *m_name, *m_type;
	Listener *m_listener;
	guint timeout_id;
	bool m_nameChanged;  // clear name field if some other changes

public:
	GtkWidget *getCollectionWidget() { return m_collection->getWidget(); }
	GtkWidget *getStatusesWidget()   { return m_statuses->getWidget(); }
	GtkWidget *getNameWidget()       { return m_name; }
	GtkWidget *getTypeWidget()       { return m_type; }

	Filters()
	: m_listener (NULL), timeout_id (0), m_nameChanged (false)
	{
		m_statuses = new StatusButtons (this);

		m_name = ygtk_find_entry_new();
		gtk_widget_set_tooltip_markup (m_name,
			_("<b>Package search:</b> Use spaces to separate your keywords. They "
			"will be matched against RPM <i>name</i> and <i>summary</i> attributes. "
			"Other criteria attributes are available by pressing the search icon.\n"
			"(usage example: \"yast dhcp\" will return yast's dhcpd tool)"));
		ygtk_find_entry_insert_item (YGTK_FIND_ENTRY (m_name), _("Filter by name & summary"),
		                             GTK_STOCK_FIND, NULL);
		ygtk_find_entry_insert_item (YGTK_FIND_ENTRY (m_name), _("Filter by description"),
		                             GTK_STOCK_EDIT, NULL);
		ygtk_find_entry_insert_item (YGTK_FIND_ENTRY (m_name), _("Filter by file"),
		                             GTK_STOCK_OPEN, NULL);
		ygtk_find_entry_insert_item (YGTK_FIND_ENTRY (m_name), _("Filter by author"),
		                             GTK_STOCK_ABOUT, NULL);
		ygtk_find_entry_insert_item (YGTK_FIND_ENTRY (m_name), _("Filter by novelty (in days)"),
			GTK_STOCK_NEW, _("Number of days since the package was built by the repository."));
		g_signal_connect (G_OBJECT (m_name), "changed",
		                  G_CALLBACK (name_changed_cb), this);
		g_signal_connect (G_OBJECT (m_name), "menu-item-selected",
		                  G_CALLBACK (name_item_changed_cb), this);

		m_type = gtk_combo_box_new_text();
		if (pkg_selector->updateMode())
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_type), _("Severity"));
		else {
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_type), _("Groups"));
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_type), _("Patterns"));
			gtk_combo_box_append_text (GTK_COMBO_BOX (m_type), _("Languages"));
			gtk_widget_set_tooltip_markup (m_type,
				_("Packages can be organized in:\n"
				"<b>Groups:</b> simple categorization of packages by purpose.\n"
				"<b>Patterns:</b> assists in installing all packages necessary "
				"for several working environments.\n"
				"<b>Languages:</b> adds another language to the system.\n"
				"<b>Repositories:</b> catalogues what the several configured "
				"repositories have available."));
		}
		gtk_combo_box_append_text (GTK_COMBO_BOX (m_type), _("Repositories"));
		gtk_combo_box_set_active (GTK_COMBO_BOX (m_type), 0);
		g_signal_connect (G_OBJECT (m_type), "changed",
		                  G_CALLBACK (type_changed_cb), this);

		m_collection = new Collections (this);
		m_collection->setType (Collections::GROUPS_TYPE);
	}

	~Filters()
	{
		if (timeout_id)
			g_source_remove (timeout_id);
	}

private:
	void clearNameEntry()
	{
		g_signal_handlers_block_by_func (m_name, (gpointer) name_changed_cb, this);
		g_signal_handlers_block_by_func (m_name, (gpointer) name_item_changed_cb, this);
		gtk_entry_set_text (GTK_ENTRY (m_name), "");
		ygtk_find_entry_select_item (YGTK_FIND_ENTRY (m_name), 0);
		g_signal_handlers_unblock_by_func (m_name, (gpointer) name_changed_cb, this);
		g_signal_handlers_unblock_by_func (m_name, (gpointer) name_item_changed_cb, this);
	}

	static void type_changed_cb (GtkComboBox *combo, Filters *pThis)
	{
		busyCursor();
		pThis->clearNameEntry();
		int type = gtk_combo_box_get_active (combo);
		if (pkg_selector->updateMode() && type == 1)
			type = Collections::REPOS_TYPE;
		pThis->m_collection->setType ((Collections::Type) type);
		pThis->signalChanged();
		normalCursor();
	}

	static void name_changed_cb (YGtkFindEntry *entry, Filters *pThis)
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
		pThis->signalChangedDelay();
	}
	static void name_item_changed_cb (YGtkFindEntry *entry, gint nb, Filters *pThis)
	{
		const gchar *text = "";
		if (nb == 4) text = "7";  // novelty is weird; show usage case
		g_signal_handlers_block_by_func (entry, (gpointer) name_changed_cb, pThis);
		gtk_entry_set_text (GTK_ENTRY (entry), text);
		g_signal_handlers_unblock_by_func (entry, (gpointer) name_changed_cb, pThis);
		gtk_editable_set_position (GTK_EDITABLE (entry), -1);
		ygtk_find_entry_set_state (entry, TRUE);

		pThis->signalChanged();
	}
	static void field_changed_cb (gpointer widget, Filters *pThis)
	{ pThis->signalChangedDelay(); }

	virtual void collectionChanged (bool immediate)
	{ immediate ? signalChanged() : signalChangedDelay(); }

	void signalChanged()
	{
		if (!m_listener) return;
		busyCursor();

		// create query
		Ypp::QueryPool::Query *query = new Ypp::QueryPool::Query();
		if (pkg_selector->updateMode())
			query->addType (Ypp::Package::PATCH_TYPE);
		else
			query->addType (Ypp::Package::PACKAGE_TYPE);

		const char *name = gtk_entry_get_text (GTK_ENTRY (m_name));
		if (*name) {
			gint item = ygtk_find_entry_get_selected_item (YGTK_FIND_ENTRY (m_name));
			if (item == 4) {  // novelty
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

			if (item == 0) {  // tip: the user may be searching for patterns
				static bool shown_pattern_tip = false;
				if (!pkg_selector->updateMode() && !shown_pattern_tip &&
					gtk_combo_box_get_active (GTK_COMBO_BOX (m_type)) == 0 &&
					(m_statuses->getActive() == StatusButtons::AVAILABLE ||
					 m_statuses->getActive() == StatusButtons::ALL)) {
					Ypp::QueryPool::Query *query = new Ypp::QueryPool::Query();
					query->addType (Ypp::Package::PATTERN_TYPE);
					query->addNames (name, ' ', true, false, false, false, false, true);
					query->setIsInstalled (false);
					Ypp::QueryPool pool (query);
					if (!pool.empty()) {
						shown_pattern_tip = true;
						//std::string first = pool.getName (pool.getFirst());
						std::string text =
							_("Patterns are available that can\n"
							"assist you in the installment\nof");
						text += " <i>" + std::string (name) + "</i> ";
						text += _("related packages.");
						ygtk_tooltip_show_at_widget (m_type, YGTK_POINTER_UP_LEFT,
							text.c_str(), GTK_STOCK_DIALOG_INFO);
					}
				}
			}
			else if (item == 2) {
				if (m_statuses->getActive() == StatusButtons::AVAILABLE) {
					const char *text =
						_("The file filter is only\n"
						"applicable to <b>installed</b> packages.");
					ygtk_tooltip_show_at_widget (
						m_statuses->getSpecificWidget (StatusButtons::INSTALLED),
						YGTK_POINTER_DOWN_RIGHT, text, GTK_STOCK_DIALOG_ERROR);
				}
			}
		}

		switch (m_statuses->getActive())
		{
			case StatusButtons::AVAILABLE:
				if (pkg_selector->updateMode())
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
		m_listener->doQuery (query);
		normalCursor();
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
		gtk_button_set_focus_on_click (GTK_BUTTON (m_remove_button), FALSE);
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
		gtk_button_set_focus_on_click (GTK_BUTTON (m_install_button), FALSE);
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
		m_locked_image = image_new_from_file ("pkg-locked.png");
		m_unlocked_image = image_new_from_file ("pkg-unlocked.png");
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

		Ypp::Package *single_package = packages.single() ? packages.front() : NULL;
		if (packages.installed()) {
			gtk_widget_show (m_installed_box);
			if (single_package) {
				std::string text;
				const Ypp::Package::Version *version = single_package->getInstalledVersion();
				if (version) {
					text = version->number;
					text += "  <small>(" + version->arch + ")</small>";
				}
				gtk_label_set_markup (GTK_LABEL (m_installed_version), text.c_str());
			}
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
				const Ypp::Repository *favoriteRepo = Ypp::get()->favoriteRepository();
				for (int i = 0; single_package->getAvailableVersion (i); i++) {
					const Ypp::Package::Version *version = single_package->getAvailableVersion (i);
					string text = version->number + "  <small>(" + version->arch + ")</small>\n";
					string repo;
					if (version->repo)
						repo = YGUtils::truncate (version->repo->name,
							MAX (20, version->number.length() + version->arch.length() + 4), 0);
					else
						yuiError() << "Repository of package '" << single_package->name()
						           << "' unknown\n";
					text += "<small>" + repo + "</small>";
					GtkTreeIter iter;
					gtk_list_store_append (GTK_LIST_STORE (model), &iter);
					gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, text.c_str(), -1);
					if (version->repo == favoriteRepo) {
						gtk_combo_box_set_active (GTK_COMBO_BOX (m_available_versions), i);
						favoriteRepo = 0;  // select only the 1st hit
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
			}
			else if (packages.notInstalled()) {
				gtk_combo_box_append_text (GTK_COMBO_BOX (m_available_versions), "(several)");
				gtk_combo_box_set_active (GTK_COMBO_BOX (m_available_versions), 0);
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
			int nb = gtk_combo_box_get_active (combo);
			if (nb == -1) return;

			const Ypp::Package::Version *version;
			version = package->getAvailableVersion (nb);
			assert (version != NULL);

			const char *installLabel = _("Ins_tall");
			if (package->isInstalled()) {
				if (version->cmp > 0)
					installLabel = _("Up_grade");
				else if (version->cmp == 0)
					installLabel = _("Re-ins_tall");
				else //if (version->cmp < 0)
					installLabel = _("Down_grade");
			}
			gtk_button_set_label (GTK_BUTTON (pThis->m_install_button), installLabel);

			gchar *tooltip = g_strdup_printf ("%s  <small>(%s)\n%s</small>",
				version->number.c_str(), version->arch.c_str(),
				version->repo ? version->repo->name.c_str() : "-repo error-");
			gtk_widget_set_tooltip_markup (GTK_WIDGET (combo), tooltip);
			g_free (tooltip);
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
		YGUtils::setWidgetFont (label, PANGO_STYLE_NORMAL, PANGO_WEIGHT_BOLD, PANGO_SCALE_MEDIUM);
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

		GtkWidget *getWidget() { return expander ? expander : text; }

		TextExpander (const char *name)
		{
			text = ygtk_rich_text_new();
			if (name) {
				string str = string ("<b>") + name + "</b>";
				expander = gtk_expander_new (str.c_str());
				gtk_expander_set_use_markup (GTK_EXPANDER (expander), TRUE);
				gtk_container_add (GTK_CONTAINER (expander), text);
				}
			else
				expander = NULL;
		}

		void setText (const std::string &str)
		{
			ygtk_rich_text_set_text (YGTK_RICH_TEXT (text), str.c_str(), FALSE);
			if (expander)
				str.empty() ? gtk_widget_hide (expander) : gtk_widget_show (expander);
		}
	};

	struct DepExpander {
		GtkWidget *expander, *table, *requires, *provides;

		GtkWidget *getWidget() { return expander; }

		DepExpander (const char *name)
		{
			requires = ygtk_rich_text_new();
			provides = ygtk_rich_text_new();

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
				requires_str += package->requires (true);
				YGUtils::replace (provides_str, "\n", 1, "<br>");
				requires_str += "</blockquote>";
				provides_str += "<br><blockquote>";
				provides_str += package->provides (true);
				YGUtils::replace (requires_str, "\n", 1, "<br>");
				provides_str += "</blockquote>";
				ygtk_rich_text_set_text (YGTK_RICH_TEXT (requires), requires_str.c_str(), FALSE);
				ygtk_rich_text_set_text (YGTK_RICH_TEXT (provides), provides_str.c_str(), FALSE);
				gtk_widget_show (expander);
			}
			else
				gtk_widget_hide (expander);
		}
	};

GtkWidget *m_widget, *m_icon, *m_icon_frame;
TextExpander *m_description, *m_filelist, *m_changelog, *m_authors, *m_support;
DepExpander *m_dependencies;
GtkWidget *m_contents_expander;
PackagesView *m_contents;
Listener *m_listener;

public:
	void setListener (Listener *listener)
	{ m_listener = listener; }

	GtkWidget *getWidget()
	{ return m_widget; }

	PackageDetails()
	: m_listener (NULL)
	{
		GtkWidget *vbox;
		m_widget = createWhiteViewPort (&vbox);

		GtkWidget *hbox = gtk_hbox_new (FALSE, 6);
		m_description = new TextExpander (NULL);
		g_signal_connect (G_OBJECT (m_description->text), "link-clicked",
		                  G_CALLBACK (link_pressed_cb), this);
		gtk_box_pack_start (GTK_BOX (hbox), m_description->getWidget(), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), createIconWidget (&m_icon, &m_icon_frame),
		                    FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

		if (!pkg_selector->updateMode()) {
			m_filelist = new TextExpander (_("File List"));
			m_changelog = new TextExpander (_("Changelog"));
			m_authors = new TextExpander (_("Authors"));
			m_support = new TextExpander (_("Support"));
			m_dependencies = new DepExpander (_("Dependencies"));
			m_contents = NULL;
			gtk_box_pack_start (GTK_BOX (vbox), m_filelist->getWidget(), FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (vbox), m_changelog->getWidget(), FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (vbox), m_authors->getWidget(), FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (vbox), m_dependencies->getWidget(), FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (vbox), m_support->getWidget(), FALSE, TRUE, 0);
			if (CAN_OPEN_URIS())
				g_signal_connect (G_OBJECT (m_filelist->text), "link-clicked",
						          G_CALLBACK (link_pressed_cb), this);
		}
		else {
			m_filelist = m_changelog = m_authors = m_support = NULL;
			m_dependencies = NULL;
			m_contents = new PackagesView (false, false, true, false, true);
			m_contents_expander = gtk_expander_new (_("<b>Applies to</b>"));
			gtk_expander_set_use_markup (GTK_EXPANDER (m_contents_expander), TRUE);
			gtk_container_add (GTK_CONTAINER (m_contents_expander), m_contents->getWidget());
			gtk_box_pack_start (GTK_BOX (vbox), m_contents_expander, FALSE, TRUE, 0);
		}
	}

	~PackageDetails()
	{
		delete m_filelist;
		delete m_changelog;
		delete m_authors;
		delete m_support;
		delete m_dependencies;
		delete m_contents;
	}

	void setPackages (const PkgList &packages)
	{
		Ypp::Package *package = packages.front();

		gtk_widget_hide (m_icon_frame);
		if (packages.single()) {
			string description = "<b>" + package->name() + "</b><br>";
			description += package->description (HTML_MARKUP);
			m_description->setText (description);

			if (m_filelist)  m_filelist->setText (package->filelist (true));
			if (m_changelog) m_changelog->setText (package->changelog());
			if (m_authors)   m_authors->setText (package->authors (true));
			if (m_support)   m_support->setText (package->support (true));
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
			string description;
			if (!packages.empty()) {
				description = "Selected:";
				description += "<ul>";
				for (PkgList::const_iterator it = packages.begin(); it != packages.end(); it++)
					description += "<li><b>" + (*it)->name() + "</b></li>";
				description += "</ul>";
			}
			m_description->setText (description);
			if (m_filelist)  m_filelist->setText ("");
			if (m_changelog) m_changelog->setText ("");
			if (m_authors)   m_authors->setText ("");
			if (m_support)   m_support->setText ("");
			if (m_dependencies) m_dependencies->setPackage (NULL);
		}
		if (m_contents) {
			Ypp::QueryPool::Query *query = new Ypp::QueryPool::Query();
			if (packages.empty()) {
				gtk_widget_hide (m_contents_expander);
				query->setClear();
			}
			else {
				gtk_widget_show (m_contents_expander);
				query->addType (Ypp::Package::PACKAGE_TYPE);
				for (PkgList::const_iterator it = packages.begin(); it != packages.end(); it++)
					query->addCollection (*it);
			}
			m_contents->setQuery (query);
		}
	}

private:
	static void link_pressed_cb (GtkWidget *text, const gchar *link, PackageDetails *pThis)
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
			OPEN_URI (link);
	}

	void scrollTop()
	{
		GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW (m_widget);
		YGUtils::scrollWidget (gtk_scrolled_window_get_vadjustment (scroll), true);
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

// either use YGtkBarGraph or GtkTreeView (w/ GtkCellRendererProgress)
//#define USE_BARGRAPH
#include "ygtkbargraph.h"

class DiskView : public Ypp::Disk::Listener
{
#ifdef USE_BARGRAPH
	static GtkWidget *DiskList_new (bool framed)
	{
		GtkWidget *widget = gtk_table_new (0, 0, FALSE);
		gtk_container_set_border_width (GTK_CONTAINER (widget), 2);
		gtk_table_set_col_spacings (GTK_TABLE (widget), 4);
		return widget;
	}

	static void DiskList_setup (GtkWidget *widget, int i, const std::string &path,
		int usage_percent, int delta_percent, const std::string &description,
		const std::string &changed, bool is_full)
	{
		if (i == 0) {
			GList *children = gtk_container_get_children (GTK_CONTAINER (widget));
			for (GList *i = children; i; i = i->next)
				gtk_container_remove (GTK_CONTAINER (widget), (GtkWidget *) i->data);
			g_list_free (children);
		}
		gtk_table_resize (GTK_TABLE (widget), 2, (i+1)*2);

		GtkWidget *path_label, *usage_label, *graph;
		path_label = gtk_label_new (path.c_str());
		gtk_misc_set_alignment (GTK_MISC (path_label), 0, 0.5);
		gtk_label_set_ellipsize (GTK_LABEL (path_label), PANGO_ELLIPSIZE_END);
		gtk_widget_set_tooltip_text (path_label, path.c_str());
		gtk_widget_set_size_request (path_label, 40, -1);
		usage_label = gtk_label_new (("used: " + description).c_str());
		gtk_misc_set_alignment (GTK_MISC (usage_label), 1, 0.5);
		YGUtils::setWidgetFont (usage_label, PANGO_STYLE_ITALIC, PANGO_WEIGHT_NORMAL, PANGO_SCALE_SMALL);
		graph = ygtk_bar_graph_new();
		gtk_widget_set_size_request (graph, 150, -1);

		YGtkBarGraph *ygraph = YGTK_BAR_GRAPH (graph);
		ygtk_bar_graph_set_style (ygraph, TRUE);
		bool show_delta = delta_percent > 0;
		int entries = show_delta ? 3 : 2;
		ygtk_bar_graph_create_entries (ygraph, entries);
		ygtk_bar_graph_setup_entry (ygraph, 0, "", usage_percent);
		if (show_delta)
			ygtk_bar_graph_setup_entry (ygraph, 1, "+", delta_percent);
		ygtk_bar_graph_setup_entry (ygraph, entries-1, "", 100 - (delta_percent+usage_percent));

// TODO: set some nice colors (do gradient for space)
		if (is_full) {
			GdkColor red = { 0, 0xffff, 0xffff, 0xffff };
			ygtk_bar_graph_customize_bg (ygraph, 0, &red);
		}

		GtkAttachOptions regularOpt = GtkAttachOptions (GTK_FILL);
		GtkAttachOptions expandOpt = GtkAttachOptions (GTK_EXPAND|GTK_FILL);
		int row = i*2;
		gtk_table_attach (GTK_TABLE (widget), path_label, 0, 1, row, row+1,
		                  regularOpt, regularOpt, 0, 0);
		gtk_table_attach (GTK_TABLE (widget), graph, 1, 2, row, row+1,
		                  expandOpt, regularOpt, 0, 0);
		gtk_table_attach (GTK_TABLE (widget), usage_label, 1, 2, row+1, row+2,
		                  regularOpt, regularOpt, 0, 6);
		gtk_widget_show_all (widget);
	}

#else  // GtkTreeView
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
#endif

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

class PackageSelector : public Filters::Listener, public PackagesView::Listener,
                         public PackageDetails::Listener
{
friend class YGPackageSelector;
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

	PackageSelector()
	{
		m_packages = new PackagesView (true, false, false, true, true);
		m_filters = new Filters();
		m_control = new PackageControl (m_filters);
		m_details = new PackageDetails();
		m_disk = new DiskView();
		m_changes = new ChangesPane();
		m_packages->setListener (this);
		m_filters->setListener (this);
		m_details->setListener (this);

		GtkWidget *categories_box = gtk_vbox_new (FALSE, 4);
		gtk_box_pack_start (GTK_BOX (categories_box), m_filters->getTypeWidget(), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (categories_box), m_filters->getCollectionWidget(),
		                    TRUE, TRUE, 0);

		GtkWidget *packages_label = gtk_label_new_with_mnemonic (_("Packages _listing:"));
		gtk_label_set_mnemonic_widget (GTK_LABEL (packages_label),
			GTK_BIN (m_packages->getWidget())->child);
		gtk_misc_set_alignment (GTK_MISC (packages_label), 0, 0.5);

		GtkWidget *packages_bar = gtk_hbox_new (FALSE, 0);
		gtk_box_pack_start (GTK_BOX (packages_bar), packages_label, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (packages_bar), m_filters->getNameWidget(), FALSE, TRUE, 0);

		GtkWidget *packages_box = gtk_vbox_new (FALSE, 4);
		gtk_box_pack_start (GTK_BOX (packages_box), packages_bar, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (packages_box), m_packages->getWidget(), TRUE, TRUE, 0);

		m_details_box = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (m_details_box), m_details->getWidget(), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_details_box), m_control->getWidget(), FALSE, TRUE, 0);

		GtkWidget *categories_pane = gtk_hpaned_new();
		gtk_paned_pack1 (GTK_PANED (categories_pane), categories_box, FALSE, TRUE);
		gtk_paned_pack2 (GTK_PANED (categories_pane), packages_box, TRUE, FALSE);
		gtk_paned_set_position (GTK_PANED (categories_pane), 180);

		GtkWidget *details_pane = gtk_vpaned_new();
		gtk_paned_pack1 (GTK_PANED (details_pane), categories_pane, TRUE, FALSE);
		gtk_paned_pack2 (GTK_PANED (details_pane), m_details_box, FALSE, FALSE);
		gtk_paned_set_position (GTK_PANED (details_pane), 30000 /* minimal size */);

		m_box = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (m_box), m_filters->getStatusesWidget(),
		                    FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (m_box), details_pane, TRUE, TRUE, 0);

		gtk_widget_show_all (m_box);
		m_changes->startHack();
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

		YGtkWizard *wizard = YGTK_WIZARD (YGWidget::get (pkg_selector)->getWidget());
		ygtk_wizard_enable_button (wizard, wizard->next_button, TRUE);
	}
};

#include "pkg-selector-help.h"

class YGPackageSelector : public YPackageSelector, public YGWidget,
                          public Ypp::Interface, public Ypp::PkgListener
{
PackageSelector *m_package_selector;

public:
	YGPackageSelector (YWidget *parent, long mode)
		: YPackageSelector (NULL, mode),
		  YGWidget (this, parent, YGTK_TYPE_WIZARD, NULL)
	{
		pkg_selector = this;
		setBorder (0);
		YGDialog::currentDialog()->setMinSize (650, 750);  // enlarge

		YGtkWizard *wizard = YGTK_WIZARD (getWidget());
		ygtk_wizard_set_header_icon (wizard,
			THEMEDIR "/icons/22x22/apps/yast-software.png");
		const char *title = onlineUpdateMode() ? _("Online Update") : _("Software Manager");
		ygtk_wizard_set_header_text (wizard, title);
		YGDialog::currentDialog()->setTitle (title);
		ygtk_wizard_set_help_text (wizard, _("Please wait..."));

		ygtk_wizard_set_button_label (wizard,  wizard->abort_button,
		                              _("_Cancel"), GTK_STOCK_CANCEL);
		ygtk_wizard_set_button_str_id (wizard, wizard->abort_button, "cancel");
		ygtk_wizard_set_button_label (wizard,  wizard->back_button, NULL, NULL);
		ygtk_wizard_set_button_label (wizard,  wizard->next_button,
		                              _("A_pply"), GTK_STOCK_APPLY);
		ygtk_wizard_set_button_str_id (wizard, wizard->next_button, "accept");
		ygtk_wizard_enable_button (wizard, wizard->next_button, FALSE);
		g_signal_connect (G_OBJECT (wizard), "action-triggered",
		                  G_CALLBACK (wizard_action_cb), this);

		YGDialog *dialog = YGDialog::currentDialog();
		dialog->setCloseCallback (confirm_cb, this);

		busyCursor();
		m_package_selector = new PackageSelector();
		ygtk_wizard_set_child (YGTK_WIZARD (wizard), m_package_selector->getWidget());

		createToolsButton();
		ygtk_wizard_set_information_widget (YGTK_WIZARD (wizard),
			m_package_selector->m_changes->getWidget(), FALSE);

		Ypp::get()->setInterface (this);
		Ypp::get()->addPkgListener (this);
		ygtk_wizard_set_help_text (wizard, onlineUpdateMode() ? _(patch_help) : _(pkg_help));
		normalCursor();
	}

	virtual ~YGPackageSelector()
	{
		delete m_package_selector;
		ygtk_zypp_model_finish();
		pkg_selector = 0;
	}

protected:
	static void wizard_action_cb (YGtkWizard *wizard, gpointer id,
	                              gint id_type, YGPackageSelector *pThis)
	{
		IMPL
		const gchar *action = (gchar *) id;

		if (!strcmp (action, "accept")) {
			yuiMilestone() << "Closing PackageSelector with 'accept'" << endl;
#if YAST2_VERSION >= 2017013
			if (pThis->confirmUnsupported())
				if (!pThis->askConfirmUnsupported())
					return;
#endif
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

	bool askConfirmUnsupported()
	{
		Ypp::QueryPool::Query *query = new Ypp::QueryPool::Query();
		query->addType (Ypp::Package::PACKAGE_TYPE);
		query->setIsInstalled (false);
		query->setToModify (true);
		query->setIsUnsupported (true);

		Ypp::QueryPool *pool = new Ypp::QueryPool (query);
		if (!pool->empty()) {
			// show which packages are unsupported
			GtkWidget *dialog;
			dialog = gtk_message_dialog_new
				(YGDialog::currentWindow(),
				 GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING,
				 GTK_BUTTONS_NONE, "%s", _("Unsupported Packages"));
			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s",
				_("Please realize that the following software is either unsupported or "
				"requires an additional customer contract for support."));
			gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
							        GTK_STOCK_OK, GTK_RESPONSE_YES, NULL);

			PackagesView *view = new PackagesView (true, false, false, false, true);
			view->setPool (pool);
			gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), view->getWidget());

			bool ok = gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES;
			gtk_widget_destroy (dialog);
			return ok;
		}
		else {
			delete pool;
			return true;
		}
	}

	bool acceptText (Ypp::Package *package, const std::string &title,
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

	virtual bool acceptLicense (Ypp::Package *package, const std::string &license)
	{
		return acceptText (package, _("License Agreement"),
			_("Do you accept the terms of this license?"), license, true);
	}

	virtual void notifyMessage (Ypp::Package *package, const std::string &msg)
	{
		acceptText (package, _("Notification"), "", msg, false);
	}

	virtual bool resolveProblems (const std::list <Ypp::Problem *> &problems)
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
			_("The following packages have necessary dependencies that are not provided "
			  "by the filtered repository. Install them?"));
		gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			GTK_STOCK_NO, GTK_RESPONSE_NO,
			GTK_STOCK_YES, GTK_RESPONSE_YES, NULL);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

		GtkWidget *label = gtk_label_new (text.c_str());
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
		gtk_label_set_selectable (GTK_LABEL (label), TRUE);
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0);

		GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
		                                GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroll), label);
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
		gtk_widget_show_all (popup);

		GtkWidget *box = gtk_hbox_new (FALSE, 12);
		gtk_box_pack_start (GTK_BOX (box), button, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (box),
			m_package_selector->m_disk->getWidget(), FALSE, TRUE, 0);
		gtk_widget_show_all (box);
		ygtk_wizard_set_extra_button (YGTK_WIZARD (getWidget()), box);
	}
};

#else
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

