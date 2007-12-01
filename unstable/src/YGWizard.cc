/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "YGUtils.h"
#include "ygtkwizard.h"
#include "YWizard.h"
#include "YPushButton.h"

class YGWizard : public YWizard, public YGWidget
{
	bool m_verboseCommands;

	/* YCP requires us to allow people to use YPushButton API on the wizard buttons.
	   Wizard already has handlers for them; this seems like bad design.

	   We could support wrapping right in our framework. One way, would be to subclass
	   YWidgetOpt to have a wrapping field where we would set a GtkWidget*. Then,
	   classes should pass opt to YGWidget and it would create a shallow instance
	   around it. However, this isn't really doable. The problem is that, like
	   in this case, the API isn't really exact; events must be sent as YWizard events.
	*/
	struct YGWButton : public YPushButton {
		/* Thin class; just changes the associated button label and keeps track
		   of id change. */
		YGWButton (GtkWidget *widget, const YCPString &label, const YCPValue &id)
		: YPushButton (YWidgetOpt(), label)
		{ m_widget = widget; setId (id); setLabel (label); }

		void setLabel (const YCPString &label) {
			string str = YGUtils::mapKBAccel (label->value_cstr());
			gtk_button_set_label (GTK_BUTTON (m_widget), str.c_str());
			str.empty() ? gtk_widget_hide (m_widget) : gtk_widget_show (m_widget);
			YGUtils::setStockIcon (m_widget, str);
			YPushButton::setLabel (label);
		}

		long nicesize (YUIDimension dim) { return 0; }
		void setEnabling (bool enable) {
			gtk_widget_set_sensitive (m_widget, enable);
			YWidget::setEnabling (enable);
		}

		private:
			GtkWidget *m_widget;
	};

	YGWButton *m_back_button, *m_abort_button, *m_next_button;
	// release notes button would be a little more hassle to support; yast-qt
	// doesn't support it too anyway.

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
		{
			// We set a YReplacePoint as child with a certain id and it will then be
			// replaced by the actual content. We put a YEmpty on it because it would
			// crash otherwise (stretchable() functions should really check for
			// hasChildren() first!).

			YWidgetOpt opt, stretchOpt;
			stretchOpt.isHStretchable.setValue( true );
			stretchOpt.isVStretchable.setValue( true );

			YWidget *align, *rp, *empty;
			align = YGUI::ui()->createAlignment (this, stretchOpt,
			                                     YAlignCenter, YAlignCenter);
			rp = YGUI::ui()->createReplacePoint (align, opt);
			rp->setId (YCPSymbol (YWizardContentsReplacePointID));
			empty = YGUI::ui()->createEmpty (rp, opt);

			((YContainerWidget *) align)->addChild (rp);
			((YContainerWidget *) rp)->addChild (empty);
			this->addChild (align);
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
		m_back_button = new YGWButton (ygtk_wizard->m_back_button, backButtonLabel,
		                               backButtonId);
		m_abort_button = new YGWButton (ygtk_wizard->m_abort_button, abortButtonLabel,
		                                abortButtonId);
		m_next_button = new YGWButton (ygtk_wizard->m_next_button, nextButtonLabel,
		                               nextButtonId);
		YContainerWidget::addChild (m_back_button);
		YContainerWidget::addChild (m_abort_button);
		YContainerWidget::addChild (m_next_button);

		ygtk_wizard_set_back_button_id (ygtk_wizard, new YCPValue (backButtonId),
		                                delete_data_cb);
		ygtk_wizard_set_abort_button_id (ygtk_wizard, new YCPValue (abortButtonId),
		                                 delete_data_cb);
		ygtk_wizard_set_next_button_id (ygtk_wizard, new YCPValue (nextButtonId),
		                                delete_data_cb);

		//** All event are sent through this signal together with an id
		g_signal_connect (G_OBJECT (getWidget()), "action-triggered",
		                  G_CALLBACK (action_triggered_cb), this);
	}

	virtual ~YGWizard()
	{
		// m_back/abort/next_button were added as children and will be freed by ~YContainerWidget
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
							y2error ("YGWizard: expected string as the %d argument for "
							         "the '%s' command.", i+1, func);
							return false;
						}
						break;
					case 0x2:
						if (!cmd->value(i)->isBoolean()) {
							y2error ("YGWizard: expected boolean as the %d argument for "
							         "the '%s' command.", i+1, func);
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
//				return YCPBoolean (false); - installer relies on this succeeding
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

		else if (isCommand (cmd, "SetAbortButtonID", 1, 0x0)) {
			YCPValue *id = new YCPValue (getAnyArg (cmd, 0));
			ygtk_wizard_set_abort_button_id (wizard, id, delete_data_cb);
			m_abort_button->setId (*id);
		}
		else if (isCommand (cmd, "SetBackButtonID", 1, 0x0)) {
			YCPValue *id = new YCPValue (getAnyArg (cmd, 0));
			ygtk_wizard_set_back_button_id (wizard, id, delete_data_cb);
			m_back_button->setId (*id);
		}
		else if (isCommand (cmd, "SetNextButtonID", 1, 0x0)) {
			YCPValue *id = new YCPValue (getAnyArg (cmd, 0));
			ygtk_wizard_set_next_button_id (wizard, id, delete_data_cb);
			m_next_button->setId (*id);
		}

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

	virtual YCPString currentTreeSelection()
	{
		YGtkWizard *wizard = YGTK_WIZARD (getWidget());
		const char *selected = ygtk_wizard_get_tree_selection (wizard);
		if (selected)
			return YCPString (selected);
		return YCPString ("");
	}

	static void action_triggered_cb (YGtkWizard *wizard, gpointer id,
	                                 gint id_type, YGWizard *pThis)
	{
		IMPL
		if ((GType) id_type == G_TYPE_STRING) {
			YCPString _id ((char *) id);
			YGUI::ui()->sendEvent (new YMenuEvent (YCPValue (_id)));
		}
		else
			YGUI::ui()->sendEvent (new YMenuEvent (*((YCPValue *) id)));
	}

	static void delete_data_cb (gpointer data)
	{ delete (YCPValue*) data; }


	YGWIDGET_IMPL_COMMON
	YGWIDGET_IMPL_CHILD_ADDED (getWidget())
	YGWIDGET_IMPL_CHILD_REMOVED (getWidget())
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
