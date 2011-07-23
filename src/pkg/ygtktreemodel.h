/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Wraps GtkTreeModel as a C++ model.
*/

#ifndef YGTK_TREE_MODEL_H
#define YGTK_TREE_MODEL_H

#include <gtk/gtk.h>

struct YGtkTreeModel
{
	virtual void getValue (int row, int col, GValue *value) = 0;
	virtual int rowsNb() = 0;
	virtual int columnsNb() const = 0;
	virtual GType columnType (int col) const = 0;
	// if 'showEmptyEntry' will call getValue(row=-1) for the empty entry
	virtual bool showEmptyEntry() const = 0;

	struct Listener {
		virtual void rowChanged (int row) = 0;
		virtual void rowInserted (int row) = 0;
		virtual void rowDeleted (int row) = 0;
	};
	Listener *listener;

	virtual ~YGtkTreeModel() {}
};

GtkTreeModel *ygtk_tree_model_new (YGtkTreeModel *model);

YGtkTreeModel *ygtk_tree_model_get_model (GtkTreeModel *model);

#endif /*YGTK_TREE_MODEL_H*/

