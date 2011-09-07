/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* YGtkPkgVestigialDialog, dialog */
// check the header file for information about this widget

/*
  Textdomain "gtk"
 */

#include "YGi18n.h"
#include "config.h"
#include "YGDialog.h"
#include "ygtkpkgvestigialdialog.h"
#include "ygtkpkglistview.h"
#include "YGPackageSelector.h"
#include "yzyppwrapper.h"
#include <gtk/gtk.h>

#include <zypp/parser/HistoryLogReader.h>
#define FILENAME "/var/log/zypp/history"

struct ZyppVestigialParser
{
	std::set <std::string> m_dependencies;

	ZyppVestigialParser()
	{
		zypp::parser::HistoryLogReader parser (FILENAME, boost::ref (*this));
		try {
			parser.readAll();
		}
		catch (const zypp::Exception &ex) {
			yuiWarning () << "Error: Could not load log file" << FILENAME << ": "
				<< ex.asUserHistory() << std::endl;
		}
	}

	bool operator() (const zypp::HistoryItem::Ptr &item)
	{
		if (item->action.toEnum() == zypp::HistoryActionID::INSTALL_e) {
			zypp::HistoryItemInstall *_item =
				static_cast <zypp::HistoryItemInstall *> (item.get());

			const std::string &name (_item->name);
			bool autoreq = _item->reqby.empty();
			std::string repoName, repoUrl;
			Ypp::getRepositoryFromAlias (_item->repoalias, repoName, repoUrl);
			bool update = repoUrl.find ("update") != std::string::npos;

			if (!update) {  // updates are ambiguous
				if (autoreq)
					m_dependencies.insert (name);
				else
					m_dependencies.erase (name);
			}
		}

		return true;
	}
};

struct YGtkPkgVestigialDialog::Impl : public YGtkPkgListView::Listener
{
	GtkWidget *dialog;
	YGtkPkgListView *view;
	GtkWidget *progressbar;

	virtual void selectionChanged()
	{
		Ypp::List selected (view->getSelected());
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), 1, selected.size());
	}

	void goto_clicked()
	{
		Ypp::List selected (view->getSelected());
		if (selected.size()) {
			std::string name (selected.get(0).name());
			YGPackageSelector::get()->searchFor (Ypp::PoolQuery::NAME, name);
		}
		gtk_widget_hide (dialog);
	}

	void remove_all()
	{
		view->getList().remove();
	}

	static void response_cb (GtkDialog *dialog, gint response, Impl *pThis)
	{
		switch (response) {
			case 1: pThis->goto_clicked(); break;
			case 2: pThis->remove_all(); break;
			default: gtk_widget_hide (GTK_WIDGET (pThis->dialog)); break;
		}
	}

	void set_progress (gdouble fraction)
	{
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progressbar), fraction);
		while (g_main_context_iteration (NULL, FALSE)) ;
	}
};

static bool testIsNeededDependency (ZyppSelectable zsel)
{  // pass only installed packages: zsel->status() == S_KeepInstalled
	zsel->setStatus (zypp::ui::S_Del);
	zypp::Resolver_Ptr zResolver = zypp::getZYpp()->resolver();
	bool noProbs = zResolver->resolvePool();
	zResolver->undo();
	zsel->setStatus (zypp::ui::S_KeepInstalled);
	return !noProbs;
}

static void filterWhenContains (Ypp::Collection &col, std::list <ZyppSelectable> &pkgs)
{
	std::list <ZyppSelectable>::iterator it = pkgs.begin();
	while (it != pkgs.end()) {
		Ypp::Selectable p (*it);
		if (col.contains (p))
			it = pkgs.erase (it);
		else it++;
	}
}

static gboolean fill_list_idle_cb (void *data)
{
	YGtkPkgVestigialDialog::Impl *impl = (YGtkPkgVestigialDialog::Impl *) data;

	// 1. get list of packages installed as dependencies
	ZyppVestigialParser parser;
	const std::set <std::string> &deps = parser.m_dependencies;
	impl->set_progress (.1);
	// 2. map string names to zypp objects and check whether they are still necessary
	int size = deps.size(), i = 0;
	std::list <ZyppSelectable> unneeded;
	for (std::set <std::string>::const_iterator it = deps.begin();
	     it != deps.end(); it++) {
		Ypp::PoolQuery query (Ypp::Selectable::PACKAGE);
		query.addStringOr (*it);
		query.addStringAttribute (Ypp::PoolQuery::NAME);
		query.setStringMode (true, Ypp::PoolQuery::EXACT);
		if (query.hasNext()) {
			Ypp::Selectable sel (query.next());
			ZyppSelectable zsel = sel.zyppSel();
			if (zsel->status() == zypp::ui::S_KeepInstalled)
				if (!testIsNeededDependency (zsel))
					unneeded.push_back (zsel);
		}

		if ((i % 2) == 0)
			impl->set_progress (.1 + ((i / (gdouble) size) * .80));
		i++;
	}
	// 3. filter those installed by a container
	impl->set_progress (.90);
	Ypp::LangQuery langQuery;
	size = langQuery.guessSize(); i = 0;
	while (langQuery.hasNext()) {
		Ypp::Selectable sel (langQuery.next());
		if (!sel.isInstalled()) continue;
		Ypp::Collection col (sel);
		filterWhenContains (col, unneeded);
	}
	impl->set_progress (.95);
	Ypp::PoolQuery patternQuery (Ypp::Selectable::PATTERN);
	patternQuery.addCriteria (new Ypp::StatusMatch (Ypp::StatusMatch::IS_INSTALLED));
	size = patternQuery.guessSize(); i = 0;
	while (patternQuery.hasNext()) {
		Ypp::Selectable sel (patternQuery.next());
		Ypp::Collection col (sel);
		filterWhenContains (col, unneeded);
	}

	Ypp::List list (unneeded.size());
	for (std::list <ZyppSelectable>::const_iterator it = unneeded.begin();
	     it != unneeded.end(); it++)
		list.append (*it);
	impl->view->setList (list);

	gtk_widget_hide (impl->progressbar);
	gdk_window_set_cursor (impl->dialog->window, NULL);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (impl->dialog), 2, unneeded.size());
	gtk_window_present (GTK_WINDOW (impl->dialog));
	return FALSE;
}

YGtkPkgVestigialDialog::YGtkPkgVestigialDialog()
{
	GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
		GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_OTHER, GTK_BUTTONS_NONE,
		_("Show Unneeded Dependencies"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
		_("This is a listing of dependencies no longer used. It is neither "
		"accurate, nor comprehensive. Use with care."));

	impl = new Impl();
	GtkWidget *action_area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));
	impl->progressbar = gtk_progress_bar_new();
	gtk_widget_set_size_request (impl->progressbar, 0, 0);
	gtk_container_add (GTK_CONTAINER (action_area), impl->progressbar);

	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_JUMP_TO, 1);
	gtk_dialog_add_button (GTK_DIALOG (dialog), _("Remove All"), 2);
	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), 1, FALSE);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), 2, FALSE);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
	gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 650, 600);
	g_signal_connect (G_OBJECT (dialog), "response",
	                  G_CALLBACK (Impl::response_cb), impl);
	g_signal_connect (G_OBJECT (dialog), "delete-event",
	                  G_CALLBACK (gtk_true), NULL);

	impl->view = new YGtkPkgListView (true, Ypp::List::NAME_SORT, false, true);
	impl->view->addCheckColumn (INSTALLED_CHECK_PROP);
	impl->view->addTextColumn (_("Name"), NAME_SUMMARY_PROP, true, -1);
	impl->view->addTextColumn (_("Version"), VERSION_PROP, true, 125);
	impl->view->addTextColumn (_("Size"), SIZE_PROP, false, 85);
	impl->view->addTextColumn (_("Repository"), REPOSITORY_PROP, false, 180);
	impl->view->addTextColumn (_("Supportability"), SUPPORT_PROP, false, 120);
	impl->view->setListener (impl);

	GtkBox *content = GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG(dialog)));
	gtk_box_pack_start (content, impl->view->getWidget(), TRUE, TRUE, 6);
	gtk_widget_show_all (dialog);
	impl->dialog = dialog;

	GdkCursor *cursor = gdk_cursor_new (GDK_WATCH);
	gdk_window_set_cursor (dialog->window, cursor);
	gdk_cursor_unref (cursor);

	g_idle_add_full (G_PRIORITY_LOW, fill_list_idle_cb, impl, NULL);
}

YGtkPkgVestigialDialog::~YGtkPkgVestigialDialog()
{
	delete impl->view;
	gtk_widget_destroy (impl->dialog);
	delete impl;
}

void YGtkPkgVestigialDialog::popup()
{ gtk_window_present (GTK_WINDOW (impl->dialog)); }

