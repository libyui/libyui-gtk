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
	if (length == -1)
		length = strlen (text);

	if (str != text) {  // invalid text
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

inline void skipSpace(const char *instr, int &i)
{
	while (g_ascii_isspace (instr[i])) i++;
}

typedef struct {
	GString     *tag;
	int          tag_len : 31;
	unsigned int early_closer : 1;
} TagEntry;

static TagEntry *
tag_entry_new (GString *tag, int tag_len)
{
	static const char *early_closers[] = { "p", "li" };
	TagEntry *entry = g_new (TagEntry, 1);
	entry->tag = tag;
	entry->tag_len = tag_len;
	entry->early_closer = FALSE;

	for (unsigned int i = 0; i < G_N_ELEMENTS (early_closers); i++)
		if (!g_ascii_strncasecmp (tag->str, early_closers[i], tag_len))
			entry->early_closer = TRUE;
	return entry;
}

static void
tag_entry_free (TagEntry *entry)
{
	if (entry && entry->tag)
		g_string_free (entry->tag, TRUE);
	g_free (entry);
}

static void
emit_unclosed_tags_for (GString *outp, GQueue *tag_queue, const char *tag_str, int tag_len)
{
	gboolean matched = FALSE;

	// top-level tag ...
	if (g_queue_is_empty (tag_queue))
		return;

	do {
		TagEntry *last_entry = (TagEntry *)g_queue_pop_tail (tag_queue);
		if (!last_entry)
			break;

		if (last_entry->tag_len != tag_len ||
		    g_ascii_strncasecmp (last_entry->tag->str, tag_str, tag_len)) {
			/* different tag - emit a close ... */
			g_string_append (outp, "</");
			g_string_append_len (outp, last_entry->tag->str, last_entry->tag_len);
			g_string_append_c (outp, '>');
		} else
			matched = TRUE;

		tag_entry_free (last_entry);
	} while (!matched);
}

static gboolean
check_early_close (GString *outp, GQueue *tag_queue, TagEntry *entry)
{
	TagEntry *last_tag;

        // Early closers:
	if (!entry->early_closer)
		return FALSE;

	last_tag = (TagEntry *) g_queue_peek_tail (tag_queue);
	if (!last_tag || !last_tag->early_closer)
		return FALSE;

	if (entry->tag_len != last_tag->tag_len ||
	    g_ascii_strncasecmp (last_tag->tag->str, entry->tag->str, entry->tag_len))
		return FALSE;

	// Emit close & leave last tag on the stack

	g_string_append (outp, "</");
	g_string_append_len (outp, entry->tag->str, entry->tag_len);
	g_string_append_c (outp, '>');

	return TRUE;
}


// We have to:
//   + manually substitute the product entity.
//   + rewrite <br> and <hr> tags
//   + deal with <a attrib=noquotes>
gchar *ygutils_convert_to_xhmlt_and_subst (const char *instr, const char *product,
                                           gboolean cut_breaklines)
{
	GString *outp = g_string_new ("");
	GQueue *tag_queue = g_queue_new();
	int i = 0;

	skipSpace (instr, i);

	gboolean addOuterTag = FALSE;
	if ((addOuterTag = (instr[i] != '<')))
		g_string_append (outp, "<body>");

	for (; instr[i] != '\0'; i++)
	{
		// Tag foo
		if (instr[i] == '<') {
			gint j;
			gboolean is_close = FALSE;
			gboolean in_tag;
			int tag_len;
			GString *tag = g_string_sized_new (20);

			i++;
			skipSpace (instr, i);

			if (instr[i] == '/') {
			    i++;
			    is_close = TRUE;
			}

			skipSpace (instr, i);

			// find the tag name piece
			in_tag = TRUE;
			tag_len = 0;
			for (; instr[i] != '>' && instr[i]; i++) {
				if (in_tag) {
					if (!g_ascii_isalnum(instr[i]))
						in_tag = FALSE;
					else
						tag_len++;
				}
				g_string_append_c (tag, instr[i]);
			}

			// Unmatched tags
			if ( !is_close && tag_len == 2 &&
			     (!strncmp (tag->str, "hr", 2) ||
			      !strncmp (tag->str, "br", 2)) &&
			     tag->str[tag->len - 1] != '/')
				g_string_append_c (tag, '/');
			
			// Add quoting for un-quoted attributes
			for (j = 0; j < (signed) tag->len; j++) {
				if (tag->str[j] == '=' && tag->str[j+1] != '"') {
					g_string_insert_c (tag, j+1, '"');
					for (j++; !g_ascii_isspace (tag->str[j]) && tag->str[j]; j++);
					g_string_insert_c (tag, j, '"');
				}
			}

			// Is it an open or close ?
			j = tag->len - 1;

			while (j > 0 && g_ascii_isspace (tag->str[j])) j--;

			gboolean is_open_close = (tag->str[j] == '/');
			if (is_open_close)
				; // ignore it
			else if (is_close)
				emit_unclosed_tags_for (outp, tag_queue, tag->str, tag_len);
			else {
				TagEntry *entry = tag_entry_new (tag, tag_len);

				entry->tag = tag;
				entry->tag_len = tag_len;

				if (!check_early_close (outp, tag_queue, entry))
					g_queue_push_tail (tag_queue, entry);
				else {
					entry->tag = NULL;
					tag_entry_free (entry);
				}
			}

			g_string_append_c (outp, '<');
			if (is_close)
			    g_string_append_c (outp, '/');
			g_string_append_len (outp, tag->str, tag->len);
			g_string_append_c (outp, '>');

			if (is_close || is_open_close)
				g_string_free (tag, TRUE);
		}
		
		else if (instr[i] == '&' &&
			 !g_ascii_strncasecmp (instr + i, PROD_ENTITY,
					       sizeof (PROD_ENTITY) - 1)) {
			// 1 Magic entity
			g_string_append (outp, product);
			i += sizeof (PROD_ENTITY) - 2;
			
		}

		// removing new-lines chars, sure isn't a xhtml conversion
		// but it's still valid xhtml and GtkTextView appreciates it
		else if (cut_breaklines && instr[i] == '\n')
			;

		else // Normal text
			g_string_append_c (outp, instr[i]);
	}

	emit_unclosed_tags_for (outp, tag_queue, "", 0);
	g_queue_free (tag_queue);

	if (addOuterTag)
		g_string_append (outp, "</body>");

	return g_string_free (outp, FALSE);
}

int YGUtils::getCharsWidth (GtkWidget *widget, int chars_nb)
{
	PangoContext *context = gtk_widget_get_pango_context (widget);
	PangoFontMetrics *metrics = pango_context_get_metrics (context,
		widget->style->font_desc, pango_context_get_language (context));

	int char_width = pango_font_metrics_get_approximate_char_width (metrics);
	char_width /= PANGO_SCALE;
	pango_font_metrics_unref (metrics);

	return char_width * chars_nb;
}

int YGUtils::getCharsHeight (GtkWidget *widget, int chars_nb)
{
	PangoFontDescription *font_desc = widget->style->font_desc;
	int size = pango_font_description_get_size (font_desc);
	if (pango_font_description_get_size_is_absolute (font_desc))
		size /= PANGO_SCALE;
	return size * chars_nb;
}

int ygutils_getCharsWidth (GtkWidget *widget, int chars_nb)
{ return YGUtils::getCharsWidth (widget, chars_nb); }

int ygutils_getCharsHeight (GtkWidget *widget, int chars_nb)
{ return YGUtils::getCharsHeight (widget, chars_nb); }


#define IS_DIGIT(x) (x >= '0' && x <= '9')
int YGUtils::strcmp (const char *str1, const char *str2)
{
	// (if you think this is ugly, just wait for the Perl version! :P)
	const char *i, *j;
	for (i = str1, j = str2; *i || *j; i++, j++) {
		// number comparasion
		if (IS_DIGIT (*i) && IS_DIGIT (*j)) {
			int n1 = 0, n2 = 0;
			for (; IS_DIGIT (*i); i++)
				n1 = (*i - '0') + (n1 * 10);
			for (; IS_DIGIT (*j); j++)
				n2 = (*j - '0') + (n2 * 10);

			if (n1 != n2) {
				if (n1 < n2)
					return -1;
				// if (n1 > n2)
					return +1;
			}
			// prepare for loop
			i--; j--;
		}

		// regular character comparasion
		else if (*i != *j) {
			if (*i < *j)
				return -1;
			// if (*i > *j)
				return +1;
		}
			break;
	}
	return 0;  // identicals
}

bool YGUtils::contains (const string &haystack, const string &needle)
{
	unsigned int i, j;
	for (i = 0; i < haystack.length(); i++) {
		for (j = 0; j < needle.length() && i+j < haystack.length(); j++)
			if (g_ascii_tolower (haystack[i+j]) != g_ascii_tolower (needle[j]))
				break;
		if (j == needle.length())
			return true;
	}
	return false;
}

void YGUtils::splitPath (const string &path, string &dirname, string &filename)
{
	string::size_type i = path.find_last_of ("/");
	if (i == string::npos) {
		dirname = path;
		filename = "";
	}
	else {
		dirname = path.substr (0, i);
		filename = path.substr (i+1);
	}
}

std::list <string> YGUtils::splitString (const string &str, char separator)
{
	std::list <string> parts;
	unsigned int i, j;
	for (j = 0, i = 0; i < str.length(); i++)
		if (str[i] == separator) {
			parts.push_back (str.substr (j, i - j));
			j = ++i;
		}
	parts.push_back (str.substr (j));
	return parts;
}

void YGUtils::print_model (GtkTreeModel *model, int string_col)
{
	fprintf (stderr, "printing model...\n");
	int depth = 0;
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter_first (model, &iter)) {
		fprintf (stderr, "Couldn't even get a first iterator\n");
		return;
	}

	while (true)
	{
		// print node
		gchar *package_name = 0;
		gtk_tree_model_get (model, &iter, string_col, &package_name, -1);
		for (int i = 0; i < depth*4; i++)
			fprintf (stderr, " ");
		fprintf (stderr, "%s\n", package_name);
		g_free (package_name);

		if (gtk_tree_model_iter_has_child (model, &iter)) {
			GtkTreeIter parent = iter;
			gtk_tree_model_iter_children (model, &iter, &parent);
			depth++;
		}
		else {
			if (gtk_tree_model_iter_next (model, &iter))
				;		// continue
			else {
				// let's see if there is a parent
				GtkTreeIter child = iter;
				if (gtk_tree_model_iter_parent (model, &iter, &child) &&
				    gtk_tree_model_iter_next (model, &iter))
					depth--;
				else
					break;
			}
		}
	}
}

void YGUtils::tree_view_toggle_cb (GtkCellRendererToggle *renderer,
                                   gchar *path_str, GtkTreeModel *model)
{
	// Toggle the box
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	gint *column = (gint*) g_object_get_data (G_OBJECT (renderer), "column");
	GtkTreeIter iter;
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);

	gboolean state;
	gtk_tree_model_get (model, &iter, column, &state, -1);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, !state, -1);
}

void YGUtils::tree_view_radio_toggle_cb (GtkCellRendererToggle *renderer,
                                         gchar *path_str, GtkTreeModel *model)
{
	// Toggle the box
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	gint *column = (gint*) g_object_get_data (G_OBJECT (renderer), "column");
	GtkTreeIter iter;
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);

	// Disable radio buttons from the same parent
	GtkTreeIter parent_iter;
	if (gtk_tree_model_iter_parent (model, &parent_iter, &iter))
		do gtk_tree_store_set (GTK_TREE_STORE (model), &iter, column, FALSE, -1);
			while (gtk_tree_model_iter_next (model, &parent_iter));

	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, column, TRUE, -1);
}
