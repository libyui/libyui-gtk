//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

/* YGtkBarGraph is just a simple graph of bars that may be
   changed during run-time (see ygtk_bar_graph_setup_entry()).
*/

#ifndef YGTK_BAR_GRAPH_H
#define YGTK_BAR_GRAPH_H

#include "ygtkratiobox.h"
#include <gtk/gtktooltips.h>

G_BEGIN_DECLS

#define YGTK_TYPE_BAR_GRAPH            (ygtk_bar_graph_get_type ())
#define YGTK_BAR_GRAPH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                        YGTK_TYPE_BAR_GRAPH, YGtkBarGraph))
#define YGTK_BAR_GRAPH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                        YGTK_TYPE_BAR_GRAPH, YGtkBarGraphClass))
#define IS_YGTK_BAR_GRAPH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                        YGTK_TYPE_BAR_GRAPH))
#define IS_YGTK_BAR_GRAPH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                        YGTK_TYPE_BAR_GRAPH))
#define YGTK_BAR_GRAPH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                        YGTK_TYPE_BAR_GRAPH, YGtkBarGraphClass))

typedef struct _YGtkBarGraph	     YGtkBarGraph;
typedef struct _YGtkBarGraphClass  YGtkBarGraphClass;

struct _YGtkBarGraph
{
	YGtkRatioHBox ratio_box;

	// private:
	GtkTooltips *m_tooltips;
};

struct _YGtkBarGraphClass
{
	YGtkRatioBoxClass parent_class;
};

GtkWidget *ygtk_bar_graph_new (void);
GType ygtk_bar_graph_get_type (void) G_GNUC_CONST;

void ygtk_bar_graph_create_entries (YGtkBarGraph *bar, guint entries);
void ygtk_bar_graph_setup_entry (YGtkBarGraph *bar, int index,
                                 const gchar *label_entry, int value);

G_END_DECLS

#endif /*YGTK_BAR_GRAPH_H*/
