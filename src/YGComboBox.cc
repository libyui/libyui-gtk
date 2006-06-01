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
  {                 // FIXME: shouldn't pass "text-column" to regular GtkComboBox
  g_signal_connect (G_OBJECT (getWidget()), "changed", G_CALLBACK (selected_changed_cb), this);
  }

	/* Doesn't honor the index argument. As Qt module does the same, I guess
	   that's okay, maybe even expected. */
	virtual void itemAdded (const YCPString & string, int index, bool selected)
		{
		IMPL;
		GtkComboBox* combobox = GTK_COMBO_BOX(getWidget());
		gtk_combo_box_insert_text (combobox, index, string->value_cstr());
		if(selected || index == 0 /* always select the 1st by default */)
			gtk_combo_box_set_active (combobox, index);
		}

	virtual void setValue (const YCPString &value)
	{
		IMPL;
#if 0
printf("\n\n\n\n");

    GtkTreeModel *model = GTK_TREE_MODEL(gtk_combo_box_get_model(GTK_COMBO_BOX(getWidget())));
    GtkTreeIter iter;

    if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX(getWidget()), &iter))
      {
      gchar* str;
      gtk_tree_model_get (model, &iter, 0, &str, -1);
      strcpy(str, value->value_cstr());
      }
#endif
	}

	virtual YCPString getValue() const
	{
		IMPL;
		return YCPString (gtk_combo_box_get_active_text(
		  GTK_COMBO_BOX((const_cast<YGComboBox *>(this)->getWidget()))));
	}

	virtual int getCurrentItem() const
	{
		return gtk_combo_box_get_active(
		  GTK_COMBO_BOX((const_cast<YGComboBox *>(this)->getWidget())));
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

  // Slots
  static void selected_changed_cb(GtkComboBox* widget, YGComboBox* pThis)
  {
    if (pThis->getNotify())
      if (!YGUI::ui()->eventPendingFor(pThis))
        {
        if (GTK_IS_COMBO_BOX_ENTRY(widget) && pThis->getCurrentItem() == -1)
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

	YGComboBox* y_widget = new YGComboBox (opt, YGWidget::get (parent), label,
	  opt.isEditable.value() ? GTK_TYPE_COMBO_BOX_ENTRY : GTK_TYPE_COMBO_BOX,
	  GTK_TREE_MODEL(store));

	g_object_unref (store);

	if(!opt.isEditable.value())
		{
		GtkCellRenderer* cell = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (y_widget->getWidget()), cell, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (y_widget->getWidget()), cell, "text", 0, NULL);
		}

	return y_widget;
}
