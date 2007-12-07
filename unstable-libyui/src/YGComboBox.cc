/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YComboBox.h"
#include "YGSelectionModel.h"
#include "YGWidget.h"

class YGComboBox : public YComboBox, public YGLabeledWidget, public YGSelectionModel
{
	public:
		YGComboBox (YWidget *parent, const string &label, bool editable)
		: YComboBox (NULL, label, editable)
		, YGLabeledWidget (this, parent, label, YD_HORIZ, true,
		    editable ? GTK_TYPE_COMBO_BOX_ENTRY : GTK_TYPE_COMBO_BOX, NULL)
		, YGSelectionModel (this, true, false)
	{
		gtk_combo_box_set_model (getComboBox(), getModel());
		GtkCellRenderer* cell;
		if (editable) {
			gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (getWidget()),
			                                     YGSelectionModel::LABEL_COLUMN);
		}
		else {
			cell = gtk_cell_renderer_text_new ();
			gtk_cell_layout_pack_end (GTK_CELL_LAYOUT (getWidget()), cell, TRUE);
			gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (getWidget()), cell,
				"text", YGSelectionModel::LABEL_COLUMN, NULL);
		}
		cell = gtk_cell_renderer_pixbuf_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (getWidget()), cell, FALSE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (getWidget()), cell,
			"pixbuf", YGSelectionModel::ICON_COLUMN, NULL);

		g_signal_connect (G_OBJECT (getWidget()), "changed",
		                  G_CALLBACK (selected_changed_cb), this);
	}

	inline GtkComboBox *getComboBox()
	{ return GTK_COMBO_BOX (getWidget()); }

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

	virtual string text()
	{
		return gtk_combo_box_get_active_text (getComboBox());
	}

	virtual void setText (const string &value)
	{
		IMPL
		gtk_entry_set_text (getEntry(), value.c_str());
	}

	// YGSelectionModel
	virtual void setFocusItem (GtkTreeIter *iter, bool addingRow)
	{
		// GtkComboBox wants a string on the model, not NULL, when setting it as
		// the active row. As, we only set that value after addingRow, we need to
		// initialize it here...
		if (addingRow)
			setCellLabel (iter, YGSelectionModel::LABEL_COLUMN, string());
		gtk_combo_box_set_active_iter (getComboBox(), iter);
	}

	virtual void unsetFocus()
	{
		gtk_combo_box_set_active (getComboBox(), -1);
	}

    virtual YItem *focusItem()
    {
    	GtkTreeIter iter;
    	if (gtk_combo_box_get_active_iter (getComboBox(), &iter))
    		return getItem (&iter);
    	return NULL;
    }

	// YComboBox
	virtual void setInputMaxLength (const YCPInteger &numberOfChars)
	{
		gtk_entry_set_width_chars (getEntry(), numberOfChars->asInteger()->value());
	}

	virtual void setValidChars (const string &validChars)
	{
		YGUtils::setFilter (getEntry(), validChars);
		YComboBox::setValidChars (validChars);
	}

	// Events notifications
	static void selected_changed_cb (GtkComboBox *widget, YGComboBox *pThis)
	{
		/* selected_changed_cb() is called when a new item was selected or the user has
		   typed some text on a writable ComboBox. text_changed is true for the later and
		   false for the former. */
		bool text_changed = GTK_IS_COMBO_BOX_ENTRY (widget)
		                    && pThis->selectedItem() == NULL;
		if (text_changed)
			pThis->emitEvent (YEvent::ValueChanged, true, true);
		else
			pThis->emitEvent (YEvent::SelectionChanged, true, true);
	}

	YGWIDGET_IMPL_COMMON
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN (YComboBox)
	YGSELECTION_WIDGET_IMPL_ALL (YComboBox)
};

YComboBox *YGWidgetFactory::createComboBox (YWidget *parent, const string &label, bool editable)
{
	return new YGComboBox (parent, label, editable);
}

