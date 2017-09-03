/*
    A candidate replacement for Pspp's sheet
    Copyright (C) 2016, 2017  John Darrington

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
#include <gtk/gtk.h>

#include "ssw-axis-model.h"

#define P_(X) (X)



static gpointer
datum_create_func (SswAxisModel *am, guint id)
{
  gchar *text = g_strdup_printf ("%u", id);
  GtkWidget *button = gtk_button_new_with_label (text);
  g_free (text);

  return button;
}



static guint
gni (GListModel *list)
{
  return SSW_AXIS_MODEL(list)->size;
}

static GType
git (GListModel *list)
{
  return GTK_TYPE_BUTTON;
}

static gpointer
gi (GListModel *list, guint position)
{
  guint id = position + SSW_AXIS_MODEL(list)->offset;

  return datum_create_func (SSW_AXIS_MODEL (list), id);
}


static void
ssw_init_iface (GListModelInterface *iface)
{
  iface->get_n_items = gni;
  iface->get_item = gi;
  iface->get_item_type = git;
}

G_DEFINE_TYPE_WITH_CODE (SswAxisModel, ssw_axis_model,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, ssw_init_iface));

static void
ssw_axis_model_init (SswAxisModel *d)
{
}

enum
  {
    PROP_0,
    PROP_SIZE,
    PROP_OFFSET
  };

/* GObject vfuncs {{{ */
static void
__set_property (GObject *object,
                guint prop_id, const GValue *value, GParamSpec * pspec)
{
  switch (prop_id)
    {
    case PROP_SIZE:
      SSW_AXIS_MODEL (object)->size = g_value_get_uint (value);
      break;
    case PROP_OFFSET:
      SSW_AXIS_MODEL (object)->offset = g_value_get_int (value);
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
  SswAxisModel *m = SSW_AXIS_MODEL (object);
  switch (prop_id)
    {
    case PROP_SIZE:
      g_value_set_uint (value, m->size);
      break;
    case PROP_OFFSET:
      g_value_set_int (value, m->offset);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
ssw_axis_model_class_init (SswAxisModelClass *dc)
{
  GObjectClass *object_class = G_OBJECT_CLASS (dc);

  object_class->set_property = __set_property;
  object_class->get_property = __get_property;

  GParamSpec *size_spec =
    g_param_spec_uint ("size",
		      P_("Size"),
		      P_("The number of items in the model"),
		      0, UINT_MAX, 10000,
		      G_PARAM_READWRITE);

  GParamSpec *offset_spec =
    g_param_spec_int ("offset",
		      P_("Offset"),
		      P_("The enumeration of the first item in the model"),
		      -INT_MAX, INT_MAX, 1,
		      G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   PROP_SIZE,
                                   size_spec);

  g_object_class_install_property (object_class,
                                   PROP_OFFSET,
                                   offset_spec);
}

