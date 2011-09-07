/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* YGtkPkgUndoList, list of changes */
// check the header file for information about this utility

/*
  Textdomain "gtk"
 */

#include "YGi18n.h"
#include "config.h"
#include "YGDialog.h"
#include "YGUtils.h"
#include "ygtkpkgundolist.h"
#include <gtk/gtk.h>

struct YGtkPkgUndoList::Impl : public Ypp::SelListener
{
	YGtkPkgUndoList *parent;
	Ypp::List changes;
	std::list <YGtkPkgUndoList::Listener *> listeners;

	Impl (YGtkPkgUndoList *parent) : parent (parent), changes (0)
	{ Ypp::addSelListener (this); }

	~Impl()
	{ Ypp::removeSelListener (this); }

	virtual void selectableModified()
	{
		// remove any previously changed selectable now undone
		Ypp::List _changes (changes.size());
		for (int i = 0; i < changes.size(); i++) {
			Ypp::Selectable &sel = changes.get(i);
			if (sel.toModify())
				_changes.append (sel);
		}
		changes = _changes;

		// check for any new changed selectable
		// first, check for user-modified, then for auto ones, to keep them ordered
		for (int step = 0; step < 2; step++) {
			Ypp::PoolQuery query (Ypp::Selectable::ALL);
			query.addCriteria (new Ypp::StatusMatch (Ypp::StatusMatch::TO_MODIFY));
			while (query.hasNext()) {
				Ypp::Selectable sel = query.next();
				if ((step == 0 && sel.toModifyAuto()) || (step == 1 && !sel.toModifyAuto()))
					continue;
				if (changes.find (sel) == -1)
					changes.append (sel);
			}
		}

		for (std::list <YGtkPkgUndoList::Listener *>::iterator it = listeners.begin();
		     it != listeners.end(); it++)
			(*it)->undoChanged (parent);
	}
};

YGtkPkgUndoList::YGtkPkgUndoList()
: impl (new Impl (this)) {}

YGtkPkgUndoList::~YGtkPkgUndoList()
{ delete impl; }

Ypp::Selectable *YGtkPkgUndoList::front (int *autoCount)
{
	Ypp::Selectable *sel = 0;
	if (autoCount)
		*autoCount = 0;
	for (int i = impl->changes.size()-1; i >= 0; i--) {
		sel = &impl->changes.get (i);
		if (sel->toModifyAuto()) {
			if (autoCount) (*autoCount)++;
		}
		else
			break;
	}
	return sel;
}

Ypp::List YGtkPkgUndoList::getList()
{ return impl->changes; }

void YGtkPkgUndoList::addListener (Listener *listener)
{ impl->listeners.push_back (listener); }

void YGtkPkgUndoList::removeListener (Listener *listener)
{ impl->listeners.remove (listener); }

#include "YGPackageSelector.h"
#include "ygtkpkglistview.h"

struct ChangeSizeInfo : public YGtkPkgUndoList::Listener {
	GtkWidget *vbox, *disk_label, *download_label, *warn_label;

	GtkWidget *getWidget()
	{ return vbox; }

	ChangeSizeInfo()
	{
		GtkWidget *hbox = gtk_hbox_new (FALSE, 6);
		GtkWidget *line;
		line = createSizeWidget (_("Disk space required:"), &disk_label);
		gtk_box_pack_start (GTK_BOX (hbox), line, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new ("/"), FALSE, TRUE, 0);
		line = createSizeWidget (_("Download size:"), &download_label);
		gtk_box_pack_start (GTK_BOX (hbox), line, FALSE, TRUE, 0);
		gtk_widget_show_all (hbox);

		warn_label = gtk_label_new ("");
		gtk_misc_set_alignment (GTK_MISC (warn_label), 0, .5);
		gtk_label_set_selectable (GTK_LABEL (warn_label), TRUE);

		vbox = gtk_vbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), warn_label, FALSE, TRUE, 0);
		gtk_widget_show (vbox);

		undoChanged (YGPackageSelector::get()->undoList());
		YGPackageSelector::get()->undoList()->addListener (this);
	}

	~ChangeSizeInfo()
	{ YGPackageSelector::get()->undoList()->removeListener (this); }

	virtual void undoChanged (YGtkPkgUndoList *undo)
	{
		Size_t disk, download;
		Ypp::List list = undo->getList();
		for (int i = 0; i < list.size(); i++) {
			Ypp::Selectable sel = list.get (i);
			if (sel.toInstall()) {
				Ypp::Version candidate = sel.candidate();
				download += candidate.downloadSize();
				if (sel.isInstalled())
					disk += candidate.size() - sel.installed().size();
				else
					disk += candidate.size();
			}
			else if (sel.toRemove())
				disk -= sel.installed().size();
		}

		gtk_label_set_text (GTK_LABEL (disk_label), disk.asString().c_str());
		gtk_label_set_text (GTK_LABEL (download_label), download.asString().c_str());

		gtk_widget_hide (warn_label);
		ZyppDuSet diskUsage = zypp::getZYpp()->diskUsage();
		for (ZyppDuSet::iterator it = diskUsage.begin(); it != diskUsage.end(); it++) {
			const ZyppDu &point = *it;
			if (!point.readonly && point.freeAfterCommit() < 0) {
				// Translators: please keep the %s order intact. They refer to mount_point, needed_space
				char *over = g_strdup_printf (_("Partition %s is %s over-capacity"),
					point.dir.c_str(), point.freeAfterCommit().asString().c_str());
				// Translators: please keep the %s order intact. They refer to used_space, total_space
				char *fill = g_strdup_printf (_("%s filled out of %s"),
					point.usedAfterCommit().asString().c_str(),
					point.totalSize().asString().c_str());
				char *markup = g_strdup_printf (
					"<b><span color=\"red\">%s (%s)</span></b>", over, fill);
				gtk_label_set_markup (GTK_LABEL (warn_label), markup);
				g_free (over); g_free (fill); g_free (markup);
				gtk_widget_show (warn_label);
				break;
			}
		}
	}

	static GtkWidget *createSizeWidget (const char *label, GtkWidget **widget)
	{
		*widget = gtk_label_new ("");
		YGUtils::setWidgetFont (*widget, PANGO_STYLE_ITALIC,
			PANGO_WEIGHT_NORMAL, PANGO_SCALE_MEDIUM);
		gtk_misc_set_alignment (GTK_MISC (*widget), 1, .5);
		gtk_label_set_selectable (GTK_LABEL (*widget), TRUE);

		GtkWidget *title = gtk_label_new (label);
		gtk_misc_set_alignment (GTK_MISC (title), 0, .5);

		GtkWidget *hbox = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (hbox), title, FALSE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), *widget, TRUE, TRUE, 0);
		return hbox;
	}
};

#define YAST_SYSCONFIG "/etc/sysconfig/yast2"
#include <zypp/base/Sysconfig.h>

enum { CLOSE_MODE, RESTART_MODE, SUMMARY_MODE };

static int read_PKGMGR_ACTION_AT_EXIT()
{ // from yast2-ncurses, NCPackageSelector.cc
	std::map <std::string, std::string> sysconfig =
		zypp::base::sysconfig::read (YAST_SYSCONFIG);
	std::map <std::string, std::string>::const_iterator it =
		sysconfig.find("PKGMGR_ACTION_AT_EXIT");
	if (it != sysconfig.end()) {
		yuiMilestone() << "Read sysconfig's action at pkg mgr exit value: " << it->second << endl;
		std::string mode (it->second);
		if (mode == "restart")
			return RESTART_MODE;
		else if (mode == "summary")
			return SUMMARY_MODE;
		else //if (mode == "close")
			return CLOSE_MODE;
	}
	yuiMilestone() << "Could not read PKGMGR_ACTION_AT_EXIT variable from sysconfig" << endl;
	return false;
}

static void write_PKGMGR_ACTION_AT_EXIT (int mode)
{ // from yast2-ncurses, NCPackageSelector.cc
	// this is really, really stupid. But we have no other iface for writing sysconfig so far
	std::string _mode;
	switch (mode) {
		case CLOSE_MODE: _mode = "close"; break;
		case RESTART_MODE: _mode = "restart"; break;
		case SUMMARY_MODE: _mode = "summary"; break;
	}
	int ret = -1;
	string cmd = "sed -i 's/^[ \t]*PKGMGR_ACTION_AT_EXIT.*$/PKGMGR_ACTION_AT_EXIT=\"" +
		_mode + "\"/' " + YAST_SYSCONFIG;
	ret  = system(cmd.c_str());
	yuiMilestone() << "Executing system cmd " << cmd << " returned " << ret << endl;
}

static void close_when_done_toggled_cb (GtkToggleButton *button)
{
	gtk_toggle_button_set_inconsistent (button, FALSE);
	int mode = gtk_toggle_button_get_active (button) ? CLOSE_MODE : RESTART_MODE;
	write_PKGMGR_ACTION_AT_EXIT (mode);
}

static GtkWidget *create_close_when_done_check()
{
	GtkWidget *check_box = gtk_check_button_new_with_label (_("Close software manager when done"));
	int mode = read_PKGMGR_ACTION_AT_EXIT();
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_box), mode == CLOSE_MODE);
	gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (check_box), mode == SUMMARY_MODE);
	if (access (YAST_SYSCONFIG, W_OK) != 0) {
		gtk_widget_set_sensitive (check_box, FALSE);
		gtk_widget_set_tooltip_text (check_box, "Cannot write to " YAST_SYSCONFIG);
	}
	g_signal_connect_after (G_OBJECT (check_box), "toggled",
	                        G_CALLBACK (close_when_done_toggled_cb), NULL);
	gtk_widget_show (check_box);
	return check_box;
}

struct YGtkPkgUndoView : public YGtkPkgListView, YGtkPkgUndoList::Listener
{
	YGtkPkgUndoView()
	: YGtkPkgListView (true, -1, true, false, true)
	{
		addImageColumn (NULL, STATUS_ICON_PROP, true);
		addTextColumn (_("Name"), ACTION_NAME_PROP, true, -1);
		addTextColumn (_("Version"), SINGLE_VERSION_PROP, true, 125);
		addButtonColumn (_("Revert?"), UNDO_BUTTON_PROP);

		undoChanged (YGPackageSelector::get()->undoList());
		YGPackageSelector::get()->undoList()->addListener (this);
	}

	~YGtkPkgUndoView()
	{ YGPackageSelector::get()->undoList()->removeListener (this); }

	virtual void undoChanged (YGtkPkgUndoList *undo)
	{ setList (undo->getList()); }
};

#include "ygtkpkghistorydialog.h"

bool YGtkPkgUndoList::popupDialog (bool onApply)
{
	GtkMessageType type = onApply ? GTK_MESSAGE_QUESTION : GTK_MESSAGE_OTHER;
	GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
		GtkDialogFlags (0), type, GTK_BUTTONS_NONE, _("Summary of changes"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
		_("Review the changes to perform."));
	if (onApply)
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
			GTK_STOCK_APPLY, GTK_RESPONSE_YES, NULL);
	else {
		GtkWidget *image = gtk_image_new_from_stock (GTK_STOCK_UNDO, GTK_ICON_SIZE_DIALOG);
		gtk_widget_show (image);
		gtk_message_dialog_set_image (GTK_MESSAGE_DIALOG (dialog), image);
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), _("See History"), 1,
			GTK_STOCK_CLOSE, GTK_RESPONSE_YES, NULL);

		// work-around for GTK bug: when you set a custom icon, it is packed as expanded
		GtkWidget *hbox = gtk_widget_get_parent (image);
		gtk_box_set_child_packing (GTK_BOX (hbox), image, FALSE, TRUE, 0, GTK_PACK_START);
	}
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
	gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 600, 500);

	YGtkPkgUndoView view;
	ChangeSizeInfo change_size;

	GtkWidget *vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), view.getWidget(), TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), change_size.getWidget(), FALSE, TRUE, 0);
	if (onApply)
		gtk_box_pack_start (GTK_BOX (vbox), create_close_when_done_check(), FALSE, TRUE, 0);
	gtk_widget_show (vbox);
	gtk_widget_set_vexpand (vbox, TRUE);
	GtkContainer *content = GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog)));
	gtk_container_add (content, vbox);

	int ret = gtk_dialog_run (GTK_DIALOG (dialog));
	if (ret == 1)
		YGPackageSelector::get()->showHistoryDialog();
	gtk_widget_destroy (dialog);
	return ret == GTK_RESPONSE_YES;
}

