/* Yast GTK */
#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "ygtkbargraph.h"

#include "YBarGraph.h"

class YGBarGraph : public YBarGraph, public YGWidget
{
public:
	YGBarGraph (const YWidgetOpt &opt, YGWidget *parent)
	: YBarGraph (opt)
	, YGWidget (this, parent, true, YGTK_TYPE_BAR_GRAPH, NULL)
	{ }

	// YBarGraph
	virtual void doUpdate()
	{
		ygtk_bar_graph_create_entries (YGTK_BAR_GRAPH (getWidget()), segments());
		for (int i = 0; i < segments(); i++)
			ygtk_bar_graph_setup_entry (YGTK_BAR_GRAPH (getWidget()), i,
			                             label(i).c_str(), value(i));
	}

	// YWidget
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_NICESIZE
};

YWidget *
YGUI::createBarGraph (YWidget *parent, YWidgetOpt &opt)
{
	return new YGBarGraph (opt, YGWidget::get (parent));
}

#include "ygtkslider.h"
#include "YPartitionSplitter.h"

class YGPartitionSplitter : public YPartitionSplitter, public YGWidget
{
public:
	YGtkBarGraph *m_barGraph;

	YGPartitionSplitter (const YWidgetOpt &opt, YGWidget *parent,
	             int usedSize, int totalFreeSize, int newPartSize,
	                          int minNewPartSize, int minFreeSize,
	        const YCPString &usedLabel,const YCPString &freeLabel,
	                                const YCPString &newPartLabel,
	                              const YCPString &freeFieldLabel,
	                           const YCPString &newPartFieldLabel)
	: YPartitionSplitter (opt, usedSize, totalFreeSize, newPartSize, minNewPartSize,
	                      minFreeSize, usedLabel, freeLabel, newPartLabel,
	                      freeFieldLabel, newPartFieldLabel)
	, YGWidget (this, parent, true, GTK_TYPE_VBOX, NULL)
	{
		GtkWidget *graph = ygtk_bar_graph_new();
		m_barGraph = YGTK_BAR_GRAPH (graph);
		ygtk_bar_graph_create_entries (m_barGraph, 3);
		ygtk_bar_graph_setup_entry (m_barGraph, 0,
		                            usedLabel->value_cstr(), usedSize);
		ygtk_bar_graph_setup_entry (m_barGraph, 1,
		                            freeLabel->value_cstr(), totalFreeSize);
		ygtk_bar_graph_setup_entry (m_barGraph, 2,
		                            newPartLabel->value_cstr(), newPartSize);

		GtkWidget *labels_box = gtk_hbox_new (FALSE, 0);
		GtkWidget *free_label = gtk_label_new (freeFieldLabel->value_cstr());
		GtkWidget *new_part_label = gtk_label_new (newPartFieldLabel->value_cstr());
		gtk_container_add (GTK_CONTAINER (labels_box), free_label);
		gtk_container_add (GTK_CONTAINER (labels_box), new_part_label);
		gtk_box_set_child_packing (GTK_BOX (labels_box), free_label,
		                           FALSE, FALSE, 0, GTK_PACK_START);
		gtk_box_set_child_packing (GTK_BOX (labels_box), new_part_label,
		                           FALSE, FALSE, 0, GTK_PACK_END);

		GtkWidget *slider = ygtk_slider_new (minFreeSize, minNewPartSize, TRUE, TRUE);

		gtk_container_add (GTK_CONTAINER (getWidget()), graph);
		gtk_container_add (GTK_CONTAINER (getWidget()), labels_box);
		gtk_container_add (GTK_CONTAINER (getWidget()), slider);
		gtk_box_set_child_packing (GTK_BOX (getWidget()), labels_box,
		                           FALSE, FALSE, 2, GTK_PACK_START);
		gtk_box_set_child_packing (GTK_BOX (getWidget()), slider,
		                           FALSE, FALSE, 2, GTK_PACK_START);

		gtk_widget_show_all (getWidget());

		g_signal_connect (G_OBJECT (slider), "value-changed",
		                  G_CALLBACK (slider_changed_cb), this);
	}

	// YPartitionSplitter
	virtual void setValue (int newValue)
	{
		ygtk_slider_set_value (YGTK_SLIDER (getWidget()), newValue);
		YPartitionSplitter::setValue (newValue);
	}

	static void slider_changed_cb (YGtkSlider *slider, YGPartitionSplitter *pThis)
	{
// FIXME: why is pThis->freeLabel()->value_cstr() crashing ?!
		pThis->YPartitionSplitter::setValue (ygtk_slider_get_value (slider));
printf("setting up entry 1\n");
		ygtk_bar_graph_setup_entry (pThis->m_barGraph, 1,
		                            NULL,
//		                            pThis->freeLabel()->value_cstr(),
		                            pThis->totalFreeSize());
fprintf(stderr, "setting up entry 2\n");
//fprintf(stderr, "label: %s\n", pThis->newPartLabel()->value().c_str());
fprintf(stderr, "value: %d\n", pThis->newPartSize());
		ygtk_bar_graph_setup_entry (pThis->m_barGraph, 2,
		                            NULL,
//		                            pThis->newPartLabel()->value_cstr(),
		                            pThis->newPartSize());

fprintf(stderr, "emitting value\n");
		pThis->emitEvent (YEvent::ValueChanged);
	}

	// YWidget
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_NICESIZE
};

YWidget *
YGUI::createPartitionSplitter (YWidget *parent, YWidgetOpt &opt,
               int usedSize, int totalFreeSize, int newPartSize,
                            int minNewPartSize, int minFreeSize,
          const YCPString &usedLabel,const YCPString &freeLabel,
                                  const YCPString &newPartLabel,
                                const YCPString &freeFieldLabel,
                             const YCPString &newPartFieldLabel)
{
	return new YGPartitionSplitter (opt, YGWidget::get (parent), usedSize,
		totalFreeSize, newPartSize, minNewPartSize, minFreeSize, usedLabel,
		freeLabel, newPartLabel, freeFieldLabel, newPartFieldLabel);
}
