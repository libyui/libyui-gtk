#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YComboBox.h"
#include "YGWidget.h"

class YGComboBox : public YComboBox, public YGWidget
{
public:
#if 0
	static void toggled_cb (GtkButton *button, YGComboBox *pThis)
	{
		IMPL;
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
			pThis->buttonGroup()->uncheckOtherButtons (pThis);
		fprintf (stderr, "Send event!\n");
		YGUI::ui()->sendEvent( new YWidgetEvent( pThis, YEvent::ValueChanged ) );
	}
#endif

	YGComboBox( const YWidgetOpt &opt,
		    YGWidget         *parent,
		    YCPString         label)
		:  YComboBox( opt, label ),
		   YGWidget( this, parent, true,
			     GTK_TYPE_COMBO_BOX, NULL )
	{
		IMPL;
#if 0
		gtk_button_set_use_underline (GTK_BUTTON (getWidget()), TRUE);
		g_signal_connect (G_OBJECT (getWidget ()),
				  "toggled", G_CALLBACK (toggled_cb), this);
		setLabel (label);
#endif
	}

	// YSelectionWidget
	virtual void itemAdded( const YCPString & string, int index, bool selected ) IMPL;

	// YComboBox
	virtual void setLabel (const YCPString &label)
	{
		IMPL;
		char *str = YGWidget::mapKBAccel (label->value_cstr());
#if 0
		gtk_button_set_label (GTK_BUTTON (getWidget()), str);
#endif
		g_free (str);
	}

	virtual void setValue (const YCPString &value)
	{
		IMPL;
#if 0
		g_signal_handlers_block_by_func
			(getWidget(), (gpointer)toggled_cb, this);

		if (checked->value())
			buttonGroup()->uncheckOtherButtons (this);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (getWidget()),
					      checked->value());

		g_signal_handlers_unblock_by_func
			(getWidget(), (gpointer)toggled_cb, this);
#endif
	}
	virtual YCPString getValue() const
	{
		IMPL;
#if 0
		return YCPBoolean( gtk_toggle_button_get_active (
			GTK_TOGGLE_BUTTON (getWidget() ) ) );
#endif
		return YCPString("foo");
	}

	// Remove comments as we implement ;-) cf. YComboBox.h
	/**
	 * Returns the index of the currently selected item (from 0 on)
	 * or -1 if no item is selected.
	 **/
	virtual int getCurrentItem() const IMPL_RET(0)

	/**
	 * Selects an item from the list. Notice there intentionally is no
	 * corresponding getCurrentItem() method - use getValue() instead.
	 */
	virtual void setCurrentItem( int index ) IMPL;

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
};

YWidget *
YGUI::createComboBox( YWidget *parent, YWidgetOpt & opt,
		      const YCPString & label )
{
	return new YGComboBox (opt, YGWidget::get (parent), label);
}
