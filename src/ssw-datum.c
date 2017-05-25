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
#include "ssw-datum.h"
#include "ssw-sheet-axis.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (SswDatum, ssw_datum, G_TYPE_OBJECT)

static void
__finalize (GObject *obj)
{
  SswDatum *datum = SSW_DATUM (obj);

  g_free (datum->text);
  g_free (datum->label);
}


static void
ssw_datum_class_init (SswDatumClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = __finalize;
}

static void
ssw_datum_init (SswDatum *obj)
{
  SswDatum *datum = SSW_DATUM (obj);

  datum->sensitive = TRUE;
  datum->text = NULL;
  datum->label = NULL;
  datum->wide = FALSE;
  datum->strike = FALSE;
}



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

GtkWidget *
cell_fill_func (SswSheetAxis *axis,
		gpointer   item,
		GtkWidget *old_widget,
		guint      item_index)
{
  SswDatum *datum = SSW_DATUM (item);

  if (old_widget)
    g_object_unref (old_widget);

  GtkWidget *button = gtk_button_new_with_label (datum->text);
  gtk_widget_set_tooltip_text (GTK_WIDGET (button), datum->label);


  if (datum->strike)
    g_signal_connect_after (button, "draw", G_CALLBACK (draw_strike), NULL);

  if (datum->wide)
    {
      GtkRequisition req;
      gtk_widget_show_now (GTK_WIDGET (button));
      gtk_widget_get_preferred_size (button, NULL, &req);
      gtk_widget_set_size_request (button, req.width * 3, -1);
    }

  gtk_widget_set_sensitive (GTK_WIDGET (button), datum->sensitive);

  g_object_unref (datum);

  return GTK_WIDGET (button);
}
