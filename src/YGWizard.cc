/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <YGUI.h>
#include "YGWidget.h"
#include "YGUtils.h"
#include "ygtkwizard.h"
#include "YWizard.h"
#include "YPushButton.h"
#include "YAlignment.h"
#include "YReplacePoint.h"
#include "YGDialog.h"

class YGWizard : public YWizard, public YGWidget
{
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
		YGWButton (YGWizard *parent, GtkWidget *widget, const std::string &label)
		: YPushButton (parent, label), m_widget (widget), m_wizard (parent)
		{
			setWidgetRep (NULL);
			setLabel (label);
			ygtk_wizard_set_button_ptr_id (getWizard(), widget, this);
		}

		virtual void setLabel (const string &label)
		{
			YPushButton::setLabel (label);

			// notice: we can't use functionKey() to deduce the icon because yast
			// tools code differ from the text-mode to the GUIs when setting buttons
			// up, and the opt_f10 and so on will not be set in the GUI code
			YGtkWizard *wizard = getWizard();
			std::string _label = YGUtils::mapKBAccel (label);
			ygtk_wizard_set_button_label (wizard, getWidget(), _label.c_str(), NULL);
		}

		virtual void setEnabled (bool enable)
		{
			YWidget::setEnabled (enable);
			ygtk_wizard_enable_button (getWizard(), getWidget(), enable);
		}

		virtual bool setKeyboardFocus()
		{
			gtk_widget_grab_focus (getWidget());
			return gtk_widget_is_focus (getWidget());
		}

		virtual int preferredWidth() { return 0; }
		virtual int preferredHeight() { return 0; }
		virtual void setSize (int w, int h) {}

		inline GtkWidget *getWidget() { return m_widget; }
		inline YGtkWizard *getWizard() { return YGTK_WIZARD (m_wizard->getWidget()); }

		private:
			GtkWidget *m_widget;
			YGWizard *m_wizard;
	};

	YGWButton *m_back_button, *m_abort_button, *m_next_button, *m_notes_button;
	// release notes button would be a little more hassle to support; yast-qt
	// doesn't support it too anyway.

public:
	YGWizard (YWidget *parent, const string &backButtonLabel,
		const string &abortButtonLabel, const string &nextButtonLabel,
		YWizardMode wizardMode)
	: YWizard (NULL, backButtonLabel, abortButtonLabel, nextButtonLabel, wizardMode)
	, YGWidget (this, parent, YGTK_TYPE_WIZARD, NULL)
	{
		IMPL
		setBorder (0);
		YGtkWizard *wizard = getWizard();

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
			yuiError() << "YGWizard doesn't support both steps and tree enabled at the "
			         "same time.\nDisabling the steps...\n";
			steps_enabled = false;
		}
		if (steps_enabled)
			ygtk_wizard_enable_steps (wizard);
		if (tree_enabled)
			ygtk_wizard_enable_tree (wizard);

		//** Setting the bottom buttons
		m_back_button  = new YGWButton (this, wizard->back_button, backButtonLabel);
		m_abort_button = new YGWButton (this, wizard->abort_button, abortButtonLabel);
		m_next_button  = new YGWButton (this, wizard->next_button, nextButtonLabel);
		m_notes_button = new YGWButton (this, wizard->release_notes_button, string());

		//** All event are sent through this signal together with an id
		g_signal_connect (G_OBJECT (getWidget()), "action-triggered",
		                  G_CALLBACK (action_triggered_cb), this);
	}

	virtual ~YGWizard()
	{
		// m_back/abort/next_button are added as children and
		// so will be freed by ~YContainerWidget
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

	virtual void setHelpText (const string &_text)
	{
		std::string productName = YUI::app()->productName();
		std::string text(_text);
		YGUtils::replace (text, "&product;", 9, productName.c_str());
		ygtk_wizard_set_help_text (getWizard(), text.c_str());
	}

	virtual void setDialogIcon (const string &icon)
	{
		if (!ygtk_wizard_set_header_icon (getWizard(), icon.c_str()))
			yuiWarning() << "YGWizard: could not load image: " << icon << endl;
		YGDialog::currentDialog()->setIcon (icon);
	}

	virtual void setDialogHeading (const string &heading)
	{
		ygtk_wizard_set_header_text (getWizard(), heading.c_str());
		YGDialog::currentDialog()->setTitle (heading, false);
	}

	virtual void setDialogTitle (const string &title)
	{
		YGDialog::currentDialog()->setTitle (title, true);
	}

	virtual void addStepHeading (const string &text)
	{
		ygtk_wizard_add_step_header (getWizard(), text.c_str());
	}

    virtual void addStep (const string &text, const string &id)
	{
		ygtk_wizard_add_step (getWizard(), text.c_str(), id.c_str());
	}

	virtual void setCurrentStep (const string &id)
	{
		if (!ygtk_wizard_set_current_step (getWizard(), id.c_str()))
			yuiError() << "YGWizard: there is no step with id " << id << endl;
	}

	virtual void deleteSteps()
	{
		ygtk_wizard_clear_steps (getWizard());
	}

	virtual void updateSteps()
	{}

	virtual void addTreeItem (const string &parentID, const string &text,
	                          const string &id)
	{
		if (!ygtk_wizard_add_tree_item (getWizard(), parentID.c_str(),
		        text.c_str(), id.c_str()))
			yuiError() << "YGWizard: there is no tree item with id " << parentID << endl;
	}

	virtual void selectTreeItem (const string &id)
	{
		if (!ygtk_wizard_select_tree_item (getWizard(), id.c_str()))
			yuiError() << "YGWizard: there is no tree item with id " << id << endl;
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
			yuiError() << "YGWizard: there is no menu item with id " << parentID << endl;
	}

	virtual void addMenuEntry (const string &parentID, const string &text,
	                           const string &id)
	{
		string str = YGUtils::mapKBAccel (text);
		if (!ygtk_wizard_add_menu_entry (getWizard(), parentID.c_str(),
			str.c_str(), id.c_str()))
			yuiError() << "YGWizard: there is no menu item with id " << parentID << endl;
	}

	virtual void addMenuSeparator (const string & parentID)
	{
		if (!ygtk_wizard_add_menu_separator (getWizard(), parentID.c_str()))
			yuiError() << "YGWizard: there is no menu item with id " << parentID << endl;
	}

	virtual void deleteMenus()
	{
		ygtk_wizard_clear_menu (getWizard());
	}

	virtual void showReleaseNotesButton (const string &label, const string &id)
	{
		string str = YGUtils::mapKBAccel (label.c_str());
		ygtk_wizard_set_button_label (getWizard(), m_notes_button->getWidget(), str.c_str(), NULL);
		ygtk_wizard_set_button_str_id (getWizard(), m_notes_button->getWidget(), id.c_str());
	}

	virtual void hideReleaseNotesButton()
	{
		ygtk_wizard_set_button_label (getWizard(), m_notes_button->getWidget(), NULL, NULL);
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

	// YGWidget
	virtual void doAddChild (YWidget *ychild, GtkWidget *container)
	{
		if (ychild->widgetRep())  // don't actually add the button wrappers
			ygtk_wizard_set_child (getWizard(), YGWidget::get (ychild)->getLayout());
	}

	YGWIDGET_IMPL_COMMON
	YGWIDGET_IMPL_CHILD_ADDED (YWizard, getWidget())
	YGWIDGET_IMPL_CHILD_REMOVED (YWizard, getWidget())
};

YWizard *YGOptionalWidgetFactory::createWizard (YWidget *parent,
	const string &backButtonLabel, const string &abortButtonLabel,
	const string &nextButtonLabel, YWizardMode wizardMode)
{
	return new YGWizard (parent, backButtonLabel, abortButtonLabel, nextButtonLabel,
	                     wizardMode);
}

