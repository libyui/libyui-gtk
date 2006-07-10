/* YGtkSteps widget */

#include "ygtksteps.h"
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkimage.h>

static void ygtk_steps_class_init (YGtkStepsClass *klass);
static void ygtk_steps_init       (YGtkSteps      *step);
static void ygtk_steps_update_step (YGtkSteps *steps, guint step_nb);

static GtkVBoxClass *parent_class = NULL;

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
}

static void ygtk_steps_init (YGtkSteps *step)
{
	step->steps = NULL;
	step->current_step = 0;
	gtk_box_set_homogeneous (GTK_BOX (step), TRUE);
	gtk_box_set_spacing (GTK_BOX (step), 2);
}

GtkWidget* ygtk_steps_new()
{
	YGtkSteps* steps = (YGtkSteps*) g_object_new (YGTK_TYPE_STEPS, NULL);
	return GTK_WIDGET (steps);
}

guint ygtk_steps_append (YGtkSteps *steps, const gchar *step_text)
{
	YGtkSingleStep *step = g_malloc (sizeof (YGtkSingleStep));
	step->is_heading = FALSE;
	step->widget = gtk_hbox_new (FALSE, 0);
	steps->steps = g_list_append (steps->steps, step);

	GtkWidget *image = gtk_image_new_from_file ("inexistant");
//	GtkWidget *image = gtk_image_new();
	GtkWidget *label = gtk_label_new (step_text);
	gtk_container_add (GTK_CONTAINER (step->widget), image);
	gtk_container_add (GTK_CONTAINER (step->widget), label);
	gtk_box_set_child_packing (GTK_BOX (step->widget), image,
	                           FALSE, FALSE, 2, GTK_PACK_START);

	gtk_container_add (GTK_CONTAINER (steps), step->widget);
	gtk_widget_show_all (step->widget);

	guint step_nb = g_list_length (steps->steps) - 1;
	ygtk_steps_update_step (steps, step_nb);

	return step_nb;
}

void ygtk_steps_append_heading (YGtkSteps *steps, const gchar *heading)
{
	YGtkSingleStep *step = g_malloc (sizeof (YGtkSingleStep));
	step->is_heading = TRUE;
	step->widget = gtk_label_new (heading);
	steps->steps = g_list_append (steps->steps, step);

	gtk_container_add (GTK_CONTAINER (steps), step->widget);
	gtk_widget_show (step->widget);

	// set a heading font
	PangoFontDescription* font = pango_font_description_new();
	pango_font_description_set_weight (font, PANGO_WEIGHT_BOLD);
	pango_font_description_set_size   (font, 15*PANGO_SCALE);
	gtk_widget_modify_font (step->widget, font);
	pango_font_description_free (font);
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

void ygtk_steps_clear (YGtkSteps *steps)
{
	GList* it;
	for (it = g_list_first (steps->steps); it; it = it->next) {
		YGtkSingleStep *step = it->data;
		gtk_widget_destroy (step->widget);
		g_object_unref (G_OBJECT (step->widget));
	}

	g_list_free (steps->steps);
	steps->steps = NULL;
}

static void ygtk_steps_update_step (YGtkSteps *steps, guint step_nb)
{
	YGtkSingleStep *step = g_list_nth_data (steps->steps, step_nb);
	if (step->is_heading)
		return;

	GtkImage *image;
	GList *list = gtk_container_get_children (GTK_CONTAINER (step->widget));
	image = GTK_IMAGE (g_list_nth_data (list, 0));
	g_list_free (list);

	if (step_nb < steps->current_step)
		gtk_image_set_from_stock (image, "gtk-apply", GTK_ICON_SIZE_BUTTON);
	else if (step_nb == steps->current_step)
		// FIXME: this isn't the right icon -- but that one doesn't seem available
		// through the gtk stock icons...
		gtk_image_set_from_stock (image, "gtk-media-play", GTK_ICON_SIZE_BUTTON);
	else {
		// Looks like GtkImage doesn't ask for a redraw on its place when
		// setting image clear, thus this work-around.
		GtkWidget *widget = GTK_WIDGET (image);
		gtk_widget_queue_draw_area (widget->parent, widget->allocation.x,
			widget->allocation.y, widget->allocation.width, widget->allocation.height);

		gtk_image_clear (image);
	}
}
