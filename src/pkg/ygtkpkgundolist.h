/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* A Ypp::List that stores only 
*/

#ifndef YGTK_PKG_UNDO_LIST_H
#define YGTK_PKG_UNDO_LIST_H

#include "yzyppwrapper.h"

struct YGtkPkgUndoList
{
	YGtkPkgUndoList();
	~YGtkPkgUndoList();

	Ypp::Selectable *front (int *autoCount);
	Ypp::List getList();

	struct Listener {
		virtual void undoChanged (YGtkPkgUndoList *list) = 0;
	};

	void addListener (Listener *listener);
	void removeListener (Listener *listener);

	bool popupDialog (bool onApply);

	struct Impl;
	Impl *impl;
};

#endif

