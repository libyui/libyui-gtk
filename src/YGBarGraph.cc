/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "ygtkbargraph.h"

#include "YBarGraph.h"

class YGBarGraph : public YBarGraph, public YGWidget
{
public:
	YGBarGraph (YWidget *parent)
	: YBarGraph (NULL)
	, YGWidget (this, parent, true, YGTK_TYPE_BAR_GRAPH, NULL)
	{}

	// YBarGraph
	virtual void doUpdate()
	{
		GdkColor clr;
		ygtk_bar_graph_create_entries (YGTK_BAR_GRAPH (getWidget()), segments());
		for (int i = 0; i < segments(); i++) {
			const YBarGraphSegment &s = segment (i);
			GdkColor *c = 0;
			if (s.hasSegmentColor()) {
				const YColor &color = s.segmentColor();
				clr.red   = color.red();
				clr.green = color.green();
				clr.blue  = color.blue();
				c = &clr;
			}
			ygtk_bar_graph_setup_entry (YGTK_BAR_GRAPH (getWidget()), i,
				s.label().c_str(), s.value(), c);
		}
		// FIXME: new libyui colors segments ... We probably should honor that
	}

	YGWIDGET_IMPL_COMMON
};

YBarGraph *YGOptionalWidgetFactory::createBarGraph (YWidget *parent)
{
	return new YGBarGraph (parent);
}

#include "YPartitionSplitter.h"

class YGPartitionSplitter : public YPartitionSplitter, public YGWidget
{
public:
	YGtkBarGraph *m_barGraph;
	GtkWidget *m_scale, *m_free_spin, *m_new_spin;

	YGPartitionSplitter (YWidget *parent, int usedSize, int totalFreeSize, int newPartSize,
		int minNewPartSize, int minFreeSize, const string &usedLabel, const string &freeLabel,
		const string &newPartLabel, const string &freeFieldLabel, const string &newPartFieldLabel)
	: YPartitionSplitter (NULL, usedSize, totalFreeSize, newPartSize, minNewPartSize,
		minFreeSize, usedLabel, freeLabel, newPartLabel, freeFieldLabel, newPartFieldLabel)
	, YGWidget (this, parent, true, GTK_TYPE_VBOX, NULL)
	{
		/* Bar graph widget */
		GtkWidget *graph = ygtk_bar_graph_new();
		m_barGraph = YGTK_BAR_GRAPH (graph);
		ygtk_bar_graph_create_entries (m_barGraph, 3);
		ygtk_bar_graph_setup_entry (m_barGraph, 0, usedLabel.c_str(), usedSize, NULL);

		/* Labels over the slider */
		GtkWidget *labels_box, *free_label, *new_part_label;
		labels_box = gtk_hbox_new (FALSE, 0);
		free_label = gtk_label_new (freeFieldLabel.c_str());
		new_part_label = gtk_label_new (newPartFieldLabel.c_str());
		gtk_box_pack_start (GTK_BOX (labels_box), free_label, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (labels_box), new_part_label, FALSE, FALSE, 0);

		/* Slider and the spinners */
		GtkWidget *slider_box = gtk_hbox_new (FALSE, 0);
		m_scale = gtk_hscale_new_with_range ((gdouble) minFreeSize, maxFreeSize(), 1);
		gtk_scale_set_draw_value (GTK_SCALE (m_scale), FALSE);
		m_free_spin = gtk_spin_button_new_with_range
			(minFreeSize, maxFreeSize(), 1);
		m_new_spin  = gtk_spin_button_new_with_range
			(minNewPartSize, maxNewPartSize(), 1);

		gtk_box_pack_start (GTK_BOX (slider_box), m_free_spin, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (slider_box), m_scale, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (slider_box), m_new_spin, FALSE, FALSE, 0);

		g_signal_connect (G_OBJECT (m_scale), "value-changed",
		                  G_CALLBACK (scale_changed_cb), this);
		g_signal_connect (G_OBJECT (m_free_spin), "value-changed",
		                  G_CALLBACK (free_spin_changed_cb), this);
		g_signal_connect (G_OBJECT (m_new_spin), "value-changed",
		                  G_CALLBACK (new_spin_changed_cb), this);

		/* Main layout */
		gtk_box_pack_start (GTK_BOX (getWidget()), graph, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (getWidget()), labels_box, FALSE, TRUE, 2);
		gtk_box_pack_start (GTK_BOX (getWidget()), slider_box, FALSE, TRUE, 2);

		setValue (newPartSize);  // initialization
		gtk_widget_show_all (getWidget());
	}

	// YPartitionSplitter
	virtual int value()
	{
		return gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (m_new_spin));
	}

	virtual void setValue (int newValue)
	{
		IMPL
		ygtk_bar_graph_setup_entry (m_barGraph, 1, freeLabel().c_str(),
		                            freeSize(), NULL);
		ygtk_bar_graph_setup_entry (m_barGraph, 2, newPartLabel().c_str(),
		                            newPartSize(), NULL);

		// block connections
		g_signal_handlers_block_by_func (m_scale,
		                                 (gpointer) scale_changed_cb, this);
		g_signal_handlers_block_by_func (m_free_spin,
		                                 (gpointer) free_spin_changed_cb, this);
		g_signal_handlers_block_by_func (m_new_spin,
		                                 (gpointer) new_spin_changed_cb, this);

		gtk_spin_button_set_value (GTK_SPIN_BUTTON (m_free_spin), freeSize());
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (m_new_spin), newPartSize());
		gtk_range_set_value (GTK_RANGE (m_scale), freeSize());

		// unblock connections
		g_signal_handlers_unblock_by_func (m_scale,
		                                   (gpointer) scale_changed_cb, this);
		g_signal_handlers_unblock_by_func (m_free_spin,
		                                   (gpointer) free_spin_changed_cb, this);
		g_signal_handlers_unblock_by_func (m_new_spin,
		                                   (gpointer) new_spin_changed_cb, this);
	}

	static void scale_changed_cb (GtkRange *range, YGPartitionSplitter *pThis)
	{
		IMPL
		int newFreeSize = (int) gtk_range_get_value (range);
		int newPartSize = pThis->totalFreeSize() - newFreeSize;

		pThis->setValue (newPartSize);
		pThis->emitEvent (YEvent::ValueChanged);
	}

	static void free_spin_changed_cb (GtkSpinButton *spin, YGPartitionSplitter *pThis)
	{
		IMPL
		int newFreeSize = gtk_spin_button_get_value_as_int (spin);
		int newPartSize = pThis->totalFreeSize() - newFreeSize;

		pThis->setValue (newPartSize);
		pThis->emitEvent (YEvent::ValueChanged);
	}

	static void new_spin_changed_cb (GtkSpinButton *spin, YGPartitionSplitter *pThis)
	{
		IMPL
		pThis->setValue (gtk_spin_button_get_value_as_int (spin));
		pThis->emitEvent (YEvent::ValueChanged);
	}

	YGWIDGET_IMPL_COMMON
};

YPartitionSplitter *YGOptionalWidgetFactory::createPartitionSplitter (YWidget *parent,
		int usedSize, int totalFreeSize, int newPartSize, int minNewPartSize,
		int minFreeSize, const string &usedLabel, const string &freeLabel,
		const string &newPartLabel, const string &freeFieldLabel,
		const string &newPartFieldLabel)
{
	return new YGPartitionSplitter (parent, usedSize, totalFreeSize, newPartSize,
		minNewPartSize, minFreeSize, usedLabel, freeLabel, newPartLabel, freeFieldLabel,
		newPartFieldLabel);
}

