/* YGtkBarGraph widget */

#include "ygtkbargraph.h"
#include <gtk/gtklabel.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkeventbox.h>

#define YPADDING 12

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
	gtk_container_set_border_width (GTK_CONTAINER (bar), 12);
}

GtkWidget *ygtk_bar_graph_new (void)
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
#define DECL_COLOR(r,g,b) \
		{ 0, (r) << 8, (g) << 8, (b) << 8 }
// Tango Palette
		DECL_COLOR (138, 226,  52), //	Chameleon 1
		DECL_COLOR (252, 175,  62), //	Orange 1
		DECL_COLOR (114, 159, 207), //	Sky Blue 1
		DECL_COLOR (233, 185, 110), //	Chocolate 1
		DECL_COLOR (239,  41,  41), //	Scarlet Red 1
		DECL_COLOR (252, 233,  79), //	Butter 1
		DECL_COLOR (173, 127, 168), //	Plum 1
		DECL_COLOR (115, 210,  22), //	Chameleon 2
		DECL_COLOR (245, 121,   0), //	Orange 2
		DECL_COLOR ( 52, 101, 164), //	Sky Blue 2
		DECL_COLOR (193, 125,  17), //	Chocolate 2
		DECL_COLOR (204,   0,   0), //	Scarlet Red 2
		DECL_COLOR (237, 212,   0), //	Butter 2
		DECL_COLOR (117,  80, 123), //	Plum 2
		DECL_COLOR ( 78, 154,   6), //	Chameleon 3
		DECL_COLOR (206,  92,   0), //	Orange 3
		DECL_COLOR ( 32,  74, 135), //	Sky Blue 3
		DECL_COLOR (143,  89,   2), //	Chocolate 3
		DECL_COLOR (164,   0,   0), //	Scarlet Red 3
		DECL_COLOR (196, 160,   0), //	Butter 3
		DECL_COLOR ( 92,  53, 102), //	Plum 3
		DECL_COLOR (238, 238, 236), //	Aluminium 1
		DECL_COLOR (211, 215, 207), //	Aluminium 2
		DECL_COLOR (186, 189, 182), //	Aluminium 3
		DECL_COLOR (136, 138, 133), //	Aluminium 4
		DECL_COLOR ( 85,  87,  83), //	Aluminium 5
		DECL_COLOR ( 46,  52,  54)  //	Aluminium 6
#undef DECL_COLOR
	};

	const GdkColor *color = &palette [index % G_N_ELEMENTS (palette)];
	gtk_widget_modify_bg (box,   GTK_STATE_NORMAL, color);
	gtk_widget_modify_bg (frame, GTK_STATE_NORMAL, color);
}

/* Just to avoid the size getting too big, when we have little
   bars. */
static void ygtk_bar_graph_size_request (GtkWidget      *widget,
                                         GtkRequisition *requisition)
{
	GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);
	//	const int max_width = 250;
	//	if (requisition->width > max_width)
	//		requisition->width = max_width;
	requisition->height += YPADDING;
}
