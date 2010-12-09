/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
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
		, YGLabeledWidget (this, parent, label, YD_HORIZ,
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

		connect (getWidget(), "changed", G_CALLBACK (selected_changed_cb), this);
	}

	inline GtkComboBox *getComboBox()
	{ return GTK_COMBO_BOX (getWidget()); }

	GtkEntry *getEntry()
	{
		if (!GTK_IS_COMBO_BOX_ENTRY (getWidget())) {
			yuiError() << "YGComboBox: trying to edit read-only combo box\n";
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
		gchar *str = 0;
		if (GTK_IS_COMBO_BOX_ENTRY (getWidget()))
			str = gtk_combo_box_get_active_text (getComboBox());
		else {
			GtkTreeIter iter;
			if (gtk_combo_box_get_active_iter (getComboBox(), &iter)) {
				gchar *str = 0;
				gtk_tree_model_get (getModel(), &iter, LABEL_COLUMN, &str, -1);
			}
		}
		std::string ret;
		if (str) {
			ret = str;
			g_free (str);
		}
		return ret;
	}

	virtual void setText (const string &value)
	{
		BlockEvents block (this);
		GtkTreeIter iter;
		if (findByText (value, &iter))
			doSelectItem (&iter);
        else
            gtk_entry_set_text (getEntry(), value.c_str());
	}

	// YGSelectionModel
	virtual void doSelectItem (GtkTreeIter *iter)
	{
		BlockEvents block (this);
		gtk_combo_box_set_active_iter (getComboBox(), iter);
	}

	virtual void doUnselectAll()
	{
		BlockEvents block (this);
		gtk_combo_box_set_active (getComboBox(), -1);
	}


    virtual YItem *doSelectedItem()
    {
    	GtkTreeIter iter;
    	if (gtk_combo_box_get_active_iter (getComboBox(), &iter))
    		return getItem (&iter);
    	return NULL;
    }

	// YComboBox
	virtual void setInputMaxLength (int length)
	{
		YComboBox::setInputMaxLength (length);
		gtk_entry_set_width_chars (getEntry(), length);
	}

	virtual void setValidChars (const string &validChars)
	{
		YComboBox::setValidChars (validChars);
		YGUtils::setFilter (getEntry(), validChars);
	}

	// Events notifications
	static void selected_changed_cb (GtkComboBox *widget, YGComboBox *pThis)
	{ pThis->emitEvent (YEvent::ValueChanged); }

	YGLABEL_WIDGET_IMPL (YComboBox)
	YGSELECTION_WIDGET_IMPL (YComboBox)
};

YComboBox *YGWidgetFactory::createComboBox (YWidget *parent, const string &label, bool editable)
{
	return new YGComboBox (parent, label, editable);
}

