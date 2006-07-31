#ifndef YGUTILS_H
#define YGUTILS_H

#include <string>
#include <gtk/gtktextview.h>
#include <gtk/gtkeditable.h>

/* YGUtils.h/cc have some functionality that is shared between different parts
   of the code. */

namespace YGUtils
{
	/* Replaces '&' accelerator like Yast likes by the '_' that Gnome prefers. */
	string mapKBAccel(const char *src);

	/* Filters characters that are not on the valids_chars array from the text string
	   Length is used to tell the length of text, in case it isn't NUL
	   terminated (you may pass -1, if it is).
	   Use the compare string member if you won't to see if there was any change.  */
	string filterText (const char* text, int length, const char* valid_chars);

	/* Convinience call for widgets that implement GtkEditable interface.
	   This function inserts and deletes text, if needed, so you may want
	   to block those signals, if you have them set.  */
	void filterText (GtkEditable *editable, int pos, int length,
	                 const char *valid_chars);

	/* Adds functionality to GtkTextView to scroll to bottom. */
	void scrollTextViewDown(GtkTextView *text_view);

	/* Returns the average width of the given number of characters in pixels. */
	int getCharsWidth (GtkWidget *widget, int chars_nb);
	int getCharsHeight (GtkWidget *widget, int chars_nb);

	/* A more sane strcmp() from the user point of view that honors numbers. */
	int strcmp (const char *str1, const char *str2);
};

extern "C" {
	/* Convert html to xhtml (or at least try) */
	gchar *ygutils_convert_to_xhmlt_and_subst (const char *instr, const char *product);
};

#endif // YGUTILS_H
