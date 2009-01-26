/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Builds a model for the given YItems to be used by YSelectedWidgets. */

#ifndef YGSELECTION_MODEL_H
#define YGSELECTION_MODEL_H

#include <YItem.h>
#include <YSelectionWidget.h>
#include <gtk/gtktreemodel.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtktreestore.h>

struct YGSelectionModel
{
	// ordinary model columns
	enum ColumnIndex {
		LABEL_COLUMN, ICON_COLUMN
	};

	YGSelectionModel (YSelectionWidget *ywidget, bool ordinaryModel, bool isTree);
	virtual ~YGSelectionModel();

	GtkTreeModel *getModel();
	void createModel (const vector <GType> &types);

	virtual void doAddItem (YItem *item);
	virtual void doDeleteAllItems();
	virtual YItem *doSelectedItem() = 0;
	virtual void doSelectItem (GtkTreeIter *iter) = 0;
	virtual void doUnselectAll() = 0;

	YItemConstIterator itemsBegin() { return ywidget->itemsBegin(); }
	YItemConstIterator itemsEnd()   { return ywidget->itemsEnd(); }

	// to be implemented by trees
	virtual void expand (GtkTreeIter *iter) {}

	YItem *getItem (GtkTreeIter *iter);
	bool getIter (const YItem *item, GtkTreeIter *iter);
    bool findByText (const string &text, GtkTreeIter *iter);

	void addRow (GtkTreeIter *iter, YItem *item, bool honor_select);
	void setCellLabel (GtkTreeIter *iter, int col, const string &label);
	void setCellIcon (GtkTreeIter *iter, int col, const string &icon);
	void setCellToggle (GtkTreeIter *iter, int col, bool selected);

	int getPtrCol();
	int getIdCol();

	int getMaxDepth (int *rows);  // not cached

protected:
	void implSelectItem (YItem *item);
private:
	GtkTreeModel *m_model;
	bool isTree;
	GtkListStore *getListStore();
	GtkTreeStore *getTreeStore();
	bool isEmpty();
	YSelectionWidget *ywidget;  // we use it, to get the path for icons
	gpointer dataIndex;
};

#define YGSELECTION_WIDGET_IMPL(ParentClass)             \
	virtual void addItem(YItem *item) {                  \
		ParentClass::addItem (item);                     \
		doAddItem (item);                                \
	}                                                    \
	virtual void deleteAllItems() {                      \
		ParentClass::deleteAllItems();                   \
		doDeleteAllItems();                              \
	}                                                    \
	virtual void selectItem (YItem *item, bool select) { \
		ParentClass::selectItem (item, select);          \
		if (select)                                      \
			implSelectItem (item);                       \
	}                                                    \
	virtual void deselectAllItems() {                    \
		ParentClass::deselectAllItems();                 \
		doUnselectAll();                                 \
	}                                                    \
	virtual YItem *selectedItem() {                      \
		return doSelectedItem();                         \
	}

#endif /*YGSELECTION_MODEL_H*/

