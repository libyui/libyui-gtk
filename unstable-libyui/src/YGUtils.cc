/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>
#include "YGUI.h"
#include "YGUtils.h"

string YGUtils::mapKBAccel (const string &src)
{
	// we won't use use replace since we also want to escape _ to __
	int length = src.length();
	string str;
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

void YGUtils::setFilter (GtkEntry *entry, const string &validChars)
{
	struct inner {
		static void insert_text_cb (GtkEditable *editable, const gchar *new_text,
		                            gint new_text_length, gint *pos)
		{
			const gchar *valid_chars = (gchar *) g_object_get_data (G_OBJECT (editable),
			                                                        "valid-chars");
			if (valid_chars) {
				const gchar *i, *j;
				for (i = new_text; *i; i++) {
					for (j = valid_chars; *j; j++) {
						if (*i == *j)
							break;
					}
					if (!*j) {
						// not valid text
						g_signal_stop_emission_by_name (editable, "insert_text");
						gdk_beep();
						return;
					}
				}
			}
		}
	};

	if (!validChars.empty()) {
		gchar *chars = g_strdup (validChars.c_str());
		g_object_set_data_full (G_OBJECT (entry), "valid-chars", chars, g_free);
		g_signal_connect (G_OBJECT (entry), "insert-text",
		                  G_CALLBACK (inner::insert_text_cb), NULL);
	}
	else
		g_object_disconnect (G_OBJECT (entry), "insert-text",
		                     G_CALLBACK (inner::insert_text_cb), NULL);
}

void ygutils_setFilter (GtkEntry *entry, const char *validChars)
{ YGUtils::setFilter (entry, validChars); }

void YGUtils::replace (string &str, const char *mouth, int mouth_len, const char *food)
{
	if (mouth_len < 0)
		mouth_len = strlen (mouth);
	unsigned int i = 0;
	while ((i = str.find (mouth, i)) != string::npos)
	{
		str.erase (i, mouth_len);
		str.insert (i, food);
	}
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

void YGUtils::escapeMarkup (string &str)
{
	for (unsigned int i = 0; i < str.length(); i++) {
		switch (str[i]) {
			case '<':
				str.erase (i, 1);
				str.insert (i, "&lt;");
				break;
			case '>':
				str.erase (i, 1);
				str.insert (i, "&gt;");
				break;
			case '&':
				str.erase (i, 1);
				str.insert (i, "&amp;");
				break;
			default:
				break;
		}
	}
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
gchar *ygutils_convert_to_xhmlt_and_subst (const char *instr, const char *product)
{
	GString *outp = g_string_new ("");
	GQueue *tag_queue = g_queue_new();
	int i = 0;

	skipSpace (instr, i);

	// We must add an outer tag to make GMarkup happy
	gboolean addOuterTag = TRUE;
//	gboolean addOuterTag = FALSE;
//	if ((addOuterTag = (instr[i] != '<')))
		g_string_append (outp, "<body>");

	for (; instr[i] != '\0'; i++)
	{
		// Tag foo
		if (instr[i] == '<') {
			// ignore comments
			if (strncmp (&instr[i], "<!--", 4) == 0) {
				for (i += 3; instr[i] != '\0'; i++)
					if (strncmp (&instr[i], "-->", 3) == 0) {
						i += 2;
						break;
					}
				continue;
			}

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
			     (!g_ascii_strncasecmp (tag->str, "hr", 2) ||
			      !g_ascii_strncasecmp (tag->str, "br", 2)) &&
			     tag->str[tag->len - 1] != '/')
				g_string_append_c (tag, '/');

			// Add quoting for un-quoted attributes
			for (int j = 0; j < (signed) tag->len; j++) {
				if (tag->str[j] == '=') {
					if (tag->str[j+1] != '"') {
						g_string_insert_c (tag, j+1, '"');
						for (j++; !g_ascii_isspace (tag->str[j]) && tag->str[j]; j++) ;
						g_string_insert_c (tag, j, '"');
					}
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
/*
		else if (instr[i] == '&' &&
			 !g_ascii_strncasecmp (instr + i, PROD_ENTITY,
					       sizeof (PROD_ENTITY) - 1)) {
			// 1 Magic entity
			g_string_append (outp, product);
			i += sizeof (PROD_ENTITY) - 2;
		}
*/
		// non-break space entity
		else if (instr[i] == '&' &&
			 !g_ascii_strncasecmp (instr + i, "&nbsp;",
					       sizeof ("&nbsp;") - 1)) {
			// Replace this by a white-space
			g_string_append (outp, " ");
			i += sizeof ("&nbsp;") - 2;
		}

#if 0
		// removing new-lines chars sure isn't a xhtml conversion
		// but it's still valid xhtml and GtkTextView appreciates it
		else if (/*cut_breaklines &&*/ instr[i] == '\n') {
			// In HTML a breakline should be treated as a white space when
			// not in the start of a paragraph.
			if (i > 0 && instr[i-1] != '>' && !g_ascii_isspace (instr[i-1]))
				g_string_append_c (outp, ' ');
		}
#endif

		else // Normal text
			g_string_append_c (outp, instr[i]);
	}

	emit_unclosed_tags_for (outp, tag_queue, "", 0);
	g_queue_free (tag_queue);

	if (addOuterTag)
		g_string_append (outp, "</body>");

	gchar *ret = g_string_free (outp, FALSE);
	return ret;
}

int YGUtils::getCharsWidth (GtkWidget *widget, int chars_nb)
{
	PangoContext *context = gtk_widget_get_pango_context (widget);
	PangoFontMetrics *metrics = pango_context_get_metrics (context,
		widget->style->font_desc, NULL);

	int width = pango_font_metrics_get_approximate_char_width (metrics);
	pango_font_metrics_unref (metrics);

	return PANGO_PIXELS (width) * chars_nb;
}

int YGUtils::getCharsHeight (GtkWidget *widget, int chars_nb)
{
	PangoContext *context = gtk_widget_get_pango_context (widget);
	PangoFontMetrics *metrics = pango_context_get_metrics (context,
		widget->style->font_desc, NULL);

	int height = pango_font_metrics_get_ascent (metrics) +
	             pango_font_metrics_get_descent (metrics);
	pango_font_metrics_unref (metrics);

	return PANGO_PIXELS (height) * chars_nb;
}

void YGUtils::setWidgetFont (GtkWidget *widget, PangoWeight weight, double scale)
{
	PangoFontDescription *font_desc = widget->style->font_desc;
	int size = pango_font_description_get_size (font_desc);

	PangoFontDescription* font = pango_font_description_new();
	pango_font_description_set_weight (font, weight);
	pango_font_description_set_size   (font, (int)(size * scale));

	gtk_widget_modify_font (widget, font);
	pango_font_description_free (font);
}

int ygutils_getCharsWidth (GtkWidget *widget, int chars_nb)
{ return YGUtils::getCharsWidth (widget, chars_nb); }
int ygutils_getCharsHeight (GtkWidget *widget, int chars_nb)
{ return YGUtils::getCharsHeight (widget, chars_nb); }
void ygutils_setWidgetFont (GtkWidget *widget, PangoWeight weight, double scale)
{ YGUtils::setWidgetFont (widget, weight, scale); }

int YGUtils::strcmp (const char *str1, const char *str2)
{
	// (if you think this is ugly, just wait for the Perl version! :P)
	const char *i, *j;
	for (i = str1, j = str2; *i && *j; i++, j++) {
		// number comparasion
		if (isdigit (*i) && isdigit (*j)) {
			int n1, n2;
			for (n1 = 0; isdigit (*i); i++)
				n1 = (*i - '0') + (n1 * 10);
			for (n2 = 0; isdigit (*j); j++)
				n2 = (*j - '0') + (n2 * 10);

			if (n1 != n2)
				return n1 - n2;

			// prepare for loop
			i--; j--;
		}

		// regular character comparasion
		else if (g_ascii_tolower (*i) != g_ascii_tolower(*j))
			return g_ascii_tolower (*i) - g_ascii_tolower (*j);
	}
	if (*i)
		return -1;
	if (*j)
		return 1;
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

std::list <string> YGUtils::splitString (const string &str, char separator)
{
	std::list <string> parts;
	unsigned int i, j;
	// ignore first character, if separator
	i = j = (str[0] == separator) ? 1 : 0;
	for (; i < str.length(); i++)
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
	GtkTreeIter parent_iter, child_iter;
	if (gtk_tree_model_iter_parent (model, &parent_iter, &iter) &&
	    gtk_tree_model_iter_children (model, &child_iter, &parent_iter)) {
		do gtk_tree_store_set (GTK_TREE_STORE (model), &child_iter, column, FALSE, -1);
			while (gtk_tree_model_iter_next (model, &child_iter));
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter, column, TRUE, -1);
	}
}

gint YGUtils::sort_compare_cb (GtkTreeModel *model, GtkTreeIter *a,
                               GtkTreeIter *b, gpointer data)
{
	IMPL
	gint col = GPOINTER_TO_INT (data);
	gchar *str1 = 0, *str2 = 0;
	gtk_tree_model_get (model, a, col, &str1, -1);
	gtk_tree_model_get (model, b, col, &str2, -1);
	gint cmp = 0;
	if (str1 && str2)
		cmp = YGUtils::strcmp (str1, str2);

	if (str1)
		g_free (str1);
	if (str2)
		g_free (str2);
	return cmp;
}

static void header_clicked_cb (GtkTreeViewColumn *column, GtkTreeSortable *sortable)
{
	IMPL
	GtkTreeViewColumn *last_sorted =
		(GtkTreeViewColumn *) g_object_get_data (G_OBJECT (sortable), "last-sorted");
	int id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (column), "id"));

	GtkSortType sort = GTK_SORT_ASCENDING;
	if (last_sorted != column) {
		if (last_sorted)
			gtk_tree_view_column_set_sort_indicator (last_sorted, FALSE);
		gtk_tree_view_column_set_sort_indicator (column, TRUE);
		g_object_set_data (G_OBJECT (sortable), "last-sorted", column);
	}
	else {
		sort = gtk_tree_view_column_get_sort_order (column);
		sort = sort == GTK_SORT_ASCENDING ? GTK_SORT_DESCENDING : GTK_SORT_ASCENDING;
	}

	gtk_tree_view_column_set_sort_order (column, sort);
	gtk_tree_sortable_set_sort_column_id (sortable, id, sort);
}

void YGUtils::tree_view_set_sortable (GtkTreeView *view, int default_sort_col)
{
	IMPL
	g_assert (gtk_tree_view_get_headers_visible (view));

	/* Set all string columns clickable. */
	GtkTreeSortable *sortable = GTK_TREE_SORTABLE (gtk_tree_view_get_model (view));
	// we need a pointer, this function is used by different stuff, so to we'll
	// use g_object_*_data() to do of garbage collector
	g_object_set_data (G_OBJECT (sortable), "last-sorted", NULL);

	GList *columns = gtk_tree_view_get_columns (view);
	for (GList *i = g_list_first (columns); i; i = i->next) {
		GtkTreeViewColumn *column = (GtkTreeViewColumn *) i->data;
		int col_nb = g_list_position (columns, i);

		// check if it really is a string column
		bool string_col = false;
		GList *renderers = gtk_tree_view_column_get_cell_renderers (column);
		for (GList *j = g_list_first (renderers); j; j = j->next)
			if (GTK_IS_CELL_RENDERER_TEXT (j->data)) {
				string_col = true;
				break;
			}
		g_list_free (renderers);
		if (!string_col)
			continue;

		// set sortable and clickable
		gtk_tree_sortable_set_sort_func (sortable, col_nb, sort_compare_cb,
		                                 GINT_TO_POINTER (col_nb), NULL);
		g_object_set_data (G_OBJECT (column), "id", GINT_TO_POINTER (col_nb));
		gtk_tree_view_column_set_clickable (column, TRUE);
		g_signal_connect (G_OBJECT (column), "clicked",
		                  G_CALLBACK (header_clicked_cb), sortable);
		if (col_nb == default_sort_col)
			header_clicked_cb (column, sortable);
	}
	g_list_free (columns);
}

GValue YGUtils::floatToGValue (float num)
{
	GValue value = { 0 };
	g_value_init (&value, G_TYPE_FLOAT);
	g_value_set_float (&value, num);
	return value;
}

#define SCROLLING_TIME 700
#define SCROLLING_STEP 100
struct ScrollData {
	int orix, oriy, diffx, diffy, time;
	GtkTreeView *view;
};

static gboolean scroll_timeout (gpointer _data) {
	ScrollData *data = (ScrollData *) _data;
	data->time += SCROLLING_STEP;

	int x = data->orix + data->diffx;
	int y = ((data->diffy * data->time) / SCROLLING_TIME) + data->oriy;

	gtk_tree_view_scroll_to_point (data->view, x, y);
	return data->time < SCROLLING_TIME;
}

void YGUtils::tree_view_smooth_scroll_to_point (GtkTreeView *view, gint x, gint y) {
	GdkRectangle rect;
	gtk_tree_view_get_visible_rect (view, &rect);
	if (rect.x == x && rect.y == y)
		return;

	ScrollData *data = (ScrollData *) g_malloc (sizeof (ScrollData));
	data->orix = rect.x;
	data->oriy = rect.y;
	data->diffx = x - rect.x;
	data->diffy = y - rect.y;
	data->time = 0;
	data->view = view;

	static int id = 0;
	if (id)
		g_source_remove (id);
	id = g_timeout_add_full (G_PRIORITY_DEFAULT, SCROLLING_STEP, scroll_timeout, data, g_free);
}

GdkPixbuf *YGUtils::loadPixbuf (const string &filename)
{
	GdkPixbuf *pixbuf = NULL;
	if (!filename.empty()) {
		GError *error = 0;
		pixbuf = gdk_pixbuf_new_from_file (filename.c_str(), &error);
		if (!pixbuf)
			y2warning ("Could not load icon: %s.\nReason: %s\n",
				filename.c_str(), error->message);
	}
	return pixbuf;
}

#define ENGLISH_STOCK_ITEMS

#ifdef ENGLISH_STOCK_ITEMS
struct StockMap { const char *ycp_label, *gtk_stock; };
static const StockMap stockMap[] = {
	// keep them sorted!
	{"Abort",     GTK_STOCK_CANCEL      },
	{"Accept",    GTK_STOCK_APPLY       },
	{"Add",       GTK_STOCK_ADD         },
	{"Back",      GTK_STOCK_GO_BACK     },
	{"Cancel",    GTK_STOCK_CANCEL      },
	{"Configure", GTK_STOCK_PREFERENCES },
	{"Continue",  GTK_STOCK_OK          },
	{"Delete",    GTK_STOCK_DELETE      },
	{"Down",      GTK_STOCK_GO_DOWN     },
	{"Edit",      GTK_STOCK_EDIT        },
	{"Launch",    GTK_STOCK_EXECUTE     },
	{"Next",      GTK_STOCK_GO_FORWARD  },
	{"No",        GTK_STOCK_NO          },
	{"OK",        GTK_STOCK_OK          },
	{"Quit",      GTK_STOCK_QUIT        },
	{"Search",    GTK_STOCK_FIND        },
	{"Up",        GTK_STOCK_GO_UP       },
	{"Yes",       GTK_STOCK_YES         },
};
#define STOCKMAP_SIZE (sizeof (stockMap)/sizeof(StockMap))
static int strcmp_cb (const void *a, const void *b)
{ return strcmp ((char *) a, ((StockMap *) b)->ycp_label); }

void YGUtils::setStockIcon (GtkWidget *button, std::string ycp_str)
{
	// is English the current locale?
	static bool firstTime = true, isEnglish;
	if (firstTime) {
		char *lang = getenv ("LANG");
		isEnglish = !lang || (!*lang) || !strcmp (lang, "C") ||
		            (lang[0] == 'e' && lang[1] == 'n') ||
		            !strcmp (lang, "POSIX");
		firstTime = false;
	}
	if (!isEnglish)
		return;

	unsigned int i = 0;
	while ((i = ycp_str.find ('_', i)) != string::npos)
		ycp_str.erase (i, 1);

	bool failed = true;
	void *ptr;
	ptr = bsearch (ycp_str.c_str(), stockMap, STOCKMAP_SIZE,
	               sizeof(stockMap[0]), strcmp_cb);
	if (ptr) {
		const char *stock = ((StockMap *) ptr)->gtk_stock;
		GdkPixbuf *pixbuf;
		pixbuf = gtk_widget_render_icon (button, stock,
		                                 GTK_ICON_SIZE_BUTTON, NULL);
		if (pixbuf) {
			GtkWidget *image = gtk_image_new_from_pixbuf (pixbuf);
			gtk_button_set_image (GTK_BUTTON (button), image);
			g_object_unref (G_OBJECT (pixbuf));
			failed = false;
		}
	}
	if (failed)
		gtk_button_set_image (GTK_BUTTON (button), NULL);
}

void ygutils_setStockIcon (GtkWidget *button, const char *ycp_str)
{ YGUtils::setStockIcon (button, ycp_str); }

#else
void ygutils_setStockIcon (GtkWidget *button, const char *ycp_str) {}
#endif

