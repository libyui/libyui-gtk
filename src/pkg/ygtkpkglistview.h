/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* A view for Ypp's query results.
*/

#ifndef YGTK_PKG_LIST_VIEW_H
#define YGTK_PKG_LIST_VIEW_H

#include "yzyppwrapper.h"
#include <gtk/gtk.h>

enum Property {
	INSTALLED_CHECK_PROP, NAME_PROP, ACTION_NAME_PROP, NAME_SUMMARY_PROP,
	VERSION_PROP, SINGLE_VERSION_PROP, REPOSITORY_PROP, SUPPORT_PROP,
	SIZE_PROP, STATUS_ICON_PROP, ACTION_BUTTON_PROP, UNDO_BUTTON_PROP, TOTAL_PROPS
};

struct YGtkPkgListView {
	YGtkPkgListView (bool descriptiveTooltip, int default_sort /* -1 to disable */,
		bool indentAuto, bool colorModified, bool variableHeight = false);
	~YGtkPkgListView();

	GtkWidget *getWidget();
	GtkWidget *getView();

	void setQuery (Ypp::Query &query);
	void setList (Ypp::List list);

	void setHighlight (const std::list <std::string> &keywords);

	void addTextColumn (const char *header, int property, bool visible, int size);
	void addCheckColumn (int checkProperty);
	void addImageColumn (const char *header, int property, bool onlyManualModified = false);
	void addButtonColumn (const char *header, int property);

	Ypp::List getList();

	struct Listener {
		virtual void selectionChanged() = 0;
	};
	void setListener (Listener *listener);

	Ypp::List getSelected();
	void selectAll();

	struct Impl;
	Impl *impl;
};

// some utilities:

const char *getStatusAction (Ypp::Selectable *sel);
std::string getStatusSummary (Ypp::Selectable &sel);
const char *getStatusStockIcon (Ypp::Selectable &sel);

std::string getRepositoryLabel (Ypp::Repository &repo);
const char *getRepositoryStockIcon (Ypp::Repository &repo);
const char *getRepositoryStockIcon (const std::string &url);

void highlightMarkup (std::string &text, const std::list <std::string> &keywords,
                      const char *openTag, const char *closeTag, int openTagLen, int closeTagLen);

void highlightMarkupSpan (std::string &text, const std::list <std::string> &keywords);

#endif

