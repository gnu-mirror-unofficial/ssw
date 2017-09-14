/*
  A widget to display and manipulate tabular data
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

/* A demonstration program for the SswSheet widget */


#include <config.h>
#include <gtk/gtk.h>

#include "ssw-sheet.h"
#include "ssw-axis-model.h"
#include "ssw-virtual-model.h"
#include "ssw-sheet-body.h"
#include "custom-axis.h"

static CustomAxisModel *ca;


static void
row_clicked (GtkWidget *w, gint i, gpointer ud)
{
  g_print ("You clicked on row %d\n", i);
}


static void
column_clicked (GtkWidget *w, gint i, gpointer ud)
{
  g_print ("You clicked on column %d\n", i);
}

static void
selection_label_update (GtkWidget *sheet, SswRange *sel, GtkWidget *label)
{
  gchar *t = g_strdup_printf ("Selected %d,%d -  %d,%d", sel->start_x, sel->start_y,
			      sel->end_x, sel->end_y);

  gtk_label_set_text (GTK_LABEL (label), t);
  g_free (t);
}



static void
change_split (GtkToggleButton *tb, gpointer d)
{
  SswSheet *sheet = SSW_SHEET (d);
  g_object_set (sheet, "split", gtk_toggle_button_get_active (tb), NULL);
}

static GtkTreeModel *data_model[3];

static void
on_combo_select (GtkComboBox *tb, gpointer d)
{
  SswSheet *sheet = SSW_SHEET (d);

  gint x = gtk_combo_box_get_active (tb);
  g_object_set (sheet, "vmodel", ca, NULL);
  g_object_set (sheet, "hmodel", NULL, NULL);
  g_object_set (sheet, "data-model", data_model[x], NULL);
}


static void
toggle_direction (GtkToggleButton *tb, gpointer d)
{
  SswSheet *sheet = SSW_SHEET (d);

  gtk_widget_set_default_direction (gtk_toggle_button_get_active (tb) ? GTK_TEXT_DIR_RTL : GTK_TEXT_DIR_LTR) ;
}


static void
cb (GtkClipboard *clip, GdkAtom *atoms, gint n_atoms, gpointer data)
{
  g_print ("Here\n");

  int i;
  for (i = 0; i < n_atoms; ++i)
    {
      g_print ("Name %s\n", gdk_atom_name (atoms[i]));
    }

}

static void
on_spare (GtkButton *b, gpointer d)
{
  SswSheet *sheet = SSW_SHEET (d);

  //  ensure_visible_widgets (sheet->vertical_axis[0], TRUE);

  /* GtkClipboard *clip = */
  /*   gtk_clipboard_get_for_display (gtk_widget_get_display (GTK_WIDGET (sheet)), */
  /* 				   GDK_SELECTION_CLIPBOARD); */

  /* gtk_clipboard_request_targets (clip, cb, sheet); */


  //  ssw_sheet_axis_jump_end (sheet->vertical_axis[0], 50);
  g_print ("Spare\n");
  //  ssw_sheet_scroll_to (sheet, 10, 1);

  ssw_sheet_set_active_cell (sheet, 10, 2, 0);
}


static GtkWidget *row_control ;
static GtkWidget *col_control ;


static void
change_size_cb (GtkButton *butt, gpointer ud)
{
  SswSheet *sheet = SSW_SHEET (ud);

  GObject *model = NULL;
  g_object_get (sheet, "data-model", &model, NULL);

  {
    gdouble change = gtk_spin_button_get_value (GTK_SPIN_BUTTON (col_control));
    gint columns;
    g_object_get (model, "columns", &columns, NULL);
    g_object_set (model, "columns", columns + (gint) change, NULL);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (col_control), 0);
  }

  {
    gdouble change = gtk_spin_button_get_value (GTK_SPIN_BUTTON (row_control));
    gint rows;
    g_object_get (model, "rows", &rows, NULL);
    g_object_set (model, "rows", rows + (gint) change, NULL);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (row_control), 0);
  }

}





static void
copy (GtkButton *b, gpointer d)
{
  SswSheet *sheet = SSW_SHEET (d);

  g_print ("Copy\n");

  GtkClipboard *clip =
    gtk_clipboard_get_for_display (gtk_widget_get_display (GTK_WIDGET (sheet)),
				   GDK_SELECTION_CLIPBOARD);


  ssw_sheet_set_clip (sheet, clip);
}



static void
toggle_gridlines (GtkToggleButton *tb, gpointer d)
{
  SswSheet *sheet = SSW_SHEET (d);
  g_object_set (sheet, "gridlines", gtk_toggle_button_get_active (tb), NULL);
}

static void
toggle_editable (GtkToggleButton *tb, gpointer d)
{
  SswSheet *sheet = SSW_SHEET (d);
  g_object_set (sheet, "editable", gtk_toggle_button_get_active (tb), NULL);
}


static void
toggle_strike (GtkToggleButton *tb, gpointer d)
{
  SswSheet *sheet = SSW_SHEET (d);

  GListModel *vstore = NULL;

  g_object_get (sheet, "vmodel", &vstore, NULL);


  gboolean strike = gtk_toggle_button_get_active (tb);
  g_object_set (vstore, "strike", strike, NULL);
}



static void
on_click (GtkButton *b, gpointer d)
{
  GtkLabel *label = d;
  static int x = 0;
  gchar *t = g_strdup_printf ("You clicked %s", x%2 ? " again" :"" );
  gtk_label_set_text (label, t);
  g_free (t);
  ++x;
}


static GtkWidget *
create_size_control (GtkWidget *sheet)
{
  GtkWidget *frame = gtk_frame_new ("Change size");

  GtkWidget *size_controls = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add (GTK_CONTAINER (frame), size_controls);

  row_control = gtk_spin_button_new_with_range (-G_MAXINT, +G_MAXINT, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (row_control), 0.0);

  col_control = gtk_spin_button_new_with_range (-G_MAXINT, +G_MAXINT, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (col_control), 0.0);

  GtkWidget *change_size_button = gtk_button_new_with_label ("Change");

  gtk_box_pack_start (GTK_BOX (size_controls), gtk_label_new ("Rows:"),
		      FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (size_controls), row_control, FALSE, FALSE, 5);

  gtk_box_pack_start (GTK_BOX (size_controls), gtk_label_new ("Columns:"),
		      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (size_controls), col_control, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (size_controls), change_size_button,
		      FALSE, FALSE, 5);

  g_signal_connect (change_size_button, "clicked",
		    G_CALLBACK (change_size_cb), sheet);

  return frame;
}


static void
on_value_change (SswSheet *sheet, gint col, gint row, const GValue *value, gpointer ud)
{
  gchar *x = g_strdup_value_contents (value);
  g_print ("Value changed for %p at column %d, row  %d. New value: %s\n", sheet, col, row, x);
  g_free (x);
}


static GtkWidget *
create_control_box (GtkWidget *sheet)
{
  GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);

  GtkWidget *toggle1 = gtk_check_button_new_with_label ("4 way  split");
  GtkWidget *toggle2 = gtk_check_button_new_with_label ("Toggle gridlines");
  GtkWidget *toggle_ed = gtk_check_button_new_with_label ("Editable");
  GtkWidget *toggle3 = gtk_check_button_new_with_label ("Strike each 5th row");
  GtkWidget *toggle4 = gtk_check_button_new_with_label ("RTL");
  GtkWidget *spare = gtk_button_new_with_label ("Spare Button");
  GtkWidget *clip = gtk_button_new_with_label ("Copy to clip");
  GtkWidget *selection_label = gtk_label_new ("");

  GtkWidget *size_control = create_size_control (sheet);

  GtkWidget *combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_insert (GTK_COMBO_BOX_TEXT (combo), 0,
			     "a", "Empty");
  gtk_combo_box_text_insert (GTK_COMBO_BOX_TEXT (combo), 1,
			     "b", "Reasonable");
  gtk_combo_box_text_insert (GTK_COMBO_BOX_TEXT (combo), 2,
			     "c", "Enormous");
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 1);

  GtkWidget *label = gtk_label_new ("");

  g_signal_connect (combo, "changed", G_CALLBACK (on_combo_select), sheet);
  g_signal_emit_by_name (combo, "changed", sheet);

  gboolean ac;
  g_object_get (sheet, "gridlines", &ac, NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle2), ac);


  gboolean ed;
  g_object_get (sheet, "editable", &ed, NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle_ed), ed);

  GtkWidget *sb = ssw_sheet_get_button (SSW_SHEET (sheet));
  g_signal_connect (sb, "clicked", G_CALLBACK (on_click), label);

  gtk_button_set_label (GTK_BUTTON (sb), "Click me");
  g_signal_connect (spare, "clicked", G_CALLBACK (on_spare), sheet);
  g_signal_connect (clip, "clicked", G_CALLBACK (copy), sheet);
  g_signal_connect (toggle1, "toggled", G_CALLBACK (change_split), sheet);
  g_signal_connect (toggle2, "toggled", G_CALLBACK (toggle_gridlines), sheet);
  g_signal_connect (toggle_ed, "toggled", G_CALLBACK (toggle_editable), sheet);
  g_signal_connect (toggle3, "toggled", G_CALLBACK (toggle_strike), sheet);
  g_signal_connect (toggle4, "toggled", G_CALLBACK (toggle_direction), sheet);

  gtk_container_add (GTK_CONTAINER (vbox), toggle1);
  gtk_container_add (GTK_CONTAINER (vbox), toggle2);
  gtk_container_add (GTK_CONTAINER (vbox), toggle_ed);
  gtk_container_add (GTK_CONTAINER (vbox), toggle3);
  gtk_container_add (GTK_CONTAINER (vbox), toggle4);
  gtk_container_add (GTK_CONTAINER (vbox), label);
  gtk_container_add (GTK_CONTAINER (vbox), spare);
  gtk_container_add (GTK_CONTAINER (vbox), combo);
  gtk_container_add (GTK_CONTAINER (vbox), clip);
  gtk_container_add (GTK_CONTAINER (vbox), selection_label);
  gtk_container_add (GTK_CONTAINER (vbox), size_control);


  g_signal_connect (sheet, "selection-changed", G_CALLBACK (selection_label_update), selection_label);

  g_signal_connect (sheet, "value-changed", G_CALLBACK (on_value_change), NULL);

  return vbox;
}

int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  data_model[0] = g_object_new (SSW_TYPE_VIRTUAL_MODEL,
				"rows", 0,
				"columns", 0,
				NULL);

  data_model[1] = g_object_new (SSW_TYPE_VIRTUAL_MODEL,
				"rows", 10,
				"columns", 20,
				NULL);

  data_model[2] = g_object_new (SSW_TYPE_VIRTUAL_MODEL,
				"rows", 1000000,
				"columns", 100,
				NULL);

  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  GtkWidget *sheet = ssw_sheet_new ();

  ca = custom_axis_model_new ();
  g_object_set (sheet, "vmodel", ca, NULL);
  
  GtkWidget *cbox = create_control_box (sheet);

  g_signal_connect (sheet, "row-header-clicked",
		    G_CALLBACK (row_clicked), 0);

  g_signal_connect (sheet, "column-header-clicked",
		    G_CALLBACK (column_clicked), 0);

  gtk_container_add (GTK_CONTAINER (hbox), cbox);
  gtk_container_add (GTK_CONTAINER (hbox), sheet);

  gtk_container_add (GTK_CONTAINER (window), hbox);

  g_signal_connect (G_OBJECT (window), "delete-event",
		    G_CALLBACK (gtk_main_quit), NULL);

  gtk_window_resize (GTK_WINDOW (window), 800, 400);
  gtk_window_maximize (GTK_WINDOW (window));

  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
