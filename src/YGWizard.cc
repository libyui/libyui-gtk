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
#include "YAlignment.h"
#include "YReplacePoint.h"
#include "YGDialog.h"


// FIXME: wizard events need to be ported
// We'll probably want to also make YGtkWizard buttons actual YGPushButtons...
// (let's just create them and pass button->getWidget() to the wizard...)
// (or let's just improve on the wrapper and id.)


class YGWizard : public YWizard, public YGWidget
{
	bool m_verboseCommands;
	YReplacePoint *m_replacePoint;

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
		YGWButton (YWidget *parent, GtkWidget *widget, const std::string &label)
		: YPushButton (parent, label)
		{ setWidgetRep (NULL); m_widget = widget; setLabel (label); }

		void setLabel (const string &label) {
			string str = YGUtils::mapKBAccel (label.c_str());
			gtk_button_set_label (GTK_BUTTON (m_widget), str.c_str());
			str.empty() ? gtk_widget_hide (m_widget) : gtk_widget_show (m_widget);
			YGUtils::setStockIcon (m_widget, str);
			YPushButton::setLabel (label);
		}

		void setEnabled (bool enable) {
			gtk_widget_set_sensitive (m_widget, enable);
			YWidget::setEnabled (enable);
		}

		int preferredWidth() { return 0; }
		int preferredHeight() { return 0; }
		void setSize (int w, int h) {}

		private:
			GtkWidget *m_widget;
	};

	YGWButton *m_back_button, *m_abort_button, *m_next_button, *m_notes_button;
	// release notes button would be a little more hassle to support; yast-qt
	// doesn't support it too anyway.

public:
	YGWizard (YWidget *parent, const string &backButtonLabel,
		const string &abortButtonLabel, const string &nextButtonLabel,
		YWizardMode wizardMode)
	: YWizard (NULL, backButtonLabel, abortButtonLabel, nextButtonLabel, wizardMode)
	, YGWidget (this, parent, true, YGTK_TYPE_WIZARD, NULL)
	{
		IMPL
		setBorder (0);
		m_verboseCommands = false;

		//** Application area
		{
			YAlignment *align = YUI::widgetFactory()->createAlignment (this,
				YAlignCenter, YAlignCenter);
			align->setStretchable (YD_HORIZ, true);
			align->setStretchable (YD_VERT, true);

			m_replacePoint = YUI::widgetFactory()->createReplacePoint ((YWidget *) align);
			YUI::widgetFactory()->createEmpty ((YWidget *) m_replacePoint);
			m_replacePoint->showChild();
		}

		//** Steps/tree pane
		bool steps_enabled = wizardMode == YWizardMode_Steps;
		bool tree_enabled  = wizardMode == YWizardMode_Tree;
		if (steps_enabled && tree_enabled) {
			y2error ("YGWizard doesn't support both steps and tree enabled at the "
			         "same time.\nDisabling the steps...");
			steps_enabled = false;
		}
		if (steps_enabled)
			ygtk_wizard_enable_steps (getWizard());
		if (tree_enabled)
			ygtk_wizard_enable_tree (getWizard());

		//** Setting the bottom buttons
		m_back_button  = new YGWButton (this, getWizard()->m_back_button, backButtonLabel);
		m_abort_button = new YGWButton (this, getWizard()->m_abort_button, abortButtonLabel);
		m_next_button  = new YGWButton (this, getWizard()->m_next_button, nextButtonLabel);
		m_notes_button = new YGWButton (this, getWizard()->m_release_notes_button, string());

		ygtk_wizard_set_back_button_ptr_id (getWizard(), (gpointer) m_back_button);
		ygtk_wizard_set_next_button_ptr_id (getWizard(), (gpointer) m_next_button);
		ygtk_wizard_set_abort_button_ptr_id (getWizard(), (gpointer) m_abort_button);
		ygtk_wizard_set_release_notes_button_ptr_id (getWizard(), (gpointer) m_notes_button);

		//** All event are sent through this signal together with an id
		g_signal_connect (G_OBJECT (getWidget()), "action-triggered",
		                  G_CALLBACK (action_triggered_cb), this);
	}

	virtual ~YGWizard()
	{
		// m_back/abort/next_button are added as children and will be freed by ~YContainerWidget
	}

	inline YGtkWizard *getWizard()
	{ return YGTK_WIZARD (getWidget()); }

	virtual YReplacePoint *contentsReplacePoint() const
	{ return m_replacePoint; }

	virtual YPushButton *backButton()  const
	{ return m_back_button; }
	virtual YPushButton *abortButton() const
	{ return m_abort_button; }
	virtual YPushButton *nextButton()  const
	{ return m_next_button; }

	virtual void setButtonLabel (YPushButton *button, const string &label)
	{
		button->setLabel (label);
	}

	virtual void setHelpText (const string &text)
	{
		ygtk_wizard_set_help_text (getWizard(), text.c_str());
	}

	virtual void setDialogIcon (const string &icon)
	{
		if (!ygtk_wizard_set_header_icon (getWizard(), YGDialog::currentWindow(),
		        icon.c_str()))
			y2warning ("YGWizard: could not load image: %s", icon.c_str());
	}

	virtual void setDialogHeading (const string &heading)
	{
		ygtk_wizard_set_header_text (getWizard(), YGDialog::currentWindow(),
		                             heading.c_str());
	}

    virtual void addStep (const string &text, const string &id)
	{
		ygtk_wizard_add_step (getWizard(), text.c_str(), id.c_str());
	}

	virtual void addStepHeading (const string &text)
	{
		ygtk_wizard_add_step_header (getWizard(), text.c_str());
	}

	virtual void deleteSteps()
	{
		ygtk_wizard_clear_steps (getWizard());
	}

	virtual void setCurrentStep (const string &id)
	{
		if (!ygtk_wizard_set_current_step (getWizard(), id.c_str()))
			y2error ("YGWizard: there is no step with id %s.", id.c_str());
	}

	virtual void updateSteps()
	{}

	virtual void addTreeItem (const string &parentID, const string &text,
	                          const string &id)
	{
		if (!ygtk_wizard_add_tree_item (getWizard(), parentID.c_str(),
		        text.c_str(), id.c_str()))
			y2error ("YGWizard: there is no tree item with id '%s'",
			         parentID.c_str());
	}

	virtual void selectTreeItem (const string &id)
	{
		if (!ygtk_wizard_select_tree_item (getWizard(), id.c_str()))
			y2error ("YGWizard: there is no tree item with id '%s'", id.c_str());
	}

	virtual string currentTreeSelection()
	{
		const char *selected = ygtk_wizard_get_tree_selection (getWizard());
		if (selected)
			return selected;
		return string();
	}

	virtual void deleteTreeItems()
	{
		ygtk_wizard_clear_tree (getWizard());
	}

	virtual void addMenu (const string &text, const string &id)
	{
		string str = YGUtils::mapKBAccel (text);
		ygtk_wizard_add_menu (getWizard(), str.c_str(), id.c_str());
	}

	virtual void addSubMenu (const string &parentID, const string &text,
	                         const string &id)
	{
		string str = YGUtils::mapKBAccel(text);
		if (!ygtk_wizard_add_sub_menu (getWizard(), parentID.c_str(), str.c_str(),
		        id.c_str()))
			y2error ("YGWizard: there is no menu item with id '%s'",
			         parentID.c_str());
	}

	virtual void addMenuEntry (const string &parentID, const string &text,
	                           const string &id)
	{
		string str = YGUtils::mapKBAccel (text);
		if (!ygtk_wizard_add_menu_entry (getWizard(), parentID.c_str(),
			str.c_str(), id.c_str()))
			y2error ("YGWizard: there is no menu item with id '%s'",
			         parentID.c_str());
	}

	virtual void addMenuSeparator (const string & parentID)
	{
		if (!ygtk_wizard_add_menu_separator (getWizard(), parentID.c_str()))
			y2error ("YGWizard: there is no menu item with id '%s'",
			         parentID.c_str());
	}

	virtual void deleteMenus()
	{
		ygtk_wizard_clear_menu (getWizard());
	}

	virtual void showReleaseNotesButton (const string &label, const string &id)
	{
		string str = YGUtils::mapKBAccel (label.c_str());
		ygtk_wizard_set_release_notes_button_label (getWizard(), str.c_str());
		ygtk_wizard_set_release_notes_button_str_id (getWizard(), id.c_str());
	}

	virtual void hideReleaseNotesButton()
	{
		ygtk_wizard_show_release_notes_button (getWizard(), FALSE);
	}

	virtual void retranslateInternalButtons()
	{}

	static void action_triggered_cb (YGtkWizard *wizard, gpointer id,
	                                 gint id_type, YGWizard *pThis)
	{
		IMPL
		if ((GType) id_type == G_TYPE_STRING)
			YGUI::ui()->sendEvent (new YMenuEvent ((char *) id));
		else
			YGUI::ui()->sendEvent (new YWidgetEvent ((YWidget *) id, YEvent::Activated));
	}

	virtual void doAddChild (YWidget *ychild, GtkWidget *container)
	{
		if (ychild->widgetRep())
			// don't actually add the button wrappers
			YGWidget::doAddChild (ychild, container);
	}

	YGWIDGET_IMPL_COMMON
	YGWIDGET_IMPL_CHILD_ADDED (getWidget())
	YGWIDGET_IMPL_CHILD_REMOVED (getWidget())
};

YWizard *YGOptionalWidgetFactory::createWizard (YWidget *parent,
	const string &backButtonLabel, const string &abortButtonLabel,
	const string &nextButtonLabel, YWizardMode wizardMode)
{
	return new YGWizard (parent, backButtonLabel, abortButtonLabel, nextButtonLabel,
	                     wizardMode);
}

