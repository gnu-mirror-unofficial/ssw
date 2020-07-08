/*
  A widget to display and manipulate tabular data
  Copyright (C) 2016, 2017, 2020  John Darrington

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

/*
  Objects of this class contain 4 objects of type SswSheetSingle, arranged
  into a 2 x 2 grid.   Normally, only one such object is visible.
  If all objects are made visible (by setting the "split" property to TRUE)
  then the user is able to see both 'ends' of the data concurrently.
*/


#ifndef _SSW_SHEET_H
#define _SSW_SHEET_H

#include <gtk/gtk.h>

typedef struct
{
  gint start_x;
  gint end_x;
  gint start_y;
  gint end_y;
} SswRange;

struct _SswSheet
{
  GtkBin parent_instance;

  /* This refers to the selected range.
     Its units are cells. Not pixels */
  SswRange *selection;

  GtkAdjustment *vadj[2] ;
  GtkAdjustment *hadj[2] ;

  GtkWidget *horizontal_axis[2];
  GtkWidget *vertical_axis[2];
  GtkWidget *sheet[2 * 2];
  GtkWidget *sw [2 * 2];

  /* True if the view is split 4 ways */
  gboolean split;

  /* The data model */
  GListModel *vmodel;
  GListModel *hmodel;
  GtkTreeModel *data_model;

  gboolean gridlines;
  gboolean editable;

  gboolean dispose_has_run;
  GtkWidget *selected_body;

  gpointer renderer_func_datum;

  GSList *cursor_stack;
  GdkCursor *wait_cursor;
};

struct _SswSheetClass
{
  GtkBinClass parent_class;
};

#define SSW_TYPE_SHEET ssw_sheet_get_type ()

G_DECLARE_FINAL_TYPE (SswSheet, ssw_sheet, SSW, SHEET, GtkBin)

  GtkWidget *ssw_sheet_new (void);
GtkWidget *ssw_sheet_get_button (SswSheet *s);

/* Prime CLIP such that a paste request will act upon this SHEET */
void ssw_sheet_set_clip (SswSheet *sheet, GtkClipboard *clip);

void ssw_sheet_wait_push (SswSheet *sheet);
void ssw_sheet_wait_pop (SswSheet *sheet);

typedef void (*ssw_sheet_set_cell) (GtkTreeModel *store, gint col, gint row,
                                    const GValue *value);

void ssw_sheet_paste (SswSheet *sheet, GtkClipboard *clip, ssw_sheet_set_cell sc);

/* Check if an editable is focused. If yes, cut to clipboard and return TRUE */
gboolean ssw_sheet_try_cut (SswSheet *sheet);

/* Scroll the sheet so that the cell HPOS,VPOS is approximately in the center.
   Either HPOS or VPOS may be -1, in which case that position is unchanged. */
void ssw_sheet_scroll_to (SswSheet *sheet, gint hpos, gint vpos);

void ssw_sheet_set_active_cell (SswSheet *sheet,
                                gint col, gint row, GdkEvent *e);

gboolean ssw_sheet_get_active_cell (SswSheet *sheet,
                                    gint *col, gint *row);


typedef gboolean (*ssw_sheet_reverse_conversion_func)
(GtkTreeModel *model, gint col, gint row, const gchar *in, GValue *out);

typedef gchar * (*ssw_sheet_forward_conversion_func)
  (SswSheet *sheet, GtkTreeModel *m, gint col, gint row, const GValue *in);


gchar * ssw_sheet_default_forward_conversion (SswSheet *sheet, GtkTreeModel *m,
                                              gint col, gint row,
                                              const GValue *value);

gboolean ssw_sheet_default_reverse_conversion (GtkTreeModel *model,
                                               gint col, gint row,
                                               const gchar *in, GValue *out);
#endif
