/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "YGUtils.h"
#include "YDumbTab.h"

#include "ygtkratiobox.h"

class YGDumbTab : public YDumbTab, public YGWidget
{
	GtkWidget *m_containee;
	GtkWidget *m_last_tab;

public:
	YGDumbTab (YWidget *parent)
		: YDumbTab (parent),
		  YGWidget (this, parent, true, GTK_TYPE_NOTEBOOK, NULL)
	{
		IMPL
		m_containee = gtk_event_box_new();
		gtk_widget_show (m_containee);
		g_object_ref (G_OBJECT (m_containee));
		gtk_object_sink (GTK_OBJECT (m_containee));

		m_last_tab = 0;
		// GTK+ keeps the notebook size set to the biggset page. We can't
		// do this since this is dynamic, but at least don't let the notebook
		// reduce its size.
		ygtk_adj_size_set_only_expand (YGTK_ADJ_SIZE (m_adj_size), TRUE);

		g_signal_connect (G_OBJECT (getWidget()), "switch-page",
		                  G_CALLBACK (changed_tab_cb), this);
	}

	~YGDumbTab()
	{
		IMPL
		gtk_widget_destroy (m_containee);
		g_object_unref (G_OBJECT (m_containee));
	}

	virtual void addItem (YItem *item)
	{
		GtkWidget *tab_label, *image = 0, *label;

		string str = YGUtils::mapKBAccel (item->label());
		label = gtk_label_new (str.c_str());
		gtk_widget_show (label);

		if (item->hasIconName()) {
			GdkPixbuf *pixbuf = YGUtils::loadPixbuf (item->iconName());
			if (pixbuf)
				image = gtk_image_new_from_pixbuf (pixbuf);
		}

		if (image) {
			tab_label = gtk_hbox_new (FALSE, 6);
			gtk_box_pack_start (GTK_BOX (tab_label), image, FALSE, TRUE, 0);
			gtk_box_pack_start (GTK_BOX (tab_label), label, TRUE, TRUE, 0);
		}
		else
			tab_label = label;
		gtk_widget_show_all (tab_label);

		GtkNotebook *notebook = GTK_NOTEBOOK (getWidget());
		g_signal_handlers_block_by_func (notebook, (gpointer) changed_tab_cb, this);

		GtkWidget *empty = gtk_event_box_new();

		g_object_set_data (G_OBJECT (empty), "yitem", item);
		gtk_widget_show (empty);

		gtk_notebook_append_page (notebook, empty, tab_label);

		if (!m_last_tab)
			change_tab (0);

		g_signal_handlers_unblock_by_func (notebook, (gpointer) changed_tab_cb, this);
	}

	virtual void deleteAllItems()
	{
		GList *children = gtk_container_get_children (GTK_CONTAINER (getWidget()));
		for (GList *i = children; i; i = i->next) {
			GtkWidget *child = (GtkWidget *) i->data;
			gtk_container_remove (GTK_CONTAINER (getWidget()), child);
		}
		g_list_free (children);
	}

	// to re-use the same widget in all tabs (m_fixed), we will remove and
	// add to the tabs' child as tabs are changed
	void change_tab (int tab_nb)
	{
		if (m_last_tab)
			gtk_container_remove (GTK_CONTAINER (m_last_tab), m_containee);

		GtkNotebook *notebook = GTK_NOTEBOOK (getWidget());
		int nb = gtk_notebook_get_current_page (notebook);
		GtkWidget *tab = gtk_notebook_get_nth_page (notebook, nb);

		gtk_container_add (GTK_CONTAINER (tab), m_containee);
		m_last_tab = tab;
	}

	virtual YItem *selectedItem()
	{
		IMPL
		GtkNotebook *notebook = GTK_NOTEBOOK (getWidget());
		int nb = gtk_notebook_get_current_page (notebook);
		GtkWidget *child = gtk_notebook_get_nth_page (notebook, nb);
		return (YItem *) g_object_get_data (G_OBJECT (child), "yitem");
	}

	virtual void selectItem (YItem *item, bool selected)
	{
		IMPL
		if (selected) {
			GList *children = gtk_container_get_children (GTK_CONTAINER (getWidget()));
			int nb = 0;
			for (GList *i = children; i; i = i->next) {
				GtkWidget *child = (GtkWidget *) i->data;
				if (g_object_get_data (G_OBJECT (child), "yitem") == item) {
					change_tab (nb);
					g_signal_handlers_block_by_func (getWidget(),
						(gpointer) changed_tab_cb, this);
					gtk_notebook_set_current_page (GTK_NOTEBOOK (getWidget()), nb);
					g_signal_handlers_unblock_by_func (getWidget(),
						(gpointer) changed_tab_cb, this);
					break;
				}
				nb++;
			}
			g_list_free (children);
		}
	}

	static void changed_tab_cb (GtkNotebook *notebook, GtkNotebookPage *page,
	                            gint tab_nb, YGDumbTab *pThis)
	{
		GtkWidget *child = gtk_notebook_get_nth_page (notebook, tab_nb);
		YItem *item = (YItem *) g_object_get_data (G_OBJECT (child), "yitem");

		YGUI::ui()->sendEvent (new YMenuEvent (item));
		pThis->change_tab (tab_nb);
	}

	YGWIDGET_IMPL_COMMON
	YGWIDGET_IMPL_CHILD_ADDED (m_containee)
	YGWIDGET_IMPL_CHILD_REMOVED (m_containee)
};

YDumbTab *YGOptionalWidgetFactory::createDumbTab (YWidget *parent)
{
	IMPL
	return new YGDumbTab (parent);
}

