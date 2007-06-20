/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YComboBox.h"
#include "YGWidget.h"

class YGComboBox : public YComboBox, public YGLabeledWidget
{
bool m_showingIcons;

	public:
		YGComboBox (const YWidgetOpt &opt, YGWidget *parent, YCPString label)
		: YComboBox (opt, label)
		, YGLabeledWidget (this, parent, label, YD_HORIZ, true,
		    opt.isEditable.value() ? GTK_TYPE_COMBO_BOX_ENTRY : GTK_TYPE_COMBO_BOX, NULL)
	{
		// pixbufs will be enabled if icons are provided
		m_showingIcons = false;

		GtkListStore *store = gtk_list_store_new (2, G_TYPE_STRING, GDK_TYPE_PIXBUF);
		gtk_combo_box_set_model (GTK_COMBO_BOX (getWidget()), GTK_TREE_MODEL (store));
		g_object_unref (store);

		if(opt.isEditable.value())
			gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (getWidget()), 0);
		else {
			GtkCellRenderer* cell = gtk_cell_renderer_text_new ();
			gtk_cell_layout_pack_end (GTK_CELL_LAYOUT (getWidget()), cell, TRUE);
			gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (getWidget()), cell,
			                                "text", 0, NULL);
		}

		g_signal_connect (G_OBJECT (getWidget()), "changed",
		                  G_CALLBACK (selected_changed_cb), this);
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

	virtual void itemAdded (const YCPString &string, int index, bool selected)
	{
		GtkListStore *store = GTK_LIST_STORE (gtk_combo_box_get_model (getComboBox()));
		GtkTreeIter iter;
		gtk_list_store_insert (store, &iter, index);
		gtk_list_store_set (store, &iter, 0, string->value_cstr(), -1);

		if(hasIcons() && !m_showingIcons) {
			GtkCellRenderer* cell = gtk_cell_renderer_pixbuf_new ();
			gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (getWidget()), cell, FALSE);
			gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (getWidget()), cell,
			                                "pixbuf", 1, NULL);
			m_showingIcons = true;
		}

		if (hasIcons() && !itemIcon (index)->value().empty()) {
			std::string path = itemIcon (index)->value();
			if (path[0] != '/')
				path = ICON_DIR + path;

			GdkPixbuf *pixbuf;
			GError *error = 0;
			pixbuf = gdk_pixbuf_new_from_file (path.c_str(), &error);
			if (!pixbuf)
				y2warning ("YGComboBox: Could not load icon: %s.\n"
				           "Because %s", path.c_str(), error->message);
			gtk_list_store_set (store, &iter, 1, pixbuf, -1);
		}

		if (selected || index == 0)
			gtk_combo_box_set_active (getComboBox(), index);
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

	YGWIDGET_IMPL_COMMON
	YGLABEL_WIDGET_IMPL_SET_LABEL_CHAIN(YComboBox)
};

YWidget *
YGUI::createComboBox (YWidget *parent, YWidgetOpt & opt,
                      const YCPString & label)
{
	return new YGComboBox (opt, YGWidget::get (parent), label);
}
