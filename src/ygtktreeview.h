/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkTreeView hacks support for a right-click signal.
*/

#ifndef YGTK_TREE_VIEW_H
#define YGTK_TREE_VIEW_H

#include <gtk/gtk.h>
G_BEGIN_DECLS

#define YGTK_TYPE_TREE_VIEW            (ygtk_tree_view_get_type ())
#define YGTK_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	YGTK_TYPE_TREE_VIEW, YGtkTreeView))
#define YGTK_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
	YGTK_TYPE_TREE_VIEW, YGtkTreeViewClass))
#define YGTK_IS_TREE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                              YGTK_TYPE_TREE_VIEW))
#define YGTK_IS_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                              YGTK_TYPE_TREE_VIEW))
#define YGTK_TREE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
	YGTK_TYPE_TREE_VIEW, YGtkTreeViewClass))

typedef struct _YGtkTreeView
{
	GtkTreeView parent;

	// members:
	gchar *empty_text;
} YGtkTreeView;

typedef struct _YGtkTreeViewClass
{
	GtkTreeViewClass parent_class;

	// signals:
	void (*right_click) (YGtkTreeView *view, gboolean outreach);
} YGtkTreeViewClass;

GtkWidget* ygtk_tree_view_new (const gchar *empty_text);  /* null to disable */
GType ygtk_tree_view_get_type (void) G_GNUC_CONST;

void ygtk_tree_view_set_empty_text (YGtkTreeView *view, const gchar *empty_text);

void ygtk_tree_view_popup_menu (YGtkTreeView *view, GtkWidget *menu);

GtkWidget *ygtk_tree_view_create_show_columns_menu (YGtkTreeView *view);
GtkWidget *ygtk_tree_view_append_show_columns_item (YGtkTreeView *view, GtkWidget *menu);

// use this method of adding columns to overload gtk Arabic tree-view layout by
// a more sane and simpler one.
// (if you do, use it for all your column adding needs.)
void ygtk_tree_view_append_column (YGtkTreeView *view, GtkTreeViewColumn *column);

// use this if you have used our ygtk_tree_view_append_column()
GtkTreeViewColumn *ygtk_tree_view_get_column (YGtkTreeView *view, gint nb);

G_END_DECLS
#endif /*YGTK_TREE_VIEW_H*/

