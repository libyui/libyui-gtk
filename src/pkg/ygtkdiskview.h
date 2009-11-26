/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* Show partitions.
*/

#ifndef YGTK_DISK_VIEW_H
#define YGTK_DISK_VIEW_H

#include "yzyppwrapper.h"
#include <gtk/gtk.h>

class DiskView : public Ypp::Disk::Listener
{
GdkPixbuf *m_diskPixbuf, *m_diskFullPixbuf;
bool m_hasWarn;
GtkWidget *m_button, *m_view;

public:
	GtkWidget *getWidget();

	DiskView ();
	~DiskView();

private:
	virtual void updateDisk();
	void popupWarning();
};

#endif /*YGTK_DISK_VIEW_H*/

