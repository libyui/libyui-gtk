//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

/* YGtkRatioBox widget */
// check the header file for information about this widget

#include "ygtkratiobox.h"

static void ygtk_ratio_box_class_init (YGtkRatioBoxClass *klass);
static void ygtk_ratio_box_init       (YGtkRatioBox      *box);
static void ygtk_ratio_box_add        (GtkContainer   *container,
                                       GtkWidget      *widget);
static void ygtk_ratio_box_remove     (GtkContainer   *container,
                                       GtkWidget      *widget);
static void ygtk_ratio_box_forall     (GtkContainer   *container,
                                       gboolean        include_internals,
                                       GtkCallback     callback,
                                       gpointer        callback_data);
static GType ygtk_ratio_box_child_type (GtkContainer  *container);

G_DEFINE_ABSTRACT_TYPE (YGtkRatioBox, ygtk_ratio_box, GTK_TYPE_CONTAINER)

static void ygtk_ratio_box_class_init (YGtkRatioBoxClass *klass)
{
	ygtk_ratio_box_parent_class = (GtkContainerClass*) g_type_class_peek_parent (klass);

	GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);
	container_class->add = ygtk_ratio_box_add;
	container_class->remove = ygtk_ratio_box_remove;
	container_class->forall = ygtk_ratio_box_forall;
	container_class->child_type = ygtk_ratio_box_child_type;
}

static void ygtk_ratio_box_init (YGtkRatioBox *box)
{
	GTK_WIDGET_SET_FLAGS (box, GTK_NO_WINDOW);
	gtk_widget_set_redraw_on_allocate (GTK_WIDGET (box), FALSE);
}

static GType ygtk_ratio_box_child_type (GtkContainer* container)
{ return GTK_TYPE_WIDGET; }

static void ygtk_ratio_box_pack (YGtkRatioBox *box, GtkWidget *child,
                                 gfloat ratio, gboolean xfill, gboolean yfill,
                                 guint padding, GtkPackType pack)
{
	YGtkRatioBoxChild* child_info;
	child_info = g_new (YGtkRatioBoxChild, 1);
	child_info->widget = child;
	child_info->ratio = ratio;
	child_info->padding = padding;
	child_info->xfill = xfill;
	child_info->yfill = yfill;
	child_info->pack = pack;
	child_info->fully_expandable = 0;

	box->children = g_list_append (box->children, child_info);

	gtk_widget_freeze_child_notify (child);
	gtk_widget_set_parent (child, GTK_WIDGET (box));
	gtk_widget_thaw_child_notify (child);
}

void ygtk_ratio_box_pack_start (YGtkRatioBox *box, GtkWidget *child, gfloat ratio,
                                gboolean xfill, gboolean yfill, guint padding)
{
	ygtk_ratio_box_pack (box, child, ratio, xfill, yfill, padding, GTK_PACK_START);
}

void ygtk_ratio_box_pack_end (YGtkRatioBox *box, GtkWidget *child, gfloat ratio,
                              gboolean xfill, gboolean yfill, guint padding)
{
	ygtk_ratio_box_pack (box, child, ratio, xfill, yfill, padding, GTK_PACK_END);
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

void ygtk_ratio_box_set_child_packing (YGtkRatioBox *box, GtkWidget *child,
                                       gfloat ratio, gboolean xfill, gboolean yfill,
                                       guint padding, GtkPackType pack)
{
	YGtkRatioBoxChild *child_info;
	child_info = ygtk_ratio_get_child_info (box, child);

	if (child_info) {
		gtk_widget_freeze_child_notify (child);

		child_info->ratio = ratio;
		child_info->xfill = xfill;
		child_info->yfill = yfill;
		child_info->padding = padding;
		child_info->pack = pack;
		child_info->fully_expandable = 0;

		if (GTK_WIDGET_VISIBLE (child) && GTK_WIDGET_VISIBLE (box))
			gtk_widget_queue_resize (child);

		gtk_widget_thaw_child_notify (child);
	}
}

void ygtk_ratio_box_get_child_packing (YGtkRatioBox *box, GtkWidget *child,
                                       gfloat *ratio, gboolean *xfill,
                                       gboolean *yfill, guint *padding,
                                       gboolean *expand, GtkPackType *pack)
{
	YGtkRatioBoxChild *child_info;
	child_info = ygtk_ratio_get_child_info (box, child);

	if (child_info) {
		gtk_widget_freeze_child_notify (child);

		if (ratio)   *ratio   = child_info->ratio;
		if (xfill)   *xfill   = child_info->xfill;
		if (yfill)   *yfill   = child_info->yfill;
		if (padding) *padding = child_info->padding;
		if (expand)  *expand  = child_info->fully_expandable;
		if (pack)    *pack    = child_info->pack;
	}
}

void ygtk_ratio_box_set_child_expand (YGtkRatioBox *box, GtkWidget *child,
                                      gboolean expand)
{
	YGtkRatioBoxChild *child_info;
	child_info = ygtk_ratio_get_child_info (box, child);
	if (child_info)
		child_info->fully_expandable = expand;
}

void ygtk_ratio_box_set_child_ratio (YGtkRatioBox *box, GtkWidget *child,
                                      gfloat ratio)
{
	YGtkRatioBoxChild *child_info;
	child_info = ygtk_ratio_get_child_info (box, child);
	if (child_info)
		child_info->ratio = ratio;
}

void ygtk_ratio_box_set_homogeneous (YGtkRatioBox *box, gboolean homogeneous)
{
	box->homogeneous = homogeneous;
}

void ygtk_ratio_box_set_spacing (YGtkRatioBox *box, gint spacing)
{
	box->spacing = spacing;
}

void ygtk_ratio_box_add (GtkContainer *container, GtkWidget *child)
{
	ygtk_ratio_box_pack_start (YGTK_RATIO_BOX (container), child,
	                           1.0, TRUE, TRUE, 0);
}

void ygtk_ratio_box_remove (GtkContainer *container, GtkWidget *widget)
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

void ygtk_ratio_box_forall (GtkContainer *container, gboolean include_internals,
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
	gfloat ratios_sum = 0, max_ratio = 1;
	GList* child;
	YGtkRatioBox* box = YGTK_RATIO_BOX (widget);
	for (child = box->children; child; child = child->next) {
		YGtkRatioBoxChild* box_child = (YGtkRatioBoxChild*) child->data;
		if (!GTK_WIDGET_VISIBLE (box_child->widget))
			continue;
		if (box_child->ratio) {
			ratios_sum += box_child->ratio;
			max_ratio = MAX (max_ratio, box_child->ratio);
		}
	}

	// If we want to calculate horizontal size, primary_req would be horizontal
	// length, while secondary the height. Idem for the inverse.
	guint primary_req = 0, secondary_req = 0;
	gfloat pixels_per_percent = 0;

	for (child = box->children; child; child = child->next) {
		YGtkRatioBoxChild* box_child = (YGtkRatioBoxChild*) child->data;
		if (!GTK_WIDGET_VISIBLE (box_child->widget))
			continue;

		if (box_child->fully_expandable)
			box_child->ratio = max_ratio;

		GtkRequisition child_req;
		gtk_widget_size_request (box_child->widget, &child_req);

		guint prim_length, sec_length;
		if (orientation == GTK_ORIENTATION_HORIZONTAL) {
			prim_length = child_req.width;
			sec_length = child_req.height;
		}
		else {
			prim_length = child_req.height;
			sec_length = child_req.width;
		}

		if (box->homogeneous && box_child->ratio) {
			gfloat percent_of_whole = box_child->ratio / ratios_sum * 100;
			pixels_per_percent = MAX (pixels_per_percent, prim_length / percent_of_whole);
		}
		else
			primary_req += prim_length;
		primary_req += box_child->padding + box->spacing;
		secondary_req = MAX (secondary_req, sec_length);
	}

	primary_req += (guint) (pixels_per_percent * 100);

	guint border = GTK_CONTAINER (widget)->border_width * 2;
	primary_req += border*2; secondary_req += border*2;

	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
		requisition->width = primary_req;
		requisition->height = secondary_req;
	}
	else {
		requisition->width = secondary_req;
		requisition->height = primary_req;
	}
}

static void ygtk_ratio_box_size_allocate (GtkWidget     *widget,
                                          GtkAllocation *allocation,
                                          GtkOrientation orientation)
{
	guint border = GTK_CONTAINER (widget)->border_width;

	// a first loop to get some data for expansibles (ie. childs with weight)
	gfloat ratios_sum = 0;
	gint expansable_length;
	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		expansable_length = allocation->width - border*2;
	else
		expansable_length = allocation->height - border*2;

	GList* child;
	YGtkRatioBox* box = YGTK_RATIO_BOX (widget);
	for (child = box->children; child; child = child->next) {
		YGtkRatioBoxChild* box_child = (YGtkRatioBoxChild*) child->data;
		if (!GTK_WIDGET_VISIBLE (box_child->widget))
			continue;

		if (box_child->ratio)
			ratios_sum += box_child->ratio;

		GtkRequisition child_req;
		gtk_widget_get_child_requisition (box_child->widget, &child_req);

		if (box->homogeneous && box_child->ratio)
			;
		else {
			if (orientation == GTK_ORIENTATION_HORIZONTAL)
				expansable_length -= child_req.width;
			else
				expansable_length -= child_req.height;
		}
		expansable_length -= box->spacing - box_child->padding;
	}

	gint child_pos = 0;
	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		child_pos = allocation->x + border;
	else
		child_pos = allocation->y + border;

	gboolean right_to_left = gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL;
	gint pack;
	// outer loop to pack from start to end
	for (pack = 0; pack <= 1; pack++)
		for (child = box->children; child; child = child->next) {
			YGtkRatioBoxChild* box_child = (YGtkRatioBoxChild*) child->data;
			if (box_child->pack == pack || !GTK_WIDGET_VISIBLE (box_child->widget))
				continue;

			GtkAllocation child_alloc;
			gint length;

			GtkRequisition child_req;
			gtk_widget_get_child_requisition (box_child->widget, &child_req);

			if (box->homogeneous && box_child->ratio)
				length = 0;
			else {
				if (orientation == GTK_ORIENTATION_HORIZONTAL)
					length = child_req.width;
				else
					length = child_req.height;
			}

			if (box_child->ratio)
				length += ((box_child->ratio / ratios_sum) * expansable_length);

			if (orientation == GTK_ORIENTATION_HORIZONTAL) {
				child_alloc.x = child_pos;
				child_alloc.y = allocation->y + border;
				child_alloc.width = length;
				child_alloc.height = allocation->height - border*2;
			}
			else { // GTK_ORIENTATION_VERTICAL
				child_alloc.x = allocation->x + border;
				child_alloc.y = child_pos;
				child_alloc.width = allocation->width - border*2;
				child_alloc.height = length;
			}

			if (!box_child->xfill) {
				// we also need to center the widget
				gint width = MIN (child_alloc.width, child_req.width);
				child_alloc.x += MAX ((child_alloc.width - width) / 2, 0);
				child_alloc.width  = width;
			}
			if (!box_child->yfill) {
				gint height = MIN (child_alloc.height, child_req.height);
				child_alloc.y += MAX ((child_alloc.height - height) / 2, 0);
				child_alloc.height  = height;
			}

			if (right_to_left)
				child_alloc.x = allocation->width - child_alloc.x - child_alloc.width;

			gtk_widget_size_allocate (box_child->widget, &child_alloc);
			child_pos += length + box->spacing + box_child->padding;
		}
}

/* RatioHBox */

static void ygtk_ratio_hbox_class_init (YGtkRatioHBoxClass *klass);
static void ygtk_ratio_hbox_init       (YGtkRatioHBox      *box);
static void ygtk_ratio_hbox_size_request  (GtkWidget      *widget,
                                           GtkRequisition *requisition);
static void ygtk_ratio_hbox_size_allocate (GtkWidget      *widget,
                                           GtkAllocation  *allocation);

G_DEFINE_TYPE (YGtkRatioHBox, ygtk_ratio_hbox, YGTK_TYPE_RATIO_BOX)

static void ygtk_ratio_hbox_class_init (YGtkRatioHBoxClass *klass)
{
	ygtk_ratio_hbox_parent_class = (YGtkRatioBoxClass*) g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->size_request  = ygtk_ratio_hbox_size_request;
	widget_class->size_allocate = ygtk_ratio_hbox_size_allocate;
}

static void ygtk_ratio_hbox_init (YGtkRatioHBox *box)
{
}

GtkWidget* ygtk_ratio_hbox_new (gboolean homogeneous, gint spacing)
{
	YGtkRatioBox* box = (YGtkRatioBox*) g_object_new (YGTK_TYPE_RATIO_HBOX, NULL);
	box->homogeneous = homogeneous;
	box->spacing = spacing;
	return GTK_WIDGET (box);
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

/* RatioVBox */


static void ygtk_ratio_vbox_class_init (YGtkRatioVBoxClass *klass);
static void ygtk_ratio_vbox_init       (YGtkRatioVBox      *box);
static void ygtk_ratio_vbox_size_request  (GtkWidget      *widget,
                                           GtkRequisition *requisition);
static void ygtk_ratio_vbox_size_allocate (GtkWidget      *widget,
                                           GtkAllocation  *allocation);

G_DEFINE_TYPE (YGtkRatioVBox, ygtk_ratio_vbox, YGTK_TYPE_RATIO_BOX)

static void ygtk_ratio_vbox_class_init (YGtkRatioVBoxClass *klass)
{
	ygtk_ratio_vbox_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->size_request  = ygtk_ratio_vbox_size_request;
	widget_class->size_allocate = ygtk_ratio_vbox_size_allocate;
}

static void ygtk_ratio_vbox_init (YGtkRatioVBox *box)
{
}

GtkWidget* ygtk_ratio_vbox_new (gboolean homogeneous, gint spacing)
{
	YGtkRatioBox* box = (YGtkRatioBox*) g_object_new (YGTK_TYPE_RATIO_VBOX, NULL);
	box->homogeneous = homogeneous;
	box->spacing = spacing;
	return GTK_WIDGET (box);
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

/* YGtkMinSize */

static void ygtk_min_size_class_init (YGtkMinSizeClass *klass);
static void ygtk_min_size_init       (YGtkMinSize      *min_size);
static void ygtk_min_size_size_request (GtkWidget      *widget,
                                        GtkRequisition *requisition);
static void ygtk_min_size_size_allocate (GtkWidget      *widget,
                                         GtkAllocation  *allocation);

G_DEFINE_TYPE (YGtkMinSize, ygtk_min_size, GTK_TYPE_BIN)

void ygtk_min_size_class_init (YGtkMinSizeClass *klass)
{
	ygtk_min_size_parent_class = g_type_class_peek_parent (klass);

	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->size_request  = ygtk_min_size_size_request;
	widget_class->size_allocate  = ygtk_min_size_size_allocate;
}

void ygtk_min_size_init (YGtkMinSize *min_size)
{
	GTK_WIDGET_SET_FLAGS (min_size, GTK_NO_WINDOW);
	gtk_widget_set_redraw_on_allocate (GTK_WIDGET (min_size), FALSE);
}

GtkWidget* ygtk_min_size_new(guint min_width, guint min_height)
{
	YGtkMinSize *min_size = g_object_new (YGTK_TYPE_MIN_SIZE, NULL);
	ygtk_min_size_set_width (min_size, min_width);
	ygtk_min_size_set_height (min_size, min_height);
	return GTK_WIDGET (min_size);
}

void ygtk_min_size_set_width (YGtkMinSize *min_size, guint min_width)
{
	min_size->min_width = min_width;
}

void ygtk_min_size_set_height (YGtkMinSize *min_size, guint min_height)
{
	min_size->min_height = min_height;
}

void ygtk_min_size_set_only_expand (YGtkMinSize *min_size, gboolean only_expand)
{
	min_size->only_expand = only_expand;
}

void ygtk_min_size_size_request (GtkWidget      *widget,
                                 GtkRequisition *requisition)
{
	GtkWidget *child = GTK_BIN (widget)->child;
	requisition->width = requisition->height = 0;
	if (child && GTK_WIDGET_VISIBLE (child)) {
		gtk_widget_size_request (child, requisition);
		guint border = GTK_CONTAINER (widget)->border_width;
		requisition->width += border * 2;
		requisition->height += border * 2;

		YGtkMinSize *min_size = YGTK_MIN_SIZE (widget);
		requisition->width = MAX (requisition->width, min_size->min_width);
		requisition->height = MAX (requisition->height, min_size->min_height);

		if (min_size->only_expand) {
			min_size->min_width = requisition->width;
			min_size->min_height = requisition->height;
		}
	}
}

void ygtk_min_size_size_allocate (GtkWidget      *widget,
                                  GtkAllocation  *allocation)
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
	GTK_WIDGET_CLASS (ygtk_min_size_parent_class)->size_allocate (widget, allocation);
}
