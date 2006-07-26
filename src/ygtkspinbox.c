/* Yast-GTK */
/* YGtkSpinBox is an extension of GtkSpinButton with the added
   functionality of being able to have more than one field (eg.
   for time and date choosers). The number of fields, their
   individual range and separation character is customizable.
*/

#include "ygtkspinbox.h"
#include <stdlib.h>
#include <string.h>

static void ygtk_spin_box_class_init (YGtkSpinBoxClass *klass);
static void ygtk_spin_box_init       (YGtkSpinBox      *box);
static void ygtk_spin_box_finalize   (GObject           *object);

static void ygtk_spin_box_move_cursor (GtkEntry *entry, GtkMovementStep step,
                                       gint count, gboolean extend_selection);
static void ygtk_spin_box_insert_text (GtkEditable *editable, const gchar *text,
                                       gint length, gint *position, YGtkSpinBox *box);
static void ygtk_spin_box_backspace (GtkEntry *entry);
static void ygtk_spin_box_delete (GtkEntry *entry, GtkDeleteType type, gint count);
static void ygtk_spin_box_change_value (GtkSpinButton *spin_button,
                                        GtkScrollType  scroll);

static YGtkSpinBoxField *ygtk_spin_box_get_field (YGtkSpinBox *box, gint char_pos);
static void ygtk_spin_box_update (YGtkSpinBox *box);

static gint ygtk_spin_box_button_press (GtkWidget          *widget,
                                        GdkEventButton     *event);

static GtkSpinButtonClass *parent_class = NULL;

GType ygtk_spin_box_get_type()
{
	static GType box_type = 0;
	if (!box_type) {
		static const GTypeInfo box_info = {
			sizeof (YGtkSpinBoxClass),
			NULL, NULL, (GClassInitFunc) ygtk_spin_box_class_init, NULL, NULL,
			sizeof (YGtkSpinBox), 0, (GInstanceInitFunc) ygtk_spin_box_init, NULL
		};

		box_type = g_type_register_static (GTK_TYPE_SPIN_BUTTON, "YGtkSpinBox",
		                                   &box_info, (GTypeFlags) 0);
	}
	return box_type;
}

/* utility */
static int get_digits_nb (int n)
{
	if (n / 10 == 0)
		return 1;
	return get_digits_nb (n / 10) + 1;
}

static void ygtk_spin_box_class_init (YGtkSpinBoxClass *klass)
{
	parent_class = (GtkSpinButtonClass*) g_type_class_peek_parent (klass);

	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = ygtk_spin_box_finalize;

	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->button_press_event = ygtk_spin_box_button_press;

	GtkEntryClass *entry_class = GTK_ENTRY_CLASS (klass);
	entry_class->move_cursor = ygtk_spin_box_move_cursor;
	entry_class->backspace = ygtk_spin_box_backspace;
	entry_class->delete_from_cursor = ygtk_spin_box_delete;

	GtkSpinButtonClass *spin_box_class = GTK_SPIN_BUTTON_CLASS (klass);
	spin_box_class->change_value = ygtk_spin_box_change_value;
}

static void ygtk_spin_box_init (YGtkSpinBox *box)
{
	box->separator = ':';
	box->fields = NULL;
	box->active_field = 0;

	GTK_SPIN_BUTTON (box)->wrap = TRUE;
	gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (box), GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (box), FALSE);

	g_signal_connect (G_OBJECT (box), "insert-text",
	                  G_CALLBACK (ygtk_spin_box_insert_text), box);
}

static void ygtk_spin_box_finalize (GObject *object)
{
	YGtkSpinBox *box = YGTK_SPIN_BOX (object);

	GList *it;
	for (it = g_list_first (box->fields); it; it = it->next)
		g_free (it->data);
	g_list_free (box->fields);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget* ygtk_spin_box_new()
{
	YGtkSpinBox* box = (YGtkSpinBox*) g_object_new (YGTK_TYPE_SPIN_BOX, NULL);
	return GTK_WIDGET (box);
}

guint ygtk_spin_box_add_field (YGtkSpinBox *spin_box, int min_range, int max_range)
{
	YGtkSpinBoxField *field = g_malloc (sizeof (YGtkSpinBoxField));
	spin_box->fields = g_list_append (spin_box->fields, field);

	field->value = field->min_value = min_range;
	field->max_value = max_range;
	field->digits_nb = get_digits_nb (max_range);

	ygtk_spin_box_update (spin_box);
	return g_list_length (spin_box->fields) - 1;
}

void ygtk_spin_box_set_separator_character (YGtkSpinBox *spin_box, char separator)
{
	spin_box->separator = separator;
	ygtk_spin_box_update (spin_box);
}

gint ygtk_spin_box_get_value (YGtkSpinBox *box, guint field_nb)
{
	YGtkSpinBoxField *field = g_list_nth_data (box->fields, field_nb);
	return field->value;
}

void ygtk_spin_box_set_value (YGtkSpinBox *box, guint field_nb, gint value)
{
	YGtkSpinBoxField *field = g_list_nth_data (box->fields, field_nb);
	field->value = value;
	ygtk_spin_box_update (box);
}

void ygtk_spin_box_move_cursor (GtkEntry *entry, GtkMovementStep step,
                                gint count, gboolean extend_selection)
{
	YGtkSpinBox *box = YGTK_SPIN_BOX (entry);
	if (step == GTK_MOVEMENT_VISUAL_POSITIONS) {
		int fields_nb = g_list_length (box->fields);

		if (count < 0) {
			box->active_field--;
			if (box->active_field < 0)
				box->active_field = fields_nb - 1;
		}
		else {
			box->active_field++;
			if (box->active_field >= fields_nb)
				box->active_field = 0;
		}

		ygtk_spin_box_update (box);
	}
}

YGtkSpinBoxField *ygtk_spin_box_get_field (YGtkSpinBox *box, gint char_pos)
{
	int chars = 0;
	GList *it;
	for (it = g_list_first (box->fields); it; it = it->next) {
		YGtkSpinBoxField *field = it->data;

		chars += field->digits_nb;
		if (chars >= char_pos)
			return field;

		chars++;  // separator
	}
	return NULL;
}

void ygtk_spin_box_insert_text (GtkEditable *editable, const gchar *text,
                                gint length, gint *position, YGtkSpinBox *box)
{
	if (!strcmp (text, "0") && *position == 0) ;
	// work around to avoid the GtkSpinButton meddling

	else {
		YGtkSpinBoxField *field = ygtk_spin_box_get_field (box, *position);
		if (field) {
			int value = atoi (text);

			field->value = (field->value*10) + value;
			if (field->value > field->max_value)
				field->value = value;
		}
	}

	ygtk_spin_box_update (box);
	g_signal_stop_emission_by_name (editable, "insert_text");
}

void ygtk_spin_box_backspace (GtkEntry *entry)
{
	YGtkSpinBox *box = YGTK_SPIN_BOX (entry);
	YGtkSpinBoxField *field = g_list_nth_data (box->fields, box->active_field);
	field->value = 0;
	ygtk_spin_box_update (box);
}

void ygtk_spin_box_delete (GtkEntry *entry, GtkDeleteType type, gint count)
{ ygtk_spin_box_backspace (entry); }

void ygtk_spin_box_change_value (GtkSpinButton *spin_button, GtkScrollType scroll)
{
	YGtkSpinBox *box = YGTK_SPIN_BOX (spin_button);
	YGtkSpinBoxField *field = g_list_nth_data (box->fields, box->active_field);
	switch (scroll) {
		case GTK_SCROLL_STEP_UP:
			field->value++;
			break;
		case GTK_SCROLL_STEP_DOWN:
			field->value--;
			break;
		case GTK_SCROLL_PAGE_DOWN:
			field->value -= 5;
			break;
		case GTK_SCROLL_PAGE_UP:
			field->value += 5;
			break;
		case GTK_SCROLL_START:
			field->value = field->min_value;
			break;
		case GTK_SCROLL_END:
			field->value = field->max_value;
			break;
		case GTK_SCROLL_NONE:
			break;
		default:
			g_warning ("YGtkSpinBox doesn't support %d scroll type", scroll);
			break;
	}

	ygtk_spin_box_update (box);
}

// update to reflect fields data
void ygtk_spin_box_update (YGtkSpinBox *box)
{
	GString *string = g_string_new ("");
	int min_selection = 0, len_selection = 0;

	GList *it;
	for (it = g_list_first (box->fields); it; it = it->next) {
		YGtkSpinBoxField *field = it->data;

		// keep range
		if (field->value < field->min_value)
			field->value = field->min_value;
		else if (field->value > field->max_value)
			field->value = field->max_value;

		if (box->active_field == g_list_position (box->fields, it)) {
			min_selection = string->len;
			len_selection = field->digits_nb;
		}

		// fill with zeros at the left
		int zeros = field->digits_nb - get_digits_nb (field->value);
		for (; zeros > 0; zeros--)
			g_string_append (string, "0");

		g_string_append_printf (string, "%d", field->value);
		if (it->next)  // if not last
			g_string_append_c (string, box->separator);
	}

	// set text
	g_signal_handlers_block_by_func
		(box, (gpointer) ygtk_spin_box_insert_text, box);
	gtk_entry_set_text (GTK_ENTRY (box), string->str);
	g_signal_handlers_unblock_by_func
		(box, (gpointer) ygtk_spin_box_insert_text, box);

	gtk_entry_set_width_chars (GTK_ENTRY (box), string->len);
	g_string_free (string, TRUE);

	// select active field
	gtk_editable_select_region (GTK_EDITABLE (box), min_selection,
	                            min_selection + len_selection);
}

// NOTE: we need to re-implement button press as GtkSpinButton calls
// gtk_spin_button_real_change_value directly rather than using the
// change value callback
gint ygtk_spin_box_button_press (GtkWidget *widget, GdkEventButton *event)
{
	GtkSpinButton *spin = GTK_SPIN_BUTTON (widget);

	if (!spin->button && event->window == spin->panel) {
		if (!GTK_WIDGET_HAS_FOCUS (widget))
			gtk_widget_grab_focus (widget);
		spin->button = event->button;

		// FIXME: we don't support holding the button pressed
		if (event->y <= widget->requisition.height / 2) {
			if (event->button == 1)
				ygtk_spin_box_change_value (spin, GTK_SCROLL_STEP_UP);
			else
				spin->click_child = GTK_ARROW_UP;
		}
		else  {
			if (event->button == 1)
				ygtk_spin_box_change_value (spin, GTK_SCROLL_STEP_DOWN);
			else
				spin->click_child = GTK_ARROW_DOWN;
		}
	}

	else {
		// check what field is being pressed
		if (GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event)) {

		YGtkSpinBox *box = YGTK_SPIN_BOX (widget);
		gint pos = gtk_editable_get_position (GTK_EDITABLE (widget));

		YGtkSpinBoxField *field = ygtk_spin_box_get_field (box, pos);
		if (field)
			box->active_field = g_list_index (box->fields, field);

		ygtk_spin_box_update (box);
		}
	}

	return TRUE;
}
