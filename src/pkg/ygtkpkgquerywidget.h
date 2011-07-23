/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Abstract for widgets that support query.
*/

#ifndef YGTK_PKG_QUERY_WIDGET_H
#define YGTK_PKG_QUERY_WIDGET_H

#include <gtk/gtk.h>
#include "yzyppwrapper.h"

struct YGtkPkgQueryWidget
{
	virtual ~YGtkPkgQueryWidget() {}
	virtual GtkWidget *getWidget() = 0;

	// if begsUpdate is true, the current List without this filter applied
	// will be passed to updateList() so you can e.g. avoid presenting
	// entries for which there is no match.
	virtual bool begsUpdate() = 0;
	virtual void updateList (Ypp::List list) = 0;

	virtual void clearSelection() = 0;
	virtual bool writeQuery (Ypp::PoolQuery &query) = 0;

	virtual GtkWidget *createToolbox() { return NULL; }

	struct Listener {
		virtual void refreshQuery() = 0;
	};

	virtual void setListener (Listener *listener) { this->listener = listener; }
	Listener *listener;

	bool modified;  // flag for internal use

	//protected:
		void notify() { if (listener) listener->refreshQuery(); }

		void notifyDelay (int delay)
		{
			static guint timeout_id = 0;
			struct inner {
				static gboolean timeout_cb (gpointer data)
				{
					YGtkPkgQueryWidget *pThis = (YGtkPkgQueryWidget *) data;
					timeout_id = 0;
					pThis->notify();
					return FALSE;
				}
			};
			if (timeout_id) g_source_remove (timeout_id);
			timeout_id = g_timeout_add_full (
				G_PRIORITY_LOW, delay, inner::timeout_cb, this, NULL);
		}

protected:
	YGtkPkgQueryWidget() : listener (NULL) {}
};

#endif

