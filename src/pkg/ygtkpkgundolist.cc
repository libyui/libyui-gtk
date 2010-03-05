/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* Textdomain "yast2-gtk" */
/* YGtkPkgUndoList, list of changes */
// check the header file for information about this utility

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
				char *str = g_strdup_printf (
					_("%sPartition %s is %s over-capacity (%s filled out of %s).%s"),
					"<b><span color=\"red\">",
					point.dir.c_str(), point.freeAfterCommit().asString().c_str(),
					point.usedAfterCommit().asString().c_str(),
					point.totalSize().asString().c_str(), "</span></b>");
				gtk_label_set_markup (GTK_LABEL (warn_label), str);
				g_free (str);
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

#define PATH_TO_YAST_SYSCONFIG  "/etc/sysconfig/yast2"
#include <zypp/base/Sysconfig.h>

static bool read_PKGMGR_ACTION_AT_EXIT()
{ // from yast2-ncurses, NCPackageSelector.cc
	std::map <std::string, std::string> sysconfig =
		zypp::base::sysconfig::read( PATH_TO_YAST_SYSCONFIG );
	std::map <std::string, std::string>::const_iterator it =
		sysconfig.find("PKGMGR_ACTION_AT_EXIT");
	if (it != sysconfig.end()) {
		yuiMilestone() << "Read sysconfig's action at pkg mgr exit value: " << it->second << endl;
		return it->second == "restart";
	}
	yuiMilestone() << "Could not read PKGMGR_ACTION_AT_EXIT variable from sysconfig" << endl;
	return false;
}

static void write_PKGMGR_ACTION_AT_EXIT (bool restart)
{ // from yast2-ncurses, NCPackageSelector.cc
	// this is really, really stupid. But we have no other iface for writing sysconfig so far
	std::string actionAtExit (restart ? "restart" : "close");
	int ret = -1;
	string cmd = "sed -i 's/^[ \t]*PKGMGR_ACTION_AT_EXIT.*$/PKGMGR_ACTION_AT_EXIT=\"" +
		actionAtExit + "\"/' " + PATH_TO_YAST_SYSCONFIG;
	ret  = system(cmd.c_str());
	yuiMilestone() << "Executing system cmd " << cmd << " returned " << ret << endl;
}

static void close_when_done_toggled_cb (GtkToggleButton *button)
{ write_PKGMGR_ACTION_AT_EXIT (!gtk_toggle_button_get_active (button)); }

static GtkWidget *create_close_when_done_check()
{
	GtkWidget *check_box = gtk_check_button_new_with_label (_("Close window when done"));
	bool active = !read_PKGMGR_ACTION_AT_EXIT();
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_box), active);
	g_signal_connect (G_OBJECT (check_box), "toggled",
	                  G_CALLBACK (close_when_done_toggled_cb), NULL);
	gtk_widget_show (check_box);
	return check_box;
}

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
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_YES, NULL);
	}
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
	gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 600, 500);

	YGtkPkgListView view (true, -1, true, false);
	view.addImageColumn (NULL, STATUS_ICON_PROP);
	view.addTextColumn (_("Name"), NAME_SUMMARY_PROP, true, -1);
	view.addTextColumn (_("Version"), VERSION_PROP, true, 125);
	view.addUndoButtonColumn (_("Revert?"));
	view.setList (impl->changes);

	ChangeSizeInfo change_size;

	GtkWidget *vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), view.getWidget(), TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), change_size.getWidget(), FALSE, TRUE, 0);
	if (onApply)
		gtk_box_pack_start (GTK_BOX (vbox), create_close_when_done_check(), FALSE, TRUE, 0);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);

	int ret = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	return ret == GTK_RESPONSE_YES;
}

