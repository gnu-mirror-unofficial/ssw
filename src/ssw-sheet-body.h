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

#ifndef _SSW_SHEET_BODY_H
#define _SSW_SHEET_BODY_H

#include <gtk/gtk.h>

#include "ssw-sheet-axis.h"
#include "ssw-sheet.h"


struct _SswSheetBodyClass
{
  GtkLayoutClass parent_class;
};

#define SSW_TYPE_SHEET_BODY ssw_sheet_body_get_type ()

G_DECLARE_DERIVABLE_TYPE (SswSheetBody, ssw_sheet_body, SSW, SHEET_BODY, GtkLayout)

  GtkWidget * ssw_sheet_body_new (SswSheet *sheet);

void ssw_sheet_body_set_clip (SswSheetBody *body, GtkClipboard *clip);

void ssw_sheet_body_unset_selection (SswSheetBody *body);

void ssw_sheet_body_value_to_string (SswSheetBody *body, gint col, gint row,
                                     GString *output);

void ssw_sheet_body_set_active_cell (SswSheetBody *body,
                                     gint col, gint row, GdkEvent *e);

gboolean ssw_sheet_body_get_active_cell (SswSheetBody *body,
                                         gint *col, gint *row);

/* Check if the editable is focused. If yes paste into the
   editable. Return False if the editable is not focused */
gboolean ssw_sheet_body_paste_editable (SswSheetBody *body);

/* Check if editable is focused. If yes do the clipboard cut and return TRUE */
gboolean ssw_sheet_body_cut_editable (SswSheetBody *body);

#endif
