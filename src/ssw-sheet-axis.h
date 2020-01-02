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

#ifndef _SSW_SHEET_AXIS_H
#define _SSW_SHEET_AXIS_H

#include <gtk/gtk.h>

typedef struct
{
  gint position;
  gint size;
} SswGeometry;

struct _SswSheetAxis
{
  GtkContainer parent_instance;

  gint last_cell;
  gint first_cell;
  GPtrArray *cell_limits;
};

struct _SswSheetAxisClass
{
  GtkContainerClass parent_class;
};

#define SSW_TYPE_SHEET_AXIS ssw_sheet_axis_get_type ()

G_DECLARE_FINAL_TYPE (SswSheetAxis, ssw_sheet_axis, SSW, SHEET_AXIS, GtkContainer)

GtkWidget * ssw_sheet_axis_new (GtkOrientation orientation);

void ssw_sheet_axis_set_model (SswSheetAxis *box, GListModel *model);

GListModel *ssw_sheet_axis_get_model (SswSheetAxis *box);

gint ssw_sheet_axis_find_cell (SswSheetAxis *axis,  gdouble pos, gint *offset, gint *size);

gint ssw_sheet_axis_find_boundary (SswSheetAxis *axis,  gint pos, gint *offset, gint *size);

void ssw_sheet_axis_override_size (SswSheetAxis *axis, gint pos, gint size);

gint ssw_sheet_axis_get_size (SswSheetAxis *axis);
gint ssw_sheet_axis_get_extent (SswSheetAxis *axis);

gint ssw_sheet_axis_get_visible_size (SswSheetAxis *axis);

void ssw_sheet_axis_jump_start (SswSheetAxis *axis, gint whereto);
void ssw_sheet_axis_jump_end (SswSheetAxis *axis, gint whereto);
void ssw_sheet_axis_jump_center (SswSheetAxis *axis, gint whereto);

gint ssw_sheet_axis_get_last (SswSheetAxis *axis);
gint ssw_sheet_axis_get_first (SswSheetAxis *axis);


void ssw_sheet_axis_info (SswSheetAxis *axis);

gboolean ssw_sheet_axis_rtl (SswSheetAxis *axis);


#endif
