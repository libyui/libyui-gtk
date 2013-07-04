/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkSteps widget */
// check the header file for information about this widget

/*
  Textdomain "gtk"
 */

#include <yui/Libyui_config.h>
#include "ygtksteps.h"
#include <gtk/gtk.h>
#define YGI18N_C
#include "YGi18n.h"

#define CURRENT_MARK_ANIMATION_TIME  250
#define CURRENT_MARK_ANIMATION_OFFSET  3
#define CURRENT_MARK_FRAMES_NB (CURRENT_MARK_ANIMATION_OFFSET*2)

G_DEFINE_TYPE (YGtkSteps, ygtk_steps, GTK_TYPE_BOX)

static void ygtk_steps_init (YGtkSteps *steps)
{
	gtk_box_set_spacing (GTK_BOX (steps), 8);
        gtk_orientable_set_orientation (GTK_ORIENTABLE (steps), GTK_ORIENTATION_VERTICAL);
	gtk_container_set_border_width (GTK_CONTAINER (steps), 4);

	const gchar *check = "\u2714", *current = "\u25b6", *todo = "\u26ab";
	if (gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL)
		current = "\u25c0";
	PangoContext *context = gtk_widget_get_pango_context (GTK_WIDGET (steps));
	steps->check_mark_layout = pango_layout_new (context);
	steps->current_mark_layout = pango_layout_new (context);
	steps->todo_mark_layout = pango_layout_new (context);
	pango_layout_set_text (steps->check_mark_layout, check, -1);
	pango_layout_set_text (steps->current_mark_layout, current, -1);
	pango_layout_set_text (steps->todo_mark_layout, todo, -1);
	steps->current_mark_timeout_id = steps->current_mark_frame = 0;
}

static void ygtk_steps_destroy (GtkWidget *widget)
{
	YGtkSteps *steps = YGTK_STEPS (widget);
	if (steps->current_mark_timeout_id) {
		g_source_remove (steps->current_mark_timeout_id);
		steps->current_mark_timeout_id = 0;
	}
	if (steps->check_mark_layout)
		g_object_unref (steps->check_mark_layout);
	steps->check_mark_layout = NULL;
	if (steps->current_mark_layout)
		g_object_unref (steps->current_mark_layout);
	if (steps->todo_mark_layout)
		g_object_unref (steps->todo_mark_layout);
	steps->todo_mark_layout = NULL;

	GTK_WIDGET_CLASS (ygtk_steps_parent_class)->destroy(widget);
}

static void ygtk_step_update_layout (YGtkSteps *steps, gint step)
{
	if (step < 0) return;
	gboolean bold = steps->current_step == step;
	GList *children = gtk_container_get_children (GTK_CONTAINER (steps));
	GtkWidget *label = (GtkWidget *) g_list_nth_data (children, step);
	if (g_object_get_data (G_OBJECT (label), "is-header"))
		return;
	if (bold) {
		PangoAttrList *attrbs = pango_attr_list_new();
		pango_attr_list_insert (attrbs, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
		gtk_label_set_attributes (GTK_LABEL (label), attrbs);
		pango_attr_list_unref (attrbs);
		atk_object_set_description (gtk_widget_get_accessible (label), _("Current step"));
	}
	else {
		gtk_label_set_attributes (GTK_LABEL (label), NULL);
		atk_object_set_description (gtk_widget_get_accessible (label), "");
	}
	g_list_free (children);
}

static gboolean ygtk_steps_draw (GtkWidget *widget, cairo_t *cr)
{
	GTK_WIDGET_CLASS (ygtk_steps_parent_class)->draw(widget, cr);

	YGtkSteps *steps = YGTK_STEPS (widget);
	gboolean reverse = gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL;
	GList *children = gtk_container_get_children (GTK_CONTAINER (widget)), *i;

	cairo_set_source_rgb (cr, 0, 0, 0);
	int n = 0;
	for (i = children; i; i = i->next, n++) {
		GtkWidget *label = i->data;
                GtkAllocation alloc;
                gtk_widget_get_allocation(label, &alloc);

		if (g_object_get_data (G_OBJECT (label), "is-header"))
			continue;
		PangoLayout *layout;
		if (n < steps->current_step)
			layout = steps->check_mark_layout;
		else if (n == steps->current_step)
			layout = steps->current_mark_layout;
		else //if (n > steps->current_step)
			layout = steps->todo_mark_layout;
		int x = alloc.x, y = alloc.y;
		if (reverse) {
			PangoRectangle rect;
			pango_layout_get_pixel_extents (layout, NULL, &rect);
			x += alloc.width - rect.width - 4;
		}
		else
			x += 4;
		if (n == steps->current_step) {
			int offset;
			if (steps->current_mark_frame < CURRENT_MARK_FRAMES_NB/2)
				offset = steps->current_mark_frame * CURRENT_MARK_ANIMATION_OFFSET;
			else
				offset = (CURRENT_MARK_FRAMES_NB - steps->current_mark_frame) *
				      CURRENT_MARK_ANIMATION_OFFSET;
			x += offset * (reverse ? 1 : -1);
		}

		cairo_move_to (cr, x, y);
		pango_cairo_show_layout (cr, layout);
	}
	g_list_free (children);
	return 	FALSE;
}

GtkWidget* ygtk_steps_new (void)
{
	return g_object_new (YGTK_TYPE_STEPS, NULL);
}

gint ygtk_steps_append (YGtkSteps *steps, const gchar *text)
{
	GtkWidget *label = gtk_label_new (text);
	GdkRGBA black = { 0.0, 0.0, 0.0, 1.0 };
	gtk_widget_override_color (label, GTK_STATE_NORMAL, &black);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	int mark_width = 10;
	pango_layout_get_pixel_size (steps->check_mark_layout, &mark_width, NULL);
	gtk_misc_set_padding (GTK_MISC (label), mark_width+12, 0);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (steps), label, FALSE, TRUE, 0);
	return ygtk_steps_total (steps)-1;
}

void ygtk_steps_append_heading (YGtkSteps *steps, const gchar *heading)
{
	GtkWidget *label = gtk_label_new (heading);
	GdkRGBA black = { 0.0, 0.0, 0.0, 1.0 };
	gtk_widget_override_color (label, GTK_STATE_NORMAL, &black);
	g_object_set_data (G_OBJECT (label), "is-header", GINT_TO_POINTER (1));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);

	PangoAttrList *attrbs = pango_attr_list_new();
	pango_attr_list_insert (attrbs, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
	pango_attr_list_insert (attrbs, pango_attr_scale_new (PANGO_SCALE_LARGE));
	gtk_label_set_attributes (GTK_LABEL (label), attrbs);
	pango_attr_list_unref (attrbs);

	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (steps), label, FALSE, TRUE, 6);
}

static gboolean current_mark_animation_cb (void *steps_ptr)
{
	YGtkSteps *steps = steps_ptr;

	// should use gtk_widget_queue_draw_area (widget, x, y, w, h)...
	gtk_widget_queue_draw (GTK_WIDGET (steps));

	if (++steps->current_mark_frame == CURRENT_MARK_FRAMES_NB) {
		steps->current_mark_frame = 0;
		return FALSE;
	}
	return TRUE;
}

void ygtk_steps_set_current (YGtkSteps *steps, gint step)
{
	gint old_step = steps->current_step;
	steps->current_step = step;

	// update step icons
	if (old_step != step) {
		ygtk_step_update_layout (steps, old_step);
		ygtk_step_update_layout (steps, step);
	}

	if (step != -1 && step != old_step) {
		steps->current_mark_frame = 0;
		steps->current_mark_timeout_id = g_timeout_add
			(CURRENT_MARK_ANIMATION_TIME / CURRENT_MARK_FRAMES_NB,
			current_mark_animation_cb, steps);
	}
}

gint ygtk_steps_total (YGtkSteps *steps)
{
	GList *children = gtk_container_get_children (GTK_CONTAINER (steps));
	int steps_nb = g_list_length (children);
	g_list_free (children);
	return steps_nb;
}

const gchar *ygtk_steps_get_nth_label (YGtkSteps *steps, gint n)
{
	if (n < 0) return NULL;
	GtkWidget *step;
	GList *children = gtk_container_get_children (GTK_CONTAINER (steps));
	step = g_list_nth_data (children, n);
	g_list_free (children);
	if (step)
		return gtk_label_get_text (GTK_LABEL (step));
	return NULL;
}

void ygtk_steps_clear (YGtkSteps *steps)
{
	GList *children = gtk_container_get_children (GTK_CONTAINER (steps)), *i;
	for (i = children; i; i = i->next)
		gtk_container_remove (GTK_CONTAINER (steps), (GtkWidget *) i->data);
	g_list_free (children);
}

static void ygtk_steps_class_init (YGtkStepsClass *klass)
{
	ygtk_steps_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->draw = ygtk_steps_draw;
	widget_class->destroy = ygtk_steps_destroy;
}

