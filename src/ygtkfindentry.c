//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

/* YGtkFindEntry widget */
// check the header file for information about this widget

#include "ygtkfindentry.h"
#include <gtk/gtk.h>
#include <string.h>

G_DEFINE_TYPE (YGtkFindEntry, ygtk_find_entry, GTK_TYPE_FRAME)

static gboolean inner_entry_focus_cb (GtkWidget *widget, GdkEventFocus *event,
                                      YGtkFindEntry *entry)
{
	gtk_widget_queue_draw (GTK_WIDGET (entry));
	return FALSE;
}

static void inner_entry_changed_cb (GtkEditable *editable, YGtkFindEntry *entry)
{
	gboolean has_text = strlen (gtk_entry_get_text (GTK_ENTRY (editable)));
	if (has_text)
		gtk_widget_show (entry->clear_icon);
	else
		gtk_widget_hide (entry->clear_icon);
}

static gboolean inner_clear_button_pressed_cb (GtkWidget *widget, GdkEventButton *event,
                                               YGtkFindEntry *entry)
{
	gtk_editable_delete_text (GTK_EDITABLE (entry->entry), 0, -1);
	return TRUE;
}

static gboolean inner_find_button_pressed_cb (GtkWidget *widget, GdkEventButton *event,
                                              YGtkFindEntry *entry)
{
	gtk_widget_grab_focus (entry->entry);
	gtk_editable_select_region (GTK_EDITABLE (entry->entry), 0, -1);
	return TRUE;
}

static void ygtk_find_entry_init (YGtkFindEntry *entry)
{
	GtkWidget *widget = GTK_WIDGET (entry);

	entry->find_icon = gtk_event_box_new();
	entry->clear_icon = gtk_event_box_new();
	gtk_container_add (GTK_CONTAINER (entry->find_icon),
	                   gtk_image_new_from_stock (GTK_STOCK_FIND, GTK_ICON_SIZE_MENU));
	gtk_container_add (GTK_CONTAINER (entry->clear_icon),
	                   gtk_image_new_from_stock (GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU));

	entry->entry = gtk_entry_new();
	widget->style->xthickness = entry->entry->style->xthickness;
	widget->style->ythickness = entry->entry->style->ythickness;
	gtk_entry_set_has_frame (GTK_ENTRY (entry->entry), FALSE);

	GtkWidget *box = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (box), entry->find_icon, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), entry->entry, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (box), entry->clear_icon, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (entry), box);

	gtk_widget_show (entry->entry);
	gtk_widget_show_all (entry->find_icon);
	gtk_widget_show (GTK_BIN (entry->clear_icon)->child);

	/* re-paint frame on focus */
	g_signal_connect (G_OBJECT (entry->entry), "focus-in-event",
	                  G_CALLBACK (inner_entry_focus_cb), entry);
	g_signal_connect (G_OBJECT (entry->entry), "focus-out-event",
	                  G_CALLBACK (inner_entry_focus_cb), entry);
	/* clear button handling */
	g_signal_connect (G_OBJECT (entry->entry), "changed",
	                  G_CALLBACK (inner_entry_changed_cb), entry);
	g_signal_connect (G_OBJECT (entry->clear_icon), "button-press-event",
	                  G_CALLBACK (inner_clear_button_pressed_cb), entry);

	// fixes size changing due to clear button getting hide
	gtk_widget_set_size_request (widget, 170, -1);
}

static void ygtk_find_entry_realize (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (ygtk_find_entry_parent_class)->realize (widget);

	GdkDisplay *display = gtk_widget_get_display (widget);
	GdkCursor *cursor = gdk_cursor_new_for_display (display, GDK_HAND1);

	YGtkFindEntry *entry = YGTK_FIND_ENTRY (widget);
	gtk_widget_realize (entry->find_icon);
	gtk_widget_realize (entry->clear_icon);

	gdk_window_set_cursor (entry->find_icon->window, cursor);
	gdk_window_set_cursor (entry->clear_icon->window, cursor);
	gdk_cursor_unref (cursor);
/*
	gdk_window_clear (entry->find_icon->window);
	gdk_window_clear (entry->clear_icon->window);
*/
	gtk_widget_hide (entry->clear_icon);
}

static void ygtk_find_entry_size_request (GtkWidget *widget, GtkRequisition *req)
{
	GTK_WIDGET_CLASS (ygtk_find_entry_parent_class)->size_request (widget, req);
	req->width += widget->style->xthickness*2;
	req->height += widget->style->ythickness*2;
}

static void ygtk_find_entry_size_allocate (GtkWidget *widget, GtkAllocation *alloc)
{
	GtkRequisition req;
	gtk_widget_get_child_requisition (GTK_BIN (widget)->child, &req);

	widget->allocation = *alloc;
	widget->allocation.y += (widget->allocation.height - req.height) / 2;
	widget->allocation.height = req.height;

	GtkAllocation child_alloc = widget->allocation;
	child_alloc.x += widget->style->xthickness + 2;
	child_alloc.width -= widget->style->xthickness*2 + 4;

	widget->allocation.y -= widget->style->ythickness;
	widget->allocation.height += widget->style->ythickness*2;

	gtk_widget_size_allocate (GTK_BIN (widget)->child, &child_alloc);
}

static gboolean ygtk_find_entry_expose (GtkWidget *widget, GdkEventExpose *event)
{
	// draw frame
	YGtkFindEntry *entry = YGTK_FIND_ENTRY (widget);
	gtk_container_propagate_expose (GTK_CONTAINER (widget), GTK_BIN (widget)->child, event);

	gint x, y, w, h;
	x = widget->allocation.x;
	y = widget->allocation.y;
	w = widget->allocation.width;
	h = widget->allocation.height;
	gtk_paint_shadow (widget->style, widget->window, GTK_STATE_NORMAL,
	                  GTK_SHADOW_IN, NULL, entry->entry, "entry", x, y, w, h);
	return TRUE;
}

GtkWidget *ygtk_find_entry_new (gboolean will_use_find_icon)
{
	GtkWidget *widget = g_object_new (YGTK_TYPE_FIND_ENTRY, NULL);
	if (!will_use_find_icon) {
		YGtkFindEntry *entry = YGTK_FIND_ENTRY (widget);
		g_signal_connect (G_OBJECT (entry->find_icon), "button-press-event",
		                  G_CALLBACK (inner_find_button_pressed_cb), entry);
	}
	return widget;
}

static void ygtk_find_entry_class_init (YGtkFindEntryClass *klass)
{
	GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);
	gtkwidget_class->realize = ygtk_find_entry_realize;
	gtkwidget_class->expose_event = ygtk_find_entry_expose;
	gtkwidget_class->size_request = ygtk_find_entry_size_request;
	gtkwidget_class->size_allocate = ygtk_find_entry_size_allocate;
}
