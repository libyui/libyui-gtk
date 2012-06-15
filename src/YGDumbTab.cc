/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#define YUILogComponent "gtk"
#include <yui/Libyui_config.h>
#include "YGUI.h"
#include "YGWidget.h"
#include "YGUtils.h"
#include "YDumbTab.h"
#include <gtk/gtk.h>
#include "ygtkratiobox.h"

class YGDumbTab : public YDumbTab, public YGWidget
{
	GtkWidget *m_containee;
	GtkWidget *m_last_tab;

public:
	YGDumbTab (YWidget *parent)
		: YDumbTab (NULL),
		  YGWidget (this, parent, GTK_TYPE_NOTEBOOK, NULL)
	{
		m_containee = gtk_event_box_new();
		g_object_ref_sink (G_OBJECT (m_containee));
		gtk_widget_show (m_containee);

		m_last_tab = 0;
		// GTK+ keeps the notebook size set to the biggset page. We can't
		// do this since pages are set dynamically, but at least don't let
		// the notebook reduce its size.
		ygtk_adj_size_set_only_expand (YGTK_ADJ_SIZE (m_adj_size), TRUE);

		connect (getWidget(), "switch-page", G_CALLBACK (switch_page_cb), this);
	}

	virtual ~YGDumbTab()
	{
		gtk_widget_destroy (m_containee);
		g_object_unref (G_OBJECT (m_containee));
	}

	virtual GtkWidget *getContainer()
	{ return m_containee; }

	virtual void addItem (YItem *item)
	{
		BlockEvents block (this);
		YDumbTab::addItem (item);
		GtkWidget *tab_label, *image = 0, *label;
		label = gtk_label_new (YGUtils::mapKBAccel (item->label()).c_str());
		gtk_label_set_use_underline (GTK_LABEL (label), TRUE);
		if (item->hasIconName()) {
			string path = iconFullPath (item->iconName());
			GdkPixbuf *pixbuf = YGUtils::loadPixbuf (path);
			if (pixbuf) {
				image = gtk_image_new_from_pixbuf (pixbuf);
				g_object_unref (G_OBJECT (pixbuf));
			}
		}
		if (image) {
			tab_label = gtk_hbox_new (FALSE, 6);
			gtk_box_pack_start (GTK_BOX (tab_label), image, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (tab_label), label, TRUE, TRUE, 0);
		}
		else
			tab_label = label;
		gchar *label_id = g_strdup_printf ("label-%d", item->index());
		g_object_set_data (G_OBJECT (getWidget()), label_id, label);
		g_free (label_id);
		gtk_widget_show_all (tab_label);

		GtkNotebook *notebook = GTK_NOTEBOOK (getWidget());

		GtkWidget *page = gtk_event_box_new();
		gtk_widget_show (page);
		item->setData ((void *) page);
		g_object_set_data (G_OBJECT (page), "yitem", item);

		gtk_notebook_append_page (notebook, page, tab_label);
		selectItem (item, item->selected() || !m_last_tab /*first tab*/);
	}

	virtual void deleteAllItems()
	{
		GList *children = gtk_container_get_children (GTK_CONTAINER (getWidget()));
		for (GList *i = children; i; i = i->next)
			gtk_container_remove (GTK_CONTAINER (getWidget()), (GtkWidget *) i->data);
		g_list_free (children);
		YDumbTab::deleteAllItems();
	}

	// to re-use the same widget in all tabs (m_fixed), we will remove and
	// add to the tabs' child as tabs are changed
	void syncTabPage()
	{
		if (m_last_tab)
			gtk_container_remove (GTK_CONTAINER (m_last_tab), m_containee);

		GtkNotebook *notebook = GTK_NOTEBOOK (getWidget());
		int nb = gtk_notebook_get_current_page (notebook);
		m_last_tab = gtk_notebook_get_nth_page (notebook, nb);
		gtk_container_add (GTK_CONTAINER (m_last_tab), m_containee);
	}

	virtual YItem *selectedItem()
	{
		GtkNotebook *notebook = GTK_NOTEBOOK (getWidget());
		int nb = gtk_notebook_get_current_page (notebook);
		if (nb < 0) return NULL;
		GtkWidget *child = gtk_notebook_get_nth_page (notebook, nb);
		return (YItem *) g_object_get_data (G_OBJECT (child), "yitem");
	}

	virtual void selectItem (YItem *item, bool selected)
	{
		if (selected) {
			BlockEvents block (this);
			GtkWidget *child = (GtkWidget *) item->data();
			int page = gtk_notebook_page_num (GTK_NOTEBOOK (getWidget()), child);

			gtk_notebook_set_current_page (GTK_NOTEBOOK (getWidget()), page);
			syncTabPage();
		}
		YDumbTab::selectItem (item, selected);
	}

	virtual void shortcutChanged()
	{
		for (YItemConstIterator it = itemsBegin(); it != itemsEnd(); it++) {
			YItem *item = *it;
			gchar *label_id = g_strdup_printf ("label-%d", item->index());
			GtkWidget *label;
			label = (GtkWidget *) g_object_get_data (G_OBJECT (getWidget()), label_id);
			g_free (label_id);

			std::string text = YGUtils::mapKBAccel (item->label());
			gtk_label_set_text (GTK_LABEL (label), text.c_str());
			gtk_label_set_use_underline (GTK_LABEL (label), TRUE);
		}
	}

	// callbacks
	static void switch_page_cb (GtkNotebook *notebook, GtkWidget *page,
	                              guint tab_nb, YGDumbTab *pThis)
	{
		GtkWidget *child = gtk_notebook_get_nth_page (notebook, tab_nb);
		YItem *item = (YItem *) g_object_get_data (G_OBJECT (child), "yitem");

		pThis->YDumbTab::selectItem (item);
		YGUI::ui()->sendEvent (new YMenuEvent (item));
		pThis->syncTabPage();
	}

	YGWIDGET_IMPL_CONTAINER (YDumbTab)
};

YDumbTab *YGOptionalWidgetFactory::createDumbTab (YWidget *parent)
{ return new YGDumbTab (parent); }

