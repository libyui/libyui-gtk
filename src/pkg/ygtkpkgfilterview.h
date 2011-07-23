/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Several user interfaces for zypp query attributes.

   Create a List-model and pass it to FilterView in order to
   use it. Example:
       QueryWidget *query_widget = new YGtkPkgFilterView (new YGtkPkgStatusModel());
       GtkWidget *gtk_widget = query_widget->getWidget();

   One can very easily re-implement YGtkPkgFilterView to use the GtkComboBox.
*/

#ifndef YGTK_PKG_FILTER_VIEW_H
#define YGTK_PKG_FILTER_VIEW_H

#include "ygtkpkgquerywidget.h"
#include <gtk/gtk.h>

struct YGtkPkgFilterModel  // abstract
{
	enum Column { ICON_COLUMN, TEXT_COLUMN, COUNT_NUMBER_COLUMN,
		VISIBLE_COLUMN, ENABLED_COLUMN, WEIGHT_COLUMN, DATA_COLUMN, TOTAL_COLUMNS };

	YGtkPkgFilterModel();
	virtual ~YGtkPkgFilterModel();
	GtkTreeModel *getModel();

	virtual void updateList (Ypp::List list);
	virtual bool writeQuery (Ypp::PoolQuery &query, GtkTreeIter *iter);
	virtual GtkWidget *createToolbox (GtkTreeIter *iter);

	virtual bool hasIconCol() = 0;
	virtual bool firstRowIsAll() = 0;

	virtual bool begsUpdate() = 0;
	virtual void updateRow (Ypp::List list, int row, gpointer data) = 0;
	virtual bool writeRowQuery (Ypp::PoolQuery &query, int row, gpointer data) = 0;

	virtual GtkWidget *createToolboxRow (int selectedRow) { return NULL; }
	virtual GtkWidget *createInternalToolbox() { return NULL; }
	virtual GtkWidget *createInternalPopup() { return NULL; }

	void addRow (const char *icon, const char *text, bool enabled, gpointer data, bool defaultVisible = true);
	void addSeparator();
	void setRowCount (int row, int count);

	struct Impl;
	Impl *impl;
};

// implementations

struct YGtkPkgStatusModel : public YGtkPkgFilterModel
{
	YGtkPkgStatusModel();
	virtual ~YGtkPkgStatusModel();
	virtual bool hasIconCol() { return false; }
	virtual bool firstRowIsAll();
	virtual bool begsUpdate() { return true; }
	virtual void updateRow (Ypp::List list, int row, gpointer data);
	virtual bool writeRowQuery (Ypp::PoolQuery &query, int row, gpointer data);
	virtual GtkWidget *createToolboxRow (int selectedRow);

	struct Impl;
	Impl *impl;
};

struct YGtkPkgPKGroupModel : public YGtkPkgFilterModel
{
	YGtkPkgPKGroupModel();
	virtual bool hasIconCol() { return true; }
	virtual bool firstRowIsAll() { return true; }
	virtual bool begsUpdate() { return true; }
	virtual void updateRow (Ypp::List list, int row, gpointer data);
	virtual bool writeRowQuery (Ypp::PoolQuery &query, int row, gpointer data);
};

struct YGtkPkgRepositoryModel : public YGtkPkgFilterModel
{
	YGtkPkgRepositoryModel();
	virtual ~YGtkPkgRepositoryModel();
	virtual bool hasIconCol() { return true; }
	virtual bool firstRowIsAll() { return true; }
	virtual bool begsUpdate() { return true; }
	virtual void updateRow (Ypp::List list, int row, gpointer data);
	virtual bool writeRowQuery (Ypp::PoolQuery &query, int row, gpointer data);
	virtual GtkWidget *createToolboxRow (int selectedRow);
	virtual GtkWidget *createInternalToolbox();
	virtual GtkWidget *createInternalPopup();

	struct Impl;
	Impl *impl;
};

struct YGtkPkgSupportModel : public YGtkPkgFilterModel
{
	YGtkPkgSupportModel();
	virtual bool hasIconCol() { return false; }
	virtual bool firstRowIsAll() { return true; }
	virtual bool begsUpdate() { return true; }
	virtual void updateRow (Ypp::List list, int row, gpointer data);
	virtual bool writeRowQuery (Ypp::PoolQuery &query, int row, gpointer data);
};

struct YGtkPkgPriorityModel : public YGtkPkgFilterModel
{
	YGtkPkgPriorityModel();
	virtual bool hasIconCol() { return false; }
	virtual bool firstRowIsAll() { return true; }
	virtual bool begsUpdate() { return true; }
	virtual void updateRow (Ypp::List list, int row, gpointer data);
	virtual bool writeRowQuery (Ypp::PoolQuery &query, int row, gpointer data);
};

// widget

struct YGtkPkgFilterView : public YGtkPkgQueryWidget
{
	YGtkPkgFilterView (YGtkPkgFilterModel *model);
	virtual ~YGtkPkgFilterView();
	virtual GtkWidget *getWidget();

	virtual bool begsUpdate();
	virtual void updateList (Ypp::List list);

	virtual void clearSelection();
	virtual bool writeQuery (Ypp::PoolQuery &query);

	virtual GtkWidget *createToolbox();

	void select (int row);

	struct Impl;
	Impl *impl;
};

#endif

