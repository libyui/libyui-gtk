//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

/* YGtkWizard is a widget based on GtkAssistant, which we couldn't
   use (for starters, GtkAssistant comes from GtkWindow and we need
   a widget, not a window).
*/

#ifndef YGTK_WIZARD_H
#define YGTK_WIZARD_H

#include <gdk/gdk.h>
#include <gtk/gtkbin.h>
#include <gtk/gtkwindow.h>
G_BEGIN_DECLS

// YGtkHelpDialog (for showing help text)
/* Shows help text. It is GtkWindow-inherited, rather than GtkDialog, because
   we want to place a search entry next to the buttons. */

#define YGTK_TYPE_HELP_DIALOG            (ygtk_help_dialog_get_type ())
#define YGTK_HELP_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                          YGTK_TYPE_HELP_DIALOG, YGtkHelpDialog))
#define YGTK_HELP_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                          YGTK_TYPE_HELP_DIALOG, YGtkHelpDialogClass))
#define YGTK_IS_HELP_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                          YGTK_TYPE_HELP_DIALOG))
#define YGTK_IS_HELP_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                          YGTK_TYPE_HELP_DIALOG))
#define YGTK_HELP_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                          YGTK_TYPE_HELP_DIALOG, YGtkHelpDialogClass))

typedef struct _YGtkHelpDialog
{
	GtkWindow parent;

	// parent:
	GtkWidget *title_box, *title_label, *title_image;
	GtkWidget *help_box, *help_text;
	GtkWidget *search_entry, *close_button;
	GtkWidget *vbox;
} YGtkHelpDialog;

typedef struct _YGtkHelpDialogClass
{
	GtkWindowClass parent_class;

	// signals
	void (*find_next) (YGtkHelpDialog *dialog);
	void (*close) (YGtkHelpDialog *dialog);
} YGtkHelpDialogClass;

GtkWidget *ygtk_help_dialog_new (GtkWindow *parent);
GType ygtk_help_dialog_get_type (void) G_GNUC_CONST;

void ygtk_help_dialog_set_text (YGtkHelpDialog *dialog, const char *text);

// YGtkWizard

#define YGTK_TYPE_WIZARD            (ygtk_wizard_get_type ())
#define YGTK_WIZARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                     YGTK_TYPE_WIZARD, YGtkWizard))
#define YGTK_WIZARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                     YGTK_TYPE_WIZARD, YGtkWizardClass))
#define YGTK_IS_WIZARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                     YGTK_TYPE_WIZARD))
#define YGTK_IS_WIZARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                     YGTK_TYPE_WIZARD))
#define YGTK_WIZARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                     YGTK_TYPE_WIZARD, YGtkWizardClass))

typedef struct _YGtkWizard
{
	GtkBin bin;

	// private:
	/* Hash tables to assign Yast Ids to Gtk entries. */
	GHashTable *menu_ids;   /* gchar* -> GtkWidget*    */
	GHashTable *tree_ids;   /* gchar* -> GtkTreePath*  */
	GHashTable *steps_ids;  /* gchar* -> guint         */

	/* Widgets for layout. */
	GtkWidget *m_menu, *m_title, *m_navigation, *m_buttons;
	// containee can be accessed via GTK_BIN (wizard)->child

	/* Widgets we need to have access to. */
	GtkWidget *m_title_label, *m_title_image, *m_navigation_widget,
	          *m_back_button, *m_abort_button, *m_next_button, *m_help_button,
	          *m_release_notes_button;

	/* The help text. */
	gchar     *m_help;
	GtkWidget *m_help_dialog;
} YGtkWizard;

typedef struct _YGtkWizardClass
{
	GtkBinClass parent_class;

	// signals:
	void (*action_triggered) (YGtkWizard *wizard, gpointer id, gint id_type);
} YGtkWizardClass;

GtkWidget *ygtk_wizard_new (void);
GType ygtk_wizard_get_type (void) G_GNUC_CONST;

// Enable a steps or tree pane widget -- should be called right after
// initilization and only _one_ of them
void ygtk_wizard_enable_steps (YGtkWizard *wizard);
void ygtk_wizard_enable_tree  (YGtkWizard *wizard);

// convinience method that removes the current child, if set, and swaps it by
// the given one (you may pass NULL to just remove current child)
void ygtk_wizard_set_child (YGtkWizard *wizard, GtkWidget *widget);

// commands
// (commands that may fail return a sucess boolean.)
void ygtk_wizard_set_help_text (YGtkWizard *wizard, const gchar *text);

// you may pass a window widget if you want the title/icon to be set on it as well
void ygtk_wizard_set_header_text (YGtkWizard *wizard, GtkWindow *window,
                                  const char *text);
gboolean ygtk_wizard_set_header_icon (YGtkWizard *wizard, GtkWindow *window,
                                      const char *icon);

void ygtk_wizard_set_back_button_label (YGtkWizard *wizard, const char *text);
void ygtk_wizard_set_abort_button_label (YGtkWizard *wizard, const char *text);
void ygtk_wizard_set_next_button_label (YGtkWizard *wizard, const char *text);
void ygtk_wizard_set_back_button_id (YGtkWizard *wizard, gpointer id,
                                     GDestroyNotify destroy_cb);
void ygtk_wizard_set_next_button_id (YGtkWizard *wizard, gpointer id,
                                     GDestroyNotify destroy_cb);
void ygtk_wizard_set_abort_button_id (YGtkWizard *wizard, gpointer id,
                                      GDestroyNotify destroy_cb);
void ygtk_wizard_enable_back_button (YGtkWizard *wizard, gboolean enable);
void ygtk_wizard_enable_next_button (YGtkWizard *wizard, gboolean enable);
void ygtk_wizard_enable_abort_button (YGtkWizard *wizard, gboolean enable);
gboolean ygtk_wizard_is_next_button_protected (YGtkWizard *wizard);
void ygtk_wizard_protect_next_button (YGtkWizard *wizard, gboolean protect);
void ygtk_wizard_focus_next_button (YGtkWizard *wizard);
void ygtk_wizard_focus_back_button (YGtkWizard *wizard);

void ygtk_wizard_add_menu (YGtkWizard *wizard, const char *text,
                           const char *id);
gboolean ygtk_wizard_add_menu_entry (YGtkWizard *wizard, const char *parent_id,
                                     const char *text, const char *id);
gboolean ygtk_wizard_add_sub_menu (YGtkWizard *wizard, const char *parent_id,
                                   const char *text, const char *id);
gboolean ygtk_wizard_add_menu_separator (YGtkWizard *wizard, const char *parent_id);

void ygtk_wizard_add_step_header (YGtkWizard *wizard, const char *text);
void ygtk_wizard_add_step (YGtkWizard *wizard, const char* text, const char *id);
gboolean ygtk_wizard_set_current_step (YGtkWizard *wizard, const char *id);
void ygtk_wizard_clear_steps (YGtkWizard *wizard);

gboolean ygtk_wizard_add_tree_item (YGtkWizard *wizard, const char *parent_id,
                                    const char *text, const char *id);
void ygtk_wizard_clear_tree (YGtkWizard *wizard);
gboolean ygtk_wizard_select_tree_item (YGtkWizard *wizard, const char *id);
const gchar *ygtk_wizard_get_tree_selection (YGtkWizard *wizard);

void ygtk_wizard_set_release_notes_button_label (YGtkWizard *wizard,
                                     const gchar *text, gpointer id,
                                     GDestroyNotify destroy_cb);
void ygtk_wizard_show_release_notes_button (YGtkWizard *wizard, gboolean enable);

// You should call this method rather than GtkWidget's if you want
// to honor next_button protect flag
void ygtk_wizard_set_sensitive (YGtkWizard *wizard, gboolean sensitive);

G_END_DECLS
#endif /* YGTK_WIZARD_H */
