//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#ifndef YGUI_H
#define YGUI_H

#include <gtk/gtk.h>
#include <YSimpleEventHandler.h>
#include <YUI.h>

using std::string;
using std::vector;

/* Comment the following line to disable debug messages */
// #define IMPL_DEBUG
#define LOC       fprintf (stderr, "%s (%s)\n", G_STRLOC, G_STRFUNC)
#ifdef IMPL_DEBUG
	#define IMPL      { LOC; }
#else
	#define IMPL      { }
#endif
#define IMPL_NULL   { IMPL; return NULL; }
#define IMPL_VOID   { IMPL; return YCPVoid(); }
#define IMPL_RET(a) { IMPL; return (a); }

#define ICON_DIR   THEMEDIR "/icons/22x22/apps/"

//#define ENABLE_BEEP

class YGUI: public YUI
{
public:
    YGUI (int argc, char **argv,
          bool with_threads, const char *macro_file);
    virtual ~YGUI();

    static YGUI *ui() { return (YGUI *)YUI::ui(); }

    // non abstract loop bits:
    virtual void blockEvents (bool block = true) IMPL
    virtual bool eventsBlocked() const IMPL_RET (false)
    virtual void internalError (const char *msg);
    virtual void idleLoop (int fd_ycp);
    virtual YEvent * userInput (unsigned long timeout_millisec);
    virtual YEvent * pollInput() IMPL_NULL

    virtual void showDialog (YDialog *dialog);
    virtual void closeDialog (YDialog *dialog);
    // Non abstract virtuals:
    virtual YCPValue setLanguage (const YCPTerm & term) IMPL_VOID

    // event pieces:
 private:
    YSimpleEventHandler m_event_handler;
 public:
    void    sendEvent (YEvent *event);
    YEvent *pendingEvent() const { return m_event_handler.pendingEvent(); }
    bool    eventPendingFor (YWidget *widget) const { return m_event_handler.eventPendingFor (widget); }

    // container widgets
    virtual YDialog *createDialog (YWidgetOpt &opt);
    virtual YContainerWidget *createSplit (YWidget *parent, YWidgetOpt &opt, YUIDimension dimension);
    virtual YContainerWidget *createReplacePoint (YWidget *parent, YWidgetOpt &opt);
    virtual YContainerWidget *createAlignment (YWidget *parent, YWidgetOpt &opt,
                                               YAlignmentType halign,
                                               YAlignmentType valign);
    virtual YContainerWidget *createSquash (YWidget *parent, YWidgetOpt &opt,
                                            bool hsquash, bool vsquash);
    virtual YContainerWidget *createRadioButtonGroup (YWidget *parent, YWidgetOpt &opt);
    virtual YContainerWidget *createFrame (YWidget *parent, YWidgetOpt &opt,
                                           const YCPString &label);
#if YAST2_VERSION > 2014004 || YAST2_VERSION > 2013032
    virtual YContainerWidget *createCheckBoxFrame( YWidget *parent, YWidgetOpt & opt, const YCPString & label, bool checked );
#endif

    // leaf widgets
    virtual YWidget *createEmpty (YWidget *parent, YWidgetOpt &opt);
    virtual YWidget *createSpacing (YWidget *parent, YWidgetOpt &opt, float size,
                                    bool horizontal, bool vertical);
    virtual YWidget *createLabel (YWidget *parent, YWidgetOpt &opt,
                                  const YCPString &text);
    virtual YWidget *createRichText (YWidget *parent, YWidgetOpt &opt,
                                     const YCPString &text);
    virtual YWidget *createLogView (YWidget *parent, YWidgetOpt &opt,
                                    const YCPString &label, int visibleLines,
                                    int maxLines);
    virtual YWidget *createPushButton (YWidget *parent, YWidgetOpt &opt,
                                       const YCPString & label);
    virtual YWidget *createMenuButton (YWidget *parent, YWidgetOpt &opt,
                                       const YCPString & label);
    virtual YWidget *createRadioButton (YWidget *parent, YWidgetOpt &opt,
                                        YRadioButtonGroup *rbg,
                                        const YCPString &label, bool checked);
    virtual YWidget *createCheckBox (YWidget *parent, YWidgetOpt &opt,
                                     const YCPString &label, bool checked);
    virtual YWidget *createTextEntry (YWidget *parent, YWidgetOpt &opt,
                                      const YCPString &label, const YCPString &text);
    virtual YWidget *createMultiLineEdit (YWidget *parent, YWidgetOpt &opt,
                                          const YCPString &label,
                                          const YCPString &text);
    virtual YWidget *createSelectionBox (YWidget *parent, YWidgetOpt &opt,
                                         const YCPString &label);
    virtual YWidget *createMultiSelectionBox (YWidget *parent, YWidgetOpt &opt,
                                              const YCPString &label);
    virtual YWidget *createComboBox (YWidget *parent, YWidgetOpt &opt,
                                     const YCPString &label);
    virtual YWidget *createTree (YWidget *parent, YWidgetOpt &opt,
                                 const YCPString &label);
    virtual YWidget *createTable (YWidget *parent, YWidgetOpt &opt,
                                  std::vector<std::string> header);
    virtual YWidget *createProgressBar (YWidget *parent, YWidgetOpt &opt,
                                        const YCPString &label,
                                        const YCPInteger &maxprogress,
                                        const YCPInteger &progress);
    virtual YWidget *createImage (YWidget *parent, YWidgetOpt &opt,
                                  YCPByteblock imagedata, YCPString defaulttext);
    virtual YWidget *createImage (YWidget *parent, YWidgetOpt &opt,
                                  YCPString file_name, YCPString defaulttext);
    virtual YWidget *createIntField (YWidget *parent, YWidgetOpt &opt,
                                     const YCPString &label, int minValue,
                                     int maxValue, int initialValue);

    // Package selector
    virtual YWidget *createPackageSelector (YWidget *parent, YWidgetOpt &opt,
                                            const YCPString &floppyDevice);
    virtual YWidget *createPkgSpecial (YWidget *parent, YWidgetOpt &opt,
                                       const YCPString &subwidget);
    virtual YCPValue runPkgSelection (YWidget *packageSelector);

    // Optional widgets
    virtual YWidget *createDummySpecialWidget (YWidget *parent, YWidgetOpt &opt)
        IMPL_NULL;
    virtual bool     hasDummySpecialWidget() { return false; }

    virtual YWidget *createDownloadProgress (YWidget *parent, YWidgetOpt &opt,
                                             const YCPString &label,
                                             const YCPString &filename,
                                             int expectedSize);
    virtual bool     hasDownloadProgress() { return true; }

    virtual YWidget *createBarGraph (YWidget *parent, YWidgetOpt &opt);
    virtual bool hasBarGraph() { return true; }

    virtual YWidget *createColoredLabel (YWidget *parent, YWidgetOpt &opt,
                                         YCPString label, YColor foreground,
                                         YColor background, int margin);
    virtual bool hasColoredLabel() { return true; }

    virtual YWidget *createDate (YWidget *parent, YWidgetOpt &opt,
                                 const YCPString &label, const YCPString &date);
    virtual bool     hasDate() { return true; }

    virtual YWidget *createTime (YWidget *parent, YWidgetOpt &opt,
                                 const YCPString &label, const YCPString &time);
    virtual bool     hasTime() { return true; }

    virtual YWidget *createDumbTab (YWidget *parent, YWidgetOpt &opt);
    virtual bool     hasDumbTab() { return true; }

    virtual YWidget *createMultiProgressMeter (YWidget *parent, YWidgetOpt &opt,
                                      bool horizontal, const YCPList & maxValues);
    virtual bool     hasMultiProgressMeter() { return true; }

    virtual YWidget *createSlider (YWidget *parent, YWidgetOpt &opt,
                                   const YCPString &label, int min,
                                   int max, int initial);
    virtual bool hasSlider() { return true; }

    virtual YWidget *createPartitionSplitter (YWidget    *parent,
                                              YWidgetOpt &opt,
                                              int         usedSize,
                                              int         totalFreeSize,
                                              int         newPartSize,
                                              int         minNewPartSize,
                                              int         minFreeSize,
                                         const YCPString &usedLabel,
                                         const YCPString &freeLabel,
                                         const YCPString &newPartLabel,
                                         const YCPString &freeFieldLabel,
                                         const YCPString &newPartFieldLabel);
    virtual bool     hasPartitionSplitter()  { return true; }

    virtual YWidget *createPatternSelector (YWidget *parent, YWidgetOpt &opt) IMPL_NULL;
    virtual bool     hasPatternSelector() { return false; }

    virtual YWidget *createWizard (YWidget *parent,
                                YWidgetOpt &opt,
                            const YCPValue &backButtonId,
                           const YCPString &backButtonLabel,
                            const YCPValue &abortButtonId,
                           const YCPString &abortButtonLabel,
                            const YCPValue &nextButtonId,
                           const YCPString &nextButtonLabel);
    virtual bool     hasWizard() { return true; }


    virtual int  getDisplayWidth();
    virtual int  getDisplayHeight();
    // let's fool YWidgets here because it tries to be smart if our default
    // dialog size is too small
    virtual int  getDefaultWidth()  { return 10000; }
    virtual int  getDefaultHeight() { return 10000; }
    // Dunno where this is used in practice, but these can be got by a YCP
    // apps, so better implement them.
    virtual int  getDisplayDepth()  { return 24;    }
    virtual long getDisplayColors() { return 256*256*256; }

    virtual long deviceUnits (YUIDimension dim, float size);
    virtual float layoutUnits (YUIDimension dim, long device_units);

    virtual bool textMode();
    virtual bool hasImageSupport();
    virtual bool hasLocalImageSupport();
    virtual bool hasAnimationSupport();
    virtual bool hasIconSupport();
    virtual bool hasFullUtf8Support();
    virtual bool richTextSupportsTable();
    virtual bool leftHandedMouse();

    virtual YCPString glyph (const YCPSymbol &glyph);
    virtual void redrawScreen();

    // convience function to be used rather than currentDialog()
    // NULL if there is no dialog at the moment.
    GtkWindow *currentWindow();

    virtual void busyCursor();
    virtual void normalCursor();
    guint busy_timeout;  // for busy cursor
    static gboolean busy_timeout_cb (gpointer data);

#ifdef ENABLE_BEEP
    virtual void beep();
#endif
    virtual void makeScreenShot (string filename);

    /* File/directory dialogs. */
    virtual YCPValue askForExistingDirectory (const YCPString &startDir,
                                              const YCPString &headline);
    virtual YCPValue askForExistingFile (const YCPString &startWith,
                                         const YCPString &filter,
                                         const YCPString &headline);
    virtual YCPValue askForSaveFileName (const YCPString &startWith,
                                         const YCPString &filter,
                                         const YCPString &headline);

    // Starts macro recording and asks for a filename to save it to
    // If there is already one in progress, it just resumes/pauses as appropriate
    // activated by Ctrl-Shift-Alt-M
    void toggleRecordMacro();

    // Plays a macro, opening a dialog first to ask for the filename
    // activated by Ctrl-Shift-Alt-P
    void askPlayMacro();

 private:
    // window-related arguments
    bool m_have_wm, m_no_border, m_fullscreen;
    GtkRequisition m_default_size;

    // for delayed gtk+ init in the right thread
    bool   m_done_init;
    int    m_argc;
    char **m_argv;
    void  checkInit();

    // for screenshots:
    map <string, int> screenShotNb;
    string screenShotNameTemplate;

 public:
    // Helpers for internal use [ visibility hidden ]
    int getDefaultSize (YUIDimension dim);
    bool setFullscreen() const { return m_fullscreen || !m_have_wm; }
    bool hasWM() const         { return m_have_wm; }
    bool unsetBorder() const   { return m_no_border; }
};

// debug helpers.
void dumpYastTree (YWidget *widget, GtkWindow *parent_window = 0);

#endif // YGUI_H
