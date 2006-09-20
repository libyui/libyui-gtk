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
	GtkWidget *m_fixed;

	GtkWidget *m_last_tab;
	vector <GtkWidget *> m_tab_widgets;

public:
	YGDumbTab (const YWidgetOpt &opt, YGWidget *parent)
		: YDumbTab (opt),
		  YGWidget (this, parent, true, GTK_TYPE_NOTEBOOK, NULL)
	{
		IMPL
		m_fixed = gtk_fixed_new();
		gtk_widget_show (m_fixed);
		g_object_ref (G_OBJECT (m_fixed));
		gtk_object_sink (GTK_OBJECT (m_fixed));

		m_last_tab = 0;
		m_label_req.width = m_label_req.height = 0;

		g_signal_connect (G_OBJECT (getWidget()), "switch-page",
		                  G_CALLBACK (changed_tab_cb), this);
	}

	~YGDumbTab()
	{
		IMPL
		gtk_widget_destroy (m_fixed);
		g_object_unref (G_OBJECT (m_fixed));

	}

	virtual GtkFixed* getFixed()
	{
		IMPL
		return GTK_FIXED (m_fixed);
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
			gtk_container_remove (GTK_CONTAINER (m_last_tab), m_fixed);

		GtkWidget *tab = m_tab_widgets [tab_nb];
		gtk_container_add (GTK_CONTAINER (tab), m_fixed);
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

	// YWidget
	YGWIDGET_IMPL_SET_ENABLING

	virtual void setSize (long width, long height)
	{
		IMPL
		doSetSize (width, height);

		if (hasChildren()) {
			int border = GTK_CONTAINER (getWidget())->border_width;
			width -= 2*xthickness() + 2*border;
			height -= 2*ythickness() + 2*border + m_label_req.height;

			YContainerWidget::child(0)->setSize (width, height);
		}
	}

	virtual long nicesize (YUIDimension dim)
	{
		IMPL
		long niceSize = 0;
		if (hasChildren())
			niceSize = YContainerWidget::child(0)->nicesize (dim);

		if (dim == YD_HORIZ)
			niceSize = MAX (niceSize, m_label_req.width);
		else
			niceSize += m_label_req.height;

		niceSize += GTK_CONTAINER (getWidget())->border_width;
		niceSize += thickness (dim) * 2;
		return niceSize;
	}

	static void changed_tab_cb (GtkNotebook *notebook, GtkNotebookPage *page,
	                            gint tab_nb, YGDumbTab *pThis)
	{
		YCPValue id = pThis->_tabs[tab_nb].id();
		YGUI::ui()->sendEvent (new YMenuEvent (id));

		pThis->change_tab (tab_nb);
	}
};

YWidget *
YGUI::createDumbTab (YWidget *parent, YWidgetOpt &opt)
{
	IMPL
	return new YGDumbTab (opt, YGWidget::get (parent));
}
