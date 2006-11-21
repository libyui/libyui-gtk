//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "YGUtils.h"
#include "YDumbTab.h"

class YGDumbTab : public YDumbTab, public YGWidget
{
	GtkRequisition m_label_req;

	GtkWidget *m_containee;
	GtkWidget *m_last_tab;
	vector <GtkWidget *> m_tab_widgets;

public:
	YGDumbTab (const YWidgetOpt &opt, YGWidget *parent)
		: YDumbTab (opt),
		  YGWidget (this, parent, true, GTK_TYPE_NOTEBOOK, NULL)
	{
		IMPL
		m_containee = gtk_event_box_new();
		gtk_widget_show (m_containee);
		g_object_ref (G_OBJECT (m_containee));
		gtk_object_sink (GTK_OBJECT (m_containee));

		m_last_tab = 0;
		m_label_req.width = m_label_req.height = 0;

		g_signal_connect (G_OBJECT (getWidget()), "switch-page",
		                  G_CALLBACK (changed_tab_cb), this);
	}

	~YGDumbTab()
	{
		IMPL
		gtk_widget_destroy (m_containee);
		g_object_unref (G_OBJECT (m_containee));
	}

	virtual void addTab (const YCPString &label_text)
	{
		IMPL
		// the tab label
		string str = YGUtils::mapKBAccel (label_text->value_cstr());
		GtkWidget *label = gtk_label_new (str.c_str());
		gtk_widget_show (label);

		GtkNotebook *notebook = GTK_NOTEBOOK (getWidget());
		g_signal_handlers_block_by_func (notebook, (gpointer) changed_tab_cb, this);

		GtkWidget *empty = gtk_event_box_new();
		gtk_widget_show (empty);

		gtk_notebook_append_page (notebook, empty, label);
		m_tab_widgets.push_back (empty);

		if (!m_last_tab)
			change_tab (0);

		g_signal_handlers_unblock_by_func (notebook, (gpointer) changed_tab_cb, this);

		// for setsize and nicesize...
		int focus_width;
		gtk_widget_style_get (getWidget(), "focus-line-width", &focus_width, NULL);

		GtkRequisition req;
		gtk_widget_size_request (label, &req);
		req.width  += (focus_width + notebook->tab_hborder + 2) * 2 + 1;
		req.height += (focus_width + notebook->tab_vborder + 2) * 2;

		m_label_req.width += req.width;
		m_label_req.height = MAX (req.height, m_label_req.height);
	}

	// to re-use the same widget in all tabs (m_fixed), we will remove and
	// add to the tabs' child as tabs are changed
	void change_tab (int tab_nb)
	{
		if (m_last_tab)
			gtk_container_remove (GTK_CONTAINER (m_last_tab), m_containee);

		GtkWidget *tab = m_tab_widgets [tab_nb];
		gtk_container_add (GTK_CONTAINER (tab), m_containee);
		m_last_tab = tab;
	}

	virtual int getSelectedTabIndex()
	{
		IMPL
		return gtk_notebook_get_current_page (GTK_NOTEBOOK (getWidget()));
	}

	virtual void setSelectedTab (int index)
	{
		IMPL
		change_tab (index);

		g_signal_handlers_block_by_func (getWidget(), (gpointer) changed_tab_cb, this);
		gtk_notebook_set_current_page (GTK_NOTEBOOK (getWidget()), index);
		g_signal_handlers_unblock_by_func (getWidget(), (gpointer) changed_tab_cb, this);
	}

	static void changed_tab_cb (GtkNotebook *notebook, GtkNotebookPage *page,
	                            gint tab_nb, YGDumbTab *pThis)
	{
		YCPValue id = pThis->_tabs[tab_nb].id();
		YGUI::ui()->sendEvent (new YMenuEvent (id));

		pThis->change_tab (tab_nb);
	}

	YGWIDGET_IMPL_COMMON
	YGWIDGET_IMPL_CHILD_ADDED (m_containee)
	YGWIDGET_IMPL_CHILD_REMOVED (m_containee)
};

YWidget *
YGUI::createDumbTab (YWidget *parent, YWidgetOpt &opt)
{
	IMPL
	return new YGDumbTab (opt, YGWidget::get (parent));
}
