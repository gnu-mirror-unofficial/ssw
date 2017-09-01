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

#ifndef _SSW_DATUM_H
#define _SSW_DATUM_H

#include <glib-object.h>
#include "ssw-axis-model.h"
#include <gtk/gtk.h>

struct _SswDatum
{
  GObject parent_instance;

  /* The text of the button */
  gchar *text;

  /* The tooltip of the button */
  gchar *label;

  /* Whether the button should be overstruck with a diagonal bar */
  gboolean strike;

  /* Make this item extra wide */
  gboolean wide;

  gboolean sensitive;
};

struct _SswDatumClass
{
  GObjectClass parent_class;
};

#define SSW_TYPE_DATUM ssw_datum_get_type ()

G_DECLARE_FINAL_TYPE (SswDatum, ssw_datum, SSW, DATUM, GObject)

GtkWidget *cell_fill_func (gpointer   item);
gpointer datum_create_func (SswAxisModel *am, guint id);



#endif
