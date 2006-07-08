#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "YGUtils.h"
#include "YDumbTab.h"

class YGDumbTab : public YDumbTab, public YGWidget
{
	GtkWidget *m_fixed;

public:
	YGDumbTab (const YWidgetOpt &opt, YGWidget *parent)
		: YDumbTab (opt),
		  YGWidget (this, parent, true, GTK_TYPE_NOTEBOOK, NULL)
	{
		IMPL
		m_fixed = gtk_fixed_new();

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

		g_signal_handlers_block_by_func (getWidget(), (gpointer) changed_tab_cb, this);
		gtk_notebook_append_page (GTK_NOTEBOOK (getWidget()), m_fixed, label);
		g_signal_handlers_unblock_by_func (getWidget(), (gpointer) changed_tab_cb, this);

		gtk_widget_show_all (getWidget());
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

	virtual void setSize (long newWidth, long newHeight)
	{
		IMPL
		doSetSize (newWidth, newHeight);

		long newChildWidth  = max (0L, newWidth);
		long newChildHeight = max (0L, newHeight);

		if (numChildren() > 0) {
			int border = GTK_CONTAINER (getWidget())->border_width;
			YContainerWidget::child(0)->setSize (newChildWidth - 2*xthickness() - border,
				newChildHeight - 2*ythickness() - border);
		}
	}

	virtual long nicesize (YUIDimension dim)
	{
		IMPL
		long niceSize = numChildren() ? YContainerWidget::child(0)->nicesize (dim) : 0;

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
