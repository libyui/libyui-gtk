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

	void doAddItem (YItem *item);
	void doDeleteAllItems();

	virtual YItem *focusItem() = 0;
	virtual void setFocusItem (GtkTreeIter *iter) = 0;  // NULL to unset

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

	int getMaxDepth (int *rows);  // not cached

protected:
	void implFocusItem (YItem *item);
private:
	GtkTreeModel *m_model;
	bool isTree;
	GtkListStore *getListStore();
	GtkTreeStore *getTreeStore();
	bool isEmpty();
	YSelectionWidget *ywidget;  // we use it, to get the path for icons
};

#define YGSELECTION_WIDGET_IMPL_ADD(ParentClass)      \
	virtual void addItem(YItem *item) {               \
		ParentClass::addItem (item);                  \
		doAddItem (item);                             \
	}

#define YGSELECTION_WIDGET_IMPL_CLEAR(ParentClass)    \
	virtual void deleteAllItems() {                   \
		ParentClass::deleteAllItems();                \
		doDeleteAllItems();                           \
	}

#define YGSELECTION_WIDGET_IMPL_SELECT(ParentClass)   \
	virtual void selectItem (YItem *item, bool select) { \
		ParentClass::selectItem (item, select);       \
		if (select)                                   \
			implFocusItem (item);                     \
	}                                                 \
	virtual void deselectAllItems() {                 \
		ParentClass::deselectAllItems();              \
		setFocusItem (NULL);                          \
	}                                                 \
	virtual YItem *selectedItem() {                   \
		return focusItem();                           \
	}                                                 \

#define YGSELECTION_WIDGET_IMPL_ALL(ParentClass)      \
	YGSELECTION_WIDGET_IMPL_ADD(ParentClass)          \
	YGSELECTION_WIDGET_IMPL_CLEAR(ParentClass)        \
	YGSELECTION_WIDGET_IMPL_SELECT(ParentClass)

#endif /*YGSELECTION_MODEL_H*/

