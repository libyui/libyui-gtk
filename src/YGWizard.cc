/* Yast-GTK */

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "YGUtils.h"
#include "YGLayout.h"
#include "ygtkrichtext.h"
#include "ygtksteps.h"
#include "YWizard.h"

#define CONTENT_PADDING 15
#define TITLE_HEIGHT   45
#define MAIN_BORDER 8
#define HELP_BOX_CHARS_WIDTH 25

class YGWizard : public YWizard, public YGWidget
{
	/* Hash tables to assign Yast Ids to Gtk entries. */
	typedef map <string, GtkWidget*> MenuId;
	typedef map <string, GtkTreePath*> TreeId;
	typedef map <string, guint> StepId;
	MenuId menu_ids;
	TreeId tree_ids;
	StepId steps_ids;

	/* Widgets that we need to have access to. */
	GtkWidget *m_menu, *m_title_label, *m_title_image, *m_help_widget,
	          *m_steps_widget, *m_tree_widget, *m_release_notes_button;
	GtkWidget *m_back_button, *m_abort_button, *m_next_button;

	GtkWidget *m_fixed;  // containee

	/* Layouts that we need for size_request. */
	GtkWidget *m_button_box, *m_pane_box, *m_help_vbox, *m_title_hbox,
	          *m_main_hbox;
	GtkWidget *m_help_program_pane;

	/* Miscellaneous */
	bool m_verboseCommands;

public:
	YGWizard (const YWidgetOpt &opt, YGWidget *parent,
	          const YCPValue &backButtonId,  const YCPString &backButtonLabel,
	          const YCPValue &abortButtonId, const YCPString &abortButtonLabel,
	          const YCPValue &nextButtonId,  const YCPString &nextButtonLabel)
	: YWizard (opt, backButtonId, backButtonLabel,
	                abortButtonId, abortButtonLabel,
	                nextButtonId, nextButtonLabel),
	  YGWidget (this, parent, true, GTK_TYPE_EVENT_BOX, NULL)
	{
		IMPL
		m_verboseCommands = false;

		// Layout widgets
		GtkWidget *main_vbox;

		//** Menu bar
		m_menu = gtk_menu_bar_new();

		//** Title
		m_title_hbox = gtk_hbox_new (FALSE, 0);
		gtk_widget_set_size_request (m_title_hbox, -1, TITLE_HEIGHT);

		m_title_image = gtk_image_new();
		m_title_label = gtk_label_new("");

		// setup label look
		gtk_widget_modify_bg (m_title_label, GTK_STATE_NORMAL,
		                      &m_title_label->style->bg[GTK_STATE_SELECTED]);
		gtk_widget_modify_fg (m_title_label, GTK_STATE_NORMAL,
		                      &m_title_label->style->fg[GTK_STATE_SELECTED]);

		// set a strong font to the heading label
		PangoFontDescription* font = pango_font_description_new();
		int size = pango_font_description_get_size (m_title_label->style->font_desc);
		pango_font_description_set_weight (font, PANGO_WEIGHT_ULTRABOLD);
		pango_font_description_set_size   (font, (int)(size * PANGO_SCALE_XX_LARGE));
		gtk_widget_modify_font (m_title_label, font);
		pango_font_description_free (font);

		gtk_container_add (GTK_CONTAINER (m_title_hbox), m_title_label);
		gtk_container_add (GTK_CONTAINER (m_title_hbox), m_title_image);
		gtk_box_set_child_packing (GTK_BOX (m_title_hbox), m_title_label,
		                           FALSE, FALSE, 4, GTK_PACK_START);
		gtk_box_set_child_packing (GTK_BOX (m_title_hbox), m_title_image,
		                           FALSE, FALSE, 4, GTK_PACK_END);

		//** Application area
		m_fixed = gtk_fixed_new();

		{
			// We need to have a ReplacePoint that will then have as a kid the actual
			// content
			YWidgetOpt alignment_opt;
			alignment_opt.isHStretchable.setValue (true);
			alignment_opt.isVStretchable.setValue (true);
			YGAlignment *contents = new YGAlignment
				(alignment_opt, this, YAlignCenter, YAlignCenter);
			contents->setParent (this);

			YWidgetOpt emptyOpt;
			YGReplacePoint *replace_point = new YGReplacePoint (emptyOpt, contents);
			replace_point->setId (YCPSymbol (YWizardContentsReplacePointID));
			replace_point->setParent (contents);

			// We need to add a kid to YReplacePoint until it gets replaced
			YGEmpty *empty = new YGEmpty (emptyOpt, replace_point);
			empty->setParent (replace_point);

			replace_point->addChild (empty);
			contents->addChild (replace_point);
			addChild (contents);
		}

		//** Help box
		GtkWidget *help_scrolled_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (help_scrolled_window),
		                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		GtkWidget *help_frame = gtk_frame_new (NULL);
		gtk_frame_set_shadow_type (GTK_FRAME (help_frame), GTK_SHADOW_IN);
		m_help_vbox = gtk_vbox_new (FALSE, 0);  // shared with "Release Notes" button

		m_help_widget = ygtk_richtext_new();
		g_signal_connect (G_OBJECT (m_help_widget), "realize",
		                  G_CALLBACK (set_help_background_cb), this);

		gtk_container_add (GTK_CONTAINER (help_scrolled_window), m_help_widget);
		gtk_container_add (GTK_CONTAINER (help_frame), help_scrolled_window);

		m_release_notes_button = gtk_button_new();

		gtk_container_add (GTK_CONTAINER (m_help_vbox), help_frame);
		gtk_container_add (GTK_CONTAINER (m_help_vbox), m_release_notes_button);
		gtk_box_set_child_packing (GTK_BOX (m_help_vbox), m_release_notes_button,
		                           FALSE, FALSE, 0, GTK_PACK_START);

		gtk_widget_set_size_request (m_help_vbox,
			YGUtils::calculateCharsWidth (m_help_widget, HELP_BOX_CHARS_WIDTH), -1);

		//** Steps/tree pane
		bool steps_enabled = opt.stepsEnabled.value();
		bool tree_enabled  = opt.treeEnabled.value();
		m_steps_widget = m_tree_widget = NULL;

		if (steps_enabled && tree_enabled) {
			y2error ("YGWizard doesn't support both steps and tree enabled at the same time."
			         "\nDisabling the steps.");
			steps_enabled = false;
		}

		GtkWidget *pane_frame;
		if (steps_enabled || tree_enabled) {
			m_pane_box = gtk_event_box_new();

			pane_frame = gtk_frame_new (NULL);
			gtk_frame_set_shadow_type (GTK_FRAME (pane_frame), GTK_SHADOW_OUT);
		}
		else
			m_pane_box = NULL;

		if (steps_enabled) {
			m_steps_widget = ygtk_steps_new();
			gtk_container_add (GTK_CONTAINER (pane_frame), m_steps_widget);
			gtk_container_add (GTK_CONTAINER (m_pane_box), pane_frame);
		}

		if (tree_enabled) {
			m_tree_widget = gtk_tree_view_new_with_model
				(GTK_TREE_MODEL (gtk_tree_store_new (1, G_TYPE_STRING)));
			gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (m_tree_widget),
				0, "(no title)", gtk_cell_renderer_text_new(), "text", 0, NULL);
			gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (m_tree_widget), FALSE);
			g_signal_connect (G_OBJECT (m_tree_widget), "cursor-changed",
			                  G_CALLBACK (tree_item_selected_cb), this);

			GtkWidget *tree_scrolled_window = gtk_scrolled_window_new (NULL, NULL);
			gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tree_scrolled_window),
			                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
			gtk_container_add (GTK_CONTAINER (tree_scrolled_window), m_tree_widget);
			gtk_container_add (GTK_CONTAINER (pane_frame), tree_scrolled_window);
			gtk_container_add (GTK_CONTAINER (m_pane_box), pane_frame);
		}

		//** Adding the bottom buttons
		/* NOTE: we can't really use gtk_button_new_from_stock() because the
		   nextButton can really be a "Ok" button. Standardize ids is really the
		   way to go. */
		string backLabel  = YGUtils::mapKBAccel(backButtonLabel->value_cstr());
		string abortLabel = YGUtils::mapKBAccel(abortButtonLabel->value_cstr());
		string nextLabel  = YGUtils::mapKBAccel(nextButtonLabel->value_cstr());

		m_back_button  = gtk_button_new_with_mnemonic (backLabel.c_str());
		m_abort_button = gtk_button_new_with_mnemonic (abortLabel.c_str());
		m_next_button  = gtk_button_new_with_mnemonic (nextLabel.c_str());

		g_object_set_data_full (G_OBJECT (m_back_button),  "id",
		                        new YCPValue (backButtonId), delete_data_cb);
		g_object_set_data_full (G_OBJECT (m_abort_button), "id",
		                        new YCPValue (abortButtonId), delete_data_cb);
		g_object_set_data_full (G_OBJECT (m_next_button),  "id",
		                        new YCPValue (nextButtonId), delete_data_cb);
		g_object_set_data (G_OBJECT (m_next_button),  "protect", GINT_TO_POINTER (0));

		g_signal_connect (G_OBJECT (m_back_button), "clicked",
		                  G_CALLBACK (button_clicked_cb), this);
		g_signal_connect (G_OBJECT (m_abort_button), "clicked",
		                  G_CALLBACK (button_clicked_cb), this);
		g_signal_connect (G_OBJECT (m_next_button), "clicked",
		                  G_CALLBACK (button_clicked_cb), this);

		g_signal_connect (G_OBJECT (m_release_notes_button), "clicked",
		                  G_CALLBACK (button_clicked_cb), this);

		m_button_box = gtk_hbutton_box_new();
		gtk_container_add (GTK_CONTAINER (m_button_box), m_abort_button);
		gtk_container_add (GTK_CONTAINER (m_button_box), m_back_button);
		gtk_container_add (GTK_CONTAINER (m_button_box), m_next_button);
		gtk_button_box_set_layout (GTK_BUTTON_BOX (m_button_box), GTK_BUTTONBOX_END);
		gtk_container_set_border_width (GTK_CONTAINER (m_button_box), 10);

		//** Setting general layouts
		m_help_program_pane = gtk_hpaned_new();
		gtk_paned_pack1 (GTK_PANED (m_help_program_pane), m_help_vbox, FALSE, TRUE);
		gtk_paned_pack2 (GTK_PANED (m_help_program_pane), m_fixed, TRUE, TRUE);
		gtk_container_set_border_width (GTK_CONTAINER (m_help_program_pane),
		                                CONTENT_PADDING);

//		g_signal_connect (G_OBJECT (m_help_program_pane), "move-handle",
//		                  G_CALLBACK (help_pane_moved_cb), this);

		g_signal_connect (G_OBJECT (m_help_vbox), "size-allocate",
		                  G_CALLBACK (program_pane_resized_cb), this);

		main_vbox = gtk_vbox_new (FALSE, 0);
		gtk_container_add (GTK_CONTAINER (main_vbox), m_title_hbox);
		gtk_container_add (GTK_CONTAINER (main_vbox), m_help_program_pane);
		gtk_container_add (GTK_CONTAINER (main_vbox), m_button_box);

		m_main_hbox = gtk_hbox_new (FALSE, 0);
		if (m_pane_box) {
			gtk_container_add (GTK_CONTAINER (m_main_hbox), m_pane_box);
			gtk_box_set_child_packing (GTK_BOX (m_main_hbox), m_pane_box,
			                           FALSE, FALSE, 0, GTK_PACK_START);
		}
		gtk_container_add (GTK_CONTAINER (m_main_hbox), main_vbox);

		if (m_pane_box)
			gtk_container_set_border_width (GTK_CONTAINER (m_pane_box), MAIN_BORDER);
		gtk_container_set_border_width (GTK_CONTAINER (main_vbox), MAIN_BORDER);

		gtk_container_add (GTK_CONTAINER (getWidget()), m_main_hbox);

		//** Lights on!
		gtk_widget_show_all (getWidget());
		// some widgets need to start invisibly
		gtk_widget_hide (m_menu);
		// FIXME: this doesn't seem to work:
		if (backLabel.empty())  gtk_widget_hide (m_back_button);
		if (abortLabel.empty()) gtk_widget_hide (m_abort_button);
		if (nextLabel.empty())  gtk_widget_hide (m_next_button);
		gtk_widget_hide (m_release_notes_button);

		//** Get expose, so we can draw line border
		g_signal_connect (G_OBJECT (getWidget()), "expose-event",
		                  G_CALLBACK (expose_event_cb), this);
	}

	void clear_tree_ids()
	{
		IMPL
		for (TreeId::iterator it = tree_ids.begin(); it != tree_ids.end(); it++)
			gtk_tree_path_free (it->second);
		tree_ids.clear();
	}

	virtual ~YGWizard()
	{
		IMPL
		clear_tree_ids();
	}

	virtual GtkFixed* getFixed()
	{
		IMPL
		return GTK_FIXED (m_fixed);
	}

/* args_type is a reverse hexadecimal number where
   any = 0, string = 1, boolean = 2. */
bool isCommand (const YCPTerm &cmd, const char *func,
                guint args_nb, guint args_type)
{
	if (cmd->name() == func) {
		// do some sanity checks
		if ((unsigned) cmd->size() != args_nb) {
			y2error ("YGWizard: expected %d arguments for the '%s' command. %d given.",
			         args_nb, func, cmd->size());
			return false;
		}

		guint i, t;
		for (i = 0, t = args_type; i < args_nb; i++, t >>= 4)
			switch (t % 0x4) {
				case 0x1:
					if (!cmd->value(i)->isString()) {
						y2error ("YGWizard: expected string as the %d argument for the '%s'"
						         " command.", i+1, func);
						return false;
					}
					break;
				case 0x2:
					if (!cmd->value(i)->isBoolean()) {
						y2error ("YGWizard: expected boolean as the %d argument for the '%s'"
						         " command.", i+1, func);
						return false;
					}
					break;
			}
		return true;
	}
	return false;
}
#define getStdStringArg(cmd, arg) (cmd->value(arg)->asString()->value())
#define getCStringArg(cmd, arg) (cmd->value(arg)->asString()->value_cstr())
#define getBoolArg(cmd, arg) (cmd->value(arg)->asBoolean()->value())
#define getAnyArg(cmd, arg) (cmd->value(arg))

#define enable_widget(enable, widget) \
	(enable ? gtk_widget_show (widget) : gtk_widget_hide (widget))

	YCPValue command (const YCPTerm &cmd)
	{
		IMPL
		if (m_verboseCommands)
			y2milestone ("Processing wizard command: %s\n", cmd->name().c_str());

		if (isCommand (cmd, "SetHelpText", 1, 0x1))
			ygtk_richtext_set_text (YGTK_RICHTEXT (m_help_widget),
			                        getCStringArg (cmd, 0), FALSE);

		else if (isCommand (cmd, "AddTreeItem", 3, 0x111)) {
			string tree_id = getStdStringArg (cmd, 0);
			GtkTreeIter *parent_iter, iter;
			GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (m_tree_widget));

			if (tree_id.empty())
				parent_iter = NULL;
			else {
				TreeId::iterator it = tree_ids.find (tree_id);
				if (it == tree_ids.end()) {
					y2error ("YGWizard: there is no tree item with id '%s'", tree_id.c_str());
					return YCPBoolean (false);
				}
				GtkTreePath *path = it->second;
				parent_iter = new GtkTreeIter;
				gtk_tree_model_get_iter (model, parent_iter, path);
			}

			gtk_tree_store_append (GTK_TREE_STORE (model), &iter, parent_iter);
			gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 0, getCStringArg (cmd, 1), -1);

			tree_ids [getStdStringArg (cmd, 2)] = gtk_tree_model_get_path (model, &iter);
			delete parent_iter;
		}
		else if (isCommand (cmd, "DeleteTreeItems", 0, 0)) {
			gtk_tree_store_clear
				(GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (m_tree_widget))));
			clear_tree_ids();
		}
		else if (isCommand (cmd, "SelectTreeItem", 1, 0x1)) {
			TreeId::iterator it = tree_ids.find (getStdStringArg (cmd, 0));
			if (it == tree_ids.end()) {
				y2error ("YGWizard: there is no tree item with id '%s'", getCStringArg (cmd, 0));
				return YCPBoolean (false);
			}
			GtkTreePath *path = it->second;
			gtk_tree_view_expand_to_path (GTK_TREE_VIEW (m_tree_widget), path);
			gtk_tree_view_set_cursor (GTK_TREE_VIEW (m_tree_widget), path, NULL, FALSE);
		}

		else if (isCommand (cmd, "SetDialogHeading", 1, 0x1))
			gtk_label_set_text (GTK_LABEL (m_title_label), getCStringArg (cmd, 0));
		else if (isCommand (cmd, "SetDialogIcon", 1, 0x1)) {
			GError *error = 0;
			GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (getCStringArg (cmd, 0), &error);
			if (pixbuf) {
				gtk_image_set_from_pixbuf (GTK_IMAGE (m_title_image), pixbuf);
				g_object_unref (G_OBJECT (pixbuf));
			}
			else
				y2warning ("YGWizard: could not load image: %s", getCStringArg (cmd, 0));
		}

		else if (isCommand (cmd, "SetAbortButtonLabel", 1, 0x1) ||
		         isCommand (cmd, "SetCancelButtonLabel", 1, 0x1)) {
			string str = YGUtils::mapKBAccel(getCStringArg (cmd, 0));
			gtk_button_set_label (GTK_BUTTON (m_abort_button), str.c_str());
		}
		else if (isCommand (cmd, "SetBackButtonLabel", 1, 0x1)) {
			string str = YGUtils::mapKBAccel(getCStringArg (cmd, 0));
			gtk_button_set_label (GTK_BUTTON (m_back_button), str.c_str());
		}
		else if (isCommand (cmd, "SetNextButtonLabel", 1, 0x1) ||
		         isCommand (cmd, "SetAcceptButtonLabel", 1, 0x1)) {
			string str = YGUtils::mapKBAccel(getCStringArg (cmd, 0));
			gtk_button_set_label (GTK_BUTTON (m_next_button), str.c_str());
		}

		else if (isCommand (cmd, "SetAbortButtonID", 1, 0x0))
			g_object_set_data_full (G_OBJECT (m_abort_button), "id",
			                        new YCPValue (getAnyArg (cmd, 0)), delete_data_cb);
		else if (isCommand (cmd, "SetBackButtonID", 1, 0x0))
			g_object_set_data_full (G_OBJECT (m_back_button), "id",
			                        new YCPValue (getAnyArg (cmd, 0)), delete_data_cb);
		else if (isCommand (cmd, "SetNextButtonID", 1, 0x0))
			g_object_set_data_full (G_OBJECT (m_next_button), "id",
			                        new YCPValue (getAnyArg (cmd, 0)), delete_data_cb);

		else if (isCommand (cmd, "EnableAbortButton", 1, 0x2))
			enable_widget (getBoolArg (cmd, 0), m_abort_button);
		else if (isCommand (cmd, "EnableBackButton", 1, 0x2))
			enable_widget (getBoolArg (cmd, 0), m_back_button);
		else if (isCommand (cmd, "EnableNextButton", 1, 0x2))
			enable_widget (getBoolArg (cmd, 0), m_next_button);

		else if (isCommand (cmd, "ProtectNextButton", 1, 0x2))
			g_object_set_data (G_OBJECT (m_next_button), "protect",
			                   GINT_TO_POINTER (getBoolArg (cmd, 0)));

		else if (isCommand (cmd, "SetFocusToNextButton", 0, 0))
			gtk_widget_grab_focus (m_next_button);
		else if (isCommand (cmd, "SetFocusToBackButton", 0, 0))
			gtk_widget_grab_focus (m_back_button);

		else if (isCommand (cmd, "AddMenu", 2, 0x11)) {
			string str = YGUtils::mapKBAccel(getCStringArg (cmd, 0));
			GtkWidget *entry = gtk_menu_item_new_with_mnemonic (str.c_str());

			gtk_menu_shell_append (GTK_MENU_SHELL (m_menu), entry);

			GtkWidget *submenu = gtk_menu_new();
			gtk_menu_item_set_submenu (GTK_MENU_ITEM (entry), submenu);

			menu_ids [getStdStringArg (cmd, 1)] = submenu;
			gtk_widget_show_all (m_menu);
		}
		else if (isCommand (cmd, "AddMenuEntry", 3, 0x111)) {
			MenuId::iterator it = menu_ids.find (getStdStringArg (cmd, 0));
			if (it == menu_ids.end()) {
				y2error ("YGWizard: there is no menu item with id '%s'", getCStringArg (cmd, 0));
				return YCPBoolean (false);
			}
			GtkWidget *parent = it->second;

			string str = YGUtils::mapKBAccel(getCStringArg (cmd, 1));
			GtkWidget *entry = gtk_menu_item_new_with_mnemonic (str.c_str());

			gtk_menu_shell_append (GTK_MENU_SHELL (parent), entry);

			gtk_widget_show (entry);
			g_signal_connect_data (G_OBJECT (entry), "activate",
			                       G_CALLBACK (selected_menu_item_cb),
			                       (void*) g_strdup (getCStringArg (cmd, 2)),
			                       free_data_cb, GConnectFlags (0));
		}
		else if (isCommand (cmd, "AddSubMenu", 3, 0x111)) {
			MenuId::iterator it = menu_ids.find (getStdStringArg (cmd, 0));
			if (it == menu_ids.end()) {
				y2error ("YGWizard: there is no menu with id %s.", getCStringArg (cmd, 0));
				return YCPBoolean (false);
			}
			GtkWidget *parent = it->second;

			string str = YGUtils::mapKBAccel(getCStringArg (cmd, 1));
			GtkWidget *entry = gtk_menu_item_new_with_mnemonic (str.c_str());

			gtk_menu_shell_append (GTK_MENU_SHELL (parent), entry);

			GtkWidget *submenu = gtk_menu_new();
			gtk_menu_item_set_submenu (GTK_MENU_ITEM (entry), submenu);

			menu_ids [getStdStringArg (cmd, 2)] = submenu;
			gtk_widget_show_all (entry);
		}
		else if (isCommand (cmd, "AddMenuSeparator", 1, 0x1)) {
			MenuId::iterator it = menu_ids.find (getStdStringArg (cmd, 0));
			if (it == menu_ids.end()) {
				y2error ("YGWizard: there is no menu with id %s.", getCStringArg (cmd, 0));
				return YCPBoolean (false);
			}
			GtkWidget *parent = it->second;

			GtkWidget *separator = gtk_separator_menu_item_new();
			gtk_menu_shell_append (GTK_MENU_SHELL (parent), separator);
			gtk_widget_show (separator);
		}

		else if (isCommand (cmd, "SetVerboseCommands", 1, 0x2))
			m_verboseCommands = getBoolArg (cmd, 0);
		else if (isCommand (cmd, "Ping", 0, 0))
			y2debug ("YGWizard is active");

		else if (isCommand (cmd, "AddStepHeading", 1, 0x1))
			ygtk_steps_append_heading (YGTK_STEPS (m_steps_widget), getCStringArg (cmd, 0));
		else if (isCommand (cmd, "AddStep", 2, 0x11)) {
			steps_ids [getStdStringArg (cmd, 1)]
				= ygtk_steps_append (YGTK_STEPS (m_steps_widget), getCStringArg (cmd, 0));
		}
		else if (isCommand (cmd, "SetCurrentStep", 1, 0x1)) {
			StepId::iterator it = steps_ids.find (getStdStringArg (cmd, 0));
			if (it == steps_ids.end()) {
				y2error ("YGWizard: there is no step with id %s.", getCStringArg (cmd, 0));
				return YCPBoolean (false);
			}
			ygtk_steps_set_current (YGTK_STEPS (m_steps_widget), it->second);
		}
		else if (isCommand (cmd, "UpdateSteps", 0, 0))
			;
		else if (isCommand (cmd, "DeleteSteps", 0, 0)) {
			ygtk_steps_clear (YGTK_STEPS (m_steps_widget));
			steps_ids.clear();
		}

		else if (isCommand (cmd, "ShowReleaseNotesButton", 2, 0x01)) {
			string label = YGUtils::mapKBAccel(getCStringArg (cmd, 0));
			gtk_button_set_label (GTK_BUTTON (m_release_notes_button), label.c_str());
			gtk_button_set_use_underline (GTK_BUTTON (m_release_notes_button), TRUE);
			g_object_set_data_full (G_OBJECT (m_release_notes_button), "id",
			                        new YCPValue (getAnyArg (cmd, 1)), delete_data_cb);
			gtk_widget_show (m_release_notes_button);
		}
		else if (isCommand (cmd, "HideReleaseNotesButton", 0, 0))
			gtk_widget_hide (m_release_notes_button);

		else if (isCommand (cmd, "RetranslateInternalButtons", 0, 0))
			;  // NOTE: we don't need this as we don't use switch buttons

		else {
			y2error ("Unsupported wizard command (or invalid arguments): %s\n",
			         cmd->name().c_str());
			return YCPBoolean (false);
		}
		return YCPBoolean (true);
	}

	YCPString currentTreeSelection()
	{
		GtkTreePath *path;
		GtkTreeViewColumn *column;
		gtk_tree_view_get_cursor (GTK_TREE_VIEW (m_tree_widget), &path, &column);
		if (path == NULL || column == NULL)
			return YCPString ("");

		/* A bit cpu-consuming, but saving the strings is something I'd rather avoid. */
		gint* path_depth = gtk_tree_path_get_indices (path);
		for (TreeId::iterator it = tree_ids.begin(); it != tree_ids.end(); it++) {
			gint* it_depth = gtk_tree_path_get_indices (it->second);
			if (gtk_tree_path_get_depth (path) == gtk_tree_path_get_depth (it->second)) {
				int i;
				for (i = 0; i < gtk_tree_path_get_depth (path) &&
				            it_depth[i] == path_depth[i]; i++) ;
				if (i == gtk_tree_path_get_depth (path))
					return it->first;
			}
		}
		g_warning ("YGWizard: internal error: current selection doesn't match any id");
		return YCPString ("");
	}

	// nicesize, but only for the GTK widgets
	long size_request (YUIDimension dim)
	{
		GtkRequisition requisition;
		long request = 0;

		if (dim == YD_HORIZ) {
			if (m_pane_box) {
				gtk_widget_size_request (m_pane_box, &requisition);
				request += requisition.width
				        + gtk_container_get_border_width (GTK_CONTAINER (m_pane_box));
			}
			gtk_widget_size_request (m_help_vbox, &requisition);
			request += requisition.width
			        + gtk_container_get_border_width (GTK_CONTAINER (m_title_hbox))
			        + CONTENT_PADDING * 2;
			request += MAIN_BORDER * 2;
		}
		else {
			gtk_widget_size_request (m_title_hbox, &requisition);
			request += requisition.height
			        + gtk_container_get_border_width (GTK_CONTAINER (m_title_hbox))
			        + CONTENT_PADDING * 2;
			request += MAIN_BORDER * 2;
			gtk_widget_size_request (m_button_box, &requisition);
			request += requisition.height
			        + gtk_container_get_border_width (GTK_CONTAINER (m_button_box));
		}
		return request;
	}

	// FIXME: this doesn't seem to get called ?!
	virtual long nicesize (YUIDimension dim)
	{
		IMPL
printf("ygwizard nicesize requested\n");
		return child (0)->nicesize(dim) + size_request (dim);
	}

	virtual void setSize (long width, long height)
	{
		IMPL
		doSetSize (width, height);

		width  -= size_request (YD_HORIZ);
		height -= size_request (YD_VERT);
		child (0)->setSize (width, height);
	}

	virtual void setEnabling (bool enabled)
	{
		IMPL
		// FIXME: this chains it all, no?
		gtk_widget_set_sensitive (getWidget(), enabled);

		bool protectNextButton =
			GPOINTER_TO_INT (g_object_get_data (G_OBJECT (m_next_button),  "protect"));
		if (!enabled && protectNextButton)
			gtk_widget_set_sensitive (m_next_button, TRUE);
	}

	// callbacks
	static void program_pane_resized_cb (GtkWidget *widget, GtkAllocation *allocation,
	                                     YGWizard *pThis)
	{
		// When the user moves the GtkPaned, we need to tell our children
		// a new size.
		allocation = &pThis->m_fixed->allocation;
		pThis->child (0)->setSize (allocation->width, allocation->height);
	}

	static void set_help_background_cb (GtkWidget *widget, YGWizard *pThis)
	{
		// FIXME: we need to set some THEMEDIR on the configure process to avoid
		// absolute paths
		ygtk_richtext_set_background (YGTK_RICHTEXT (widget),
			"/usr/share/YaST2/theme/SuSELinux/wizard/help-background.png");
	}

	static gboolean expose_event_cb (GtkWidget *widget, GdkEventExpose *event,
	                                 YGWizard *pThis)
	{
		// Let's paint the square boxes
		// (a filled for the title and a stroke around the content area)
		int x, y, w, h;
		cairo_t *cr = gdk_cairo_create (widget->window);
		gdk_cairo_set_source_color (cr, &widget->style->bg[GTK_STATE_SELECTED]);

		GtkWidget *title_widget = pThis->m_title_hbox;
		GtkWidget *help_box     = pThis->m_help_vbox;

		// title
		x = title_widget->allocation.x;
		y = title_widget->allocation.y;
		w = title_widget->allocation.width;
		h = title_widget->allocation.height;

		cairo_rectangle (cr, x, y, w, h);
		cairo_fill (cr);

		// content area
		x += 1; w -= 2;
		y += h;
		h = help_box->allocation.height + CONTENT_PADDING*2;

		cairo_rectangle (cr, x, y, w, h);
		cairo_stroke (cr);

		cairo_destroy (cr);

		GtkContainer *container = GTK_CONTAINER (widget);
		gtk_container_propagate_expose (container, pThis->m_main_hbox, event);

		return TRUE;
	}

	static void button_clicked_cb (GtkButton *button, YGWizard *pThis)
	{
		YCPValue *id = (YCPValue *) g_object_get_data (G_OBJECT (button), "id");
		YGUI::ui()->sendEvent (new YMenuEvent (*id));
	}

	static void delete_data_cb (gpointer data)
	{ delete (YCPValue*) data; }
	static void free_data_cb (gpointer data, GClosure *closure)
	{ g_free ((gchar*) data); }

	static void selected_menu_item_cb (GtkMenuItem *item, const char* id)
	{
		YGUI::ui()->sendEvent (new YMenuEvent (YCPValue (YCPString (id))));
	}

	static void tree_item_selected_cb (GtkTreeView *tree_view, YGWizard *pThis)
	{
		YCPString id = pThis->currentTreeSelection();
		if (!id->value().empty())
			YGUI::ui()->sendEvent (new YMenuEvent (YCPValue (id)));
	}
};

YWidget *
YGUI::createWizard (YWidget *parent, YWidgetOpt &opt,
	const YCPValue &backButtonId,  const YCPString &backButtonLabel,
	const YCPValue &abortButtonId, const YCPString &abortButtonLabel,
	const YCPValue &nextButtonId,  const YCPString &nextButtonLabel)
{
	return new YGWizard (opt, YGWidget::get (parent),
	                     backButtonId,  backButtonLabel,
	                     abortButtonId, abortButtonLabel,
	                     nextButtonId,  nextButtonLabel);
}
