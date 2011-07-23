/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* YGtkPkgQueryCombo, an umbrella for any group of QueryWidget widgets */
// check the header file for information about this widget

/*
  Textdomain "yast2-gtk"
 */

#define YUILogComponent "gtk-pkg"
#include "YGi18n.h"
#include "config.h"
#include "YGUtils.h"
#include "ygtkpkgquerycombo.h"
#include <gtk/gtk.h>

struct YGtkPkgQueryCombo::Impl {
	GtkWidget *vbox, *combo, *content;
	Factory *factory;
	YGtkPkgQueryWidget *child;

	Impl (Factory *factory) : factory (factory), child (NULL) {}

	~Impl()
	{ delete child; 	}
};

static void set_child (YGtkPkgQueryCombo *pThis, int index)
{
	if (pThis->impl->child)
		delete pThis->impl->child;
	YGtkPkgQueryWidget *child = pThis->impl->factory->createQueryWidget (pThis, index);
	pThis->impl->child = child;
	child->setListener (pThis->listener);

	GtkWidget *cur_child = gtk_bin_get_child (GTK_BIN (pThis->impl->content));
	if (cur_child)
		gtk_container_remove (GTK_CONTAINER (pThis->impl->content), cur_child);
	gtk_container_add (GTK_CONTAINER (pThis->impl->content), child->getWidget());
	gtk_widget_grab_focus (child->getWidget());
}

static void combo_changed_cb (GtkComboBox *combo, YGtkPkgQueryCombo *pThis)
{
	Ypp::Busy busy (0);
	set_child (pThis, gtk_combo_box_get_active (combo));
	pThis->notify();
}

YGtkPkgQueryCombo::YGtkPkgQueryCombo (Factory *factory)
: impl (new Impl (factory))
{
	impl->combo = gtk_combo_box_text_new();
	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (impl->combo),
		YGUtils::empty_row_is_separator_cb, GINT_TO_POINTER (0), NULL);
	g_signal_connect_after (G_OBJECT (impl->combo), "changed",
	                        G_CALLBACK (combo_changed_cb), this);
	impl->content = gtk_event_box_new();
	impl->vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (impl->vbox), impl->combo, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (impl->vbox), impl->content, TRUE, TRUE, 0);
}

YGtkPkgQueryCombo::~YGtkPkgQueryCombo()
{ delete impl; }

GtkWidget *YGtkPkgQueryCombo::getWidget()
{ return impl->vbox; }

void YGtkPkgQueryCombo::add (const char *title)
{
	GtkComboBoxText *combo = GTK_COMBO_BOX_TEXT (impl->combo);
	gtk_combo_box_text_append (combo, NULL, title);
}

void YGtkPkgQueryCombo::setActive (int index)
{
	GtkComboBox *combo = GTK_COMBO_BOX (impl->combo);
	if (gtk_combo_box_get_active (combo) != index) {
		g_signal_handlers_block_by_func (combo, (gpointer) combo_changed_cb, this);
		gtk_combo_box_set_active (combo, index);
		g_signal_handlers_unblock_by_func (combo, (gpointer) combo_changed_cb, this);
		set_child (this, index);
	}
}

bool YGtkPkgQueryCombo::begsUpdate()
{ return impl->child->begsUpdate(); }

void YGtkPkgQueryCombo::updateList (Ypp::List list)
{ return impl->child->updateList (list); }

void YGtkPkgQueryCombo::clearSelection()
{ return impl->child->clearSelection(); }

bool YGtkPkgQueryCombo::writeQuery (Ypp::PoolQuery &query)
{ return impl->child->writeQuery (query); }

void YGtkPkgQueryCombo::setListener (Listener *listener)
{
	YGtkPkgQueryWidget::setListener (listener);
	if (impl->child)
		impl->child->setListener (listener);
}

GtkWidget *YGtkPkgQueryCombo::createToolbox()
{ return impl->child->createToolbox(); }

