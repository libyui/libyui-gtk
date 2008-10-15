/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkExtEntry widget */
// check the header file for information about this widget

#include <config.h>
#include "ygtkfindentry.h"
#include <gtk/gtk.h>

extern GdkPixbuf *ygutils_setOpacity (const GdkPixbuf *src, int opacity);

static guint menu_item_selected_signal = 0;

G_DEFINE_TYPE (YGtkExtEntry, ygtk_ext_entry, GTK_TYPE_ENTRY)

static void ygtk_ext_entry_init (YGtkExtEntry *entry)
{
}

static void ygtk_ext_entry_destroy (GtkObject *object)
{
	GTK_OBJECT_CLASS (ygtk_ext_entry_parent_class)->destroy (object);

	YGtkExtEntry *entry = YGTK_EXT_ENTRY (object);
	ygtk_ext_entry_set_border_window_size (entry, YGTK_EXT_ENTRY_LEFT_WIN, 0);
	ygtk_ext_entry_set_border_window_size (entry, YGTK_EXT_ENTRY_RIGHT_WIN, 0);
}

static void ygtk_ext_entry_map (GtkWidget *widget)
{
	if (GTK_WIDGET_REALIZED (widget) && !GTK_WIDGET_MAPPED (widget)) {
		GTK_WIDGET_CLASS (ygtk_ext_entry_parent_class)->map (widget);
		YGtkExtEntry *entry = YGTK_EXT_ENTRY (widget);
		if (entry->left_window)
			gdk_window_show (entry->left_window);
		if (entry->right_window)
			gdk_window_show (entry->right_window);
	}
}

static void ygtk_ext_entry_unmap (GtkWidget *widget)
{
	if (GTK_WIDGET_MAPPED (widget)) {
		YGtkExtEntry *entry = YGTK_EXT_ENTRY (widget);
		if (entry->left_window)
			gdk_window_hide (entry->left_window);
		if (entry->right_window)
			gdk_window_hide (entry->right_window);

		GTK_WIDGET_CLASS (ygtk_ext_entry_parent_class)->unmap (widget);
	}
}

static void ygtk_ext_entry_sync_color (YGtkExtEntry *entry)
{
	/* We don't use gtk_style_set_background() since we want to reflect
	   gtk_widget_modify_base() color, if changed. */

	GtkWidget *widget = GTK_WIDGET (entry);
	GdkColor color = widget->style->base [GTK_STATE_NORMAL];
	gdk_rgb_find_color (gtk_widget_get_colormap (widget), &color);

	if (entry->left_window)
		gdk_window_set_background (entry->left_window, &color);
	if (entry->right_window)
		gdk_window_set_background (entry->right_window, &color);
}

static void ygtk_ext_entry_style_set (GtkWidget *widget, GtkStyle *prev_style)
{
	GTK_WIDGET_CLASS (ygtk_ext_entry_parent_class)->style_set (widget, prev_style);
	ygtk_ext_entry_sync_color (YGTK_EXT_ENTRY (widget));
}

GdkWindow *ygtk_ext_entry_get_window (YGtkExtEntry *entry,
                                      YGtkExtEntryWindowType type)
{
	switch (type) {
		case YGTK_EXT_ENTRY_WIDGET_WIN:
			return gtk_widget_get_parent_window (GTK_WIDGET (entry));
		case YGTK_EXT_ENTRY_TEXT_WIN:
			return GTK_ENTRY (entry)->text_area;
		case YGTK_EXT_ENTRY_LEFT_WIN:
			return entry->left_window;
		case YGTK_EXT_ENTRY_RIGHT_WIN:
			return entry->right_window;
	}
	return NULL;
}

void ygtk_ext_entry_set_border_window_size (YGtkExtEntry *entry,
                                            YGtkExtEntryWindowType type, gint size)
{
	GtkWidget *widget = GTK_WIDGET (entry);
	g_return_if_fail (type == YGTK_EXT_ENTRY_LEFT_WIN || type == YGTK_EXT_ENTRY_RIGHT_WIN);

	GdkWindow **window;
	if (type == YGTK_EXT_ENTRY_LEFT_WIN)
		window = &entry->left_window;
	else
		window = &entry->right_window;

	if (size) {
		if (!*window) {
			// must be realized to create a window
			GdkWindowAttr attributes;
			gint attributes_mask;
			attributes.window_type = GDK_WINDOW_CHILD;
			attributes.wclass = GDK_INPUT_OUTPUT;
			attributes.visual = gtk_widget_get_visual (widget);
			attributes.colormap = gtk_widget_get_colormap (widget);
			attributes.event_mask = gtk_widget_get_events (widget);
			attributes.event_mask |= GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK 
				| GDK_BUTTON_RELEASE_MASK | GDK_LEAVE_NOTIFY_MASK
				| GDK_ENTER_NOTIFY_MASK | GDK_POINTER_MOTION_MASK
				| GDK_POINTER_MOTION_HINT_MASK;
			attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
			// this will be tuned on size_allocate
			if (type == YGTK_EXT_ENTRY_LEFT_WIN)
				attributes.x = 0;
			else
				attributes.x = widget->allocation.width - size;
			attributes.y = widget->allocation.y;
			attributes.width = size;
			attributes.height = widget->allocation.height;

			*window = gdk_window_new (widget->window,
			                          &attributes, attributes_mask);
			gdk_window_set_user_data (*window, widget);
			// set background style
			ygtk_ext_entry_sync_color (entry);

			if (GTK_WIDGET_MAPPED (widget))
				gdk_window_show (*window);
			gtk_widget_queue_resize (widget);
		}
		else {
			gint width, height;
			gdk_drawable_get_size (GDK_DRAWABLE (*window), &width, &height);
			if (width != size) {
				gdk_window_resize (*window, size, height);
				gtk_widget_queue_resize (widget);
			}
		}
	}
	else {  // remove the window
		if (*window) {
			gdk_window_set_user_data (*window, NULL);
			gdk_window_destroy (*window);
			*window = NULL;
			gtk_widget_queue_resize (widget);
		}
    }
}

gint ygtk_ext_entry_get_border_window_size (YGtkExtEntry *entry,
                                            YGtkExtEntryWindowType type)
{
	g_return_val_if_fail (type == YGTK_EXT_ENTRY_LEFT_WIN || type == YGTK_EXT_ENTRY_RIGHT_WIN, 0);
	GdkWindow *window = ygtk_ext_entry_get_window (entry, type);
	gint size = 0;
	if (window && gdk_window_is_visible (window))
		gdk_drawable_get_size (GDK_DRAWABLE (window), &size, NULL);
	return size;
}

static void ygtk_ext_entry_size_request (GtkWidget *widget, GtkRequisition *req)
{
	GTK_WIDGET_CLASS (ygtk_ext_entry_parent_class)->size_request (widget, req);

#if 0
	YGtkExtEntry *entry = YGTK_EXT_ENTRY (widget);
	req->width += ygtk_ext_entry_get_border_window_size (entry,
	                                                     YGTK_EXT_ENTRY_LEFT_WIN);
	req->width += ygtk_ext_entry_get_border_window_size (entry,
	                                                     YGTK_EXT_ENTRY_RIGHT_WIN);
#endif
}

static void ygtk_ext_entry_size_allocate (GtkWidget *widget,
                                          GtkAllocation *allocation)
{
	GTK_WIDGET_CLASS (ygtk_ext_entry_parent_class)->size_allocate
	                                                    (widget, allocation);
	if (!GTK_WIDGET_REALIZED (widget))
		return;

	YGtkExtEntry *entry = YGTK_EXT_ENTRY (widget);
	gint left_border, right_border;
	left_border = ygtk_ext_entry_get_border_window_size (entry,
	                                                     YGTK_EXT_ENTRY_LEFT_WIN);
	right_border = ygtk_ext_entry_get_border_window_size (entry,
	                                                      YGTK_EXT_ENTRY_RIGHT_WIN);

	GdkWindow *window;
	gint _x, _y, _w, _h, x, w;

	// text window
	window = ygtk_ext_entry_get_window (entry, YGTK_EXT_ENTRY_TEXT_WIN);
	gdk_window_get_geometry (window, &_x, &_y, &_w, &_h, NULL);
	if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
		x = _x + left_border;
	else
		x = _x + right_border;
	w = _w - (left_border + right_border);
	gdk_window_move_resize (window, x, _y, w, _h);

	// left window
	window = ygtk_ext_entry_get_window (entry, YGTK_EXT_ENTRY_LEFT_WIN);
	if (window && gdk_window_is_visible (window)) {
		if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
			x = _x;
		else
			x = _x + _w - left_border;
		w = left_border;
		gdk_window_move_resize (window, x, _y, w, _h);
	}

	// right window
	window = ygtk_ext_entry_get_window (entry, YGTK_EXT_ENTRY_RIGHT_WIN);
	if (window && gdk_window_is_visible (window)) {
		if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
			x = _x + _w - right_border;
		else
			x = _x;
		w = right_border;
		gdk_window_move_resize (window, x, _y, w, _h);
	}
}

GtkWidget *ygtk_ext_entry_new (void)
{
    return g_object_new (YGTK_TYPE_EXT_ENTRY, NULL);
}

static void ygtk_ext_entry_class_init (YGtkExtEntryClass *klass)
{
	GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);
	gtkwidget_class->map = ygtk_ext_entry_map;
	gtkwidget_class->unmap = ygtk_ext_entry_unmap;
	gtkwidget_class->style_set = ygtk_ext_entry_style_set;
	gtkwidget_class->size_request = ygtk_ext_entry_size_request;
	gtkwidget_class->size_allocate = ygtk_ext_entry_size_allocate;

	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
	gtkobject_class->destroy = ygtk_ext_entry_destroy;
}

/* YGtkFindEntry widget */
// check the header file for information about this widget

static void generate_icon (GtkWidget *widget, const char *stock_icon,
                           GdkPixbuf **normal_icon, GdkPixbuf **hover_icon)
{
	if (*normal_icon) {
		g_object_unref (G_OBJECT (*normal_icon));
		*normal_icon = NULL;
	}
	if (*hover_icon) {
		g_object_unref (G_OBJECT (*hover_icon));
		*hover_icon = NULL;
	}
	if (stock_icon) {
		*normal_icon = gtk_widget_render_icon (widget, stock_icon, GTK_ICON_SIZE_MENU, NULL);
		*hover_icon = ygutils_setOpacity (*normal_icon, 88);
	}
}

#include <string.h>
#define ARROW_SIZE 7

static void ygtk_find_entry_editable_init (GtkEditableClass *iface);

G_DEFINE_TYPE_WITH_CODE (YGtkFindEntry, ygtk_find_entry, YGTK_TYPE_EXT_ENTRY,
	G_IMPLEMENT_INTERFACE (GTK_TYPE_EDITABLE, ygtk_find_entry_editable_init))

static void ygtk_find_entry_init (YGtkFindEntry *entry)
{
	entry->selected_item = -1;
}

static void ygtk_find_entry_destroy (GtkObject *object)
{
	GTK_OBJECT_CLASS (ygtk_find_entry_parent_class)->destroy (object);

	YGtkFindEntry *fentry = YGTK_FIND_ENTRY (object);
	generate_icon (NULL, NULL, &fentry->find_icon, &fentry->find_hover_icon);
	generate_icon (NULL, NULL, &fentry->clear_icon, &fentry->clear_hover_icon);

	if (fentry->completion_timer_id) {
		g_source_remove (fentry->completion_timer_id);
		fentry->completion_timer_id = 0;
	}
}

static void ygtk_find_entry_set_borders (YGtkFindEntry *entry)
{
	YGtkExtEntry *eentry = YGTK_EXT_ENTRY (entry);
	int left_border = 0, right_border = 0;
	if (entry->find_icon) {
		left_border = gdk_pixbuf_get_width (entry->find_icon) + 2;
		if (entry->context_menu)
			left_border += ARROW_SIZE - 1;
	}
	if (entry->clear_icon)
	    right_border = gdk_pixbuf_get_width (entry->clear_icon) + 2;

	ygtk_ext_entry_set_border_window_size (eentry,
		YGTK_EXT_ENTRY_LEFT_WIN, left_border);
	ygtk_ext_entry_set_border_window_size (eentry,
		YGTK_EXT_ENTRY_RIGHT_WIN, right_border);
	gtk_widget_queue_resize (GTK_WIDGET (entry));
}

static void generate_find_icon (YGtkFindEntry *entry)
{
	char *stock = GTK_STOCK_FIND;
	if (entry->context_menu && entry->selected_item >= 0) {
		GtkImageMenuItem *item;
		GList *items = gtk_container_get_children (GTK_CONTAINER (entry->context_menu));
		item = (GtkImageMenuItem *) g_list_nth_data (items, entry->selected_item);
		g_list_free (items);
		gtk_image_get_stock (GTK_IMAGE (gtk_image_menu_item_get_image (item)), &stock, NULL);
	}
	generate_icon (GTK_WIDGET (entry), stock, &entry->find_icon, &entry->find_hover_icon);
}

static void ygtk_find_entry_realize (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (ygtk_find_entry_parent_class)->realize (widget);

	YGtkExtEntry *eentry = YGTK_EXT_ENTRY (widget);
	YGtkFindEntry *fentry = YGTK_FIND_ENTRY (widget);

	generate_find_icon (fentry);
	generate_icon (widget, GTK_STOCK_CLEAR, &fentry->clear_icon, &fentry->clear_hover_icon);
	ygtk_find_entry_set_borders (fentry);

	GdkDisplay *display = gtk_widget_get_display (widget);
	GdkCursor *cursor = gdk_cursor_new_for_display (display, GDK_HAND2);

	gdk_window_set_cursor (eentry->left_window, cursor);
	gdk_window_set_cursor (eentry->right_window, cursor);
	gdk_cursor_unref (cursor);
	gdk_window_hide (eentry->right_window);  // show when text is inserted
}

static void ygtk_find_entry_map (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (ygtk_find_entry_parent_class)->map (widget);
	if (GTK_WIDGET_REALIZED (widget)) {
		// only show clear icon when the entry has text
		GdkWindow *clear_win = YGTK_EXT_ENTRY (widget)->right_window;
		if (clear_win)
			gdk_window_hide (clear_win);
	}
}

static gboolean ygtk_find_entry_expose (GtkWidget *widget, GdkEventExpose *event)
{
	YGtkExtEntry *eentry = YGTK_EXT_ENTRY (widget);
	YGtkFindEntry *fentry = YGTK_FIND_ENTRY (widget);

	GdkWindow *hover_window = gdk_display_get_window_at_pointer (
		gtk_widget_get_display (widget), NULL, NULL);

	if (event->window == eentry->left_window ||
	    event->window == eentry->right_window) {
		gboolean hover = hover_window == event->window;
		GdkPixbuf *pixbuf;
		if (event->window == eentry->left_window)
			pixbuf = hover ? fentry->find_hover_icon : fentry->find_icon;
		else
			pixbuf = hover ? fentry->clear_hover_icon : fentry->clear_icon;

		int pix_height = gdk_pixbuf_get_height (pixbuf), win_width, win_height, y;
		gdk_drawable_get_size (event->window, &win_width, &win_height);
		y = (win_height - pix_height) / 2;

		gdk_draw_pixbuf (event->window, widget->style->fg_gc[0], pixbuf,
		                 0, 0, 1, y, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);

		if (fentry->context_menu && event->window == eentry->left_window)
			gtk_paint_arrow (widget->style, event->window, GTK_STATE_NORMAL,
			                 GTK_SHADOW_NONE, &event->area, widget, NULL,
			                 GTK_ARROW_DOWN, FALSE,
			                 win_width - ARROW_SIZE - 1, win_height - ARROW_SIZE,
			                 ARROW_SIZE, ARROW_SIZE);
	}
	else
		GTK_WIDGET_CLASS (ygtk_find_entry_parent_class)->expose_event (widget, event);
	return TRUE;
}

/* Re-draw find/clear windows on mouse hover. */
static gboolean ygtk_find_entry_enter_leave_notify_event (GtkWidget *widget,
                                                          GdkEventCrossing *event)
{
	if (widget == gtk_get_event_widget ((GdkEvent*) event)) {
		YGtkExtEntry *eentry = YGTK_EXT_ENTRY (widget);
		if (event->window == eentry->left_window ||
		    event->window == eentry->right_window)
			gdk_window_invalidate_rect (event->window, NULL, FALSE);
	}
	return FALSE;
}

static void popup_menu_position (GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer pdata)
{
	GtkWidget *widget = (GtkWidget *) pdata;
	GtkAllocation *alloc = &widget->allocation;
	gdk_window_get_origin (widget->window, x, y);
	*y += alloc->height;
	*push_in = TRUE;

	GtkRequisition req;
	gtk_widget_size_request (GTK_WIDGET (menu), &req);
	if (alloc->width > req.width)
		gtk_widget_set_size_request (GTK_WIDGET (menu), alloc->width, -1);
}

static void ygtk_find_entry_actual_popup_menu (GtkWidget *widget, guint button,
                                               guint32 time)
{
	GtkMenu *menu = YGTK_FIND_ENTRY (widget)->context_menu;
	gtk_menu_popup (menu, NULL, NULL, popup_menu_position, widget, button, time);
}

static gboolean ygtk_find_entry_popup_menu (GtkWidget *widget)
{
	if (YGTK_FIND_ENTRY (widget)->context_menu) {
		ygtk_find_entry_actual_popup_menu (widget, 0, gtk_get_current_event_time());
		return TRUE;
	}
	return 	GTK_WIDGET_CLASS (ygtk_find_entry_parent_class)->popup_menu (widget);
}

static gboolean ygtk_find_entry_button_press_event (GtkWidget *widget,
                                                    GdkEventButton *event)
{
	YGtkFindEntry *fentry = YGTK_FIND_ENTRY (widget);
	YGtkExtEntry *eentry = YGTK_EXT_ENTRY (widget);
	if (event->window == eentry->left_window) {
		// If the entry has an associated context menu, use it.
		// Otherwise, find icon selects entry's text.
		gtk_widget_grab_focus (widget);
		if (fentry->context_menu)
			ygtk_find_entry_actual_popup_menu (widget, event->button, event->time);
		else
			gtk_editable_select_region (GTK_EDITABLE (widget), 0, -1);
	}
	else if (event->window == eentry->right_window) {
		gtk_editable_delete_text (GTK_EDITABLE (widget), 0, -1);
		gtk_widget_grab_focus (widget);
	}
	else
		return GTK_WIDGET_CLASS (ygtk_find_entry_parent_class)->button_press_event
		                                                            (widget, event);
	return TRUE;
}

static gboolean ygtk_find_entry_is_empty (YGtkFindEntry *entry)
{
	return *gtk_entry_get_text (GTK_ENTRY (entry)) == '\0';
}

static void ygtk_find_entry_insert_completion (YGtkFindEntry *fentry, const char *text)
{
	if (strlen (text) < 5)
		return;
	GtkEntry *entry = GTK_ENTRY (fentry);
	GtkEntryCompletion *completion = gtk_entry_get_completion (entry);
	if (!completion) {
		completion = gtk_entry_completion_new();
		gtk_entry_set_completion (entry, completion);
		g_object_unref (completion);

		GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
		gtk_entry_completion_set_model (completion, GTK_TREE_MODEL (store));
		g_object_unref (G_OBJECT (store));
		gtk_entry_completion_set_text_column (completion, 0);
	}

	GtkTreeModel *model = gtk_entry_completion_get_model (completion);
	GtkTreeIter iter;
	gboolean already_there = FALSE;
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			gchar *i;
			gtk_tree_model_get (model, &iter, 0, &i, -1);
			if (!g_ascii_strcasecmp (text, i))
				already_there = TRUE;
			g_free (i);
		} while (gtk_tree_model_iter_next (model, &iter) && !already_there);
	}
	if (!already_there) {
		GtkListStore *store = GTK_LIST_STORE (model);
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, text, -1);
	}
}

#define INSERT_COMPLETION_TIMOUT 500

static gboolean completion_timeout_cb (void *pdata)
{
	YGtkFindEntry *fentry = (YGtkFindEntry *) pdata;
	GtkEntry *entry = GTK_ENTRY (fentry);
	const char *text = gtk_entry_get_text (entry);
	if (*text)
		ygtk_find_entry_insert_completion (fentry, text);
	fentry->completion_timer_id = 0;
	return FALSE;
}

static void ygtk_find_entry_insert_text (GtkEditable *editable,
	const gchar *new_text, gint new_text_len, gint *pos)
{
	YGtkFindEntry *fentry = YGTK_FIND_ENTRY (editable);
	gboolean empty = ygtk_find_entry_is_empty (fentry);

	GtkEditableClass *parent_editable_iface = g_type_interface_peek
		(ygtk_find_entry_parent_class, GTK_TYPE_EDITABLE);
	parent_editable_iface->insert_text (editable, new_text, new_text_len, pos);

	GdkWindow *clear_win = YGTK_EXT_ENTRY (fentry)->right_window;
	if (empty && *new_text && clear_win) {
		gdk_window_show (clear_win);
		gtk_widget_queue_resize (GTK_WIDGET (editable));
	}
	if (fentry->completion_timer_id)
		g_source_remove (fentry->completion_timer_id);
	fentry->completion_timer_id = g_timeout_add (INSERT_COMPLETION_TIMOUT,
	                                             completion_timeout_cb, fentry);
}

static void ygtk_find_entry_delete_text (GtkEditable *editable, gint start_pos,
                                         gint end_pos)
{
	GtkEditableClass *parent_editable_iface = g_type_interface_peek
		(ygtk_find_entry_parent_class, GTK_TYPE_EDITABLE);
	parent_editable_iface->delete_text (editable, start_pos, end_pos);

	YGtkFindEntry *fentry = YGTK_FIND_ENTRY (editable);
	gboolean empty = ygtk_find_entry_is_empty (fentry);
	GdkWindow *clear_win = YGTK_EXT_ENTRY (fentry)->right_window;
	if (empty && clear_win) {
		gdk_window_hide (clear_win);
		gtk_widget_queue_resize (GTK_WIDGET (editable));
	}
	if (fentry->completion_timer_id)
		g_source_remove (fentry->completion_timer_id);
	fentry->completion_timer_id = 0;
}

void ygtk_find_entry_attach_menu (YGtkFindEntry *entry, GtkMenu *menu)
{
	if (entry->context_menu)
		gtk_menu_detach (entry->context_menu);
	entry->context_menu = menu;
	if (menu)
		gtk_menu_attach_to_widget (menu, GTK_WIDGET (entry), NULL);
	ygtk_find_entry_set_borders (entry);
}

static void menu_item_activate_cb (GtkMenuItem *item, YGtkFindEntry *entry)
{
	GtkWidget *menu = gtk_widget_get_parent (GTK_WIDGET (item));
	GList *items = gtk_container_get_children (GTK_CONTAINER (menu));
	entry->selected_item = g_list_index (items, item);
	g_signal_emit (entry, menu_item_selected_signal, 0, entry->selected_item);
	g_list_free (items);
	generate_find_icon (entry);
}

guint ygtk_find_entry_insert_item (YGtkFindEntry *entry, const char *text, const char *stock)
{
	if (!entry->context_menu)
		ygtk_find_entry_attach_menu (entry, GTK_MENU (gtk_menu_new()));
	GtkWidget *item = gtk_image_menu_item_new_with_label (text);
	GtkWidget *image = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
	gtk_menu_shell_append (GTK_MENU_SHELL (entry->context_menu), item);
	gtk_widget_show_all (GTK_WIDGET (entry->context_menu));
	g_signal_connect (G_OBJECT (item), "activate",
		              G_CALLBACK (menu_item_activate_cb), entry);
	GList *items = gtk_container_get_children (GTK_CONTAINER (entry->context_menu));
	guint nb = g_list_length (items)-1;
	g_list_free (items);
	return nb;
}

gint ygtk_find_entry_get_selected_item (YGtkFindEntry *entry)
{
	return entry->selected_item;
}

GtkWidget *ygtk_find_entry_new (void /*gboolean will_use_find_icon */)
{
    return g_object_new (YGTK_TYPE_FIND_ENTRY, NULL);
}

static void ygtk_find_entry_class_init (YGtkFindEntryClass *klass)
{
	GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);
	gtkwidget_class->realize = ygtk_find_entry_realize;
	gtkwidget_class->map = ygtk_find_entry_map;
	gtkwidget_class->expose_event = ygtk_find_entry_expose;
	gtkwidget_class->enter_notify_event = ygtk_find_entry_enter_leave_notify_event;
	gtkwidget_class->leave_notify_event = ygtk_find_entry_enter_leave_notify_event;
	gtkwidget_class->button_press_event = ygtk_find_entry_button_press_event;
	gtkwidget_class->popup_menu = ygtk_find_entry_popup_menu;

	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
	gtkobject_class->destroy = ygtk_find_entry_destroy;

	menu_item_selected_signal = g_signal_new ("menu_item_selected",
		G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET (YGtkFindEntryClass, menu_item_selected),
		NULL, NULL, gtk_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);
}

static void ygtk_find_entry_editable_init (GtkEditableClass *iface)
{
  iface->insert_text = ygtk_find_entry_insert_text;
  iface->delete_text = ygtk_find_entry_delete_text;
}

