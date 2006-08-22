//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YComboBox.h"
#include "YGWidget.h"

class YGComboBox : public YComboBox, public YGLabeledWidget
{
	int items_nb;

	public:
		YGComboBox (const YWidgetOpt &opt, YGWidget *parent, YCPString label)
		: YComboBox (opt, label)
		, YGLabeledWidget (this, parent, label, YD_VERT, true,
		    opt.isEditable.value() ? GTK_TYPE_COMBO_BOX_ENTRY : GTK_TYPE_COMBO_BOX, NULL)
	{
		/* Making the combo box accepting text items. */
		GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
		gtk_combo_box_set_model (GTK_COMBO_BOX (getWidget()),
		                         GTK_TREE_MODEL (store));
		g_object_unref (store);

		if(!opt.isEditable.value()) {
			GtkCellRenderer* cell = gtk_cell_renderer_text_new ();
			gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (getWidget()), cell, TRUE);
			gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (getWidget()), cell,
			                                "text", 0, NULL);
		}
		else
			gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY(getWidget()), 0);

		g_signal_connect (G_OBJECT (getWidget()), "changed",
		                  G_CALLBACK (selected_changed_cb), this);

		items_nb = 0;
	}

	GtkComboBox *getComboBox() const
	{
		return GTK_COMBO_BOX ((const_cast<YGComboBox *>(this)->getWidget()));
	}

	GtkEntry *getEntry()
	{
		if (!GTK_IS_COMBO_BOX_ENTRY (getWidget())) {
			y2error ("YGComboBox: trying to edit read-only combo box");
			return NULL;
		}

		GtkWidget *entry = gtk_bin_get_child (GTK_BIN (getWidget()));
		if (!GTK_IS_ENTRY (entry)) {
			g_error ("YGComboBox: GtkComboBoxEntry doesn't have a GtkEntry as child");
			return NULL;
		}

		return GTK_ENTRY (entry);
	}

	virtual void itemAdded (const YCPString & string, int index, bool selected)
	{
		gtk_combo_box_insert_text (getComboBox(), index, string->value_cstr());
		if (selected || index == 0) /* always select the 1st by default */
			gtk_combo_box_set_active (getComboBox(), index);
		items_nb++;
	}

	virtual void setValue (const YCPString &value)
	{
		IMPL;
		gtk_entry_set_text (getEntry(), value->value_cstr());
	}

	virtual YCPString getValue() const
	{
		return YCPString (gtk_combo_box_get_active_text (getComboBox()));
	}

	virtual int getCurrentItem() const
	{
		return gtk_combo_box_get_active (getComboBox());
	}

	virtual void setCurrentItem (int index)
	{
		gtk_combo_box_set_active (GTK_COMBO_BOX(getWidget()), index);
	}

	virtual void setInputMaxLength (const YCPInteger &numberOfChars)
	{
		gtk_entry_set_width_chars (getEntry(), numberOfChars->asInteger()->value());
	}

	// YWidget
	YGWIDGET_IMPL_NICESIZE
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_KEYBOARD_FOCUS

	// YGLabelWidget
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN(YComboBox)
	
	// Events notifications
	static void selected_changed_cb (GtkComboBox *widget, YGComboBox *pThis)
	{
		/* selected_changed_cb() is called when a new item was selected or the user has
		   typed some text on a writable ComboBox. text_changed is true for the later and
		   false for the former. */
		bool text_changed = GTK_IS_COMBO_BOX_ENTRY (widget)
		                    && pThis->getCurrentItem() == -1;

		if (text_changed) {
			g_signal_handlers_block_by_func (pThis->getWidget(), (gpointer)
			                                 selected_changed_cb, pThis);
			YGUtils::filterText (GTK_EDITABLE (pThis->getEntry()), 0, -1,
			                     pThis->getValidChars()->value_cstr());
			g_signal_handlers_unblock_by_func (pThis->getWidget(), (gpointer)
			                                   selected_changed_cb, pThis);

			pThis->emitEvent (YEvent::ValueChanged, true, true);
		}
		else
			pThis->emitEvent (YEvent::SelectionChanged, true, true);
	}
};

YWidget *
YGUI::createComboBox (YWidget *parent, YWidgetOpt & opt,
                      const YCPString & label)
{
	return new YGComboBox (opt, YGWidget::get (parent), label);
}
