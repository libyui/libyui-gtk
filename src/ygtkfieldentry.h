//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

/* YGtkFieldEntry is an extension of GtkEntry with the added
   functionality of being able to define fields (useful for when
   you need the user to set a IP address or time/date). The number
   of fields, their individual range and separation character
   is all customizable.

   Code from the VMware widgets set (http://view.sourceforge.net/classes.php).
*/

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

#ifndef YGTK_FIELD_ENTRY_H
#define YGTK_FIELD_ENTRY_H

#include <gdk/gdk.h>
#include <gtk/gtkentry.h>


G_BEGIN_DECLS


#define YGTK_TYPE_FIELD_ENTRY                 (ygtk_field_entry_get_type ())
#define YGTK_FIELD_ENTRY(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), YGTK_TYPE_FIELD_ENTRY, YGtkFieldEntry))
#define GTK_FIELD_ENTRY_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), YGTK_TYPE_FIELD_ENTRY, YGtkFieldEntryClass))
#define GTK_IS_FIELD_ENTRY(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), YGTK_TYPE_FIELD_ENTRY))
#define GTK_IS_FIELD_ENTRY_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), YGTK_TYPE_FIELD_ENTRY))
#define YGTK_FIELD_ENTRY_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), YGTK_TYPE_FIELD_ENTRY, YGtkFieldEntryClass))


typedef struct _YGtkFieldEntry        YGtkFieldEntry;
typedef struct _YGtkFieldEntryClass   YGtkFieldEntryClass;

typedef enum {
	YGTK_FIELD_ALIGN_LEFT,
	YGTK_FIELD_ALIGN_CENTER,
	YGTK_FIELD_ALIGN_RIGHT
} YGtkFieldAlignment;

struct _YGtkFieldEntry;

struct _YGtkFieldEntryClass
{
	GtkEntryClass parent_class;

	/* signals */
	void     (* field_text_changed)(YGtkFieldEntry *field, gint field_num);
	void     (* current_field_changed)(YGtkFieldEntry *field, gint field_num);

	/* vtable */
	const gchar *(* get_allowed_field_chars)(YGtkFieldEntry *field, gint field_num);
};

struct _YGtkFieldEntry
{
	GtkEntry entry;

	YGtkFieldAlignment field_alignment;
	gunichar delim;
	gchar *delim_str;

	GArray *fields;
	PangoTabArray *tabs;
	GString *marked_up;

	gboolean block_cfc_signal;
};


GType ygtk_field_entry_get_type(void) G_GNUC_CONST;

GtkWidget* ygtk_field_entry_new(size_t field_count, guint *max_field_widths, gunichar delim, YGtkFieldAlignment align);

void ygtk_field_entry_set_delim(YGtkFieldEntry *field, gunichar delim);
gunichar ygtk_field_entry_get_delim(YGtkFieldEntry *field) G_GNUC_CONST;

void ygtk_field_entry_set(YGtkFieldEntry *field, size_t field_count, guint *max_field_widths, gunichar delim, YGtkFieldAlignment align);

void ygtk_field_entry_set_text(YGtkFieldEntry *field, const gchar *text);
gchar *ygtk_field_entry_get_text(YGtkFieldEntry *field);

void ygtk_field_entry_set_field_text(YGtkFieldEntry *field, size_t field_num, const gchar *text);
const gchar *ygtk_field_entry_get_field_text(YGtkFieldEntry *field, size_t field_num) G_GNUC_CONST;

void ygtk_field_entry_set_current_field(YGtkFieldEntry *field, size_t field_num, size_t pos_in_field);
size_t ygtk_field_entry_get_current_field(YGtkFieldEntry *field, size_t *pos_in_field) G_GNUC_CONST;

size_t ygtk_field_entry_get_field_count(YGtkFieldEntry *field) G_GNUC_CONST;


G_END_DECLS

#endif /* YYGTK_FIELD_ENTRY_H */
