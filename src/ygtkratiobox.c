/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkRatioBox container */
// check the header file for information about this container

#include <config.h>
#include <math.h>
#include "ygtkratiobox.h"

G_DEFINE_ABSTRACT_TYPE (YGtkRatioBox, ygtk_ratio_box, GTK_TYPE_CONTAINER)

static void ygtk_ratio_box_init (YGtkRatioBox *box)
{
	GTK_WIDGET_SET_FLAGS (box, GTK_NO_WINDOW);
	gtk_widget_set_redraw_on_allocate (GTK_WIDGET (box), FALSE);
}

static GType ygtk_ratio_box_child_type (GtkContainer* container)
{ return GTK_TYPE_WIDGET; }

void ygtk_ratio_box_pack (YGtkRatioBox *box, GtkWidget *child, gfloat ratio)
{
	YGtkRatioBoxChild* child_info;
	child_info = g_new (YGtkRatioBoxChild, 1);
	child_info->widget = child;
	child_info->ratio = ratio;

	box->children = g_list_append (box->children, child_info);

	gtk_widget_freeze_child_notify (child);
	gtk_widget_set_parent (child, GTK_WIDGET (box));
	gtk_widget_thaw_child_notify (child);
}

static YGtkRatioBoxChild *ygtk_ratio_get_child_info (YGtkRatioBox *box, GtkWidget *child)
{
	YGtkRatioBoxChild *i = NULL;
	GList *list;
	for (list = box->children; list; list = list->next) {
		i = (YGtkRatioBoxChild*) list->data;
		if (i->widget == child)
			break;
	}
	if (!list)
		return NULL;
	return i;
}

static void ygtk_ratio_box_add (GtkContainer *container, GtkWidget *child)
{
	ygtk_ratio_box_pack (YGTK_RATIO_BOX (container), child, 1.0);
}

static void ygtk_ratio_box_remove (GtkContainer *container, GtkWidget *widget)
{
	YGtkRatioBox* box = YGTK_RATIO_BOX (container);

	GList* child = box->children;
	for (child = box->children; child; child = child->next) {
		YGtkRatioBoxChild *box_child = (YGtkRatioBoxChild*) child->data;
		if (box_child->widget == widget) {
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

static void ygtk_ratio_box_forall (GtkContainer *container, gboolean include_internals,
                                   GtkCallback callback, gpointer callback_data)
{
	g_return_if_fail (callback != NULL);

	YGtkRatioBox* box = YGTK_RATIO_BOX (container);

	GList* children = box->children;
	while (children) {
		YGtkRatioBoxChild* child = (YGtkRatioBoxChild*) children->data;
		children = children->next;
		(* callback) (child->widget, callback_data);
	}
}

/* We put size_request and _allocate in the same functions for both
   orientations because it's just easier to maintain having the
   logic in the same place. */

static void ygtk_ratio_box_size_request (GtkWidget      *widget,
                                         GtkRequisition *requisition,
                                         GtkOrientation  orientation)
{
	requisition->width = requisition->height = 0;

	YGtkRatioBox* box = YGTK_RATIO_BOX (widget);
	gint children_nb = 0;
	GList *i;
	for (i = box->children; i; i = i->next) {
		YGtkRatioBoxChild* child = i->data;
		if (!GTK_WIDGET_VISIBLE (child->widget))
			continue;

		GtkRequisition child_req;
		gtk_widget_size_request (child->widget, &child_req);
		if (orientation == GTK_ORIENTATION_HORIZONTAL)
			requisition->height = MAX (requisition->height, child_req.height);
		else
			requisition->width = MAX (requisition->width, child_req.width);
		children_nb++;
	}
	gint spacing = children_nb ? box->spacing*(children_nb-1) : 0;
	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		requisition->width += spacing;
	else
		requisition->height += spacing;
}

static void ygtk_ratio_box_size_allocate (GtkWidget     *widget,
                                          GtkAllocation *allocation,
                                          GtkOrientation orientation)
{
	YGtkRatioBox* box = YGTK_RATIO_BOX (widget);

	gfloat ratios_sum = 0;
	gint children_nb = 0;

	GList* i;
	for (i = box->children; i; i = i->next) {
		YGtkRatioBoxChild* child = i->data;
		if (!GTK_WIDGET_VISIBLE (child->widget))
			continue;

		ratios_sum += child->ratio;
		children_nb++;
	}

	gint spacing = children_nb ? box->spacing*(children_nb-1) : 0;

	gint length;
	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		length = allocation->width - spacing;
	else
		length = allocation->height - spacing;
	gint child_pos = 0;

	for (i = box->children; i; i = i->next) {
		YGtkRatioBoxChild* child = i->data;
		if (!GTK_WIDGET_VISIBLE (child->widget))
			continue;

		GtkRequisition child_req;
		gtk_widget_get_child_requisition (child->widget, &child_req);

		gint child_length = (child->ratio * length) / ratios_sum;

		GtkAllocation child_alloc;
		if (orientation == GTK_ORIENTATION_HORIZONTAL) {
			child_alloc.x = child_pos;
			child_alloc.y = allocation->y;
			child_alloc.width = child_length;
			child_alloc.height = allocation->height;
		}
		else { // GTK_ORIENTATION_VERTICAL
			child_alloc.x = allocation->x;
			child_alloc.y = child_pos;
			child_alloc.width = allocation->width;
			child_alloc.height = child_length;
		}

		gtk_widget_size_allocate (child->widget, &child_alloc);
		child_pos += child_length + box->spacing;
	}
}

void ygtk_ratio_box_set_child_packing (YGtkRatioBox *box, GtkWidget *child, gfloat ratio)
{
	YGtkRatioBoxChild *child_info;
	child_info = ygtk_ratio_get_child_info (box, child);
	if (child_info) {
		gtk_widget_freeze_child_notify (child);
		child_info->ratio = ratio;
		if (GTK_WIDGET_VISIBLE (child) && GTK_WIDGET_VISIBLE (box))
			gtk_widget_queue_resize (child);

		gtk_widget_thaw_child_notify (child);
	}
}

void ygtk_ratio_box_set_spacing (YGtkRatioBox *box, guint spacing)
{
	box->spacing = spacing;
}

static void ygtk_ratio_box_class_init (YGtkRatioBoxClass *klass)
{
	ygtk_ratio_box_parent_class = (GtkContainerClass*) g_type_class_peek_parent (klass);

	GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);
	container_class->add = ygtk_ratio_box_add;
	container_class->remove = ygtk_ratio_box_remove;
	container_class->forall = ygtk_ratio_box_forall;
	container_class->child_type = ygtk_ratio_box_child_type;
}

//** RatioHBox

G_DEFINE_TYPE (YGtkRatioHBox, ygtk_ratio_hbox, YGTK_TYPE_RATIO_BOX)

static void ygtk_ratio_hbox_init (YGtkRatioHBox *box)
{
}

static void ygtk_ratio_hbox_size_request (GtkWidget      *widget,
                                         GtkRequisition *requisition)
{
	ygtk_ratio_box_size_request (widget, requisition, GTK_ORIENTATION_HORIZONTAL);
}

static void ygtk_ratio_hbox_size_allocate (GtkWidget     *widget,
                                          GtkAllocation *allocation)
{
	ygtk_ratio_box_size_allocate (widget, allocation, GTK_ORIENTATION_HORIZONTAL);
}

GtkWidget* ygtk_ratio_hbox_new (gint spacing)
{
	YGtkRatioBox* box = (YGtkRatioBox*) g_object_new (YGTK_TYPE_RATIO_HBOX, NULL);
	box->spacing = spacing;
	return GTK_WIDGET (box);
}

static void ygtk_ratio_hbox_class_init (YGtkRatioHBoxClass *klass)
{
	ygtk_ratio_hbox_parent_class = (YGtkRatioBoxClass*) g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->size_request  = ygtk_ratio_hbox_size_request;
	widget_class->size_allocate = ygtk_ratio_hbox_size_allocate;
}

//** RatioVBox

G_DEFINE_TYPE (YGtkRatioVBox, ygtk_ratio_vbox, YGTK_TYPE_RATIO_BOX)

static void ygtk_ratio_vbox_init (YGtkRatioVBox *box)
{
}

static void ygtk_ratio_vbox_size_request (GtkWidget      *widget,
                                         GtkRequisition *requisition)
{
	ygtk_ratio_box_size_request (widget, requisition, GTK_ORIENTATION_VERTICAL);
}

static void ygtk_ratio_vbox_size_allocate (GtkWidget     *widget,
                                          GtkAllocation *allocation)
{
	ygtk_ratio_box_size_allocate (widget, allocation, GTK_ORIENTATION_VERTICAL);
}

GtkWidget* ygtk_ratio_vbox_new (gint spacing)
{
	YGtkRatioBox* box = (YGtkRatioBox*) g_object_new (YGTK_TYPE_RATIO_VBOX, NULL);
	box->spacing = spacing;
	return GTK_WIDGET (box);
}

static void ygtk_ratio_vbox_class_init (YGtkRatioVBoxClass *klass)
{
	ygtk_ratio_vbox_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->size_request  = ygtk_ratio_vbox_size_request;
	widget_class->size_allocate = ygtk_ratio_vbox_size_allocate;
}

//** YGtkAdjSize

G_DEFINE_TYPE (YGtkAdjSize, ygtk_adj_size, GTK_TYPE_BIN)

static void ygtk_adj_size_init (YGtkAdjSize *adj_size)
{
	GTK_WIDGET_SET_FLAGS (adj_size, GTK_NO_WINDOW);
	gtk_widget_set_redraw_on_allocate (GTK_WIDGET (adj_size), FALSE);
}

static void ygtk_adj_size_size_request (GtkWidget *widget,
                                        GtkRequisition *requisition)
{
	GtkWidget *child = GTK_BIN (widget)->child;
	requisition->width = requisition->height = 0;
	if (child && GTK_WIDGET_VISIBLE (child)) {
		gtk_widget_size_request (child, requisition);
		guint border = GTK_CONTAINER (widget)->border_width;
		requisition->width += border * 2;
		requisition->height += border * 2;

		YGtkAdjSize *adj_size = YGTK_ADJ_SIZE (widget);
		requisition->width = MAX (requisition->width, adj_size->min_width);
		requisition->height = MAX (requisition->height, adj_size->min_height);

		if (adj_size->max_width)
			requisition->width = MIN (requisition->width, adj_size->max_width);
		if (adj_size->max_height)
			requisition->height = MIN (requisition->height, adj_size->max_height);

		if (adj_size->only_expand) {
			adj_size->min_width = requisition->width;
			adj_size->min_height = requisition->height;
		}
	}
}

static void ygtk_adj_size_size_allocate (GtkWidget *widget,
                                         GtkAllocation *allocation)
{
	GtkWidget *child = GTK_BIN (widget)->child;
	if (child && GTK_WIDGET_VISIBLE (child)) {
		GtkAllocation child_alloc = *allocation;
		guint border = GTK_CONTAINER (widget)->border_width;
		child_alloc.x += border;
		child_alloc.y += border;
		child_alloc.width -= border * 2;
		child_alloc.height -= border * 2;
		gtk_widget_size_allocate (child, &child_alloc);
	}
	GTK_WIDGET_CLASS (ygtk_adj_size_parent_class)->size_allocate (widget, allocation);
}

GtkWidget* ygtk_adj_size_new (void)
{
	return GTK_WIDGET (g_object_new (YGTK_TYPE_ADJ_SIZE, NULL));
}

void ygtk_adj_size_set_min (YGtkAdjSize *adj_size, guint min_width, guint min_height)
{
	adj_size->min_width = min_width;
	adj_size->min_height = min_height;
}

void ygtk_adj_size_set_max (YGtkAdjSize *adj_size, guint max_width, guint max_height)
{
	adj_size->max_width = max_width;
	adj_size->max_height = max_height;
}

void ygtk_adj_size_set_only_expand (YGtkAdjSize *adj_size, gboolean only_expand)
{
	adj_size->only_expand = only_expand;
}

static void ygtk_adj_size_class_init (YGtkAdjSizeClass *klass)
{
	ygtk_adj_size_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->size_request  = ygtk_adj_size_size_request;
	widget_class->size_allocate  = ygtk_adj_size_size_allocate;
}

//** YGtkTunedScrolledWindow

G_DEFINE_TYPE (YGtkTunedScrolledWindow, ygtk_tuned_scrolled_window, GTK_TYPE_SCROLLED_WINDOW)

static void ygtk_tuned_scrolled_window_init (YGtkTunedScrolledWindow *scroll)
{
}

static void ygtk_tuned_scrolled_window_size_request (GtkWidget *widget,
                                               GtkRequisition *requisition)
{
	YGtkTunedScrolledWindow *scroll = YGTK_TUNED_SCROLLED_WINDOW (widget);

	GtkRequisition child_req;
	GtkWidget *child = GTK_BIN (widget)->child;
	gtk_widget_size_request (child, &child_req);

	GtkScrolledWindow *gscroll = GTK_SCROLLED_WINDOW (widget);
	GtkPolicyType hpolicy, vpolicy;
	gtk_scrolled_window_get_policy (gscroll, &hpolicy, &vpolicy);

	if (scroll->max_width) {
		if (child_req.width < scroll->max_width)
			gscroll->hscrollbar_policy = GTK_POLICY_NEVER;
		else
			gscroll->hscrollbar_policy = GTK_POLICY_AUTOMATIC;
	}
	if (scroll->max_height) {
		if (child_req.height < scroll->max_height)
			gscroll->vscrollbar_policy = GTK_POLICY_NEVER;
		else
			gscroll->vscrollbar_policy = GTK_POLICY_AUTOMATIC;
	}

	GTK_WIDGET_CLASS (ygtk_tuned_scrolled_window_parent_class)->size_request (widget, requisition);
	if (scroll->max_width)
		requisition->width = MIN (requisition->width, scroll->max_width);
	if (scroll->max_height)
		requisition->height = MIN (requisition->height, scroll->max_height);
}

GtkWidget* ygtk_tuned_scrolled_window_new (GtkWidget *child)
{
	YGtkTunedScrolledWindow *scroll = g_object_new (YGTK_TYPE_TUNED_SCROLLED_WINDOW, NULL);
	if (child)
		gtk_container_add (GTK_CONTAINER (scroll), child);
	return GTK_WIDGET (scroll);
}

void ygtk_tuned_scrolled_window_set_auto_policy (YGtkTunedScrolledWindow *scroll,
                                           guint max_width, guint max_height)
{
	scroll->max_width = max_width;
	scroll->max_height = max_height;
}

static void ygtk_tuned_scrolled_window_class_init (YGtkTunedScrolledWindowClass *klass)
{
	ygtk_tuned_scrolled_window_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->size_request  = ygtk_tuned_scrolled_window_size_request;
}

