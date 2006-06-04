#include <config.h>
#include <YGUI.h>
#include "YGUtils.h"

void YGUtils::setLabel (GtkLabel * widget, const YCPString & label)
{
	IMPL;
	static const char *m_start_tag, *m_end_tag;
	char *escaped = g_markup_escape_text (label->value_cstr(), -1);
	char *str;
	if (m_start_tag) {
			str = g_strdup_printf ("<%s>%s</%s>",
		          m_start_tag, escaped, m_end_tag);
			g_free (escaped);
	} else
			str = escaped;
	fprintf (stderr, "Set label '%s'\n", str);
	gtk_label_set_markup (widget, str);
	g_free (str);
}

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

bool YGUtils::is_str_valid (const char* text, int length, const char* valids)
{
	if (strlen(valids) == 0)
	  return true;

	const char *i, *j;
	for(i = text; *i && length == -1 ? true : i < text+length; i++) {
		for(j = valids; *j; j++)
			if(*i == *j)
				break;
		if (!*j)
		  return false;
		}

	return true;
}
