/* Yast-GTK */
#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "YGUtils.h"
#include "YGLayout.h"
#include "ygtkwizard.h"
#include "YWizard.h"

class YGWizard : public YWizard, public YGWidget
{
	GtkWidget *m_fixed;
	bool m_verboseCommands;

public:
	YGWizard (const YWidgetOpt &opt, YGWidget *parent,
	          const YCPValue &backButtonId,  const YCPString &backButtonLabel,
	          const YCPValue &abortButtonId, const YCPString &abortButtonLabel,
	          const YCPValue &nextButtonId,  const YCPString &nextButtonLabel)
	: YWizard (opt, backButtonId, backButtonLabel,
	                abortButtonId, abortButtonLabel,
	                nextButtonId, nextButtonLabel),
	  YGWidget (this, parent, true, YGTK_TYPE_WIZARD, NULL)
	{
		IMPL
		setBorder (0);
		m_verboseCommands = false;

		//** Application area
		m_fixed = gtk_fixed_new();
		ygtk_wizard_set_child (YGTK_WIZARD (getWidget()), m_fixed);
		gtk_widget_show (m_fixed);

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

		YGtkWizard *ygtk_wizard = YGTK_WIZARD (getWidget());

		//** Steps/tree pane
		bool steps_enabled = opt.stepsEnabled.value();
		bool tree_enabled  = opt.treeEnabled.value();
		if (steps_enabled && tree_enabled) {
			y2error ("YGWizard doesn't support both steps and tree enabled at the "
			         "same time.\nDisabling the steps...");
			steps_enabled = false;
		}
		if (steps_enabled)
			ygtk_wizard_enable_steps (ygtk_wizard);
		if (tree_enabled)
			ygtk_wizard_enable_tree (ygtk_wizard);

		//** Setting the bottom buttons
		string backLabel  = YGUtils::mapKBAccel(backButtonLabel->value_cstr());
		string abortLabel = YGUtils::mapKBAccel(abortButtonLabel->value_cstr());
		string nextLabel  = YGUtils::mapKBAccel(nextButtonLabel->value_cstr());
		ygtk_wizard_set_back_button_label (ygtk_wizard, backLabel.c_str());
		ygtk_wizard_set_abort_button_label (ygtk_wizard, abortLabel.c_str());
		ygtk_wizard_set_next_button_label (ygtk_wizard, nextLabel.c_str());

		ygtk_wizard_set_back_button_id (ygtk_wizard, new YCPValue (backButtonId),
                                    delete_data_cb);
		ygtk_wizard_set_next_button_id (ygtk_wizard, new YCPValue (nextButtonId),
                                    delete_data_cb);
		ygtk_wizard_set_abort_button_id (ygtk_wizard, new YCPValue (abortButtonId),
                                     delete_data_cb);

		//** All event are sent through this signal together with an id
		g_signal_connect (G_OBJECT (getWidget()), "action-triggered",
		                  G_CALLBACK (action_triggered_cb), this);
	}

	virtual GtkFixed* getFixed()
	{
		IMPL
		return GTK_FIXED (m_fixed);
	}

	/* The purpose of this function is to do some sanity checks, besides
	   the simple test "cmd->name() == func".
	     args_type is a reverse hexadecimal number where
	     any = 0, string = 1, boolean = 2.                             */
	static bool isCommand (const YCPTerm &cmd, const char *func,
	                       guint args_nb, guint args_type)
	{
		if (cmd->name() == func) {
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

	YCPValue command (const YCPTerm &cmd)
	{
		IMPL
		if (m_verboseCommands)
			y2milestone ("Processing wizard command: %s\n", cmd->name().c_str());

		YGtkWizard *wizard = YGTK_WIZARD (getWidget());
		if (isCommand (cmd, "SetHelpText", 1, 0x1))
			ygtk_wizard_set_help_text (wizard, getCStringArg (cmd, 0));

		else if (isCommand (cmd, "AddTreeItem", 3, 0x111)) {
			if (!ygtk_wizard_add_tree_item (wizard, getCStringArg (cmd, 0),
                       getCStringArg (cmd, 1), getCStringArg (cmd, 2))) {
				y2error ("YGWizard: there is no tree item with id '%s'",
				         getCStringArg (cmd, 0));
				return YCPBoolean (false);
			}
		}
		else if (isCommand (cmd, "DeleteTreeItems", 0, 0))
			ygtk_wizard_clear_tree (wizard);
		else if (isCommand (cmd, "SelectTreeItem", 1, 0x1)) {
			if (!ygtk_wizard_select_tree_item (wizard, getCStringArg (cmd, 0))) {
				y2error ("YGWizard: there is no tree item with id '%s'",
				         getCStringArg (cmd, 0));
				return YCPBoolean (false);
			}
		}

		else if (isCommand (cmd, "SetDialogHeading", 1, 0x1))
			ygtk_wizard_set_header_text (wizard, YGUI::ui()->currentWindow(),
			                             getCStringArg (cmd, 0));
		else if (isCommand (cmd, "SetDialogIcon", 1, 0x1)) {
			if (!ygtk_wizard_set_header_icon (wizard,
			         YGUI::ui()->currentWindow(), getCStringArg (cmd, 0))) {
				y2warning ("YGWizard: could not load image: %s", getCStringArg (cmd, 0));
				return YCPBoolean (false);
			}
		}

		else if (isCommand (cmd, "SetAbortButtonLabel", 1, 0x1) ||
		          isCommand (cmd, "SetCancelButtonLabel", 1, 0x1)) {
			string str = YGUtils::mapKBAccel(getCStringArg (cmd, 0));
			ygtk_wizard_set_abort_button_label (wizard, str.c_str());
		}
		else if (isCommand (cmd, "SetBackButtonLabel", 1, 0x1)) {
			string str = YGUtils::mapKBAccel(getCStringArg (cmd, 0));
			ygtk_wizard_set_back_button_label (wizard, str.c_str());
		}
		else if (isCommand (cmd, "SetNextButtonLabel", 1, 0x1) ||
		         isCommand (cmd, "SetAcceptButtonLabel", 1, 0x1)) {
			string str = YGUtils::mapKBAccel(getCStringArg (cmd, 0));
			ygtk_wizard_set_next_button_label (wizard, str.c_str());
		}

		else if (isCommand (cmd, "SetAbortButtonID", 1, 0x0))
			ygtk_wizard_set_abort_button_id (wizard, new YCPValue (getAnyArg (cmd, 0)),
                                       delete_data_cb);
		else if (isCommand (cmd, "SetBackButtonID", 1, 0x0))
			ygtk_wizard_set_back_button_id (wizard, new YCPValue (getAnyArg (cmd, 0)),
                                      delete_data_cb);
		else if (isCommand (cmd, "SetNextButtonID", 1, 0x0))
			ygtk_wizard_set_next_button_id (wizard, new YCPValue (getAnyArg (cmd, 0)),
                                      delete_data_cb);

		else if (isCommand (cmd, "EnableAbortButton", 1, 0x2))
			ygtk_wizard_enable_abort_button (wizard, getBoolArg (cmd, 0));
		else if (isCommand (cmd, "EnableBackButton", 1, 0x2))
			ygtk_wizard_enable_back_button (wizard, getBoolArg (cmd, 0));
		else if (isCommand (cmd, "EnableNextButton", 1, 0x2))
			ygtk_wizard_enable_next_button (wizard, getBoolArg (cmd, 0));

		else if (isCommand (cmd, "ProtectNextButton", 1, 0x2))
			ygtk_wizard_protect_next_button (wizard, getBoolArg (cmd, 0));

		else if (isCommand (cmd, "SetFocusToNextButton", 0, 0))
			ygtk_wizard_focus_next_button (wizard);
		else if (isCommand (cmd, "SetFocusToBackButton", 0, 0))
			ygtk_wizard_focus_back_button (wizard);

		else if (isCommand (cmd, "AddMenu", 2, 0x11)) {
			string str = YGUtils::mapKBAccel(getCStringArg (cmd, 0));
			ygtk_wizard_add_menu (wizard, str.c_str(), getCStringArg (cmd, 1));
		}
		else if (isCommand (cmd, "AddMenuEntry", 3, 0x111)) {
				string str = YGUtils::mapKBAccel(getCStringArg (cmd, 1));
			if (!ygtk_wizard_add_menu_entry (wizard, getCStringArg (cmd, 0),
                      str.c_str(), getCStringArg (cmd, 2))) {
				y2error ("YGWizard: there is no menu item with id '%s'",
				         getCStringArg (cmd, 0));
				return YCPBoolean (false);
			}
		}
		else if (isCommand (cmd, "AddSubMenu", 3, 0x111)) {
			string str = YGUtils::mapKBAccel(getCStringArg (cmd, 1));
			if (!ygtk_wizard_add_sub_menu (wizard, getCStringArg (cmd, 0),
                    str.c_str(), getCStringArg (cmd, 2))) {
				y2error ("YGWizard: there is no menu item with id '%s'",
				         getCStringArg (cmd, 0));
				return YCPBoolean (false);
			}
		}
		else if (isCommand (cmd, "AddMenuSeparator", 3, 0x111)) {
			if (!ygtk_wizard_add_menu_separator (wizard, getCStringArg (cmd, 0))) {
				y2error ("YGWizard: there is no menu item with id '%s'",
				         getCStringArg (cmd, 0));
				return YCPBoolean (false);
			}
		}

		else if (isCommand (cmd, "SetVerboseCommands", 1, 0x2))
			m_verboseCommands = getBoolArg (cmd, 0);
		else if (isCommand (cmd, "Ping", 0, 0))
			y2debug ("YGWizard is active");

		else if (isCommand (cmd, "AddStepHeading", 1, 0x1))
			ygtk_wizard_add_step_header (wizard, getCStringArg (cmd, 0));
		else if (isCommand (cmd, "AddStep", 2, 0x11))
			ygtk_wizard_add_step (wizard, getCStringArg (cmd, 0), getCStringArg (cmd, 1));
		else if (isCommand (cmd, "SetCurrentStep", 1, 0x1)) {
			if (!ygtk_wizard_set_current_step (wizard, getCStringArg (cmd, 0))) {
				y2error ("YGWizard: there is no step with id %s.", getCStringArg (cmd, 0));
				return YCPBoolean (false);
			}
		}
		else if (isCommand (cmd, "UpdateSteps", 0, 0))
			;
		else if (isCommand (cmd, "DeleteSteps", 0, 0))
			ygtk_wizard_clear_steps (wizard);

		else if (isCommand (cmd, "ShowReleaseNotesButton", 2, 0x01)) {
			string label = YGUtils::mapKBAccel(getCStringArg (cmd, 0));
			ygtk_wizard_set_release_notes_button_label (wizard, label.c_str(),
			                new YCPValue (getAnyArg (cmd, 1)), delete_data_cb);
		}
		else if (isCommand (cmd, "HideReleaseNotesButton", 0, 0))
			ygtk_wizard_show_release_notes_button (wizard, FALSE);

		else if (isCommand (cmd, "RetranslateInternalButtons", 0, 0))
			;  // we don't need this as we don't use switch buttons

		else {
			y2error ("Unsupported wizard command (or invalid arguments): %s\n",
			         cmd->name().c_str());
			return YCPBoolean (false);
		}
		return YCPBoolean (true);
	}

	// nicesize, but only for the GTK widget
	long size_request (YUIDimension dim)
	{
		GtkRequisition req;
		ygtk_wizard_size_request (YGTK_WIZARD (getWidget()), &req, TRUE);
printf ("wizard without child: %d x %d\n", req.width, req.height);
		return dim == YD_HORIZ ? req.width : req.height;
	}

	virtual long nicesize (YUIDimension dim)
	{
		IMPL
		long ret = size_request (dim);
		if (hasChildren())
			ret += child (0)->nicesize(dim);
		return ret;
	}

	virtual void setSize (long width, long height)
	{
		IMPL
		doSetSize (width, height);

		if (hasChildren()) {
			width  -= size_request (YD_HORIZ);
			height -= size_request (YD_VERT);
			child (0)->setSize (width, height);
		}
	}

	virtual void setEnabling (bool enable)
	{
		IMPL
		ygtk_wizard_set_sensitive (YGTK_WIZARD (getWidget()), enable);
	}

	static void action_triggered_cb (YGtkWizard *wizard, gpointer id,
	                                 gint id_type, YGWizard *pThis)
	{
		IMPL
		if ((GType) id_type == G_TYPE_STRING) {
			gchar *id_ = (gchar *) id;
			YGUI::ui()->sendEvent (new YMenuEvent (YCPValue (YCPString (id_))));
		}
		else
			YGUI::ui()->sendEvent (new YMenuEvent (*((YCPValue *) id)));
	}

	static void delete_data_cb (gpointer data)
	{ delete (YCPValue*) data; }
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
