/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#ifndef YGUTILS_H
#define YGUTILS_H

#include <string>
#include <list>
#include <gtk/gtktextview.h>
#include <gtk/gtkeditable.h>

/* YGUtils.h/cc have some functionality that is shared between different parts
   of the code. */

namespace YGUtils
{
	/* Replaces Yast's '&' accelerator by Gnome's '_' (and proper escaping). */
	string mapKBAccel (const char *src);

	/* Filters characters that are not on the valids_chars array from the text string
	   Length is used to tell the length of text, in case it isn't NUL
	   terminated (you may pass -1, if it is).
	   Use the compare string member if you won't to see if there was any change.  */
	string filterText (const char* text, int length, const char* valid_chars);

	/* Convenience call for widgets that implement GtkEditable interface.
	   This function inserts and deletes text, if needed, so you may want
	   to block those signals, if you have them set.  */
	void filterText (GtkEditable *editable, int pos, int length,
	                 const char *valid_chars);

	/* Replaces every 'mouth' by 'food' in 'str'. */
	void replace (string &str, const char *mouth, int mouth_len, const char *food);

	/* Escapes markup text (eg. changes '<' by '\<').
	   If break_line is true, the break line character will be transformed in <br/> */
	string escape_markup (const string &str, bool break_line = false);

	/* Adds functionality to GtkTextView to scroll to bottom. */
	void scrollTextViewDown(GtkTextView *text_view);

	/* Returns the average width of the given number of characters in pixels. */
	int getCharsWidth (GtkWidget *widget, int chars_nb);
	int getCharsHeight (GtkWidget *widget, int chars_nb);

	/* Sets some widget font proprities. */
	void setWidgetFont (GtkWidget *widget, PangoWeight weight, double scale);

	/* A more sane strcmp() from the user point of view that honors numbers.
	   i.e. "20" < "100" */
	int strcmp (const char *str1, const char *str2);

	/* Checks if a std::string contains some other string (case insensitive). */
	bool contains (const string &haystack, const string &needle);

	/* Splits a string into parts as separated by the separator characters.
	   eg: splitString ("Office/Writer", '/') => { "Office", "Writer" } */
	std::list <string> splitString (const string &str, char separator);

	/* Prints a GtkTreeModel for debugging purposes. */
	void print_model (GtkTreeModel *model, int string_col);

	/* To be used as a callback to sort tree views. */
	gint sort_compare_cb (GtkTreeModel *model, GtkTreeIter *a,
	                      GtkTreeIter *b, gpointer data);

	/* To be used as a callback for a GtkTreeView with toggle cells. */
	void tree_view_radio_toggle_cb (GtkCellRendererToggle *renderer,
	                                gchar *path_str, GtkTreeModel *model);

	/* Goes through all GtkTreeView columns and checks for TextCellRenderers,
	   setting those columns as sortable. */
	void tree_view_set_sortable (GtkTreeView *view, int default_sort_col);

	/* Like gtk_tree_view_scroll_to_point(), but does smooth scroll. */
	void tree_view_smooth_scroll_to_point (GtkTreeView *view, gint x, gint y);

	/* Converts stuff to GValues */
	GValue floatToGValue (float num);
};

extern "C" {
	int ygutils_getCharsWidth (GtkWidget *widget, int chars_nb);
	int ygutils_getCharsHeight (GtkWidget *widget, int chars_nb);
	void ygutils_setWidgetFont (GtkWidget *widget, PangoWeight weight, double scale);

	/* Convert html to xhtml (or at least try) */
	gchar *ygutils_convert_to_xhmlt_and_subst (const char *instr, const char *product);
};

#endif // YGUTILS_H

