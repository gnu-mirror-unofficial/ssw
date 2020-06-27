/*
  A widget to display and manipulate tabular data
  Copyright (C) 2016, 2020  John Darrington

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>
#include "ssw-constraint.h"



#define P_(X) (X)


G_DEFINE_TYPE (SswConstraint, ssw_constraint, GTK_TYPE_BIN)

  enum
    {
     PROP_0,
     PROP_VCONSTRAINT,
     PROP_HCONSTRAINT
    };


static void
__set_property (GObject *object,
                guint prop_id, const GValue *value, GParamSpec *pspec)
{
  switch (prop_id)
    {
    case PROP_VCONSTRAINT:
      SSW_CONSTRAINT (object)->vconstraint = g_value_get_int (value);
      break;
    case PROP_HCONSTRAINT:
      SSW_CONSTRAINT (object)->hconstraint = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
__get_property (GObject * object,
                guint prop_id, GValue * value, GParamSpec * pspec)
{
  switch (prop_id)
    {
    case PROP_VCONSTRAINT:
      g_value_set_int (value, SSW_CONSTRAINT (object)->vconstraint);
      break;
    case PROP_HCONSTRAINT:
      g_value_set_int (value, SSW_CONSTRAINT (object)->hconstraint);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static const gint border = 3;

GtkWidget *
ssw_constraint_new (void)
{
  return GTK_WIDGET (g_object_new (SSW_TYPE_CONSTRAINT, NULL));
}


static void
__size_allocate (GtkWidget    *widget,
                 GdkRectangle *allocation)
{
  SswConstraint *c = SSW_CONSTRAINT (widget);

  if (allocation->width > c->hconstraint)
    allocation->width = c->hconstraint;

  if (allocation->height > c->vconstraint)
    allocation->height = c->vconstraint;

  GtkAllocation clip = *allocation;
  clip.x -= border;
  clip.y -= border;
  clip.width += 2 * border + 1;
  clip.height += 2 * border + 1;

  gtk_widget_set_clip (widget, &clip);

  GTK_WIDGET_CLASS (ssw_constraint_parent_class)->size_allocate (widget, allocation);
}



static void
ssw_constraint_class_init (SswConstraintClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  widget_class->size_allocate = __size_allocate;

  object_class->set_property = __set_property;
  object_class->get_property = __get_property;


  GParamSpec *vcons_spec =
    g_param_spec_int ("vconstraint",
                      P_("Vertical Constraint"),
                      P_("The upper limit on the child's vertical size"),
                      0, INT_MAX, INT_MAX,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT);


  GParamSpec *hcons_spec =
    g_param_spec_int ("hconstraint",
                      P_("Horizontal Constraint"),
                      P_("The upper limit on the child's horizontal size"),
                      0, INT_MAX, INT_MAX,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT);


  g_object_class_install_property (object_class,
                                   PROP_VCONSTRAINT,
                                   vcons_spec);

  g_object_class_install_property (object_class,
                                   PROP_HCONSTRAINT,
                                   hcons_spec);
}

static void
ssw_constraint_init (SswConstraint *constraint)
{
}
