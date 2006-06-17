#ifndef YGUI_H
#define YGUI_H

#include <gtk/gtk.h>
#include <YSimpleEventHandler.h>
#include <YUI.h>

using std::string;
using std::vector;

#define IMPL_DEBUG true /* Print IMPL debug messages? */
#define LOC       fprintf (stderr, "%s (%s)\n", G_STRLOC, G_STRFUNC)
#ifdef IMPL_DEBUG
	#define IMPL      { LOC; }
#else
	#define IMPL      { }
#endif
#define IMPL_NULL { LOC; return NULL; }
#define IMPL_VOID { LOC; return YCPVoid(); }
#define IMPL_RET(a) { LOC; return (a); }

class YGUI: public YUI
{
public:
	YGUI (int argc, char **argv,
	      bool with_threads,
	      const char *macro_file);
	virtual ~YGUI();

	static YGUI *ui() { return (YGUI *)YUI::ui(); }

    // non abstract loop bits:
    virtual void blockEvents( bool block = true ) { LOC; }
    virtual bool eventsBlocked() const { LOC; return false; }
    virtual void internalError( const char *msg ) { fprintf (stderr, "Internal err '%s'\n", msg); LOC; }
//    virtual long deviceUnits( YUIDimension dim, float layout_units );
//    virtual float layoutUnits( YUIDimension dim, long device_units );
    virtual void idleLoop( int fd_ycp );
    virtual YEvent * userInput( unsigned long timeout_millisec );
    virtual YEvent * pollInput();

    virtual void showDialog( YDialog *dialog );
    virtual void closeDialog( YDialog *dialog );
    // Non abstract virtuals:
    virtual YCPValue setLanguage( const YCPTerm & term ) IMPL_VOID;
#if 0
    virtual YCPString glyph( const YCPSymbol & glyphSymbol ) { return YCPString( "" ); }
    virtual YCPValue setConsoleFont( const YCPString & console_magic,
				     const YCPString & font,
				     const YCPString & screen_map,
				     const YCPString & unicode_map,
				     const YCPString & encoding );
    virtual YCPValue setKeyboard();
#endif

    // event pieces:
 private:
    YSimpleEventHandler m_event_handler;
 public:
    void    sendEvent (YEvent *event);
    YEvent *pendingEvent() const { return m_event_handler.pendingEvent(); }
    bool    eventPendingFor (YWidget *widget) const { return m_event_handler.eventPendingFor( widget ); }

    // container widgets
    virtual YDialog *createDialog( YWidgetOpt & opt );
    virtual YContainerWidget *createSplit( YWidget *parent, YWidgetOpt & opt, YUIDimension dimension );
    virtual YContainerWidget *createReplacePoint( YWidget *parent, YWidgetOpt & opt );
    virtual YContainerWidget *createAlignment( YWidget *parent, YWidgetOpt & opt,
					       YAlignmentType halign,
					       YAlignmentType valign );
    virtual YContainerWidget *createSquash( YWidget *parent, YWidgetOpt & opt,
											bool hsquash, bool vsquash );
    virtual YContainerWidget *createRadioButtonGroup( YWidget *parent, YWidgetOpt & opt );
    virtual YContainerWidget *createFrame( YWidget *parent, YWidgetOpt & opt,
										   const YCPString & label );

    // leaf widgets
    virtual YWidget *createEmpty( YWidget *parent, YWidgetOpt & opt );
    virtual YWidget *createSpacing( YWidget *parent, YWidgetOpt & opt, float size,
				    bool horizontal, bool vertical );
    virtual YWidget *createLabel( YWidget *parent, YWidgetOpt & opt,
				    const YCPString & text );
    virtual YWidget *createRichText( YWidget *parent, YWidgetOpt & opt,
					const YCPString & text );
    virtual YWidget *createLogView( YWidget *parent, YWidgetOpt & opt,
									const YCPString & label, int visibleLines,
									int maxLines );
    virtual YWidget *createPushButton( YWidget *parent, YWidgetOpt & opt,
									   const YCPString & label );
    virtual YWidget *createMenuButton( YWidget *parent, YWidgetOpt & opt,
									   const YCPString & label );
    virtual YWidget *createRadioButton( YWidget *parent, YWidgetOpt & opt,
										YRadioButtonGroup *rbg, const YCPString & label,
										bool checked );
    virtual YWidget *createCheckBox( YWidget *parent, YWidgetOpt & opt,
									 const YCPString & label, bool checked );
    virtual YWidget *createTextEntry( YWidget *parent, YWidgetOpt & opt,
									  const YCPString & label, const YCPString & text );
    virtual YWidget *createMultiLineEdit( YWidget *parent, YWidgetOpt & opt,
										  const YCPString & label, const YCPString & text );
    virtual YWidget *createSelectionBox( YWidget *parent, YWidgetOpt & opt,
										 const YCPString & label );
    virtual YWidget *createMultiSelectionBox( YWidget *parent, YWidgetOpt & opt,
											  const YCPString & label );
    virtual YWidget *createComboBox( YWidget *parent, YWidgetOpt & opt,
									 const YCPString & label );
    virtual YWidget *createTree( YWidget *parent, YWidgetOpt & opt,
								 const YCPString & label );
    virtual YWidget *createTable( YWidget *parent, YWidgetOpt & opt,
								  std::vector<std::string> header );
    virtual YWidget *createProgressBar( YWidget *parent, YWidgetOpt & opt,
										const YCPString & label,
										const YCPInteger & maxprogress,
										const YCPInteger & progress );
    virtual YWidget *createImage( YWidget *parent, YWidgetOpt & opt,
								  YCPByteblock imagedata, YCPString defaulttext );
    virtual YWidget *createImage( YWidget *parent, YWidgetOpt & opt,
								  YCPString file_name, YCPString defaulttext );
    virtual YWidget *createIntField( YWidget *parent, YWidgetOpt & opt,
									 const YCPString & label,
									 int minValue, int maxValue, int initialValue );
    virtual YWidget *createPackageSelector( YWidget *parent, YWidgetOpt & opt,
											const YCPString & floppyDevice );
    virtual YWidget *createPkgSpecial( YWidget *parent, YWidgetOpt & opt,
									   const YCPString & subwidget );

    // Optional widgets
    virtual YWidget *createDummySpecialWidget( YWidget *parent, YWidgetOpt & opt ) IMPL_NULL;
    virtual bool     hasDummySpecialWidget() { return false; }

    virtual YWidget *createDownloadProgress( YWidget *parent, YWidgetOpt & opt,
					     const YCPString & label,
					     const YCPString & filename,
					     int expectedSize ) IMPL_NULL;
    virtual bool     hasDownloadProgress() { return false; }

    virtual YWidget *createBarGraph( YWidget *parent, YWidgetOpt & opt ) IMPL_NULL;
    virtual bool     hasBarGraph()  { return false; }

	virtual YWidget *createColoredLabel (YWidget *parent, YWidgetOpt &opt,
	                                     YCPString label,
	                                     YColor foreground, YColor background,
	                                     int margin);
	virtual bool hasColoredLabel() { return true; }

    virtual YWidget *createDate( YWidget *parent,
				 YWidgetOpt & opt, 
				 const YCPString & label,
				 const YCPString & date ) IMPL_NULL;
    virtual bool     hasDate() { return false; } 

    virtual YWidget *createTime( YWidget *parent,
				 YWidgetOpt & opt,   
				 const YCPString & label,
				 const YCPString & time ) IMPL_NULL;
    virtual bool     hasTime() { return false; }

    virtual YWidget *createDumbTab( YWidget *parent, YWidgetOpt & opt ) IMPL_NULL;
    virtual bool     hasDumbTab() { return false; }

    virtual YWidget *createMultiProgressMeter( YWidget *parent, YWidgetOpt & opt,
					       bool horizontal,
					       const YCPList & maxValues ) IMPL_NULL;
    virtual bool     hasMultiProgressMeter() { return false; }

	virtual YWidget *createSlider (YWidget *parent, YWidgetOpt &opt,
	                               const YCPString &label,
	                               int min, int max, int initial);
	virtual bool hasSlider() { return true; }

    virtual YWidget *createPartitionSplitter( YWidget *		parent,
					      YWidgetOpt &	opt,
					      int		usedSize,
					      int		totalFreeSize,
					      int		newPartSize,
					      int		minNewPartSize,
					      int		minFreeSize,
					      const YCPString & usedLabel,
					      const YCPString & freeLabel,
					      const YCPString & newPartLabel,
					      const YCPString & freeFieldLabel,
					      const YCPString & newPartFieldLabel ) IMPL_NULL
    virtual bool     hasPartitionSplitter()  { return false; }

    virtual YWidget *createPatternSelector( YWidget *parent, YWidgetOpt & opt ) IMPL_NULL;
    virtual bool     hasPatternSelector() { return false; }

    virtual YWidget *createWizard( YWidget *parent, YWidgetOpt & opt,
				   const YCPValue & backButtonId,
				   const YCPString & backButtonLabel,
				   const YCPValue & abortButtonId,
				   const YCPString & abortButtonLabel,
				   const YCPValue & nextButtonId,
				   const YCPString & nextButtonLabel ) IMPL_NULL;
    virtual bool     hasWizard() { return false; }

    virtual int  getDisplayWidth();
    virtual int  getDisplayHeight();
    virtual int  getDisplayDepth();
    virtual long getDisplayColors();
    virtual int  getDefaultWidth();
    virtual int  getDefaultHeight();
    virtual bool textMode()              IMPL_RET(false);
    virtual bool hasImageSupport()       IMPL_RET(true);
    virtual bool hasLocalImageSupport()  IMPL_RET(true);
    virtual bool hasAnimationSupport()   IMPL_RET(true);
    virtual bool hasIconSupport()        IMPL_RET(true);
    virtual bool hasFullUtf8Support()    IMPL_RET(true);
    virtual bool richTextSupportsTable() IMPL_RET(false);
    virtual bool leftHandedMouse()       IMPL_RET(false);

    virtual void busyCursor() IMPL;
    virtual void normalCursor() IMPL;
    virtual void redrawScreen() IMPL;
    virtual void makeScreenShot( string filename ) IMPL;

    virtual YCPValue runPkgSelection( YWidget * packageSelector ) IMPL_VOID

	/* File/directory dialogs. */
	virtual YCPValue askForExistingDirectory (const YCPString &startDir,
	     const YCPString &headline);
	virtual YCPValue askForExistingFile (const YCPString &startWith,
	     const YCPString &filter, const YCPString &headline);
	virtual YCPValue askForSaveFileName (const YCPString &startWith,
	     const YCPString &filter, const YCPString &headline);
};

// debug helpers.
void dumpWidgetTree (GtkWidget *widget, int indent = 0);
void dumpYastTree (YWidget *widget, int indent = 0);


#endif // YGUI_H
