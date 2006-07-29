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

public:
	YGDumbTab (const YWidgetOpt &opt, YGWidget *parent)
		: YDumbTab (opt),
		  YGWidget (this, parent, true, GTK_TYPE_NOTEBOOK, NULL)
	{
		IMPL
		m_fixed = gtk_fixed_new();
		m_label_req.width = m_label_req.height = 0;

		g_signal_connect (G_OBJECT (getWidget()), "switch-page",
		                  G_CALLBACK (changed_tab_cb), this);
	}

	virtual GtkFixed* getFixed()
	{
		IMPL
		return GTK_FIXED (m_fixed);
	}

	virtual void addTab (const YCPString &label_text)
	{
		IMPL
		string str = YGUtils::mapKBAccel (label_text->value_cstr());
		GtkWidget *label = gtk_label_new (str.c_str());

		GtkNotebook *notebook = GTK_NOTEBOOK (getWidget());
		g_signal_handlers_block_by_func (notebook, (gpointer) changed_tab_cb, this);
		gtk_notebook_append_page (notebook, m_fixed, label);
		g_signal_handlers_unblock_by_func (notebook, (gpointer) changed_tab_cb, this);

		gtk_widget_show_all (getWidget());
		gtk_widget_show_all (m_fixed);

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

	virtual int getSelectedTabIndex()
	{
		IMPL
		return gtk_notebook_get_current_page (GTK_NOTEBOOK (getWidget()));
	}

	virtual void setSelectedTab (int index)
	{
		IMPL
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

		long childWidth  = max (0L, width);
		long childHeight = max (0L, height);

		if (hasChildren()) {
			int border = GTK_CONTAINER (getWidget())->border_width;
			YContainerWidget::child(0)->setSize
				(childWidth  - 2*xthickness() - 2*border,
				 childHeight - 2*ythickness() - 2*border - m_label_req.height);
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
	                            gint tab, YGDumbTab *pThis)
	{
		YCPValue id = pThis->_tabs[tab].id();
		YGUI::ui()->sendEvent (new YMenuEvent (id));
	}
};

YWidget *
YGUI::createDumbTab (YWidget *parent, YWidgetOpt &opt)
{
	IMPL;
	return new YGDumbTab (opt, YGWidget::get (parent));
}
