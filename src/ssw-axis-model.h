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

#ifndef _SSW_AXIS_MODEL_H
#define _SSW_AXIS_MODEL_H

#include <glib-object.h>

G_DECLARE_FINAL_TYPE (SswAxisModel, ssw_axis_model, SSW, AXIS_MODEL, GObject)


struct _SswAxisModel
{
  GObject parent_instance;

  guint size;
  gint offset;
  void (*button_post_create_func) (GtkWidget *, guint, gpointer);
  gpointer button_post_create_func_data;
};

struct _SswAxisModelClass
{
  GObjectClass parent_instance;
};



#define SSW_TYPE_AXIS_MODEL ssw_axis_model_get_type ()

#endif
