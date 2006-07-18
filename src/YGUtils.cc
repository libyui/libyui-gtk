#include <config.h>
#include <YGUI.h>
#include "YGUtils.h"

string YGUtils::mapKBAccel(const char *src)
{
	string str (src);
	for (unsigned int i = 0; i < str.length(); i++)
		if (str[i] == '&') {
			str[i] = '_';
			break;
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
	int i, j;
	for(i = 0; text[i] && i < length; i++)
		for(j = 0; valid_chars[j]; j++)
			if(text[i] == valid_chars[j]) {
				str += text[i];
				break;
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
