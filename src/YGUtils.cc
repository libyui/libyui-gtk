#include <config.h>
#include <YGUI.h>
#include "YGUtils.h"

string YGUtils::mapKBAccel(const char *src)
{
	string str;
	int length = strlen (src);

	str.reserve (length);
	for (int i = 0; i < length; i++) {
		if (src[i] == '_')
			str += "__";
		else if (src[i] == '&')
			str += '_';
		else
		    str += src[i];
	}

	return str;
}

string YGUtils::filterText (const char* text, int length, const char *valid_chars)
{
	if (length == -1)
		length = strlen (text);
	if (strlen (valid_chars) == 0)
		return string(text);

	string str;
	for (int i = 0; text[i] && i < length; i++) {
		for (int j = 0; valid_chars[j]; j++)
			if(text[i] == valid_chars[j]) {
				str += text[i];
				break;
			}
	}
	return str;
}

void YGUtils::filterText (GtkEditable *editable, int pos, int length,
                          const char *valid_chars)
{
	gchar *text = gtk_editable_get_chars (editable, pos, pos + length);
	string str = filterText (text, length, valid_chars);
printf ("inserted text: %s\n", text);
printf ("pos: %d - length: %d\n", pos, length);
printf ("cursor pos: %d\n", gtk_editable_get_position (editable));
	if (length == -1)
		length = strlen (text);

	if (str != text) {  // invalid text
printf ("to be replaced by: %s\n", str.c_str());
		// delete current text
		gtk_editable_delete_text (editable, pos, length);
		// insert correct text
		gtk_editable_insert_text (editable, str.c_str(), str.length(), &pos);

		g_signal_stop_emission_by_name (editable, "insert_text");
		gdk_beep();  // BEEP!
	}

	g_free (text);
}

void YGUtils::scrollTextViewDown(GtkTextView *text_view)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (text_view);
	GtkTextIter end_iter;
	gtk_text_buffer_get_end_iter (buffer, &end_iter);
	GtkTextMark *end_mark;
	end_mark = gtk_text_buffer_create_mark
	               (buffer, NULL, &end_iter, FALSE);
	gtk_text_view_scroll_to_mark (text_view,
	               end_mark, 0.0, FALSE, 0, 0);
	gtk_text_buffer_delete_mark (buffer, end_mark);
}

#define PROD_ENTITY "&product;"

// We have to:
//   + manually substitute the product entity.
//   + rewrite <br> and <hr> tags
//   + deal with <a attrib=noquotes>
gchar *ygutils_convert_to_xhmlt_and_subst (const char *instr, const char *product)
{
	GString *outp = g_string_new ("");
	int i = 0;
	// elide leading whitespace
	while (g_ascii_isspace (instr[i++]));

	gboolean addOuterTag = FALSE;
	if ((addOuterTag = (instr[i] != '<')))
		g_string_append (outp, "<body>");

	for (i = 0; instr[i] != '\0'; i++)
	{
		// Tag foo
		if (instr[i] == '<') {
			GString *tag = g_string_sized_new (20);

			i++;
			for (; instr[i] != '>'; i++)
				g_string_append_c (tag, instr[i]);

			// Unmatched tags
			if ( (!strncmp (tag->str, "hr", 2) ||
			      !strncmp (tag->str, "br", 2)) &&
			     tag->str[tag->len - 1] != '/')
				g_string_append_c (tag, '/');
			
			// Add quoting for un-quoted attributes
			for (guint j = 0; j < tag->len; j++) {
				if (tag->str[j] == '=' && tag->str[j+1] != '"') {
					g_string_insert_c (tag, j+1, '"');
					for (j++; !g_ascii_isspace (tag->str[j]) && tag->str[j]; j++);
					g_string_insert_c (tag, j, '"');
				}
			}

			g_string_append_c (outp, '<');
			g_string_append_len (outp, tag->str, tag->len);
			g_string_append_c (outp, '>');
			
			g_string_free (tag, TRUE);
		}
		
		else if (instr[i] == '&' &&
			 !g_ascii_strncasecmp (instr + i, PROD_ENTITY,
					       sizeof (PROD_ENTITY) - 1)) {
			// 1 Magic entity
			g_string_append (outp, product);
			i += sizeof (PROD_ENTITY) - 2;
			
		} else // Normal text
			g_string_append_c (outp, instr[i]);
	}

	if (addOuterTag)
		g_string_append (outp, "</body>");

	return g_string_free (outp, FALSE);
}
