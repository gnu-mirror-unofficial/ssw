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

/*  This class is a GtkGrid which determines the structure of the sheet widget.

    +-----------+-----------------------------------------+
    | Button    |        Horizontal Axis                  |
    +-----------+-----------------------------------------+
    |           |                                         |
    |           |                                         |
    |           |                                         |
    |           |                                         |
    | Vertical  |                                         |
    |   Axis    |                                         |
    |           |             Body                        |
    |           |                                         |
    |           |                                         |
    |           |                                         |
    |           |                                         |
    |           |                                         |
    |           |                                         |
    |           |                                         |
    |           |                                         |
    |           |                                         |
    +-----------+-----------------------------------------+
*/


#ifndef _SSW_SHEET_SINGLE_H
#define _SSW_SHEET_SINGLE_H

#include <gtk/gtk.h>
#include "ssw-sheet.h"
#include "ssw-sheet-axis.h"

struct _SswSheetSingle
{
  GtkGrid parent_instance;

  /* Visible components of the sheet.
     See above diagram. */
  GtkWidget *vertical_axis;
  GtkWidget *horizontal_axis;
  GtkWidget *body;
  GtkWidget *button;

  GtkWidget *sheet;

  GtkAdjustment *vadj;
  GtkAdjustment *hadj;

  GtkTreeModel *data_model;

  gboolean dispose_has_run;
};

struct _SswSheetSingleClass
{
  GtkGridClass parent_class;
};

#define SSW_TYPE_SHEET_SINGLE ssw_sheet_single_get_type ()

G_DECLARE_FINAL_TYPE (SswSheetSingle, ssw_sheet_single, SSW, SHEET_SINGLE, GtkGrid)

  GtkWidget * ssw_sheet_single_new (SswSheet *sheet,
                                    SswSheetAxis *haxis, SswSheetAxis *vaxis,
                                    SswRange *selection);


#endif
