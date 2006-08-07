/* Yast-GTK */
#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "YPackageSelector.h"
#include "ygtkrichtext.h"

#include <zypp/ZYppFactory.h>

//#define TEST_DEVEL

#ifdef TEST_DEVEL
#include <set>
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
#endif

class YGPackageSelector : public YPackageSelector, public YGWidget
{
	GtkWidget *m_available_list, *m_installed_list;

public:
	YGPackageSelector (const YWidgetOpt &opt, YGWidget *parent)
		: YPackageSelector (opt),
		  YGWidget (this, parent, true, GTK_TYPE_VBOX, NULL)
	{
		setBorder (12);
		GtkWidget *packages_hbox = gtk_hbox_new (FALSE, 0);

		GtkWidget *available_box, *installed_box;
		available_box = createListWidget ("<b>Available Software:</b>",
		                                  "gtk-cdrom", m_available_list);
		installed_box = createListWidget ("<b>Installed Software:</b>",
		                                  "computer", m_installed_list);

		GtkWidget *selection_buttons_vbox, *install_button, *remove_button;
		selection_buttons_vbox = gtk_vbox_new (FALSE, 0);
		install_button = gtk_button_new_with_mnemonic ("_install >");
		remove_button = gtk_button_new_with_mnemonic ("< _remove");
		gtk_box_pack_start (GTK_BOX (selection_buttons_vbox), install_button, TRUE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (selection_buttons_vbox), remove_button, TRUE, FALSE, 0);
		gtk_container_set_border_width (GTK_CONTAINER (selection_buttons_vbox), 6);

		gtk_box_pack_start (GTK_BOX (packages_hbox), available_box, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (packages_hbox), selection_buttons_vbox, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (packages_hbox), installed_box, TRUE, TRUE, 0);

		GtkWidget *categories_view_check;
		categories_view_check = gtk_check_button_new_with_mnemonic ("View in categories");

		GtkWidget *search_hbox, *search_label, *search_entry, *search_advanced_button;
		search_hbox = gtk_hbox_new (FALSE, 0);
		search_label = gtk_label_new_with_mnemonic ("_Search:");
		search_entry = gtk_entry_new ();
		gtk_label_set_mnemonic_widget (GTK_LABEL (search_label), search_entry);
		search_advanced_button = gtk_button_new_with_mnemonic ("A_dvanced...");
		gtk_box_pack_start (GTK_BOX (search_hbox), search_label, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (search_hbox), search_entry, TRUE, TRUE, 4);
		gtk_box_pack_start (GTK_BOX (search_hbox), search_advanced_button, FALSE, FALSE, 4);

		GtkWidget *informations_expanders, *informations_window, *informations_vbox;
		informations_expanders = gtk_expander_new ("Package Information");
		informations_window = gtk_event_box_new();
		informations_vbox = gtk_vbox_new (FALSE, 0);
		gtk_container_add (GTK_CONTAINER (informations_expanders), informations_window);
		gtk_container_add (GTK_CONTAINER (informations_window), informations_vbox);
		GdkColor white = { 0, 255 << 8, 255 << 8, 255 << 8 };
		gtk_widget_modify_bg (informations_window, GTK_STATE_NORMAL, &white);

		GtkWidget *files_info_expander, *files_info_richtext,
		          *history_info_expander, *history_info_richtext;

		files_info_expander = gtk_expander_new ("File listing");
		files_info_richtext = ygtk_richtext_new ();
		ygtk_richtext_set_text (YGTK_RICHTEXT (files_info_richtext),
		                        "<i>empty for now</i>", FALSE);
		gtk_container_add (GTK_CONTAINER (files_info_expander), files_info_richtext);
		gtk_box_pack_start (GTK_BOX (informations_vbox), files_info_expander,
		                    FALSE, FALSE, 0);
		gtk_widget_modify_bg (files_info_expander, GTK_STATE_NORMAL, &white);

		history_info_expander = gtk_expander_new ("Packaging history");
		history_info_richtext = ygtk_richtext_new ();
		ygtk_richtext_set_text (YGTK_RICHTEXT (history_info_richtext),
		                        "<i>empty for now</i>", FALSE);
		gtk_container_add (GTK_CONTAINER (history_info_expander), history_info_richtext);
		gtk_box_pack_start (GTK_BOX (informations_vbox), history_info_expander,
		                    FALSE, FALSE, 0);
		gtk_widget_modify_bg (history_info_expander, GTK_STATE_NORMAL, &white);


		gtk_box_pack_start (GTK_BOX (getWidget()), packages_hbox, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (getWidget()), categories_view_check, FALSE, FALSE, 6);
		gtk_box_pack_start (GTK_BOX (getWidget()), search_hbox, FALSE, FALSE, 2);
		gtk_box_pack_start (GTK_BOX (getWidget()), informations_expanders, FALSE, FALSE, 6);
		gtk_widget_show_all (getWidget());

		// Let's start by showing all available / installed packages
		// NOTE: Yast-Qt seems to like to use Zypp for searching. We will only
		// use that for advanced search. We will generate a full list now, so
		// that the user can make simple as-you-type searches.
		// TODO: I guess we probably want to generate two models (GtkListStore
		// and GtkTreeStore, with categories), that we can easily change
		// when the user presses view with categories.

/*
		GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);

		for (zypp::ResPoolProxy::const_iterator it =
		         zyppPool().byKindBegin <zypp::Selection> ();
		     it != zyppEnd<zypp::Selection>(); it++) {
			zypp::Selection::constPtr zyppSelection =
				zypp::dynamic_pointer_cast <const zypp::Selection> ((*it)->theObj());

			if (zyppSelection) {
				if (zyppSelection->visible() && !zyppSelection->isBase()) {
					zypp::ui::Selectable::Ptr selectable = *it;
					//addPkgSelItem (*it, zyppSelection);
					printf ("selectable.name: %s\n", selectable.name.c_str());
				}
			}
			else
				printf ("!zyppSelection - shouldn't happen!\n");
		}


#if 0
		zypp::Pattern pattern (zypp::NVRAD ("*"));
		std::set<std::string> packages = pattern.install_packages();
		for (std::set<std::string>::iterator it = pattern.being();
		     it != pattern.end(); it++) {
			GtkTreeIter iter,
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, it->c_str(), -1);
		}
#endif

*/



		// append / set


		g_signal_connect (G_OBJECT (search_advanced_button), "clicked",
		                  G_CALLBACK (advanced_search_cb), this);
	}

	GtkWidget *createListWidget (const char *header, const char *icon,
	                             GtkWidget *&list)
	{
		GtkWidget *vbox, *header_hbox, *image, *label, *scrolled_window;

		vbox = gtk_vbox_new (FALSE, 0);

		scrolled_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_shadow_type
			(GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		list = gtk_tree_view_new();
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list), FALSE);
		gtk_container_add (GTK_CONTAINER (scrolled_window), list);

		header_hbox = gtk_hbox_new (FALSE, 0);
		image = gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_BUTTON);
		label = gtk_label_new_with_mnemonic (header);
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
		gtk_label_set_mnemonic_widget (GTK_LABEL (label), list);
		gtk_box_pack_start (GTK_BOX (header_hbox), image, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (header_hbox), label, FALSE, FALSE, 4);

		gtk_box_pack_start (GTK_BOX (vbox), header_hbox, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 4);

		return vbox;
	}

	virtual ~YGPackageSelector() {}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_KEYBOARD_FOCUS


#ifdef TEST_DEVEL
typedef zypp::ResPoolProxy			ZyppPool;
typedef zypp::ResPoolProxy::const_iterator	ZyppPoolIterator;
inline ZyppPool		zyppPool()		{ return zypp::getZYpp()->poolProxy();	}

template<class T> ZyppPoolIterator zyppBegin()	{ return zyppPool().byKindBegin<T>();	}
template<class T> ZyppPoolIterator zyppEnd()	{ return zyppPool().byKindEnd<T>();	}

inline ZyppPoolIterator zyppPkgBegin()		{ return zyppBegin<zypp::Package>();	}
inline ZyppPoolIterator zyppPkgEnd()		{ return zyppEnd<zypp::Package>();	}
typedef zypp::ui::Selectable::Ptr		ZyppSel;
#endif

	// callbacks
	static void advanced_search_cb (GtkButton *button, YGPackageSelector *pThis)
	{
printf("iterating through pool\n");

#ifdef TEST_DEVEL
	for ( ZyppPoolIterator it = zyppPkgBegin();
	      it != zyppPkgEnd();
	      ++it )
	{
		ZyppSel selectable = *it;

		printf ("package: %s\n", selectable.name.c_str());

	}
#endif

#if 0
		zypp::ResPool pool;
		for (zypp::ResPool::const_iterator it = pool.begin(); it != pool.end(); it++) {
			printf ("summary: %s\n", (*it)->summary().c_str());
		}
#endif
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
