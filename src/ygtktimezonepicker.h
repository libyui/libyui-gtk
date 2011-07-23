/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkTimeZonePicker provides an input for selecting a time zone
   by clicking on a world map.
   Based on yast-qt version.
*/

#ifndef YGTK_TIME_ZONE_PICKER_H
#define YGTK_TIME_ZONE_PICKER_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>
G_BEGIN_DECLS

#define YGTK_TYPE_TIME_ZONE_PICKER            (ygtk_time_zone_picker_get_type ())
#define YGTK_TIME_ZONE_PICKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	YGTK_TYPE_TIME_ZONE_PICKER, YGtkTimeZonePicker))
#define YGTK_TIME_ZONE_PICKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
	YGTK_TYPE_TIME_ZONE_PICKER, YGtkTimeZonePickerClass))
#define YGTK_IS_TIME_ZONE_PICKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                               YGTK_TYPE_TIME_ZONE_PICKER))
#define YGTK_IS_TIME_ZONE_PICKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                    YGTK_TYPE_TIME_ZONE_PICKER))
#define YGTK_TIME_ZONE_PICKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
	YGTK_TYPE_TIME_ZONE_PICKER, YGtkTimeZonePickerClass))

typedef struct _YGtkTimeZonePicker YGtkTimeZonePicker;
typedef struct _YGtkTimeZonePickerClass YGtkTimeZonePickerClass;
typedef struct _YGtkTimeZoneLocation YGtkTimeZoneLocation;

// converts time zone code to human-readable name
typedef const gchar *(*TimeZoneToName) (const gchar *code, gpointer data);

struct _YGtkTimeZonePicker
{
	GtkWidget parent;

	// private:
	GdkPixbuf *map_pixbuf;
	gint map_width, map_height;
	gint map_x, map_y;  // the center of the focus in pixbuf-metrics
	GdkWindow *map_window;
	gdouble scale;  // map-to-window scale
	guint closeup : 2;

	GList *locations;
	YGtkTimeZoneLocation *selected_loc, *hover_loc;

	gint last_mouse_x, last_mouse_y;
};

struct _YGtkTimeZonePickerClass
{
	GtkWidgetClass parent_class;

	// signals:
	void (*zone_clicked) (YGtkTimeZonePicker *picker, const gchar *zone);
};

struct _YGtkTimeZoneLocation
{
	gchar *country, *zone, *comment, *tooltip;
	gdouble latitude, longitude;
	gint x, y;
};

GType ygtk_time_zone_picker_get_type (void) G_GNUC_CONST;

void ygtk_time_zone_picker_set_map (YGtkTimeZonePicker *picker, const char *filename,
	TimeZoneToName converter, gpointer converter_user_data);

const gchar *ygtk_time_zone_picker_get_current_zone (YGtkTimeZonePicker *picker);
void ygtk_time_zone_picker_set_current_zone (YGtkTimeZonePicker *picker, const gchar *zone,
                                             gboolean zoom);

G_END_DECLS
#endif /* YGTK_TIME_ZONE_PICKER_H */

