/* Yast Control Center (Yast-GTK) */

#include <string>
#include <set>
#include <map>
#include <gtk/gtk.h>
#include <stdlib.h>

/* Definitions */
static GdkColor bg_color = { 0, 255 << 8, 255 << 8, 255 << 8 };
#define YAST_GROUPS  "/usr/share/applications/YaST2/groups/"
#define YAST_ENTRIES "/usr/share/applications/YaST2/"
#define ICONS "/usr/share/YaST2/theme/SuSELinux/icons/32x32/apps/"

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
	GtkWidget *m_widget, *m_vbox;
	GtkAdjustment *adjustment;
	std::map <std::string, GtkListStore*> m_stores;

public:
	View()
	{
// FIXME: view port not working. I dunno how to deal with gtk_adjustment :-/
		adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 1.0, 1.0, 2.0, 1.0));
		m_widget = gtk_viewport_new (NULL, adjustment);

		GtkWidget *box = gtk_event_box_new();
		gtk_widget_modify_bg (box, GTK_STATE_NORMAL, &bg_color);

		m_vbox = gtk_vbox_new (FALSE, 0);
		gtk_container_add (GTK_CONTAINER (box), m_vbox);
		gtk_container_add (GTK_CONTAINER (m_widget), box);
	}

	~View() {}

	GtkWidget *getWidget()
	{ return m_widget; }

	void addGroup (const gchar *name, const gchar *nick)
	{
		GtkWidget *hbox, *label, *separator;
		hbox = gtk_hbox_new (FALSE, 0);
		label = gtk_label_new (("<b>" + std::string (name) + "</b>").c_str());
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
		separator = gtk_hseparator_new();
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), separator, TRUE, TRUE, 6);
		gtk_box_pack_start (GTK_BOX (m_vbox), hbox, FALSE, FALSE, 0);

		GtkListStore *store = gtk_list_store_new (3, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING);
		m_stores [nick] = store;

		GtkWidget *icons_view;
		icons_view = gtk_icon_view_new_with_model (GTK_TREE_MODEL (store));
		gtk_widget_modify_base (icons_view, GTK_STATE_NORMAL, &bg_color);
		gtk_icon_view_set_text_column   (GTK_ICON_VIEW (icons_view), 0);
		gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (icons_view), 1);
		gtk_box_pack_start (GTK_BOX (m_vbox), icons_view, TRUE, TRUE, 0);
		g_signal_connect(G_OBJECT (icons_view), "item-activated",
		                 G_CALLBACK (executeCommand), this);
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

gtk_adjustment_value_changed (adjustment);
	}

	static void executeCommand (GtkIconView *iconview, GtkTreePath *path, View *pThis)
	{
		GtkTreeModel *model = gtk_icon_view_get_model (iconview);
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter (model, &iter, path)) {
			gchar *command;
			gtk_tree_model_get (model, &iter, 2, &command, -1);

printf ("running %s\n", command);
			gtk_widget_hide (window);

// FIXME: we need to flush the g-event stuff so that the window gets really hidden

			gchar **argv = g_strsplit (command, " ", 0);
			g_spawn_sync (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL, NULL, NULL);

// or we could just use:
//			system (command);


			gtk_widget_show (window);

			g_strfreev (argv);
			g_free (command);
		}
	}
};

/* Main code */
int main(int argc, char* argv[])
{
	gtk_init (&argc, &argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Suse Control Center");
	gtk_window_set_default_size (GTK_WINDOW (window), 400, 300);
	g_signal_connect(G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit), NULL);

	View view;
	{
		bool is_root = getuid () == 0;

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
				if (name && nick)
					view.addGroup (name, nick);

				if (name) g_free (name);
				if (nick) g_free (nick);
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
	}

	gtk_container_add (GTK_CONTAINER (window), view.getWidget());
	gtk_widget_show_all (window);

	gtk_main();
	return 0;
}
