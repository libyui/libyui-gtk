/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Comprises several QueryWidget widgets.
   You probably want to use this in cooperation with YGtkPkgFilerBox.
*/

#ifndef YGTK_PKG_QUERY_COMBO_H
#define YGTK_PKG_QUERY_COMBO_H

#include "ygtkpkgquerywidget.h"
#include <gtk/gtk.h>

struct YGtkPkgQueryCombo : public YGtkPkgQueryWidget
{
	struct Factory {
		virtual YGtkPkgQueryWidget *createQueryWidget (
			YGtkPkgQueryCombo *combo, int index) = 0;
	};

	YGtkPkgQueryCombo (Factory *factory);
	virtual ~YGtkPkgQueryCombo();
	virtual GtkWidget *getWidget();

	void add (const char *title);
	void setActive (int index);

	virtual bool begsUpdate();
	virtual void updateList (Ypp::List list);

	virtual void clearSelection();
	virtual bool writeQuery (Ypp::PoolQuery &query);

	virtual GtkWidget *createToolbox();

	virtual void setListener (Listener *listener);

	struct Impl;
	Impl *impl;
};

#endif

