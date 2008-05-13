/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#ifndef YGUI_H
#define YGUI_H

#include <gtk/gtk.h>
#include <YSimpleEventHandler.h>
#include <map>

#define YUILogComponent "gtk"
#include <YUILog.h>

#define ICON_DIR   THEMEDIR "/icons/22x22/apps/"

/* Comment the following line to disable debug messages */
// #define IMPL_DEBUG
#define LOC       fprintf (stderr, "%s (%s)\n", G_STRLOC, G_STRFUNC)
#ifdef IMPL_DEBUG
	#define IMPL      { LOC; }
#else
	#define IMPL      { }
#endif
#define IMPL_RET(a) { IMPL; return (a); }

/* Compatibility */
/*
#if YAST2_VERSION > 2014004
#  define YAST2_YGUI_CHECKBOX_FRAME 1
#elif YAST2_VERSION >= 2014000
#  define YAST2_YGUI_CHECKBOX_FRAME 0
#elif YAST2_VERSION > 2013032
#  define YAST2_YGUI_CHECKBOX_FRAME 1
#else
#  define YAST2_YGUI_CHECKBOX_FRAME 0
#endif
*/

class YGDialog;

#include <YUI.h>

class YGUI: public YUI
{
public:
    YGUI (bool with_threads);
    void checkInit();  // called 1st time when execution thread kicks in

    static YGUI *ui() { return (YGUI *) YUI::ui(); }

protected:
	virtual YWidgetFactory *createWidgetFactory();
	virtual YOptionalWidgetFactory *createOptionalWidgetFactory();
	virtual YApplication *createApplication();

public:
    static void setTextdomain( const char * domain );

	virtual void idleLoop (int fd_ycp);
	// called by YDialog::waitInput() / pollEvent()...
    YEvent *waitInput (unsigned long timeout_ms, bool block);

	virtual YEvent * runPkgSelection (YWidget *packageSelector);

	// used internally: for public use, see YApplication
	void busyCursor();
	void normalCursor();
	void makeScreenShot();

    // Plays a macro, opening a dialog first to ask for the filename
    // activated by Ctrl-Shift-Alt-P
    void askPlayMacro();
    void toggleRecordMacro();

	// On Shift-F8, run save_logs
	void askSaveLogs();

    YSimpleEventHandler m_event_handler;
    void    sendEvent (YEvent *event);
    YEvent *pendingEvent() const { return m_event_handler.pendingEvent(); }
    bool    eventPendingFor (YWidget *widget) const
    { return m_event_handler.eventPendingFor (widget); }

private:
    bool m_done_init;
    guint busy_timeout;  // for busy cursor
    static gboolean busy_timeout_cb (gpointer data);

    // window-related arguments
    bool m_have_wm, m_no_border, m_fullscreen;
    GtkRequisition m_default_size;

public:
    // Helpers for internal use [ visibility hidden ]
    int _getDefaultWidth(); int _getDefaultHeight();
    bool setFullscreen() const { return m_fullscreen; }
    bool hasWM() const         { return m_have_wm; }
    bool unsetBorder() const   { return m_no_border; }
};

// debug helpers.
void dumpYastTree (YWidget *widget);
void dumpYastHtml (YWidget *widget);
void dumpGtkTree  (GtkWidget *widget);

#include <YWidgetFactory.h>

class YGWidgetFactory : public YWidgetFactory
{
	virtual YDialog *createDialog (YDialogType dialogType, YDialogColorMode colorMode);

    virtual YPushButton *createPushButton (YWidget *parent, const string &label);
	virtual YLabel *createLabel (YWidget *parent, const string &text, bool isHeading, bool isOutputField);
	virtual YInputField *createInputField (YWidget *parent, const string &label, bool passwordMode);
	virtual YCheckBox *createCheckBox (YWidget *parent, const string &label, bool isChecked);
	virtual YRadioButton *createRadioButton (YWidget *parent, const string &label, bool isChecked);
    virtual YComboBox *createComboBox (YWidget * parent, const string & label, bool editable);
	virtual YSelectionBox *createSelectionBox (YWidget *parent, const string &label);
	virtual YTree *createTree (YWidget *parent, const string &label);
	virtual YTable *createTable (YWidget * parent, YTableHeader *header);
	virtual YProgressBar *createProgressBar	(YWidget *parent, const string &label, int maxValue);
	virtual YBusyIndicator *createBusyIndicator (YWidget *parent, const string &label, int timeout);
	virtual YRichText *createRichText (YWidget *parent, const string &text, bool plainTextMode);

	virtual YIntField *createIntField (YWidget *parent, const string &label, int minVal, int maxVal, int initialVal);
	virtual YMenuButton *createMenuButton (YWidget *parent, const string &label);
	virtual YMultiLineEdit *createMultiLineEdit	(YWidget *parent, const string &label);
	virtual YImage *createImage (YWidget *parent, const string &imageFileName, bool animated);
	virtual YLogView *createLogView (YWidget *parent, const string &label, int visibleLines, int storedLines);
	virtual YMultiSelectionBox *createMultiSelectionBox (YWidget *parent, const string &label);

	virtual YPackageSelector *createPackageSelector (YWidget * parent, long ModeFlags);
	virtual YWidget *createPkgSpecial (YWidget * parent, const string & subwidgetName)
		IMPL_RET (NULL)  // for ncurses

	virtual YLayoutBox *createLayoutBox (YWidget *parent, YUIDimension dimension);

	virtual YSpacing *createSpacing (YWidget *parent, YUIDimension dim, bool stretchable, YLayoutSize_t size);
	virtual YEmpty *createEmpty (YWidget *parent);
	virtual YAlignment *createAlignment (YWidget *parent, YAlignmentType horAlignment, YAlignmentType vertAlignment);
	virtual YSquash *createSquash (YWidget *parent, bool horSquash, bool vertSquash);

	virtual YFrame *createFrame (YWidget *parent, const string &label);
	virtual YCheckBoxFrame *createCheckBoxFrame	(YWidget *parent, const string &label, bool checked);

	virtual YRadioButtonGroup *createRadioButtonGroup (YWidget *parent);
	virtual YReplacePoint *createReplacePoint (YWidget *parent);
};

#include <YOptionalWidgetFactory.h>

class YGOptionalWidgetFactory : public YOptionalWidgetFactory
{
public:
	virtual bool hasWizard() IMPL_RET (true)
	virtual YWizard *createWizard (YWidget *parent, const string &backButtonLabel,
		const string &abortButtonLabel, const string &nextButtonLabel,
		YWizardMode wizardMode);

	virtual bool hasDumbTab() IMPL_RET (true)
	virtual YDumbTab *createDumbTab (YWidget *parent);

	virtual bool hasSlider() IMPL_RET (true)
	virtual YSlider *createSlider (YWidget *parent, const string &label, int minVal,
		int maxVal, int initialVal);

	virtual bool hasDateField() IMPL_RET (true)
	virtual YDateField *createDateField (YWidget *parent, const string &label);

	virtual bool hasTimeField() IMPL_RET (true)
	virtual YTimeField *createTimeField (YWidget *parent, const string &label);

	virtual bool hasBarGraph() IMPL_RET (true)
	virtual YBarGraph *createBarGraph (YWidget *parent);

	virtual bool hasMultiProgressMeter() IMPL_RET (true)
	virtual YMultiProgressMeter *createMultiProgressMeter (YWidget *parent,
		YUIDimension dim, const vector<float> &maxValues);

	virtual bool hasPartitionSplitter() IMPL_RET (true)
	virtual YPartitionSplitter *createPartitionSplitter (YWidget *parent,
		int usedSize, int totalFreeSize, int newPartSize, int minNewPartSize,
		int minFreeSize, const string &usedLabel, const string &freeLabel,
		const string &newPartLabel, const string &freeFieldLabel,
		const string &newPartFieldLabel);

	virtual bool hasDownloadProgress() IMPL_RET (true)
	virtual YDownloadProgress *createDownloadProgress (YWidget *parent,
		const string &label, const string & filename, YFileSize_t expectedFileSize);

	virtual bool hasSimplePatchSelector() IMPL_RET (false)
	virtual YWidget *createSimplePatchSelector (YWidget *parent, long modeFlags)
		IMPL_RET (NULL)
	virtual bool hasPatternSelector() IMPL_RET (false)
	virtual YWidget *createPatternSelector (YWidget *parent, long modeFlags)
		IMPL_RET (NULL)
};

#include <YApplication.h>

class YGApplication : public YApplication
{
public:
	YGApplication();

	virtual std::string glyph (const std::string &symbolName);

	virtual std::string askForExistingDirectory (const std::string &startDir,
		const std::string &headline);
	virtual std::string askForExistingFile (const std::string &startWith,
		const std::string &filter, const std::string &headline);
	virtual std::string askForSaveFileName (const std::string &startWith,
		const std::string &filter, const std::string &headline);

	virtual void busyCursor() { YGUI::ui()->busyCursor(); }
	virtual void normalCursor() { YGUI::ui()->normalCursor(); }

	virtual void makeScreenShot (string filename);
	virtual void beep();

	virtual int deviceUnits (YUIDimension dim, float layout_units);
	virtual float layoutUnits (YUIDimension dim, int device_units);

	virtual int  displayWidth();
	virtual int  displayHeight();
	virtual int  displayDepth();
	virtual long displayColors();
	virtual int  defaultWidth();  // internally, use _defaultWidth / Height()
	virtual int  defaultHeight();

    virtual bool isTextMode()            IMPL_RET (false)
	virtual bool leftHandedMouse()       IMPL_RET (false)
	virtual bool hasImageSupport()       IMPL_RET (true)
	virtual bool hasLocalImageSupport()  IMPL_RET (true)
	virtual bool hasAnimationSupport()   IMPL_RET (true)
	virtual bool hasIconSupport()        IMPL_RET (true)
	virtual bool hasFullUtf8Support()    IMPL_RET (true)
	virtual bool richTextSupportsTable() IMPL_RET (false)

private:
    // for screenshots:
    std::map <std::string, int> screenShotNb;
    std::string screenShotNameTemplate;
};

#endif /*YGUI_H*/

