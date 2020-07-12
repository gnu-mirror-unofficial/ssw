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

#include <config.h>
#include <math.h>
#include <string.h>
#include "ssw-sheet.h"
#include "ssw-sheet-body.h"
#include "ssw-constraint.h"
#include "ssw-marshaller.h"

#define P_(X) (X)

typedef GtkCellRenderer * (* fetch_renderer_func) (SswSheet *sheet, gint col, gint row, GType type, gpointer ud);

static const int linewidth = 1;

struct _SswSheetBodyPrivate
{
  SswSheetAxis *vaxis;
  SswSheetAxis *haxis;

  /* The widget which is editing the active cell,
     or null if no cell is being edited */
  GtkWidget *editor;
  GtkWidget *active_cell_holder;

  /* A string of the form "%d:%d" describing the current cell */
  gchar path[512];

  gboolean show_gridlines;
  gboolean editable;

  GtkTreeModel *data_model;

  /* Cursors used when resizing columns and rows respectively */
  GdkCursor *vc;
  GdkCursor *hc;

  /* Cursor used when extending/defining a cell selection */
  GdkCursor *xc;

  /* Gestures used when selecting / resizing */
  GtkGesture *selection_gesture;
  GtkGesture *horizontal_resize_gesture;
  GtkGesture *vertical_resize_gesture;

  gboolean dispose_has_run;

  fetch_renderer_func renderer_func;

  GtkCellRenderer *default_renderer;

  SswRange *selection;

  /* The sheet to which this body belongs */
  SswSheet *sheet;

  ssw_sheet_forward_conversion_func cf;
  ssw_sheet_reverse_conversion_func revf;
};

typedef struct _SswSheetBodyPrivate SswSheetBodyPrivate;

G_DEFINE_TYPE_WITH_CODE (SswSheetBody, ssw_sheet_body,
                         GTK_TYPE_LAYOUT,
                         G_ADD_PRIVATE (SswSheetBody));

#define PRIV_DECL(l) SswSheetBodyPrivate *priv = ((SswSheetBodyPrivate *)ssw_sheet_body_get_instance_private ((SswSheetBody *)l))
#define PRIV(l) ((SswSheetBodyPrivate *)ssw_sheet_body_get_instance_private ((SswSheetBody *)l))


static void start_editing (SswSheetBody *body, GdkEvent *e);

static void
limit_selection (SswSheetBody *body)
{
  PRIV_DECL (body);

  if (priv->selection->end_x < 0)
    priv->selection->end_x = 0;

  if (priv->selection->end_y < 0)
    priv->selection->end_y = 0;

  if (priv->selection->end_x >= ssw_sheet_axis_get_size (priv->haxis))
    priv->selection->end_x = ssw_sheet_axis_get_size (priv->haxis) - 1;

  if (priv->selection->end_y >= ssw_sheet_axis_get_size (priv->vaxis))
    priv->selection->end_y = ssw_sheet_axis_get_size (priv->vaxis) - 1;
}

enum  {SELECTION_CHANGED,
       VALUE_CHANGED,
       n_SIGNALS};

static guint signals [n_SIGNALS];

static inline gboolean
get_active_cell (SswSheetBody *body, gint *col, gint *row)
{
  PRIV_DECL (body);

  SswSheetBody *active_body = NULL;

  sscanf (priv->path, "r%dc%ds%p", row, col, &active_body);

  if (active_body == NULL) /* There is no active cell! */
    return FALSE;

  if (active_body != body) /* This body is not active */
    return FALSE;

  if (*col < 0 || *row < 0)
    return FALSE;

  return TRUE;
}

static inline
void set_active_cell (SswSheetBody *body, gint col, gint row)
{
  PRIV_DECL (body);

  gint old_start_x = priv->selection->start_x;
  gint old_start_y = priv->selection->start_y;

  priv->selection->start_x = col;
  priv->selection->start_y = row;

  if (old_start_y < row)
    priv->selection->end_y = row;

  if (old_start_x < col)
    priv->selection->end_x = col;

  snprintf (priv->path, 512, "r%dc%ds%p", row, col, body);
}

static void
scroll_vertically (SswSheetBody *body)
{
  PRIV_DECL (body);

  if (!gtk_widget_is_visible (GTK_WIDGET (body)))
    return;

  gint row = -1, col = -1;
  get_active_cell (body, &col, &row);

  if (row >= 0 && col >= 0)
    {
      gint vlocation, hlocation;
      gint visible = ssw_sheet_axis_find_boundary (priv->vaxis, row, &vlocation, NULL);

      gtk_container_child_get (GTK_CONTAINER (body),
                               priv->active_cell_holder, "x", &hlocation, NULL);

      gtk_layout_move (GTK_LAYOUT (body), priv->active_cell_holder,
                       hlocation, vlocation + 1);

      gtk_widget_set_visible (priv->active_cell_holder, (visible == 0));
    }

  gtk_widget_queue_draw (GTK_WIDGET (body));
}

static void
scroll_horizontally (SswSheetBody *body)
{
  PRIV_DECL (body);

  if (!gtk_widget_is_visible (GTK_WIDGET (body)))
    return;

  gint row = -1, col = -1;
  get_active_cell (body, &col, &row);

  if (row >= 0 && col >= 0)
    {
      gint vlocation, hlocation;
      gint visible = ssw_sheet_axis_find_boundary (priv->haxis, col, &hlocation, NULL);

      gtk_container_child_get (GTK_CONTAINER (body), priv->active_cell_holder, "y", &vlocation, NULL);

      gtk_layout_move (GTK_LAYOUT (body), priv->active_cell_holder, hlocation + 1, vlocation);
      gtk_widget_set_visible (priv->active_cell_holder, visible == 0);
    }

  gtk_widget_queue_draw (GTK_WIDGET (body));
}




static void
normalise_selection (const SswRange *in, SswRange *out)
{
  if (in->start_y < in->end_y)
    {
      out->end_y = in->end_y;
      out->start_y = in->start_y;
    }
  else
    {
      out->start_y = in->end_y;
      out->end_y = in->start_y;
    }

  if (in->start_x < in->end_x)
    {
      out->end_x = in->end_x;
      out->start_x = in->start_x;
    }
  else
    {
      out->start_x = in->end_x;
      out->end_x = in->start_x;
    }
}

static void
draw_selection (SswSheetBody *body, cairo_t *cr)
{
  GtkWidget *w = GTK_WIDGET (body);
  guint width = gtk_widget_get_allocated_width (w);
  guint height = gtk_widget_get_allocated_height (w);
  PRIV_DECL (body);

  GdkRGBA *color;
  GtkStyleContext *sc = gtk_widget_get_style_context (w);
  gtk_style_context_get (sc,
                         GTK_STATE_FLAG_SELECTED,
                         "background-color",
                         &color, NULL);

  GtkCssProvider *cp = gtk_css_provider_new ();
  gchar *css = g_strdup_printf ("* {background-color: rgba(%d, %d, %d, 0.25);}",
                                (int) (100.0 * color->red),
                                (int) (100.0 * color->green),
                                (int) (100.0 * color->blue));

  gtk_css_provider_load_from_data (cp, css, strlen (css), 0);

  gtk_style_context_add_provider (sc, GTK_STYLE_PROVIDER (cp),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  /* Draw the selection */
  if ((priv->selection->start_x != priv->selection->end_x)
      ||
      (priv->selection->start_y != priv->selection->end_y))
    {
      SswRange mySelect;
      normalise_selection (priv->selection, &mySelect);
      gint xpos_start = 0;
      gint ypos_start = 0;
      if (0 < ssw_sheet_axis_find_boundary (priv->haxis, mySelect.start_x,
                                            &xpos_start,  NULL))
        goto done;

      if (0 < ssw_sheet_axis_find_boundary (priv->vaxis, mySelect.start_y,
                                            &ypos_start,  NULL))
        goto done;

      gint xpos_end, ypos_end, yextent, xextent;
      gint xrel = ssw_sheet_axis_find_boundary (priv->haxis,
                                                mySelect.end_x, &xpos_end,
                                                &xextent);
      if (xrel < 0)
        goto done;

      gint xsize = (xrel == 0) ? xpos_end - xpos_start + xextent : width;


      gint yrel = ssw_sheet_axis_find_boundary (priv->vaxis,
                                                mySelect.end_y, &ypos_end,
                                                &yextent);
      if (yrel < 0)
        goto done;

      gint ysize = (yrel == 0) ? ypos_end - ypos_start + yextent : height;

      gtk_render_background (sc, cr,
                             xpos_start, ypos_start,
                             xsize, ysize);
    }

 done:
  gtk_style_context_remove_provider (sc, GTK_STYLE_PROVIDER (cp));
  g_free (css);
  g_object_unref (cp);
  gdk_rgba_free (color);
}

gboolean
ssw_sheet_default_reverse_conversion (GtkTreeModel *model, gint col, gint row,
                                      const gchar *in, GValue *out)
{
  GType src_type = G_TYPE_STRING;
  GType target_type = gtk_tree_model_get_column_type (model, col);

  GValue src_value = G_VALUE_INIT;

  if (! g_value_type_transformable (src_type, target_type))
    {
      g_critical ("Value of type %s cannot be transformed to type %s",
                  g_type_name (src_type),
                  g_type_name (target_type));
      return FALSE;
    }

  g_value_init (&src_value, src_type);
  g_value_set_string (&src_value, in);

  g_value_init (out, target_type);

  g_value_transform (&src_value, out);
  g_value_unset (&src_value);
  return TRUE;
}


gchar *
ssw_sheet_default_forward_conversion (SswSheet *sheet, GtkTreeModel *m,
                                      gint col, gint row, const GValue *value)
{
  GValue target_value = G_VALUE_INIT;

  g_value_init (&target_value, G_TYPE_STRING);

  if (!g_value_transform (value, &target_value))
    {
      if (G_VALUE_HOLDS_VARIANT (value))
        {
          GVariant *vt = g_value_get_variant (value);
          gchar *s = g_variant_print (vt, FALSE);
          g_value_unset (&target_value);
          return s;
        }
      else
        {
          g_warning ("Failed to transform type \"%s\" to type \"%s\"\n",
                     G_VALUE_TYPE_NAME (&value),
                     g_type_name (G_TYPE_STRING));
        }
    }

  gchar *cell_text = g_value_dup_string (&target_value);

  g_value_unset (&target_value);

  return cell_text;
}


static GtkCellRenderer * choose_renderer (SswSheetBody *body, gint col, gint row);

static const gchar *unfocused_border = "* {\n"
  " border-width: 2px;\n"
  " border-radius: 2px;\n"
  " border-color: black;\n"
  " border-style: dotted;\n"
  "}";


static const gchar *focused_border = "* {\n"
  " border-width: 2px;\n"
  " border-radius: 2px;\n"
  " border-color: black;\n"
  " border-style: solid;\n"
  "}";

static gboolean
__draw (GtkWidget *widget, cairo_t *cr)
{
  SswSheetBody *body = SSW_SHEET_BODY (widget);
  PRIV_DECL (body);

  if (!priv->haxis || !priv->vaxis)
    return TRUE;

  gint active_row = -1, active_col = -1;
  get_active_cell (body, &active_col, &active_row);

  guint width = gtk_widget_get_allocated_width (widget);
  guint height = gtk_widget_get_allocated_height (widget);

  GtkStyleContext *sc = gtk_widget_get_style_context (widget);

  GtkCssProvider *cp = gtk_css_provider_new ();

  GtkBorder border;
  if (priv->editable)
    {
      gint yy = ssw_sheet_axis_find_boundary (priv->vaxis, active_row, NULL, NULL);
      gint xx = ssw_sheet_axis_find_boundary (priv->haxis, active_col, NULL, NULL);

      if (yy == 0 && xx == 0 && priv->editor == NULL)
        start_editing (body, NULL);

      if ((gtk_widget_is_focus (widget) ||
           (priv->editor && gtk_widget_is_focus (priv->editor))))
        gtk_css_provider_load_from_data (cp, focused_border, strlen (focused_border), 0);
      else
        gtk_css_provider_load_from_data (cp, unfocused_border,
                                         strlen (unfocused_border), 0);

      gtk_style_context_add_provider (sc, GTK_STYLE_PROVIDER (cp),
                                      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

      gtk_style_context_get_border (sc,
                                    gtk_widget_get_state_flags (widget),
                                    &border);
    }

  int row = priv->vaxis->last_cell;
  gint y;
  for (y = priv->vaxis->cell_limits->len - 1;
       y >= 0;
       --y)
    {
      const SswGeometry *vgeom = g_ptr_array_index (priv->vaxis->cell_limits, y);


      if (priv->show_gridlines)
        {
          gint ypos = vgeom->position;
          ypos += vgeom->size;

          /* Draw horizontal grid lines */
          gtk_render_line (sc, cr, 0, ypos, width, ypos);
        }

      --row;
      GtkTreeIter iter;
      if (priv->data_model)
        gtk_tree_model_iter_nth_child (priv->data_model, &iter, NULL, row);
      int col = priv->haxis->last_cell;
      gint x;
      for (x = priv->haxis->cell_limits->len - 1;
           x >= 0;
           --x)
        {
          const SswGeometry *hgeom = g_ptr_array_index (priv->haxis->cell_limits, x);

          GdkRectangle rect;
          rect.x = hgeom->position;
          rect.y = vgeom->position;
          rect.width = hgeom->size;
          rect.height = vgeom->size;

          --col;

          if (priv->editable && col == active_col && row == active_row)
            {
              /* Draw frame */
              gtk_render_frame (sc, cr,
                                rect.x - border.left,
                                rect.y - border.top,
                                rect.width + border.left + border.right + 1,
                                rect.height + border.top + border.bottom + 1);
            }
          if (priv->data_model
              && row < gtk_tree_model_iter_n_children (priv->data_model, NULL)
              && col < gtk_tree_model_get_n_columns (priv->data_model))
            {
              GtkCellRenderer *renderer = choose_renderer (body, col, row);

              if (GTK_IS_CELL_RENDERER_TEXT (renderer))
                {
                  char *cell_text = NULL;

                  if (col != active_col || row != active_row ||
                      ! priv->editable ||
                      (priv->sheet->selected_body != GTK_WIDGET (body)))
                    {
                      /* Don't render the active cell.
                         It is already rendered by the cell_editable widget
                         and rendering twice looks unaesthetic */
                      GValue value = G_VALUE_INIT;
                      gtk_tree_model_get_value (priv->data_model, &iter, col, &value);
                      cell_text = priv->cf (priv->sheet, priv->data_model, col, row, &value);
                      g_value_unset (&value);
                    }

                  g_object_set (renderer,
                                "text", cell_text,
                                NULL);

                  g_free (cell_text);
                }


              gtk_cell_renderer_render (renderer, cr, widget,
                                        &rect, &rect, 0);
            }

          if (priv->show_gridlines)
            {
              /* Draw vertical grid lines */
              gtk_render_line (sc, cr,
                               rect.x + rect.width, 0,
                               rect.x + rect.width, height);
            }
        }
    }

  if (gtk_gesture_is_active (priv->horizontal_resize_gesture))
    {
      gdouble start_x, start_y;
      gdouble offsetx, offsety;
      gtk_gesture_drag_get_start_point (GTK_GESTURE_DRAG (priv->horizontal_resize_gesture), &start_x, &start_y);
      gtk_gesture_drag_get_offset (GTK_GESTURE_DRAG (priv->horizontal_resize_gesture), &offsetx, &offsety);

      gtk_render_line (sc, cr, start_x + offsetx, 0, start_x + offsetx, height);
    }

  if (gtk_gesture_is_active (priv->vertical_resize_gesture))
    {
      gdouble start_x, start_y;
      gdouble offsetx, offsety;
      gtk_gesture_drag_get_start_point (GTK_GESTURE_DRAG (priv->vertical_resize_gesture), &start_x, &start_y);
      gtk_gesture_drag_get_offset (GTK_GESTURE_DRAG (priv->vertical_resize_gesture), &offsetx, &offsety);

      gtk_render_line (sc, cr, 0, start_y + offsety, width, start_y + offsety);
    }

  draw_selection (body, cr);

  gtk_style_context_remove_provider (sc, GTK_STYLE_PROVIDER (cp));

  g_object_unref (cp);

  return GTK_WIDGET_CLASS (ssw_sheet_body_parent_class)->draw (widget, cr);
}


static void
__realize (GtkWidget *w)
{
  GTK_WIDGET_CLASS (ssw_sheet_body_parent_class)->realize (w);

  GdkWindow *win = gtk_widget_get_window (w);
  gdk_window_set_events (win, GDK_KEY_PRESS_MASK |
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
}

static void
__dispose (GObject *object)
{
  SswSheetBody *body = SSW_SHEET_BODY (object);
  PRIV_DECL (body);

  if (priv->dispose_has_run)
    return;

  if (priv->data_model)
    g_object_unref (priv->data_model);

  priv->dispose_has_run = TRUE;

  G_OBJECT_CLASS (ssw_sheet_body_parent_class)->dispose (object);
}


static void
__finalize (GObject *obj)
{
  SswSheetBody *body = SSW_SHEET_BODY (obj);
  PRIV_DECL (body);

  g_object_unref (priv->vc);
  g_object_unref (priv->hc);
  g_object_unref (priv->xc);

  g_object_unref (priv->horizontal_resize_gesture);
  g_object_unref (priv->vertical_resize_gesture);
  g_object_unref (priv->selection_gesture);
  g_object_unref (priv->default_renderer);

  G_OBJECT_CLASS (ssw_sheet_body_parent_class)->finalize (obj);
}



static void
announce_selection (SswSheetBody *body)
{
  gtk_widget_queue_draw (GTK_WIDGET (body));

  PRIV_DECL (body);

  g_signal_emit (body, signals [SELECTION_CHANGED], 0, priv->selection);

  GtkClipboard *primary =
    gtk_clipboard_get_for_display (gtk_widget_get_display (GTK_WIDGET (body)),
                                   GDK_SELECTION_PRIMARY);

  ssw_sheet_body_set_clip (body, primary);
}

static void
set_selection (SswSheetBody *body, gint start_x, gint start_y, gint end_x, gint end_y)
{
  PRIV_DECL (body);

  priv->selection->start_x = start_x;
  priv->selection->start_y = start_y;

  priv->selection->end_x = end_x;
  priv->selection->end_y = end_y;

  announce_selection (body);
}

/* Forces COL and ROW to be within the range of the currently loaded
   model.   Returns TRUE if COL or ROW was changed.
*/
static gboolean
trim_to_model_limits (SswSheetBody *body, gint old_col, gint old_row, gint *col, gint *row)
{
  gboolean clipped = FALSE;
  PRIV_DECL (body);

  /* Allow editing of a new cell */
  if (priv->editable)
    {
      if (old_row == ssw_sheet_axis_get_size (priv->vaxis) && old_col == 0)
        return FALSE;

      if (old_col == ssw_sheet_axis_get_size (priv->haxis) && old_row == 0)
        return FALSE;

      if (*row == ssw_sheet_axis_get_size (priv->vaxis) && *col == 0)
        return FALSE;

      if (*col == ssw_sheet_axis_get_size (priv->haxis) && *row == 0)
        return FALSE;
    }

  if (*col >= ssw_sheet_axis_get_size (priv->haxis))
    {
      *col = ssw_sheet_axis_get_size (priv->haxis) - 1;
      clipped = TRUE;
    }

  if (*row >= ssw_sheet_axis_get_size (priv->vaxis))
    {
      *row = ssw_sheet_axis_get_size (priv->vaxis) - 1;
      clipped = TRUE;
    }

  if (*col < 0)
    {
      *col = 0;
      clipped = TRUE;
    }

  if (*row < 0)
    {
      *row = 0;
      clipped = TRUE;
    }

  return clipped;
}

static gboolean
__button_release_event (GtkWidget *w, GdkEventButton *e)
{
  return GTK_WIDGET_CLASS (ssw_sheet_body_parent_class)->button_release_event (w, e);
}



static void
destroy_editor_widget (SswSheetBody *body)
{
  PRIV_DECL (body);
  gtk_cell_editable_remove_widget (GTK_CELL_EDITABLE (priv->editor));
}

static void
finish_editing (GtkSpinButton *s, gpointer user_data)
{
  GtkCellEditable *ce = GTK_CELL_EDITABLE (s);
  g_object_set (ce, "editing-canceled", FALSE, NULL);
  gtk_cell_editable_editing_done (ce);
}

static void
start_editing (SswSheetBody *body, GdkEvent *e)
{
  PRIV_DECL (body);

  gint row = -1, col = -1;
  get_active_cell (body, &col, &row);

  /* There seems to be a bug in GtkCellRendererSpinButton, where
     its GtkAdjustment spins at wierd moments.  Blocking the handler
     here seems to work around the problem. */
  if (priv->editor && GTK_IS_SPIN_BUTTON (priv->editor))
    g_signal_handlers_block_by_func (priv->editor, finish_editing, NULL);

  if (GTK_WIDGET (body) == priv->sheet->selected_body)
    {
      GtkCellRenderer *renderer = choose_renderer (body, col, row);
      GtkCellEditable *ce =
        gtk_cell_renderer_start_editing (renderer, e, GTK_WIDGET (body),
                                         priv->path, NULL, NULL,
                                         GTK_CELL_RENDERER_SELECTED);

      /* We use this property with a slightly different nuance:
         It is rather a "not-started" flag than a canceled flag. That is
         to say, it is TRUE  by default and becomes FALSE, once an
         edit has commenced */
      g_object_set (ce, "editing-canceled", TRUE, NULL);
    }
}

static void on_editing_done (GtkCellEditable *e, SswSheetBody *body);


static gboolean
__button_press_event (GtkWidget *w, GdkEventButton *e)
{
  SswSheetBody *body = SSW_SHEET_BODY (w);
  PRIV_DECL (body);

  if (e->type != GDK_BUTTON_PRESS)
    return FALSE;

  GdkEventButton *eb = (GdkEventButton *) e;

  gint old_row = -1, old_col = -1;
  get_active_cell (body, &old_col, &old_row);

  gint col  = ssw_sheet_axis_find_cell (priv->haxis, eb->x, NULL, NULL);
  gint row  = ssw_sheet_axis_find_cell (priv->vaxis, eb->y, NULL, NULL);

  if (trim_to_model_limits (body, old_col, old_row, &col, &row))
    return FALSE;

  if (priv->editor)
    on_editing_done (GTK_CELL_EDITABLE (priv->editor), body);

  if (priv->sheet->selected_body != GTK_WIDGET (body))
    {
      destroy_editor_widget (SSW_SHEET_BODY (priv->sheet->selected_body));
    }

  GdkWindow *win = gtk_widget_get_window (GTK_WIDGET (body));

  if (gdk_window_get_cursor (win) != priv->hc
      && priv->editable
      && gdk_window_get_cursor (win) != priv->vc)
    set_active_cell (body, col, row);

  priv->sheet->selected_body = GTK_WIDGET (body);
  gtk_widget_grab_focus (w);

  if (priv->editable)
    start_editing (body, (GdkEvent *) e);

  return GTK_WIDGET_CLASS (ssw_sheet_body_parent_class)->button_press_event (w, e);
}

static gboolean
is_printable_key (gint keyval)
{
  switch (keyval)
    {
    case GDK_KEY_Return:
    case GDK_KEY_ISO_Left_Tab:
    case GDK_KEY_Tab:
      return FALSE;
      break;
    }

  return (0 != gdk_keyval_to_unicode (keyval));
}

static gboolean
__key_press_event_shifted (GtkWidget *w, GdkEventKey *e)
{
  SswSheetBody *body = SSW_SHEET_BODY (w);
  PRIV_DECL (body);

  switch (e->keyval)
    {
    case GDK_KEY_Left:
      if (e->state & GDK_CONTROL_MASK)
        priv->selection->end_x = 0;
      else
        priv->selection->end_x--;
      break;
    case GDK_KEY_Right:
      if (e->state & GDK_CONTROL_MASK)
        priv->selection->end_x = ssw_sheet_axis_get_size (priv->haxis) - 1;
      else
        priv->selection->end_x++;
      break;
    case GDK_KEY_Up:
      if (e->state & GDK_CONTROL_MASK)
        priv->selection->end_y = 0;
      else
        priv->selection->end_y--;
      break;
    case GDK_KEY_Down:
      if (e->state & GDK_CONTROL_MASK)
        priv->selection->end_y = ssw_sheet_axis_get_size (priv->vaxis) - 1;
      else
        priv->selection->end_y++;
      break;
    default:
      return FALSE;
      break;
    }

  limit_selection (body);

  announce_selection (body);

  return FALSE;
}

gboolean
ssw_sheet_body_get_active_cell (SswSheetBody *body,
                                gint *col, gint *row)
{
  return get_active_cell (body, col, row);
}

void
ssw_sheet_body_set_active_cell (SswSheetBody *body,
                                gint col, gint row, GdkEvent *e)
{
  PRIV_DECL (body);

  if (!priv->editable)
    return;

  if (priv->editor && (GTK_WIDGET (body) == priv->sheet->selected_body))
    gtk_cell_editable_editing_done (GTK_CELL_EDITABLE (priv->editor));

  gint oldrow = -1, oldcol = -1;
  get_active_cell (body, &oldcol, &oldrow);

  if (row == -1)
    row = oldrow;

  if (col == -1)
    col = oldcol;

  if (row == -1)
    row = 0;

  if (col == -1)
    col = 0;

  set_active_cell (body, col, row);

  start_editing (body, e);

  /* Do not announce the selection if it hasn't changed.
     Otherwise infinite loops can occur :( */
  if (oldrow != row || oldcol != col)
    {
      set_selection (body, col, row, col, row);
    }
}

static gboolean
__key_press_event (GtkWidget *w, GdkEventKey *e)
{
  SswSheetBody *body = SSW_SHEET_BODY (w);
  PRIV_DECL (body);
  /* If send_event is true, then this event has been generated by on_entry_activate*/
  if ( !e->send_event &&
       GTK_WIDGET_CLASS (ssw_sheet_body_parent_class)->key_press_event (w, e))
    return TRUE;

  if (is_printable_key (e->keyval))
    {
      if (priv->editor && priv->editable &&
          (priv->sheet->selected_body == GTK_WIDGET (body)))
        {
          gtk_widget_grab_focus (priv->editor);
          if (GTK_IS_ENTRY (priv->editor))
            {
              gchar out[7] = {0,0,0,0,0,0,0};
              guint32 code = gdk_keyval_to_unicode (e->keyval);
              g_unichar_to_utf8 (code, out);
              gtk_entry_set_text (GTK_ENTRY(priv->editor), out);
              gtk_editable_set_position (GTK_EDITABLE (priv->editor), -1);
            }
        }
      return FALSE;
    }

  if (e->state & GDK_SHIFT_MASK &&
      e->keyval != GDK_KEY_ISO_Left_Tab)
    return __key_press_event_shifted (w, e);

  const gint vlimit = ssw_sheet_axis_get_size (priv->vaxis) - 1;
  gint row = -1, col = -1;
  get_active_cell (body, &col, &row);

  gint page_length = ssw_sheet_axis_get_visible_size (priv->vaxis) - 1;

  gint hstep ;
  gint h_end_limit ;
  gint h_start_limit ;

  if (ssw_sheet_axis_rtl (priv->haxis))
    {
      hstep = -1;
      h_end_limit = 0;
      h_start_limit = ssw_sheet_axis_get_size (priv->haxis) - 1;
    }
  else
    {
      hstep = +1;
      h_end_limit = ssw_sheet_axis_get_size (priv->haxis) - 1;
      h_start_limit = 0;
    }

  const gint old_row = row;
  const gint old_col = col;

  switch (e->keyval)
    {
    case GDK_KEY_Home:
      col = 0;
      row = 0;
      break;
    case GDK_KEY_Left:
      if (e->state & GDK_CONTROL_MASK)
        col = h_start_limit;
      else
        col -= hstep;
      break;
    case GDK_KEY_Right:
      if (e->state & GDK_CONTROL_MASK)
        col = h_end_limit;
      else
        col += hstep;
      break;
    case GDK_KEY_Page_Up:
      row -= page_length;
      break;
    case GDK_KEY_Page_Down:
      row += page_length;
      break;
    case GDK_KEY_Up:
      if (e->state & GDK_CONTROL_MASK)
        row = 0;
      else
        row--;
      break;
    case GDK_KEY_Down:
      if (e->state & GDK_CONTROL_MASK)
        row = G_MAXINT;
      else
        row++;
      break;
    case GDK_KEY_Return:
      row++;
      break;
    case GDK_KEY_ISO_Left_Tab:
      {
        col --;
        if (col < 0 && row > 0)
          {
            col = ssw_sheet_axis_get_size (priv->haxis) - 1;
            row --;
          }
      }
      break;
    case GDK_KEY_Tab:
      {
        col ++;
        if (col >= ssw_sheet_axis_get_size (priv->haxis)  && row < vlimit)
          {
            col = 0;
            row ++;
          }
      }
      break;
    default:
      return FALSE;
      break;
    }

  trim_to_model_limits (body, old_col, old_row, &col, &row);

  if (col > ssw_sheet_axis_get_last (priv->haxis))
    ssw_sheet_axis_jump_end (priv->haxis, col);

  if (col < ssw_sheet_axis_get_first (priv->haxis))
    ssw_sheet_axis_jump_start (priv->haxis, col);

  if (row > ssw_sheet_axis_get_last (priv->vaxis))
    ssw_sheet_axis_jump_end (priv->vaxis, row);

  if (row < ssw_sheet_axis_get_first (priv->vaxis))
    ssw_sheet_axis_jump_start (priv->vaxis, row);

  ssw_sheet_body_set_active_cell (body, col, row, (GdkEvent *) e);

  if (old_row != row || old_col != col)
    set_selection (body, col, row, col, row);

  /* Return true, to stop keys unfocusing the sheet */
  return TRUE;
}



static gboolean
__motion_notify_event (GtkWidget *w, GdkEventMotion *e)
{
  SswSheetBody *body = SSW_SHEET_BODY (w);
  PRIV_DECL (body);

  if (GTK_WIDGET_CLASS (ssw_sheet_body_parent_class)->motion_notify_event (w, e))
    return TRUE;

  if (gtk_gesture_is_active (priv->horizontal_resize_gesture) || gtk_gesture_is_active (priv->selection_gesture))
    return FALSE;

  const int threshold = 5;

  gint xpos;
  gint xsize;
  gint ypos;
  gint ysize;

  GdkWindow *win = gtk_widget_get_window (w);

  gdouble xm = e->x;
  gdouble ym = e->y;
  if (win != e->window)
    {
      gdk_window_coords_to_parent (e->window, e->x, e->y, &xm, &ym);
    }

  gint x = ssw_sheet_axis_find_cell (priv->haxis, xm, &xpos, &xsize);
  gint y = ssw_sheet_axis_find_cell (priv->vaxis, ym, &ypos, &ysize);

  if (x < 0 || y < 0)
    {
      gdk_window_set_cursor (win, NULL);
      return FALSE;
    }

  gint nearest_x =
    (fabs (xpos - xm) < fabs (xpos + xsize - xm)) ? xpos : xpos + xsize;
  gint nearest_y =
    (fabs (ypos - ym) < fabs (ypos + ysize - ym)) ? ypos : ypos + ysize;

  int xdiff = fabs (nearest_x - xm);
  int ydiff = fabs (nearest_y - ym);

  x = (fabs (xpos - xm) < fabs (xpos + xsize - xm)) ? x : x + 1;
  y = (fabs (ypos - ym) < fabs (ypos + ysize - ym)) ? y : y + 1;

  gint row = -1, col = -1;
  get_active_cell (body, &col, &row);

  if ((xdiff < threshold) && (ydiff < threshold) &&
      (row == y - 1 && col == x - 1))
    {
      gdk_window_set_cursor (win, priv->xc);
    }
  else if (xdiff < threshold && x > 0)
    {
      gdk_window_set_cursor (win, priv->hc);
    }
  else if (ydiff < threshold && y > 0)
    {
      gdk_window_set_cursor (win, priv->vc);
    }
  else
    {
      gdk_window_set_cursor (win, NULL);
    }

  return FALSE;
}


GtkWidget *

ssw_sheet_body_new (SswSheet *sheet)
{
  return GTK_WIDGET (g_object_new (SSW_TYPE_SHEET_BODY,
                                   "sheet", sheet,
                                   NULL));
}


enum
  {
   PROP_0,
   PROP_VAXIS,
   PROP_HAXIS,
   PROP_DATA_MODEL,
   PROP_GRIDLINES,
   PROP_EDITABLE,
   PROP_SELECTION,
   PROP_RENDERER_FUNC,
   PROP_CONVERT_FWD_FUNC,
   PROP_CONVERT_REV_FUNC,
   PROP_SHEET
  };

static void
select_entire_row (SswSheetBody *body, gint i, guint state, gpointer ud)
{
  PRIV_DECL (body);

  GdkDisplay *disp = gtk_widget_get_display (GTK_WIDGET (body));
  GdkKeymap *km = gdk_keymap_get_for_display (disp);
  GdkModifierType extend_mask =
    gdk_keymap_get_modifier_mask (km, GDK_MODIFIER_INTENT_EXTEND_SELECTION);

  gint start_y = (extend_mask & state) ? priv->selection->start_y : i;

  set_selection (body,
                 0, start_y,
                 ssw_sheet_axis_get_size (priv->haxis) - 1, i);
  start_editing (body, NULL);
}

static void
select_entire_column (SswSheetBody *body, gint i, guint state, gpointer ud)
{
  PRIV_DECL (body);
  GdkDisplay *disp = gtk_widget_get_display (GTK_WIDGET (body));
  GdkKeymap *km = gdk_keymap_get_for_display (disp);
  GdkModifierType extend_mask =
    gdk_keymap_get_modifier_mask (km, GDK_MODIFIER_INTENT_EXTEND_SELECTION);

  gint start_x = (extend_mask & state) ? priv->selection->start_x : i;

  set_selection (body,
                 start_x, 0,
                 i, ssw_sheet_axis_get_size (priv->vaxis) - 1);
  start_editing (body, NULL);
}


static void
set_editor_widget_value (SswSheetBody *body, GValue *value, GtkEditable *editable)
{
  PRIV_DECL (body);

  if (editable == NULL)
    return;

  gint row = -1, col = -1;
  get_active_cell (body, &col, &row);

  /* Allow the datum to be changed only if the "editable" property is set */
  gtk_editable_set_editable (editable, priv->editable);

  if (GTK_IS_SPIN_BUTTON (editable))
    {
      if (G_IS_VALUE (value))
        {
          GValue dvalue = G_VALUE_INIT;
          g_value_init (&dvalue, G_TYPE_DOUBLE);
          if (g_value_transform (value, &dvalue))
            {
              gtk_spin_button_set_value (GTK_SPIN_BUTTON (editable), g_value_get_double (&dvalue));
            }
          g_value_unset (&dvalue);
        }
    }
  else if (GTK_IS_ENTRY (editable))
    {
      gchar *s = NULL;
      if (G_IS_VALUE (value))
        s = priv->cf (priv->sheet, priv->data_model, col, row, value);
      gtk_entry_set_text (GTK_ENTRY (editable), s ? s : "");
      g_free (s);
    }
  else if (GTK_IS_COMBO_BOX (editable))
    {
      gint n = -1;
      if (G_IS_VALUE (value))
        n = g_value_get_enum (value);
      gtk_combo_box_set_active (GTK_COMBO_BOX (editable), n);
    }
  else
    {
      gchar *x = NULL;
      if (G_IS_VALUE (value))
        x = g_strdup_value_contents (value);
      g_warning ("Unhandled edit widget %s, when dealing with %s\n",
                 G_OBJECT_TYPE_NAME (editable), x);
      g_free (x);
    }
}

static void
update_editable (SswSheetBody *body)
{
  PRIV_DECL (body);

  gint row = -1, col = -1;
  if (! get_active_cell (body, &col, &row))
    return;

  GtkTreeIter iter;
  if (gtk_tree_model_iter_nth_child (priv->data_model, &iter, NULL, row))
    {
      GValue value = G_VALUE_INIT;
      gtk_tree_model_get_value (priv->data_model, &iter, col, &value);

      set_editor_widget_value (body, &value, GTK_EDITABLE (priv->editor));
      g_value_unset (&value);
    }
}

static void
on_data_change (GtkTreeModel *tm, guint posn, guint rm, guint add, gpointer p)
{
  /* Set the "editor widget" (typically a GtkEntry) to reflect the new value */
  SswSheetBody *body = SSW_SHEET_BODY (p);
  gtk_widget_queue_draw (GTK_WIDGET (body));
  update_editable (body);
}

static void
__set_property (GObject *object,
                guint prop_id, const GValue *value, GParamSpec *pspec)
{
  SswSheetBody *body = SSW_SHEET_BODY (object);
  PRIV_DECL (body);

  switch (prop_id)
    {
    case PROP_SHEET:
      priv->sheet = g_value_get_object (value);
      break;
    case PROP_VAXIS:
      priv->vaxis = g_value_get_object (value);
      g_signal_connect_swapped (priv->vaxis, "changed",
                                G_CALLBACK (scroll_vertically), object);
      g_signal_connect_swapped (priv->vaxis, "header-clicked",
                                G_CALLBACK (select_entire_row), object);
      break;
    case PROP_HAXIS:
      priv->haxis = g_value_get_object (value);
      g_signal_connect_swapped (priv->haxis, "changed",
                                G_CALLBACK (scroll_horizontally), object);
      g_signal_connect_swapped (priv->haxis, "header-clicked",
                                G_CALLBACK (select_entire_column), object);
      break;
    case PROP_GRIDLINES:
      priv->show_gridlines = g_value_get_boolean (value);
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_EDITABLE:
      priv->editable = g_value_get_boolean (value);
      break;
    case PROP_RENDERER_FUNC:
      priv->renderer_func = g_value_get_pointer (value);
      break;
    case PROP_CONVERT_FWD_FUNC:
      priv->cf = g_value_get_pointer (value);
      gtk_widget_queue_draw (GTK_WIDGET (body));
      update_editable (body);
      break;
    case PROP_CONVERT_REV_FUNC:
      priv->revf = g_value_get_pointer (value);
      break;
    case PROP_SELECTION:
      priv->selection = g_value_get_pointer (value);
      break;
    case PROP_DATA_MODEL:
      g_set_object (&priv->data_model, g_value_get_object (value));
      g_signal_connect_object (priv->data_model, "items-changed",
                               G_CALLBACK (on_data_change), body, 0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
__get_property (GObject * object,
                guint prop_id, GValue * value, GParamSpec * pspec)
{
  SswSheetBody *body = SSW_SHEET_BODY (object);
  PRIV_DECL (body);

  switch (prop_id)
    {
    case PROP_SHEET:
      g_value_set_object (value, priv->sheet);
      break;
    case PROP_VAXIS:
      g_value_set_object (value, priv->vaxis);
      break;
    case PROP_HAXIS:
      g_value_set_object (value, priv->haxis);
      break;
    case PROP_RENDERER_FUNC:
      g_value_set_pointer (value, priv->renderer_func);
      break;
    case PROP_CONVERT_FWD_FUNC:
      g_value_set_pointer (value, priv->cf);
      break;
    case PROP_CONVERT_REV_FUNC:
      g_value_set_pointer (value, priv->revf);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ssw_sheet_body_class_init (SswSheetBodyClass * class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  GParamSpec *sheet_spec =
    g_param_spec_object ("sheet",
                         P_("Sheet"),
                         P_("The SswSheet to which this body belongs"),
                         SSW_TYPE_SHEET,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  GParamSpec *haxis_spec =
    g_param_spec_object ("horizontal-axis",
                         P_("Horizontal Axis"),
                         P_("The Horizontal Axis"),
                         SSW_TYPE_SHEET_AXIS,
                         G_PARAM_READWRITE);

  GParamSpec *vaxis_spec =
    g_param_spec_object ("vertical-axis",
                         P_("Vertical Axis"),
                         P_("The Vertical Axis"),
                         SSW_TYPE_SHEET_AXIS,
                         G_PARAM_READWRITE);

  GParamSpec *data_model_spec =
    g_param_spec_object ("data-model",
                         P_("Data Model"),
                         P_("The model describing the contents of the data"),
                         GTK_TYPE_TREE_MODEL,
                         G_PARAM_READWRITE);

  GParamSpec *gridlines_spec =
    g_param_spec_boolean ("gridlines",
                          P_("Show Gridlines"),
                          P_("True if gridlines should be shown"),
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  GParamSpec *editable_spec =
    g_param_spec_boolean ("editable",
                          P_("Editable"),
                          P_("True if the data may be edited"),
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  GParamSpec *renderer_func_spec =
    g_param_spec_pointer ("select-renderer-func",
                          P_("Select Renderer Function"),
                          P_("Function returning the renderer to use for a cell"),
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  GParamSpec *convert_fwd_spec =
    g_param_spec_pointer ("forward-conversion",
                          P_("Forward conversion function"),
                          P_("A function to convert a cell datum to a string"),
                          G_PARAM_READWRITE);

  GParamSpec *convert_rev_spec =
    g_param_spec_pointer ("reverse-conversion",
                          P_("Reverse conversion function"),
                          P_("A function to convert a string to a cell datum"),
                          G_PARAM_READWRITE);

  GParamSpec *selection_spec =
    g_param_spec_pointer ("selection",
                          P_("The selection"),
                          P_("A pointer to the current selection"),
                          G_PARAM_READWRITE);

  object_class->set_property = __set_property;
  object_class->get_property = __get_property;


  g_object_class_install_property (object_class,
                                   PROP_SHEET,
                                   sheet_spec);

  g_object_class_install_property (object_class,
                                   PROP_VAXIS,
                                   vaxis_spec);

  g_object_class_install_property (object_class,
                                   PROP_HAXIS,
                                   haxis_spec);

  g_object_class_install_property (object_class,
                                   PROP_GRIDLINES,
                                   gridlines_spec);

  g_object_class_install_property (object_class,
                                   PROP_EDITABLE,
                                   editable_spec);

  g_object_class_install_property (object_class,
                                   PROP_DATA_MODEL,
                                   data_model_spec);

  g_object_class_install_property (object_class,
                                   PROP_RENDERER_FUNC,
                                   renderer_func_spec);

  g_object_class_install_property (object_class,
                                   PROP_CONVERT_FWD_FUNC,
                                   convert_fwd_spec);

  g_object_class_install_property (object_class,
                                   PROP_CONVERT_REV_FUNC,
                                   convert_rev_spec);

  g_object_class_install_property (object_class,
                                   PROP_SELECTION,
                                   selection_spec);

  object_class->dispose = __dispose;
  object_class->finalize = __finalize;

  widget_class->draw = __draw;
  widget_class->realize = __realize;
  widget_class->button_press_event = __button_press_event;
  widget_class->button_release_event = __button_release_event;
  widget_class->motion_notify_event = __motion_notify_event;
  widget_class->key_press_event = __key_press_event;

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
}


static void
selection_drag_begin (GtkGesture *gesture,
                      gdouble         start_x,
                      gdouble         start_y,
                      gpointer        user_data)
{
  SswSheetBody *body = SSW_SHEET_BODY (user_data);
  PRIV_DECL (body);

  GdkWindow *win = gtk_widget_get_window (GTK_WIDGET (body));

  if (gdk_window_get_cursor (win) == priv->hc || gdk_window_get_cursor (win) == priv->vc)
    {
      gtk_gesture_set_state (gesture, GTK_EVENT_SEQUENCE_DENIED);
      return;
    }

  gtk_gesture_set_state (gesture, GTK_EVENT_SEQUENCE_CLAIMED);

  ssw_sheet_body_unset_selection (body);

  priv->selection->start_x = ssw_sheet_axis_find_cell (priv->haxis, start_x, NULL, NULL);
  priv->selection->start_y = ssw_sheet_axis_find_cell (priv->vaxis, start_y, NULL, NULL);

  priv->selection->end_x = priv->selection->start_x;
  priv->selection->end_y = priv->selection->start_y;
}



static void
selection_drag_update (GtkGestureDrag *gesture,
                       gdouble         offset_x,
                       gdouble         offset_y,
                       gpointer        user_data)
{
  SswSheetBody *body = SSW_SHEET_BODY (user_data);
  PRIV_DECL (body);

  gdouble start_x, start_y;
  gtk_gesture_drag_get_start_point (GTK_GESTURE_DRAG (gesture), &start_x, &start_y);

  gint xsize, ysize;
  gint xpos, ypos;

  gint x = ssw_sheet_axis_find_cell (priv->haxis, start_x + offset_x, &xpos, &xsize);
  gint y = ssw_sheet_axis_find_cell (priv->vaxis, start_y + offset_y, &ypos, &ysize);

  if (x < 0 || y < 0)
    return ;

  priv->selection->end_x = x - 1;
  priv->selection->end_y = y - 1;

  if (start_x + offset_x > (gdouble) xpos - (gdouble) xsize/2.0)
    priv->selection->end_x++;

  if (start_y + offset_y > (gdouble) ypos - (gdouble) ysize/2.0)
    priv->selection->end_y++;

  limit_selection (body);


  gtk_widget_queue_draw (GTK_WIDGET (body));
}

static void
selection_drag_end (GtkGestureDrag *gesture,
                    gdouble         offset_x,
                    gdouble         offset_y,
                    gpointer        user_data)
{
  SswSheetBody *body = SSW_SHEET_BODY (user_data);

  GdkEventSequence *seq =
    gtk_gesture_get_last_updated_sequence (GTK_GESTURE(gesture));

  GtkEventSequenceState state =
    gtk_gesture_get_sequence_state (GTK_GESTURE (gesture), seq);

  if (state == GTK_EVENT_SEQUENCE_DENIED)
    return;

  announce_selection (body);
}



/* The following targets are emitted by Libreoffice:

   text/plain;charset=utf-8
   text/plain;charset=UTF-8
   UTF-8
   UTF8_STRING
   COMPOUND_TEXT
   STRING
   application/x-openoffice-embed-source-xml;windows_formatname="Star Embed Source (XML)"
   application/x-openoffice-objectdescriptor-xml;windows_formatname="Star Object Descriptor (XML)";classname="47BBB4CB-CE4C-4E80-a591-42d9ae74950f";typename="calc8";viewaspect="1";width="4517";height="3162";posx="0";posy="0"
   application/x-openoffice-gdimetafile;windows_formatname="GDIMetaFile"
   application/x-openoffice-emf;windows_formatname="Image EMF"
   application/x-openoffice-wmf;windows_formatname="Image WMF"
   image/png
   application/x-openoffice-bitmap;windows_formatname="Bitmap"
   PIXMAP
   image/bmp
   text/html
   application/x-openoffice-sylk;windows_formatname="Sylk"
   application/x-openoffice-link;windows_formatname="Link"
   application/x-openoffice-dif;windows_formatname="DIF"
   text/richtext
   MULTIPLE

   And Gnumeric generates the following targets:

   TIMESTAMP
   TARGETS
   MULTIPLE
   SAVE_TARGETS
   application/x-gnumeric
   text/html
   UTF8_STRING
   COMPOUND_TEXT
   STRING

*/


enum target_info
  {
   TARGET_TEXT_TSV,
   TARGET_TEXT_CSV,
   TARGET_HTML,
   TARGET_TEXT_PLAIN,
   TARGET_UTF8_STRING,
   TARGET_STRING,
   N_TARGETS
  };

static const GtkTargetEntry targets[] =
  {
   {
    "text/tab-separated-values",
    GTK_TARGET_OTHER_APP,
    TARGET_TEXT_TSV
   },
   {
    "text/csv",
    GTK_TARGET_OTHER_APP,
    TARGET_TEXT_CSV
   },
   {
    "text/html",
    GTK_TARGET_OTHER_APP,
    TARGET_HTML
   },
   {
    "text/plain",
    GTK_TARGET_OTHER_APP,
    TARGET_TEXT_PLAIN
   },
   {
    "UTF8_STRING",
    GTK_TARGET_OTHER_APP,
    TARGET_UTF8_STRING
   },
   {
    "STRING",
    GTK_TARGET_OTHER_APP,
    TARGET_STRING
   },
  };


/*
  Append a string representation of the value contained in MODEL,ITER,COL
  to the string OUTPUT.
*/
static void
append_value_to_string (SswSheetBody *body,
                        GtkTreeIter *iter, gint col, gint row,
                        GString *output)
{
  PRIV_DECL (body);
  GtkTreeModel *model = priv->data_model;
  GValue value = G_VALUE_INIT;
  GValue target_value = G_VALUE_INIT;
  g_value_init (&target_value, G_TYPE_STRING);
  gtk_tree_model_get_value (model, iter, col, &value);

  if (priv->cf)
    {
      gchar *x = priv->cf (priv->sheet, model, col, row, &value);
      g_string_append (output, x);
      g_free (x);
    }
  else if (g_value_transform (&value, &target_value))
    {
      g_string_append (output, g_value_get_string (&target_value));
    }
  else
    {
      g_warning ("Pasting from SswSheet failed.  "
                 "You must register a transform function for source "
                 "type \"%s\" to dest type \"%s\"\n",
                 G_VALUE_TYPE_NAME (&value),
                 g_type_name (G_TYPE_STRING));
    }

  g_value_unset (&value);
  g_value_unset (&target_value);
}



static void
clipit_html (SswSheetBody *body, GString *output, SswRange *source_range)
{
  PRIV_DECL (body);
  if (!priv->haxis || !priv->vaxis || !priv->data_model)
    return;

  g_string_append (output, "<body>\n");
  g_string_append (output, "<table>\n");

  gint row;
  gint col;
  for (row = source_range->start_y ; row <= source_range->end_y; ++row)
    {
      GtkTreeIter iter;
      gtk_tree_model_iter_nth_child (priv->data_model, &iter, NULL, row);
      g_string_append (output, "<tr>\n");
      for (col = source_range->start_x; col <= source_range->end_x; ++col)
        {
          if (row < gtk_tree_model_iter_n_children (priv->data_model, NULL)
              && col < gtk_tree_model_get_n_columns (priv->data_model))
            {
              g_string_append (output, "<td>");
              append_value_to_string (body, &iter, col, row, output);
              g_string_append (output, "</td>\n");
            }
        }
      g_string_append (output, "</tr>\n");
    }

  g_string_append (output, "</table>\n");
  g_string_append (output, "</body>\n");
}


static void
clipit_ascii (SswSheetBody *body, GString *output, SswRange *source_range)
{
  PRIV_DECL (body);
  if (!priv->haxis || !priv->vaxis || !priv->data_model)
    return;

  gint row;
  gint col;
  for (row = source_range->start_y ; row <= source_range->end_y; ++row)
    {
      GtkTreeIter iter;
      gtk_tree_model_iter_nth_child (priv->data_model, &iter, NULL, row);

      for (col = source_range->start_x; col <= source_range->end_x; ++col)
        {
          if (priv->data_model
              && row < gtk_tree_model_iter_n_children (priv->data_model, NULL)
              && col < gtk_tree_model_get_n_columns (priv->data_model))
            {
              append_value_to_string (body, &iter, col, row, output);

              if (col < source_range->end_x)
                g_string_append (output, "\t");
            }
        }
      if (row < source_range->end_y)
        g_string_append (output, "\n");
    }
}

static void
clipit_utf8 (SswSheetBody *body, GString *output, SswRange *source_range)
{
  PRIV_DECL (body);
  if (!priv->haxis || !priv->vaxis || !priv->data_model)
    return;

  gint row;
  gint col;
  for (row = source_range->start_y ; row <= source_range->end_y; ++row)
    {
      GtkTreeIter iter;
      gtk_tree_model_iter_nth_child (priv->data_model, &iter, NULL, row);

      for (col = source_range->start_x; col <= source_range->end_x; ++col)
        {
          if (row < gtk_tree_model_iter_n_children (priv->data_model, NULL)
              && col < gtk_tree_model_get_n_columns (priv->data_model))
            {
              append_value_to_string (body, &iter, col, row, output);

              if (col < source_range->end_x)
                g_string_append_c (output, '\t');
            }
        }
      if (row < source_range->end_y)
        g_string_append (output, "\n");
    }
}


static void
get_func (GtkClipboard *clipboard,
          GtkSelectionData *selection_data,
          guint info,
          gpointer owner)
{
  SswSheetBody *body = SSW_SHEET_BODY (owner);

  if (body == NULL)
    {
      gtk_clipboard_clear (clipboard);
      return;
    }

  SswRange *source_range = g_object_get_data (G_OBJECT (clipboard), "source-range");
  g_return_if_fail (source_range);

  GString *stuff = g_string_new ("");
  switch (info)
    {
    case TARGET_STRING:
      clipit_ascii (body, stuff, source_range);
      break;
    case TARGET_UTF8_STRING:
      clipit_utf8 (body, stuff, source_range);
      break;
    case TARGET_HTML:
      clipit_html (body, stuff, source_range);
      break;
    default:
      g_warning ("Request for unknown target %d\n", info);
      return ;
      break;
    }

  gtk_selection_data_set (selection_data,
                          gdk_atom_intern_static_string (targets[info].target),
                          CHAR_BIT,
                          (gpointer) stuff->str, stuff->len);

  g_string_free (stuff, TRUE);
}


static void
clear_func (GtkClipboard *clipboard,
            gpointer user_data_or_owner)
{
}

void
ssw_sheet_body_set_clip (SswSheetBody *body, GtkClipboard *clip)
{
  PRIV_DECL (body);
  if (body == NULL)
    return;

  if (priv->editor &&
      GTK_IS_EDITABLE (priv->editor) &&
      gtk_widget_is_focus (priv->editor))
    {
      gtk_editable_copy_clipboard (GTK_EDITABLE(priv->editor));
      return;
    }

  SswRange *source_range = g_object_get_data (G_OBJECT (clip), "source-range");
  g_free (source_range);
  source_range = g_malloc (sizeof (*source_range));
  g_object_set_data (G_OBJECT (clip), "source-range", source_range);
  normalise_selection (priv->selection, source_range);

  if (!gtk_clipboard_set_with_owner (clip, targets, N_TARGETS,
                                     get_func, clear_func, G_OBJECT (body)))
    {
      g_warning ("Clip failed\n");
    }
}

static void
drag_begin_resize_horizontal (GtkGesture *gesture,
                              gdouble         start_x,
                              gdouble         start_y,
                              gpointer        user_data)
{
  SswSheetBody *body = SSW_SHEET_BODY (user_data);
  PRIV_DECL (body);
  GdkWindow *win = gtk_widget_get_window (GTK_WIDGET (body));

  if (gdk_window_get_cursor (win) == priv->hc)
    {
      gtk_gesture_set_state (gesture, GTK_EVENT_SEQUENCE_CLAIMED);
    }
  else
    {
      gtk_gesture_set_state (gesture, GTK_EVENT_SEQUENCE_DENIED);
    }
}

static void
drag_begin_resize_vertical (GtkGesture *gesture,
                            gdouble         start_x,
                            gdouble         start_y,
                            gpointer        user_data)
{
  SswSheetBody *body = SSW_SHEET_BODY (user_data);
  PRIV_DECL (body);
  GdkWindow *win = gtk_widget_get_window (GTK_WIDGET (body));

  if (gdk_window_get_cursor (win) == priv->vc)
    {
      gtk_gesture_set_state (gesture, GTK_EVENT_SEQUENCE_CLAIMED);
    }
  else
    {
      gtk_gesture_set_state (gesture, GTK_EVENT_SEQUENCE_DENIED);
    }
}

static void
drag_update_resize (GtkGestureDrag *gesture,
                    gdouble         offset_x,
                    gdouble         offset_y,
                    gpointer        user_data)
{
  SswSheetBody *body = SSW_SHEET_BODY (user_data);

  gtk_widget_queue_draw (GTK_WIDGET (body));
}



static void
drag_end_resize_horizontal (GtkGestureDrag *gesture,
                            gdouble         offset_x,
                            gdouble         offset_y,
                            gpointer        user_data)
{
  gdouble start_x, start_y;
  SswSheetBody *body = SSW_SHEET_BODY (user_data);
  PRIV_DECL (body);
  gtk_gesture_drag_get_start_point (gesture, &start_x, &start_y);

  GdkEventSequence *seq =
    gtk_gesture_get_last_updated_sequence (GTK_GESTURE(gesture));

  GtkEventSequenceState state =
    gtk_gesture_get_sequence_state (GTK_GESTURE (gesture), seq);

  if (state == GTK_EVENT_SEQUENCE_DENIED)
    return;

  gint nearest = -1;
  gint pos, size;
  gint item = ssw_sheet_axis_find_cell (priv->haxis, start_x, &pos, &size);
  if (ssw_sheet_axis_rtl (priv->haxis))
    {
      nearest =
        (start_x - pos) < (pos + size - start_x) ? item: item - 1 ;
    }
  else
    {
      nearest =
        (start_x - pos) < (pos + size - start_x) ? item - 1: item ;
    }

  gint old_size;
  ssw_sheet_axis_find_boundary (priv->haxis, nearest, NULL, &old_size);
  const gint new_size = ssw_sheet_axis_rtl (priv->haxis) ?
    old_size - offset_x : old_size + offset_x;

  gint row = -1, col = -1;
  get_active_cell (body, &col, &row);

  if (nearest == col)
    {
      gint vsize;

      ssw_sheet_axis_find_boundary (priv->vaxis, row, NULL, &vsize);


      gtk_widget_set_size_request (GTK_WIDGET (priv->editor),
                                   new_size - linewidth, vsize);

      g_object_set (priv->active_cell_holder,
                    "hconstraint", new_size - linewidth,
                    NULL);
    }

  ssw_sheet_axis_override_size (priv->haxis, nearest, new_size);
}


static void
drag_end_resize_vertical (GtkGestureDrag *gesture,
                          gdouble         offset_x,
                          gdouble         offset_y,
                          gpointer        user_data)
{
  gdouble start_x, start_y;
  SswSheetBody *body = SSW_SHEET_BODY (user_data);
  PRIV_DECL (body);
  gtk_gesture_drag_get_start_point (gesture, &start_x, &start_y);

  GdkEventSequence *seq =
    gtk_gesture_get_last_updated_sequence (GTK_GESTURE(gesture));

  GtkEventSequenceState state =
    gtk_gesture_get_sequence_state (GTK_GESTURE (gesture), seq);

  if (state == GTK_EVENT_SEQUENCE_DENIED)
    return;

  gint pos, size;
  gint item = ssw_sheet_axis_find_cell (priv->vaxis, start_y, &pos, &size);
  gint nearest = (start_y - pos) < (pos + size - start_y) ? item - 1: item ;


  gint old_size;
  ssw_sheet_axis_find_boundary (priv->vaxis, nearest, NULL, &old_size);
  const gint new_size = old_size + offset_y;

  gint row = -1, col = -1;
  get_active_cell (body, &col, &row);

  if (nearest == row)
    {
      gint hsize;
      ssw_sheet_axis_find_boundary (priv->haxis, nearest, &hsize, NULL);

      gtk_widget_set_size_request (GTK_WIDGET (priv->editor),
                                   hsize,
                                   new_size - linewidth);

      g_object_set (priv->active_cell_holder,
                    "vconstraint", new_size - linewidth,
                    NULL);
    }

  ssw_sheet_axis_override_size (priv->vaxis, nearest, new_size);
}



static void
ssw_sheet_body_init (SswSheetBody *body)
{
  PRIV_DECL (body);
  GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (body));
  GtkStyleContext *context = gtk_widget_get_style_context (GTK_WIDGET (body));

  gtk_style_context_add_class (context, "cell");

  priv->sheet = NULL;
  priv->path[0] = '\0';
  priv->dispose_has_run = FALSE;

  priv->default_renderer = gtk_cell_renderer_text_new ();

  priv->vc = gdk_cursor_new_for_display (display, GDK_SB_V_DOUBLE_ARROW);
  priv->hc = gdk_cursor_new_for_display (display, GDK_SB_H_DOUBLE_ARROW);
  priv->xc = gdk_cursor_new_for_display (display, GDK_SIZING);

  priv->selection_gesture = gtk_gesture_drag_new (GTK_WIDGET (body));


  g_signal_connect (priv->selection_gesture, "drag-begin",
                    G_CALLBACK (selection_drag_begin), body);

  g_signal_connect (priv->selection_gesture, "drag-update",
                    G_CALLBACK (selection_drag_update), body);

  g_signal_connect (priv->selection_gesture, "drag-end",
                    G_CALLBACK (selection_drag_end), body);

  priv->horizontal_resize_gesture =
    gtk_gesture_drag_new (GTK_WIDGET (body));

  g_signal_connect (priv->horizontal_resize_gesture, "drag-begin",
                    G_CALLBACK (drag_begin_resize_horizontal), body);

  g_signal_connect (priv->horizontal_resize_gesture, "drag-update",
                    G_CALLBACK (drag_update_resize), body);

  g_signal_connect (priv->horizontal_resize_gesture, "drag-end",
                    G_CALLBACK (drag_end_resize_horizontal), body);

  priv->vertical_resize_gesture =
    gtk_gesture_drag_new (GTK_WIDGET (body));

  g_signal_connect (priv->vertical_resize_gesture, "drag-begin",
                    G_CALLBACK (drag_begin_resize_vertical), body);

  g_signal_connect (priv->vertical_resize_gesture, "drag-update",
                    G_CALLBACK (drag_update_resize), body);

  g_signal_connect (priv->vertical_resize_gesture, "drag-end",
                    G_CALLBACK (drag_end_resize_vertical), body);


  priv->data_model = NULL;
  priv->editor = NULL;
  priv->cf = ssw_sheet_default_forward_conversion;
  priv->revf = ssw_sheet_default_reverse_conversion;

  priv->active_cell_holder = ssw_constraint_new ();
  /* Keep the constraint widget well out of the visible range */
  gtk_layout_put (GTK_LAYOUT (body), priv->active_cell_holder,  -99, -99);
}

void
ssw_sheet_body_unset_selection (SswSheetBody *body)
{
  PRIV_DECL (body);
  priv->selection->start_x = priv->selection->start_y = -1;
  priv->selection->end_x = priv->selection->end_y = -1;

  gtk_widget_queue_draw (GTK_WIDGET (body));
}


void
ssw_sheet_body_value_to_string (SswSheetBody *body, gint col, gint row,
                                GString *output)
{
  GtkTreeIter iter;
  PRIV_DECL (body);

  gtk_tree_model_iter_nth_child (priv->data_model, &iter, NULL, row);

  append_value_to_string (body, &iter, col, row, output);
}




static void text_editing_started (GtkCellRenderer *cell,
                                  GtkCellEditable *editable,
                                  const gchar     *path,
                                  gpointer         data);


static GtkCellRenderer *
choose_renderer (SswSheetBody *body, gint col, gint row)
{
  PRIV_DECL (body);

  GtkCellRenderer *r = NULL;

  if (priv->renderer_func)
    {
      GType t = gtk_tree_model_get_column_type (priv->data_model, col);
      r = priv->renderer_func (priv->sheet, col, row, t,
                               priv->sheet->renderer_func_datum);
    }

  if (r == NULL)
    r = priv->default_renderer;

  g_object_set (r,
                "mode", GTK_CELL_RENDERER_MODE_EDITABLE,
                "editable", TRUE,
                NULL);

  if (!g_object_get_data (G_OBJECT (r), "ess"))
    {
      g_signal_connect (r,
                        "editing-started",
                        G_CALLBACK (text_editing_started),
                        NULL);

      g_object_set_data (G_OBJECT (r), "ess", GINT_TO_POINTER (TRUE));
    }

  return r;
}

static void
on_editing_done (GtkCellEditable *e, SswSheetBody *body)
{
  PRIV_DECL (body);

  gint row = -1, col = -1;
  get_active_cell (body, &col, &row);

  gboolean cell_value_unchanged = -1;
  g_object_get (e, "editing-canceled", &cell_value_unchanged, NULL);

  /* Grab the focus back from the CellEditable */
  gtk_widget_grab_focus (GTK_WIDGET (body));

  if (cell_value_unchanged)
    return;

  GValue value = G_VALUE_INIT;
  if (GTK_IS_SPIN_BUTTON (e))
    {
      GType t = gtk_tree_model_get_column_type (priv->data_model, col);
      g_value_init (&value, t);

      gdouble v = gtk_spin_button_get_value (GTK_SPIN_BUTTON (e));
      GValue dbl_value = G_VALUE_INIT;
      g_value_init (&dbl_value, G_TYPE_DOUBLE);
      g_value_set_double (&dbl_value, v);
      g_value_transform (&dbl_value, &value);
      g_value_unset (&dbl_value);
    }
  else if (GTK_IS_ENTRY (e))
    {
      const char *s = gtk_entry_get_text (GTK_ENTRY (e));
      priv->revf (priv->data_model, col, row, s, &value);
    }
  else if (GTK_IS_COMBO_BOX (e))
    {
      g_value_init (&value, G_TYPE_INT);
      gint n = gtk_combo_box_get_active (GTK_COMBO_BOX (e));
      g_value_set_int (&value, n);
    }

  g_signal_emit (body, signals[VALUE_CHANGED], 0, col, row, &value);

  g_value_unset (&value);
}

static void
on_entry_activate (GtkEntry *e, gpointer ud)
{
  SswSheetBody *body = SSW_SHEET_BODY (ud);
  GdkWindow *win = gtk_widget_get_window (GTK_WIDGET (body));
  g_return_if_fail (GDK_IS_WINDOW (win));
  GdkEvent *ev = gdk_event_new (GDK_KEY_PRESS);
  GdkEventKey *event = (GdkEventKey *) ev;
  event->window = g_object_ref(win);
  event->keyval = GDK_KEY_Return;
  event->send_event = TRUE;
  gboolean ret;
  g_signal_emit_by_name (body, "key-press-event", event, &ret);
  gdk_event_free (ev);
}

static void
done_editing (GtkCellRendererCombo *combo,
              gchar                *path_string,
              GtkTreeIter          *new_iter,
              gpointer              user_data)
{
  GtkCellEditable *ce = user_data;
  g_object_set (ce, "editing-canceled", FALSE, NULL);
  gtk_cell_editable_editing_done (ce);
}


static void
on_remove_widget (GtkCellEditable *e, gpointer ud)
{
  SswSheetBody *body = ud;
  PRIV_DECL (body);

  gtk_widget_destroy (GTK_WIDGET (e));
  priv->editor = NULL;
  gtk_widget_hide (priv->active_cell_holder);
}

static void
on_changed (GtkEditable *editable, gpointer user_data)
{
  g_object_set (editable, "editing-canceled", FALSE, NULL);
}

static gboolean
on_focus_out (GtkWidget *w, GdkEvent *e, gpointer ud)
{
  /* It seems that GtkEntry by default emits the "widget-remove" signal,
     in response to focus-out.  We don't want that to happen, so disable
     it here. */
  return TRUE;
}

static void
text_editing_started (GtkCellRenderer *cell,
                      GtkCellEditable *editable,
                      const gchar     *path,
                      gpointer         data)
{
  SswSheetBody *body = NULL;
  gint row = -1, col = -1;
  sscanf (path, "r%*dc%*ds%p", &body);

  if (body == NULL) /* There is no active cell! */
    return;

  get_active_cell (body, &col, &row);
  PRIV_DECL (body);

  if (col < 0 || row < 0)
    {
      gtk_widget_destroy (GTK_WIDGET (editable));
      return;
    }

  if (! gtk_widget_is_visible (GTK_WIDGET (body)))
    return;

  if (priv->editor)
    {
      gtk_cell_renderer_stop_editing (GTK_CELL_RENDERER (cell),
                                      TRUE);

      g_signal_emit_by_name (priv->editor, "remove-widget");
    }

#if DEBUGGING
  GdkRGBA color = {1.0, 0, 0, 1};
  gtk_widget_override_background_color (GTK_WIDGET (editable),
                                        GTK_STATE_NORMAL,
                                        &color);
#endif

  priv->editor = NULL;

  gint vlocation, vsize;
  gint hlocation, hsize;
  if (0 != ssw_sheet_axis_find_boundary (priv->vaxis, row, &vlocation, &vsize))
    return;

  if (0 != ssw_sheet_axis_find_boundary (priv->haxis, col, &hlocation, &hsize))
    return;

  priv->editor = GTK_WIDGET (editable);

  /* Don't show the stupid "Insert Emoji" in the popup menu.  */
  if (GTK_IS_ENTRY (priv->editor))
    g_object_set (priv->editor,
                  "input-hints", GTK_INPUT_HINT_NO_EMOJI,
                  NULL);

  g_signal_connect (editable, "remove-widget",
                    G_CALLBACK (on_remove_widget), body);


  /* Avoid the gridlines */
  hsize -= linewidth;
  hlocation += linewidth;
  vsize -= linewidth;
  vlocation += linewidth;

  gtk_widget_set_size_request (GTK_WIDGET (editable),  hsize, vsize);

  g_object_set (priv->active_cell_holder,
                "hconstraint", hsize,
                "vconstraint", vsize,
                NULL);

  gtk_container_add (GTK_CONTAINER (priv->active_cell_holder),
                     GTK_WIDGET (editable));

  gtk_layout_move (GTK_LAYOUT (body), priv->active_cell_holder,
                   hlocation, vlocation);

  GtkTreeIter iter;
  GValue value = G_VALUE_INIT;
  if (gtk_tree_model_iter_nth_child (priv->data_model,
                                     &iter, NULL, row))
    {
      gtk_tree_model_get_value (priv->data_model, &iter,
                                col, &value);
    }

  if (GTK_IS_ENTRY (editable))
    {
      g_signal_connect (editable, "focus-out-event", G_CALLBACK (on_focus_out), NULL);
      gtk_entry_set_text (GTK_ENTRY (editable), "");
      g_signal_connect (editable, "changed", G_CALLBACK (on_changed), NULL);
    }
  else if (GTK_IS_COMBO_BOX (editable))
    {
      gtk_combo_box_set_active (GTK_COMBO_BOX (editable), 0);
    }

  g_signal_connect (editable, "editing-done",
                    G_CALLBACK (on_editing_done), body);


  set_editor_widget_value (body, &value, GTK_EDITABLE (editable));
  g_value_unset (&value);

  if (GTK_IS_SPIN_BUTTON (editable))
    {
      g_signal_connect (editable, "value-changed", G_CALLBACK (finish_editing), NULL);
    }
  else if (GTK_IS_ENTRY (editable))
    {
      /* "activate" means when the Enter key is pressed */
      g_signal_connect (editable, "activate", G_CALLBACK (on_entry_activate), body);
    }
  else if (GTK_IS_COMBO_BOX (editable))
    {
      g_signal_connect_object (cell, "changed", G_CALLBACK (done_editing),
                               editable, 0);
    }

  gtk_widget_show_all (priv->active_cell_holder);
}

gboolean
ssw_sheet_body_paste_editable (SswSheetBody *body)
{
  PRIV_DECL (body);
  if (body &&
      priv->editor &&
      GTK_IS_EDITABLE (priv->editor) &&
      gtk_widget_is_focus (priv->editor))
    {
      gtk_editable_paste_clipboard (GTK_EDITABLE (priv->editor));
      return TRUE;
    }
  return FALSE;
}

gboolean
ssw_sheet_body_cut_editable (SswSheetBody *body)
{
  PRIV_DECL (body);
  if (body &&
      priv->editor &&
      GTK_IS_ENTRY (priv->editor) &&
      gtk_widget_is_focus (priv->editor))
    {
      gtk_editable_cut_clipboard ( GTK_EDITABLE (priv->editor));
      return TRUE;
    }
  return FALSE;
}
