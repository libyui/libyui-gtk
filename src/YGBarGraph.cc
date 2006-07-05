/* Yast GTK */
#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "YBarGraph.h"
#include "ygtkratiobox.h"

class YGBarGraph : public YBarGraph, public YGWidget
{
public:
	YGBarGraph (const YWidgetOpt &opt, YGWidget *parent)
	: YBarGraph (opt)
	, YGWidget (this, parent, true, TYPE_RATIO_BOX, NULL)
	{ }

	// YBarGraph
	virtual void doUpdate()
	{
		// FIXME: We could remove this pain of removing/adding everything by
		// just removing no longer existing labels and adding new ones. And
		// re-settings container ratios, labels text and colors.

		// Remove possible existing labels
		GList* children = gtk_container_get_children (GTK_CONTAINER (getWidget()));
		for (GList* child = children; child; child = child->next) {
			GtkWidget *label = (GtkWidget*) child->data;
			gtk_container_remove (GTK_CONTAINER (getWidget()), label);  // un-refs widget
		}
		g_list_free (children);

		// Max value needed for the color intensity
		int maxValue = 0;
		for (int i = 0; i < segments(); i++)
			maxValue = std::max (maxValue, value(i));

		// Add the labels
		for (int i = 0; i < segments(); i++) {
			// Reading label text
			string label_text = label (i);
				{  // Replace %1 by value(i)
				string::size_type pos = label_text.find ("%1", 0);
				if (pos != string::npos) {
					gchar* value_str = g_strdup_printf ("%d", value(i));
					label_text.erase (pos, 2);
					label_text.insert (pos, value_str);
					g_free (value_str);
					}
				}
			GtkWidget *label = gtk_label_new (label_text.c_str());

			// Box so that we can draw a background on the label
			GtkWidget *box = gtk_event_box_new();
			gtk_container_add (GTK_CONTAINER (box), label);

			// A frame around the label
			GtkWidget *frame = gtk_frame_new (NULL);
			gtk_container_add (GTK_CONTAINER (frame), box);
			gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);

			// Set background color
			static const bool colors_mask [][3] = {
				{ 1, 0, 0 },	// red
				{ 0, 1, 1 },	// cyan
				{ 1, 1, 0 },	// yellow
				{ 0, 1, 0 },	// green
				{ 1, 0, 1 } 	// purple
			};
			const bool *mask = colors_mask [i % (sizeof (colors_mask)/3)];
			int intensity = (int)((1.0 - ((float)value(i) / maxValue)) * 255);
			GdkColor color = { 0, (mask[0] ? 255 : intensity) << 8,
			                      (mask[1] ? 255 : intensity) << 8,
			                      (mask[2] ? 255 : intensity) << 8 };
			gtk_widget_modify_bg (box,   GTK_STATE_NORMAL, &color);
			gtk_widget_modify_bg (frame, GTK_STATE_NORMAL, &color);

			gtk_container_add (GTK_CONTAINER (getWidget()), frame);
			int val = value(i);
			if (val == -1) val = 0;
			YGtk::ratio_box_set_child_packing
				(RATIO_BOX (getWidget()), frame, val, TRUE, 0);
		}
		gtk_widget_show_all (getWidget());
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
