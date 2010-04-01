/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* Textdomain "yast2-gtk" */
/* YGtkPkgMenuBar, menu bar */
// check the header file for information about this widget

#include "YGi18n.h"
#include "config.h"
#include "YGDialog.h"
#include "ygtkpkghistorydialog.h"
#include <gtk/gtk.h>
#include "ygtktextview.h"

#include <zypp/parser/HistoryLogReader.h>
#define FILENAME "/var/log/zypp/history"

static void selection_changed_cb (GtkTreeSelection *selection, GtkTextView *text)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		char *date;
		gtk_tree_model_get (model, &iter, 0, &date, -1);

		GtkTextBuffer *buffer = gtk_text_view_get_buffer (text);
		GtkTextMark *mark = gtk_text_buffer_get_mark (buffer, date);
		gtk_text_view_scroll_to_mark (text, mark, 0, TRUE, 0, 0);
		g_free (date);
	}
}

struct Handler
{
	Handler (GtkListStore *store, GtkTextBuffer *buffer)
	: store (store), buffer (buffer)
	{
		bold_tag = gtk_text_buffer_create_tag (buffer, "bold",
			"weight", PANGO_WEIGHT_BOLD, "foreground", "#5c5c5c",
			"scale", PANGO_SCALE_LARGE, "pixels-below-lines", 6,
			"pixels-above-lines", 6, NULL);
	}

	bool operator() (const zypp::HistoryItem::Ptr &item)
	{
		std::string _date (item->date.form ("%d %B %Y"));
		bool new_date = date != _date;

		GtkTextIter text_iter;
		gtk_text_buffer_get_end_iter (buffer, &text_iter);

		if (new_date) {
			date = _date;
			GtkTreeIter list_iter;
			gtk_list_store_append (store, &list_iter);
			gtk_list_store_set (store, &list_iter, 
				0, date.c_str(), -1);

			gtk_text_buffer_create_mark (buffer, date.c_str(), &text_iter, TRUE);
			gtk_text_buffer_insert_with_tags (buffer, &text_iter,
				(date + "\n").c_str(), -1, bold_tag, NULL);
		}

		std::string line;
		line.reserve (1024);
		line = "\t";
		switch (item->action.toEnum()) {
			case zypp::HistoryActionID::NONE_e:
				break;
			case zypp::HistoryActionID::INSTALL_e: {
				zypp::HistoryItemInstall *_item =
					static_cast <zypp::HistoryItemInstall *> (item.get());
				line += _("install"); line += "  ";
				line += _item->name + "  ";
				line += _item->edition.version();
				break;
			}
			case zypp::HistoryActionID::REMOVE_e: {
				zypp::HistoryItemRemove *_item =
					static_cast <zypp::HistoryItemRemove *> (item.get());
				line += _("remove"); line += "  ";
				line += _item->name + "  ";
				line += _item->edition.version();
				break;
			}
			case zypp::HistoryActionID::REPO_ADD_e: {
				zypp::HistoryItemRepoAdd *_item =
					static_cast <zypp::HistoryItemRepoAdd *> (item.get());
				line += _("add repository"); line += " ";
				line += _item->alias;
				break;
			}
			case zypp::HistoryActionID::REPO_REMOVE_e: {
				zypp::HistoryItemRepoRemove *_item =
					static_cast <zypp::HistoryItemRepoRemove *> (item.get());
				line += _("remove repository"); line += " ";
				line += _item->alias;
				break;
			}
			case zypp::HistoryActionID::REPO_CHANGE_ALIAS_e: {
				zypp::HistoryItemRepoAliasChange *_item =
					static_cast <zypp::HistoryItemRepoAliasChange *> (item.get());
				gchar *str = g_strdup_printf ("change repository %s alias: %s",
					_item->oldalias.c_str(), _item->newalias.c_str());
				line += str;
				g_free (str);
				break;
			}
			case zypp::HistoryActionID::REPO_CHANGE_URL_e: {
				zypp::HistoryItemRepoUrlChange *_item =
					static_cast <zypp::HistoryItemRepoUrlChange *> (item.get());
				gchar *str = g_strdup_printf ("change repository %s url: %s",
					_item->alias.c_str(), _item->newurl.asString().c_str());
				line += str;
				g_free (str);
				break;
			}
		}
		line += "\n";
		gtk_text_buffer_insert (buffer, &text_iter, line.c_str(), -1);
		return true;
	}

	GtkListStore *store;
	GtkTextBuffer *buffer;
	std::string date;
	GtkTextTag *bold_tag;
};

static void write_file (const char *filename, const char *text)
{
	FILE *file = fopen (filename, "w");
	fwrite (text, strlen (text), sizeof (char), file);
	fclose (file);
}

static void response_cb (GtkDialog *dialog, gint response, GtkTextBuffer *buffer)
{
	if (response == 1) {
		GtkWidget *_dialog = gtk_file_chooser_dialog_new ("", GTK_WINDOW (dialog),
			GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_OPEN, GTK_RESPONSE_OK,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
		gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (_dialog), TRUE);
		gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (_dialog), TRUE);
		if (gtk_dialog_run (GTK_DIALOG (_dialog)) == GTK_RESPONSE_OK) {
			gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (_dialog));
			GtkTextIter start, end;
			gtk_text_buffer_get_bounds (buffer, &start, &end);
			gchar *text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
			write_file (filename, text);
			g_free (text);
			g_free (filename);
		}
		gtk_widget_destroy (_dialog);
	}
	else
		gtk_widget_destroy (GTK_WIDGET (dialog));
}

YGtkPkgHistoryDialog::YGtkPkgHistoryDialog()
{
	GtkWidget *text = ygtk_text_view_new (FALSE);
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
	GtkWidget *text_scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (text_scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (
		GTK_SCROLLED_WINDOW (text_scroll), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (text_scroll), text);

	GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
	GtkWidget *date_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	g_object_unref (G_OBJECT (store));
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (date_view), 0);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (date_view), FALSE);

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (
		NULL, renderer, "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (date_view), column);

	//gtk_tree_view_column_set_sort_column_id (column, 1);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (date_view), TRUE);

	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (date_view));
	g_signal_connect (G_OBJECT (selection), "changed",
	                  G_CALLBACK (selection_changed_cb), text);

	GtkWidget *date_scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (date_scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (
		GTK_SCROLLED_WINDOW (date_scroll), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (date_scroll), date_view);

	GtkWidget *dialog = gtk_message_dialog_new (YGDialog::currentWindow(),
		GtkDialogFlags (0), GTK_MESSAGE_OTHER, GTK_BUTTONS_NONE, _("History of Changes"));
//	gtk_dialog_add_button (GTK_DIALOG (dialog), _("Save to file"), 1);
	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_DELETE_EVENT);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), FILENAME);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
	gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 600, 500);
	g_signal_connect (G_OBJECT (dialog), "response",
	                  G_CALLBACK (response_cb), buffer);

	GtkWidget *hpaned = gtk_hpaned_new();
	gtk_paned_pack1 (GTK_PANED (hpaned), date_scroll, FALSE, FALSE);
	gtk_paned_pack2 (GTK_PANED (hpaned), text_scroll, TRUE, FALSE);
	gtk_paned_set_position (GTK_PANED (hpaned), 180);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hpaned);

	Handler handler (store, buffer);
	zypp::parser::HistoryLogReader parser (FILENAME, boost::ref (handler));
	try {
		parser.readAll();
	}
	catch (const zypp::Exception &ex) {
		yuiWarning () << "Error: Could not load log file" << FILENAME << ": "
			<< ex.asUserHistory() << std::endl;
	}

	gtk_widget_show_all (dialog);
	m_dialog = dialog;
}

YGtkPkgHistoryDialog::~YGtkPkgHistoryDialog()
{} //gtk_widget_destroy (m_dialog); }

void YGtkPkgHistoryDialog::popup()
{ gtk_widget_show (m_dialog); }

