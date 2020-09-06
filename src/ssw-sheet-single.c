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

#include <config.h>
#include "ssw-sheet-single.h"
#include "ssw-sheet-axis.h"
#include "ssw-sheet-body.h"
#include "ssw-axis-model.h"
#include "ssw-cell.h"

#define P_(X) (X)

static AtkObject*
__ref_at (AtkTable      *table,
          gint          row,
          gint          column)
{
  SswSheetBody *body = SSW_SHEET_BODY (SSW_SHEET_SINGLE (table)->body);
  GString *output = g_string_new (NULL);

  ssw_sheet_body_value_to_string (body, column, row, output);

  AtkObject *o = g_object_new (SSW_TYPE_CELL,
                               "content", output->str,
                               NULL);

  g_string_free (output, FALSE);

  return o;
}


static   gint
__get_n_cols (AtkTable *table)
{
  GtkTreeModel *model = SSW_SHEET_SINGLE (table)->data_model;
  return gtk_tree_model_get_n_columns (model);
}

static   gint
__get_n_rows (AtkTable *table)
{
  GtkTreeModel *model = SSW_SHEET_SINGLE (table)->data_model;
  return gtk_tree_model_iter_n_children (model, NULL);
}


static void
__atk_table_init (AtkTableIface *iface)
{
  iface->ref_at = __ref_at;
  iface->get_n_columns = __get_n_cols;
  iface->get_n_rows = __get_n_rows;
}



G_DEFINE_TYPE_WITH_CODE (SswSheetSingle, ssw_sheet_single, GTK_TYPE_GRID,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_SCROLLABLE, NULL)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_TABLE, __atk_table_init)
                         );


static void
__realize (GtkWidget *w)
{
  SswSheetSingle *single = SSW_SHEET_SINGLE (w);
  SswSheet *psheet = SSW_SHEET (single->sheet);

  GTK_WIDGET_CLASS (ssw_sheet_single_parent_class)->realize (w);

  g_object_set (single->body,
                "sheet",     single->sheet,
                "expand",    TRUE,
                "can-focus", TRUE,
                NULL);

  GList focus_chain;
  focus_chain.next = NULL;
  focus_chain.prev = NULL;
  focus_chain.data = single->body;
  gtk_container_set_focus_chain (GTK_CONTAINER (single), &focus_chain);


  /* The button determines the dimensions of the axes.
     But the button is only shown on the primary sheet (sheet[0])
     So we must force the axes on the other sheets to
     the dimensions here. */

  GtkAllocation alloc;
  gtk_widget_get_allocation (SSW_SHEET_SINGLE(psheet->sheet[0])->button,
                             &alloc);

  gtk_widget_set_size_request (single->vertical_axis, alloc.width, -1);
  gtk_widget_set_size_request (single->horizontal_axis, -1, alloc.height);
}


GtkWidget *
ssw_sheet_single_new (SswSheet *psheet,
                      SswSheetAxis *haxis, SswSheetAxis *vaxis, SswRange *selection)
{
  GObject *sheet = g_object_new (SSW_TYPE_SHEET_SINGLE,
                                 "sheet", psheet,
                                 "horizontal-axis", haxis,
                                 "vertical-axis", vaxis,
                                 "selection", selection,
                                 NULL);
  return GTK_WIDGET (sheet);
}


enum
  {
   PROP_0,
   PROP_VAXIS,
   PROP_HAXIS,
   PROP_VADJUSTMENT,
   PROP_HADJUSTMENT,
   PROP_VSCROLL_POLICY,
   PROP_HSCROLL_POLICY,
   PROP_DATA_MODEL,
   PROP_SHEET,
   PROP_SELECTION
  };


static void
__arrange_properties (GObject *object)
{
  SswSheetSingle *sheet = SSW_SHEET_SINGLE (object);

  if (sheet->horizontal_axis)
    g_object_set (sheet->horizontal_axis, "adjustment", sheet->hadj, NULL);

  if (sheet->vertical_axis)
    g_object_set (sheet->vertical_axis, "adjustment", sheet->vadj, NULL);

  /* Only show the button if both axes belong to "us".
     Or to put it another way:  Don't show the button if either
     axis is "borrowed" from another sheet. */
  GtkWidget *vparent = sheet->vertical_axis ? gtk_widget_get_parent (sheet->vertical_axis) : NULL;
  GtkWidget *hparent = sheet->horizontal_axis ? gtk_widget_get_parent (sheet->horizontal_axis) : NULL;

  g_object_set (sheet->button,
                "no-show-all", (vparent != GTK_WIDGET (sheet)) || (hparent != GTK_WIDGET (sheet)),
                NULL);
}

static void
__set_property (GObject *object,
                guint prop_id, const GValue *value, GParamSpec *pspec)
{
  SswSheetSingle *sheet = SSW_SHEET_SINGLE (object);
  if (sheet->dispose_has_run)
    return;

  switch (prop_id)
    {
    case PROP_VAXIS:
      SSW_SHEET_SINGLE (object)->vertical_axis = g_value_get_object (value);
      g_object_set (sheet->body, "vertical-axis", sheet->vertical_axis, NULL);
      if (NULL == gtk_widget_get_parent (sheet->vertical_axis))
        gtk_grid_attach (GTK_GRID (sheet), sheet->vertical_axis, 0, 1, 1, 1);
      __arrange_properties (object);
      break;
    case PROP_HAXIS:
      SSW_SHEET_SINGLE (object)->horizontal_axis = g_value_get_object (value);
      g_object_set (sheet->body, "horizontal-axis", sheet->horizontal_axis, NULL);
      if (NULL == gtk_widget_get_parent (sheet->horizontal_axis))
        gtk_grid_attach (GTK_GRID (sheet), sheet->horizontal_axis, 1, 0, 1, 1);
      __arrange_properties (object);
      break;
    case PROP_DATA_MODEL:
      SSW_SHEET_SINGLE (object)->data_model = g_value_get_object (value);
      g_object_set (SSW_SHEET_SINGLE (object)->body, "data-model",
                    SSW_SHEET_SINGLE (object)->data_model, NULL);
      break;
    case PROP_HADJUSTMENT:
      g_set_object (&SSW_SHEET_SINGLE (object)->hadj, g_value_get_object (value));
      __arrange_properties (object);
      break;
    case PROP_VADJUSTMENT:
      g_set_object (&SSW_SHEET_SINGLE (object)->vadj, g_value_get_object (value));
      __arrange_properties (object);
      break;
    case PROP_VSCROLL_POLICY:
    case PROP_HSCROLL_POLICY:
      break;
    case PROP_SHEET:
      SSW_SHEET_SINGLE (object)->sheet = g_value_get_object (value);
      break;
    case PROP_SELECTION:
      g_object_set (sheet->body, "selection", g_value_get_pointer (value), NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
  if (sheet->body && SSW_SHEET_SINGLE (object)->sheet)
    g_object_set (sheet->body, "sheet", SSW_SHEET_SINGLE (object)->sheet, NULL);
}

static void
__get_property (GObject * object,
                guint prop_id, GValue * value, GParamSpec * pspec)
{
  switch (prop_id)
    {
    case PROP_VAXIS:
      g_value_set_object (value, SSW_SHEET_SINGLE (object)->vertical_axis);
      break;
    case PROP_HAXIS:
      g_value_set_object (value, SSW_SHEET_SINGLE (object)->horizontal_axis);
      break;
    case PROP_SHEET:
      g_value_set_object (value, SSW_SHEET_SINGLE (object)->sheet);
      break;
    case PROP_VSCROLL_POLICY:
    case PROP_HSCROLL_POLICY:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
__dispose (GObject *object)
{
  SswSheetSingle *sheet = SSW_SHEET_SINGLE (object);

  if (sheet->dispose_has_run)
    return;

  sheet->dispose_has_run = TRUE;

  if (SSW_SHEET_SINGLE (object)->hadj)
    g_object_unref (SSW_SHEET_SINGLE (object)->hadj);

  if (SSW_SHEET_SINGLE (object)->vadj)
    g_object_unref (SSW_SHEET_SINGLE (object)->vadj);

  G_OBJECT_CLASS (ssw_sheet_single_parent_class)->dispose (object);
}

static void
ssw_sheet_single_class_init (SswSheetSingleClass * class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  GParamSpec *sheet_spec =
    g_param_spec_object ("sheet",
                         P_("Sheet"),
                         P_("The Parent Sheet"),
                         SSW_TYPE_SHEET,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);


  GParamSpec *haxis_spec =
    g_param_spec_object ("horizontal-axis",
                         P_("Horizontal Axis"),
                         P_("The Horizontal Axis"),
                         SSW_TYPE_SHEET_AXIS,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  GParamSpec *vaxis_spec =
    g_param_spec_object ("vertical-axis",
                         P_("Vertical Axis"),
                         P_("The Vertical Axis"),
                         SSW_TYPE_SHEET_AXIS,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  GParamSpec *data_model_spec =
    g_param_spec_object ("data-model",
                         P_("Data Model"),
                         P_("The model describing the contents of the data"),
                         GTK_TYPE_TREE_MODEL,
                         G_PARAM_READWRITE);

  GParamSpec *selection_spec =
    g_param_spec_pointer ("selection",
                          P_("The selection"),
                          P_("A pointer to the current selection"),
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  object_class->dispose = __dispose;
  widget_class->realize = __realize;

  object_class->set_property = __set_property;
  object_class->get_property = __get_property;

  g_object_class_install_property (object_class,
                                   PROP_SELECTION,
                                   selection_spec);

  g_object_class_install_property (object_class,
                                   PROP_VAXIS,
                                   vaxis_spec);

  g_object_class_install_property (object_class,
                                   PROP_HAXIS,
                                   haxis_spec);


  g_object_class_install_property (object_class,
                                   PROP_DATA_MODEL,
                                   data_model_spec);

  g_object_class_install_property (object_class,
                                   PROP_SHEET,
                                   sheet_spec);

  g_object_class_override_property (object_class,
                                    PROP_VADJUSTMENT,
                                    "vadjustment");

  g_object_class_override_property (object_class,
                                    PROP_HADJUSTMENT,
                                    "hadjustment");


  g_object_class_override_property (object_class,
                                    PROP_HSCROLL_POLICY,
                                    "hscroll-policy");

  g_object_class_override_property (object_class,
                                    PROP_VSCROLL_POLICY,
                                    "vscroll-policy");
}

static void
ssw_sheet_single_init (SswSheetSingle *sheet)
{
  gtk_widget_set_has_window (GTK_WIDGET (sheet), FALSE);

  sheet->vadj = NULL;
  sheet->hadj = NULL;
  sheet->horizontal_axis = NULL;
  sheet->vertical_axis = NULL;
  sheet->dispose_has_run = FALSE;

  sheet->button = gtk_button_new_with_label ("");
  gtk_grid_attach (GTK_GRID (sheet), sheet->button, 0, 0, 1, 1);

  sheet->body = ssw_sheet_body_new (NULL);
  gtk_grid_attach (GTK_GRID (sheet), sheet->body,
                   1, 1, 1, 1);
}
