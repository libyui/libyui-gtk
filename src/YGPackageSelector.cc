/* Yast-GTK */
#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "YPackageSelector.h"

#include <zypp/ui/Status.h>
#include <zypp/ui/Selectable.h>
#include <zypp/ResObject.h>
#include <zypp/Package.h>
#include <zypp/Selection.h>
#include <zypp/Pattern.h>
#include <zypp/Language.h>
#include <zypp/Patch.h>
#include <zypp/ZYppFactory.h>
#include <zypp/ResPoolProxy.h>

class YGPackageSelector : public YPackageSelector, public YGWidget
{
	GtkWidget *m_list;

public:
	YGPackageSelector (const YWidgetOpt &opt, YGWidget *parent)
		: YPackageSelector (opt)
		, YGWidget (this, parent, true, GTK_TYPE_VBOX, NULL)
	{
		// Package selector list widget
		GtkListStore *store = gtk_list_store_new (2, G_TYPE_BOOLEAN, G_TYPE_STRING);
		m_list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

		{
			GtkTreeViewColumn *column;
			GtkCellRenderer *renderer = gtk_cell_renderer_toggle_new();
			column = gtk_tree_view_column_new_with_attributes ("",
			             renderer, "active", 0, NULL);
			g_signal_connect (G_OBJECT (renderer), "toggled",
			                  G_CALLBACK (toggle_cb), this);
			gtk_tree_view_insert_column (GTK_TREE_VIEW (m_list), column, 0);

			column = gtk_tree_view_column_new_with_attributes ("Packages",
			             gtk_cell_renderer_text_new(), "text", 1, NULL);
			gtk_tree_view_insert_column (GTK_TREE_VIEW (getWidget()), column, 1);
		}

		// Buttons
		GtkWidget *install_button;
		install_button = gtk_button_new_with_mnemonic ("_Proceed");

		// Widgets events
		g_signal_connect (G_OBJECT (install_button), "clicked",
		                  G_CALLBACK (install_cb), this);

		// Layout
		gtk_box_pack_start (GTK_BOX (getWidget()), m_list, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (getWidget()), install_button, FALSE, FALSE, 0);
		gtk_widget_show_all (getWidget());
	}

	virtual ~YGPackageSelector() {}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_KEYBOARD_FOCUS

	// callbacks
	static void toggle_cb (GtkCellRendererToggle *renderer, gchar *path_str,
	                       YGPackageSelector *pThis)
	{
		GtkTreeModel* model = GTK_TREE_MODEL (pThis->m_list);
		GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
		gint *column = (gint*) g_object_get_data (G_OBJECT (renderer), "column");
		GtkTreeIter iter;
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_path_free (path);

		gboolean state;
		gtk_tree_model_get (model, &iter, column, &state, -1);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, !state, -1);
	}

	static void install_cb (GtkButton *install_button, YGPackageSelector *pThis)
	{
		// TODO
	}
};

YWidget *
YGUI::createPackageSelector (YWidget *parent, YWidgetOpt &opt,
                             const YCPString &floppyDevice)
{
	// TODO floppyDevice
	return new YGPackageSelector (opt, YGWidget::get (parent));
}

YWidget *
YGUI::createPkgSpecial (YWidget *parent, YWidgetOpt &opt,
                        const YCPString &subwidget)
{
	// TODO subwidget
	return new YGPackageSelector (opt, YGWidget::get (parent));
}
