//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <stdio.h>
#include <ycp/y2log.h>
#include <YEvent.h>
#include <YDialog.h>
#include <YMacroRecorder.h>
#include "YGUI.h"
#include "YGUtils.h"
#include "YGWidget.h"
#include <glib/gthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <rpc/types.h>  // MAXHOSTNAMELEN

#define DEFAULT_MACRO_FILE_NAME  "macro.ycp"

YGUI::YGUI (int argc, char ** argv,
	    bool with_threads,
	    const char *macro_file) :
	YUI (with_threads),
	m_have_wm (true),
	m_fullscreen (false),
	m_no_border (false),
	m_done_init (false),
	m_argc (0),
	m_argv (NULL)
{
	IMPL;
	m_argc = argc;
	m_argv = g_new0 (char *, argc);
	memcpy (m_argv, argv, sizeof (char *) * argc);
	if (!with_threads)
		checkInit();

	fprintf (stderr, "I'm initialized '%s' - come & get me !\n",
		with_threads ? "with threads !" : "no threads");

	for (int i = 0; i < m_argc; i++)
	{
		const char *argp = m_argv[i];
		if (!argp) continue;
		if (argp[0] != '-')
		{
			fprintf (stderr, "Unknown argument '%s'\n", argp);
			continue;
		}
		argp++;
		if (argp[0] == '-') argp++;
		if (!strcmp (argp, "no-wm"))
			m_have_wm = false;

		else if (!strcmp (argp, "fullscreen"))
			m_fullscreen = true;

		else if (!strcmp (argp, "noborder"))
			m_no_border = true;

		// FIXME: handle/parse geometry option

		else if (!strcmp (argp, "help"))
		{
			fprintf( stderr,
				 "Command line options for the YaST2 Gtk UI:\n"
				 "\n"
				 "--nothreads   run without additional UI threads\n"
				 "--no-wm       assume no window manager is running\n"
				 "--fullscreen  use full screen for `opt(`defaultsize) dialogs\n"
				 "--noborder    no window manager border for `opt(`defaultsize) dialogs\n"
				 "--help        this help text\n"
				 "\n"
				 );

			// FIXME: raiseFatalError
		}
	}

	m_default_size.width = -1;
	m_default_size.height = -1;

	if (macro_file)
		playMacro (macro_file);

	// without this none of the (default) threading action works ...
	topmostConstructorHasFinished();
}

YGUI::~YGUI()
{
	IMPL
	g_free (m_argv);
}

static gboolean
ycp_wakeup_fn (GIOChannel   *source,
				GIOCondition  condition,
				gpointer      data)
{
	*(int *)data = TRUE;
	return TRUE;
}

void YGUI::checkInit()
{
	if (!m_done_init)
		gtk_init (&m_argc, &m_argv);
	m_done_init = TRUE;
}

void
YGUI::idleLoop (int fd_ycp)
{
	IMPL;

	// The rational for this is that we need somewhere to run
	// the magic 'main' thread, that can process thread unsafe
	// incoming CORBA messages for us
	checkInit();

	GIOChannel *wakeup;

	wakeup = g_io_channel_unix_new (fd_ycp);
	g_io_channel_set_encoding (wakeup, NULL, NULL);
	g_io_channel_set_buffered (wakeup, FALSE);

	int woken = FALSE;
	guint watch_tag = g_io_add_watch (wakeup, (GIOCondition)(G_IO_IN | G_IO_PRI),
					  ycp_wakeup_fn, &woken);
	while (!woken)
		g_main_iteration (TRUE);

	g_source_remove (watch_tag);
	g_io_channel_unref (wakeup);
}

static gboolean
set_timeout (YGUI *pThis)
{
	IMPL;
	if (!pThis->pendingEvent())
		pThis->sendEvent (new YTimeoutEvent());
	return FALSE;
}

void
YGUI::sendEvent (YEvent *event)
{
	m_event_handler.sendEvent (event);
	g_main_context_wakeup (NULL);
}

YEvent *
YGUI::userInput( unsigned long timeout_millisec )
{
	guint timeout = 0;
	YEvent *event = NULL;
	LOC;
	if (timeout_millisec > 0)
		timeout = g_timeout_add (timeout_millisec,
					(GSourceFunc)set_timeout, this);

	// FIXME: do it only if currentDialog (?) ...
	while (!pendingEvent())
		g_main_iteration (TRUE);

	event = m_event_handler.consumePendingEvent();

	if (timeout)
		g_source_remove (timeout);

	return event;
}

YEvent *
YGUI::pollInput()
{
	LOC;
	return NULL;
}

// dialog bits

void dumpWidgetTree (GtkWidget *widget, int indent)
{
	for (int i = 0; i < indent; i++)
		fprintf (stderr, "\t");

	if (!widget)
		fprintf (stderr, "NULL widget\n");

	GList *children = NULL;
	if (GTK_IS_CONTAINER (widget))
		children = gtk_container_get_children (GTK_CONTAINER (widget));

	fprintf (stderr, "Widget %p (%s) [%s] children (%d) req (%d,%d) alloc (%d,%d, %d,%d)\n",
		widget, g_type_name_from_instance ((GTypeInstance *)widget),
		GTK_WIDGET_VISIBLE (widget) ? "visible" : "invisible",
		g_list_length (children),
		widget->requisition.width, widget->requisition.height,
		widget->allocation.x, widget->allocation.y,
		widget->allocation.width, widget->allocation.height);

	for (GList *l = children; l; l = l->next)
		dumpWidgetTree ((GtkWidget *)l->data, indent + 1);
	g_list_free (children);
}

void dumpYastTree (YWidget *widget, int indent)
{
	YContainerWidget *cont = NULL;

	for (int i = 0; i < indent; i++)
		fprintf (stderr, "\t");

	if (!widget)
		fprintf (stderr, "NULL widget\n");

	if (widget->isContainer())
		cont = (YContainerWidget *) widget;

	fprintf (stderr, "Widget %p (%s) children (%d)\n",
		widget, widget->widgetClass(),
		cont ? cont->numChildren() : -1);

	if (cont)
		for (int i = 0; i < cont->numChildren(); i++)
			dumpYastTree (cont->child (i), indent + 1);
}

static GdkScreen *
getScreen ()
{
	return gdk_display_get_default_screen (
		gdk_display_get_default());
}

int
YGUI::getDisplayWidth()
{
	IMPL;
	return gdk_screen_get_width (getScreen());
}

int
YGUI::getDisplayHeight()
{
	IMPL;
	return gdk_screen_get_height (getScreen());
}

int YGUI::getDisplayDepth()
{
	IMPL;
	return 24; // FIXME: what is this used for ?
}

long YGUI::getDisplayColors()
{
	IMPL;
	return 256*256*256; // FIXME: what is this used for ?
}

// 70% of screen size ... (or 800x600)
#define SHRINK(a) (int)(0.7 * (a))
int YGUI::getDefaultWidth()
{
	// FIXME: -fullscreen option ?
	IMPL;
	return MAX(SHRINK(gdk_screen_get_width (getScreen())), 800);
}

int YGUI::getDefaultHeight()
{
	IMPL;
	return MAX(SHRINK(gdk_screen_get_height (getScreen())), 600);
}

static void errorMsg (const char *msg)
{
	GtkWidget* dialog = gtk_message_dialog_new (NULL,
		GtkDialogFlags (0), GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, msg);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

void YGUI::internalError (const char *msg)
{
	errorMsg (msg);
	abort();		// going down
}

GtkWindow *YGUI::currentWindow()
{
	YWidget *dialog = YGUI::ui()->currentDialog();
	if (dialog)
		return GTK_WINDOW (((YGWidget*) dialog->widgetRep())->getWidget());
	else
		return NULL;
}

/* File/directory dialogs. */
#include <sstream>

static YCPValue askForFileOrDirectory (GtkFileChooserAction action,
		const YCPString &startWith, const YCPString &filter_pattern,
		const YCPString &headline)
{
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new (headline->value_cstr(),
		YGUI::ui()->currentWindow(), action,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN,   GTK_RESPONSE_ACCEPT,
		NULL);

	// Yast gives startWith as an URL that can be a directory or a file
	// All this chunck of code is because GTK+ likes to get those
	// in separated.
	if (g_file_test (startWith->value_cstr(), G_FILE_TEST_EXISTS)) {
		if (action == GTK_FILE_CHOOSER_ACTION_OPEN ||
		    action == GTK_FILE_CHOOSER_ACTION_SAVE) {
				if (g_file_test (startWith->value_cstr(), G_FILE_TEST_IS_DIR))
					gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),
					                                     startWith->value_cstr());
				else {
					string dirname, filename;
					YGUtils::splitPath (startWith->value(), dirname, filename);
					gtk_file_chooser_set_current_folder
						(GTK_FILE_CHOOSER (dialog),dirname.c_str());
					gtk_file_chooser_set_current_name
						(GTK_FILE_CHOOSER (dialog), filename.c_str());
				}
		}
		else
			gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),
			                                     startWith->value_cstr());
	}
	else if (!startWith->value().empty() &&
	         action == GTK_FILE_CHOOSER_ACTION_SAVE)
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog),
		                                   startWith->value_cstr());

	if (action != GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER) {
		GtkFileFilter *filter = gtk_file_filter_new();
		gtk_file_filter_set_name (filter, filter_pattern->value_cstr());
		// cut filter_pattern into chuncks like GTK likes
		std::istringstream stream (filter_pattern->value());
		while (!stream.eof()) {
			string str;
			stream >> str;
			if (!str.empty() && str [str.size()-1] == ',')
				str.erase (str.size()-1);
			gtk_file_filter_add_pattern (filter, str.c_str());
		}
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
	}

	YCPValue ret;
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char* filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		YCPString ret (filename);
		g_free (filename);

		gtk_widget_destroy (dialog);
		return ret;
	}
	gtk_widget_destroy (dialog);
	return YCPVoid();
}

YCPValue YGUI::askForExistingDirectory (const YCPString &startDir,
		const YCPString &headline)
{
	return askForFileOrDirectory (GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, startDir,
	                              YCPString ("*"), headline);
}

YCPValue YGUI::askForExistingFile (const YCPString &startWith,
		const YCPString &filter, const YCPString &headline)
{
	return askForFileOrDirectory (GTK_FILE_CHOOSER_ACTION_OPEN, startWith,
	                              filter, headline);
}

YCPValue YGUI::askForSaveFileName (const YCPString &startWith,
		const YCPString &filter, const YCPString &headline)
{
	return askForFileOrDirectory (GTK_FILE_CHOOSER_ACTION_SAVE, startWith,
	                              filter, headline);
}

bool YGUI::textMode()              IMPL_RET(false)
bool YGUI::hasImageSupport()       IMPL_RET(true)
bool YGUI::hasLocalImageSupport()  IMPL_RET(true)
bool YGUI::hasAnimationSupport()   IMPL_RET(true)
bool YGUI::hasIconSupport()        IMPL_RET(true)
bool YGUI::hasFullUtf8Support()    IMPL_RET(true)
bool YGUI::richTextSupportsTable() IMPL_RET(false)
bool YGUI::leftHandedMouse()       IMPL_RET(false)

void YGUI::busyCursor()
{
// FIXME: obusy cursor should be disabled automatically, like Qt's
#if 0
	GtkWidget *window = GTK_WIDGET (currentWindow());
	if (!window) return;

	// NOTE: GdkDisplay won't change for new dialogs, so we don't
	// have to synchronize between them or something.
	static GdkCursor *cursor = NULL;
	if (!cursor) {
		GdkDisplay *display = gtk_widget_get_display (window);
		cursor = gdk_cursor_new_for_display (display, GDK_WATCH);
	}

	gdk_window_set_cursor (window->window, cursor);
#endif
}

void YGUI::normalCursor()
{
#if 0
	GtkWidget *window = GTK_WIDGET (currentWindow());
	if (window)
		gdk_window_set_cursor (window->window, NULL);
#endif
}

void YGUI::redrawScreen()
{
	gtk_widget_queue_draw (GTK_WIDGET (currentWindow()));
}

YCPValue YGUI::runPkgSelection (YWidget *packageSelector)
{
printf ("runPkgSelection()\n");
	y2milestone ("Running package selection...");

	// TODO: we may have to do some trickery here to disable close button
	// and auto activate dialogs (whatever that is)
//	_wm_close_blocked           = true;
//	_auto_activate_dialogs      = false;

	YCPValue input = YCPVoid();
	try {
		input = evaluateUserInput();
	}
	catch (const std::exception &e) {
		y2error ("Caught std::exception: %s", e.what());
		y2error ("This is a libzypp problem. Do not file a bug against the UI!");
	}
	catch (...) {
		y2error ("Caught unspecified exception.");
		y2error ("This is a libzypp problem. Do not file a bug against the UI!");
	}

//	_auto_activate_dialogs      = true;
//	_wm_close_blocked           = false;
	y2milestone ("Package selection done - returning %s", input->toString().c_str());

	return input;
}

void YGUI::makeScreenShot (string filename)
{
	IMPL
	bool interactive = filename.empty();

	GtkWidget *widget = GTK_WIDGET (currentWindow());
	if (!widget) {
		if (interactive)
			errorMsg ("No dialog to take screenshot of.");
		return;
	}

	GError *error = 0;
	GdkPixbuf *shot =
		gdk_pixbuf_get_from_drawable (NULL, GDK_DRAWABLE (widget->window),
			gdk_colormap_get_system(), 0, 0, 0, 0, widget->allocation.width,
			widget->allocation.height);

	if (!shot) {
		if (interactive)
			errorMsg ("Couldn't take a screenshot.");
		return;
	}

	if (interactive) {
		//** ask user for filename
		// calculate a default directory...
		if (screenShotNameTemplate.empty()) {
			string dir;
			const char *homedir = getenv("HOME");
			const char *ssdir = getenv("Y2SCREENSHOTS");
			if (!homedir || !strcmp (homedir, "/")) {
				// no homedir defined (installer)
				dir = "/tmp/" + (ssdir ? (string(ssdir)) : (string("")));
				if (mkdir (dir.c_str(), 0700) == -1)
					dir = "";
			}
			else {
				dir = homedir + (ssdir ? ("/" + string(ssdir)) : (string("")));
				mkdir (dir.c_str(), 0750);  // create a dir for what to put the pics
			}

			screenShotNameTemplate = dir + "/%s-%03d.png";
		}

		// calculate a default filename...
		const char *baseName = moduleName();
		if  (!baseName)
			baseName = "scr";

		int nb;
		map <string, int>::iterator it = screenShotNb.find (baseName);
		if (it == screenShotNb.end())
			nb = 0;

		{
			char *filename_t = g_strdup_printf (screenShotNameTemplate.c_str(), baseName, nb);
			filename = filename_t;
			g_free (filename_t);
		}
		y2debug ("screenshot: %s", filename.c_str());

		YCPValue ret = askForSaveFileName (YCPString (filename.c_str()),
		                                   YCPString ("*.png"),
		                                   YCPString ("Save screenshot to"));
		if (!ret->isString()) {  // user dismissed the dialog
			y2debug ("Save screen shot canceled by user");
			goto makeScreenShot_ret;
		}

		filename = ret->asString()->value();
		screenShotNb.erase (baseName);
		screenShotNb[baseName] = nb + 1;
	}

	y2debug ("Saving screen shot to %s", filename.c_str());
	if (gdk_pixbuf_save (shot, filename.c_str(), "png", &error, NULL)) {
		y2error ("Couldn't save screen shot %s", filename.c_str());
		if (interactive) {
			string msg = "Couldn't save screenshot to file " + filename
			             + " - " + error->message;
			errorMsg (msg.c_str());
		}
		goto makeScreenShot_ret;
	}

	if (recordingMacro()) {
		// save the taking of the screenshot and its name to the macro
		macroRecorder->beginBlock();
		currentDialog()->saveUserInput (macroRecorder);
		macroRecorder->recordMakeScreenShot (true, filename.c_str());
		macroRecorder->recordUserInput (YCPVoid());
		macroRecorder->endBlock();
	}

	makeScreenShot_ret:
		g_object_unref (G_OBJECT (shot));
}

YCPString YGUI::glyph (const YCPSymbol &symbol)
{
    string sym = symbol->symbol();
	if (sym == YUIGlyph_ArrowLeft)
		return YCPString ("\u2190");
	if (sym == YUIGlyph_ArrowRight)
		return YCPString ("\u2192");
	if (sym == YUIGlyph_ArrowUp)
		return YCPString ("\u2191");
	if (sym == YUIGlyph_ArrowDown)
		return YCPString ("\u2193");
	if (sym == YUIGlyph_CheckMark)
		return YCPString ("\u2714");
	if (sym == YUIGlyph_BulletArrowRight)
		return YCPString ("\u279c");
	if (sym == YUIGlyph_BulletCircle)
		return YCPString ("\u274d");
	if (sym == YUIGlyph_BulletSquare)
		return YCPString ("\u274f");
	return YCPString ("");
}

void YGUI::toggleRecordMacro()
{
	if (recordingMacro()) {
		stopRecordMacro();
		normalCursor();

		GtkWidget* dialog = gtk_message_dialog_new (NULL,
			GtkDialogFlags (0), GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
			"Macro recording done.");
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
	else {
		normalCursor();
		YCPValue ret = askForSaveFileName (YCPString (DEFAULT_MACRO_FILE_NAME),
		                                   YCPString ("*.ycp"),
		                                   YCPString ("Select Macro File to Record to"));
		if (ret->isString()) {
			YCPString filename = ret->asString();
			recordMacro (filename->value_cstr());
		}
	}
}

void YGUI::askPlayMacro()
{
	normalCursor();
	YCPValue ret = askForExistingFile (YCPString (DEFAULT_MACRO_FILE_NAME),
	                                   YCPString ("*.ycp"),
	                                   YCPString ("Select Macro File to Play"));

	if (ret->isString()) {
//		busyCursor();  // we need to stop this...
		YCPString filename = ret->asString();

		playMacro (filename->value_cstr());
		sendEvent (new YEvent());  // flush
	}
}

// Internal helper functions
bool YGUI::haveWM() const
{
	return m_have_wm;
}

long YGUI::defaultSize (YUIDimension dim)
{
	// FIXME: check for panel etc. bits available vs. full screen size
	GtkRequisition availableSize = { getDisplayWidth(), getDisplayHeight() };

	fprintf (stderr, "YGUI::defaultSize %d %d\n", m_fullscreen, haveWM());

	if (m_fullscreen || !haveWM())
		return dim == YD_HORIZ ? getDisplayWidth() : getDisplayHeight();

	// FIXME: should parse --geometry options ?
	m_default_size.width = -1;
	m_default_size.height = -1;

	if (m_default_size.width  < 800 ||
	    m_default_size.height < 600) {
		if (getDisplayWidth() >= 1024 &&
		    getDisplayHeight() >= 768) { // 70% of screen size
			fprintf (stderr, "YGUI::defaultSize - set 70%%\n");
			m_default_size.width = MAX ((int)(availableSize.width * 0.7), 800);
			m_default_size.height = MAX ((int)(availableSize.height * 0.7), 600);
		}
	} else {
		fprintf (stderr, "YGUI::defaultSize - has sane m_default_size already\n");
		m_default_size = availableSize;
	}
	
	return (dim == YD_HORIZ) ? m_default_size.width : m_default_size.height;
}
