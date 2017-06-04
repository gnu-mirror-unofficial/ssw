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

#include <config.h>
#include "ssw-sheet.h"

#include "ssw-sheet-single.h"
#include "ssw-sheet-axis.h"
#include "ssw-axis-model.h"
#include "ssw-sheet-body.h"
#include "ssw-marshaller.h"
#include "ssw-xpaned.h"

#define P_(X) (X)

G_DEFINE_TYPE (SswSheet, ssw_sheet, GTK_TYPE_BIN)

#define DIM 2

enum  {ROW_HEADER_CLICKED,
       ROW_HEADER_DOUBLE_CLICKED,
       COLUMN_HEADER_CLICKED,
       COLUMN_HEADER_DOUBLE_CLICKED,
       ROW_HEADER_PRESSED,
       ROW_HEADER_RELEASED,
       COLUMN_HEADER_PRESSED,
       COLUMN_HEADER_RELEASED,
       SELECTION_CHANGED,
       VALUE_CHANGED,
       ROW_MOVED,
       COLUMN_MOVED,
       n_SIGNALS};

static guint signals [n_SIGNALS];

GtkWidget *
ssw_sheet_new (void)
{
  GObject *object = g_object_new (SSW_TYPE_SHEET, NULL);

  return GTK_WIDGET (object);
}

GtkWidget *
ssw_sheet_get_button (SswSheet *s)
{
  return SSW_SHEET_SINGLE (s->sheet[0])->button;
}


enum
  {
    PROP_0,
    PROP_SPLITTER,
    PROP_VMODEL,
    PROP_HMODEL,
    PROP_DATA_MODEL,
    PROP_SPLIT,
    PROP_GRIDLINES,
    PROP_EDITABLE,
    PROP_RENDERER_FUNC,
    PROP_RENDERER_FUNC_DATUM,
    PROP_CONVERT_FWD_FUNC,
    PROP_CONVERT_REV_FUNC
  };

static void
on_header_button_pressed (SswSheetAxis *axis, gint id, guint button, guint state,
			  gpointer user_data)
{
  SswSheet *sheet = SSW_SHEET (user_data);

  GtkOrientation o = gtk_orientable_get_orientation (GTK_ORIENTABLE (axis));

  if (o == GTK_ORIENTATION_HORIZONTAL)
    g_signal_emit (sheet, signals [COLUMN_HEADER_PRESSED], 0, id, button, state);
  else
    g_signal_emit (sheet, signals [ROW_HEADER_PRESSED], 0, id, button, state);
}


static void
on_header_button_released (SswSheetAxis *axis, gint id, guint button, guint state,
			   gpointer user_data)
{
  SswSheet *sheet = SSW_SHEET (user_data);

  GtkOrientation o = gtk_orientable_get_orientation (GTK_ORIENTABLE (axis));

  if (o == GTK_ORIENTATION_HORIZONTAL)
    g_signal_emit (sheet, signals [COLUMN_HEADER_RELEASED], 0, id, button, state);
  else
    g_signal_emit (sheet, signals [ROW_HEADER_RELEASED], 0, id, button, state);
}


static void
on_header_clicked (SswSheetAxis *axis, gint which, guint state, gpointer user_data)
{
  SswSheet *sheet = SSW_SHEET (user_data);

  GtkOrientation o = gtk_orientable_get_orientation (GTK_ORIENTABLE (axis));

  if (o == GTK_ORIENTATION_HORIZONTAL)
    g_signal_emit (sheet, signals [COLUMN_HEADER_CLICKED], 0, which);
  else
    g_signal_emit (sheet, signals [ROW_HEADER_CLICKED], 0, which);
}

static void
on_header_double_clicked (SswSheetAxis *axis, gint which, guint state, gpointer user_data)
{
  SswSheet *sheet = SSW_SHEET (user_data);

  GtkOrientation o = gtk_orientable_get_orientation (GTK_ORIENTABLE (axis));

  if (o == GTK_ORIENTATION_HORIZONTAL)
    g_signal_emit (sheet, signals [COLUMN_HEADER_DOUBLE_CLICKED], 0, which);
  else
    g_signal_emit (sheet, signals [ROW_HEADER_DOUBLE_CLICKED], 0, which);
}


static void
arrange (SswSheet *sheet)
{
  gint i;

  for (i = 0; i < DIM; ++i)
    {
      if (sheet->vmodel)
	ssw_sheet_axis_set_model (SSW_SHEET_AXIS (sheet->vertical_axis[i]),
				  sheet->vmodel);

      if (sheet->hmodel)
	ssw_sheet_axis_set_model (SSW_SHEET_AXIS (sheet->horizontal_axis[i]),
				  sheet->hmodel);
    }
}

static void
__finalize (GObject *obj)
{
  SswSheet *sheet = SSW_SHEET (obj);

  g_free (sheet->selection);

  G_OBJECT_CLASS (ssw_sheet_parent_class)->finalize (obj);
}

static void
__dispose (GObject *object)
{
  SswSheet *sheet = SSW_SHEET (object);

  if (sheet->dispose_has_run)
    return;

  g_object_unref (sheet->vmodel);
  g_object_unref (sheet->hmodel);

  sheet->dispose_has_run = TRUE;

  G_OBJECT_CLASS (ssw_sheet_parent_class)->dispose (object);
}


static void
resize_vmodel (GtkTreeModel *tm, guint posn, guint rm, guint add, SswSheet *sheet)
{
  gint rows = gtk_tree_model_iter_n_children (tm, NULL);

  g_object_set (sheet->vmodel, "size", rows, NULL);
}

static void
resize_hmodel (GtkTreeModel *tm, guint posn, guint rm, guint add, SswSheet *sheet)
{
  gint cols = gtk_tree_model_get_n_columns (tm);

  g_object_set (sheet->hmodel, "size", cols, NULL);
}



static void
__set_property (GObject *object,
                guint prop_id, const GValue *value, GParamSpec *pspec)
{
  gint i;
  SswSheet *sheet = SSW_SHEET (object);

  switch (prop_id)
    {
    case PROP_SPLITTER:
      {
	GType t = g_value_get_gtype (value);

	if (t == GTK_TYPE_CONTAINER)
	  t = SSW_TYPE_XPANED;

	GtkWidget *splitter = g_object_new (t, NULL);
	gtk_container_add (GTK_CONTAINER (sheet), splitter);

	for (i = 0; i < DIM * DIM ; ++i)
	  {
	    gtk_container_add_with_properties (GTK_CONTAINER (splitter),
					       sheet->sw[i],
					       "left-attach", i%DIM,
					       "top-attach", i/DIM,
					       NULL);
	  }
      }
      break;
    case PROP_GRIDLINES:
      {
	gboolean lines = g_value_get_boolean (value);

	for (i = 0; i < DIM * DIM ; ++i)
	  {
	    g_object_set (SSW_SHEET_SINGLE (sheet->sheet[i])->body, "gridlines", lines, NULL);
	  }
	sheet->gridlines = lines;
      }
      break;
    case PROP_EDITABLE:
      {
	gboolean editable = g_value_get_boolean (value);

	for (i = 0; i < DIM * DIM ; ++i)
	  {
	    g_object_set (SSW_SHEET_SINGLE (sheet->sheet[i])->body, "editable", editable, NULL);
	  }
	sheet->editable = editable;
      }
      break;

    case PROP_CONVERT_FWD_FUNC:
      {
	gpointer p = g_value_get_pointer (value);

	if (p)
	  for (i = 0; i < DIM * DIM ; ++i)
	    {
	      g_object_set (SSW_SHEET_SINGLE (sheet->sheet[i])->body,
			    "forward-conversion", p,
			    NULL);
	    }
      }
      break;

      case PROP_CONVERT_REV_FUNC:
      {
	gpointer p = g_value_get_pointer (value);

	if (p)
	  for (i = 0; i < DIM * DIM ; ++i)
	    {
	      g_object_set (SSW_SHEET_SINGLE (sheet->sheet[i])->body,
			    "reverse-conversion", p,
			    NULL);
	    }
      }
      break;

      case PROP_RENDERER_FUNC:
      {
	gpointer select_renderer_func = g_value_get_pointer (value);

	for (i = 0; i < DIM * DIM ; ++i)
	  {
	    g_object_set (SSW_SHEET_SINGLE (sheet->sheet[i])->body,
			  "select-renderer-func", select_renderer_func,
			  NULL);
	  }
	//	sheet->select_renderer_func = select_renderer_func;
      }
      break;

    case PROP_RENDERER_FUNC_DATUM:
      sheet->renderer_func_datum = g_value_get_pointer (value);
      break;

    case PROP_SPLIT:
      {
	gboolean split = g_value_get_boolean (value);

	for (i = 0; i < DIM * DIM ; ++i)
	  {
	    gtk_widget_hide (sheet->sw[i]);
	    g_object_set (sheet->sw[i], "no-show-all", TRUE, NULL);
	  }

	const int dimens = (split ? 2 : 1);
	for (i = 0; i < dimens * dimens; ++i)
	  {
	    g_object_set (sheet->sw[i],  "vscrollbar-policy",
			  ((i%dimens) == dimens - 1) ? GTK_POLICY_ALWAYS : GTK_POLICY_NEVER,
			  NULL);

	    g_object_set (sheet->sw[i], "hscrollbar-policy",
			  ((i/dimens) == dimens - 1) ? GTK_POLICY_ALWAYS : GTK_POLICY_NEVER,
			  NULL);

	    g_object_set (sheet->sw[i], "no-show-all", FALSE, NULL);
	    gtk_widget_show_all (GTK_WIDGET (sheet->sw[i]));
	  }

	sheet->split = split;
      }
      break;
    case PROP_VMODEL:
      if (sheet->vmodel)
	g_object_unref (sheet->vmodel);
      sheet->vmodel = g_value_dup_object (value);
      arrange (sheet);
      break;
    case PROP_HMODEL:
      if (sheet->hmodel)
	g_object_unref (sheet->hmodel);
      sheet->hmodel = g_value_dup_object (value);
      arrange (sheet);
      break;
    case PROP_DATA_MODEL:
      sheet->data_model = g_value_get_object (value);

      GtkTreeModelFlags flags =
	gtk_tree_model_get_flags (GTK_TREE_MODEL (sheet->data_model));

      if (!(flags & GTK_TREE_MODEL_LIST_ONLY))
	g_warning ("SswSheet can interpret list models only. Child nodes will be ignored.");

      if (sheet->vmodel == NULL)
	{
	  int n_rows = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (sheet->data_model), NULL);

	  sheet->vmodel = g_object_new (SSW_TYPE_AXIS_MODEL,
					"size", n_rows,
					NULL);

	  g_signal_connect_object (sheet->data_model, "items-changed",
				   G_CALLBACK (resize_vmodel), sheet, 0);

	  arrange (sheet);
	}

      if (sheet->hmodel == NULL)
	{
	  int n_cols = gtk_tree_model_get_n_columns (GTK_TREE_MODEL (sheet->data_model));

	  sheet->hmodel = g_object_new (SSW_TYPE_AXIS_MODEL,
					"size", n_cols,
					NULL);

	  g_signal_connect_object (sheet->data_model, "items-changed",
				   G_CALLBACK (resize_hmodel), sheet, 0);

	  arrange (sheet);
	}
      for (i = 0; i < DIM * DIM; ++i)
	{
	  g_object_set (sheet->sheet[i], "data-model", sheet->data_model, NULL);
	}
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
__get_property (GObject *object,
                guint prop_id, GValue *value, GParamSpec *pspec)
{
  switch (prop_id)
    {
    case PROP_DATA_MODEL:
      g_value_set_object (value, SSW_SHEET (object)->data_model);
      break;
    case PROP_VMODEL:
      g_value_set_object (value, SSW_SHEET (object)->vmodel);
      break;
    case PROP_HMODEL:
      g_value_set_object (value, SSW_SHEET (object)->hmodel);
      break;
    case PROP_SPLIT:
      g_value_set_boolean (value, SSW_SHEET (object)->split);
      break;
    case PROP_GRIDLINES:
      g_value_set_boolean (value, SSW_SHEET (object)->gridlines);
      break;
    case PROP_EDITABLE:
      g_value_set_boolean (value, SSW_SHEET (object)->gridlines);
      break;
    case PROP_RENDERER_FUNC_DATUM:
      g_value_set_pointer (value, SSW_SHEET (object)->renderer_func_datum);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
ssw_sheet_class_init (SswSheetClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  GParamSpec *convert_fwd_spec =
    g_param_spec_pointer ("forward-conversion",
			  P_("Forward conversion function"),
			  P_("A function to convert a cell datum to a string"),
			  G_PARAM_WRITABLE);

  GParamSpec *convert_rev_spec =
    g_param_spec_pointer ("reverse-conversion",
			  P_("Reverse conversion function"),
			  P_("A function to convert a string to a cell datum"),
			  G_PARAM_WRITABLE);

  GParamSpec *splitter_spec =
    g_param_spec_gtype ("splitter",
			P_("Splitter Container Type"),
			P_("The type of container widget to handle splits"),
			GTK_TYPE_CONTAINER,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  GParamSpec *vmodel_spec =
    g_param_spec_object ("vmodel",
			 P_("Vertical Model"),
			 P_("The model describing the rows"),
			 G_TYPE_LIST_MODEL,
			 G_PARAM_READWRITE);

  GParamSpec *hmodel_spec =
    g_param_spec_object ("hmodel",
			 P_("Horizontal Model"),
			 P_("The model describing the columns"),
			 G_TYPE_LIST_MODEL,
			 G_PARAM_READWRITE);

  GParamSpec *data_model_spec =
    g_param_spec_object ("data-model",
			 P_("Data Model"),
			 P_("The model describing the contents of the data"),
			 GTK_TYPE_TREE_MODEL,
			 G_PARAM_READWRITE);



  GParamSpec *split_spec =
    g_param_spec_boolean ("split",
			  P_("Split View"),
			  P_("If TRUE the sheet view is split four ways"),
			  FALSE,
			  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);


  GParamSpec *gridlines_spec =
    g_param_spec_boolean ("gridlines",
			  P_("Show Gridlines"),
			  P_("True if gridlines should be shown"),
			  TRUE,
			  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  GParamSpec *editable_spec =
    g_param_spec_boolean ("editable",
			  P_("Editable"),
			  P_("True if the sheet is editable"),
			  TRUE,
			  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  GParamSpec *renderer_func_spec =
    g_param_spec_pointer ("select-renderer-func",
			  P_("Select Renderer Function"),
			  P_("Function returning the renderer to use for a cell"),
			  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  GParamSpec *renderer_func_datum_spec =
    g_param_spec_pointer ("select-renderer-datum",
			  P_("Select Renderer Function Datum"),
			  P_("The Datum to be passed to the \"select-renderer-func\" property"),
			  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  object_class->set_property = __set_property;
  object_class->get_property = __get_property;
  object_class->dispose = __dispose;
  object_class->finalize = __finalize;

  g_object_class_install_property (object_class,
                                   PROP_CONVERT_FWD_FUNC,
                                   convert_fwd_spec);

  g_object_class_install_property (object_class,
                                   PROP_CONVERT_REV_FUNC,
                                   convert_rev_spec);

  g_object_class_install_property (object_class,
                                   PROP_RENDERER_FUNC,
                                   renderer_func_spec);

  g_object_class_install_property (object_class,
                                   PROP_RENDERER_FUNC_DATUM,
                                   renderer_func_datum_spec);

  g_object_class_install_property (object_class,
                                   PROP_SPLITTER,
                                   splitter_spec);

  g_object_class_install_property (object_class,
                                   PROP_VMODEL,
                                   vmodel_spec);

  g_object_class_install_property (object_class,
                                   PROP_HMODEL,
                                   hmodel_spec);

  g_object_class_install_property (object_class,
                                   PROP_DATA_MODEL,
                                   data_model_spec);

  g_object_class_install_property (object_class,
                                   PROP_SPLIT,
                                   split_spec);

  g_object_class_install_property (object_class,
                                   PROP_GRIDLINES,
                                   gridlines_spec);

  g_object_class_install_property (object_class,
                                   PROP_EDITABLE,
                                   editable_spec);


  signals [ROW_HEADER_CLICKED] =
    g_signal_new ("row-header-clicked",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_FIRST,
		  0,
		  NULL, NULL,
		  g_cclosure_marshal_VOID__INT,
		  G_TYPE_NONE,
		  1,
		  G_TYPE_INT);

  signals [ROW_HEADER_DOUBLE_CLICKED] =
    g_signal_new ("row-header-double-clicked",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_FIRST,
		  0,
		  NULL, NULL,
		  g_cclosure_marshal_VOID__INT,
		  G_TYPE_NONE,
		  1,
		  G_TYPE_INT);

  signals [COLUMN_HEADER_CLICKED] =
    g_signal_new ("column-header-clicked",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_FIRST,
		  0,
		  NULL, NULL,
		  g_cclosure_marshal_VOID__INT,
		  G_TYPE_NONE,
		  1,
		  G_TYPE_INT);

  signals [COLUMN_HEADER_DOUBLE_CLICKED] =
    g_signal_new ("column-header-double-clicked",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_FIRST,
		  0,
		  NULL, NULL,
		  g_cclosure_marshal_VOID__INT,
		  G_TYPE_NONE,
		  1,
		  G_TYPE_INT);


  signals [ROW_HEADER_PRESSED] =
    g_signal_new ("row-header-pressed",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_FIRST,
		  0,
		  NULL, NULL,
		  ssw_cclosure_marshal_VOID__INT_UINT_UINT,
		  G_TYPE_NONE,
		  3,
		  G_TYPE_INT,
		  G_TYPE_UINT,
		  G_TYPE_UINT);

  signals [ROW_HEADER_RELEASED] =
    g_signal_new ("row-header-released",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_FIRST,
		  0,
		  NULL, NULL,
		  ssw_cclosure_marshal_VOID__INT_UINT_UINT,
		  G_TYPE_NONE,
		  3,
		  G_TYPE_INT,
		  G_TYPE_UINT,
		  G_TYPE_UINT);

  signals [COLUMN_HEADER_PRESSED] =
    g_signal_new ("column-header-pressed",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_FIRST,
		  0,
		  NULL, NULL,
		  ssw_cclosure_marshal_VOID__INT_UINT_UINT,
		  G_TYPE_NONE,
		  3,
		  G_TYPE_INT,
		  G_TYPE_UINT,
		  G_TYPE_UINT);

  signals [COLUMN_HEADER_RELEASED] =
    g_signal_new ("column-header-released",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_FIRST,
		  0,
		  NULL, NULL,
		  ssw_cclosure_marshal_VOID__INT_UINT_UINT,
		  G_TYPE_NONE,
		  3,
		  G_TYPE_INT,
		  G_TYPE_UINT,
		  G_TYPE_UINT);


  signals [SELECTION_CHANGED] =
    g_signal_new ("selection-changed",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_FIRST,
		  0,
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE,
		  1,
		  G_TYPE_POINTER);


    signals [VALUE_CHANGED] =
    g_signal_new ("value-changed",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_FIRST,
		  0,
		  NULL, NULL,
		  ssw_cclosure_marshal_VOID__INT_INT_POINTER,
		  G_TYPE_NONE,
		  3,
		  G_TYPE_INT,
		  G_TYPE_INT,
		  G_TYPE_POINTER);

    signals [ROW_MOVED] =
      g_signal_new ("row-moved",
		    G_TYPE_FROM_CLASS (class),
		    G_SIGNAL_RUN_FIRST,
		    0,
		    NULL, NULL,
		    ssw_cclosure_marshal_VOID__INT_INT,
		    G_TYPE_NONE,
		    2,
		    G_TYPE_INT,
		    G_TYPE_INT);

    signals [COLUMN_MOVED] =
      g_signal_new ("column-moved",
		    G_TYPE_FROM_CLASS (class),
		    G_SIGNAL_RUN_FIRST,
		    0,
		    NULL, NULL,
		    ssw_cclosure_marshal_VOID__INT_INT,
		    G_TYPE_NONE,
		    2,
		    G_TYPE_INT,
		    G_TYPE_INT);
}

static void
forward_signal (SswSheet *sheet, ...)
{
  va_list ap;
  va_start (ap, sheet);
  g_signal_emit_valist (sheet, signals [VALUE_CHANGED], 0, ap);
  va_end (ap);
}

static void
forward_selection_signal (SswSheet *sheet, SswRange *sel, gpointer ud)
{
  gint i;

  for (i = 0; i < DIM * DIM ; ++i)
    {
      GtkWidget *body = SSW_SHEET_SINGLE (sheet->sheet[i])->body;
      if (body == ud)
	continue;

      gtk_widget_queue_draw (body);
    }
  g_signal_emit (sheet, signals [SELECTION_CHANGED], 0, sel);
}

static void
on_drag_n_drop (SswSheet *sheet, gint from, gint to, GtkOrientable *axis)
{
  if (to - from == 1 || to == from) /* This is a null move */
    return;

  if (gtk_orientable_get_orientation (axis) == GTK_ORIENTATION_HORIZONTAL)
    g_signal_emit (sheet, signals [COLUMN_MOVED], 0, from, to);
  else
    g_signal_emit (sheet, signals [ROW_MOVED], 0, from, to);
}

static void
ssw_sheet_init (SswSheet *sheet)
{
  gint i;
  for (i = 0; i < DIM; ++i)
    {
      sheet->vadj[i] = gtk_adjustment_new (0, 0, 0, 0, 0, 0);
      sheet->hadj[i] = gtk_adjustment_new (0, 0, 0, 0, 0, 0);

      sheet->horizontal_axis[i] =
	ssw_sheet_axis_new (GTK_ORIENTATION_HORIZONTAL);


      g_signal_connect (sheet->horizontal_axis[i], "header-clicked",
			G_CALLBACK (on_header_clicked), sheet);

      g_signal_connect (sheet->horizontal_axis[i], "header-double-clicked",
			G_CALLBACK (on_header_double_clicked), sheet);

      g_signal_connect (sheet->horizontal_axis[i], "header-button-pressed",
			G_CALLBACK (on_header_button_pressed), sheet);

      g_signal_connect (sheet->horizontal_axis[i], "header-button-released",
			G_CALLBACK (on_header_button_released), sheet);

      sheet->vertical_axis[i] =
	ssw_sheet_axis_new (GTK_ORIENTATION_VERTICAL);

      g_signal_connect (sheet->vertical_axis[i], "header-clicked",
			G_CALLBACK (on_header_clicked), sheet);

      g_signal_connect (sheet->vertical_axis[i], "header-double-clicked",
			G_CALLBACK (on_header_double_clicked), sheet);

      g_signal_connect (sheet->vertical_axis[i], "header-button-pressed",
			G_CALLBACK (on_header_button_pressed), sheet);

      g_signal_connect (sheet->vertical_axis[i], "header-button-released",
			G_CALLBACK (on_header_button_released), sheet);
    }

  sheet->selection = g_malloc (sizeof *sheet->selection);
  sheet->selection->start_x = -1;
  sheet->selection->start_y = -1;
  sheet->selection->end_x = -1;
  sheet->selection->end_y = -1;

  for (i = 0; i < DIM ; ++i)
    {
      g_signal_connect_swapped (sheet->horizontal_axis[i], "drag-n-dropped",
				G_CALLBACK (on_drag_n_drop), sheet);

      g_signal_connect_swapped (sheet->vertical_axis[i], "drag-n-dropped",
				G_CALLBACK (on_drag_n_drop), sheet);
    }

  for (i = 0; i < DIM * DIM ; ++i)
    {
      sheet->sw[i] = gtk_scrolled_window_new (sheet->hadj[i%DIM],
					      sheet->vadj[i/DIM]);

      g_object_set (sheet->sw[i], "shadow-type", GTK_SHADOW_IN, NULL);

      sheet->sheet[i] =
	ssw_sheet_single_new (sheet,
			      SSW_SHEET_AXIS (sheet->horizontal_axis[i%DIM]),
			      SSW_SHEET_AXIS (sheet->vertical_axis[i/DIM]),
			      sheet->selection);

      gtk_container_add (GTK_CONTAINER (sheet->sw[i]), sheet->sheet[i]);

      g_signal_connect_swapped (SSW_SHEET_SINGLE (sheet->sheet[i])->body,
				"selection-changed", G_CALLBACK (forward_selection_signal), sheet);

      g_signal_connect_swapped (SSW_SHEET_SINGLE (sheet->sheet[i])->body,
				"value-changed", G_CALLBACK (forward_signal), sheet);
    }

  sheet->renderer_func_datum = NULL;
  sheet->vmodel = NULL;
  sheet->hmodel = NULL;
  sheet->dispose_has_run = FALSE;
  sheet->selected_body = SSW_SHEET_SINGLE (sheet->sheet[0])->body;
}

void
ssw_sheet_set_clip (SswSheet *sheet, GtkClipboard *clip)
{
  if (!sheet->data_model)
    return;

  ssw_sheet_body_set_clip (SSW_SHEET_BODY (sheet->selected_body), clip);
}


void
ssw_sheet_scroll_to (SswSheet *sheet, gint hpos, gint vpos)
{
  if (hpos >= 0)
    ssw_sheet_axis_jump_center (SSW_SHEET_AXIS (sheet->horizontal_axis[0]), hpos);

  if (vpos >= 0)
    ssw_sheet_axis_jump_center (SSW_SHEET_AXIS (sheet->vertical_axis[0]), vpos);
}


void
ssw_sheet_set_active_cell (SswSheet *sheet,
			   gint col, gint row, GdkEvent *e)
{
  ssw_sheet_body_set_active_cell (SSW_SHEET_BODY (sheet->selected_body),
				  col, row, e);
}


gboolean
ssw_sheet_get_active_cell (SswSheet *sheet,
			   gint *col, gint *row)
{
  return ssw_sheet_body_get_active_cell (SSW_SHEET_BODY (sheet->selected_body),
				  col, row);
}



struct paste_state
{
  gint col0;
  gint row0;
  gint col;
  gint row;
  SswSheet *sheet;
  ssw_sheet_set_cell set_cell;
};

static void
paste_datum (const gchar *x, size_t len, struct paste_state *t)
{
  SswSheet *sheet = t->sheet;

  GValue value = G_VALUE_INIT;

  gint col = t->col + t->col0;
  gint row = t->row + t->row0;

  ssw_sheet_reverse_conversion_func rcf;

  g_object_get (SSW_SHEET_SINGLE (sheet->sheet[0])->body,
		"reverse-conversion", &rcf,
		NULL);

  if (rcf (sheet->data_model, col, row, x, &value))
    t->set_cell (sheet->data_model, col, row, &value);
  g_value_unset (&value);

  t->col ++;
}

static void
end_of_row (struct paste_state *t)
{
  t->row++;
  t->col = 0;
}

static void
end_of_paste (struct paste_state *t)
{
  g_free (t);
}


static void
parse_delimited_data (const gchar *data, int len, const char *delim,
		      void (*payload)(const gchar *, size_t, struct paste_state *),
		      void (*endload)(struct paste_state *),
		      struct paste_state * dw)
{
  while (len > 0)
    {
      const gchar *x = g_strstr_len (data, len, delim);
      if (x == NULL)
	{
	  char *f = g_strndup (data, len);
	  payload (f, len, dw);
	  g_free (f);
	  break;
	}
      len -= x - data + 1;

      gchar *line = g_strndup (data, x - data);
      payload (line, x - data, dw);
      data = x + 1;
      g_free (line);
    }
  if (endload)
    endload (dw);
}

static void
parseit (const gchar *x, size_t len, struct paste_state *dw)
{
  parse_delimited_data (x, len, "\t", paste_datum, end_of_row, dw);
}

static void
utf8_tab_delimited_parse (GtkClipboard *clip, GtkSelectionData *sd,
			  gpointer user_data)
{
  struct paste_state *ps = user_data;
  SswSheet *sheet = SSW_SHEET (ps->sheet);
  const gchar *data = gtk_selection_data_get_data (sd);
  gint len = gtk_selection_data_get_length (sd);

  if (len < 0)
    {
      g_free (ps);
      return;
    }

  ps->row = 0;
  ps->col = 0;

  parse_delimited_data (data, len, "\n", parseit, end_of_paste, ps);
  gtk_widget_queue_draw (GTK_WIDGET (sheet));
}

static void
target_marshaller (GtkClipboard *clip, GdkAtom *atoms, gint n_atoms,
		   gpointer user_data)
{
  int i;
  struct paste_state *ps = user_data;

  if (atoms == NULL)
    {
      g_free (ps);
      return;
    }

  for (i = 0; i < n_atoms; ++i)
    {
      /* Right now this is the only target we support */
      if (atoms[i] == gdk_atom_intern ("UTF8_STRING", TRUE))
	{
	  gtk_clipboard_request_contents (clip, atoms[i], utf8_tab_delimited_parse,
					  ps);
	  break;
	}
    }

  if (i == n_atoms)
    g_free (ps);
}

void
ssw_sheet_paste (SswSheet *sheet, GtkClipboard *clip, ssw_sheet_set_cell sc)
{
  gint col, row;
  if (ssw_sheet_get_active_cell (sheet, &col, &row))
    {
      struct paste_state *ps = g_malloc (sizeof *ps);
      ps->sheet = sheet;
      ps->set_cell = sc;

      ps->col0 = col;
      ps->row0 = row;

      gtk_clipboard_request_targets (clip, target_marshaller, ps);
    }
}
