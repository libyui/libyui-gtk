/* YGtkBarGraph widget */

#include "ygtkbargraph.h"
#include <gtk/gtklabel.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkeventbox.h>

static void ygtk_bar_graph_class_init (YGtkBarGraphClass *klass);
static void ygtk_bar_graph_init       (YGtkBarGraph      *bar);
static void ygtk_bar_graph_size_request  (GtkWidget      *widget,
                                     GtkRequisition *requisition);

static YGtkRatioHBoxClass *parent_class = NULL;

GType ygtk_bar_graph_get_type()
{
	static GType bar_type = 0;
	if (!bar_type) {
		static const GTypeInfo bar_info = {
			sizeof (YGtkBarGraphClass),
			NULL, NULL, (GClassInitFunc) ygtk_bar_graph_class_init, NULL, NULL,
			sizeof (YGtkBarGraph), 0, (GInstanceInitFunc) ygtk_bar_graph_init, NULL
		};

		bar_type = g_type_register_static (YGTK_TYPE_RATIO_HBOX, "YGtkBarGraph",
		                                   &bar_info, (GTypeFlags) 0);
	}
	return bar_type;
}

static void ygtk_bar_graph_class_init (YGtkBarGraphClass *klass)
{
	parent_class = (YGtkRatioHBoxClass*) g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->size_request  = ygtk_bar_graph_size_request;
}

static void ygtk_bar_graph_init (YGtkBarGraph *bar)
{
	YGTK_RATIO_BOX (bar)->spacing = 0;
	bar->m_tooltips = gtk_tooltips_new();
}

GtkWidget *ygtk_bar_graph_new()
{
	YGtkBarGraph* bar = (YGtkBarGraph*) g_object_new (YGTK_TYPE_BAR_GRAPH, NULL);
	return GTK_WIDGET (bar);
}

void ygtk_bar_graph_create_entries (YGtkBarGraph *bar, guint entries)
{
	// Remove the ones in excess
	guint i;
	for (i = entries; i < g_list_length (YGTK_RATIO_BOX (bar)->children); i++)
		gtk_container_remove (GTK_CONTAINER (bar),
				(GtkWidget*) g_list_nth_data (YGTK_RATIO_BOX (bar)->children, i));

	// Add new ones, if missing
	for (i = g_list_length (YGTK_RATIO_BOX (bar)->children); i < entries; i++) {
		GtkWidget *label = gtk_label_new (NULL);
		gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);

		// Box so that we can draw a background on the label
		GtkWidget *box = gtk_event_box_new();
		gtk_container_add (GTK_CONTAINER (box), label);

		// A frame around the label
		GtkWidget *frame = gtk_frame_new (NULL);
		gtk_container_add (GTK_CONTAINER (frame), box);
		gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);

		gtk_container_add (GTK_CONTAINER (bar), frame);

		if (GTK_WIDGET_VISIBLE (bar))
			gtk_widget_show_all (frame);
	}
}

void ygtk_bar_graph_setup_entry (YGtkBarGraph *bar, int index,
                                 const gchar *label_entry, int value)
{
	YGtkRatioBoxChild *box_child = (YGtkRatioBoxChild *)
		g_list_nth_data (YGTK_RATIO_BOX (bar)->children, index);

	GtkWidget *frame = box_child->widget;
	GtkWidget *box   = gtk_bin_get_child (GTK_BIN (frame));
	GtkWidget *label = gtk_bin_get_child (GTK_BIN (box));

	if (value < 0)
		value = 0;

	// Reading label text
	if (label_entry) {
		GString *label_text = g_string_new (label_entry);
			{  // Replace %1 by value(i)
			guint i;
			for (i = 0; i < label_text->len; i++)
				if (label_text->str[i] == '%' && label_text->str[i+1] == '1') {
					gchar *value_str = g_strdup_printf ("%d", value);
					label_text = g_string_erase (label_text, i, 2);
					label_text = g_string_insert (label_text, i, value_str);
					g_free (value_str);
				}
			}
		gtk_label_set_label (GTK_LABEL (label), label_text->str);

		// Tooltip with the label -- useful if the bar entry gets too small
		gtk_tooltips_set_tip (bar->m_tooltips, box, label_text->str, NULL);
	}

	// Set proportion
	ygtk_ratio_box_set_child_packing (YGTK_RATIO_BOX (bar), frame,
	                                  MAX (value, 1), TRUE, 0);

	// Set background color
	const GdkColor palette [] = {
		{ 0, 224 << 8,   0 << 8,   0 << 8 },	// red
		{ 0, 237 << 8, 238 << 8, 236 << 8 },	// gray
		{ 0, 252 << 8, 233 << 8,  79 << 8 },	// yellow
		{ 0, 138 << 8, 226 << 8,  52 << 8 },	// green
		{ 0, 173 << 8, 127 << 8, 168 << 8 },	// pink
		{ 0, 114 << 8, 159 << 8, 207 << 8 },	// blue
		{ 0, 252 << 8, 175 << 8,  62 << 8 },	// orange
		{ 0, 193 << 8, 125 << 8,  17 << 8 },	// brown
	};
#define palette_size (sizeof (palette) / sizeof (GdkColor))

	const GdkColor *color = &palette [index % palette_size];
	gtk_widget_modify_bg (box,   GTK_STATE_NORMAL, color);
	gtk_widget_modify_bg (frame, GTK_STATE_NORMAL, color);
}

/* Just to avoid the size getting too big, when we have little
   bars. */
static void ygtk_bar_graph_size_request (GtkWidget      *widget,
                                         GtkRequisition *requisition)
{
	GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);
	const int max_width = 250;
	if (requisition->width > max_width)
		requisition->width = max_width;
}
