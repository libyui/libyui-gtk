/* */

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YComboBox.h"
#include "YGLabelWidget.h"

class YGComboBox : public YComboBox, public YGLabelWidget
{
	int indices_nb;

	public:
		YGComboBox (const YWidgetOpt &opt, YGWidget *parent, YCPString label)
		: YComboBox (opt, label)
		, YGLabelWidget (this, parent, label, YD_VERT, true,
		    opt.isEditable.value() ? GTK_TYPE_COMBO_BOX_ENTRY : GTK_TYPE_COMBO_BOX, NULL)
	{
		/* Making the combo box accepting text items. */
		GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);

		gtk_combo_box_set_model(GTK_COMBO_BOX(getWidget()), GTK_TREE_MODEL (store));

		g_object_unref (store);

		if(!opt.isEditable.value())
		{
			GtkCellRenderer* cell = gtk_cell_renderer_text_new ();
			gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (getWidget()), cell, TRUE);
			gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (getWidget()), cell, "text", 0, NULL);
		}
		else
			gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY(getWidget()), 0);

		g_signal_connect (G_OBJECT (getWidget()), "changed",
				  G_CALLBACK (selected_changed_cb), this);

		indices_nb = 0;
	}

	GtkComboBox *getComboBox() const
	{
		return GTK_COMBO_BOX ((const_cast<YGComboBox *>(this)->getWidget()));
	}

	virtual void itemAdded (const YCPString & string, int index, bool selected)
	{
		IMPL;
		gtk_combo_box_insert_text (getComboBox(), index, string->value_cstr());
		if (selected || index == 0) /* always select the 1st by default */
			gtk_combo_box_set_active (getComboBox(), index);
		indices_nb++;
	}

	virtual void setValue (const YCPString &value)
	{
		IMPL;
		/* This does seem to work, but may break, so should be replaced. */
		gtk_combo_box_append_text (GTK_COMBO_BOX(getWidget()), value->value_cstr());
		setCurrentItem (indices_nb);
		gtk_combo_box_remove_text (GTK_COMBO_BOX(getWidget()), indices_nb);
	}

	virtual YCPString getValue() const
	{
		return YCPString (gtk_combo_box_get_active_text (getComboBox()));
	}

	virtual int getCurrentItem() const
	{
		GtkComboBox *box = GTK_COMBO_BOX((const_cast<YGComboBox *>(this)->getWidget()));
		return gtk_combo_box_get_active(box);
	}

	virtual void setCurrentItem (int index)
	{
		gtk_combo_box_set_active (GTK_COMBO_BOX(getWidget()), index);
	}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_KEYBOARD_FOCUS

	// YGLabelWidget
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN(YComboBox)
	
	// Slots
	static void selected_changed_cb (GtkComboBox *widget, YGComboBox *pThis)
	{
		if (pThis->getNotify () &&
		    !YGUI::ui()->eventPendingFor (pThis))
		{
			if (GTK_IS_COMBO_BOX_ENTRY (widget) &&
			    pThis->getCurrentItem() == -1)
				YGUI::ui()->sendEvent (new YWidgetEvent (pThis, YEvent::ValueChanged));
			else
				YGUI::ui()->sendEvent (new YWidgetEvent (pThis, YEvent::SelectionChanged));
		}
	}
};

YWidget *
YGUI::createComboBox (YWidget *parent, YWidgetOpt & opt,
                      const YCPString & label)
{
	return new YGComboBox (opt, YGWidget::get (parent), label);
}
