#include <config.h>
#include <YGUI.h>
#include "YGUtils.h"

string YGUtils::filter_text (const char* text, int length, const char* valid_chars)
{
	if (strlen (valid_chars) == 0) {
		if (length == -1) return string(text);
		else              return string(text, length);
		}

	string str;
	const char *i, *j;
	for(i = text; *i && length == -1 ? true : i < text+length; i++)
		for(j = valid_chars; *j; j++)
			if(*i == *j) {
				str += *i;
				break;
				}
	return str;
}
