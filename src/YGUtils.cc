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

string YGUtils::filterText (const char* text, int length, const char* valid_chars)
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
