/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <yui/Libyui_config.h>
#include "YGUI.h"
#include "YGWidget.h"
#include "ygtkbargraph.h"

#include "YBarGraph.h"

class YGBarGraph : public YBarGraph, public YGWidget
{
public:
	YGBarGraph (YWidget *parent)
	: YBarGraph (NULL)
	, YGWidget (this, parent, YGTK_TYPE_BAR_GRAPH, NULL)
	{}

	// YBarGraph
	virtual void doUpdate()
	{
		YGtkBarGraph *graph = YGTK_BAR_GRAPH (getWidget());
		ygtk_bar_graph_create_entries (graph, segments());
		for (int i = 0; i < segments(); i++) {
			const YBarGraphSegment &s = segment (i);
			ygtk_bar_graph_setup_entry (graph, i, s.label().c_str(), s.value());
			if (s.hasSegmentColor()) {
				GdkRGBA color = ycolorToGdk (s.segmentColor());
				ygtk_bar_graph_customize_bg (graph, i, &color);
			}
			if (s.hasTextColor()) {
				GdkRGBA color = ycolorToGdk (s.textColor());
				ygtk_bar_graph_customize_fg (graph, i, &color);
			}
		}
	}

	static GdkRGBA ycolorToGdk (const YColor &ycolor)
	{
		GdkRGBA color = { 0,
				  static_cast<gdouble> ( guint16(ycolor.red() << 8 ) ),
				  static_cast<gdouble> ( guint16(ycolor.green() << 8 ) ),
				  static_cast<gdouble> ( guint16(ycolor.blue() << 8 ) )
				};
		return color;
	}

	virtual unsigned int getMinSize (YUIDimension dim)
	{ return dim == YD_HORIZ ? 80 : 30; }

	YGWIDGET_IMPL_COMMON (YBarGraph)
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
		int minNewPartSize, int minFreeSize, const std::string &usedLabel, const std::string &freeLabel,
		const std::string &newPartLabel, const std::string &freeFieldLabel, const std::string &newPartFieldLabel)
	: YPartitionSplitter (NULL, usedSize, totalFreeSize, newPartSize, minNewPartSize,
		minFreeSize, usedLabel, freeLabel, newPartLabel, freeFieldLabel, newPartFieldLabel)
	, YGWidget (this, parent, GTK_TYPE_VBOX, NULL)
	{
		/* Bar graph widget */
		GtkWidget *graph = ygtk_bar_graph_new();
		m_barGraph = YGTK_BAR_GRAPH (graph);
		ygtk_bar_graph_create_entries (m_barGraph, 3);
		ygtk_bar_graph_setup_entry (m_barGraph, 0, usedLabel.c_str(), usedSize);

		/* Labels over the slider */
		GtkWidget *labels_box = gtk_hbox_new (FALSE, 0);
		gtk_box_pack_start (GTK_BOX (labels_box),
			gtk_label_new (freeFieldLabel.c_str()), FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (labels_box), gtk_label_new (NULL), TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (labels_box),
			gtk_label_new (newPartFieldLabel.c_str()), FALSE, TRUE, 0);

		/* Slider and the spinners */
		GtkWidget *slider_box = gtk_hbox_new (FALSE, 0);
		m_scale = gtk_hscale_new_with_range ((gdouble) minFreeSize, maxFreeSize(), 1);
		gtk_scale_set_draw_value (GTK_SCALE (m_scale), FALSE);
		m_free_spin = gtk_spin_button_new_with_range
			(minFreeSize, maxFreeSize(), 1);
		m_new_spin  = gtk_spin_button_new_with_range
			(minNewPartSize, maxNewPartSize(), 1);

		// keep the partition's order
		gtk_widget_set_direction (labels_box, GTK_TEXT_DIR_LTR);
		gtk_widget_set_direction (slider_box, GTK_TEXT_DIR_LTR);

		gtk_box_pack_start (GTK_BOX (slider_box), m_free_spin, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (slider_box), m_scale, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (slider_box), m_new_spin, FALSE, FALSE, 0);

		connect (m_scale, "value-changed", G_CALLBACK (scale_changed_cb), this);
		connect (m_free_spin, "value-changed", G_CALLBACK (free_spin_changed_cb), this);
		connect (m_new_spin, "value-changed", G_CALLBACK (new_spin_changed_cb), this);

		/* Main layout */
		gtk_box_pack_start (GTK_BOX (getWidget()), graph, TRUE, TRUE, 6);
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
		BlockEvents block (this);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (m_new_spin), newValue);
		int freeSize = totalFreeSize() - newValue;
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (m_free_spin), freeSize);
		gtk_range_set_value (GTK_RANGE (m_scale), freeSize);

		gtk_spin_button_set_value (GTK_SPIN_BUTTON (m_free_spin), freeSize);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (m_new_spin), newValue);

		ygtk_bar_graph_setup_entry (m_barGraph, 1, freeLabel().c_str(), freeSize);
		ygtk_bar_graph_setup_entry (m_barGraph, 2, newPartLabel().c_str(), newValue);
	}

	static void scale_changed_cb (GtkRange *range, YGPartitionSplitter *pThis)
	{
		int newFreeSize = (int) gtk_range_get_value (range);
		int newPartSize = pThis->totalFreeSize() - newFreeSize;

		pThis->setValue (newPartSize);
		pThis->emitEvent (YEvent::ValueChanged);
	}

	static void free_spin_changed_cb (GtkSpinButton *spin, YGPartitionSplitter *pThis)
	{
		int newFreeSize = gtk_spin_button_get_value_as_int (spin);
		int newPartSize = pThis->totalFreeSize() - newFreeSize;
		pThis->setValue (newPartSize);
		pThis->emitEvent (YEvent::ValueChanged);
	}

	static void new_spin_changed_cb (GtkSpinButton *spin, YGPartitionSplitter *pThis)
	{
		pThis->setValue (gtk_spin_button_get_value_as_int (spin));
		pThis->emitEvent (YEvent::ValueChanged);
	}

	YGWIDGET_IMPL_COMMON (YPartitionSplitter)
};

YPartitionSplitter *YGOptionalWidgetFactory::createPartitionSplitter (YWidget *parent,
		int usedSize, int totalFreeSize, int newPartSize, int minNewPartSize,
		int minFreeSize, const std::string &usedLabel, const std::string &freeLabel,
		const std::string &newPartLabel, const std::string &freeFieldLabel,
		const std::string &newPartFieldLabel)
{
	return new YGPartitionSplitter (parent, usedSize, totalFreeSize, newPartSize,
		minNewPartSize, minFreeSize, usedLabel, freeLabel, newPartLabel, freeFieldLabel,
		newPartFieldLabel);
}

