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

class YGWizard : public YWizard, public YGWidget
{
	// Layout boxes
	GtkWidget *m_heading_box, *m_main_box, *m_buttons_box;  // vertical
	GtkWidget *m_help_box;  // horizontals

	// The menu
	GtkWidget *m_menu;
	typedef map <string, GtkWidget*> MenuId;
	MenuId menu_ids;

	// Headings
	GtkWidget *m_heading_image, *m_heading_label;

	// Containee
	GtkWidget *m_fixed;

	// Help box
	GtkWidget *m_help_frame, *m_help_text, *m_help_tree, *m_help_steps;

	typedef map <string, GtkTreePath*> TreeId;
	TreeId tree_ids;
	typedef map <string, guint> StepId;
	StepId steps_ids;

	bool m_treeEnabled, m_stepsEnabled;

	// Bottom buttons
	GtkWidget *backButton, *abortButton, *nextButton;
	bool m_protectNextButton;

	// Miscellaneous
	bool m_verboseCommands;
	GtkWidget *m_releaseNotesButton;

public:
	YGWizard (const YWidgetOpt &opt, YGWidget *parent,
	          const YCPValue &backButtonId,  const YCPString &backButtonLabel,
	          const YCPValue &abortButtonId, const YCPString &abortButtonLabel,
	          const YCPValue &nextButtonId,  const YCPString &nextButtonLabel)
	: YWizard (opt, backButtonId, backButtonLabel,
	                abortButtonId, abortButtonLabel,
	                nextButtonId, nextButtonLabel),
	  YGWidget (this, parent, true, GTK_TYPE_VBOX, NULL)
	{
		IMPL
		m_stepsEnabled = opt.stepsEnabled.value();
		m_treeEnabled  = opt.treeEnabled.value();

		m_verboseCommands = true;  // for now...

		//** A menu bar
		m_menu = gtk_menu_bar_new();

		//** Heading space
		m_heading_box = gtk_vbox_new (FALSE, 2);
		GtkWidget *title_box = gtk_hbox_new (FALSE, 0);
		GtkWidget *line = gtk_hseparator_new();

		m_heading_image = gtk_image_new();
		m_heading_label = gtk_label_new ("");

		// set a strong font to the heading label
		PangoFontDescription* font = pango_font_description_new();
		pango_font_description_set_weight (font, PANGO_WEIGHT_HEAVY);
		pango_font_description_set_size   (font, 16*PANGO_SCALE);
		gtk_widget_modify_font (m_heading_label, font);
		pango_font_description_free (font);

		gtk_container_add (GTK_CONTAINER (title_box), m_heading_image);
		gtk_container_add (GTK_CONTAINER (title_box), m_heading_label);
		gtk_box_set_child_packing (GTK_BOX (title_box), m_heading_image,
		                           FALSE, FALSE, 0, GTK_PACK_START);

		gtk_container_add (GTK_CONTAINER (m_heading_box), title_box);
		gtk_container_add (GTK_CONTAINER (m_heading_box), line);

		//** Main widgets go here
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

		//** Help space
		GtkWidget *m_help_box = gtk_vbox_new (FALSE, 0);
		m_help_frame = gtk_frame_new (NULL);
		gtk_frame_set_shadow_type (GTK_FRAME (m_help_frame), GTK_SHADOW_IN);
		gtk_widget_set_size_request (m_help_box, 250, -1);
		gtk_container_add (GTK_CONTAINER (m_help_box), m_help_frame);

		m_help_text = ygtk_richtext_new();
		g_object_ref (G_OBJECT (m_help_text));  // to survive container removals

		m_help_steps = m_help_tree = NULL;
		if (m_treeEnabled) {
			m_help_tree = gtk_tree_view_new_with_model
				(GTK_TREE_MODEL (gtk_tree_store_new (1, G_TYPE_STRING)));
			gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (m_help_tree),
				0, "(no title)", gtk_cell_renderer_text_new(), "text", 0, NULL);
			gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (m_help_tree), FALSE);
			g_signal_connect (G_OBJECT (m_help_tree), "cursor-changed",
			                  G_CALLBACK (tree_item_selected_cb), this);

			g_object_ref (G_OBJECT (m_help_tree));
			setHelpBarWidget (m_help_tree);
		}
		if (m_stepsEnabled) {
			m_help_steps = ygtk_steps_new();
			g_object_ref (G_OBJECT (m_help_steps));
			setHelpBarWidget (m_help_steps);
		}

		if (m_treeEnabled || m_stepsEnabled) {
			// create a switch button
			GtkWidget *switch_button = gtk_button_new_with_mnemonic ("Help");
			gtk_container_add (GTK_CONTAINER (m_help_box), switch_button);
			gtk_box_set_child_packing (GTK_BOX (m_help_box), switch_button,
			                           FALSE, FALSE, 10, GTK_PACK_END);
			g_signal_connect (G_OBJECT (switch_button), "clicked",
			                  G_CALLBACK (switch_button_clicked_cb), this);
		}
		else
			setHelpBarWidget (m_help_text);

		m_releaseNotesButton = NULL;

		//** Adding the bottom buttons
		/* NOTE: we can't really use gtk_button_new_from_stock() because the
		   nextButton can really be a "Ok" button. Standardize ids is really the
		   way to go. */
		string backLabel  = YGUtils::mapKBAccel(backButtonLabel->value_cstr());
		string abortLabel = YGUtils::mapKBAccel(abortButtonLabel->value_cstr());
		string nextLabel  = YGUtils::mapKBAccel(nextButtonLabel->value_cstr());

		backButton  = gtk_button_new_with_mnemonic (backLabel.c_str());
		abortButton = gtk_button_new_with_mnemonic (abortLabel.c_str());
		nextButton  = gtk_button_new_with_mnemonic (nextLabel.c_str());
		m_protectNextButton = false;
		g_object_set_data_full (G_OBJECT (backButton),  "id",
		                        new YCPValue (backButtonId), delete_data_cb);
		g_object_set_data_full (G_OBJECT (abortButton), "id",
		                        new YCPValue (abortButtonId), delete_data_cb);
		g_object_set_data_full (G_OBJECT (nextButton),  "id",
		                        new YCPValue (nextButtonId), delete_data_cb);

		g_signal_connect (G_OBJECT (backButton), "clicked",
		                  G_CALLBACK (button_clicked_cb), this);
		g_signal_connect (G_OBJECT (abortButton), "clicked",
		                  G_CALLBACK (button_clicked_cb), this);
		g_signal_connect (G_OBJECT (nextButton), "clicked",
		                  G_CALLBACK (button_clicked_cb), this);

		GtkWidget *m_buttons_box = gtk_hbutton_box_new();
		gtk_container_add (GTK_CONTAINER (m_buttons_box), backButton);
		gtk_container_add (GTK_CONTAINER (m_buttons_box), abortButton);
		gtk_container_add (GTK_CONTAINER (m_buttons_box), nextButton);

		//** Setting the main layout
		m_main_box = gtk_hbox_new (FALSE, 5);
		gtk_container_add (GTK_CONTAINER (m_main_box), m_help_box);
		gtk_box_set_child_packing (GTK_BOX (getWidget()), m_help_box,
		                           FALSE, FALSE, 2, GTK_PACK_START);
		gtk_container_add (GTK_CONTAINER (m_main_box), m_fixed);

		//** Setting the whole layout
		gtk_container_add (GTK_CONTAINER (getWidget()), m_menu);
		gtk_box_set_child_packing (GTK_BOX (getWidget()), m_menu,
		                           FALSE, FALSE, 0, GTK_PACK_START);
		gtk_container_add (GTK_CONTAINER (getWidget()), m_heading_box);
		gtk_box_set_child_packing (GTK_BOX (getWidget()), m_heading_box,
		                           FALSE, FALSE, 2, GTK_PACK_START);
		gtk_container_add (GTK_CONTAINER (getWidget()), m_main_box);
		gtk_container_add (GTK_CONTAINER (getWidget()), m_buttons_box);
		gtk_box_set_child_packing (GTK_BOX (getWidget()), m_buttons_box,
		                           FALSE, FALSE, 2, GTK_PACK_END);
		gtk_widget_show_all (getWidget());

		//** Some should be invisible until initializated
		gtk_widget_hide (m_menu);
		// FIXME: this doesn't seem to work ?!
		if (backLabel.empty())  gtk_widget_hide (backButton);
		if (abortLabel.empty()) gtk_widget_hide (abortButton);
		if (nextLabel.empty())  gtk_widget_hide (nextButton);
	}

	void clear_tree_ids()
	{
		for (TreeId::iterator it = tree_ids.begin(); it != tree_ids.end(); it++)
			gtk_tree_path_free (it->second);
		tree_ids.clear();
	}

	virtual ~YGWizard()
	{
		clear_tree_ids();
		// we need to manually delete them, as only one is in the container
		// at the moment
		// FIXME: do we also need to call gtk_widget_destroy() ?
		g_object_unref (G_OBJECT (m_help_text));
		if (m_treeEnabled)
			g_object_unref (G_OBJECT (m_help_tree));
		if (m_stepsEnabled)
			g_object_unref (G_OBJECT (m_help_steps));
	}

	virtual GtkFixed* getFixed()
	{
		IMPL
		return GTK_FIXED (m_fixed);
	}

// TODO: some sanity checks might be nice
#define isCommand(cmd, str) (cmd->name() == str)
#define getStdStringArg(cmd, arg) (cmd->value(arg)->asString()->value())
#define getCStringArg(cmd, arg) (cmd->value(arg)->asString()->value_cstr())
#define getBoolArg(cmd, arg) (cmd->value(arg)->asBoolean()->value())
#define getAnyArg(cmd, arg) (cmd->value(arg))

#define enable_widget(enable, widget) \
	(enable ? gtk_widget_show (widget) : gtk_widget_hide (widget))

	YCPValue command (const YCPTerm &cmd)
	{
		IMPL
		if (isCommand (cmd, "SetHelpText"))
			ygtk_richtext_set_text (YGTK_RICHTEXT (m_help_text), getCStringArg (cmd, 0), FALSE);

		else if (isCommand (cmd, "AddTreeItem")) {
			string tree_id = getStdStringArg (cmd, 0);
			GtkTreeIter *parent_iter, iter;
			GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (m_help_tree));

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
		else if (isCommand (cmd, "DeleteTreeItems")) {
			gtk_tree_store_clear
				(GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (m_help_tree))));
			clear_tree_ids();
		}
		else if (isCommand (cmd, "SelectTreeItem")) {
			TreeId::iterator it = tree_ids.find (getStdStringArg (cmd, 0));
			if (it == tree_ids.end()) {
				y2error ("YGWizard: there is no tree item with id '%s'", getCStringArg (cmd, 0));
				return YCPBoolean (false);
			}
			GtkTreePath *path = it->second;
			gtk_tree_view_expand_to_path (GTK_TREE_VIEW (m_help_tree), path);
			gtk_tree_view_set_cursor (GTK_TREE_VIEW (m_help_tree), path, NULL, FALSE);
		}

		else if (isCommand (cmd, "SetDialogHeading"))
			gtk_label_set_text (GTK_LABEL (m_heading_label), getCStringArg (cmd, 0));
		else if (isCommand (cmd, "SetDialogIcon")) {
			GError *error = 0;
			GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (getCStringArg (cmd, 0), &error);
			if (pixbuf) {
				gtk_image_set_from_pixbuf (GTK_IMAGE (m_heading_image), pixbuf);
				g_object_unref (G_OBJECT (pixbuf));
			}
			else
				y2warning ("YGWizard: could not load image: %s", getCStringArg (cmd, 0));
		}

		else if (isCommand (cmd, "SetAbortButtonLabel") ||
		         isCommand (cmd, "SetCancelButtonLabel")) {
			string str = YGUtils::mapKBAccel(getCStringArg (cmd, 0));
			gtk_button_set_label (GTK_BUTTON (abortButton), str.c_str());
		}
		else if (isCommand (cmd, "SetBackButtonLabel")) {
			string str = YGUtils::mapKBAccel(getCStringArg (cmd, 0));
			gtk_button_set_label (GTK_BUTTON (backButton), str.c_str());
		}
		else if (isCommand (cmd, "SetNextButtonLabel") ||
		         isCommand (cmd, "SetAcceptButtonLabel")) {
			string str = YGUtils::mapKBAccel(getCStringArg (cmd, 0));
			gtk_button_set_label (GTK_BUTTON (nextButton), str.c_str());
		}

		else if (isCommand (cmd, "SetAbortButtonID"))
			g_object_set_data_full (G_OBJECT (abortButton), "id",
			                        new YCPValue (getAnyArg (cmd, 0)), delete_data_cb);
		else if (isCommand (cmd, "SetBackButtonID"))
			g_object_set_data_full (G_OBJECT (backButton), "id",
			                        new YCPValue (getAnyArg (cmd, 0)), delete_data_cb);
		else if (isCommand (cmd, "SetNextButtonID"))
			g_object_set_data_full (G_OBJECT (nextButton), "id",
			                        new YCPValue (getAnyArg (cmd, 0)), delete_data_cb);

		else if (isCommand (cmd, "EnableAbortButton"))
			enable_widget (getBoolArg (cmd, 0), abortButton);
		else if (isCommand (cmd, "EnableBackButton"))
			enable_widget (getBoolArg (cmd, 0), backButton);
		else if (isCommand (cmd, "EnableNextButton"))
			enable_widget (getBoolArg (cmd, 0), nextButton);

		else if (isCommand (cmd, "ProtectNextButton"))
			m_protectNextButton = getBoolArg (cmd, 0);

		else if (isCommand (cmd, "SetFocusToNextButton"))
			gtk_widget_grab_focus (nextButton);
		else if (isCommand (cmd, "SetFocusToBackButton"))
			gtk_widget_grab_focus (backButton);

		else if (isCommand (cmd, "AddMenu")) {
			string str = YGUtils::mapKBAccel(getCStringArg (cmd, 0));
			GtkWidget *entry = gtk_menu_item_new_with_mnemonic (str.c_str());

			gtk_menu_shell_append (GTK_MENU_SHELL (m_menu), entry);

			GtkWidget *submenu = gtk_menu_new();
			gtk_menu_item_set_submenu (GTK_MENU_ITEM (entry), submenu);

			menu_ids [getStdStringArg (cmd, 1)] = submenu;
			gtk_widget_show_all (m_menu);
		}
		else if (isCommand (cmd, "AddMenuEntry")) {
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
			g_signal_connect (G_OBJECT (entry), "activate",
			                  G_CALLBACK (selected_menu_item_cb),
			                  (void*) getCStringArg (cmd, 2));
		}
		else if (isCommand (cmd, "AddSubMenu")) {
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
		else if (isCommand (cmd, "AddMenuSeparator")) {
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

		else if (isCommand (cmd, "SetVerboseCommands"))
			m_verboseCommands = getBoolArg (cmd, 0);
		else if (isCommand (cmd, "Ping"))
			y2debug ("YGWizard is active");

		else if (isCommand (cmd, "AddStepHeading"))
			ygtk_steps_append_heading (YGTK_STEPS (m_help_steps), getCStringArg (cmd, 0));
		else if (isCommand (cmd, "AddStep"))
			steps_ids [getStdStringArg (cmd, 1)]
				= ygtk_steps_append (YGTK_STEPS (m_help_steps), getCStringArg (cmd, 0));
		else if (isCommand (cmd, "SetCurrentStep")) {
			StepId::iterator it = steps_ids.find (getStdStringArg (cmd, 0));
			if (it == steps_ids.end()) {
				y2error ("YGWizard: there is no step with id %s.", getCStringArg (cmd, 0));
				return YCPBoolean (false);
			}
			ygtk_steps_set_current (YGTK_STEPS (m_help_steps), it->second);
		}
		else if (isCommand (cmd, "UpdateSteps"))
			;
		else if (isCommand (cmd, "DeleteSteps"))
			ygtk_steps_clear (YGTK_STEPS (m_help_steps));

		else if (isCommand (cmd, "ShowReleaseNotesButton")) {
			if (!m_stepsEnabled)
				y2error ("YGWizard: Release button only possible when tree enabled");

			if (!m_releaseNotesButton) {
				string label = YGUtils::mapKBAccel(getCStringArg (cmd, 0));
				m_releaseNotesButton = gtk_button_new_with_mnemonic (label.c_str());
				g_object_set_data_full (G_OBJECT (m_releaseNotesButton), "id",
				                        new YCPValue (getAnyArg (cmd, 1)), delete_data_cb);
				g_signal_connect (G_OBJECT (m_releaseNotesButton), "clicked",
				                  G_CALLBACK (button_clicked_cb), this);
			}
			gtk_container_add (GTK_CONTAINER (m_help_steps), m_releaseNotesButton);
			gtk_box_set_child_packing (GTK_BOX (m_help_steps), m_releaseNotesButton,
			                           FALSE, FALSE, 2, GTK_PACK_END);
			gtk_widget_show (m_releaseNotesButton);
		}
		else if (isCommand (cmd, "HideReleaseNotesButton")) {
			gtk_widget_destroy (m_releaseNotesButton);
			m_releaseNotesButton = NULL;
		}

#if 0
		// to implement:
		else if (isCommand (cmd, "RetranslateInternalButtons")) {}
#endif

		else {
			y2error ("Unsupported wizard command: %s\n", cmd->name().c_str());
			return YCPBoolean (false);
		}

		if (m_verboseCommands)
			y2milestone ("Processed wizard command: %s\n", cmd->name().c_str());
		return YCPBoolean (true);
	}

	// YGWidget
	YGWIDGET_IMPL_NICESIZE
	virtual void setSize (long width, long height)
	{
		doSetSize (width, height);

		GtkRequisition requisition;
		width -= 250;  // for the help box

		gtk_widget_size_request (m_heading_box, &requisition);
		height -= requisition.height;
		gtk_widget_size_request (m_buttons_box, &requisition);
		height -= requisition.height;

		child (0)->setSize (width, height);
	}

	virtual void setEnabling (bool enabled)
	{
		IMPL
		gtk_widget_set_sensitive (getWidget(), enabled);
		if (m_protectNextButton)
			gtk_widget_set_sensitive (nextButton, TRUE);
	}

	// callbacks
	static void button_clicked_cb (GtkButton *button, YGWizard *pThis)
	{
		YCPValue *id = (YCPValue *) g_object_get_data (G_OBJECT (button), "id");
		YGUI::ui()->sendEvent (new YMenuEvent (*id));
	}

	void setHelpBarWidget (GtkWidget* widget)
	{
		GtkWidget* current;
		if ((current = gtk_bin_get_child (GTK_BIN (m_help_frame))))
			gtk_container_remove (GTK_CONTAINER (m_help_frame), current);
		gtk_container_add (GTK_CONTAINER (m_help_frame), widget);
		gtk_widget_show (widget);
	}

	static void switch_button_clicked_cb (GtkButton *button, YGWizard *pThis)
	{
		string label = string (gtk_button_get_label (button));
		if (label == "Tree") {
			pThis->setHelpBarWidget (pThis->m_help_tree);
			if (pThis->m_stepsEnabled)
				gtk_button_set_label (button, "Steps");
			else
				gtk_button_set_label (button, "Help");
		}
		else if (label == "Help") {
			pThis->setHelpBarWidget (pThis->m_help_text);
			if (pThis->m_treeEnabled)
				gtk_button_set_label (button, "Tree");
			else
				gtk_button_set_label (button, "Steps");
		}
		else if (label == "Steps") {
			pThis->setHelpBarWidget (pThis->m_help_steps);
			gtk_button_set_label (button, "Help");
		}
	}

	static void delete_data_cb (gpointer data)
	{
		delete (YCPValue*) data;
	}

	static void selected_menu_item_cb (GtkMenuItem *item, const char* id)
	{
		YGUI::ui()->sendEvent (new YMenuEvent (YCPValue (YCPString (id))));
	}

	YCPString currentTreeSelection()
	{
		GtkTreePath *path;
		GtkTreeViewColumn *column;
		gtk_tree_view_get_cursor (GTK_TREE_VIEW (m_help_tree), &path, &column);
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
