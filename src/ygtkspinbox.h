/* YGtkSpinBox widget */

#ifndef YGTK_SPIN_BOX_H
#define YGTK_SPIN_BOX_H

#include <gtk/gtkspinbutton.h>

G_BEGIN_DECLS

#define YGTK_TYPE_SPIN_BOX            (ygtk_spin_box_get_type ())
#define YGTK_SPIN_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                      YGTK_TYPE_SPIN_BOX, YGtkSpinBox))
#define YGTK_SPIN_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                                      YGTK_TYPE_SPIN_BOX, YGtkSpinBoxClass))
#define IS_YGTK_SPIN_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                      YGTK_TYPE_SPIN_BOX))
#define IS_YGTK_SPIN_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  \
                                      YGTK_TYPE_SPIN_BOX))
#define YGTK_SPIN_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  \
                                      YGTK_TYPE_SPIN_BOX, YGtkSpinBoxClass))

typedef struct _YGtkSpinBox       YGtkSpinBox;
typedef struct _YGtkSpinBoxClass  YGtkSpinBoxClass;
typedef struct _YGtkSpinBoxField  YGtkSpinBoxField;

struct _YGtkSpinBox
{
	GtkSpinButton spin_button;

	// members
	GList *fields;  // of YGtkSpinBoxField
	int active_field;
	char separator;
};

struct _YGtkSpinBoxClass
{
	GtkSpinButtonClass parent_class;
};

struct _YGtkSpinBoxField
{
	int value;
	int min_value, max_value;
	int digits_nb;
};

GtkWidget* ygtk_spin_box_new();
GType ygtk_spin_box_get_type (void) G_GNUC_CONST;

guint ygtk_spin_box_add_field (YGtkSpinBox *spin_box, int min_range, int max_range);
void ygtk_spin_box_set_separator_character (YGtkSpinBox *spin_box, char separator);
// (':' by default)

gint ygtk_spin_box_get_value (YGtkSpinBox *spin_box, guint field_nb);
void ygtk_spin_box_set_value (YGtkSpinBox *spin_box, guint field_nb, gint value);

G_END_DECLS

#endif /*YGTK_SPIN_BOX_H*/
