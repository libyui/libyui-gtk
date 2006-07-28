/* Yast-GTK */
/* YGtkSteps is a widget that displays a list of steps, useful
   to show the progress of some configuration tool as it does
   the work of updating files or services or whatever.

   You use the append functions to initializate the steps. If,
   for some reason, you want one step to count as more than one
   (ie. so that a ygtk_steps_advance() needs to be called more than
    once to actually advance for a given step), you may append a
   step with the same name of the previous, that they'll be collapsed.
*/

#include "ygtksteps.h"
#include <gtk/gtkhbox.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkimage.h>

static void ygtk_steps_class_init (YGtkStepsClass *klass);
static void ygtk_steps_init       (YGtkSteps      *step);
static void ygtk_steps_finalize   (GObject        *object);
static void ygtk_steps_update_step (YGtkSteps *steps, guint step_nb);
static void ygtk_steps_size_allocate (GtkWidget     *widget,
                                      GtkAllocation *allocation);

static GtkVBoxClass *parent_class = NULL;

// Graphics for the current and done icons
static const char *current_xpm[] = {
"12 12 2 1",
" 	c None",
".	c #000000",
"    .       ",
"    ..      ",
"    ...     ",
"    ....    ",
"    .....   ",
"    ......  ",
"    .....   ",
"    ....    ",
"    ...     ",
"    ..      ",
"    .       ",
"            "};

static const char *done_xpm[] = {
"12 12 15 1",
" 	c None",
".	c #000000",
"+	c #B3C2A7",
"@	c #708C58",
"#	c #859D71",
"$	c #627B4D",
"%	c #566C43",
"&	c #A2BD9E",
"*	c #789774",
"=	c #88AC84",
"-	c #5E764A",
";	c #698566",
">	c #6D8855",
",	c #668050",
"'	c #4F633E",
"        ..  ",
"       .+@. ",
"       .#$. ",
" ..   .#%.  ",
".&*. .#@.   ",
".==...@%.   ",
" .=*.-@.    ",
" .==;>%.    ",
"  .==,.     ",
"   .='.     ",
"    ..      ",
"    .       "};

static GdkPixbuf *current_pixbuf = 0;
static GdkPixbuf *done_pixbuf    = 0;
static guint ref_icons = 0;

GType ygtk_steps_get_type()
{
	static GType step_type = 0;
	if (!step_type) {
		static const GTypeInfo step_info = {
			sizeof (YGtkStepsClass),
			NULL, NULL, (GClassInitFunc) ygtk_steps_class_init, NULL, NULL,
			sizeof (YGtkSteps), 0, (GInstanceInitFunc) ygtk_steps_init, NULL
		};

		step_type = g_type_register_static (GTK_TYPE_VBOX, "YGtkSteps",
		                                    &step_info, (GTypeFlags) 0);
	}
	return step_type;
}

static void ygtk_steps_class_init (YGtkStepsClass *klass)
{
	parent_class = (GtkVBoxClass*) g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->size_allocate  = ygtk_steps_size_allocate;

	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = ygtk_steps_finalize;
}

static void ygtk_steps_init (YGtkSteps *steps)
{
	steps->steps = NULL;
	steps->current_step = 0;
	gtk_box_set_spacing (GTK_BOX (steps), 2);
	gtk_container_set_border_width (GTK_CONTAINER (steps), 4);

	// load images
	if (!ref_icons) {
		done_pixbuf    = gdk_pixbuf_new_from_xpm_data (done_xpm);
		current_pixbuf = gdk_pixbuf_new_from_xpm_data (current_xpm);
	}
	ref_icons++;
}

static void ygtk_steps_finalize (GObject *object)
{
	ygtk_steps_clear (YGTK_STEPS (object));
	if (--ref_icons == 0) {
		g_object_unref (G_OBJECT (done_pixbuf));
		g_object_unref (G_OBJECT (current_pixbuf));
	}
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget* ygtk_steps_new()
{
	YGtkSteps* steps = (YGtkSteps*) g_object_new (YGTK_TYPE_STEPS, NULL);
	return GTK_WIDGET (steps);
}

guint ygtk_steps_append (YGtkSteps *steps, const gchar *step_text)
{
	YGtkSingleStep *last_step = 0;
	if (ygtk_steps_total (steps))
		last_step = g_list_last (steps->steps)->data;

	// Don't add the same step twice -- but emulate like if you did
	if (last_step && !g_strcasecmp (step_text,
	        gtk_label_get_text (GTK_LABEL (last_step->label)))) {
		YGtkSingleStep *step = g_malloc (sizeof (YGtkSingleStep));
		step->is_heading = FALSE;
		step->label = last_step->label;
		step->image = last_step->image;
		step->is_alias = TRUE;
		steps->steps = g_list_append (steps->steps, step);
		return ygtk_steps_total (steps) - 1;
	}
	// New step
	else {
		YGtkSingleStep *step = g_malloc (sizeof (YGtkSingleStep));
		step->is_heading = FALSE;
		step->is_alias = FALSE;
		steps->steps = g_list_append (steps->steps, step);

		GtkWidget *box = gtk_hbox_new (FALSE, 0);
		step->image = gtk_image_new();
		step->label = gtk_label_new (step_text);

		gtk_container_add (GTK_CONTAINER (box), step->image);
		gtk_container_add (GTK_CONTAINER (box), step->label);
		gtk_box_set_child_packing (GTK_BOX (box), step->image,
		                           FALSE, FALSE, 0, GTK_PACK_START);
		gtk_box_set_child_packing (GTK_BOX (box), step->label,
		                           FALSE, FALSE, 5, GTK_PACK_START);

		GtkWidget *alignment = gtk_alignment_new (0, 0.5, 0, 0);
		gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 2, 2, 20, 0);
		gtk_container_add (GTK_CONTAINER (alignment), box);

		gtk_container_add (GTK_CONTAINER (steps), alignment);
		gtk_widget_show_all (alignment);

		guint step_nb = g_list_length (steps->steps) - 1;
		ygtk_steps_update_step (steps, step_nb);
		return step_nb;
	}
}

void ygtk_steps_append_heading (YGtkSteps *steps, const gchar *heading)
{
	YGtkSingleStep *step = g_malloc (sizeof (YGtkSingleStep));
	step->is_heading = TRUE;
	step->label = gtk_label_new (heading);
	step->image = NULL;
	step->is_alias = FALSE;
	steps->steps = g_list_append (steps->steps, step);

	gtk_misc_set_alignment (GTK_MISC (step->label), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (step->label), 0, 6);

	gtk_container_add (GTK_CONTAINER (steps), step->label);
	gtk_widget_show (step->label);

	// set a heading font
	PangoFontDescription *font = pango_font_description_new();
	pango_font_description_set_weight (font, PANGO_WEIGHT_BOLD);
	int size = pango_font_description_get_size (step->label->style->font_desc);
	pango_font_description_set_size   (font, size * PANGO_SCALE_LARGE);
	gtk_widget_modify_font (step->label, font);
	pango_font_description_free (font);
}

void ygtk_steps_advance (YGtkSteps *steps)
{
	if (steps->current_step + 1 < ygtk_steps_total (steps))
		ygtk_steps_set_current (steps, steps->current_step + 1);
}

void ygtk_steps_set_current (YGtkSteps *steps, guint step)
{
	guint old_step = steps->current_step;
	steps->current_step = step;

	// update step icons
	guint min, max, i;
	min = MIN (old_step, step);
	max = MAX (old_step, step);
	for (i = min; i <= max; i++)
		ygtk_steps_update_step (steps, i);
}

guint ygtk_steps_total (YGtkSteps *steps)
{
	return g_list_length (steps->steps);
}

void ygtk_steps_clear (YGtkSteps *steps)
{
	GList *children = gtk_container_get_children (GTK_CONTAINER (steps));
	GList *it;
	for (it = children; it; it = it->next)
		gtk_container_remove (GTK_CONTAINER (steps), GTK_WIDGET (it->data));
	g_list_free (children);

	for (it = steps->steps; it; it = it->next)
		g_free ((YGtkSingleStep *) it->data);

	g_list_free (steps->steps);
	steps->steps = NULL;
}

static void ygtk_steps_update_step (YGtkSteps *steps, guint step_nb)
{
	YGtkSingleStep *step = g_list_nth_data (steps->steps, step_nb);
	if (step->is_heading)
		return;
	if (step->is_alias && step_nb > steps->current_step)
		return;

	// update step label -- set bold if current step
	PangoFontDescription *font = pango_font_description_new();
	if (step_nb == steps->current_step)
		pango_font_description_set_weight (font, PANGO_WEIGHT_BOLD);
	gtk_widget_modify_font (step->label, font);
	pango_font_description_free (font);

	// update step icon
	GtkWidget *image = step->image;
	if (step_nb < steps->current_step)
		gtk_image_set_from_pixbuf (GTK_IMAGE (image), done_pixbuf);
	else if (step_nb == steps->current_step)
		gtk_image_set_from_pixbuf (GTK_IMAGE (image), current_pixbuf);
	else {
		// Looks like GtkImage doesn't ask for a redraw on its place when
		// setting image clear, thus this work-around.
		gtk_widget_queue_draw_area (image->parent, image->allocation.x,
			image->allocation.y, image->allocation.width, image->allocation.height);

		gtk_image_clear (GTK_IMAGE (image));

		// Set the clear image to have the same size of the others
		gtk_widget_set_size_request (image, gdk_pixbuf_get_width (done_pixbuf),
		                             gdk_pixbuf_get_height (done_pixbuf));
	}
}

static void ygtk_steps_size_allocate (GtkWidget     *widget,
                                      GtkAllocation *allocation)
{
	// Don't let the steps widget expand besides a certain point
	GtkRequisition requisition;
	gtk_widget_size_request (widget, &requisition);
	allocation->height = MIN (requisition.height, allocation->height);

	(* GTK_WIDGET_CLASS (parent_class)->size_allocate) (widget, allocation);
}
