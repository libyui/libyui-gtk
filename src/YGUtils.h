/* YGUtils.h/cc have some functionality that is shared between different parts
   of the code. */

namespace YGUtils
{
	/* Assigns a string with '&' shortcuts to a GtkLabel. */
	void setLabel (GtkLabel* widget, const YCPString & label/* TODO: , GtkWidget* buddy*/);

	/* Checks a string text against a string of valid characters and returns if text has any
	   invalid character. Length is used to tell the length of text, in case it isn't NUL
	   terminated (you may pass -1, if it is) */
	bool is_str_valid (const char* text, int length, const char* valids);

	/* Filters charatcers that are not on the valids_chars array from the text string
	   Length is used to tell the length of text, in case it isn't NUL
	   terminated (you may pass -1, if it is).
	   Use the compare string member if you won't to see if there was any change.  */
	string filter_text (const char* text, int length, const char* valid_chars);

	/* Converts an integer to a string. */
	string int_to_string (int i);
};
