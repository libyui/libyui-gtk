/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkWizard is a widget based on GtkAssistant, which we couldn't
   use (for starters, GtkAssistant comes from GtkWindow and we need
   a widget, not a window).
*/

#ifndef YGTK_WIZARD_H
#define YGTK_WIZARD_H

#include <gdk/gdk.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

// test feature: history on help dialog
//#define SET_HELP_HISTORY

typedef struct YGtkHelpText {
#ifdef SET_HELP_HISTORY
	GList *history;
#else
	gchar *text;
#endif
	GtkWidget *dialog;
} YGtkHelpText;

YGtkHelpText *ygtk_help_text_new (void);
void ygtk_help_text_destroy (YGtkHelpText *help);
void ygtk_help_text_set (YGtkHelpText *help, const gchar *title, const gchar *text);
const gchar *ygtk_help_text_get (YGtkHelpText *help, gint n);
void ygtk_help_text_sync (YGtkHelpText *help, GtkWidget *dialog);

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

	// members:
	GtkWidget *title_box, *title_label, *title_image;
	GtkWidget *help_box, *help_text;
	GtkWidget *search_entry, *close_button, *history_combo;
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

void ygtk_help_dialog_set_text (YGtkHelpDialog *dialog, const gchar *text);

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
	GtkVBox box;

	// private:
	/* Hash tables to assign Yast Ids to Gtk entries. */
	GHashTable *menu_ids;   /* gchar* -> GtkWidget*    */
	GHashTable *tree_ids;   /* gchar* -> GtkTreePath*  */
	GHashTable *steps_ids;  /* gchar* -> guint         */

	/* For layout */
	GtkWidget *m_menu_box, *m_title, *m_contents_box, *m_control_bar,
		*m_child, *m_status_box, *m_pane, *m_buttons, *m_info_box;

	/* Widgets we need to access. */
	GtkWidget *tree_view, *steps, *menu,
	          *back_button, *abort_button, *next_button, *help_button,
	          *release_notes_button, *m_default_button;

	/* The help text. */
	YGtkHelpText *m_help;
} YGtkWizard;

typedef struct _YGtkWizardClass
{
	GtkVBoxClass parent_class;

	// signals:
	void (*action_triggered) (YGtkWizard *wizard, gpointer id, gint id_type);
	void (*popup_help) (YGtkWizard *dialog);
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
void ygtk_wizard_set_information_widget (YGtkWizard *wizard, GtkWidget *widget);
void ygtk_wizard_set_control_widget (YGtkWizard *wizard, GtkWidget *widget);

// commands
// (commands that may fail return a sucess boolean.)
void ygtk_wizard_set_help_text (YGtkWizard *wizard, const gchar *text);

void ygtk_wizard_set_header_text (YGtkWizard *wizard, const char *text);
gboolean ygtk_wizard_set_header_icon (YGtkWizard *wizard, const char *icon);

void ygtk_wizard_set_button_label (YGtkWizard *wizard, GtkWidget *button,
                                   const char *text, const char *stock);
void ygtk_wizard_enable_button (YGtkWizard *wizard, GtkWidget *button,
                                 gboolean enable);
void ygtk_wizard_set_button_str_id (YGtkWizard *wizard, GtkWidget *button,
                                    const char *id);
void ygtk_wizard_set_button_ptr_id (YGtkWizard *wizard, GtkWidget *button,
                                     gpointer id);
void ygtk_wizard_set_default_button (YGtkWizard *wizard, GtkWidget *button);  // before realize

void ygtk_wizard_set_extra_button (YGtkWizard *wizard, GtkWidget *widget);

void ygtk_wizard_add_menu (YGtkWizard *wizard, const char *text,
                           const char *id);
gboolean ygtk_wizard_add_menu_entry (YGtkWizard *wizard, const char *parent_id,
                                     const char *text, const char *id);
gboolean ygtk_wizard_add_sub_menu (YGtkWizard *wizard, const char *parent_id,
                                   const char *text, const char *id);
gboolean ygtk_wizard_add_menu_separator (YGtkWizard *wizard, const char *parent_id);
void ygtk_wizard_clear_menu (YGtkWizard *wizard);
void ygtk_wizard_set_custom_menubar (YGtkWizard *wizard, GtkWidget *menu_bar, gboolean hide_header);
void ygtk_wizard_set_status_bar (YGtkWizard *wizard, GtkWidget *status_bar);

void ygtk_wizard_add_step_header (YGtkWizard *wizard, const char *text);
void ygtk_wizard_add_step (YGtkWizard *wizard, const char* text, const char *id);
gboolean ygtk_wizard_set_current_step (YGtkWizard *wizard, const char *id);
void ygtk_wizard_clear_steps (YGtkWizard *wizard);

gboolean ygtk_wizard_add_tree_item (YGtkWizard *wizard, const char *parent_id,
                                    const char *text, const char *id);
void ygtk_wizard_clear_tree (YGtkWizard *wizard);
gboolean ygtk_wizard_select_tree_item (YGtkWizard *wizard, const char *id);
const gchar *ygtk_wizard_get_tree_selection (YGtkWizard *wizard);

G_END_DECLS
#endif /* YGTK_WIZARD_H */

