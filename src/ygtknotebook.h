/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Makes GtkNotebook's top-corner available for a widget.
   Also makes pages, whose tab labels are insentitive, unaccesible.
*/

#ifndef YGTK_NOTEBOOK_H
#define YGTK_NOTEBOOK_H

#include <gtk/gtknotebook.h>
G_BEGIN_DECLS

#define YGTK_TYPE_NOTEBOOK (ygtk_notebook_get_type ())
#define YGTK_NOTEBOOK(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), YGTK_TYPE_NOTEBOOK, YGtkNotebook))
#define YGTK_NOTEBOOK_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), YGTK_TYPE_NOTEBOOK, YGtkNotebookClass))
#define YGTK_IS_NOTEBOOK(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj),  YGTK_TYPE_NOTEBOOK))
#define YGTK_IS_NOTEBOOK_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), YGTK_TYPE_NOTEBOOK))
#define YGTK_NOTEBOOK_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), YGTK_TYPE_NOTEBOOK, YGtkNotebookClass))

typedef struct _YGtkNotebook
{
	GtkNotebook parent;
	// private:
	GtkWidget *corner_widget;
} YGtkNotebook;

typedef struct _YGtkNotebookClass
{
	GtkNotebookClass parent_class;
} YGtkNotebookClass;

GtkWidget* ygtk_notebook_new (void);
GType ygtk_notebook_get_type (void) G_GNUC_CONST;

void ygtk_notebook_set_corner_widget (YGtkNotebook *notebook, GtkWidget *child);

G_END_DECLS
#endif /*YGTK_NOTEBOOK_H*/

