/* */

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YEvent.h"
#include "YComboBox.h"
#include "YGLabelWidget.h"

class YGComboBox : public YComboBox, public YGLabelWidget
{
	public:
		YGComboBox (const YWidgetOpt &opt, YGWidget *parent, YCPString label,
		            GType combo_box_type, GtkTreeModel* use_model)
		: YComboBox (opt, label)
		, YGLabelWidget (this, parent, label, YD_VERT, true,
		                 combo_box_type, "model", use_model, "text-column", 0, NULL)
	{
		// FIXME: shouldn't pass "text-column" to regular GtkComboBox
		g_signal_connect (G_OBJECT (getWidget()), "changed",
				  G_CALLBACK (selected_changed_cb), this);
	}

	GtkComboBox *getComboBox() const
	{
		return GTK_COMBO_BOX ((const_cast<YGComboBox *>(this)->getWidget()));
	}

	/* Doesn't honor the index argument. As Qt module does the same, I guess
	   that's okay, maybe even expected. */
	virtual void itemAdded (const YCPString & string, int index, bool selected)
	{
		IMPL;
		gtk_combo_box_insert_text (getComboBox(), index, string->value_cstr());
		if (selected || index == 0) /* always select the 1st by default */
			gtk_combo_box_set_active (getComboBox(), index);
	}

	virtual void setValue (const YCPString &value)
	{
		IMPL;
#if 0
		printf("\n\n\n\n");

		GtkTreeModel *model = GTK_TREE_MODEL (gtk_combo_box_get_model (getComboBox()))
		GtkTreeIter iter;

		if (gtk_combo_box_get_active_iter (getComboBox(), &iter))
		{
			gchar* str;
			gtk_tree_model_get (model, &iter, 0, &str, -1);
			// FIXME: this is broken - you need to use gtk_tree_model_set surely ?
			// str is not allocated memory - this will SEGV like a beast :-)
			strcpy (str, value->value_cstr());
		}
#endif
	}
       
	virtual YCPString getValue() const
	{
		IMPL;
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
	// Making the combo box accepting text items.
	GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);

	YGComboBox* y_widget;

	y_widget = new YGComboBox (opt, YGWidget::get (parent), label,
				   opt.isEditable.value() ? GTK_TYPE_COMBO_BOX_ENTRY : GTK_TYPE_COMBO_BOX,
				   GTK_TREE_MODEL (store));

	g_object_unref (store);

	if(!opt.isEditable.value())
	{
		GtkCellRenderer* cell = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (y_widget->getWidget()), cell, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (y_widget->getWidget()), cell, "text", 0, NULL);
	}

	return y_widget;
}
