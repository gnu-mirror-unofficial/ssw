/*
    A widget to display and manipulate tabular data
    Copyright (C) 2017  John Darrington

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


#ifndef _SSW_PASTE_H
#define _SSW_PASTE_H

#include "ssw-sheet.h"

struct paste_state
{
  gint col0;
  gint row0;
  gint col;
  gint row;
  SswSheet *sheet;
  ssw_sheet_set_cell set_cell;
  gboolean cell_element;
};

void paste_insert_datum (const gchar *x, size_t len, const struct paste_state *t);

void html_parse (GtkClipboard *clip, GtkSelectionData *sd, gpointer user_data);


#endif
