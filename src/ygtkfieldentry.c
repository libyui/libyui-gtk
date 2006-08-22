//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

/* YGtkFieldEntry widget */
// check the header file for information about this widget

/*
 * YGtkFieldEntry widget for GTK+
 * Copyright (C) 2005 Baruch Even <baruch@ev-en.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <ctype.h>
#include <string.h>

#include <glib/gprintf.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkmarshal.h>

#include "ygtkfieldentry.h"
#include "utils.h"

struct Field {
	size_t pos;
	gchar *val;
	gboolean dirty;
	guint max_width;
};

static void (*parent_insert_text)(GtkEditable *editable, const gchar *text, gint new_text_length, gint *position);
static void (*parent_delete_text)(GtkEditable *editable, gint start_pos, gint end_pos);
static void (*parent_set_position)(GtkEditable *editable, gint position);
static gchar *(*parent_get_chars)(GtkEditable *editable, gint start_pos, gint end_pos);

gunichar tab_char;

enum {
  FIELD_TEXT_CHANGED,
  CURRENT_FIELD_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0,};

static GObjectClass *parent_class;

static gchar *ygtk_field_entry_get_chars(GtkEditable *editable, gint start_pos, gint end_pos);
static gboolean ygtk_field_entry_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer user_data);
static void ygtk_field_entry_set_position(GtkEditable *editable, gint position);
static void ygtk_field_entry_insert_text(GtkEditable *editable, const gchar *text, gint new_text_length, gint *position);
static void ygtk_field_entry_delete_text(GtkEditable *editable, gint start_pos, gint end_pos);
static void ygtk_field_entry_finalize(GObject *object);
static void on_scroll_offset_changed(GtkEntry *entry, gpointer user_data);
static const gchar *ygtk_field_entry_real_get_allowed_field_chars(YGtkFieldEntry *field, gint field_num);
static const gchar *ygtk_field_entry_get_allowed_field_chars(YGtkFieldEntry *field, gint field_num);
static void ygtk_field_entry_size_request(GtkWidget *widget, GtkRequisition *requisition);

static void ygtk_field_entry_class_init(YGtkFieldEntryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	gobject_class->finalize = ygtk_field_entry_finalize;

	widget_class->size_request = ygtk_field_entry_size_request;

	klass->get_allowed_field_chars = ygtk_field_entry_real_get_allowed_field_chars;

	parent_class = g_type_class_peek_parent(klass);

	tab_char = g_utf8_get_char("\t");

	signals[FIELD_TEXT_CHANGED] =
		g_signal_new("field_text_changed",
				G_OBJECT_CLASS_TYPE (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (YGtkFieldEntryClass, field_text_changed),
				NULL, NULL,
				gtk_marshal_VOID__INT,
				G_TYPE_NONE, 1, GTK_TYPE_INT);
	signals[CURRENT_FIELD_CHANGED] =
		g_signal_new("current_field_changed",
				G_OBJECT_CLASS_TYPE (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (YGtkFieldEntryClass, current_field_changed),
				NULL, NULL,
				gtk_marshal_VOID__INT,
				G_TYPE_NONE, 1, GTK_TYPE_INT);
}

static void ygtk_field_entry_init(YGtkFieldEntry *field)
{
	field->fields = g_array_new(FALSE, TRUE, sizeof(struct Field));
	field->tabs = pango_tab_array_new(0, TRUE);
	field->marked_up = g_string_new("");
	field->block_cfc_signal = FALSE;

	g_signal_connect(G_OBJECT(field), "expose-event",
			G_CALLBACK(ygtk_field_entry_expose_event), (gpointer)field);
	g_signal_connect(G_OBJECT(field), "notify::scroll-offset",
			G_CALLBACK(on_scroll_offset_changed), NULL);
}

static void ygtk_field_entry_editable_init (GtkEditableClass *iface)
{
	parent_get_chars = iface->get_chars;
	parent_insert_text = iface->insert_text;
	parent_delete_text = iface->delete_text;
	parent_set_position = iface->set_position;

	iface->get_chars = ygtk_field_entry_get_chars;
	iface->insert_text = ygtk_field_entry_insert_text;
	iface->delete_text = ygtk_field_entry_delete_text;
	iface->set_position = ygtk_field_entry_set_position;
}

GType ygtk_field_entry_get_type(void)
{
	static GType field_entry_type = 0;

	if (!field_entry_type)
	{
		static const GTypeInfo field_entry_info =
		{
			.class_size = sizeof (YGtkFieldEntryClass),
			.class_init = (GClassInitFunc) ygtk_field_entry_class_init,
			.instance_size = sizeof (YGtkFieldEntry),
			.n_preallocs = 0,
			.instance_init = (GInstanceInitFunc) ygtk_field_entry_init,
		};
		static const GInterfaceInfo editable_info =
		{
			.interface_init = (GInterfaceInitFunc) ygtk_field_entry_editable_init,
		};

		field_entry_type = g_type_register_static(GTK_TYPE_ENTRY,
				"YGtkFieldEntry", &field_entry_info, 0);
		g_type_add_interface_static(field_entry_type, GTK_TYPE_EDITABLE, &editable_info);
	}

	return field_entry_type;
}

/* <utilities> */


static size_t get_largest_char_str_width(GtkWidget *widget,
                     const gchar *charset, size_t num_dups)
{
	g_return_val_if_fail(num_dups > 0, 0);

	size_t maxWidth = 0;
	PangoLayout *layout = gtk_widget_create_pango_layout(widget, "");

	GString *str = g_string_new("");
	const gchar *p;
	for (p = charset; *p; p = g_utf8_next_char(p)) {
		gunichar c = g_utf8_get_char(p);
		size_t i;

		g_string_truncate(str, 0);
		for (i = 0; i < num_dups; i++)
			g_string_append_unichar(str, c);
		
		pango_layout_set_text(layout, str->str, -1);

		gint width;
		pango_layout_get_pixel_size(layout, &width, NULL);

		if (width > maxWidth)
			maxWidth = width;
	}

	g_object_unref(layout);
	g_string_free(str, TRUE);

	return maxWidth;
}

static gboolean find_char_in(gunichar c, const gchar *chars_set)
{
	g_return_val_if_fail(chars_set, FALSE);
	g_return_val_if_fail(*chars_set, FALSE);

	for (; *chars_set; chars_set = g_utf8_next_char(chars_set)) {
		gunichar c_set = g_utf8_get_char(chars_set);

		if (c == c_set)
			return TRUE;
	}
	
	return FALSE;
}

/* </utilities> */

static inline struct Field *get_field(YGtkFieldEntry *field, size_t field_num)
{
	return &g_array_index(field->fields, struct Field, field_num);
}

static const gchar *get_chars_for_width(YGtkFieldEntry *field, size_t field_num)
{
	const gchar *allowed_chars = ygtk_field_entry_get_allowed_field_chars(field, field_num);
	if (*allowed_chars == 0)
		allowed_chars = "W";

	return allowed_chars;
}


/*
 *-----------------------------------------------------------------------------
 *
 * FieldEntry::ComputeLayout --
 *
 *      Compute the entire layout state: marked up string and associated
 *      meta-data, tab stops positions, ...
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Updates marked_up. Consequently, all previously computed marked up
 *      positions become stale.
 *
 *-----------------------------------------------------------------------------
 */

static void compute_layout(YGtkFieldEntry *field)
{
	/*
	 * We can't cache these values, as the theme or font might change.
	 * We could listen for when the theme or font properties change, but
	 * that's a bit more work than we really should need to do right now.
	 */
	PangoLayout *layout = gtk_widget_create_pango_layout(GTK_WIDGET(field), field->delim_str);

	int delimWidth;
	pango_layout_get_pixel_size(layout, &delimWidth, NULL);

	int offset = 0;
	size_t tabIndex = 0;
	g_string_assign(field->marked_up, "");
	size_t i;

	for (i = 0; i < ygtk_field_entry_get_field_count(field); i++) {
		struct Field *f = get_field(field, i);
		int textWidth;
		pango_layout_set_text(layout, f->val, -1);
		pango_layout_get_pixel_size(layout, &textWidth, NULL);

		int maxTextWidth = get_largest_char_str_width(GTK_WIDGET(field),
					get_chars_for_width(field, i),
					f->max_width);

		int fieldOffset;

		switch (field->field_alignment) {
		case YGTK_FIELD_ALIGN_LEFT:
			fieldOffset = offset;
			break;

		case YGTK_FIELD_ALIGN_CENTER:
			fieldOffset = offset + (maxTextWidth - textWidth) / 2;
			break;

		case YGTK_FIELD_ALIGN_RIGHT:
			fieldOffset = offset + maxTextWidth - textWidth;
			break;

		default:
			g_assert_not_reached();
			break;
		}

		if (fieldOffset != offset) {
			g_string_append_unichar(field->marked_up, tab_char);
			pango_tab_array_set_tab(field->tabs, tabIndex, PANGO_TAB_LEFT, fieldOffset);
			tabIndex++;
		}

		f->pos = field->marked_up->len;
		g_string_append(field->marked_up, f->val);
		offset += maxTextWidth;

		/*
		 * Always put a tab stop after the last field. This way selecting the
		 * beginning of the entry is symetrical with slecting the end of the
		 * entry.
		 */

		if (offset != fieldOffset + textWidth) {
			g_string_append_unichar(field->marked_up, tab_char);
			pango_tab_array_set_tab(field->tabs, tabIndex, PANGO_TAB_LEFT, offset);
			tabIndex++;
		}

		if (i != ygtk_field_entry_get_field_count(field) - 1) {
			g_string_append_unichar(field->marked_up, field->delim);
			offset += delimWidth;
		}
	}
	g_object_unref(layout);

	/* Resize in case we have used less tab stops than the max. */
	pango_tab_array_resize(field->tabs, tabIndex);
}

static void apply_layout(YGtkFieldEntry *field)
{
	GtkEditable *feditable = GTK_EDITABLE(field);
	PangoLayout *layout = gtk_entry_get_layout(GTK_ENTRY(field));
	pango_layout_set_tabs(layout, field->tabs);
	pango_layout_context_changed(layout);

	gchar *text = parent_get_chars(feditable, 0, -1);
	if (g_utf8_collate(text, field->marked_up->str) != 0) {
		size_t savedField;
		size_t savedPosInField;

		field->block_cfc_signal = TRUE;
		savedField = ygtk_field_entry_get_current_field(field, &savedPosInField);

		parent_delete_text(feditable, 0, -1);
		int pos = 0;
		parent_insert_text(feditable, field->marked_up->str, field->marked_up->len, &pos);

		ygtk_field_entry_set_current_field(field, savedField, savedPosInField);
		field->block_cfc_signal = FALSE;
	}
	g_free(text);

	size_t i;
	for (i = 0; i < ygtk_field_entry_get_field_count(field); i++) {
		struct Field *f = get_field(field, i);
		if (f->dirty) {
			f->dirty = FALSE;
			g_signal_emit(field, signals[FIELD_TEXT_CHANGED], 0, i);
		}
	}
}

/*
 * Set the value of a field. The firing of the corresponding
 * 'field_text_changed' signal, if needed, is delayed until you call
 * ApplyLayout().
 *
 */
static void set_field(YGtkFieldEntry *field, size_t field_num, const gchar *text)
{
	struct Field *f = get_field(field, field_num);

	g_return_if_fail(text != NULL);

	if (g_utf8_collate(text, f->val) == 0)
		return;

	g_free(f->val);
	f->val = g_strdup(text);
	f->dirty = TRUE;
}

/*
 *-----------------------------------------------------------------------------
 *
 * FieldEntry::Field2Position --
 *
 *      Returns the position in the marked up string at which the
 *      field begins.
 *
 * Results:
 *      The position at which the field begins.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */
static size_t Field2Position(YGtkFieldEntry *field, size_t field_num)
{
	return get_field(field, field_num)->pos;
}

/*
 *-----------------------------------------------------------------------------
 *
 * YGtkFieldEntry::Position2Field --
 *
 *      Retrieve the nearest field location corresponding to a position in the
 *      marked up string.
 *
 *      XXX Can be done in O(1) time if needed, by building an
 *          array[position] = field_location at the time we build the marked up
 *          string. --hpreg
 *
 * Results:
 *      Field location.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */
static void Position2Field(YGtkFieldEntry *field, size_t pos, size_t *field_num, size_t *pos_in_field)
{
	size_t i;
	gchar *p;
	/*
	 *  mMarkedUp:  t   f f f   t   d   t   f f f   t   d   t   f f f   t
	 *             |  |  | |  |   |   |   |  | |  |   |   |   |  | |  |  |
	 *   position: 0                                                     length
	 *      field: 0  0  0 0  0   0   1   1  1 1  1   1   2   2  2 2  2  2
	 * posInField: 0  0  1 2  3   3   0   0  1 2  3   3   0   0  1 2  3  3
	 */

	*field_num = 0;
	*pos_in_field = 0;

	g_return_if_fail(pos <= g_utf8_strlen(field->marked_up->str, field->marked_up->len)+1);

	for (i = 1, p = field->marked_up->str; i <= pos; i++, p = g_utf8_next_char(p)) {
		gunichar c = g_utf8_get_char(p);
		if (c == field->delim) {
			(*field_num)++;
			*pos_in_field = 0;
		} else if (c == tab_char) {
			/* Ignored. This is good, because they are optional. */
		} else {
			/* Non-special character. */
			(*pos_in_field)++;
		}
	}
}

static gchar *ygtk_field_entry_get_chars(GtkEditable *editable, gint start_pos, gint end_pos)
{
	YGtkFieldEntry *field = YGTK_FIELD_ENTRY(editable);
	gint i;
	GString *new_str;
	gchar *p;

	new_str = g_string_new("");

	if (end_pos < 0)
		end_pos = g_utf8_strlen(field->marked_up->str, field->marked_up->len);

	p = g_utf8_offset_to_pointer(field->marked_up->str, start_pos);
	for (i = start_pos; i < end_pos; i++, p = g_utf8_next_char(p)) {
		gunichar c = g_utf8_get_char(p);
		if (c != tab_char)
			g_string_append_unichar(new_str, c);
	}

	return g_string_free(new_str, FALSE);
}

static void ygtk_field_entry_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
	YGtkFieldEntry *field = YGTK_FIELD_ENTRY(widget);

	gtk_entry_set_width_chars(GTK_ENTRY(widget), 0);
	GTK_WIDGET_CLASS(parent_class)->size_request(widget, requisition);
	
	/* Add delimiter space */
	int delimWidth;
	PangoLayout *layout = gtk_widget_create_pango_layout(widget, field->delim_str);
	pango_layout_get_pixel_size(layout, &delimWidth, NULL);
	g_object_unref(layout);

	requisition->width += delimWidth * (ygtk_field_entry_get_field_count(field)-1);

	/* We now need to add the total width of the characters we want in the field */
	guint i;
	for (i = 0; i < ygtk_field_entry_get_field_count(field); i++) {
		int maxTextWidth = get_largest_char_str_width(
				widget,
				get_chars_for_width(field, i),
				get_field(field, i)->max_width);

		requisition->width += maxTextWidth;
	}
}

/**
 * ygtk_field_entry_set:
 * @field: The field to reset.
 * @field_count:  Number of fields in the entry.
 * @max_field_widths:  An array of size field_count for field lengths.
 * @delim: The delimiter for between the fields.
 * @align: The alignment of text inside the fields.
 *
 * Resets the parameters for the Field Entry.
 *
 **/
void ygtk_field_entry_set(YGtkFieldEntry *field, size_t field_count, guint *max_field_widths, gunichar delim, YGtkFieldAlignment align)
{
	size_t i;
	size_t total_max_field_widths = 0;
	
	g_return_if_fail(GTK_IS_FIELD_ENTRY(field));

	g_assert(field_count > 0);
	g_assert(delim != 0);

	for (i = 0; i < field_count; i++) {
		g_assert(max_field_widths[i] > 0);
		total_max_field_widths += max_field_widths[i];
	}

	field->field_alignment = align;
	field->delim = delim;

	gtk_editable_set_editable(GTK_EDITABLE(field), TRUE);

	if (field->fields->len)
		g_array_remove_range(field->fields, 0, field->fields->len);
	
	struct Field dummy_field = { .dirty = FALSE };
	for (i = 0; i < field_count; i++) {
		dummy_field.val = g_strdup("");
		dummy_field.max_width = max_field_widths[i];
		g_array_append_val(field->fields, dummy_field);
	}

	GString *tmp = g_string_new("");
	g_string_append_unichar(tmp, field->delim);
	field->delim_str = g_string_free(tmp, FALSE);

	compute_layout(field);
	apply_layout(field);
}

/**
 * ygtk_field_entry_new:
 * @field_count:  Number of fields in the entry.
 * @max_field_widths:  An array of size field_count for field lengths.
 * @delim: The delimiter for between the fields.
 * @align: The alignment of text inside the fields.
 *
 * Creates a new YGtkFieldEntry
 *
 * Return value: The new field entry as #GtkWidget
 **/
GtkWidget* ygtk_field_entry_new(size_t field_count, guint *max_field_widths, gunichar delim, YGtkFieldAlignment align)
{
	GtkWidget *w = GTK_WIDGET(g_object_new(ygtk_field_entry_get_type(), NULL));
	YGtkFieldEntry *field = YGTK_FIELD_ENTRY(w);

	ygtk_field_entry_set(field, field_count, max_field_widths, delim, align);

	return w;
}

static void ygtk_field_entry_finalize(GObject *object)
{
	YGtkFieldEntry *field;

	g_return_if_fail(GTK_IS_FIELD_ENTRY(object));
	field = YGTK_FIELD_ENTRY(object);

	size_t i;
	for (i = 0; i < ygtk_field_entry_get_field_count(field); i++) {
		struct Field *f = get_field(field, i);
		g_free(f->val);
		f->val = NULL;
	}

	g_array_free(field->fields, TRUE); field->fields = NULL;
	pango_tab_array_free(field->tabs); field->tabs = NULL;
	g_string_free(field->marked_up, TRUE); field->marked_up = NULL;
	g_free(field->delim_str); field->delim_str = NULL;

	G_OBJECT_CLASS(parent_class)->finalize(object);
}

/**
 * ygtk_field_entry_set_text:
 * @field: a #YGtkFieldEntry
 * @text: a null-terminated string
 *
 * Set the text of the entry. Split it into the fields by the seperator.
 **/
void ygtk_field_entry_set_text(YGtkFieldEntry *field, const gchar *text)
{
	gtk_entry_set_text(GTK_ENTRY(field), text);
}

/**
 * ygtk_field_entry_get_text:
 * @field: a #YGtkFieldEntry
 *
 * Get the text of the field with the seperators.
 *
 * Return Value: A freshly allocated null-terminated string. Needs to be freed
 * by the user.
 **/
gchar *ygtk_field_entry_get_text(YGtkFieldEntry *field)
{
	return gtk_editable_get_chars(GTK_EDITABLE(field), 0, -1);
}

/**
 * ygtk_field_entry_set_field_text:
 * @field: a #YGtkFieldEntry
 * @field_num: The field to set
 * @text: Text to set in the field
 *
 * Sets the text for a single field.
 **/
void ygtk_field_entry_set_field_text(YGtkFieldEntry *field, size_t field_num, const gchar *text)
{
	size_t saved_field, saved_pos_in_field;

	g_assert(field_num < ygtk_field_entry_get_field_count(field));
	g_assert(g_utf8_strlen(text, -1) <= get_field(field, field_num)->max_width);

	const gchar *allowed_chars = ygtk_field_entry_get_allowed_field_chars(field, field_num);
	if (*allowed_chars) {
		const gchar *p;
		for (p = text; *p; p = g_utf8_next_char(p)) {
			gunichar c = g_utf8_get_char(p);
			if (!find_char_in(c, allowed_chars))
				return;
		}
	}

	set_field(field, field_num, text);
	compute_layout(field);

	saved_field = ygtk_field_entry_get_current_field(field, &saved_pos_in_field);
	apply_layout(field);
	ygtk_field_entry_set_current_field(field, saved_field, saved_pos_in_field);
}

/**
 * ygtk_field_entry_get_field_text:
 * @field: a #YGtkFieldEntry
 * @field_num: The field to get the text of
 *
 * Get the text for one field inside the entry.
 *
 * Return Value:  a pointer to the content of a field. This string points to
 *                internally allocated storage in the widget and must not be
 *                freed, modified or stored.
 *
 **/
const gchar *ygtk_field_entry_get_field_text(YGtkFieldEntry *field, size_t field_num)
{
	g_assert(field_num < ygtk_field_entry_get_field_count(field));
	return get_field(field, field_num)->val;
}

/**
 * ygtk_field_entry_set_current_field:
 * @field: a #YGtkFieldEntry
 * @field_num: field of cursor
 * @pos_in_field: Position inside a the field
 *
 * Set the place of the cursor inside the entry, by setting the field and the
 * position inside the field. If pos_in_field is less than zero or more than
 * the field size the pos_in_field will be at the end of the field.
 **/
void ygtk_field_entry_set_current_field(YGtkFieldEntry *field, size_t field_num, size_t pos_in_field)
{
	size_t field_len;

	g_assert(field_num < ygtk_field_entry_get_field_count(field));

	gchar *fval = get_field(field, field_num)->val;
	field_len = g_utf8_strlen(fval, -1);

	if (pos_in_field < 0 || pos_in_field > field_len)
		pos_in_field = field_len;

	gtk_editable_set_position(GTK_EDITABLE(field), Field2Position(field, field_num) + pos_in_field);
}

/**
 * ygtk_field_entry_get_current_field:
 * @field: a #YGtkFieldEntry
 * @pos_in_field: pointer to save position in field, or NULL.
 *
 * Get the current field and position in the field.
 *
 * Return Value: current active field.
 **/
size_t ygtk_field_entry_get_current_field(YGtkFieldEntry *field, size_t *pos_in_field)
{
	size_t field_pos;
	size_t tmp_pos_in_field;

	Position2Field(field, gtk_editable_get_position(GTK_EDITABLE(field)),
			&field_pos, &tmp_pos_in_field);

	if (pos_in_field)
		*pos_in_field = tmp_pos_in_field;

	return field_pos;
}

/**
 * ygtk_field_entry_get_field_count:
 * @field: a #YGtkFieldEntry
 *
 * Get the number of fields in the entry.
 * 
 * Return Value: Number of fields
 **/
size_t ygtk_field_entry_get_field_count(YGtkFieldEntry *field)
{
	return field->fields->len;
}

static gboolean ygtk_field_entry_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
	YGtkFieldEntry *field = YGTK_FIELD_ENTRY(user_data);
	compute_layout(field);
	apply_layout(field);
	return FALSE;
}

/*
 *-----------------------------------------------------------------------------
 *
 * view::FieldEntry::set_position_vfunc --
 *
 *      Overridden virtual function to set the position in the entry.
 *      This takes into account the field separators and the direction
 *      the user may be moving the cursor and intelligently places the
 *      cursor inside of a field.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */
static void ygtk_field_entry_set_position(GtkEditable *editable, gint position)
{
	YGtkFieldEntry *field = YGTK_FIELD_ENTRY(editable);
	/* Determine the old field location. */

	size_t oldField;
	size_t oldPosInField;

	Position2Field(field, gtk_editable_get_position(editable), &oldField, &oldPosInField);

	/* Normalize the new position. < 0 means the end of the string. */

	if (position < 0)
		position = field->marked_up->len;

	/* Determine the new field location. */

	size_t field_num;
	size_t posInField;

	Position2Field(field, position, &field_num, &posInField);

	gchar *fval = get_field(field, field_num)->val;
	size_t fval_len = g_utf8_strlen(fval, -1);

	size_t inFieldNewPos = Field2Position(field, field_num) + posInField;
	if (inFieldNewPos > position) {
		/*
		 * The position is on the left edge of the field.
		 *
		 * If the user was already at the beginning of the field, and the field
		 * is not the first one, jump to the end of the previous field.
		 *
		 * If the user was not already at the beginning of the field, then jump
		 * to the beginning of the field.
		 */

		if (oldField == field_num && oldPosInField == 0 && field_num > 0) {
			field_num--;
			posInField = fval_len;
		}
	} else if (inFieldNewPos < position) {
		/*
		 * The position is on the right edge of the field.
		 *
		 * If the user was already at the end of the field, and the field is not
		 * the last one, jump to the beginning of the next field.
		 *
		 * If the user was not already at the end of the field, then jump to the
		 * end of the field.
		 */

		if (   oldField == field_num && oldPosInField == fval_len
				&& field_num < ygtk_field_entry_get_field_count(field) - 1)
		{
			field_num++;
			posInField = 0;
		}
	} else {
		/* The position is inside a field. Just jump to it. */
	}

	parent_set_position(GTK_EDITABLE(field), Field2Position(field, field_num) + posInField);

	if (oldField != field_num && !field->block_cfc_signal) {
		volatile size_t savedField;
		volatile size_t savedPosInField;

		savedField = ygtk_field_entry_get_current_field(field, (size_t *)&savedPosInField);

		g_signal_emit(field, signals[CURRENT_FIELD_CHANGED], 0, oldField);

		ygtk_field_entry_set_current_field(field, savedField, savedPosInField);
	}
}

/*
 *-----------------------------------------------------------------------------
 *
 * view::FieldEntry::insert_text_vfunc --
 *
 *      Overridden virtual function handler to insert text at the
 *      specified position. This will insert across fields, taking into
 *      account field widths, the delimiters, and whether the fields
 *      are valid. This is the same function that handles user input, and
 *      thus treats a SetText() and clipboard pastes with the exact same
 *      logic as the user typing. D&D text drops are also handled by this.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Emits field_text_changed for every updated field.
 *
 *-----------------------------------------------------------------------------
 */
static void ygtk_field_entry_insert_text(GtkEditable *editable, const gchar *text, gint new_text_length, gint *position)
{
	YGtkFieldEntry *field = YGTK_FIELD_ENTRY(editable);
	size_t field_num;
	size_t posInField;

	/* Determine the field location. */

	Position2Field(field, *position, &field_num, &posInField);

	/*
	 * Iterate through all input characters, as if the user had typed them one
	 * by one.
	 */

	for (; *text; text = g_utf8_next_char(text)) {
		gunichar c = g_utf8_get_char(text);

		if (c == tab_char) {
			/* Tabs in the input would conflict with our tab stops hack. */
			gdk_display_beep(gtk_widget_get_display(GTK_WIDGET(field)));
			break;
		}

		struct Field *f = get_field(field, field_num);
		glong f_len = g_utf8_strlen(f->val, -1);

		size_t candidateField;
		size_t candidatePosInField;

		if (c == field->delim || f_len == f->max_width) {
			if (posInField != f_len ||
					field_num == ygtk_field_entry_get_field_count(field) - 1)
			{
				gdk_display_beep(gtk_widget_get_display(GTK_WIDGET(field)));
				break;
			}

			/* Try to apply operation at the beginning of the next field. */
			candidateField = field_num + 1;
			candidatePosInField = 0;

			f = get_field(field, candidateField);
			f_len = g_utf8_strlen(f->val, -1);
		} else {
			/* Try to apply operation at the current field location. */
			candidateField = field_num;
			candidatePosInField = posInField;
		}

		if (c == field->delim) {
			/* The operation is a no-op, which always succeeds. */
			field_num = candidateField;
			posInField = candidatePosInField;
		} else {
			/* The operation is an insertion of the character. */

			if (f_len == f->max_width) {
				gdk_display_beep(gtk_widget_get_display(GTK_WIDGET(field)));
				break;
			}

			const gchar *allowed_chars = ygtk_field_entry_get_allowed_field_chars(field, field_num);
			if (*allowed_chars && !find_char_in(c, allowed_chars)) {
				gdk_display_beep(gtk_widget_get_display(GTK_WIDGET(field)));
				break;
			}

			GString *temp = g_string_new(f->val);
			g_string_insert_unichar(temp, candidatePosInField, c);

			set_field(field, candidateField, temp->str);
			compute_layout(field);
			field_num = candidateField;
			posInField = candidatePosInField + 1;

			g_string_free(temp, TRUE);
		}
	}

	apply_layout(field);

	/* Make sure 'current_field_changed' will be emitted if needed. */
	gtk_editable_set_position(GTK_EDITABLE(field), Field2Position(field, field_num) + posInField);

	/*
	 * Firing the 'current_field_changed' signal can modify the content of the
	 * entry and the current position of the entry. So re-read the current
	 * position from the entry to return the correct value for 'position'.
	 */
	*position = gtk_editable_get_position(GTK_EDITABLE(field));
}

/*
 *-----------------------------------------------------------------------------
 *
 * view::FieldEntry::delete_text_vfunc --
 *
 *      Overridden virtual function to handle text deletion. Moves to
 *      the previous field if the user pressed backspace at the beginning
 *      of a field. Otherwise, deletes the text across all fields that are
 *      passed.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Emits field_text_changed for every affected field.
 *
 *-----------------------------------------------------------------------------
 */
static void ygtk_field_entry_delete_text(GtkEditable *editable, gint start_pos, gint end_pos)
{
	YGtkFieldEntry *field = YGTK_FIELD_ENTRY(editable);

	/* Normalize the end position. < 0 means the end of the string. */
	if (end_pos < 0)
		end_pos = field->marked_up->len;

	size_t startField;
	size_t start_posInField;

	/* Determine the start field location. */

	Position2Field(field, start_pos, &startField, &start_posInField);
	size_t inFieldstart_pos = Field2Position(field, startField) + start_posInField;
	if (inFieldstart_pos > start_pos) {
		/*
		 * The left edge of the start field is being deleted.
		 *
		 * If the field is not the first one, move the start location to the end
		 * of the previous field.
		 */

		if (startField > 0) {
			startField--;
			start_posInField = g_utf8_strlen(get_field(field, startField)->val, -1);
		}
	}

	/* Determine the end field location. */
	size_t endField;
	size_t end_posInField;
	Position2Field(field, end_pos, &endField, &end_posInField);

	/* Delete all content between both field locations. */

	if (startField == endField) {
		gchar *orig = get_field(field, startField)->val;
		gchar *tmp = g_strdup(orig);

		gchar *p = tmp;

		p = g_utf8_offset_to_pointer(p, start_posInField);
		orig = g_utf8_offset_to_pointer(orig, end_posInField);

		while (*orig) {
			gunichar c;

			c = g_utf8_get_char(orig);
			g_unichar_to_utf8(c, p);

			p = g_utf8_next_char(p);
			orig = g_utf8_next_char(orig);
		}
		*p = 0;

		set_field(field, startField, tmp);
		g_free(tmp);
	} else {
		{
			gchar *orig = get_field(field, startField)->val;
			gchar *tmp = g_strdup(orig);
			gchar *p = g_utf8_offset_to_pointer(tmp, start_posInField);
			*p = 0;
			set_field(field, startField, tmp);
			g_free(tmp);
		}
		size_t i;
		for (i = startField + 1; i < endField; i++) {
			set_field(field, i, "");
		}
		{
			gchar *orig = get_field(field, endField)->val;
			gchar *tmp = g_strdup(orig);
			gchar *p = tmp;

			orig = g_utf8_offset_to_pointer(orig, end_posInField);

			while (*orig) {
				gunichar c;

				c = g_utf8_get_char(orig);
				g_unichar_to_utf8(c, p);

				p = g_utf8_next_char(p);
				orig = g_utf8_next_char(orig);
			}
			*p = 0;
			set_field(field, endField, tmp);
			g_free(tmp);
		}
	}

	/* This invalidates all marked up positions. */
	compute_layout(field);
	apply_layout(field);

	gtk_editable_set_position(editable, Field2Position(field, startField) + start_posInField);
}

static void on_scroll_offset_changed(GtkEntry *entry, gpointer user_data)
{
	/*
	 * A comment in gtkentry.h says that this field is read-only and private.
	 * But it is part of the ABI, so it will not go away until GTK+ 3.0, and
	 * I'm writing to it to workaround a GTK deficiency. So yes, it is clearly
	 * an abuse, but no, I'm not ashamed of it. --hpreg
	 */

	entry->scroll_offset = 0;
}

static const gchar *ygtk_field_entry_get_allowed_field_chars(YGtkFieldEntry *field, gint field_num)
{
	g_return_val_if_fail(GTK_IS_FIELD_ENTRY(field), "");

	return YGTK_FIELD_ENTRY_GET_CLASS(field)->get_allowed_field_chars(field, field_num);
}

static const gchar *ygtk_field_entry_real_get_allowed_field_chars(YGtkFieldEntry *field, gint field_num)
{
	return "";
}


/**
 * ygtk_field_entry_set_delim:
 * @field: a #YGtkFieldEntry
 * @delim: The delimiter for the field
 *
 * Set the new delimiter of the field.
 **/
void ygtk_field_entry_set_delim(YGtkFieldEntry *field, gunichar delim)
{
	g_return_if_fail(GTK_IS_FIELD_ENTRY(field));
	
	field->delim = delim;
	compute_layout(field);
	apply_layout(field);
}


/**
 * ygtk_field_entry_get_delim:
 * @field: a #YGtkFieldEntry
 *
 * Get the delimiter of the field.
 *
 * Return Value: The field delimiter.
 **/
gunichar ygtk_field_entry_get_delim(YGtkFieldEntry *field)
{
	g_return_val_if_fail(GTK_IS_FIELD_ENTRY(field), 0);

	return field->delim;
}
