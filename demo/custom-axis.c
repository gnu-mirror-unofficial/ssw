/*
    A widget to display and manipulate tabular data
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

#include "custom-axis.h"

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
datum_create_func (CustomAxisModel *am, guint id)
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

  return datum_create_func (CUSTOM_AXIS_MODEL (list), id);
}


static void
custom_init_iface (GListModelInterface *iface)
{
  iface->get_n_items = gni;
  iface->get_item = gi;
  iface->get_item_type = git;
}


G_DEFINE_TYPE_WITH_CODE (CustomAxisModel, custom_axis_model,
                         SSW_TYPE_AXIS_MODEL,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, custom_init_iface));

static void
custom_axis_model_init (CustomAxisModel *d)
{
}


enum
  {
    PROP_0,
    PROP_STRIKE
  };

/* GObject vfuncs {{{ */
static void
__set_property (GObject *object,
                guint prop_id, const GValue *value, GParamSpec * pspec)
{
  switch (prop_id)
    {
    case PROP_STRIKE:
      CUSTOM_AXIS_MODEL (object)->strike = g_value_get_boolean (value);
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
  CustomAxisModel *m = CUSTOM_AXIS_MODEL (object);
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
custom_axis_model_class_init (CustomAxisModelClass *dc)
{
  GObjectClass *object_class = G_OBJECT_CLASS (dc);

  object_class->set_property = __set_property;
  object_class->get_property = __get_property;

  GParamSpec *strike_spec =
    g_param_spec_boolean ("strike",
			  P_("Strike"),
			  P_("Strike through some headers"),
			  FALSE,
			  G_PARAM_READWRITE);

  g_object_class_install_property (object_class,
                                   PROP_STRIKE,
                                   strike_spec);
}

GObject *
custom_axis_model_new (void)
{
  return g_object_new (CUSTOM_TYPE_AXIS_MODEL, NULL);
}
