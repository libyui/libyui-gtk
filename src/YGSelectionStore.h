/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Provides a basic model common to YSelectedWidget widgets. */

#ifndef YGSELECTION_STORE_H
#define YGSELECTION_STORE_H

#include <gtk/gtk.h>
struct YItem;
struct YSelectionWidget;

struct YGSelectionStore
{
	YGSelectionStore (bool tree);
	virtual ~YGSelectionStore();

	GtkTreeModel *getModel()  { return m_model; }
	void createStore (int cols, const GType types[]);

	void addRow (YItem *item, GtkTreeIter *iter, GtkTreeIter *parent = 0);
	void setRowText (GtkTreeIter *iter, int iconCol, const std::string &icon,
		int labelCol, const std::string &label, const YSelectionWidget *widget);
	void setRowMark (GtkTreeIter *iter, int markCol, bool mark);
	void doDeleteAllItems();

	YItem *getYItem (GtkTreeIter *iter);
	void getTreeIter (const YItem *item, GtkTreeIter *iter);

	GtkListStore *getListStore();
	GtkTreeStore *getTreeStore();

	bool isEmpty();
	int getTreeDepth();

	bool findLabel (int labelCol, const std::string &label, GtkTreeIter *iter);

protected:
	GtkTreeModel *m_model;
	bool isTree;
	gpointer m_nextRowId;
};

#define YGSELECTION_WIDGET_IMPL(ParentClass)             \
	virtual void addItem(YItem *item) {                  \
		ParentClass::addItem (item);                     \
		doAddItem (item);                                \
	}                                                    \
	virtual void deleteAllItems() {                      \
		ParentClass::deleteAllItems();                   \
		blockSelected();                                 \
		doDeleteAllItems();                              \
	}                                                    \
	virtual void selectItem (YItem *item, bool select) { \
		ParentClass::selectItem (item, select);          \
		doSelectItem (item, select);                     \
	}                                                    \
	virtual void deselectAllItems() {                    \
		ParentClass::deselectAllItems();                 \
		doDeselectAllItems();                                 \
	}

#endif /*YGSELECTION_STORE_H*/

