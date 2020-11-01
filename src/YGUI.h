/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#ifndef YGUI_H
#define YGUI_H

#include <yui/Libyui_config.h>
#include <yui/YUI.h>
#define YUILogComponent "gtk"
#include <yui/YUILog.h>
#include <yui/YSimpleEventHandler.h>
#include <map>
#include <gtk/gtk.h>


/* Comment the following line to disable debug messages */
#define RET(a) { return (a); }

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
    static void setTextdomain (const char *domain);

	virtual void idleLoop (int fd_ycp);
	// called by YDialog::waitInput() / pollEvent()...
    YEvent *waitInput (unsigned long timeout_ms, bool block);

	virtual YEvent *runPkgSelection (YWidget *packageSelector);

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
    bool m_no_border, m_fullscreen, m_swsingle;

public:
    // Helpers for internal use [ visibility hidden ]
    bool setFullscreen() const { return m_fullscreen; }
    bool unsetBorder() const   { return m_no_border; }
    bool isSwsingle() const    { return m_swsingle; }
};

#include <YWidgetFactory.h>

class YGWidgetFactory : public YWidgetFactory
{
	virtual YDialog *createDialog (YDialogType dialogType, YDialogColorMode colorMode);

    virtual YPushButton *createPushButton (YWidget *parent, const std::string &label);
	virtual YLabel *createLabel (YWidget *parent, const std::string &text, bool isHeading, bool isOutputField);
	virtual YInputField *createInputField (YWidget *parent, const std::string &label, bool passwordMode);
	virtual YCheckBox *createCheckBox (YWidget *parent, const std::string &label, bool isChecked);
	virtual YRadioButton *createRadioButton (YWidget *parent, const std::string &label, bool isChecked);
    virtual YComboBox *createComboBox (YWidget *parent, const std::string & label, bool editable);
	virtual YSelectionBox *createSelectionBox (YWidget *parent, const std::string &label);
	virtual YTree *createTree (YWidget *parent, const std::string &label, bool multiselection, bool recursiveSelection);
	virtual YTable *createTable (YWidget *parent, YTableHeader *headers, bool multiSelection);
	virtual YProgressBar *createProgressBar	(YWidget *parent, const std::string &label, int maxValue);
	virtual YBusyIndicator *createBusyIndicator (YWidget *parent, const std::string &label, int timeout);
	virtual YRichText *createRichText (YWidget *parent, const std::string &text, bool plainTextMode);

	virtual YIntField *createIntField (YWidget *parent, const std::string &label, int minVal, int maxVal, int initialVal);
	virtual YMenuButton *createMenuButton (YWidget *parent, const std::string &label);
	virtual YMultiLineEdit *createMultiLineEdit	(YWidget *parent, const std::string &label);
	virtual YImage *createImage (YWidget *parent, const std::string &imageFileName, bool animated);
	virtual YLogView *createLogView (YWidget *parent, const std::string &label, int visibleLines, int storedLines);
	virtual YMultiSelectionBox *createMultiSelectionBox (YWidget *parent, const std::string &label);

	virtual YPackageSelector *createPackageSelector (YWidget * parent, long ModeFlags);
	virtual YWidget *createPkgSpecial (YWidget * parent, const std::string & subwidgetName) RET (NULL)  // for ncurses

	virtual YLayoutBox *createLayoutBox (YWidget *parent, YUIDimension dimension);
    virtual YButtonBox *createButtonBox (YWidget *parent);

	virtual YSpacing *createSpacing (YWidget *parent, YUIDimension dim, bool stretchable, YLayoutSize_t size);
	virtual YEmpty *createEmpty (YWidget *parent);
	virtual YAlignment *createAlignment (YWidget *parent, YAlignmentType horAlignment, YAlignmentType vertAlignment);
	virtual YSquash *createSquash (YWidget *parent, bool horSquash, bool vertSquash);

	virtual YFrame *createFrame (YWidget *parent, const std::string &label);
	virtual YCheckBoxFrame *createCheckBoxFrame	(YWidget *parent, const std::string &label, bool checked);

	virtual YRadioButtonGroup *createRadioButtonGroup (YWidget *parent);
	virtual YReplacePoint *createReplacePoint (YWidget *parent);

   virtual YMenuBar *createMenuBar ( YWidget * parent );

};

#include <YOptionalWidgetFactory.h>

class YGOptionalWidgetFactory : public YOptionalWidgetFactory
{
public:
	virtual bool hasWizard() RET (true)
	virtual YWizard *createWizard (YWidget *parent, const std::string &backButtonLabel,
		const std::string &abortButtonLabel, const std::string &nextButtonLabel,
		YWizardMode wizardMode);

	virtual bool hasDumbTab() RET (true)
	virtual YDumbTab *createDumbTab (YWidget *parent);

	virtual bool hasSlider() RET (true)
	virtual YSlider *createSlider (YWidget *parent, const std::string &label, int minVal,
		int maxVal, int initialVal);

	virtual bool hasDateField() RET (true)
	virtual YDateField *createDateField (YWidget *parent, const std::string &label);

	virtual bool hasTimeField() RET (true)
	virtual YTimeField *createTimeField (YWidget *parent, const std::string &label);

    virtual bool hasTimezoneSelector() RET (true)
	virtual YTimezoneSelector *createTimezoneSelector (YWidget *parent, 
		const std::string &pixmap,  const std::map <std::string, std::string> &timezones);

	virtual bool hasBarGraph() RET (true)
	virtual YBarGraph *createBarGraph (YWidget *parent);

	virtual bool hasMultiProgressMeter() RET (true)
	virtual YMultiProgressMeter *createMultiProgressMeter (YWidget *parent,
		YUIDimension dim, const std::vector<float> &maxValues);

	virtual bool hasPartitionSplitter() RET (true)
	virtual YPartitionSplitter *createPartitionSplitter (YWidget *parent,
		int usedSize, int totalFreeSize, int newPartSize, int minNewPartSize,
		int minFreeSize, const std::string &usedLabel, const std::string &freeLabel,
		const std::string &newPartLabel, const std::string &freeFieldLabel,
		const std::string &newPartFieldLabel);

	virtual bool hasDownloadProgress() RET (true)
	virtual YDownloadProgress *createDownloadProgress (YWidget *parent,
		const std::string &label, const std::string & filename, YFileSize_t expectedFileSize);

	virtual bool hasContextMenu() RET (true)

	virtual bool hasSimplePatchSelector() RET (true)
	virtual YWidget *createSimplePatchSelector (YWidget *parent, long modeFlags);
	virtual bool hasPatternSelector() RET (true)
	virtual YWidget *createPatternSelector (YWidget *parent, long modeFlags);
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

	virtual void makeScreenShot (const std::string &filename);
	virtual void beep();

	virtual int deviceUnits (YUIDimension dim, float layout_units);
	virtual float layoutUnits (YUIDimension dim, int device_units);

	virtual int  displayWidth();
	virtual int  displayHeight();
	virtual int  displayDepth();
	virtual long displayColors();
	virtual int  defaultWidth();  // internally, use _defaultWidth / Height()
	virtual int  defaultHeight();

    virtual bool isTextMode()            RET (false)
	virtual bool leftHandedMouse()       RET (false)
	virtual bool hasImageSupport()       RET (true)
	virtual bool hasLocalImageSupport()  RET (true)
	virtual bool hasAnimationSupport()   RET (true)
	virtual bool hasIconSupport()        RET (true)
	virtual bool hasFullUtf8Support()    RET (true)
#ifdef USE_WEBKIT
	virtual bool richTextSupportsTable() RET (true)
#else
	virtual bool richTextSupportsTable() RET (false)
#endif

	virtual bool openContextMenu (const YItemCollection &itemCollection);
	
private:
    // for screenshots:
    std::map <std::string, int> screenShotNb;
    std::string screenShotNameTemplate;
};

#undef RET

#endif /*YGUI_H*/

