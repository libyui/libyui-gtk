/* RatioBox container */

#include "ratiobox.h"

static void ratio_box_class_init (RatioBoxClass *klass);
static void ratio_box_init       (RatioBox      *box);
static void ratio_box_add        (GtkContainer   *container,
                                  GtkWidget      *widget);
static void ratio_box_remove     (GtkContainer   *container,
                                  GtkWidget      *widget);
static void ratio_box_forall     (GtkContainer   *container,
                                  gboolean	include_internals,
                                  GtkCallback     callback,
                                  gpointer        callback_data);
static GType ratio_box_child_type (GtkContainer   *container);
static gboolean ratio_box_visibility_notify_event (GtkWidget	     *widget,
                                                GdkEventVisibility  *event);
static void ratio_box_size_request  (GtkWidget      *widget,
                                     GtkRequisition *requisition);
static void ratio_box_size_allocate (GtkWidget      *widget,
                                     GtkAllocation  *allocation);

//static void ratio_box_show_widget (GtkWidget *widget, RatioBox* box);
//static void ratio_box_hide_widget (GtkWidget *widget, RatioBox* box);

static GtkContainerClass *parent_class = NULL;

GType ratio_box_get_type()
{
	static GType box_type = 0;
	if (!box_type) {
		static const GTypeInfo box_info = {
			sizeof (RatioBoxClass),
			NULL, NULL, (GClassInitFunc) ratio_box_class_init, NULL, NULL,
			sizeof (RatioBox), 0, (GInstanceInitFunc) ratio_box_init, NULL
		};

		box_type = g_type_register_static (GTK_TYPE_CONTAINER, "RatioBox",
		                                   &box_info, (GTypeFlags) 0);
	}
	return box_type;
}

static void ratio_box_class_init (RatioBoxClass *klass)
{
	parent_class = (GtkContainerClass*) g_type_class_peek_parent (klass);

	GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);
	container_class->add = ratio_box_add;
	container_class->remove = ratio_box_remove;
  container_class->forall = ratio_box_forall;
  container_class->child_type = ratio_box_child_type;

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->size_request  = ratio_box_size_request;
	widget_class->size_allocate = ratio_box_size_allocate;
}

static void ratio_box_init (RatioBox *box)
{
	GTK_WIDGET_SET_FLAGS (box, GTK_NO_WINDOW);
	gtk_widget_set_redraw_on_allocate (GTK_WIDGET (box), FALSE);

	box->children = NULL;
	box->orientation = box->spacing = 0;
	box->ratios_sum = 0;
}

GtkWidget* ratio_box_new (RatioBoxOrientation orientation, gint spacing)
{
	RatioBox* box = (RatioBox*) g_object_new (TYPE_RATIO_BOX, NULL);
	RATIO_BOX (box)->orientation = orientation;
	RATIO_BOX (box)->spacing = spacing;
	return GTK_WIDGET (box);
}

static GType ratio_box_child_type (GtkContainer* container)
{ return GTK_TYPE_WIDGET; }

void ratio_box_set_child_packing (RatioBox *box, GtkWidget *child,
                                  gfloat ratio, gboolean fill,
                                  guint padding)
{
	RatioBoxChild *child_info = NULL;

	g_return_if_fail (IS_RATIO_BOX (box));
	g_return_if_fail (GTK_IS_WIDGET (child));

	GList* list;
	for (list = box->children; list; list = list->next) {
		child_info = (RatioBoxChild*) list->data;
		if (child_info->widget == child)
			break;
	}

	gtk_widget_freeze_child_notify (child);
	if (list) {
		box->ratios_sum -= child_info->ratio;

		child_info->ratio = ratio;
		child_info->fill = fill;
		child_info->padding = padding;

		box->ratios_sum += child_info->ratio;

		if (GTK_WIDGET_VISIBLE (child) && GTK_WIDGET_VISIBLE (box))
			gtk_widget_queue_resize (child);
	}
	gtk_widget_thaw_child_notify (child);
}

static void ratio_box_add (GtkContainer *container, GtkWidget *child)
{
	RatioBox* box = RATIO_BOX (container);
	RatioBoxChild* child_info;

	g_return_if_fail (IS_RATIO_BOX (box));
	g_return_if_fail (GTK_IS_WIDGET (child));
	g_return_if_fail (child->parent == NULL);

	child_info = g_new (RatioBoxChild, 1);
	child_info->widget = child;
	child_info->ratio = 1.0;
	child_info->padding = 0;
	child_info->fill = TRUE;
/*
	g_signal_connect (G_OBJECT (child), "show",
	                  G_CALLBACK (ratio_box_show_widget), box);
	g_signal_connect (G_OBJECT (child), "hide",
	                  G_CALLBACK (ratio_box_hide_widget), box);
*/
	box->ratios_sum += child_info->ratio;

	box->children = g_list_append (box->children, child_info);

	gtk_widget_freeze_child_notify (child);
	gtk_widget_set_parent (child, GTK_WIDGET (box));
	gtk_widget_thaw_child_notify (child);
}

static void ratio_box_remove (GtkContainer *container, GtkWidget *widget)
{
	RatioBox* box = RATIO_BOX (container);

	GList* child = box->children;
	for (child = box->children; child; child = child->next) {
		RatioBoxChild *box_child = (RatioBoxChild*) child->data;
		if (box_child->widget == widget) {
			box->ratios_sum -= box_child->ratio;

			gboolean was_visible = GTK_WIDGET_VISIBLE (widget);
			gtk_widget_unparent (widget);

			box->children = g_list_remove_link (box->children, child);
			g_list_free (child);
			g_free (box_child);

			if (was_visible)
				gtk_widget_queue_resize (GTK_WIDGET (container));
			break;
		}
	}
}

static void ratio_box_forall (GtkContainer *container,
                              gboolean      include_internals,
                              GtkCallback   callback,
                              gpointer      callback_data)
{
	g_return_if_fail (callback != NULL);

	RatioBox* box = RATIO_BOX (container);

	GList* children = box->children;
	while (children) {
		RatioBoxChild* child = (RatioBoxChild*) children->data;
		children = children->next;
		(* callback) (child->widget, callback_data);
	}
}

#if 0
static void ratio_box_show_widget (GtkWidget *widget, RatioBox* box)
{
	GList* child = box->children;
	for (child = box->children; child; child = child->next) {
		RatioBoxChild *box_child = (RatioBoxChild*) child->data;
		if (box_child->widget == widget)
			box->ratios_sum += box_child->ratio;
	}
}


static void ratio_box_hide_widget (GtkWidget *widget, RatioBox* box)
{
	GList* child = box->children;
	for (child = box->children; child; child = child->next) {
		RatioBoxChild *box_child = (RatioBoxChild*) child->data;
		if (box_child->widget == widget)
			box->ratios_sum -= box_child->ratio;
	}
}
#endif
/*
static gboolean ratio_box_visibility_notify_event (GtkWidget          *widget,
                                                   GdkEventVisibility *event)
{
	RatioBox* box = RATIO_BOX (widget);
printf("** widget set (in)visible\n");
	GList* child = box->children;
	for (child = box->children; child; child = child->next) {
		RatioBoxChild* box_child = (RatioBoxChild*) child->data;
		if (box_child->widget == widget) {
			if (GTK_WIDGET_VISIBLE (widget))
				box->ratios_sum += box_child->ratio;
			else
				box->ratios_sum -= box_child->ratio;
		}
	}
	return FALSE;
}
*/
static void ratio_box_size_request (GtkWidget      *widget,
                                    GtkRequisition *requisition)
{
	RatioBox* box = RATIO_BOX (widget);
	requisition->width = requisition->height =
	     GTK_CONTAINER (widget)->border_width * 2;

	GList* child;
	for (child = g_list_first (box->children); child; child = child->next) {
		RatioBoxChild* box_child = (RatioBoxChild*) child->data;

		if (GTK_WIDGET_VISIBLE (box_child->widget)) {
			GtkRequisition widget_requisition;
			gtk_widget_size_request (box_child->widget, &widget_requisition);

			if (box->orientation == HORIZONTAL_RATIO_BOX_ORIENTATION) {
				requisition->width += widget_requisition.width
				                   + (box_child->padding*2) + (box->spacing*2);
				requisition->height = MAX (requisition->height, widget_requisition.height);
			}
			else { // orientation == VERTICAL_RATIO_BOX_ORIENTATION
				requisition->height += widget_requisition.height
				                    + (box_child->padding*2) + (box->spacing*2);
				requisition->width = MAX (requisition->width, widget_requisition.width);
			}
		}
	}
}

static void ratio_box_size_allocate (GtkWidget     *widget,
                                     GtkAllocation *allocation)
{
// TODO: honor this:
// * text direction - gtk_widget_get_direction (widget);
// * visibility - GTK_WIDGET_VISIBLE (child->widget)
	GList* child;
	int box_length;
	RatioBox* box = RATIO_BOX (widget);

	// Calculate actual size for the exansable widgets
	if (box->orientation == HORIZONTAL_RATIO_BOX_ORIENTATION)
		box_length = allocation->width;
	else  // VERTICAL_RATIO_BOX_ORIENTATION
		box_length = allocation->height;

	box_length -= GTK_CONTAINER (box)->border_width * 2;

	for (child = g_list_first (box->children); child; child = child->next) {
		RatioBoxChild* box_child = (RatioBoxChild*) child->data;
		if (box_child->ratio == 0) {
			GtkRequisition requisition;
			gtk_widget_size_request (box_child->widget, &requisition);

			if (box->orientation == HORIZONTAL_RATIO_BOX_ORIENTATION)
				box_length -= requisition.width;
			else  // VERTICAL_RATIO_BOX_ORIENTATION
				box_length -= requisition.height;
		}
		box_length -= box_child->padding*2 + box->spacing*2;
	}

	gint child_pos = 0;
	for (child = g_list_first (box->children); child; child = child->next) {
		GtkAllocation child_allocation;
		RatioBoxChild* box_child = (RatioBoxChild*) child->data;

		if (!GTK_WIDGET_VISIBLE (box_child->widget))
			continue;

		GtkRequisition requisition;
		gtk_widget_size_request (box_child->widget, &requisition);

		// ratio 0 == non-expansible
		if (box_child->ratio == 0) {
			if (box->orientation == HORIZONTAL_RATIO_BOX_ORIENTATION) {
				child_allocation.x = allocation->x + child_pos;
				child_allocation.y = allocation->y;
				child_allocation.width  = requisition.width;
				child_allocation.height = allocation->height;
			}
			else {  // VERTICAL_RATIO_BOX_ORIENTATION
				child_allocation.x = allocation->x;
				child_allocation.y = allocation->y + child_pos;
				child_allocation.width  = allocation->width;
				child_allocation.height = requisition.height;
			}
		}
		// expansible
		else {
			if (box->orientation == HORIZONTAL_RATIO_BOX_ORIENTATION) {
				child_allocation.x = allocation->x + child_pos;
				child_allocation.y = allocation->y;
				child_allocation.width =
				  (gint)((box_child->ratio / box->ratios_sum) * box_length);
				child_allocation.height = allocation->height;
			}
			else {  // VERTICAL_RATIO_BOX_ORIENTATION
				child_allocation.x = allocation->x;
				child_allocation.y = allocation->y + child_pos;
				child_allocation.width = allocation->width;
				child_allocation.height =
				  (gint)((box_child->ratio / box->ratios_sum) * box_length);
			}
		}

		if (box->orientation == HORIZONTAL_RATIO_BOX_ORIENTATION) {
			child_allocation.x += box->spacing + box_child->padding;
			child_pos += child_allocation.width + box->spacing * 2 + box_child->padding * 2;
		}
		else {  // VERTICAL_RATIO_BOX_ORIENTATION
			child_allocation.y += box->spacing + box_child->padding;
			child_pos += child_allocation.height + box->spacing * 2 + box_child->padding * 2;
		}

		if (!box_child->fill) {
			child_allocation.width  = MIN (child_allocation.width, requisition.width);
			child_allocation.height = MIN (child_allocation.height, requisition.height);
		}

		gtk_widget_size_allocate (box_child->widget, &child_allocation);
	}
}
