/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/*
  Textdomain "yast2-gtk"
 */

#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <YEvent.h>
#include <YMacro.h>
#include <YCommandLine.h>
#include "YGUI.h"
#include "YGi18n.h"
#include "YGUtils.h"
#include "YGDialog.h"
#include <glib/gthread.h>

static std::string askForFileOrDirectory (GtkFileChooserAction action,
	const std::string &path, const std::string &filter, const std::string &title);

static void errorMsg (const char *msg)
{
	GtkWidget* dialog = gtk_message_dialog_new (NULL,
		GtkDialogFlags (0), GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", msg);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

#define DEFAULT_MACRO_FILE_NAME  "macro.ycp"
#define BUSY_CURSOR_TIMEOUT 250

YUI *createUI( bool withThreads )
{
	static YGUI *ui = 0;
	if (!ui)
		ui = new YGUI (withThreads);
	return ui;
}

YGUI::YGUI (bool with_threads)
	: YUI (with_threads), m_done_init (false), busy_timeout (0)
{
	m_have_wm = true;
	m_no_border = m_fullscreen = false;
    m_default_width = (m_default_height = 0);

    YGUI::setTextdomain( TEXTDOMAIN );

	// If we're running without threads, initialize Gtk stuff
	// This enables standalone libyui use Gtk interface
	if (!with_threads)
	    checkInit();

	// without this none of the (default) threading action works ...
	topmostConstructorHasFinished();
}

void YGUI::setTextdomain( const char * domain )
{
    bindtextdomain( domain, LOCALEDIR );
    bind_textdomain_codeset( domain, "utf8" );
    textdomain( domain );

    // Make change known.
    {
	extern int _nl_msg_cat_cntr;
	++_nl_msg_cat_cntr;
    }
}

static void print_log (const gchar *domain, GLogLevelFlags level, const gchar *message, void *pData)
{
	YUILogLevel_t ylevel = YUI_LOG_MILESTONE;
	switch (level) {
		case G_LOG_LEVEL_ERROR:
		case G_LOG_LEVEL_CRITICAL:
			ylevel = YUI_LOG_ERROR;
			break;
		case G_LOG_LEVEL_WARNING:
			ylevel = YUI_LOG_WARNING;
			break;
		case G_LOG_LEVEL_DEBUG:
			ylevel = YUI_LOG_DEBUG;
			break;
		case G_LOG_LEVEL_MESSAGE:
		case G_LOG_LEVEL_INFO:
		default:
			break;
	}
	// YUILog.cc assumes 'logComponent' (etc.) are static strings, that it
	// can just keep lying around for ever, and use later, so we have to
	// intern the domain - that can be allocated (or belong to a transient
	// plugin's address space).
	const char *component = domain ? g_intern_string (domain) : "yast2-gtk";
	YUILog::instance()->log (ylevel, component, "yast2-gtk", 0, "") << message << std::endl;
#if 0  // uncomment to put a stop to gdb
	static int bugStop = 0;
	if (bugStop-- <= 0)
		abort();
#endif
}

void YGUI::checkInit()
{
	if (m_done_init)
		return;
	m_done_init = true;

	// retrieve command line args from /proc/<pid>/cmdline
	YCommandLine cmdLine;
	int argc = cmdLine.argc();
	char **argv = cmdLine.argv();
	for (int i = 1; i < argc; i++) {
		const char *argp = argv[i];
		if (argp[0] != '-') {
			if (!strcmp (argp, "sw_single") || !strcmp (argp, "online_update"))
				m_swsingle = true;
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
		else if (!strcmp (argp, "help")) {
			printf (
				 _("Command line options for the YaST2 UI (Gtk plugin):\n\n"
				 "--no-wm       assume no window manager is running\n"
				 "--noborder    no window manager border for main dialogs\n"
				 "--fullscreen  use full screen for main dialogs\n"
//				 "--geometry WxH sets a default size of W per H to main dialogs\n"
				 "--nothreads   run without additional UI threads\n"
				 "--help        prints this help text\n"
				 "\n"
				 ));
			exit (0);
		}
	}

	gtk_init (&argc, &argv);

	g_log_set_default_handler (print_log, NULL);  // send gtk logs to libyui system
#if 0  // to crash right away in order to get a stack trace
	g_log_set_always_fatal (GLogLevelFlags (G_LOG_LEVEL_ERROR|G_LOG_LEVEL_CRITICAL|
		G_LOG_LEVEL_WARNING| G_LOG_LEVEL_MESSAGE|G_LOG_LEVEL_INFO|G_LOG_LEVEL_DEBUG));
#endif

	GdkPixbuf *pixbuf = YGUtils::loadPixbuf (THEMEDIR "/icons/32x32/apps/yast.png");
	if (pixbuf) {  // default window icon
		gtk_window_set_default_icon (pixbuf);
		g_object_unref (G_OBJECT (pixbuf));
	}
}

static gboolean ycp_wakeup_fn (GIOChannel *source, GIOCondition condition,
                               gpointer data)
{
	*(int *)data = TRUE;
	return TRUE;
}

void YGUI::idleLoop (int fd_ycp)
{
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
		g_main_context_iteration (NULL, TRUE);

	g_source_remove (watch_tag);
	g_io_channel_unref (wakeup);
}

static gboolean user_input_timeout_cb (YGUI *pThis)
{
	if (!pThis->pendingEvent())
		pThis->sendEvent (new YTimeoutEvent());
	return FALSE;
}

// utility that implements both userInput() and pollInput()
YEvent *YGUI::waitInput (unsigned long timeout_ms, bool block)
{
	checkInit();
	if (!YDialog::currentDialog (false))
		return NULL;

	if (block)
		normalCursor();  // waiting for input, so no more busy

	guint timeout = 0;

	if (timeout_ms > 0)
		timeout = g_timeout_add (timeout_ms,
			(GSourceFunc) user_input_timeout_cb, this);

	if (block) {
		while (!pendingEvent())
			g_main_context_iteration (NULL, TRUE);
	}
	else
		while (g_main_context_iteration (NULL, FALSE)) ;

	YEvent *event = NULL;
	if (pendingEvent())
		event = m_event_handler.consumePendingEvent();

	if (timeout)
		g_source_remove (timeout);

	if (block) {  // if YCP keeps working for more than X time, set busy cursor
		if (busy_timeout)
			g_source_remove (busy_timeout);
		busy_timeout = g_timeout_add (BUSY_CURSOR_TIMEOUT, busy_timeout_cb, this);
	}
	return event;
}

void YGUI::sendEvent (YEvent *event)
{
	m_event_handler.sendEvent (event);
	g_main_context_wakeup (NULL);
}

gboolean YGUI::busy_timeout_cb (gpointer data)
{
	YGUI *pThis = (YGUI *) data;
	pThis->busyCursor();
	pThis->busy_timeout = 0;
	return FALSE;
}

void YGUI::busyCursor()
{
	YGDialog *dialog = YGDialog::currentDialog();
	if (dialog)
		dialog->busyCursor();
}

void YGUI::normalCursor()
{
	if (busy_timeout) {
		g_source_remove (busy_timeout);
		busy_timeout = 0;
	}

	YGDialog *dialog = YGDialog::currentDialog();
	if (dialog)
		dialog->normalCursor();
}

void YGUI::makeScreenShot()
{ ((YGApplication *) app())->makeScreenShot (""); }

YEvent *YGUI::runPkgSelection (YWidget *packageSelector)
{
	yuiMilestone() << "Running package selection...\n";
	YEvent *event = 0;

	try {
		event = packageSelector->findDialog()->waitForEvent();
	} catch (const std::exception &e) {
		yuiError() << "UI::RunPkgSelection() error: " << e.what() << endl;
		yuiError() << "This is a libzypp problem. Do not file a bug against the UI!\n";
	} catch (...) {
		yuiError() << "UI::RunPkgSelection() error (unspecified)\n";
		yuiError() << "This is a libzypp problem. Do not file a bug against the UI!\n";
	}
	return event;
}

void YGUI::askPlayMacro()
{
	string filename = askForFileOrDirectory (GTK_FILE_CHOOSER_ACTION_OPEN,
		DEFAULT_MACRO_FILE_NAME, "*.ycp", _("Select Macro File to Play"));
	if (!filename.empty()) {
		busyCursor();
		YMacro::play (filename);
		sendEvent (new YEvent());  // flush
	}
}

void YGUI::toggleRecordMacro()
{
	if (YMacro::recording()) {
		YMacro::endRecording();
		normalCursor();

		GtkWidget* dialog = gtk_message_dialog_new (NULL,
			GtkDialogFlags (0), GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
			_("Macro recording done."));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
	else {
		string filename = askForFileOrDirectory (GTK_FILE_CHOOSER_ACTION_SAVE,
			DEFAULT_MACRO_FILE_NAME, "*.ycp", _("Select Macro File to Record to"));
		if (!filename.empty())
			YMacro::record (filename);
	}
}

void YGUI::askSaveLogs()
{
	string filename = askForFileOrDirectory (GTK_FILE_CHOOSER_ACTION_SAVE,
		"/tmp/y2logs.tgz", "*.tgz *.tar.gz", _("Save y2logs to..."));
	if (!filename.empty()) {
		std::string command = "/sbin/save_y2logs";
		command += " '" + filename + "'";
	    yuiMilestone() << "Saving y2logs: " << command << endl;
	    int ret = system (command.c_str());
		if (ret == 0)
			yuiMilestone() << "y2logs saved to " << filename << endl;
		else {
			char *error = g_strdup_printf (
				_("Error: couldn't save y2logs: \"%s\" (exit value: %d)"),
				command.c_str(), ret);
			yuiError() << error << endl;
			errorMsg (error);
			g_free (error);
		}
	}
}

//** YGApplication

#define ICONDIR THEMEDIR "/icons/22x22/apps/"

YGApplication::YGApplication()
{
	setIconBasePath (ICONDIR);
}

void YGApplication::makeScreenShot (const std::string &_filename)
{
	std::string filename (_filename);
	bool interactive = filename.empty();

	GtkWidget *widget = GTK_WIDGET (YGDialog::currentWindow());
	if (!widget) {
		if (interactive)
			errorMsg (_("No dialog to take screenshot of."));
		return;
	}

	GError *error = 0;
	GdkPixbuf *shot =
		gdk_pixbuf_get_from_drawable (NULL, GDK_DRAWABLE (widget->window),
			gdk_colormap_get_system(), 0, 0, 0, 0, widget->allocation.width,
			widget->allocation.height);

	if (!shot) {
		if (interactive)
			errorMsg (_("Couldn't take a screenshot."));
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
		const char *baseName = "yast2-";

		int nb;
		map <string, int>::iterator it = screenShotNb.find (baseName);
		if (it == screenShotNb.end())
			nb = 0;

		{
			char *tmp_name = g_strdup_printf (screenShotNameTemplate.c_str(), baseName, nb);
			filename = tmp_name;
			g_free (tmp_name);
		}
		yuiDebug() << "screenshot: " << filename << endl;

		filename = askForFileOrDirectory (
			GTK_FILE_CHOOSER_ACTION_SAVE, "", "*.png", _("Save screenshot to"));
		if (filename.empty()) {  // user dismissed the dialog
			yuiDebug() << "Save screen shot canceled by user\n";
			goto makeScreenShot_ret;
		}

		screenShotNb.erase (baseName);
		screenShotNb[baseName] = nb + 1;
	}

	yuiDebug() << "Saving screen shot to " << filename << endl;
	if (!gdk_pixbuf_save (shot, filename.c_str(), "png", &error, NULL)) {
		string msg = _("Couldn't save screenshot to file ") + filename;
		if (error) {
			msg += "\n";
			msg += std::string ("\n") + error->message;
		}
		yuiError() << msg << endl;
		if (interactive)
			errorMsg (msg.c_str());
		goto makeScreenShot_ret;
	}

	makeScreenShot_ret:
		g_object_unref (G_OBJECT (shot));
}

void YGApplication::beep()
{
	GtkWindow *window = YGDialog::currentWindow();
	if (window) {
		gtk_window_present (window);
		gtk_widget_error_bell (GTK_WIDGET (window));
	}
	else
		gdk_beep();
}

// File/directory dialogs
#include <sstream>

std::string askForFileOrDirectory (GtkFileChooserAction action,
	const std::string &path, const std::string &filter, const std::string &title)
{
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new (title.c_str(),
		YGDialog::currentWindow(), action, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		action == GTK_FILE_CHOOSER_ACTION_SAVE ? GTK_STOCK_SAVE : GTK_STOCK_OPEN,
		GTK_RESPONSE_ACCEPT, NULL);
	GtkFileChooser *fileChooser = GTK_FILE_CHOOSER (dialog);
	gtk_file_chooser_set_local_only (fileChooser, TRUE);
	gtk_file_chooser_set_do_overwrite_confirmation (fileChooser, TRUE);

	// filepath can be a dir or a file path, split that up
	string dirname, filename;
	if (!path.empty()) {
		if (path[0] != '/')
			yuiWarning() << "FileDialog: Relative paths are not supported: '" << path << "'\n";
		else if (!g_file_test (path.c_str(), G_FILE_TEST_EXISTS))
			yuiWarning() << "FileDialog: Path doesn't exist: '" << path << "'\n";
		else if (g_file_test (path.c_str(), G_FILE_TEST_IS_DIR))
			dirname = path;
		else {  // its a file
			string::size_type i = path.find_last_of ("/");
			if (i != string::npos) {
				dirname = path.substr (0, i+1);
				filename = path.substr (i+1);
			}
		}
	}

	if (!dirname.empty())
		gtk_file_chooser_set_current_folder (fileChooser, dirname.c_str());
	if (!filename.empty())
		gtk_file_chooser_set_current_name (fileChooser, filename.c_str());

	if (!filter.empty() && filter != "*") {
		GtkFileFilter *gtk_filter = gtk_file_filter_new();
		gtk_file_filter_set_name (gtk_filter, filter.c_str());
		// cut filter_pattern into chuncks like GTK likes
		std::istringstream stream (filter);
		while (!stream.eof()) {
			string str;
			stream >> str;
			if (!str.empty() && str [str.size()-1] == ',')
				str.erase (str.size()-1);
			gtk_file_filter_add_pattern (gtk_filter, str.c_str());
		}
		gtk_file_chooser_add_filter (fileChooser, gtk_filter);
	}

	// bug 335492: because "/" gets hidden as a an arrow at the top, make sure
	// there is a root shortcut at the side pane (will not add if already exists)
	gtk_file_chooser_add_shortcut_folder (fileChooser, "/", NULL);

	std::string ret;
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *filename = gtk_file_chooser_get_filename (fileChooser);
		ret = filename;
		g_free (filename);
	}
	gtk_widget_destroy (dialog);
	return ret;
}

std::string YGApplication::askForExistingDirectory (
	const std::string &path, const std::string &title)
{ return askForFileOrDirectory (GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, path, "", title); }

std::string YGApplication::askForExistingFile (
	const std::string &path, const std::string &filter, const std::string &title)
{ return askForFileOrDirectory (GTK_FILE_CHOOSER_ACTION_OPEN, path, filter, title); }

std::string YGApplication::askForSaveFileName (
	const std::string &path, const std::string &filter, const std::string &title)
{ return askForFileOrDirectory (GTK_FILE_CHOOSER_ACTION_SAVE, path, filter, title); }

std::string YGApplication::glyph (const std::string &sym)
{
	bool reverse = gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL;
	if (sym == YUIGlyph_ArrowLeft)
		return reverse ? "\u25b6" : "\u25c0";
	if (sym == YUIGlyph_ArrowRight)
		return reverse ? "\u25c0" : "\u25b6";
	if (sym == YUIGlyph_ArrowUp)
		return "\u25b2";
	if (sym == YUIGlyph_ArrowDown)
		return "\u25bc";
	if (sym == YUIGlyph_CheckMark)
		return "\u2714";
	if (sym == YUIGlyph_BulletArrowRight)
		return reverse ? "\u21e6" : "\u279c";
	if (sym == YUIGlyph_BulletCircle)
		return "\u26ab";
	if (sym == YUIGlyph_BulletSquare)
		return "\u25fe";
	return "";
}

// YWidget layout units -> pixels conversion. Same as yast-qt's.
int YGApplication::deviceUnits (YUIDimension dim, float size)
{
	if (dim == YD_HORIZ)
		size *= 640.0 / 80;
	else
		size *= 480.0 / 25;
	return size + 0.5;
}

float YGApplication::layoutUnits (YUIDimension dim, int units)
{
	float size = (float) units;
	if (dim == YD_HORIZ) return size * (80/640.0);
	else                 return size * (25/480.0);
}

static inline GdkScreen *getScreen ()
{ return gdk_display_get_default_screen (gdk_display_get_default()); }
// GTK doesn't seem to have some desktopWidth/Height like Qt, so we to report
// a reduced display size to compensate for the panel, or the window frame
int YGApplication::displayWidth()
{ return gdk_screen_get_width (getScreen()) - 80; }
int YGApplication::displayHeight()
{ return gdk_screen_get_height (getScreen()) - 80; }

int YGApplication::displayDepth()
{ return gdk_visual_get_best_depth(); }

long YGApplication::displayColors()
{ return 1L << displayDepth(); /*from yast-qt*/ }

// YCP uses defaultWidth/Height() to use their smaller YWizard impl; we
// want to use a smaller default size than qt though, so assume a bigger size
int YGApplication::defaultWidth() { return MIN (displayWidth(), 1024); }
int YGApplication::defaultHeight() { return MIN (displayHeight(), 768); }

YWidgetFactory *YGUI::createWidgetFactory()
{ return new YGWidgetFactory; }
YOptionalWidgetFactory *YGUI::createOptionalWidgetFactory()
{ return new YGOptionalWidgetFactory; }
YApplication *YGUI::createApplication()
{ return new YGApplication(); }

#include <YRichText.h>
#include "ygtktextview.h"

void dumpYastHtml (YWidget *widget)
{
	struct inner {
		static void dumpYastHtml (YWidget *widget, GtkBox *box)
		{
			YGWidget *ygwidget;
			if (!widget || !(ygwidget = YGWidget::get (widget)))
				return;

			YRichText *rtext = dynamic_cast <YRichText *> (widget);
			if (rtext) {
				std::string text = rtext->text();

				GtkWidget *view = ygtk_text_view_new (FALSE);
				gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_WORD);
				GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
				gtk_text_buffer_set_text (buffer, text.c_str(), -1);

				GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
				gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
							                         GTK_SHADOW_IN);
				gtk_scrolled_window_set_policy  (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
				gtk_container_add (GTK_CONTAINER (scroll), view);
				gtk_box_pack_start (box, scroll, TRUE, TRUE, 6);
			}

			for (YWidgetListConstIterator it = widget->childrenBegin();
			     it != widget->childrenEnd(); it++)
				dumpYastHtml (*it, box);
		}
		static void destroy_dialog (GtkDialog *dialog, gint arg)
		{ gtk_widget_destroy (GTK_WIDGET (dialog)); }
	};

	GtkWidget *dialog = gtk_dialog_new_with_buttons ("YWidgets HTML", NULL,
		GtkDialogFlags (GTK_DIALOG_NO_SEPARATOR), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 400, 300);

	inner::dumpYastHtml (widget, GTK_BOX (GTK_DIALOG (dialog)->vbox));

	gtk_widget_show_all (dialog);
	g_signal_connect (G_OBJECT (dialog), "response",
	                  G_CALLBACK (inner::destroy_dialog), 0);
}

