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

	YGSelectionModel (bool ordinaryModel, bool isTree);
	virtual ~YGSelectionModel();

	GtkTreeModel *getModel();
	void createModel (const vector <GType> &types);

	void doAddItem (YItem *item);
	void doDeleteAllItems();

	virtual void focusItem (GtkTreeIter *iter) = 0;
	virtual void unfocusAllItems() = 0;

	YItem *getItem (GtkTreeIter *iter);
	bool getIter (YItem *item, GtkTreeIter *iter);

	void addRow (GtkTreeIter *iter, YItem *item);
	void setCellLabel (GtkTreeIter *iter, int col, const string &label);
	void setCellIcon (GtkTreeIter *iter, int col, const string &icon);
	void setCellToggle (GtkTreeIter *iter, int col, bool selected);

	int getPtrCol();

protected:
	void implFocusItem (YItem *item);
private:
	GtkTreeModel *m_model;
	bool isTree;
	GtkListStore *getListStore();
	GtkTreeStore *getTreeStore();
	bool isEmpty();
};

#define YGSELECTION_WIDGET_IMPL_ADD(ParentClass)      \
	virtual void addItem(YItem *item) {               \
		doAddItem (item);                             \
		ParentClass::addItem (item);                  \
	}

#define YGSELECTION_WIDGET_IMPL_CLEAR(ParentClass)    \
	virtual void deleteAllItems() {                   \
		doDeleteAllItems();                           \
		ParentClass::deleteAllItems();                \
	}

#define YGSELECTION_WIDGET_IMPL_SELECT(ParentClass)   \
	virtual void selectItem (YItem *item) {           \
		implFocusItem (item);                         \
		ParentClass::selectItem (item);               \
	}                                                 \
	virtual void deselectAllItems() {                 \
		unfocusAllItems();                            \
		ParentClass::deselectAllItems();              \
	}

#define YGSELECTION_WIDGET_IMPL_ALL(ParentClass)      \
	YGSELECTION_WIDGET_IMPL_ADD(ParentClass)          \
	YGSELECTION_WIDGET_IMPL_CLEAR(ParentClass)        \
	YGSELECTION_WIDGET_IMPL_SELECT(ParentClass)

#endif /*YGSELECTION_MODEL_H*/

