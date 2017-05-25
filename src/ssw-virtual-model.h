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

/* An implementation of GtkTreeModel which is useful only for testing.
   Each cell is a text string of the form "rNcM" where N and M are the
   row and column numbers respectively.
 */

#ifndef _SSW_VIRTUAL_MODEL_H
#define _SSW_VIRTUAL_MODEL_H

#include <glib-object.h>

struct _SswVirtualModel
{
  GObject parent_instance;

  guint cols;
  guint rows;
  guint32 stamp;
};

struct _SswVirtualModelClass
{
  GObjectClass parent_class;
};

#define SSW_TYPE_VIRTUAL_MODEL ssw_virtual_model_get_type ()

G_DECLARE_FINAL_TYPE (SswVirtualModel, ssw_virtual_model, SSW, VIRTUAL_MODEL, GObject)


GObject * ssw_virtual_model_new (void);


#endif
