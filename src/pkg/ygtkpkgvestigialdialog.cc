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
#include "yzyppwrapper.h"
#include <gtk/gtk.h>

#include <zypp/parser/HistoryLogReader.h>
#define FILENAME "/var/log/zypp/history"

static void response_cb (GtkDialog *dialog, gint response, YGtkPkgVestigialDialog *pThis)
{
	switch (response) {
/*		case 1: goto_clicked (log_view); break;
		case 2: save_to_file (GTK_WINDOW (dialog)); break;*/
		default: gtk_widget_hide (GTK_WIDGET (dialog)); break;
	}
}

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
fprintf (stderr, "pkg '%s'\n", _item->name.c_str());

			const std::string &name (_item->name);
			bool autoreq = _item->reqby.empty();
			std::string repoName, repoUrl;
			Ypp::getRepositoryFromAlias (_item->repoalias, repoName, repoUrl);
			bool update = repoUrl.find ("update") != std::string::npos;

			if (!update) {  // updates are ambiguous
				fprintf (stderr, "\trepo: '%s' - autoreq: %d\n", _item->repoalias.c_str(), autoreq);
				std::set <std::string>::iterator it;
				it = m_dependencies.find (name);
				if (it != m_dependencies.end()) {
					if (autoreq)
						m_dependencies.insert (name);
				}
				else {
					if (!autoreq)
						m_dependencies.erase (it);
				}
			}
		}
else
fprintf (stderr, "action: '%d'\n", item->action);

		return true;
	}
};

static bool testIsNeededDependency (ZyppSelectable zsel)
{  // pass only installed packages: zsel->status() == S_KeepInstalled
	fprintf (stderr, "test is needed: %s\n", zsel->name().c_str());
	zsel->setStatus (zypp::ui::S_Del);
	zypp::Resolver_Ptr zResolver = zypp::getZYpp()->resolver();
	bool noProbs = zResolver->resolvePool();
	zResolver->undo();
	zsel->setStatus (zypp::ui::S_KeepInstalled);
	fprintf (stderr, "\t%d\n", !noProbs);
	return !noProbs;
}

YGtkPkgVestigialDialog::YGtkPkgVestigialDialog()
//: m_unneeded (Ypp::List (zyppPool().byKindBegin(zypp::ResKind::package).size()))
{
	ZyppVestigialParser parser;
	fprintf (stderr, "done parsing\n");
	for (std::set <std::string>::const_iterator it = parser.m_dependencies.begin();
	     it != parser.m_dependencies.end(); it++) {
		Ypp::PoolQuery query (Ypp::Selectable::PACKAGE);
		query.addStringOr (*it);
		query.addStringAttribute (Ypp::PoolQuery::NAME);
		query.setStringMode (true, Ypp::PoolQuery::EXACT);
		if (query.hasNext()) {
			Ypp::Selectable sel (query.next());
			ZyppSelectable zsel = sel.zyppSel();
			if (zsel->status() == zypp::ui::S_KeepInstalled)
				if (!testIsNeededDependency (zsel))
					fprintf (stderr, "** not needed: %s\n", zsel->name().c_str());
		}
	}



	// for each log entry set as 'auto'
		// find pkg
		// remove pkg
		// run solver
		// if no problems:
			// add to unneeded list
		// cancel solver





	GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
		GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_OTHER, GTK_BUTTONS_NONE,
		_("Show Unneeded Dependencies"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
		_("This is not a comprehensive listing of dependencies no longer in "
		"use. It is possible we missed some."));
	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_JUMP_TO, 1);
	gtk_dialog_add_button (GTK_DIALOG (dialog), _("Remove All"), 2);
	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), 1, FALSE);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
	gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 650, 600);
	g_signal_connect (G_OBJECT (dialog), "response",
	                  G_CALLBACK (response_cb), this);
	g_signal_connect (G_OBJECT (dialog), "delete-event",
	                  G_CALLBACK (gtk_true), this);

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), gtk_label_new ("TO DO"));

	gtk_widget_show_all (dialog);
	m_dialog = dialog;
}

YGtkPkgVestigialDialog::~YGtkPkgVestigialDialog()
{ gtk_widget_destroy (m_dialog); }

void YGtkPkgVestigialDialog::popup()
{ gtk_window_present (GTK_WINDOW (m_dialog)); }

