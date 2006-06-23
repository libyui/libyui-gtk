/* Yast GTK */
#include <config.h>
#include <ycp/y2log.h>
#include <YGUI.h>
#include "YGUtils.h"
#include "YGWidget.h"
#include "YBarGraph.h"

class YGBarGraph : public YBarGraph, public YGWidget
{
public:
	YGBarGraph (const YWidgetOpt &opt, YGWidget *parent)
	: YBarGraph (opt)
	, YGWidget (this, parent, true, GTK_TYPE_HBOX, NULL)
	{
	
		g_signal_connect (G_OBJECT (getWidget()), "size-request",
		                  G_CALLBACK (calculate_size_cb), this);


	}

	int m_valuesSum;

	// YBarGraph
	virtual void doUpdate()
	{
		// Remove possible existing labels
		GList* children = gtk_container_get_children (GTK_CONTAINER (getWidget()));
		for (guint i = 0; i < g_list_length (children); i++) {
			GtkWidget *label = (GtkWidget*) g_list_nth_data (children, i);
			gtk_container_remove (GTK_CONTAINER (getWidget()), label);  // un-refs widget
		}
		g_list_free (children);

		// Sum of the values for ratio calculation purposes
		for (int i = 0; i < segments(); i++)
			m_valuesSum += value(i);

		// Add new labels
		for (int i = 0; i < segments(); i++) {
			gchar *str = g_strdup_printf ("%d", value(i));
			GtkWidget *label = gtk_label_new (str);
			g_free (str);

			GtkWidget *box = gtk_event_box_new();
			GdkColor color = { 0, 255<<8, (i*100)<<8, (i*100)<<8 };  // TODO: temporary
			gtk_widget_modify_bg (box, GTK_STATE_NORMAL, &color);
			gtk_container_add (GTK_CONTAINER (box), label);

//			g_signal_connect (G_OBJECT (box), "size-request",
//			                  G_CALLBACK (calculate_size_cb), this);

/*
// using a frame
			GdkColor color = { 0, 0xf500, 0xffff, 0x8000 };  // yellow tone (temporary)
			GtkWidget *frame = gtk_aspect_frame_new (NULL, 0.5, 0.5,
			                           value(i) / values_sum, FALSE);
			gtk_widget_modify_bg (frame, GTK_STATE_NORMAL, &color);
			gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
			gtk_container_add (GTK_CONTAINER (frame), label);
*/

			gtk_container_add (GTK_CONTAINER (getWidget()), box);
		}

//		gtk_container_resize_children (GTK_CONTAINER (getWidget());  // needed?

		gtk_widget_show_all (getWidget());
	}


// for the container
	static void calculate_size_cb (GtkWidget *container, GtkRequisition *requisition,
	                               YGBarGraph *pThis)
	{
		int width = container->allocation.width;

		GList* children = gtk_container_get_children (GTK_CONTAINER (pThis->getWidget()));
		for (guint i = 0; i < g_list_length (children); i++) {
			GtkWidget *box = (GtkWidget*) g_list_nth_data (children, i);

			gdouble ratio = (gdouble) pThis->value(i) / pThis->m_valuesSum;
			GtkRequisition req = { (gint)(ratio * width), -1 };
printf("req.width: %d\n", req.width);
			gtk_widget_size_request (box, &req);
printf("YGBarGraph: setting width of %d to %d\n", requisition->width, pThis->value(i));
		}
		g_list_free (children);
	}

#if 0
// per each label
	static void calculate_size_cb (GtkWidget *widget, GtkRequisition *requisition,
	                               YGBarGraph *pThis)
	{
		// This is quite ugly, but the other way I can think of getting the value
		// involves searching for the index on the container...
		GtkWidget *label = gtk_bin_get_child (GTK_BIN (widget));
		int value = atoi (gtk_label_get_text (GTK_LABEL (label)));

		gdouble ratio = (gdouble) value / pThis->m_valuesSum;

		GtkWidget *container = pThis->getWidget();
		requisition->width = (gint) (container->allocation.width * ratio);
printf("YGBarGraph: requested width by %d is %d\n", value, requisition->width);
	}
#endif

	// YWidget
	YGWIDGET_IMPL_SET_SIZE
	YGWIDGET_IMPL_SET_ENABLING
	YGWIDGET_IMPL_NICESIZE
};

YWidget *
YGUI::createBarGraph (YWidget *parent, YWidgetOpt &opt)
{
	return new YGBarGraph (opt, YGWidget::get (parent));
}
