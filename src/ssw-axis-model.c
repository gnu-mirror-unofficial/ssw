/*
    A candidate replacement for Pspp's sheet
    Copyright (C) 2016  John Darrington

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



/* Draw a diagonal line through the widget */
static gboolean
draw_strike (GtkWidget *widget, cairo_t *cr, gpointer ud)
{
  guint width = gtk_widget_get_allocated_width (widget);
  guint height = gtk_widget_get_allocated_height (widget);

  GtkStyleContext *sc = gtk_widget_get_style_context (widget);

  gtk_render_line (sc, cr, 0, 0, width, height);

  return FALSE;
}

static gpointer
datum_create_func (SswAxisModel *am, guint id)
{
  gchar *text = g_strdup_printf ("%u", id);
  gchar *label = g_strdup_printf ("Number %u", id);

  GtkWidget *button = gtk_button_new_with_label (text);
  gtk_widget_set_tooltip_text (GTK_WIDGET (button), label);
  g_free (text);
  g_free (label);

  if ((id % 5) == 4 && am->strike)
    g_signal_connect_after (button, "draw", G_CALLBACK (draw_strike), NULL);

  if ((id % 7) == 6)
    {
      GtkRequisition req;
      gtk_widget_show_now (GTK_WIDGET (button));
      gtk_widget_get_preferred_size (button, NULL, &req);
      gtk_widget_set_size_request (button, req.width * 3, -1);
    }

  gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);

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
  return G_TYPE_POINTER;
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
    PROP_OFFSET,
    PROP_STRIKE
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
    case PROP_STRIKE:
      SSW_AXIS_MODEL (object)->strike = g_value_get_boolean (value);
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


  GParamSpec *strike_spec =
    g_param_spec_boolean ("strike",
			  P_("Strike"),
			  P_("Strike through some headers"),
			  FALSE,
			  G_PARAM_READWRITE);

  g_object_class_install_property (object_class,
                                   PROP_SIZE,
                                   size_spec);


  g_object_class_install_property (object_class,
                                   PROP_OFFSET,
                                   offset_spec);


  g_object_class_install_property (object_class,
                                   PROP_STRIKE,
                                   strike_spec);
}

