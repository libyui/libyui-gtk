/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkTimeZonePicker widget */
// check the header file for information about this widget

#include <config.h>
#include "ygtktimezonepicker.h"
#include <gtk/gtk.h>
#include <string.h>
#include <math.h>

static guint zone_clicked_signal;

// General utilities

static char *substring (const char *str, int start, int end)
{
	if (end == -1)
		return g_strdup (str+start);
	return g_strndup (str+start, end-start);
}

static gdouble convert_pos (const char *pos, int digits)
{
	if (strlen (pos) < 4 || digits > 9)
		return 0.0;

	gchar *whole = substring (pos, 0, digits+1);
	gchar *fraction = substring (pos, digits+1, -1);

	gdouble t1 = g_strtod (whole, NULL);
    gdouble t2 = g_strtod (fraction, NULL);

	int fraction_len = strlen (fraction);
	g_free (whole);
	g_free (fraction);

	if (t1 >= 0.0)
		return t1 + t2/pow (10.0, fraction_len);
	else
		return t1 - t2/pow (10.0, fraction_len);
}

static void ygtk_time_zone_picker_set_cursor_stock (YGtkTimeZonePicker *picker,
                                                    const gchar *stock)
{
	GdkPixbuf *pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(),
		stock, 24, 0, NULL);
	if (pixbuf) {
		GtkWidget *widget = GTK_WIDGET (picker);
		GdkDisplay *display = gtk_widget_get_display (widget);
		GdkCursor *cursor = gdk_cursor_new_from_pixbuf (display, pixbuf, 6, 6);
		gdk_window_set_cursor (picker->map_window, cursor);
		g_object_unref (G_OBJECT (pixbuf));
	}
}

static void ygtk_time_zone_picker_set_cursor_type (YGtkTimeZonePicker *picker,
                                                   GdkCursorType type)
{
	GtkWidget *widget = GTK_WIDGET (picker);
	GdkDisplay *display = gtk_widget_get_display (widget);
	GdkCursor *cursor = gdk_cursor_new_for_display (display, type);
	gdk_window_set_cursor (picker->map_window, cursor);
}

// Specific utilities

static void window_to_map (YGtkTimeZonePicker *picker, gint win_x, gint win_y,
                           gint *map_x, gint *map_y)
{
	int win_width, win_height;
	gdk_window_get_size (picker->map_window, &win_width, &win_height);

	*map_x = ((win_x - win_width/2) / picker->scale) + picker->map_x;
	*map_y = ((win_y - win_height/2) / picker->scale) + picker->map_y;
}

static void map_to_window (YGtkTimeZonePicker *picker, gint map_x, gint map_y,
                           gint *win_x, gint *win_y)
{
	int win_width, win_height;
	gdk_window_get_size (picker->map_window, &win_width, &win_height);

	*win_x = ((map_x - picker->map_x) * picker->scale) + win_width/2;
	*win_y = ((map_y - picker->map_y) * picker->scale) + win_height/2;
}

static void coordinates_to_map (YGtkTimeZonePicker *picker, gdouble latitude, gdouble longitude, gint *map_x, gint *map_y)
{
	*map_x = picker->map_width/2 + (picker->map_width/2 * longitude/180);
	*map_y = picker->map_height/2 - (picker->map_height/2 * latitude/90);
}

static YGtkTimeZoneLocation *find_location_closer_to (YGtkTimeZonePicker *picker,
	gint win_x, gint win_y)
{
	gint x, y;
	window_to_map (picker, win_x, win_y, &x, &y);

	double min_dist = 4000;
	YGtkTimeZoneLocation *best = 0;
	GList *i;
	for (i = picker->locations; i; i = i->next) {
		YGtkTimeZoneLocation *loc = i->data;
		gdouble dist = pow (loc->x - x, 2) + pow (loc->y - y, 2);
		if (dist < min_dist) {
			min_dist = dist;
			best = loc;
		}
	}
	return best;
}

// Internal methods

static void ygtk_time_zone_picker_sync_cursor (YGtkTimeZonePicker *picker)
{
	if (picker->last_mouse_x)
		ygtk_time_zone_picker_set_cursor_type (picker, GDK_FLEUR);
	else if (picker->closeup)
		ygtk_time_zone_picker_set_cursor_type (picker, GDK_CROSS);
	else
		ygtk_time_zone_picker_set_cursor_stock (picker, GTK_STOCK_ZOOM_IN);
}

static void ygtk_time_zone_picker_scroll_to (YGtkTimeZonePicker *picker,
                                             gint x, gint y, gboolean animate)
{
	picker->map_x = x;
	picker->map_y = y;
	gtk_widget_queue_resize (GTK_WIDGET (picker));
}

static void ygtk_time_zone_picker_move (YGtkTimeZonePicker *picker,
                                        gint diff_x, gint diff_y)
{
	ygtk_time_zone_picker_scroll_to (picker, picker->map_x + diff_x,
	                                 picker->map_y + diff_y, FALSE);
}

static void ygtk_time_zone_picker_closeup (YGtkTimeZonePicker *picker, gboolean closeup,
                                           gint map_x, gint map_y, gboolean animate)
{
	if (closeup)
		ygtk_time_zone_picker_scroll_to (picker, map_x, map_y, animate);
	picker->closeup = closeup;
	gtk_widget_queue_resize (GTK_WIDGET (picker));
	ygtk_time_zone_picker_sync_cursor (picker);
}

// Public methods

static gint compare_locations (gconstpointer pa, gconstpointer pb)
{
	const YGtkTimeZoneLocation *a = pa;
	const YGtkTimeZoneLocation *b = pb;
	return a->latitude - b->latitude;
}

void ygtk_time_zone_picker_set_map (YGtkTimeZonePicker *picker, const char *filename,
	TimeZoneToName converter_cb, gpointer converter_data)
{
	GError *error = 0;
	picker->map_pixbuf = gdk_pixbuf_new_from_file (filename, &error);
	if (picker->map_pixbuf) {
		picker->map_width = gdk_pixbuf_get_width (picker->map_pixbuf);
		picker->map_height = gdk_pixbuf_get_height (picker->map_pixbuf);
	}
	else {
		g_warning ("Couldn't load map: %s\n%s\n", filename, error ? error->message : "(unknown)");
		picker->map_width = 300; picker->map_height = 50;
	}

	char buf [4096];
	FILE *tzfile = fopen ("/usr/share/zoneinfo/zone.tab", "r");
	while (fgets (buf, sizeof (buf), tzfile)) {
		if (*buf == '#') continue;

		gchar *trim = g_strstrip (buf);
		gchar **arr = g_strsplit (trim, "\t", -1);

		int arr_length;
		for (arr_length = 0; arr [arr_length]; arr_length++) ;

		YGtkTimeZoneLocation *loc = g_new0 (YGtkTimeZoneLocation, 1);
		loc->country = g_strdup (arr[0]);
		loc->zone = g_strdup (arr[2]);
		if (arr_length > 3)
			loc->comment = g_strdup (arr[3]);
		const gchar *tooltip = converter_cb (loc->zone, converter_data);
		if (tooltip)
			loc->tooltip = g_strdup (tooltip);

		int split_i = 1;
		while (split_i < strlen (arr[1]) && arr[1][split_i] != '-' && arr[1][split_i] != '+' )
            split_i++;
		char *latitude = substring (arr[1], 0, split_i);
		char *longitude = substring (arr[1], split_i, -1);

		loc->latitude = convert_pos (latitude, 2);
        loc->longitude = convert_pos (longitude, 3);
        g_free (latitude);
        g_free (longitude);

		coordinates_to_map (picker, loc->latitude, loc->longitude, &loc->x, &loc->y);

		picker->locations = g_list_append (picker->locations, loc);
		g_strfreev (arr);
	}
	fclose (tzfile);
	picker->locations = g_list_sort (picker->locations, compare_locations);
}

const gchar *ygtk_time_zone_picker_get_current_zone (YGtkTimeZonePicker *picker)
{
	if (picker->selected_loc)
		return picker->selected_loc->zone;
	return NULL;
}

void ygtk_time_zone_picker_set_current_zone (YGtkTimeZonePicker *picker,
                                             const gchar *zone, gboolean zoom)
{
	if (picker->selected_loc && !strcmp (picker->selected_loc->zone, zone))
		return;
	GList *i;
	for (i = picker->locations; i; i = i->next) {
		YGtkTimeZoneLocation *loc = i->data;
		if (!strcmp (loc->zone, zone)) {
			picker->selected_loc = loc;
			ygtk_time_zone_picker_closeup (picker, zoom, loc->x, loc->y, TRUE);
			break;
		}
	}
	gtk_widget_queue_draw (GTK_WIDGET (picker));
}

// GTK stuff

G_DEFINE_TYPE (YGtkTimeZonePicker, ygtk_time_zone_picker, GTK_TYPE_WIDGET)

static void ygtk_time_zone_picker_init (YGtkTimeZonePicker *picker)
{
	GTK_WIDGET_SET_FLAGS (picker, GTK_NO_WINDOW);
}

static void ygtk_time_zone_picker_destroy (GtkObject *object)
{
	YGtkTimeZonePicker *picker = YGTK_TIME_ZONE_PICKER (object);
	if (picker->map_pixbuf) {
		g_object_unref (G_OBJECT (picker->map_pixbuf));
		picker->map_pixbuf = NULL;
	}
	if (picker->locations) {
		GList *i;
		for (i = picker->locations; i; i = i->next) {
			YGtkTimeZoneLocation *loc = i->data;
			g_free (loc->country);
			g_free (loc->zone);
			g_free (loc->comment);
			g_free (loc->tooltip);
			g_free (loc);
		}
		g_list_free (picker->locations);
		picker->locations = NULL;
	}
	GTK_OBJECT_CLASS (ygtk_time_zone_picker_parent_class)->destroy (object);
}

static void ygtk_time_zone_picker_realize (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (ygtk_time_zone_picker_parent_class)->realize (widget);

	YGtkTimeZonePicker *picker = YGTK_TIME_ZONE_PICKER (widget);

	GdkWindowAttr attributes;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.event_mask = gtk_widget_get_events (widget);
	attributes.event_mask |=
		(GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
		 | GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK);
	gint attributes_mask;
	attributes_mask = GDK_WA_X | GDK_WA_Y;
	picker->map_window = gdk_window_new (widget->window,
	                          &attributes, attributes_mask);
	gdk_window_set_user_data (picker->map_window, widget);
	gtk_style_set_background (widget->style, picker->map_window, GTK_STATE_NORMAL);

	ygtk_time_zone_picker_closeup (picker, FALSE, 0, 0, FALSE);
}

static void ygtk_time_zone_picker_unrealize (GtkWidget *widget)
{
	YGtkTimeZonePicker *picker = YGTK_TIME_ZONE_PICKER (widget);
	if (picker->map_window) {
		gdk_window_set_user_data (picker->map_window, NULL);
		gdk_window_destroy (picker->map_window);
		picker->map_window = NULL;
	}
	GTK_WIDGET_CLASS (ygtk_time_zone_picker_parent_class)->unrealize (widget);
}

static void ygtk_time_zone_picker_map (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (ygtk_time_zone_picker_parent_class)->map (widget);
	YGtkTimeZonePicker *picker = YGTK_TIME_ZONE_PICKER (widget);
	if (picker->map_window)
		gdk_window_show (picker->map_window);
}

static void ygtk_time_zone_picker_unmap (GtkWidget *widget)
{
	YGtkTimeZonePicker *picker = YGTK_TIME_ZONE_PICKER (widget);
	if (picker->map_window)
		gdk_window_hide (picker->map_window);
	GTK_WIDGET_CLASS (ygtk_time_zone_picker_parent_class)->unmap (widget);
}

static gboolean ygtk_time_zone_picker_motion_notify_event (GtkWidget *widget,
                                                           GdkEventMotion *event)
{
	YGtkTimeZonePicker *picker = YGTK_TIME_ZONE_PICKER (widget);
	if (event->window == picker->map_window) {
		if (picker->scale == 1) {
			YGtkTimeZoneLocation *loc;
			loc = find_location_closer_to (picker, event->x, event->y);
			if (picker->hover_loc != loc) {
				picker->hover_loc = loc;
				gtk_widget_queue_draw (widget);
			}
		}
		if (picker->last_mouse_x) {
			ygtk_time_zone_picker_move (picker, picker->last_mouse_x - event->x,
				                        picker->last_mouse_y - event->y);
			picker->last_mouse_x = event->x;
			picker->last_mouse_y = event->y;
			ygtk_time_zone_picker_sync_cursor (picker);
		}
	}
	return FALSE;
}

static gboolean ygtk_time_zone_picker_leave_notify_event (GtkWidget *widget,
                                                          GdkEventCrossing *event)
{
	YGtkTimeZonePicker *picker = YGTK_TIME_ZONE_PICKER (widget);
	if (picker->hover_loc) {
		picker->hover_loc = NULL;
		gtk_widget_queue_draw (widget);
	}
	return FALSE;
}

static gboolean ygtk_time_zone_picker_button_press_event (GtkWidget *widget,
                                                          GdkEventButton *event)
{
	YGtkTimeZonePicker *picker = YGTK_TIME_ZONE_PICKER (widget);
	if (event->window == picker->map_window) {
		if (event->button == 1) {
			if (picker->scale == 1) {
				YGtkTimeZoneLocation *loc;
				loc = find_location_closer_to (picker, event->x, event->y);
				if (loc && loc != picker->selected_loc) {
					picker->selected_loc = loc;
					g_signal_emit (picker, zone_clicked_signal, 0, picker->selected_loc->zone);
				}
				picker->last_mouse_x = event->x;
				picker->last_mouse_y = event->y;
			}
			else {
				int map_x, map_y;
				window_to_map (picker, event->x, event->y, &map_x, &map_y);
				ygtk_time_zone_picker_closeup (picker, TRUE, map_x, map_y, TRUE);
			}
		}
		else if (event->button == 3)
			ygtk_time_zone_picker_closeup (picker, FALSE, 0, 0, TRUE);
		else
			return FALSE;
		gtk_widget_queue_draw (widget);
	}
	return FALSE;
}

static gboolean ygtk_time_zone_picker_button_release_event (GtkWidget *widget,
                                                            GdkEventButton *event)
{
	YGtkTimeZonePicker *picker = YGTK_TIME_ZONE_PICKER (widget);
	picker->last_mouse_x = 0;
	ygtk_time_zone_picker_sync_cursor (picker);
	return FALSE;
}

static void ygtk_time_zone_picker_size_request (GtkWidget *widget,
                                                GtkRequisition *requisition)
{
	requisition->width = 600;
	requisition->height = 300;
	GTK_WIDGET_CLASS (ygtk_time_zone_picker_parent_class)->size_request (widget, requisition);
}

static void ygtk_time_zone_picker_size_allocate (GtkWidget     *widget,
                                                 GtkAllocation *allocation)
{
	if (!GTK_WIDGET_REALIZED (widget))
		return;
	YGtkTimeZonePicker *picker = YGTK_TIME_ZONE_PICKER (widget);
	int win_width = allocation->width, win_height = allocation->height;

	if (picker->closeup)
		picker->scale = 1;
	else {
		picker->scale = MIN ((double) win_width / picker->map_width,
			                 (double) win_height / picker->map_height);
		picker->hover_loc = NULL;
	}

	int map_win_width = picker->map_width * picker->scale;
	int map_win_height = picker->map_height * picker->scale;

	int x = 0, y = 0, w, h;
	x = MAX (0, (win_width - map_win_width) / 2) + allocation->x;
	y = MAX (0, (win_height - map_win_height) / 2) + allocation->y;
	w = MIN (win_width, map_win_width);
	h = MIN (win_height, map_win_height);

	// make sure it clumps to the new window size...
	picker->map_x = MIN (MAX (picker->map_x, (w/2)/picker->scale),
		picker->map_width - (w/2)/picker->scale);
	picker->map_y = MIN (MAX (picker->map_y, (h/2)/picker->scale),
		picker->map_height - (h/2)/picker->scale);

	gdk_window_move_resize (picker->map_window, x, y, w, h);
	GTK_WIDGET_CLASS (ygtk_time_zone_picker_parent_class)->size_allocate
		(widget, allocation);
}

static gboolean ygtk_time_zone_picker_expose_event (GtkWidget *widget,
                                                    GdkEventExpose *event)
{
	YGtkTimeZonePicker *picker = YGTK_TIME_ZONE_PICKER (widget);
	if (event->window != picker->map_window)
		return FALSE;

	cairo_t *cr = gdk_cairo_create (event->window);
	int width, height;
	gdk_window_get_size (event->window, &width, &height);

	if (!picker->map_pixbuf) {
		// show alt text if no image was loaded
		PangoLayout *layout;
		layout = gtk_widget_create_pango_layout (widget,
			"Timezone map could not be found.\nVerify the integrity of the yast2-theme-* package.");
		cairo_move_to (cr, 10, 10);
		pango_cairo_show_layout (cr, layout);
		g_object_unref (layout);
		goto cleanup;
	}

	gdk_cairo_set_source_pixbuf (cr, picker->map_pixbuf, 0, 0);
	cairo_matrix_t matrix;
	cairo_matrix_init_translate (&matrix, picker->map_x - (width/2)/picker->scale,
	                             picker->map_y - (height/2)/picker->scale);
	cairo_matrix_scale (&matrix, 1/picker->scale, 1/picker->scale);
	cairo_pattern_set_matrix (cairo_get_source (cr), &matrix);

	cairo_rectangle (cr, 0, 0, width, height);
	cairo_fill (cr);

	GList *i;
	for (i = picker->locations; i; i = i->next) {
		YGtkTimeZoneLocation *loc = i->data;
		int x, y;
		map_to_window (picker, loc->x, loc->y, &x, &y);
		int radius = (picker->scale == 1) ? 3 : 0;

		if (loc == picker->selected_loc) {
			cairo_set_source_rgb (cr, 232/255.0, 66/255.0, 66/255.0);
			radius = 3;
		}
		else if (loc == picker->hover_loc)
			cairo_set_source_rgb (cr, 255/255.0, 255/255.0, 96/255.0);
		else
			cairo_set_source_rgb (cr, 192/255.0, 112/255.0, 160/255.0);

		if (radius) {
			cairo_arc (cr, x-1, y-1, radius, 0, M_PI*2);
			if (radius > 1) {
				cairo_fill_preserve (cr);
				cairo_set_source_rgb (cr, 90/255.0, 90/255.0, 90/255.0);
				cairo_set_line_width (cr, 1.0);
				cairo_stroke (cr);
			}
			else
				cairo_fill (cr);
		}
	}

	YGtkTimeZoneLocation *label_loc = picker->hover_loc;
	if (!label_loc)
		label_loc = picker->selected_loc;
	if (label_loc) {
		const char *text = label_loc->tooltip;
		if (!text) {
			text = label_loc->country;
			if (!text)
				text = label_loc->zone;
		}

		PangoLayout *layout;
		layout = gtk_widget_create_pango_layout (widget, text);

		int x, y;
		map_to_window (picker, label_loc->x, label_loc->y, &x, &y);
		x += 11; y += 4;
		int fw;
		pango_layout_get_pixel_size (layout, &fw, NULL);
		x = MAX (MIN (x, width - fw - 5), x-11-fw);

		cairo_set_source_rgb (cr, 0, 0, 0);
		cairo_move_to (cr, x, y);
		pango_cairo_show_layout (cr, layout);

		cairo_set_source_rgb (cr, 1, 1, 1);
		cairo_move_to (cr, x-1, y-1);
		pango_cairo_show_layout (cr, layout);
		g_object_unref (G_OBJECT (layout));
		cairo_new_path (cr);
	}

cleanup:
	cairo_destroy (cr);
	gtk_paint_shadow (widget->style, event->window, GTK_STATE_NORMAL,
		GTK_SHADOW_IN, &event->area, widget, "frame", 0, 0, width, height);
	return TRUE;
}

static void ygtk_time_zone_picker_class_init (YGtkTimeZonePickerClass *klass)
{
	ygtk_time_zone_picker_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->realize = ygtk_time_zone_picker_realize;
	widget_class->unrealize = ygtk_time_zone_picker_unrealize;
	widget_class->map = ygtk_time_zone_picker_map;
	widget_class->unmap = ygtk_time_zone_picker_unmap;
	widget_class->expose_event = ygtk_time_zone_picker_expose_event;
	widget_class->size_request = ygtk_time_zone_picker_size_request;
	widget_class->size_allocate = ygtk_time_zone_picker_size_allocate;
	widget_class->button_press_event = ygtk_time_zone_picker_button_press_event;
	widget_class->button_release_event = ygtk_time_zone_picker_button_release_event;
	widget_class->motion_notify_event = ygtk_time_zone_picker_motion_notify_event;
	widget_class->leave_notify_event = ygtk_time_zone_picker_leave_notify_event;

	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
	gtkobject_class->destroy = ygtk_time_zone_picker_destroy;

	zone_clicked_signal = g_signal_new ("zone_clicked",
		G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (YGtkTimeZonePickerClass, zone_clicked), NULL, NULL,
		g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
}

