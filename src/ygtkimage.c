/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkImage widget */
// check the header file for information about this widget

#include <config.h>
#include "ygdkmngloader.h"
#include "ygtkimage.h"
#include <gtk/gtkiconfactory.h>

G_DEFINE_TYPE (YGtkImage, ygtk_image, GTK_TYPE_DRAWING_AREA)

static void ygtk_image_init (YGtkImage *image)
{
}

static void ygtk_image_free_pixbuf (YGtkImage *image)
{
	if (image->animated) {
		if (image->animation) {
			g_object_unref (G_OBJECT (image->animation->pixbuf));
			if (image->animation->timeout_id)
				g_source_remove (image->animation->timeout_id);
			g_free (image->animation);
			image->animation = NULL;
		}
	}
	else {
		if (image->pixbuf) {
			g_object_unref (G_OBJECT (image->pixbuf));
			image->pixbuf = NULL;
		}
	}
}

static void ygtk_image_destroy (GtkObject *object)
{
	YGtkImage *image = YGTK_IMAGE (object);
	if (image->alt_text)
		g_free (image->alt_text);
	image->alt_text = NULL;
	ygtk_image_free_pixbuf (image);
	GTK_OBJECT_CLASS (ygtk_image_parent_class)->destroy (object);
}

static void ygtk_image_set_pixbuf (YGtkImage *image, GdkPixbuf *pixbuf, const char *error_msg)
{
	ygtk_image_free_pixbuf (image);
	gtk_widget_queue_resize (GTK_WIDGET (image));

	if (pixbuf) {
		image->animated = FALSE;
		image->pixbuf = pixbuf;
		image->loaded = TRUE;
	}
/*	else
		g_warning ("Couldn't load image - %s", error_msg);*/
}

static gboolean ygtk_image_advance_frame_cb (gpointer data)
{
	YGtkImage *image = (YGtkImage *) data;
	struct _YGtkImageAnimation *animation = image->animation;

	if (!animation->frame)  // no frame yet loaded
		animation->frame = gdk_pixbuf_animation_get_iter (animation->pixbuf, NULL);
	else
		if (gdk_pixbuf_animation_iter_advance (animation->frame, NULL))
			gtk_widget_queue_draw (GTK_WIDGET (image));

	// shedule next frame
	int delay = gdk_pixbuf_animation_iter_get_delay_time (animation->frame);
	if (delay != -1)
		animation->timeout_id = g_timeout_add (delay, ygtk_image_advance_frame_cb, data);
	return FALSE;
}

static void ygtk_image_set_animation (YGtkImage *image, GdkPixbufAnimation *pixbuf,
                                      const char *error_msg)
{
	ygtk_image_free_pixbuf (image);
	gtk_widget_queue_resize (GTK_WIDGET (image));

	if (pixbuf) {
		image->animated = TRUE;
		image->animation = g_new0 (struct _YGtkImageAnimation, 1);
		image->animation->pixbuf = pixbuf;
		image->loaded = TRUE;
		ygtk_image_advance_frame_cb (image);
	}
	else if (error_msg)
		g_warning ("Couldn't load image - %s", error_msg);
}

void ygtk_image_set_from_file (YGtkImage *image, const char *filename, gboolean anim)
{
	GError *error = 0;
	if (anim) {
		GdkPixbufAnimation *pixbuf;
		if (ygdk_mng_pixbuf_is_file_mng (filename))
			pixbuf = ygdk_mng_pixbuf_new_from_file (filename, &error);
		else
			pixbuf = gdk_pixbuf_animation_new_from_file (filename, &error);
		ygtk_image_set_animation (image, pixbuf, error ? error->message : "(undefined)");
	}
	else {
		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (filename, &error);
		ygtk_image_set_pixbuf (image, pixbuf, error ? error->message : "(undefined)");
	}
}

void ygtk_image_set_from_pixbuf (YGtkImage *image, GdkPixbuf *pixbuf)
{
	ygtk_image_set_pixbuf (image, pixbuf, NULL);
}

static void ygtk_image_loaded_cb (GdkPixbufLoader *loader, YGtkImage *image)
{
	if (image->animated) {
		if (image->animation) {
			// a new frame loaded -- just redraw the widget
			if (gdk_pixbuf_animation_iter_on_currently_loading_frame
			       (image->animation->frame))
				gtk_widget_queue_draw (GTK_WIDGET (image));
		}
		else {
			GdkPixbufAnimation *pixbuf = gdk_pixbuf_loader_get_animation (loader);
			g_object_ref (G_OBJECT (pixbuf));
			ygtk_image_set_animation (image, pixbuf, "on block data reading callback");
		}
	}
	else {
		GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
		g_object_ref (G_OBJECT (pixbuf));
		ygtk_image_set_pixbuf (image, pixbuf, "on block data reading callback");
	}
}

void ygtk_image_set_from_data (YGtkImage *image, const guint8 *data, long data_size, gboolean anim)
{
	GError *error = 0;
	if (anim && ygdk_mng_pixbuf_is_data_mng (data, data_size)) {
		GdkPixbufAnimation *pixbuf;
		pixbuf = ygdk_mng_pixbuf_new_from_data (data, data_size, &error);	
		ygtk_image_set_animation (image, pixbuf, error ? error->message : "(undefined)");		
	}
	else {
		image->animated = anim;
		GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
		g_signal_connect (G_OBJECT (loader), "area-prepared",
		                  G_CALLBACK (ygtk_image_loaded_cb), image);
		if (!gdk_pixbuf_loader_write (loader, data, data_size, &error))
			g_warning ("Could not load image from data blocks: %s", error->message);
		gdk_pixbuf_loader_close (loader, &error);
	}
}

void ygtk_image_set_props (YGtkImage *image, YGtkImageAlign align, const gchar *alt_text)
{
	image->align = align;
	if (image->alt_text)
		g_free (image->alt_text);
	if (alt_text)
		image->alt_text = g_strdup (alt_text);
	gtk_widget_queue_draw (GTK_WIDGET (image));
}

static void ygtk_image_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	YGtkImage *image = YGTK_IMAGE (widget);
	int width = 0, height = 0;
	if (image->loaded) {
		if (image->animated) {
			width = gdk_pixbuf_animation_get_width (image->animation->pixbuf);
			height = gdk_pixbuf_animation_get_height (image->animation->pixbuf);
		}
		else {
			width = gdk_pixbuf_get_width (image->pixbuf);
			height = gdk_pixbuf_get_height (image->pixbuf);
		}
	}
	else if (image->alt_text) {
		PangoLayout *layout;
		layout = gtk_widget_create_pango_layout (widget, image->alt_text);
		pango_layout_get_pixel_size (layout, &width, &height);
	}
	requisition->width = width;
	requisition->height = height;
}

static GdkPixbuf *ygtk_image_render_state (GtkWidget *widget, GdkPixbuf *pixbuf)
{
	// as in GtkImage
	GtkIconSource *source = gtk_icon_source_new();
	GdkPixbuf *rendered;
	gtk_icon_source_set_pixbuf (source, pixbuf);
	gtk_icon_source_set_size (source, GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_icon_source_set_size_wildcarded (source, FALSE);
	rendered = gtk_style_render_icon (widget->style, source,
		gtk_widget_get_direction (widget), GTK_WIDGET_STATE (widget),
		/* arbitrary */ (GtkIconSize)-1, widget, "gtk-image");
	gtk_icon_source_free (source);
	return rendered;
}

static gboolean ygtk_image_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	YGtkImage *image = YGTK_IMAGE (widget);
	int width, height;
	width  = widget->allocation.width;
	height = widget->allocation.height;

	cairo_t *cr = gdk_cairo_create (widget->window);

	if (!image->loaded) {
		if (image->alt_text) {
			// show alt text if no image was loaded
			PangoLayout *layout;
			layout = gtk_widget_create_pango_layout (widget, image->alt_text);

			int x, y;
			x = (width - widget->requisition.width) / 2;
			y = (height - widget->requisition.height) / 2;

			cairo_move_to (cr, x, y);
			pango_cairo_show_layout (cr, layout);

			g_object_unref (layout);
		}
		cairo_destroy (cr);
		return FALSE;
	}

	GdkPixbuf *pixbuf;
	if (image->animated)
		pixbuf = gdk_pixbuf_animation_iter_get_pixbuf (image->animation->frame);
	else
		pixbuf = image->pixbuf;

	gboolean needs_transform = GTK_WIDGET_STATE (widget) != GTK_STATE_NORMAL;
	if (needs_transform)
		pixbuf = ygtk_image_render_state (widget, pixbuf);
	int x = 0, y = 0;
	if (image->align == CENTER_IMAGE_ALIGN) {
		x = (width - widget->requisition.width) / 2;
		y = (height - widget->requisition.height) / 2;
	}

	gdk_cairo_set_source_pixbuf (cr, pixbuf, x, y);

	switch (image->align) {
		case CENTER_IMAGE_ALIGN:
			break;
		case SCALE_IMAGE_ALIGN:
		{
			double scale_x = (double) gdk_pixbuf_get_width (pixbuf) / width;
			double scale_y = (double) gdk_pixbuf_get_height (pixbuf) / height;
			cairo_matrix_t matrix;
			cairo_matrix_init_scale (&matrix, scale_x, scale_y);
			cairo_pattern_set_matrix (cairo_get_source (cr), &matrix);
			break;
		}
		case TILE_IMAGE_ALIGN:
			cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
			break;
	}

	cairo_rectangle (cr, x, y, width, height);
	cairo_fill (cr);

	cairo_destroy (cr);
	if (needs_transform)
		g_object_unref (G_OBJECT (pixbuf));
	return FALSE;
}

GtkWidget* ygtk_image_new (void)
{
	return g_object_new (YGTK_TYPE_IMAGE, NULL);
}

static void ygtk_image_class_init (YGtkImageClass *klass)
{
	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->expose_event = ygtk_image_expose_event;
	widget_class->size_request = ygtk_image_size_request;

	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
	gtkobject_class->destroy = ygtk_image_destroy;
}

