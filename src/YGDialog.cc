#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YDialog.h"
#include "YGWidget.h"

class YGGenericButton;
class YGWizard;

class YGDialog : public YDialog, public YGWidget
{
	GtkAllocation m_oldSize;
public:
    YGDialog( const YWidgetOpt &opt,
	      YGWidget         *parent	= 0);
    virtual ~YGDialog();

    // YContainerWidget
//    virtual void addChild( YWidget *child ) IMPL;
    virtual void childAdded( YWidget *child ) IMPL;
    virtual void childRemoved( YWidget *child ) IMPL;

    // YWidget
    virtual void childDeleted( YWidget *child ) IMPL;
    virtual long nicesize( YUIDimension dim );
    virtual bool stretchable( YUIDimension dimension ) const IMPL_RET(true);
    virtual void setSize( long newwidth, long newheight );
    YGWIDGET_IMPL_SET_ENABLING
    virtual bool setKeyboardFocus() IMPL_RET(false);
    virtual void startMultipleChanges() IMPL;
    virtual void doneMultipleChanges() IMPL;
    virtual void saveUserInput( YMacroRecorder *macroRecorder ) IMPL;
};

static void
ygdialog_size_allocate (GtkWidget *widget,
						GtkAllocation *allocation,
						YGDialog  *pThis)
{
	pThis->setSize (allocation->width, allocation->height); 
}

YGDialog::YGDialog( const YWidgetOpt &opt,
					YGWidget         *parent )
	: YDialog( opt ),
	  YGWidget( this, parent, FALSE,
				GTK_TYPE_WINDOW, "type", GTK_WINDOW_TOPLEVEL,
				NULL )
{
	m_oldSize.width = -1;
	m_oldSize.height = -1;

	GtkWidget *fixed = gtk_fixed_new();
	gtk_widget_show (fixed);
	gtk_container_add (GTK_CONTAINER (getWidget()), fixed);

	fprintf (stderr, "%s (%s)\n", G_STRLOC, G_STRFUNC);
	gtk_window_set_modal (GTK_WINDOW (m_widget),
			      !opt.hasDefaultSize.value());
	gtk_window_set_title (GTK_WINDOW (m_widget),
			      (!opt.hasDefaultSize.value()) ?  "Yast2" : "");
	gtk_window_set_default_size (GTK_WINDOW (m_widget), 250, 250);

	g_signal_connect (G_OBJECT (m_widget), "size_allocate",
					  G_CALLBACK (ygdialog_size_allocate), this);

	//	if ( opt.hasWarnColor().value() || opt.hasInfoColor().value() )
	//fprintf (stderr, "Ignored Warn / Info colors\n");
}

YGDialog::~YGDialog()
{
}

long
YGDialog::nicesize( YUIDimension dim )
{
	LOC; // FIXME - hack.
	// The GtkFixed doesn't give a sensible answer until items
	// are positioned, they are however not at this stage
	return YDialog::nicesize (dim);
}

void YGDialog::setSize( long newWidth, long newHeight )
{
	int defw, defh;

	gtk_window_get_default_size (GTK_WINDOW (m_widget), &defw, &defh);
	fprintf (stderr, "%s:%s %ld, %ld (%d, %d) def %d, %d\n", G_STRLOC, G_STRFUNC,
		 newWidth, newHeight, m_oldSize.width, m_oldSize.height, defw, defh);

	m_oldSize.width = newWidth;
	m_oldSize.height = newHeight;

	// FIXME - was resize ...
	gtk_window_set_default_size (GTK_WINDOW (m_widget), newWidth, newHeight);
	if (numChildren() > 0)
		YDialog::setSize (newWidth, newHeight);
}

YDialog *
YGUI::createDialog( YWidgetOpt & opt )
{
	IMPL;
	return new YGDialog (opt, NULL);
}

void
YGUI::showDialog( YDialog *dialog )
{
	IMPL;
	gtk_widget_show (YGWidget::get (dialog)->getWidget());
	dumpWidgetTree (YGWidget::get (dialog)->getWidget());
	dumpYastTree (dialog);
}

void
YGUI::closeDialog( YDialog *dialog )
{
	IMPL; // FIXME - destroy ? lifecycle etc. ...
	gtk_widget_hide (YGWidget::get (dialog)->getWidget());
}
