/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* Textdomain "yast2-gtk" */
/* YGtkPkgLanguageView, language list implementation */
// check the header file for information about this widget

#include "YGi18n.h"
#include "config.h"
#include "ygtkpkglanguageview.h"
#include <gtk/gtk.h>

YGtkPkgLanguageView::YGtkPkgLanguageView()
: YGtkPkgListView (true, Ypp::List::NAME_SORT, false, true)
{
	addCheckColumn (INSTALLED_CHECK_PROP);
	addTextColumn (NULL, NAME_SUMMARY_PROP, true, -1);

	Ypp::LangQuery query;
	YGtkPkgListView::setList (query);

	YGtkPkgListView::setListener (this);
}

GtkWidget *YGtkPkgLanguageView::getWidget()
{ return YGtkPkgListView::getWidget(); }

bool YGtkPkgLanguageView::writeQuery (Ypp::PoolQuery &query)
{
	Ypp::List list (getSelected());
	if (list.size() > 0) {
		Ypp::Collection col (list.get (0));
		query.addCriteria (new Ypp::FromCollectionMatch (col));
		return true;
	}
	return false;
}

void YGtkPkgLanguageView::selectionChanged()
{ notify(); }

