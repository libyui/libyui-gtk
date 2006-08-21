/* YGtkWizard is a widget similar that offers a wizard interface
   (eg. buttons and side help text) based on the look of GtkAssistance
   that we couldn't use for several reasons (for starters, it is
   a window of its own and we need a widget).
*/

#ifndef YGTK_WIZARD_H
#define YGTK_WIZARD_H

#include <gdk/gdk.h>
#include <gtk/gtkeventbox.h>

G_BEGIN_DECLS

#define YGTK_TYPE_WIZARD            (ygtk_wizard_get_type ())
#define YGTK_WIZARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                     YGTK_TYPE_WIZARD, YGtkWizard))
#define YGTK_WIZARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                     YGTK_TYPE_WIZARD, YGtkWizardClass))
#define IS_YGTK_WIZARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                     YGTK_TYPE_WIZARD))
#define IS_YGTK_WIZARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                     YGTK_TYPE_WIZARD))
#define YGTK_WIZARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                     YGTK_TYPE_WIZARD, YGtkWizardClass))

typedef struct _YGtkWizard       YGtkWizard;
typedef struct _YGtkWizardClass  YGtkWizardClass;

struct _YGtkWizard
{
	GtkEventBox box;

	// members
	/* Hash tables to assign Yast Ids to Gtk entries. */
	GHashTable *menu_ids;   /* gchar* -> GtkWidget* */
	GHashTable *tree_ids;   /* gchar* -> GtkTreePath* */
	GHashTable *steps_ids;  /* gchar* -> guint */

	/* Widgets that we need to have access to. */
	GtkWidget *m_menu, *m_title_label, *m_title_image, *m_help_widget,
	          *m_steps_widget, *m_tree_widget, *m_release_notes_button;
	GtkWidget *m_back_button, *m_abort_button, *m_next_button;

	GtkWidget *m_containee;  // containee

	/* Layouts that we need for size_request. */
	GtkWidget *m_button_box, *m_pane_box, *m_help_vbox, *m_title_hbox,
	          *m_main_hbox;
	GtkWidget *m_help_program_pane;
};

struct _YGtkWizardClass
{
	GtkEventBoxClass parent_class;

	// signal
	void (*action_triggered) (YGtkWizard *wizard, gpointer id, gint id_type);
};

GtkWidget* ygtk_wizard_new();
GType ygtk_wizard_get_type (void) G_GNUC_CONST;

// Enable a steps or tree pane widget -- should be called right after
// initilization and only _one_ of them
void ygtk_wizard_enable_steps (YGtkWizard *wizard);
void ygtk_wizard_enable_tree  (YGtkWizard *wizard);

void ygtk_wizard_set_child (YGtkWizard *wizard, GtkWidget *widget);
void ygtk_wizard_unset_child (YGtkWizard *wizard);

// commands
// (commands that may fail return a sucess boolean.)
void ygtk_wizard_set_help_text (YGtkWizard *wizard, const char *text);

gboolean ygtk_wizard_add_tree_item (YGtkWizard *wizard, const char *parent_id,
                                    const char *text, const char *id);
void ygtk_wizard_clear_tree (YGtkWizard *wizard);
gboolean ygtk_wizard_select_tree_item (YGtkWizard *wizard, const char *id);

void ygtk_wizard_set_header_text (YGtkWizard *wizard, const char *text);
// you may pass a window widget if you want the icon to be set on it as well
gboolean ygtk_wizard_set_header_icon (YGtkWizard *wizard, GtkWidget *window,
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

void ygtk_wizard_set_release_notes_button_label (YGtkWizard *wizard,
                                     const gchar *text, gpointer id,
                                     GDestroyNotify destroy_cb);
void ygtk_wizard_show_release_notes_button (YGtkWizard *wizard, gboolean enable);

const gchar *ygtk_wizard_get_tree_selection (YGtkWizard *wizard);

// You should call this method rather than GtkWidget's if you want
// to honor next_button protect flag
void ygtk_wizard_set_sensitive (YGtkWizard *wizard, gboolean sensitive);

// gets the nice size of the widget with the exception of the given child
void ygtk_wizard_size_request (YGtkWizard *wizard, GtkRequisition *requisition);

G_END_DECLS

#endif /* YGTK_WIZARD_H */
