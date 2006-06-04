/* YGUtils.h/cc have some functionality that is shared between different parts
   of the code. */

namespace YGUtils
{
	/* Filters charatcers that are not on the valids_chars array from the text string
	   Length is used to tell the length of text, in case it isn't NUL
	   terminated (you may pass -1, if it is).
	   Use the compare string member if you won't to see if there was any change.  */
	string filter_text (const char* text, int length, const char* valid_chars);
};
