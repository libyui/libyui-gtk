/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#ifndef YGUTILS_H
#define YGUTILS_H

#include <string>
#include <list>
#include <gtk/gtk.h>

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

	/* Truncates the text with "..." past the given length.
	   pos: -1 = start, 0 = middle, 1 = end. Honors utf-8 characters. */
	std::string truncate (const std::string &str, int length, int pos);

	/* Escapes markup text (eg. changes '<' by '\<'). */
	std::string escapeMarkup (const std::string &str);

	/* Check if 'str' ends with 'suffix'. */
	bool endsWith (const std::string &str, const std::string &suffix);

	/* Adds functionality to scroll widgets to top or bottom. */
	void scrollWidget (GtkAdjustment *vadj, bool top);

	/* Returns the average width of the given number of characters in pixels. */
	int getCharsWidth (GtkWidget *widget, int chars_nb);
	int getCharsHeight (GtkWidget *widget, int chars_nb);

	/* Sets some widget font proprities. */
	void setWidgetFont (GtkWidget *widget, PangoStyle style, PangoWeight weight, double scale);

	/* Instead of setting GtkPaned::position in pixels, do so in percents. */
	void setPaneRelPosition (GtkWidget *paned, gdouble rel);

  /*
   *
   * Load a pixbuf. This tries several locations:
   *
   * - The given pathname
   * - The icon theme from the current desktop
   * - The default icon if failing others
   *
   * If the icon does not have a filename extension it is retrieved by name from theme.
   * Returns NULL if failed.
   *
   * Pixbuf need to to g_object_unref it to be removed.
   */
	GdkPixbuf *loadPixbuf (const std::string &fileneme);

	/* Shifts colors in a GdkPixbuf. */
	GdkPixbuf *setOpacity (const GdkPixbuf *src, int opacity, bool touchAlpha);

	/* Gray out some pixbuf. */
	GdkPixbuf *setGray (const GdkPixbuf *src);

	/* Tries to make sense out of the string, applying some stock icon to the button. */
	const char *setStockIcon (GtkWidget *button, const std::string &label,
	                          const char *fallbackIcon);
	const char *mapIconname(const std::string &label);


	/* For empty model rows, render a separator (can be used for GtkTreeView and GtkComboBox */
	gboolean empty_row_is_separator_cb (
		GtkTreeModel *model, GtkTreeIter *iter, gpointer text_col);

	/* Shrink widget inner-border (thickness) */
	void shrinkWidget (GtkWidget *widget);
};

extern "C" {
	char *ygutils_mapKBAccel (const char *src);

	void ygutils_setWidgetFont (GtkWidget *widget, PangoStyle style, PangoWeight weight, double scale);
	void ygutils_setPaneRelPosition (GtkWidget *paned, gdouble rel);

	void ygutils_setFilter (GtkEntry *entry, const char *validChars);

	void ygutils_scrollAdj (GtkAdjustment *vadj, gboolean top);

	const char *ygutils_mapIconname (const std::string &label);
	const char *ygutils_setStockIcon (GtkWidget *button, const char *label,
	                                  const char *fallbackIcon);

	GdkPixbuf *ygutils_setOpacity (const GdkPixbuf *src, int opacity, gboolean useAlpha);

	gchar *ygutils_headerize_help (const char *help_text, gboolean *cut);

	// convert liberal html to xhtml
	gchar *ygutils_convert_to_xhtml (const char *instr);
};

#endif // YGUTILS_H

