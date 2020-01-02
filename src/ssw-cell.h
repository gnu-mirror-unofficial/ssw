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

#ifndef _SSW_CELL_H
#define _SSW_CELL_H

#include <glib-object.h>

struct _SswCell
{
  AtkObject parent_instance;

  gchar *text;
};


typedef struct _SswCell SswCell;

struct _SswCellClass
{
  AtkObjectClass parent_instance;
};

typedef struct _SswCellClass SswCellClass;

GType ssw_cell_get_type (void) G_GNUC_CONST;

G_BEGIN_DECLS

#define SSW_TYPE_CELL ssw_cell_get_type ()

#define SSW_CELL(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj),SSW_TYPE_CELL,SswCell))
#define SSW_CELL_CLASS(class)   (G_TYPE_CHECK_CLASS_CAST ((class),SSW_TYPE_CELL,SswCellClass))
#define SSW_IS_CELL(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),SSW_TYPE_CELL))
#define SSW_IS_CELL_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class),SSW_TYPE_CELL))
#define SSW_CELL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),SSW_TYPE_CELL,SswCellClass))

G_END_DECLS

#endif
