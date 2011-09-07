/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* YGtkPkgMenuBar, menu bar */
// check the header file for information about this widget

/*
  Textdomain "gtk"
 */

#include "YGi18n.h"
#include "config.h"
#include "YGDialog.h"
#include "YGUtils.h"
#include "ygtkpkgproductdialog.h"
#include <gtk/gtk.h>
#include "ygtkrichtext.h"

#include <zypp/ui/Status.h>
#include <zypp/ui/Selectable.h>
#include <zypp/ResObject.h>
#include <zypp/Package.h>
#include <zypp/Pattern.h>
#include <zypp/Patch.h>
#include <zypp/Product.h>
#include <zypp/ZYppFactory.h>
#include <zypp/ResPoolProxy.h>

typedef zypp::ResPoolProxy			ZyppPool;
typedef zypp::ResPoolProxy::const_iterator	ZyppPoolIterator;
inline ZyppPool		zyppPool()		{ return zypp::getZYpp()->poolProxy();	}
template<class T> ZyppPoolIterator zyppBegin()	{ return zyppPool().byKindBegin<T>();	}
template<class T> ZyppPoolIterator zyppEnd()	{ return zyppPool().byKindEnd<T>();	}
inline ZyppPoolIterator zyppProductsBegin()	{ return zyppBegin<zypp::Product>();	}
inline ZyppPoolIterator zyppProductsEnd()	{ return zyppEnd<zypp::Product>();	}
typedef zypp::ui::Selectable::Ptr		ZyppSel;
typedef zypp::ResObject::constPtr		ZyppObj;
typedef zypp::Product::constPtr			ZyppProduct;
inline ZyppProduct	tryCastToZyppProduct( ZyppObj zyppObj )
{ return zypp::dynamic_pointer_cast<const zypp::Product>( zyppObj ); }

enum {
	INSTALLED_COLUMN, TEXT_COLUMN, VERSION_COLUMN, VENDOR_COLUMN, DESCRIPTION_COLUMN, TOTAL_COLUMNS
};

static void selection_changed_cb (GtkTreeSelection *selection, YGtkRichText *description)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		char *text;
		gtk_tree_model_get (model, &iter, DESCRIPTION_COLUMN, &text, -1);
		ygtk_rich_text_set_text (description, text);
		g_free (text);
	}
	else
		ygtk_rich_text_set_plain_text (description, "");
}

YGtkPkgProductDialog::YGtkPkgProductDialog()
{
	GtkListStore *store = gtk_list_store_new (TOTAL_COLUMNS, G_TYPE_BOOLEAN,
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	for (ZyppPoolIterator it = zyppProductsBegin(); it != zyppProductsEnd(); it++) {
		ZyppSel sel = *it;
		ZyppProduct prod = tryCastToZyppProduct( sel->theObj() );

		std::string text (sel->name() + "\n<small>" + prod->summary() + "</small>");

		ZyppObj available = sel->candidateObj();
		ZyppObj installed = sel->installedObj();

		std::string version;
		if (!!available && !!installed && (available->edition() != installed->edition())) {
			version = available->edition().asString() + "\n<small>";
			version += installed->edition().asString() + "</small>";
		}
		else if (!!available)
			version = available->edition().asString();
		else if (!!installed)
			version = installed->edition().asString();

		std::string description;
		if (!!available) {
			description += std::string ("<p><b>") + _("Candidate provides:") + " </b>";
			description += available->dep (zypp::Dep::PROVIDES).begin()->asString();
		}
		if (!!installed) {
			description += std::string ("<p><b>") + _("Installed provides:") + " </b>";
			description += installed->dep (zypp::Dep::PROVIDES).begin()->asString();
		}


		GtkTreeIter iter;
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, INSTALLED_COLUMN,
			sel->candidateObj().isSatisfied() || sel->hasInstalledObj(),
			TEXT_COLUMN, text.c_str(), VERSION_COLUMN, version.c_str(),
			VENDOR_COLUMN, prod->vendor().c_str(), DESCRIPTION_COLUMN,
			description.c_str(), -1);
    }

	GtkWidget *description = ygtk_rich_text_new();
	GtkWidget *description_scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (description_scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (
		GTK_SCROLLED_WINDOW (description_scroll), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (description_scroll), description);

	GtkWidget *view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	GtkTreeView *tview = GTK_TREE_VIEW (view);
	g_object_unref (G_OBJECT (store));
	gtk_tree_view_set_search_column (tview, TEXT_COLUMN);
	gtk_tree_view_set_rules_hint (tview, TRUE);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_toggle_new();
	column = gtk_tree_view_column_new_with_attributes (
		NULL, renderer, "active", INSTALLED_COLUMN, NULL);
        gtk_cell_renderer_set_sensitive(renderer, FALSE);
	gtk_tree_view_append_column (tview, column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		_("Name"), renderer, "markup", TEXT_COLUMN, NULL);
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_append_column (tview, column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		_("Version"), renderer, "markup", VERSION_COLUMN, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_append_column (tview, column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		_("Vendor"), renderer, "text", VENDOR_COLUMN, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_append_column (tview, column);

	GtkTreeSelection *selection = gtk_tree_view_get_selection (tview);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
	g_signal_connect (G_OBJECT (selection), "changed",
	                  G_CALLBACK (selection_changed_cb), description);

	GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (
		GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (scroll), view);

	GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
		// Translators: same as "Listing of Products"
		GtkDialogFlags (0), GTK_MESSAGE_OTHER, GTK_BUTTONS_CLOSE, _("Products Listing"));
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
	gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 600, 500);

	GtkWidget *vpaned = gtk_vpaned_new();
	gtk_paned_pack1 (GTK_PANED (vpaned), scroll, TRUE, FALSE);
	gtk_paned_pack2 (GTK_PANED (vpaned), description_scroll, FALSE, TRUE);
	YGUtils::setPaneRelPosition (vpaned, .70);
	gtk_widget_set_vexpand (vpaned, TRUE);
	GtkContainer *content = GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog)));
	gtk_container_add (content, vpaned);

	gtk_widget_show_all (dialog);
	m_dialog = dialog;
}

YGtkPkgProductDialog::~YGtkPkgProductDialog()
{ gtk_widget_destroy (m_dialog); }

void YGtkPkgProductDialog::popup()
{ gtk_dialog_run (GTK_DIALOG (m_dialog)); }

