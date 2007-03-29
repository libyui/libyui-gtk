/* Yast Control Center (Yast-GTK) */

#include <string>
#include <set>
#include <map>
#include <list>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

/* Definitions */
#define YAST_GROUPS  "/usr/share/applications/YaST2/groups/"
#define YAST_ENTRIES "/usr/share/applications/YaST2/"
#define ICONS "/usr/share/YaST2/theme/current/icons/32x32/apps/"

/* Globals */
static GtkWidget *window;

/* Get all names of files in a certain directory. */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

std::set<std::string> subfiles(const std::string& path) {
	DIR *dirStructP;
	struct dirent *direntp;
	std::set <std::string> sfiles;

	if((dirStructP = opendir(path.c_str())) != NULL) {
		while((direntp = readdir(dirStructP)) != NULL) {
			std::string filename;
			struct stat buf;
			filename = path + "/" + direntp->d_name;

			if(std::string (direntp->d_name) == "." ||
			   std::string (direntp->d_name) == "..")
				continue;  // ignore . and .. directories

			if (stat(filename.c_str(), &buf) == 0 && S_ISREG(buf.st_mode))
				sfiles.insert(direntp->d_name);
		}
	closedir(dirStructP);
	}
	return sfiles;
}

/* Erase everything from the given characters. */
void erase_from (std::string &str, const std::string &chars)
{
	for (unsigned int i = 0; i < chars.length(); i++) {
		std::string::size_type pos;
		pos = str.find (chars[i], 0);
		if (pos != std::string::npos)
			str.erase (pos);
	}
}

/* The widget with the icons. */
class View {
	GtkWidget *m_widget;
	GtkAdjustment *adjustment;
	std::map <std::string, GtkListStore*> m_stores;
	std::list <std::string> sort_keys;

public:
	View()
	{
		m_widget = gtk_notebook_new();
		gtk_notebook_set_tab_pos (GTK_NOTEBOOK (m_widget), GTK_POS_LEFT);
	}

	~View() {}

	GtkWidget *getWidget()
	{ return m_widget; }

	void addGroup (const gchar *name, const gchar *icon_path,
	               const gchar *nick, const gchar *sort_key)
	{
		// calculate position
		int pos;
		{
			std::list <std::string>::iterator it;
			for (it = sort_keys.begin(), pos = 0; it != sort_keys.end(); it++, pos++)
				if (strcmp (it->c_str(), sort_key) >= 0)
					break;
			sort_keys.insert (it, sort_key);
		}

		// label widget
		GtkWidget *tab_label, *image, *label;

		GdkPixbuf *icon = NULL;
		if (icon_path) {
			GError *error = 0;
			std::string path = ICONS + std::string (icon_path) + ".png";
			icon = gdk_pixbuf_new_from_file (path.c_str(), &error);
			if (!icon)
				g_warning ("Could not load icon: %s.\nReason: %s", icon_path, error->message);
		}

		tab_label = gtk_hbox_new (FALSE, 0);
		label = gtk_label_new (name);
		if (icon)
			image = gtk_image_new_from_pixbuf (icon);

		if (icon)
			gtk_box_pack_start (GTK_BOX (tab_label), image, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (tab_label), label, TRUE, TRUE, icon ? 6 : 0);

		// page widget
		GtkListStore *store = gtk_list_store_new (3, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING);
		m_stores [nick] = store;

		GtkWidget *icons_view;
		icons_view = gtk_icon_view_new_with_model (GTK_TREE_MODEL (store));
		gtk_icon_view_set_text_column   (GTK_ICON_VIEW (icons_view), 0);
		gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (icons_view), 1);
		g_signal_connect(G_OBJECT (icons_view), "item-activated",
		                 G_CALLBACK (executeCommand), this);

		GtkWidget *page;
		page = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (page),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (page), GTK_SHADOW_IN);
		gtk_container_add (GTK_CONTAINER (page), icons_view);

		// add those to the notebook
		gtk_widget_show_all (tab_label);
		gtk_notebook_insert_page_menu (GTK_NOTEBOOK (m_widget), page, tab_label, NULL, pos);
	}

	void addEntry (const gchar *group, const gchar *name,
	               const gchar *icon_path, const gchar *execute)
	{
		GtkListStore *store = m_stores [group];
		if (!store) {
			g_warning ("Didn't find group '%s' for entry '%s'.", group, name);
			return;
		}

		GdkPixbuf *icon = NULL;
		if (icon_path) {
			GError *error = 0;
			std::string path = ICONS + std::string (icon_path) + ".png";
			icon = gdk_pixbuf_new_from_file (path.c_str(), &error);
			if (!icon)
				g_warning ("Could not load icon: %s.\nReason: %s", icon_path, error->message);
		}

		GtkTreeIter iter;
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, name, 1, icon, 2, execute, -1);
	}

	static void executeCommand (GtkIconView *iconview, GtkTreePath *path, View *pThis)
	{
		GtkTreeModel *model = gtk_icon_view_get_model (iconview);
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter (model, &iter, path)) {
			gchar *command_;
			gtk_tree_model_get (model, &iter, 2, &command_, -1);

			std::string command = command_;
			g_free (command_);

			command += " &";
			printf ("running %s\n", command.c_str());
			system (command.c_str());

			static GdkCursor *cursor = NULL;
			if (!cursor) {
				GdkDisplay *display = gtk_widget_get_display (window);
				cursor = gdk_cursor_new_for_display (display, GDK_WATCH);
			}
			gdk_window_set_cursor (window->window, cursor);
			g_timeout_add (1500, set_normal_cursor_cb, NULL);
		}
	}

	static gboolean set_normal_cursor_cb (gpointer data)
	{
		gdk_window_set_cursor (window->window, NULL);
		return FALSE;
	}
};

/* Main code */
int main(int argc, char* argv[])
{
	gtk_init (&argc, &argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Suse Control Center");
	gtk_window_set_default_size (GTK_WINDOW (window), 650, 400);
	g_signal_connect(G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit), NULL);

	bool is_root = getuid () == 0;

	View view;
	{  // adding groups
		GKeyFile *file = g_key_file_new();
		std::set <std::string> groups = subfiles (YAST_GROUPS);
		for (std::set <std::string>::iterator it = groups.begin();
		     it != groups.end(); it++) {
			if (!g_key_file_load_from_file (file, (YAST_GROUPS + (*it)).c_str(),
			                                G_KEY_FILE_NONE, NULL))
				continue;

			gchar* name = g_key_file_get_locale_string (file, "Desktop Entry", "Name", 0, NULL);
			gchar *nick = g_key_file_get_string (file, "Desktop Entry", "X-SuSE-YaST-Group", NULL);
			gchar *icon = g_key_file_get_string (file, "Desktop Entry", "Icon", NULL);
			gchar *sort_key = g_key_file_get_string (file, "Desktop Entry",
			                                         "X-SuSE-YaST-SortKey", NULL);
			if (name && nick)
				view.addGroup (name, icon, nick, sort_key);

			if (name)     g_free (name);
			if (nick)     g_free (nick);
			if (icon)     g_free (icon);
			if (sort_key) g_free (sort_key);
		}
		g_key_file_free (file);
	}
	{  // adding entries
		GKeyFile *file = g_key_file_new();
		std::set <std::string> entries = subfiles (YAST_ENTRIES);
		for (std::set <std::string>::iterator it = entries.begin();
		     it != entries.end(); it++) {
			if (!g_key_file_load_from_file (file, (YAST_ENTRIES + (*it)).c_str(),
			                                G_KEY_FILE_NONE, NULL))
				continue;

			gchar *group = g_key_file_get_string (file, "Desktop Entry", "X-SuSE-YaST-Group", NULL);
			gchar* name = g_key_file_get_locale_string (file, "Desktop Entry", "Name", 0, NULL);
			gchar *icon = g_key_file_get_string (file, "Desktop Entry", "Icon", NULL);
			gchar *command = g_key_file_get_string (file, "Desktop Entry", "Exec", NULL);
			gboolean needs_root = g_key_file_get_boolean (file, "Desktop Entry",
			                          "X-SuSE-YaST-RootOnly", NULL);

			if (group && name && command && (!needs_root || is_root))
				view.addEntry (group, name, icon, command);

			if (group)   g_free (group);
			if (name)    g_free (name);
			if (icon)    g_free (icon);
			if (command) g_free (command);
		}
		g_key_file_free (file);
	}

	gtk_container_add (GTK_CONTAINER (window), view.getWidget());
	gtk_widget_show_all (window);

	if (!is_root) {
		GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (window),
			GtkDialogFlags (0), GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
			"You are executing the control center as an ordinary user.\n"
			"Only a few modules will be available.");
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}

	gtk_main();
	return 0;
}
