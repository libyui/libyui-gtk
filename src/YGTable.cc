#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YTable.h"
#include "YGWidget.h"

class YGTable : public YTable, public YGWidget
{
public:
	YGTable (const YWidgetOpt &opt,
	         YGWidget *parent,
	         vector <string> headers)
	: YTable (opt, headers.size())
	, YGWidget (this, parent, true, GTK_TYPE_TREE_VIEW, NULL)
	{
#if 0
		/* Sets a model. */
		int c;
		GType types[numCols()+1];
		for (c = 0; c < numCols(); c++)
			types[c] = G_TYPE_STRING;
		types[c] = -1;
		GtkListStore *list = gtk_list_store_new (numCols(), types);

		gtk_tree_view_set_model(GTK_TREE_VIEW(getWidget()), GTK_TREE_MODEL(list));

		/* Creates the columns. */
		GtkTreeViewColumn *column;
		GtkCellRenderer   *renderer;
		for(int i = 0; i < numCols(); i++)
		{
			renderer = gtk_cell_renderer_text_new ();
			column = gtk_tree_view_column_new_with_attributes (
		             headers[i].c_str(), renderer, "text", i, NULL);
			gtk_tree_view_insert_column (GTK_TREE_VIEW(getWidget()), column, i);
		}
#endif
	}

	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_KEYBOARD_FOCUS

protected:
    /**
     * Is called, when an item ( a row ) has been added. Overload this to
     * fill the ui specific widget with items.
     * @param elements the strings of the elements, one for each column.
     * @param index index of the new item.
     */
	virtual void itemAdded (vector<string> elements, int index)
	{
		IMPL;
#if 0
		GtkTreeModel* model = gtk_tree_view_get_model (GTK_TREE_VIEW(getWidget()));
		GtkTreeIter  iter;

		// Add the space for the rows
printf("index: %d\n", index);
		for (int r = numItems(); r <= index; r++)
//			for (int c = 0; c < numCols(); c++)
			{
printf("adding row: %d\n", r);
				gtk_list_store_append (GTK_LIST_STORE (model), &iter);
			}

		// Edit the rows
		GtkListStore *list;
		GType types[numCols()];
		for (int c = 0; c < numCols(); c++)
			types[c] = G_TYPE_STRING;
		list = gtk_list_store_new (numCols(), types);

		gtk_list_store_append(list, &iter);
		for (int c = 0; c < numCols(); c++)
{
printf("adding item: %s\n", elements[c].c_str());
			gtk_list_store_set_value (list, &iter, c, (GValue*)elements[c].c_str());
}

	// reference: if this is slow, checkout:
	// http://scentric.net/tutorial/sec-treemodel-add-rows.html#sec-treestore-adding-many-rows
#endif
	}

    /**
     * Is called, when all items have been cleared. Overload this
     * and clear the ui specific table.
     */
	virtual void itemsCleared()
	{
		IMPL;
		for (int c = 0; c < numCols(); c++)
			gtk_tree_view_remove_column (GTK_TREE_VIEW(getWidget()),
			    gtk_tree_view_get_column (GTK_TREE_VIEW(getWidget()), c));
	}

    /**
     * Is called, when the contents of a cell has been changed. Overload
     * this and change the cell text.
     */
	virtual void cellChanged (int index, int colnum, const YCPString & newtext)
	{
		IMPL;
	}

    /**
     * Returns the index of the currently
     * selected item or -1 if no item is selected.
     */
	virtual int getCurrentItem()
	{
		IMPL;
		return -1;
	}

    /**
     * Makes another item the selected one.
     */
	virtual void setCurrentItem (int index)
	{
		IMPL;
	}
};

YWidget *
YGUI::createTable (YWidget *parent, YWidgetOpt & opt,
	                 vector<string> header)
{
	return new YGTable (opt, YGWidget::get (parent), header);
}
