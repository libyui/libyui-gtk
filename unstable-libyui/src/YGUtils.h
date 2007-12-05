/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#ifndef YGUTILS_H
#define YGUTILS_H

#include <string>
#include <list>
#include <gtk/gtktextview.h>
#include <gtk/gtkentry.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreemodel.h>
#include <gtk/gtkcellrenderertoggle.h>

// TODO: do a cleanup here. We should probably split string, gtk and stuff
// Some GtkTreeView should probably go to their own files
// Let's avoid GTK+ stuff, better to replicate that, if needed, than leave in
// this general purpose utils.

/* YGUtils.h/cc have some functionality that is shared between different parts
   of the code. */

namespace YGUtils
{
	/* Replaces Yast's '&' accelerator by Gnome's '_' (and proper escaping). */
	std::string mapKBAccel (const std::string &src);

	/* Adds filter support to a GtkEntry. */
	void setFilter (GtkEntry *entry, const std::string &validChars);

	/* Replaces every 'mouth' by 'food' in 'str'. */
	void replace (std::string &str, const char *mouth, int mouth_len, const char *food);

	/* Escapes markup text (eg. changes '<' by '\<'). */
	void escapeMarkup (std::string &str);

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
	bool contains (const std::string &haystack, const std::string &needle);

	/* Splits a string into parts as separated by the separator characters.
	   eg: splitString ("Office/Writer", '/') => { "Office", "Writer" } */
	std::list <std::string> splitString (const std::string &str, char separator);

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

	void setStockIcon (GtkWidget *button, std::string ycp_str);

	GdkPixbuf *loadPixbuf (const std::string &fileneme);
};

extern "C" {
	int ygutils_getCharsWidth (GtkWidget *widget, int chars_nb);
	int ygutils_getCharsHeight (GtkWidget *widget, int chars_nb);
	void ygutils_setWidgetFont (GtkWidget *widget, PangoWeight weight, double scale);

	void ygutils_setFilter (GtkEntry *entry, const char *validChars);

	/* Convert html to xhtml (or at least try) */
	gchar *ygutils_convert_to_xhmlt_and_subst (const char *instr, const char *product);
	void ygutils_setStockIcon (GtkWidget *button, const char *ycp_str);
};

#endif // YGUTILS_H

