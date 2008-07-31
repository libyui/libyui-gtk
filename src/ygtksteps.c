/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkSteps widget */
// check the header file for information about this widget

#include <config.h>
#include "ygtksteps.h"
#include <gtk/gtkhbox.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkimage.h>
#include <string.h>

#define BORDER 6
#define STEPS_HEADER_SPACING 12
#define STEPS_SPACING        4
#define STEPS_IDENTATION    30
#define CURRENT_MARK_ANIMATION_TIME  250
#define CURRENT_MARK_ANIMATION_OFFSET  3
#define CURRENT_MARK_FRAMES_NB (CURRENT_MARK_ANIMATION_OFFSET*2)

G_DEFINE_TYPE (YGtkSteps, ygtk_steps, GTK_TYPE_WIDGET)

static void ygtk_steps_init (YGtkSteps *steps)
{
	GTK_WIDGET_SET_FLAGS (steps, GTK_NO_WINDOW);

	PangoContext *context = gtk_widget_get_pango_context (GTK_WIDGET (steps));
	steps->check_mark_layout = pango_layout_new (context);
	steps->current_mark_layout = pango_layout_new (context);
	pango_layout_set_markup (steps->check_mark_layout, "\u2714", -1);
	pango_layout_set_markup (steps->current_mark_layout, "<b>\u2192</b>", -1);
	steps->current_mark_timeout_id = steps->current_mark_frame = 0;
}

static void ygtk_steps_destroy (GtkObject *object)
{
	YGtkSteps *steps = YGTK_STEPS (object);

	if (steps->current_mark_timeout_id) {
		g_source_remove (steps->current_mark_timeout_id);
		steps->current_mark_timeout_id = 0;
	}

	if (steps->check_mark_layout)
		g_object_unref (steps->check_mark_layout);
	steps->check_mark_layout = NULL;
	if (steps->current_mark_layout)
		g_object_unref (steps->current_mark_layout);
	steps->current_mark_layout = NULL;
	
	ygtk_steps_clear (steps);
	GTK_OBJECT_CLASS (ygtk_steps_parent_class)->destroy (object);
}


static inline YGtkSingleStep *ygtk_steps_get_step (YGtkSteps *steps, guint step_nb)
{
	return g_list_nth_data (steps->steps, step_nb);
}

static void ygtk_step_invalidate_layout (YGtkSingleStep *step)
{
	if (step->layout) {
		g_object_unref (G_OBJECT (step->layout));
		step->layout = NULL;
	}
	if (step->layout_bold) {
		g_object_unref (G_OBJECT (step->layout_bold));
		step->layout_bold = NULL;
	}
}

// should be called rather than accessing step->layout
static PangoLayout *ygtk_steps_get_step_layout (YGtkSteps *steps, guint step_nb)
{
	YGtkSingleStep *step = ygtk_steps_get_step (steps, step_nb);

	if (!step->layout) {
		step->layout = gtk_widget_create_pango_layout (GTK_WIDGET (steps), NULL);

		gchar *text, *text_bold = 0;
		if (step->is_heading) {
			text = g_strdup_printf ("<span size=\"x-large\" weight=\"heavy\">%s</span>",
			                        step->text);
			pango_layout_set_spacing (step->layout, STEPS_HEADER_SPACING * PANGO_SCALE);
		}
		else {
			if (!step->layout_bold)
				step->layout_bold = gtk_widget_create_pango_layout (GTK_WIDGET (steps), NULL);
			g_assert (step->layout_bold);
			text = g_strdup (step->text);
			text_bold = g_strdup_printf ("<b>%s</b>", step->text);
			pango_layout_set_indent (step->layout, STEPS_IDENTATION * PANGO_SCALE);
			pango_layout_set_indent (step->layout_bold, STEPS_IDENTATION * PANGO_SCALE);
			pango_layout_set_spacing (step->layout, STEPS_SPACING * PANGO_SCALE);
			pango_layout_set_spacing (step->layout_bold, STEPS_SPACING * PANGO_SCALE);
		}

		pango_layout_set_markup (step->layout, text, -1);
		g_free (text);
		if (step->layout_bold)
		{
			pango_layout_set_markup (step->layout_bold, text_bold, -1);
			g_free (text_bold);
		}
	}

	if (steps->current_step == step_nb && step->layout_bold != NULL)
		return step->layout_bold;
	return step->layout;
}

// If PangoContext got invalidated (eg. system language change), re-compute it!
static void ygtk_steps_recompute_layout (YGtkSteps *steps)
{
	GList *it;
	for (it = steps->steps; it; it = it->next) {
		YGtkSingleStep *step = it->data;
		if (step->layout)
			pango_layout_context_changed (step->layout);
		if (step->layout_bold)
			pango_layout_context_changed (step->layout_bold);
	}
}

static void ygtk_steps_direction_changed (GtkWidget *widget, GtkTextDirection dir)
{
	ygtk_steps_recompute_layout (YGTK_STEPS (widget));
	GTK_WIDGET_CLASS (ygtk_steps_parent_class)->direction_changed (widget, dir);
}

static void ygtk_steps_style_set (GtkWidget *widget, GtkStyle *style)
{
	ygtk_steps_recompute_layout (YGTK_STEPS (widget));
	GTK_WIDGET_CLASS (ygtk_steps_parent_class)->style_set (widget, style);
}

static void ygtk_steps_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	requisition->width = requisition->height = 0;

	YGtkSteps *steps = YGTK_STEPS (widget);
	int i;
	for (i = 0; i < ygtk_steps_total (steps); i++) {
		PangoLayout *layout = ygtk_steps_get_step_layout (steps, i);
		YGtkSingleStep *step = ygtk_steps_get_step (steps, i);
		if (!step->is_heading)
			layout = step->layout_bold;

		int w = 0, h = 0;
		pango_layout_get_pixel_size (layout, &w, &h);
		w += PANGO_PIXELS (pango_layout_get_indent (layout));
		h += PANGO_PIXELS (pango_layout_get_spacing (layout)) * 2;

		requisition->width = MAX (w, requisition->width);
		requisition->height += h;
	}

	requisition->width += BORDER * 2;
	requisition->height += BORDER * 2;
	GTK_WIDGET_CLASS (ygtk_steps_parent_class)->size_request (widget, requisition);
}

static gboolean ygtk_steps_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	GtkStyle *style = gtk_widget_get_style (widget);

	// background
	cairo_t *cr = gdk_cairo_create (widget->window);

	int x, y, w, h;
	x = widget->allocation.x; y = widget->allocation.y;
	w = widget->allocation.width; h = widget->allocation.height;

	cairo_pattern_t *pattern = cairo_pattern_create_linear (x, y, x, y+h);
	cairo_pattern_add_color_stop_rgba (pattern, 0, 1, 1, 1, 1);
	cairo_pattern_add_color_stop_rgba (pattern, 1, 1, 1, 1, 0);
	cairo_set_source (cr, pattern);
	cairo_rectangle (cr, x, y, w, h);
	cairo_fill (cr);
	cairo_pattern_destroy (pattern);
	cairo_destroy (cr);

	// content
	x = widget->allocation.x + BORDER; y = widget->allocation.y + BORDER;
	YGtkSteps *steps = YGTK_STEPS (widget);
	int i;
	for (i = 0; i < ygtk_steps_total (steps); i++) {
		PangoLayout *layout = ygtk_steps_get_step_layout (steps, i);

		// as opposite to identation, we need to apply spacing ourselves, since we aren't
		// really using paragraphs
		y += PANGO_PIXELS (pango_layout_get_spacing (layout));

		gtk_paint_layout (style, widget->window, GTK_STATE_NORMAL, TRUE,
		                  NULL, /*event->area,*/ widget, NULL, x, y, layout);

		if (i <= steps->current_step && !ygtk_steps_get_step (steps, i)->is_heading) {
			PangoLayout *mark = (i == steps->current_step) ? steps->current_mark_layout
			                                               : steps->check_mark_layout;
			int x_;
			pango_layout_get_pixel_size (mark, &x_, NULL);
			x_ = x + STEPS_IDENTATION - x_ - 6;
			if (i == steps->current_step) {
				if (steps->current_mark_frame < CURRENT_MARK_FRAMES_NB/2)
					x_ -= steps->current_mark_frame * CURRENT_MARK_ANIMATION_OFFSET;
				else
					x_ -= (CURRENT_MARK_FRAMES_NB - steps->current_mark_frame) *
					      CURRENT_MARK_ANIMATION_OFFSET;
			}

			gtk_paint_layout (style, widget->window, GTK_STATE_NORMAL, TRUE,
			                  NULL, /*event->area,*/ widget, NULL, x_, y, mark);
		}

		int h;
		pango_layout_get_pixel_size (layout, NULL, &h);
		// we need spacing for both sides
		y += h + PANGO_PIXELS (pango_layout_get_spacing (layout));
	}
	return FALSE;
}

GtkWidget* ygtk_steps_new (void)
{
	return g_object_new (YGTK_TYPE_STEPS, NULL);
}

guint ygtk_steps_append (YGtkSteps *steps, const gchar *step_text)
{
	gint steps_nb;

    g_return_val_if_fail (YGTK_IS_STEPS (steps), 0);

    steps_nb = ygtk_steps_total (steps);

	// check if the text is the same as the previous...
	if (steps_nb) {
		YGtkSingleStep *last_step = g_list_last (steps->steps)->data;
		if (!g_strcasecmp (last_step->text, step_text)) {
			last_step->strength++;
			return steps_nb-1;
		}
	}

	// New step
	YGtkSingleStep *step = g_new0 (YGtkSingleStep, 1);
	step->is_heading = FALSE;
	step->text = g_strdup (step_text);
	step->strength = 1;
	step->layout = step->layout_bold = NULL;

	steps->steps = g_list_append (steps->steps, step);
	gtk_widget_queue_resize (GTK_WIDGET (steps));
	return steps_nb;
}

void ygtk_steps_append_heading (YGtkSteps *steps, const gchar *heading)
{
	YGtkSingleStep *step;

    g_return_if_fail (YGTK_IS_STEPS (steps));

    step = g_new0 (YGtkSingleStep, 1);
	step->is_heading = TRUE;
	step->text = g_strdup (heading);
	step->strength = 1;  // not important anyway
	step->layout = step->layout_bold = NULL;

	steps->steps = g_list_append (steps->steps, step);
	gtk_widget_queue_resize (GTK_WIDGET (steps));
}

void ygtk_steps_advance (YGtkSteps *steps)
{
	if (steps->current_step + 1 < ygtk_steps_total (steps))
		ygtk_steps_set_current (steps, steps->current_step + 1);
}

static gboolean current_mark_animation_cb (void *steps_ptr)
{
	YGtkSteps *steps = steps_ptr;

	// ugly -- should use gtk_widget_queue_draw_area (widget, x, y, w, h)
	// but we need to iterate through all steps to get current location and
	// all, so...
	gtk_widget_queue_draw (GTK_WIDGET (steps));

	if (++steps->current_mark_frame == CURRENT_MARK_FRAMES_NB) {
		steps->current_mark_frame = 0;
		return FALSE;
	}
	return TRUE;
}

void ygtk_steps_set_current (YGtkSteps *steps, guint step)
{
	guint old_step = steps->current_step;
	steps->current_step = step;

	// update step icons
	ygtk_step_invalidate_layout (ygtk_steps_get_step (steps, old_step));
	ygtk_step_invalidate_layout (ygtk_steps_get_step (steps, step));

	steps->current_mark_frame = 0;
	steps->current_mark_timeout_id = g_timeout_add
		(CURRENT_MARK_ANIMATION_TIME / CURRENT_MARK_FRAMES_NB,
		current_mark_animation_cb, steps);
}

guint ygtk_steps_total (YGtkSteps *steps)
{
	return g_list_length (steps->steps);
}

void ygtk_steps_clear (YGtkSteps *steps)
{
	GList *it;
	for (it = steps->steps; it; it = it->next) {
		YGtkSingleStep *step = it->data;
		ygtk_step_invalidate_layout (step);
		g_free (step->text);
		g_free (step);
	}
	g_list_free (steps->steps);
	steps->steps = NULL;
}

static void ygtk_steps_class_init (YGtkStepsClass *klass)
{
	ygtk_steps_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->expose_event = ygtk_steps_expose_event;
	widget_class->size_request = ygtk_steps_size_request;
	widget_class->direction_changed = ygtk_steps_direction_changed;
	widget_class->style_set = ygtk_steps_style_set;

	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
	gtkobject_class->destroy = ygtk_steps_destroy;
}

