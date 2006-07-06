/* Yast GTK */
#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"

/* YGtk::BarGraph declaration. */

#include "ygtkratiobox.h"

#define TYPE_BAR_GRAPH            (YGtk::bar_graph_get_type ())
#define BAR_GRAPH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                   TYPE_BAR_GRAPH, YGtk::BarGraph))
#define BAR_GRAPH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                   TYPE_BAR_GRAPH, YGtk::BarGraphClass))
#define IS_BAR_GRAPH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                   TYPE_BAR_GRAPH))
#define IS_BAR_GRAPH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                   TYPE_BAR_GRAPH))
#define BAR_GRAPH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                   TYPE_BAR_GRAPH, YGtk::BarGraphClass))

namespace YGtk
{
typedef struct _BarGraph	     BarGraph;
typedef struct _BarGraphClass  BarGraphClass;

struct _BarGraph
{
	YGtk::RatioBox ratio_box;
	// private:
	GtkTooltips *m_tooltips;
};

struct _BarGraphClass
{
	YGtk::RatioBoxClass parent_class;
};

GtkWidget* bar_graph_new ();
GType bar_graph_get_type (void) G_GNUC_CONST;

void bar_graph_create_entries (BarGraph *bar, guint entries);
void bar_graph_setup_entry (BarGraph *bar, int index,
                 const gchar *label_entry, int value);
}; /*namespace YGtk*/

/* YGBarGraph */

#include "YBarGraph.h"

class YGBarGraph : public YBarGraph, public YGWidget
{
public:
	YGBarGraph (const YWidgetOpt &opt, YGWidget *parent)
	: YBarGraph (opt)
	, YGWidget (this, parent, true, TYPE_BAR_GRAPH, NULL)
	{
		gtk_widget_show_all (getWidget());
	}

	// YBarGraph
	virtual void doUpdate()
	{
		YGtk::bar_graph_create_entries (BAR_GRAPH (getWidget()), segments());
		for (int i = 0; i < segments(); i++)
			YGtk::bar_graph_setup_entry (BAR_GRAPH (getWidget()), i,
			                             label(i).c_str(), value(i));
	}

	// YWidget
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	virtual long nicesize (YUIDimension dim)
	{
		long nicesize = getNiceSize (dim);
		if (dim == YD_HORIZ) {
			// Don't let the bar graph be too big
			const long max_width = 250;
			if (nicesize > max_width)
				return max_width;
		}
		return nicesize;
	}
};

YWidget *
YGUI::createBarGraph (YWidget *parent, YWidgetOpt &opt)
{
	return new YGBarGraph (opt, YGWidget::get (parent));
}

/* YGtk::BarGraph implementation. */

namespace YGtk
{
static void bar_graph_class_init (BarGraphClass *klass);
static void bar_graph_init       (BarGraph      *bar);

static BarGraphClass *parent_class = NULL;

GType bar_graph_get_type()
{
	IMPL
	static GType bar_type = 0;
	if (!bar_type) {
		static const GTypeInfo bar_info = {
			sizeof (BarGraphClass),
			NULL, NULL, (GClassInitFunc) bar_graph_class_init, NULL, NULL,
			sizeof (BarGraph), 0, (GInstanceInitFunc) bar_graph_init, NULL
		};

		bar_type = g_type_register_static (GTK_TYPE_CONTAINER, "BarGraph",
		                                   &bar_info, (GTypeFlags) 0);
	}
	return bar_type;
}

static void bar_graph_class_init (BarGraphClass *klass)
{
	IMPL
	parent_class = (BarGraphClass*) g_type_class_peek_parent (klass);
}

static void bar_graph_init (BarGraph *bar)
{
	IMPL
}

GtkWidget* bar_graph_new ()
{
	IMPL
	BarGraph* bar = (BarGraph*) g_object_new (TYPE_BAR_GRAPH, NULL);
	RATIO_BOX (bar)->orientation = YGtk::HORIZONTAL_RATIO_BOX_ORIENTATION;
	RATIO_BOX (bar)->spacing = 0;
	bar->m_tooltips = gtk_tooltips_new();
	return GTK_WIDGET (bar);
}

void bar_graph_create_entries (BarGraph *bar, guint entries)
{
// FIXME: do I really must use bar->ratio_box rather than RATIO_BOX (bar) ?

	IMPL
	// Remove the ones in excess
	for (guint i = entries; i < g_list_length (bar->ratio_box.children); i++)
		gtk_container_remove (GTK_CONTAINER (bar),
				(GtkWidget*) g_list_nth_data (bar->ratio_box.children, i));

	// Add new ones, if missing
	for (guint i = g_list_length (bar->ratio_box.children); i < entries; i++) {
		GtkWidget *label = gtk_label_new (NULL);

		// Box so that we can draw a background on the label
		GtkWidget *box = gtk_event_box_new();
		gtk_container_add (GTK_CONTAINER (box), label);

		// A frame around the label
		GtkWidget *frame = gtk_frame_new (NULL);
		gtk_container_add (GTK_CONTAINER (frame), box);
		gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);

		gtk_container_add (GTK_CONTAINER (&bar->ratio_box), frame);
	}
}

void bar_graph_setup_entry (BarGraph *bar, int index,
                 const gchar *label_entry, int value)
{
	IMPL
	RatioBoxChild* box_child = (RatioBoxChild*)
		g_list_nth_data (bar->ratio_box.children, index);

	GtkWidget *frame = box_child->widget;
	GtkWidget *box   = gtk_bin_get_child (GTK_BIN (frame));
	GtkWidget *label = gtk_bin_get_child (GTK_BIN (box));

	if (value == -1)
		value = 0;

	// Reading label text
	std::string label_text = label_entry;
		{  // Replace %1 by value(i)
		std::string::size_type pos = label_text.find ("%1", 0);
		if (pos != std::string::npos) {
			gchar* value_str = g_strdup_printf ("%d", value);
			label_text.erase (pos, 2);
			label_text.insert (pos, value_str);
			g_free (value_str);
			}
		}
	gtk_label_set_label (GTK_LABEL (label), label_text.c_str());

	// Tooltip with the label -- useful if the bar entry gets too small
	gtk_tooltips_set_tip (bar->m_tooltips, box, label_text.c_str(), NULL);

	// Set proportion
	YGtk::ratio_box_set_child_packing (RATIO_BOX (bar), frame, value, TRUE, 0);

	// Set background color
	static const bool colors_mask [][3] = {
		{ 1, 0, 0 },	// red
		{ 0, 1, 1 },	// cyan
		{ 1, 1, 0 },	// yellow
		{ 0, 1, 0 },	// green
		{ 1, 0, 1 } 	// purple
	};
	const bool *mask = colors_mask [index % (sizeof (colors_mask)/3)];
	float ratio = box_child->ratio * RATIO_BOX(bar)->ratios_sum;
	int intensity = (int)((1.0 - ratio) * 255);
	GdkColor color = { 0, (mask[0] ? 255 : intensity) << 8,
	                      (mask[1] ? 255 : intensity) << 8,
	                      (mask[2] ? 255 : intensity) << 8 };
	gtk_widget_modify_bg (box,   GTK_STATE_NORMAL, &color);
	gtk_widget_modify_bg (frame, GTK_STATE_NORMAL, &color);
}

}; /*namespace YGtk*/
