/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkBarGraph widget */
// check the header file for information about this widget

#include <config.h>
#include "ygtkratiobox.h"
#include "ygtkbargraph.h"
#include <gtk/gtk.h>

G_DEFINE_TYPE (YGtkBarGraph, ygtk_bar_graph, GTK_TYPE_FRAME)

static void ygtk_bar_graph_init (YGtkBarGraph *bar)
{
	GtkWidget *box = ygtk_ratio_hbox_new (0);
	gtk_widget_show (box);
	gtk_container_add (GTK_CONTAINER (bar), box);
	ygtk_bar_graph_set_style (bar, TRUE);
}

static void ygtk_bar_graph_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	GTK_WIDGET_CLASS (ygtk_bar_graph_parent_class)->size_request (widget, requisition);
	requisition->height += 6;  // give room for the labels
}

GtkWidget *ygtk_bar_graph_new (void)
{
	return g_object_new (YGTK_TYPE_BAR_GRAPH, NULL);
}

void ygtk_bar_graph_create_entries (YGtkBarGraph *bar, guint entries)
{
	YGtkRatioBox *box = YGTK_RATIO_BOX (GTK_BIN (bar)->child);

	// Remove the ones in excess
	guint i;
	for (i = entries; i < g_list_length (box->children); i++)
		gtk_container_remove (GTK_CONTAINER (box),
				(GtkWidget*) g_list_nth_data (box->children, i));

	// Add new ones, if missing
	for (i = g_list_length (box->children); i < entries; i++) {
		GtkWidget *label = ygtk_colored_label_new();
		gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);

		// we need a GtkEventBox or something, so we may assign a tooltip to it
		GtkWidget *lbox = gtk_event_box_new();
		gtk_container_add (GTK_CONTAINER (lbox), label);
		gtk_widget_show_all (lbox);
		gtk_container_add (GTK_CONTAINER (box), lbox);
	}
}

static GtkWidget *ygtk_bar_graph_get_label (YGtkBarGraph *bar, int index, GtkWidget **b)
{
	YGtkRatioBox *hbox = YGTK_RATIO_BOX (GTK_BIN (bar)->child);
	GtkWidget *box = ((YGtkRatioBoxChild *) g_list_nth_data (hbox->children, index))->widget;
	if (b) *b = box;
	return gtk_bin_get_child (GTK_BIN (box));
}

void ygtk_bar_graph_setup_entry (YGtkBarGraph *bar, int index, const gchar *label_entry, int value)
{
	GtkWidget *box, *label;
	label = ygtk_bar_graph_get_label (bar, index, &box);

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

		// tooltip for the labels -- useful if the bar entry gets too small
		gtk_widget_set_tooltip_text (box, label_text->str);
		g_string_free (label_text, TRUE);
	}

	// Set proportion
	gtk_widget_set_size_request (box, 0, -1);
	YGtkRatioBox *hbox = YGTK_RATIO_BOX (GTK_BIN (bar)->child);
	ygtk_ratio_box_set_child_packing (hbox, box, MAX (value, 1));

	// Set background color
	// The Tango palette
	const guint palette [][3] = {
		{ 138, 226,  52 },   // Chameleon 1
		{ 252, 175,  62 },   // Orange 1
		{ 114, 159, 207 },   // Sky Blue 1
		{ 233, 185, 110 },   // Chocolate 1
		{ 239,  41,  41 },   // Scarlet Red 1
		{ 252, 233,  79 },   // Butter 1
		{ 173, 127, 168 },   // Plum 1
		{ 115, 210,  22 },   // Chameleon 2
		{ 245, 121,   0 },   // Orange 2
		{  52, 101, 164 },   // Sky Blue 2
		{ 193, 125,  17 },   // Chocolate 2
		{ 204,   0,   0 },   // Scarlet Red 2
		{ 237, 212,   0 },   // Butter 2
		{ 117,  80, 123 },   // Plum 2
		{  78, 154,   6 },   // Chameleon 3
		{ 206,  92,   0 },   // Orange 3
		{  32,  74, 135 },   // Sky Blue 3
		{ 143,  89,   2 },   // Chocolate 3
		{ 164,   0,   0 },   // Scarlet Red 3
		{ 196, 160,   0 },   // Butter 3
		{  92,  53, 102 },   // Plum 3
		{ 238, 238, 236 },   // Aluminium 1
		{ 211, 215, 207 },   // Aluminium 2
		{ 186, 189, 182 },   // Aluminium 3
		{ 136, 138, 133 },   // Aluminium 4
		{  85,  87,  83 },   // Aluminium 5
		{  46,  52,  54 },   // Aluminium 6
	};

	YGtkColoredLabel *color_label = YGTK_COLORED_LABEL (label);
	const guint *color = palette [index % G_N_ELEMENTS (palette)];
	GdkColor gcolor = { 0, color[0] << 8, color[1] << 8, color[2] << 8 };
	ygtk_colored_label_set_background (color_label, &gcolor);
}

void ygtk_bar_graph_set_style (YGtkBarGraph *bar, gboolean flat)
{
	bar->flat = flat;
	GtkShadowType shadow = flat ? GTK_SHADOW_OUT : GTK_SHADOW_NONE;
	gtk_frame_set_shadow_type (GTK_FRAME (bar), shadow);
}

void ygtk_bar_graph_customize_bg (YGtkBarGraph *bar, int index, GdkColor *color)
{
	GtkWidget *label = ygtk_bar_graph_get_label (bar, index, NULL);
	ygtk_colored_label_set_background (YGTK_COLORED_LABEL (label), color);
}

void ygtk_bar_graph_customize_fg (YGtkBarGraph *bar, int index, GdkColor *color)
{
	GtkWidget *label = ygtk_bar_graph_get_label (bar, index, NULL);
	ygtk_colored_label_set_foreground (YGTK_COLORED_LABEL (label), color);
}

static void ygtk_bar_graph_class_init (YGtkBarGraphClass *klass)
{
	ygtk_bar_graph_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->size_request  = ygtk_bar_graph_size_request;
}

//** YGtkColoredLabel

#include <stdlib.h>

G_DEFINE_TYPE (YGtkColoredLabel, ygtk_colored_label, GTK_TYPE_LABEL)

static void ygtk_colored_label_init (YGtkColoredLabel *label)
{}

static inline double pixel_clamp (double val)
{ return MAX (0, MIN (1, val)); }

static gboolean ygtk_colored_label_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	GtkAllocation *alloc = &widget->allocation;

	GdkColor *bg_color = &widget->style->bg[GTK_STATE_NORMAL];
	double red = (bg_color->red >> 8) / 255.;
	double green = (bg_color->green >> 8) / 255.;
	double blue = (bg_color->blue >> 8) / 255.;

	cairo_t *cr = gdk_cairo_create (event->window);
	cairo_translate (cr, alloc->x, alloc->y);
	cairo_scale (cr, alloc->width, alloc->height);

	double x, y, w, h;
	x = alloc->x;
	y = alloc->y;
	w = alloc->width;
	h = alloc->height;
	cairo_pattern_t *grad = cairo_pattern_create_linear (0, 0, 0, 1);

	cairo_pattern_add_color_stop_rgba (grad, 0, pixel_clamp (red+.3), pixel_clamp (green+.3), pixel_clamp (blue+.3), 1);
	cairo_pattern_add_color_stop_rgba (grad, 0.70, red, green, blue, 1);
	cairo_pattern_add_color_stop_rgba (grad, 1, pixel_clamp (red-.2), pixel_clamp (green-.2), pixel_clamp (blue-.2), 1);

	cairo_rectangle (cr, 0, 0, 1, 1);
	cairo_set_source (cr, grad);
	cairo_fill (cr);

	cairo_pattern_destroy (grad);
	cairo_destroy (cr);

	GTK_WIDGET_CLASS (ygtk_colored_label_parent_class)->expose_event (widget, event);
	return FALSE;
}

GtkWidget *ygtk_colored_label_new (void)
{ return g_object_new (YGTK_TYPE_COLORED_LABEL, NULL); }

void ygtk_colored_label_set_background (YGtkColoredLabel *label, GdkColor *color)
{ gtk_widget_modify_bg (GTK_WIDGET (label), GTK_STATE_NORMAL, color); }

void ygtk_colored_label_set_foreground (YGtkColoredLabel *label, GdkColor *color)
{ gtk_widget_modify_fg (GTK_WIDGET (label), GTK_STATE_NORMAL, color); }

static void ygtk_colored_label_class_init (YGtkColoredLabelClass *klass)
{
	ygtk_colored_label_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->expose_event  = ygtk_colored_label_expose_event;
}

