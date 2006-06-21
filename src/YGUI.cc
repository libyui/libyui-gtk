#include <config.h>
#include <stdio.h>
#include <YEvent.h>
#include <YGUI.h>
#include <YGWidget.h>

YGUI::YGUI( int argc, char ** argv,
			bool with_threads, const char *macro_file ) :
	YUI(with_threads)
{
	IMPL;
	gtk_init (&argc, &argv);
	fprintf (stderr, "I'm initialized '%s' - come & get me !\n",
		with_threads ? "with threads !" : "no threads");

	// without this none of the (default) threading action works ...
		topmostConstructorHasFinished();
}

YGUI::~YGUI() IMPL;

static gboolean
ycp_wakeup_fn (GIOChannel   *source,
				GIOCondition  condition,
				gpointer      data)
{
	*(int *)data = TRUE;
	return TRUE;
}

void
YGUI::idleLoop (int fd_ycp)
{
	IMPL;

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

// Widgets Proper

YWidget *
YGUI::createPackageSelector( YWidget *parent, YWidgetOpt & opt,
							const YCPString & floppyDevice ) IMPL_NULL;

YWidget *
YGUI::createPkgSpecial( YWidget *parent, YWidgetOpt & opt,
							const YCPString & subwidget ) IMPL_NULL;

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

/* File/directory dialogs. */
static YCPValue askForFileOrDirectory (GtkFileChooserAction action,
		const YCPString &startWith, const YCPString &filter_pattern,
		const YCPString &headline)
{
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new (headline->value_cstr(),
		NULL /* TODO: set GtkWindow parent*/,
		action,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN,   GTK_RESPONSE_ACCEPT,
		NULL);
	// FIXME: startWith is not necessarly a folder, it can be a file!
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), startWith->value_cstr());

	if (action != GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER) {
		GtkFileFilter *filter = gtk_file_filter_new();
		gtk_file_filter_add_pattern (filter, filter_pattern->value_cstr());
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

bool YGUI::textMode()		   IMPL_RET(false);
bool YGUI::hasImageSupport()       IMPL_RET(true);
bool YGUI::hasLocalImageSupport()  IMPL_RET(true);
bool YGUI::hasAnimationSupport()   IMPL_RET(true);
bool YGUI::hasIconSupport()        IMPL_RET(true);
bool YGUI::hasFullUtf8Support()    IMPL_RET(true);
bool YGUI::richTextSupportsTable() IMPL_RET(false);
bool YGUI::leftHandedMouse()       IMPL_RET(false);

void YGUI::busyCursor() IMPL;
void YGUI::normalCursor() IMPL;
void YGUI::redrawScreen() IMPL;
void YGUI::makeScreenShot (string filename) IMPL;
