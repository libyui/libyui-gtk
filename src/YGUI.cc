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

#define DEFAULT_MACRO_FILE_NAME  "macro.ycp"
#define BUSY_CURSOR_TIMEOUT 250

YGUI::YGUI (int argc, char ** argv,
	    bool with_threads,
	    const char *macro_file) :
	YUI (with_threads),
	busy_timeout (0),
	m_done_init (false),
	m_argc (0),
	m_argv (NULL)
{
	IMPL
	m_have_wm = true;
	m_no_border = m_fullscreen = false;
	m_default_size.width = m_default_size.height = 0;

	m_argc = argc;
	m_argv = g_new0 (char *, argc);
	memcpy (m_argv, argv, sizeof (char *) * argc);
	if (!with_threads)
		checkInit();

#ifdef IMPL_DEBUG
	fprintf (stderr, "I'm initialized '%s' - come & get me !\n",
		with_threads ? "with threads !" : "no threads");
#endif

	for (int i = 1; i < m_argc; i++)
	{
		const char *argp = m_argv[i];
		if (!argp) continue;
		if (argp[0] != '-') {
			printf ("Warning: Unknown argument '%s'\n", argp);
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
		else if (!strcmp (argp, "geometry")) {
			argp = argv [++i];
			if (i == argc)
				printf ("Warning: no value passed to --geometry\n");
			else if (sscanf (argp, "%dx%d", &m_default_size.width,
			                 &m_default_size.height) == EOF) {
				printf ("Warning: invalid --geometry value: %s\n", argp);
				m_default_size.width = m_default_size.height = 0;
			}
		}
		else if (!strcmp (argp, "help")) {
			printf (
				 "Command line options for the YaST2 Gtk UI:\n"
				 "\n"
				 "--no-wm       assume no window manager is running\n"
				 "--noborder    no window manager border for main dialogs\n"
				 "--fullscreen  use full screen for main dialogs\n"
				 "--geomtry WxH sets a default size of W per H to main dialogs\n"
				 "--nothreads   run without additional UI threads\n"
				 "--help        prints this help text\n"
				 "\n"
				 );
			exit (0);
		}
		else
			printf ("Warning: Unknown argument '--%s'\n", argp);
	}

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
	IMPL
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
	IMPL
	normalCursor();  // waiting for input, so no more busy

	guint timeout = 0;
	YEvent *event = NULL;

	if (timeout_millisec > 0)
		timeout = g_timeout_add (timeout_millisec,
		                         (GSourceFunc) set_timeout, this);

	// FIXME: do it only if currentDialog (?) ...
	while (!pendingEvent())
		g_main_iteration (TRUE);

	event = m_event_handler.consumePendingEvent();

	if (timeout)
		g_source_remove (timeout);

	// if YCP keeps working for more than X time, set busy cursor
	busy_timeout = g_timeout_add (BUSY_CURSOR_TIMEOUT, busy_timeout_cb, this);

	return event;
}

// dialog bits

static void dumpYastTree (YWidget *widget, GtkTreeStore *store,
                          GtkTreeIter *parent_node)
{
	GtkTreeIter iter;
	gtk_tree_store_append (store, &iter, parent_node);

	if (!widget)
		gtk_tree_store_set (store, &iter, 0, "(no children)", -1);
	else {
		YContainerWidget *container = dynamic_cast <YContainerWidget *> (widget);

		YGWidget *ygwidget = YGWidget::get (widget);
		gchar *props = g_strdup_printf ("stretch: %d x %d - weight: %ld x %ld",
		               ygwidget->isStretchable (YD_HORIZ),
		               ygwidget->isStretchable (YD_VERT), widget->weight (YD_HORIZ),
		               widget->weight (YD_VERT));
		gtk_tree_store_set (store, &iter, 0, widget->widgetClass(),
			1, container ? "" : widget->debugLabel().c_str(), 2, props, -1);
		g_free (props);

		if (container)
			for (int i = 0; i < container->numChildren(); i++)
				dumpYastTree (container->child (i), store, &iter);
	}
}

static void destroy_dialog (GtkDialog *dialog, gint arg)
{ IMPL; gtk_widget_destroy (GTK_WIDGET (dialog)); }

void dumpYastTree (YWidget *widget, GtkWindow *parent_window)
{
	IMPL
	GtkTreeStore *store = gtk_tree_store_new (3, G_TYPE_STRING, G_TYPE_STRING,
	                                          G_TYPE_STRING);
	dumpYastTree (widget, store, NULL);

	GtkWidget *dialog = gtk_dialog_new_with_buttons ("YWidgets Tree",
		parent_window,
		GtkDialogFlags (GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR),
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 400, 300);

	GtkWidget *view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	gtk_tree_view_append_column (GTK_TREE_VIEW (view),
		gtk_tree_view_column_new_with_attributes ("Type",
		gtk_cell_renderer_text_new(), "text", 0, NULL));
	gtk_tree_view_append_column (GTK_TREE_VIEW (view),
		gtk_tree_view_column_new_with_attributes ("Label",
		gtk_cell_renderer_text_new(), "text", 1, NULL));
	gtk_tree_view_append_column (GTK_TREE_VIEW (view),
		gtk_tree_view_column_new_with_attributes ("Properties",
		gtk_cell_renderer_text_new(), "text", 2, NULL));
	gtk_tree_view_expand_all (GTK_TREE_VIEW (view));

	GtkWidget *scroll_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll_win),
	                                     GTK_SHADOW_OUT);
	gtk_scrolled_window_set_policy  (GTK_SCROLLED_WINDOW (scroll_win),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_container_add (GTK_CONTAINER (scroll_win), view);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), scroll_win);
	gtk_widget_show_all (dialog);

	g_signal_connect (G_OBJECT (dialog), "response",
	                  G_CALLBACK (destroy_dialog), 0);
}

static GdkScreen *getScreen ()
{
	return gdk_display_get_default_screen (gdk_display_get_default());
}

int YGUI::getDisplayWidth()
{
	IMPL
	return gdk_screen_get_width (getScreen());
}

int YGUI::getDisplayHeight()
{
	IMPL
	return gdk_screen_get_height (getScreen());
}

int YGUI::getDefaultSize (YUIDimension dim)
{
	if (dim == YD_HORIZ) {
		if (!m_default_size.width) {
			if (m_fullscreen)
				m_default_size.width = getDisplayWidth();
			else
				m_default_size.width = MIN (600, getDisplayWidth());
		}
		return m_default_size.width;
	}
	else {  // YD_VERT
		if (!m_default_size.height) {
			if (m_fullscreen)
				m_default_size.height = getDisplayHeight();
			else
				m_default_size.height = MIN (450, getDisplayHeight());
		}
		return m_default_size.height;
	}
}

// YWidget layout units -> pixels conversion. Same as yast-qt's.
long YGUI::deviceUnits (YUIDimension dim, float size)
{
	if (dim == YD_HORIZ) return (long) ((size * (640.0/80)) + 0.5);
	else                 return (long) ((size * (480.0/25)) + 0.5);
}

float YGUI::layoutUnits (YUIDimension dim, long device_units)
{
	float size = (float) device_units;
	if (dim == YD_HORIZ) return size * (80/640.0);
	else                 return size * (25/480.0);
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

/* File/directory dialogs. */
#include <sstream>

static YCPValue askForFileOrDirectory (GtkFileChooserAction action,
		const YCPString &path, const YCPString &filter_pattern,
		const YCPString &title)
{
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new (title->value_cstr(),
		YGUI::ui()->currentWindow(), action, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		action == GTK_FILE_CHOOSER_ACTION_SAVE ? GTK_STOCK_SAVE : GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);

	// Yast gives the path as an URL that can be a directory or a file.
	// We need to pass only the directory path and then select that file.
	string dirpath (path->value()), filename;
	if (!dirpath.empty() && !g_file_test (dirpath.c_str(), G_FILE_TEST_IS_DIR))
		YGUtils::splitPath (path->value(), dirpath, filename);
	if (!dirpath.empty())
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), dirpath.c_str());
	if (!filename.empty())
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), filename.c_str());

	string filter_str (filter_pattern->value());
	if (filter_str != "" && filter_str != "*") {
		GtkFileFilter *filter = gtk_file_filter_new();
		gtk_file_filter_set_name (filter, filter_str.c_str());
		// cut filter_pattern into chuncks like GTK likes
		std::istringstream stream (filter_str);
		while (!stream.eof()) {
			string str;
			stream >> str;
			if (!str.empty() && str [str.size()-1] == ',')
				str.erase (str.size()-1);
			gtk_file_filter_add_pattern (filter, str.c_str());
		}
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
	}

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

YCPValue YGUI::askForExistingDirectory (const YCPString &path,
		const YCPString &title)
{
	return askForFileOrDirectory (GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, path,
	                              YCPString (""), title);
}

YCPValue YGUI::askForExistingFile (const YCPString &path,
		const YCPString &filter, const YCPString &title)
{
	return askForFileOrDirectory (GTK_FILE_CHOOSER_ACTION_OPEN, path, filter, title);
}

YCPValue YGUI::askForSaveFileName (const YCPString &path,
		const YCPString &filter, const YCPString &title)
{
	return askForFileOrDirectory (GTK_FILE_CHOOSER_ACTION_SAVE, path, filter, title);
}

bool YGUI::textMode()              IMPL_RET(false)
bool YGUI::hasImageSupport()       IMPL_RET(true)
bool YGUI::hasLocalImageSupport()  IMPL_RET(true)
bool YGUI::hasAnimationSupport()   IMPL_RET(true)
bool YGUI::hasIconSupport()        IMPL_RET(true)
bool YGUI::hasFullUtf8Support()    IMPL_RET(true)
bool YGUI::richTextSupportsTable() IMPL_RET(false)
bool YGUI::leftHandedMouse()       IMPL_RET(false)

gboolean YGUI::busy_timeout_cb (gpointer data)
{
	YGUI *pThis = (YGUI *) data;
	pThis->busyCursor();
	pThis->busy_timeout = 0;
	return FALSE;
}

void YGUI::busyCursor()
{
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
}

void YGUI::normalCursor()
{
	if (busy_timeout) {
		g_source_remove (busy_timeout);
		busy_timeout = 0;
	}

	GtkWidget *window = GTK_WIDGET (currentWindow());
	if (window)
		gdk_window_set_cursor (window->window, NULL);
}

void YGUI::redrawScreen()
{
	gtk_widget_queue_draw (GTK_WIDGET (currentWindow()));
}

YCPValue YGUI::runPkgSelection (YWidget *packageSelector)
{
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

#ifdef ENABLE_BEEP
void YGUI::beep()
{
	gdk_beep();
	GtkWindow *window = currentWindow();
	if (window)
		gtk_window_present (window);
}
#endif

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
	YCPValue ret = askForExistingFile (YCPString (DEFAULT_MACRO_FILE_NAME),
	                                   YCPString ("*.ycp"),
	                                   YCPString ("Select Macro File to Play"));

	if (ret->isString()) {
		busyCursor();
		YCPString filename = ret->asString();

		playMacro (filename->value_cstr());
		sendEvent (new YEvent());  // flush
	}
}
