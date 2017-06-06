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

/*
   This work is derived from Timm Bäder's original work GD_MODEL_LIST_BOX.
   Downloaded from https://github.com/baedert/listbox-c
   His copyright notice is below.
*/

/*
 *  Copyright 2015 Timm Bäder
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include "ssw-sheet-axis.h"
#include "ssw-datum.h"
#include "ssw-marshaller.h"
#include <math.h>
#include <stdlib.h>


#define P_(X) (X)

struct _SswSheetAxisPrivate
{
  GtkOrientation orientation;

  /* Adjustment in the direction of the bin's orientation */
  GtkAdjustment *adjustment;

  GPtrArray *widgets;
  GPtrArray *pool;
  GdkWindow *bin_window;

  GListModel *model;

  guint model_from;
  guint model_to;
  gdouble bin_start_diff;

  gint (*get_allocated_p_size) (GtkWidget *);
  gint (*get_allocated_q_size) (GtkWidget *);

  void (*get_preferred_p_for_q) (GtkWidget *, gint, gint *, gint *);
  gint (*get_window_p_size) (GdkWindow *);

  void (*set_q_offset) (GtkAllocation *, gint);
  void (*set_p_offset) (GtkAllocation *, gint);
  void (*set_q_size) (GtkAllocation *, gint);
  void (*set_p_size) (GtkAllocation *, gint);

  gint (*get_q_size) (GtkAllocation *);
  gint (*get_p_size) (GtkAllocation *);

  /* A table of all the items whose sizes have been manually overridden */
  GHashTable *size_override;

  GtkGesture *button_gest;
  gint n_press;
  gint press_id;
  guint button;
  guint state;

  /* The cursor to be shown during resizing of the rows/columns */
  GdkCursor *resize_cursor;
  GtkGesture *resize_gest;
  gint resize_target;
  gint new_size;

  GtkGesture *drag_gest;
  GtkTargetList *drag_target_list;
};

typedef struct _SswSheetAxisPrivate SswSheetAxisPrivate;

enum  {CHANGED,
       HEADER_CLICKED,
       HEADER_DOUBLE_CLICKED,
       HEADER_BUTTON_PRESSED,
       HEADER_BUTTON_RELEASED,
       DRAG_N_DROP,
       n_SIGNALS};

static guint signals [n_SIGNALS];

static void
zz_set_alloc_x (GtkAllocation *alloc, gint offset)
{
  alloc->x = offset;
}

static gint
zz_get_alloc_width (GtkAllocation *alloc)
{
  return alloc->width;
}

static void
zz_set_alloc_width (GtkAllocation *alloc, gint size)
{
  alloc->width = size;
}

static void
zz_set_alloc_y (GtkAllocation *alloc, gint offset)
{
  alloc->y = offset;
}

static gint
zz_get_alloc_height (GtkAllocation *alloc)
{
  return alloc->height;
}

static void
zz_set_alloc_height (GtkAllocation *alloc, gint size)
{
  alloc->height = size;
}

G_DEFINE_TYPE_WITH_CODE (SswSheetAxis, ssw_sheet_axis,
                         GTK_TYPE_CONTAINER,
                         G_ADD_PRIVATE (SswSheetAxis)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL));

#define PRIV_DECL(l) SswSheetAxisPrivate *priv = ((SswSheetAxisPrivate *)ssw_sheet_axis_get_instance_private ((SswSheetAxis *)l))
#define PRIV(l) ((SswSheetAxisPrivate *)ssw_sheet_axis_get_instance_private ((SswSheetAxis *)l))

gboolean
ssw_sheet_axis_rtl (SswSheetAxis *axis)
{
  PRIV_DECL (axis);

  return ((priv->orientation == GTK_ORIENTATION_HORIZONTAL)
	  &&
	  (GTK_TEXT_DIR_RTL == gtk_widget_get_direction (GTK_WIDGET (axis))));
}


#define Foreach_Item {int i; for (i = 0; i < priv->widgets->len; i ++){ \
  GtkWidget *item \
  = g_ptr_array_index (priv->widgets, ssw_sheet_axis_rtl (axis) ? \
		       (priv->widgets->len - i - 1) : i);



#define Foreach_Item_Fwd {int i; for (i = 0; i < priv->widgets->len; i ++){ \
  GtkWidget *item \
  = g_ptr_array_index (priv->widgets, i);

#define EndFor }} while(0)

enum
  {
    PROP_0,
    PROP_ADJUSTMENT,
    PROP_ORIENTATION
  };


static void
__axis_set_value (SswSheetAxis *axis, gdouble x)
{
  PRIV_DECL (axis);

  if (ssw_sheet_axis_rtl (axis))
    {
      gdouble u = gtk_adjustment_get_upper (priv->adjustment);
      gdouble ps = gtk_adjustment_get_page_size (priv->adjustment);
      gtk_adjustment_set_value (priv->adjustment, u - ps - x);
    }
    else
      gtk_adjustment_set_value (priv->adjustment, x);
}

static gdouble
__axis_get_value (SswSheetAxis *axis)
{
  PRIV_DECL (axis);

  if (ssw_sheet_axis_rtl (axis))
    {
      gdouble u = gtk_adjustment_get_upper (priv->adjustment);
      gdouble ps = gtk_adjustment_get_page_size (priv->adjustment);
      return u - ps - gtk_adjustment_get_value (priv->adjustment);
    }
  else
    return gtk_adjustment_get_value (priv->adjustment);
}

static void
stopped (GtkGesture *g,	 gpointer ud)
{
  SswSheetAxis *axis = SSW_SHEET_AXIS (ud);

  PRIV_DECL (axis);

  /* I don't know why this is necessary. The gesture should not have
     got this far if this is the case. */
  GdkWindow *win = gtk_widget_get_window (GTK_WIDGET (axis));
  if (gdk_window_get_cursor (win) == priv->resize_cursor)
    {
      priv->n_press = 0;
      return;
    }

  if (priv->n_press == 1 && priv->button == 1)
    g_signal_emit (axis, signals [HEADER_CLICKED], 0,
		   priv->press_id, priv->state);


  if (priv->n_press == 2 && priv->button == 1)
    g_signal_emit (axis, signals [HEADER_DOUBLE_CLICKED], 0,
		   priv->press_id, priv->state);

  priv->n_press = 0;
  priv->button = 0;
  priv->state = 0;
}


static void
button_pressed (GtkGesture *g,
		gint n_press, gdouble x, gdouble y, gpointer ud)
{
  SswSheetAxis *axis = SSW_SHEET_AXIS (ud);

  const GdkEvent *e = gtk_gesture_get_last_event
    (g, gtk_gesture_get_last_updated_sequence (g));

  /* This can happen if the gesture is denied for any reason */
  if (!e || e->type != GDK_BUTTON_PRESS)
    return;

  GdkWindow *win = ((const GdkEventButton *)e)->window;
  GObject *widget = NULL;
  gdk_window_get_user_data (win, (gpointer*)&widget);
  gint id = GPOINTER_TO_INT (g_object_get_data (widget, "item-id"));

  guint button = ((const GdkEventButton *)e)->button;
  guint state = ((const GdkEventButton *)e)->state;

  g_signal_emit (axis, signals [HEADER_BUTTON_PRESSED], 0, id, button, state);
}

static void
button_released (GtkGesture *g,
		 gint n_press, gdouble x, gdouble y, gpointer ud)
{
  SswSheetAxis *axis = SSW_SHEET_AXIS (ud);
  PRIV_DECL (axis);
  const GdkEvent *e = gtk_gesture_get_last_event (g,
					    gtk_gesture_get_last_updated_sequence (g));

  /* This can happen if the gesture is denied for any reason */
  if (!e || e->type != GDK_BUTTON_RELEASE)
    return;

  GdkWindow *win = ((const GdkEventButton *)e)->window;
  GObject *widget = NULL;
  gdk_window_get_user_data (win, (gpointer*)&widget);
  gint id = GPOINTER_TO_INT (g_object_get_data (widget, "item-id"));

  if (!gtk_widget_get_sensitive (GTK_WIDGET (widget)))
    return;

  priv->n_press = n_press;
  priv->press_id = id;
  priv->button = ((const GdkEventButton *)e)->button;
  priv->state = ((const GdkEventButton *)e)->state;

  g_signal_emit (axis, signals [HEADER_BUTTON_RELEASED], 0,
		 priv->press_id, priv->button, priv->state);
}

static GtkWidget *
get_widget (SswSheetAxis *axis, guint index)
{
  gpointer item;
  GtkWidget *old_widget = NULL;
  GtkWidget *new_widget;
  PRIV_DECL (axis);

  item = g_list_model_get_item (priv->model, index);

  if (priv->pool->len > 0)
    old_widget = g_ptr_array_remove_index_fast (priv->pool, 0);

  new_widget = cell_fill_func (axis, item, old_widget, index);

  g_object_set_data (G_OBJECT (new_widget), "item-id", GINT_TO_POINTER (index));

  gtk_widget_set_sensitive (new_widget, index < ssw_sheet_axis_get_size (axis));
  if (index >= ssw_sheet_axis_get_size (axis))
    g_object_set (new_widget,
		  "has-tooltip", FALSE,
		  NULL);

  /* Check if the size of this item has been specifically overridden
     by the user.  If it has set the size request accordingly. */
  gpointer size = g_hash_table_lookup (priv->size_override, GINT_TO_POINTER (index));
  if (size)
    {
      if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
       gtk_widget_set_size_request (new_widget, GPOINTER_TO_INT (size), -1);
      else
       gtk_widget_set_size_request (new_widget, -1, GPOINTER_TO_INT (size));
    }

  if (g_object_is_floating (new_widget))
    g_object_ref_sink (new_widget);

  g_assert (GTK_IS_WIDGET (new_widget));

  /* We just enforce this here. */
  gtk_widget_show (new_widget);

  return new_widget;
}


static void
insert_child_ginternal (SswSheetAxis *axis, GtkWidget *widget, guint index)
{
  PRIV_DECL (axis);

  g_object_ref (widget);

  gtk_widget_set_parent_window (widget, priv->bin_window);
  gtk_widget_set_parent (widget, GTK_WIDGET (axis));

  g_ptr_array_insert (priv->widgets, index, widget);
}

static void
remove_child_ginternal (SswSheetAxis *axis, GtkWidget *widget)
{
  PRIV_DECL (axis);

  g_object_unref (widget);

  gtk_widget_unparent (widget);
  g_ptr_array_remove (priv->widgets, widget);
  g_ptr_array_add (priv->pool, widget);
}

static inline gint
bin_start (SswSheetAxis *axis)
{
  return -__axis_get_value (axis) +
    PRIV (axis)->bin_start_diff;
}


static void
free_limit (gpointer data)
{
  g_slice_free (SswGeometry, data);
}

static void
position_children (SswSheetAxis *axis)
{
  GtkAllocation alloc;
  GtkAllocation child_alloc;
  PRIV_DECL (axis);
  gint offset = 0;

  gtk_widget_get_allocation (GTK_WIDGET (axis), &alloc);

  priv->set_q_offset (&child_alloc, 0);
  priv->set_q_size (&child_alloc, priv->get_q_size (&alloc));


  if (axis->cell_limits)
    g_ptr_array_free (axis->cell_limits, TRUE);
  axis->cell_limits = g_ptr_array_new_full (priv->widgets->len,
					    free_limit);

  axis->last_cell = priv->model_to;
  axis->first_cell = priv->model_from;

  Foreach_Item
    gint size;

  priv->get_preferred_p_for_q (item, priv->get_q_size (&alloc),
			       &size, NULL);
  priv->set_p_offset (&child_alloc, offset);
  priv->set_p_size (&child_alloc, size);
  gtk_widget_size_allocate (item, &child_alloc);

  offset += size;

  EndFor;


  guint width = gtk_widget_get_allocated_width (GTK_WIDGET (axis));

  offset = 0;
  Foreach_Item_Fwd
    gint size;

  priv->get_preferred_p_for_q (item, priv->get_q_size (&alloc),
			       &size, NULL);

  SswGeometry *geom = g_slice_new (SswGeometry);
  geom->position = offset + bin_start (axis);

  if (ssw_sheet_axis_rtl (axis))
    geom->position = width - geom->position - size;

  geom->size = size;
  g_ptr_array_insert (axis->cell_limits, i, geom);

  offset += size;
  EndFor;
}

static inline gint
item_size (SswSheetAxis *axis, GtkWidget *w)
{
  PRIV_DECL (axis);
  gint min, nat;
  priv->get_preferred_p_for_q (w,
			       priv->get_allocated_q_size (GTK_WIDGET(axis)),
			       &min, &nat);

  return nat;
}

static inline gint
item_start (SswSheetAxis *axis, guint index)
{
  gint y = 0;
  gint i;
  PRIV_DECL (axis);

  for (i = 0; i < index; i++)
    y += item_size (axis, g_ptr_array_index (priv->widgets, i));

  return y;
}

static gint
estimated_widget_size (SswSheetAxis *axis)
{
  gint avg_widget_size = 0;
  PRIV_DECL (axis);

  Foreach_Item avg_widget_size += item_size (axis, item);
  EndFor;

  if (priv->widgets->len > 0)
    avg_widget_size /= priv->widgets->len;
  else
    return 0;

  return avg_widget_size;
}

static gboolean
bin_window_full (SswSheetAxis *axis)
{
  gint bin_size;
  gint widget_size;
  PRIV_DECL (axis);

  if (gtk_widget_get_realized (GTK_WIDGET (axis)))
    bin_size = priv->get_window_p_size (priv->bin_window);
  else
    bin_size = 0;

  widget_size = priv->get_allocated_p_size (GTK_WIDGET (axis));

  /*
   * We consider the bin_window "full" if either the end of it is out of view,
   * OR it already contains all the items.
   */
  return (bin_start (axis) + bin_size > widget_size) ||
    (priv->model_to - priv->model_from ==
     ssw_sheet_axis_get_extent (axis));
}

static gint
estimated_list_size (SswSheetAxis *axis,
                     guint *start_part, guint *end_part)
{
  gint avg_size;
  gint start_widgets;
  gint end_widgets;
  gint exact_size = 0;
  PRIV_DECL (axis);

  avg_size = estimated_widget_size (axis);
  start_widgets = priv->model_from;
  end_widgets = ssw_sheet_axis_get_extent (axis) - priv->model_to;

  g_assert (start_widgets + end_widgets + priv->widgets->len ==
            ssw_sheet_axis_get_extent (axis));

  Foreach_Item
    exact_size += item_size (axis, item);
  EndFor;

  if (start_part)
    *start_part = start_widgets * avg_size;

  if (end_part)
    *end_part = end_widgets * avg_size;

  return (start_widgets * avg_size) + exact_size + (end_widgets * avg_size);
}


static void
update_bin_window (SswSheetAxis *axis)
{
  GtkAllocation alloc;
  gint size = 0;
  PRIV_DECL (axis);

  gtk_widget_get_allocation (GTK_WIDGET (axis), &alloc);

  Foreach_Item
    gint min = item_size (axis, item);
  g_assert (min >= 0);
  size += min;
  EndFor;

  if (size == 0)
    size = 1;

  if (priv->orientation == GTK_ORIENTATION_VERTICAL)
    {
      if (size != gdk_window_get_height (priv->bin_window) ||
	  alloc.width != gdk_window_get_width (priv->bin_window))
	{
	  gdk_window_move_resize (priv->bin_window,
				  0, bin_start (axis), alloc.width, size);
	}
      else
	gdk_window_move (priv->bin_window, 0, bin_start (axis));
    }
  else if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      gint xoffset = ssw_sheet_axis_rtl (axis)
	? (alloc.width - bin_start (axis) -
	   priv->get_window_p_size (priv->bin_window))
	: bin_start (axis);

      if (size != gdk_window_get_width (priv->bin_window) ||
	  alloc.height != gdk_window_get_height (priv->bin_window))
	{
	  gdk_window_move_resize (priv->bin_window,
				  xoffset,
				  0, size, alloc.height);
	}
      else
	gdk_window_move (priv->bin_window, xoffset, 0);
    }
}

static void
configure_adjustment (SswSheetAxis *axis)
{
  gint widget_size;
  gint list_size;
  gdouble cur_upper;
  gdouble cur_value;
  gdouble page_size;
  PRIV_DECL (axis);

  widget_size = priv->get_allocated_p_size (GTK_WIDGET (axis));
  list_size = estimated_list_size (axis, NULL, NULL);
  cur_upper = gtk_adjustment_get_upper (priv->adjustment);
  page_size = gtk_adjustment_get_page_size (priv->adjustment);
  cur_value = __axis_get_value (axis);

  if ((gint) cur_upper != MAX (list_size, widget_size))
    {
      /*g_message ("New upper: %d (%d, %d)", MAX (list_size, widget_size), list_size, widget_size); */
      gtk_adjustment_set_upper (priv->adjustment,
                                MAX (list_size, widget_size));
    }
  else if (list_size == 0)
    gtk_adjustment_set_upper (priv->adjustment, widget_size);


  if ((gint) page_size != widget_size)
    gtk_adjustment_set_page_size (priv->adjustment, widget_size);

  if (cur_value > cur_upper - widget_size)
    {
      __axis_set_value (axis, cur_upper - widget_size);
    }
}

#define NOT_BOTH(A,B) !((A)&&(B))

static void
ensure_visible_widgets (SswSheetAxis *axis, gboolean force_reload)
{
  GtkWidget *widget = GTK_WIDGET (axis);
  gint bin_size;
  gboolean start_removed, start_added, end_removed, end_added;
  PRIV_DECL (axis);

  if (!gtk_widget_get_mapped (widget))
    return;

  const gint widget_size = priv->get_allocated_p_size (widget);
  bin_size = priv->get_window_p_size (priv->bin_window);

  if (bin_size == 1)
    bin_size = 0;

  /* This "out of sight" case happens when the new value is so different from the old one
   * that we rather just remove all widgets and adjust the model_from/model_to values
   */
  if (bin_start (axis) + bin_size < 0 || bin_start (axis) >= widget_size || force_reload)
    {
      gint avg_widget_size = estimated_widget_size (axis);
      gdouble percentage;
      gdouble value = __axis_get_value (axis);
      gdouble upper = gtk_adjustment_get_upper (priv->adjustment);
      gdouble page_size = gtk_adjustment_get_page_size (priv->adjustment);
      guint start_widget_index;
      gint i;

      /* g_message ("OUT OF SIGHT"); */

      for (i = priv->widgets->len - 1; i >= 0; i--)
        remove_child_ginternal (axis, g_ptr_array_index (priv->widgets, i));
      bin_size = 0;             /* The window is empty now, obviously */

      g_assert (priv->widgets->len == 0);

      /* Percentage of the current adjustment value */
      percentage = value / (upper - page_size);

      start_widget_index =
        (guint) (ssw_sheet_axis_get_extent (axis) * percentage);

      if (start_widget_index > ssw_sheet_axis_get_extent (axis))
        {
          /* XXX Can this still happen? */
          priv->model_from = ssw_sheet_axis_get_extent (axis);
          priv->model_to = ssw_sheet_axis_get_extent (axis);
          priv->bin_start_diff = value + page_size;
          g_assert (FALSE);
        }
      else
        {
          priv->model_from = start_widget_index;
          priv->model_to = start_widget_index;
          priv->bin_start_diff = start_widget_index * avg_widget_size;
        }

      g_assert (priv->model_from >= 0);
      g_assert (priv->model_to >= 0);
      g_assert (priv->model_from <= ssw_sheet_axis_get_extent (axis));
      g_assert (priv->model_to <= ssw_sheet_axis_get_extent (axis));

      if (bin_start (axis) > widget_size)
        g_critical ("Start of widget is outside of the container");
    }

  /* Remove start widgets */
  {
    gint i;
    start_removed = FALSE;

    for (i = priv->widgets->len - 1; i >= 0; i--)
      {
        GtkWidget *w = g_ptr_array_index (priv->widgets, i);
        gint w_size = item_size (axis, w);
        if (bin_start (axis) + item_start (axis, i) + w_size < 0)
          {
            /* g_message ("Removing start widget %d", i); */
            priv->bin_start_diff += w_size;
            bin_size -= w_size;
            remove_child_ginternal (axis, w);
            priv->model_from++;
            start_removed = TRUE;
          }
      }
  }

  /* Add start widgets */
  {
    start_added = FALSE;
    while (priv->model_from > 0 && bin_start (axis) >= 0)
      {
        GtkWidget *new_widget;
        gint min;
        priv->model_from--;

        new_widget = get_widget (axis, priv->model_from);
        g_assert (new_widget != NULL);
        insert_child_ginternal (axis, new_widget, 0);
        min = item_size (axis, new_widget);
        priv->bin_start_diff -= min;
        bin_size += min;
        start_added = TRUE;
        /* g_message ("Adding start widget for index %d", priv->model_from); */
      }
  }

  /* Remove end widgets */
  {
    end_removed = FALSE;
    gint i;
    for (i = priv->widgets->len - 1; i >= 0; i--)
      {
        GtkWidget *w = g_ptr_array_index (priv->widgets, i);
        gint y = bin_start (axis) + item_start (axis, i);

        /*g_message ("%d: %d + %d > %d", i, bin_start (axis), item_start (axis, i), widget_size); */
        if (y > widget_size)
          {
            /*g_message ("Removing widget %d", i); */
            gint w_size = item_size (axis, w);
            remove_child_ginternal (axis, w);
            bin_size -= w_size;
            priv->model_to--;
            end_removed = TRUE;
          }
        else
          break;
      }
  }

  /* Insert end widgets */
  {
    end_added = FALSE;
    /*g_message ("%d + %d <= %d", bin_start (axis), bin_size, widget_size); */
    while (bin_start (axis) + bin_size <= widget_size &&
           priv->model_to < ssw_sheet_axis_get_extent (axis))
      {
        GtkWidget *new_widget;
        gint min;

        /* g_message ("Inserting end widget for position %u at %u", priv->model_to, priv->widgets->len); */
        new_widget = get_widget (axis, priv->model_to);
        insert_child_ginternal (axis, new_widget, priv->widgets->len);
        min = item_size (axis, new_widget);
        bin_size += min;

        priv->model_to++;
        end_added = TRUE;
      }
  }

  {
    gdouble new_upper;
    guint start_part, end_part;
    gint bin_window_y = bin_start (axis);

    new_upper = estimated_list_size (axis, &start_part, &end_part);

    if (new_upper > gtk_adjustment_get_upper (priv->adjustment))
      priv->bin_start_diff =
        MAX (start_part, __axis_get_value (axis));
    else
      priv->bin_start_diff =
        MIN (start_part, __axis_get_value (axis));

    configure_adjustment (axis);

    /*g_message ("Setting value to %f - %d", priv->bin_start_diff, bin_window_y); */
    __axis_set_value (axis,
                              priv->bin_start_diff - bin_window_y);
    if (__axis_get_value (axis) < priv->bin_start_diff)
      {
        __axis_set_value (axis, priv->bin_start_diff);
        /*g_message ("Case 1"); */
      }

    if (bin_start (axis) > 0)
      {
        /*g_message ("CRAP"); */
        priv->bin_start_diff = __axis_get_value (axis);
      }
  }

  update_bin_window (axis);
  position_children (axis);
  configure_adjustment (axis);

  gtk_widget_queue_draw (widget);

  g_signal_emit (axis, signals [CHANGED], 0);
}

static void
value_changed_cb (GtkAdjustment *adjustment, gpointer user_data)
{
  SswSheetAxis *axis = SSW_SHEET_AXIS (user_data);
  ensure_visible_widgets (axis, FALSE);
}

static void
items_changed_cb (GListModel *model,
                  guint position,
                  guint removed, guint added, gpointer user_data)
{
  SswSheetAxis *axis = SSW_SHEET_AXIS (user_data);
  gint i;
  PRIV_DECL (user_data);

  /* If the change is out of our visible range anyway,
   *we don't care. */
  if (position > priv->model_to && bin_window_full (axis))
    {
      configure_adjustment (axis);
      return;
    }

  /* Empty the current view */
  for (i = priv->widgets->len - 1; i >= 0; i--)
    remove_child_ginternal (axis, g_ptr_array_index (priv->widgets, i));

  priv->model_to = priv->model_from;
  update_bin_window (axis);
  ensure_visible_widgets (axis, FALSE);
}

/* GtkContainer vfuncs {{{ */
static void
__add (GtkContainer *container, GtkWidget *child)
{
  g_error ("Don't use that.");
}

static void
__remove (GtkContainer *container, GtkWidget *child)
{
  g_assert (gtk_widget_get_parent (child) == GTK_WIDGET (container));
  /*g_assert (gtk_widget_get_parent_window (child) == priv->bin_window); XXX ??? */
}

static void
__forall (GtkContainer *w,
          gboolean include_ginternals,
          GtkCallback callback, gpointer callback_data)
{
  SswSheetAxis *axis = SSW_SHEET_AXIS (w);
  PRIV_DECL (axis);

  Foreach_Item
    (*callback) (item, callback_data);
  EndFor;
}

/* }}} */

/* GtkWidget vfuncs {{{ */
static void
__size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  PRIV_DECL (widget);
  gboolean size_changed =
    priv->get_p_size (allocation) !=
    priv->get_allocated_p_size (widget);

  gtk_widget_set_allocation (widget, allocation);

  position_children ((SswSheetAxis *) widget);

  if (gtk_widget_get_realized (widget))
    {
      gdk_window_move_resize (gtk_widget_get_window (widget),
                              allocation->x, allocation->y,
                              allocation->width, allocation->height);

      update_bin_window ((SswSheetAxis *) widget);
    }


  //  if (!bin_window_full ((SswSheetAxis *)widget) && size_changed)
  if (size_changed)
    ensure_visible_widgets ((SswSheetAxis *) widget, FALSE);

  configure_adjustment ((SswSheetAxis *) widget);
}

static gboolean
__draw (GtkWidget *w, cairo_t *ct)
{
  SswSheetAxis *axis = SSW_SHEET_AXIS (w);
  GtkAllocation alloc;
  GtkStyleContext *context;

  PRIV_DECL (axis);

  context = gtk_widget_get_style_context (w);
  gtk_widget_get_allocation (w, &alloc);

  gtk_render_background (context, ct, 0, 0, alloc.width, alloc.height);

  if (gtk_cairo_should_draw_window (ct, priv->bin_window))
    Foreach_Item
      gtk_container_propagate_draw (GTK_CONTAINER (axis), item, ct);
  EndFor;

  return GDK_EVENT_PROPAGATE;
}

static void
__realize (GtkWidget *w)
{
  SswSheetAxis *axis = SSW_SHEET_AXIS (w);
  PRIV_DECL (axis);
  GtkAllocation allocation;
  GdkWindowAttr attributes = { 0, };
  GdkWindow *window;

  gtk_widget_get_allocation (w, &allocation);
  gtk_widget_set_realized (w, TRUE);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (w) |
    GDK_ALL_EVENTS_MASK;
  attributes.wclass = GDK_INPUT_OUTPUT;

  window = gdk_window_new (gtk_widget_get_parent_window (w),
                           &attributes, GDK_WA_X | GDK_WA_Y);
  gdk_window_set_user_data (window, w);
  gtk_widget_set_window (w, window);


  priv->bin_window =
    gdk_window_new (window, &attributes, GDK_WA_X | GDK_WA_Y);
  gtk_widget_register_window (w, priv->bin_window);
  gdk_window_show (priv->bin_window);

  Foreach_Item
    gtk_widget_set_parent_window (item, priv->bin_window);
  EndFor;

  GdkDisplay *display = gtk_widget_get_display (w);

  if (priv->orientation == GTK_ORIENTATION_VERTICAL)
    priv->resize_cursor = gdk_cursor_new_for_display (display, GDK_SB_V_DOUBLE_ARROW);
  else
    priv->resize_cursor = gdk_cursor_new_for_display (display, GDK_SB_H_DOUBLE_ARROW);
}

static void
__unrealize (GtkWidget *widget)
{
  PRIV_DECL (widget);

  g_object_unref (priv->resize_cursor);

  if (priv->bin_window != NULL)
    {
      gtk_widget_unregister_window (widget, priv->bin_window);
      gdk_window_destroy (priv->bin_window);
      priv->bin_window = NULL;
    }

  GTK_WIDGET_CLASS (ssw_sheet_axis_parent_class)->unrealize (widget);
}

static void
__map (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (ssw_sheet_axis_parent_class)->map (widget);

  ensure_visible_widgets ((SswSheetAxis *)widget, TRUE);
}

static void
__get_preferred_width (GtkWidget *widget, gint *min, gint *nat)
{
  *min = 0;
  *nat = 0;
}

static void
__get_preferred_height (GtkWidget *widget, gint *min, gint *nat)
{
  *min = 0;
  *nat = 0;
}

/* }}} */



static void
__set_orientation (GObject *object)
{
  PRIV_DECL (object);

  if (priv->orientation == GTK_ORIENTATION_VERTICAL)
    {
      g_object_set (object, "vexpand", TRUE, NULL);
      priv->get_allocated_p_size = gtk_widget_get_allocated_height;
      priv->get_allocated_q_size = gtk_widget_get_allocated_width;
      priv->get_preferred_p_for_q =
        gtk_widget_get_preferred_height_for_width;
      priv->get_window_p_size = gdk_window_get_height;

      priv->set_q_offset = zz_set_alloc_x;

      priv->get_q_size = zz_get_alloc_width;
      priv->set_q_size = zz_set_alloc_width;

      priv->set_p_offset = zz_set_alloc_y;

      priv->get_p_size = zz_get_alloc_height;
      priv->set_p_size = zz_set_alloc_height;
    }
  else
    {
      g_object_set (object, "hexpand", TRUE, NULL);

      priv->get_allocated_p_size = gtk_widget_get_allocated_width;
      priv->get_allocated_q_size = gtk_widget_get_allocated_height;
      priv->get_preferred_p_for_q =
        gtk_widget_get_preferred_width_for_height;
      priv->get_window_p_size = gdk_window_get_width;

      priv->set_q_offset = zz_set_alloc_y;

      priv->get_q_size = zz_get_alloc_height;
      priv->set_q_size = zz_set_alloc_height;

      priv->set_p_offset = zz_set_alloc_x;

      priv->get_p_size = zz_get_alloc_width;
      priv->set_p_size = zz_set_alloc_width;
    }
}

static void
__set_pq_adjustments (GObject *object)
{
  if (PRIV (object)->adjustment)
    g_signal_connect (G_OBJECT (PRIV (object)->adjustment),
		      "value-changed", G_CALLBACK (value_changed_cb),
		      object);
}

/* GObject vfuncs {{{ */
static void
__set_property (GObject *object,
                guint prop_id, const GValue *value, GParamSpec *pspec)
{
  switch (prop_id)
    {
    case PROP_ADJUSTMENT:
      PRIV (object)->adjustment = g_value_get_object (value);
      __set_pq_adjustments (object);
      break;
    case PROP_ORIENTATION:
      PRIV (object)->orientation = g_value_get_enum (value);
      __set_orientation (object);
      __set_pq_adjustments (object);
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
    case PROP_ADJUSTMENT:
      g_value_set_object (value, PRIV (object)->adjustment);
      break;
    case PROP_ORIENTATION:
      g_value_set_enum (value, PRIV (object)->orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
__finalize (GObject *obj)
{
  SswSheetAxis *axis = SSW_SHEET_AXIS (obj);
  PRIV_DECL (obj);

  g_hash_table_destroy (priv->size_override);
  g_ptr_array_free (priv->pool, TRUE);
  g_ptr_array_free (priv->widgets, TRUE);
  gtk_target_list_unref (priv->drag_target_list);


  if (axis->cell_limits)
    g_ptr_array_free (axis->cell_limits, TRUE);

  g_object_unref (priv->button_gest);
  g_object_unref (priv->resize_gest);
  g_object_unref (priv->drag_gest);

  G_OBJECT_CLASS (ssw_sheet_axis_parent_class)->finalize (obj);
}

/* }}} */

GtkWidget *
ssw_sheet_axis_new (GtkOrientation orientation)
{
  return GTK_WIDGET (g_object_new (SSW_TYPE_SHEET_AXIS,
				   "orientation", orientation,
				   NULL));
}

static void
force_update (SswSheetAxis *axis)
{
  ensure_visible_widgets (axis, TRUE);
  gtk_widget_queue_draw (GTK_WIDGET (axis));
}

void
ssw_sheet_axis_set_model (SswSheetAxis *axis, GListModel *model)
{
  PRIV_DECL (axis);

  if (priv->model != NULL)
    g_object_unref (priv->model);

  PRIV (axis)->model = model;
  if (model != NULL)
    {
      g_signal_connect_object (G_OBJECT (model), "items-changed",
			       G_CALLBACK (items_changed_cb), axis, 0);
      g_object_ref (model);
    }

  g_signal_connect_object (model, "notify", G_CALLBACK (force_update),
			   axis, G_CONNECT_SWAPPED);
  ensure_visible_widgets (axis, TRUE);
}

GListModel *
ssw_sheet_axis_get_model (SswSheetAxis *axis)
{
  return PRIV (axis)->model;
}


static void
__direction_changed (GtkWidget *w, GtkTextDirection prev_dir)
{
  SswSheetAxis *axis = SSW_SHEET_AXIS (w);
  PRIV_DECL (axis);

  if (gtk_orientable_get_orientation (GTK_ORIENTABLE (w)) ==
      GTK_ORIENTATION_HORIZONTAL)
    {
      gdouble u = gtk_adjustment_get_upper (priv->adjustment);
      gdouble ps = gtk_adjustment_get_page_size (priv->adjustment);
      gdouble val = gtk_adjustment_get_value (priv->adjustment);

      gtk_adjustment_set_value (priv->adjustment, u - ps - val);
    }

  GTK_WIDGET_CLASS (ssw_sheet_axis_parent_class)->direction_changed (w, prev_dir);
}

static void
ssw_sheet_axis_class_init (SswSheetAxisClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (class);

  GParamSpec *adjust_spec =
    g_param_spec_object ("adjustment",
			 P_("Adjustment"),
			 P_("The Adjustment"),
			 GTK_TYPE_ADJUSTMENT,
			 G_PARAM_READWRITE);


  object_class->set_property = __set_property;
  object_class->get_property = __get_property;
  object_class->finalize = __finalize;

  widget_class->draw = __draw;
  widget_class->size_allocate = __size_allocate;
  widget_class->map = __map;
  widget_class->get_preferred_width = __get_preferred_width;
  widget_class->get_preferred_height = __get_preferred_height;
  widget_class->realize = __realize;
  widget_class->unrealize = __unrealize;
  widget_class->direction_changed = __direction_changed;

  container_class->add = __add;
  container_class->remove = __remove;
  container_class->forall = __forall;

  signals [CHANGED] =
    g_signal_new ("changed",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_FIRST,
		  0,
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE,
		  0);


  signals [HEADER_CLICKED] =
    g_signal_new ("header-clicked",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_FIRST,
		  0,
		  NULL, NULL,
		  ssw_cclosure_marshal_VOID__INT_UINT,
		  G_TYPE_NONE,
		  2,
		  G_TYPE_INT,
		  G_TYPE_UINT);

  signals [HEADER_DOUBLE_CLICKED] =
    g_signal_new ("header-double-clicked",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_FIRST,
		  0,
		  NULL, NULL,
		  ssw_cclosure_marshal_VOID__INT_UINT,
		  G_TYPE_NONE,
		  2,
		  G_TYPE_INT,
		  G_TYPE_UINT);

  signals [HEADER_BUTTON_PRESSED] =
    g_signal_new ("header-button-pressed",
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

  signals [HEADER_BUTTON_RELEASED] =
    g_signal_new ("header-button-released",
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


  signals [DRAG_N_DROP] =
    g_signal_new ("drag-n-dropped",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_FIRST,
		  0,
		  NULL, NULL,
		  ssw_cclosure_marshal_VOID__INT_INT,
		  G_TYPE_NONE,
		  2,
		  G_TYPE_INT,
		  G_TYPE_INT);


  g_object_class_install_property (object_class,
                                   PROP_ADJUSTMENT,
                                   adjust_spec);

  g_object_class_override_property (object_class, PROP_ORIENTATION,
                                    "orientation");
}

static gboolean
on_motion_notify (SswSheetAxis *axis, GdkEventMotion *e, gpointer ud)
{
  const gint threshold = 5;
  PRIV_DECL (axis);

  gdouble x, y;
  gdk_window_coords_to_parent (e->window, e->x, e->y, &x, &y);

  gdouble posn = (priv->orientation == GTK_ORIENTATION_HORIZONTAL) ? x : y;

  gint pos, size;
  ssw_sheet_axis_find_cell (axis, posn, &pos, &size);

  gint boundary =
    (fabs (pos - posn) < fabs (pos + size - posn)) ? pos : pos + size;

  gint diff = abs (boundary - posn);

  GdkWindow *win = gtk_widget_get_window (GTK_WIDGET (axis));

  if (diff < threshold && pos > 0)
    gdk_window_set_cursor (win, priv->resize_cursor);
  else
    gdk_window_set_cursor (win, NULL);

  return FALSE;
}

static void
on_resize_drag_begin (GtkGesture *g, gdouble x, gdouble y, gpointer ud)
{
  SswSheetAxis *axis = SSW_SHEET_AXIS (ud);
  PRIV_DECL (axis);

  gdouble posn = (priv->orientation == GTK_ORIENTATION_HORIZONTAL) ? x : y;

  gint pos, size;
  gint which = ssw_sheet_axis_find_cell (axis, posn, &pos, &size);

  gint target =
    (fabs (pos - posn) < fabs (pos + size - posn)) ? which - 1: which ;

  priv->resize_target = target;
}

static gboolean
do_resize (gpointer ud)
{
  SswSheetAxis *axis = SSW_SHEET_AXIS (ud);
  PRIV_DECL (axis);

  ssw_sheet_axis_override_size (axis, priv->resize_target, priv->new_size);

  return FALSE;
}

static void
on_resize_end (GtkGesture *g, gdouble x, gdouble y, gpointer ud)
{
  SswSheetAxis *axis = SSW_SHEET_AXIS (ud);
  PRIV_DECL (axis);

  gint old_size;
  gdouble delta = (priv->orientation == GTK_ORIENTATION_HORIZONTAL) ? x : y;

  ssw_sheet_axis_find_boundary (axis, priv->resize_target, NULL, &old_size);
  priv->new_size = old_size + delta;

  /* I have no idea why this must be in an idle callback.
     I get critical warnings if I call it directly */
  g_idle_add (do_resize, axis);
}

static void
on_resize_begin (GtkGesture *g, GdkEventSequence *seq, gpointer ud)
{
  SswSheetAxis *axis = SSW_SHEET_AXIS (ud);
  PRIV_DECL (axis);

  GdkWindow *win = gtk_widget_get_window (GTK_WIDGET (axis));
  if (gdk_window_get_cursor (win) == priv->resize_cursor)
    gtk_gesture_set_sequence_state (g, seq, GTK_EVENT_SEQUENCE_CLAIMED);
}

static gboolean
on_drag_drop (GtkWidget      *widget,
	      GdkDragContext *context,
	      gint            x_,
	      gint            y_,
	      guint           time,
	      gpointer        user_data)
{
  /* For reasons I don't understand, the x_ and y_ parameters of this
     function are wrong when the target is beyond the size of the axis.
     So we don't use them.  Instead we get the pointer position
     explicitly. */
  gint xx, yy;
  GdkWindow *win = gtk_widget_get_window (widget);
  GdkDevice *device = gdk_drag_context_get_device (context);
  gdk_window_get_device_position (win, device, &xx, &yy, NULL);

  gint posn;
  switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (widget)))
    {
    case GTK_ORIENTATION_HORIZONTAL:
      posn = xx;
      break;
    case GTK_ORIENTATION_VERTICAL:
      posn = yy;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  gint to = ssw_sheet_axis_find_cell (SSW_SHEET_AXIS (widget),  posn,
					 NULL, NULL);

  gint from = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
						  "from"));

  if (to >= ssw_sheet_axis_get_size (SSW_SHEET_AXIS (widget)))
    to = ssw_sheet_axis_get_size (SSW_SHEET_AXIS (widget));

  g_signal_emit (widget, signals [DRAG_N_DROP], 0, from, to);
  gtk_drag_finish (context, TRUE, TRUE, time);

  return TRUE;
}

static void
on_multi_press_begin (GtkGesture *g, GdkEventSequence *seq, gpointer ud)
{
  SswSheetAxis *axis = SSW_SHEET_AXIS (ud);
  PRIV_DECL (axis);

  GdkWindow *win = gtk_widget_get_window (GTK_WIDGET (axis));
  if (gdk_window_get_cursor (win) != priv->resize_cursor)
    {
      gtk_gesture_set_sequence_state (g, seq, GTK_EVENT_SEQUENCE_CLAIMED);
    }
}

/* Note: This is a GtkWidget handler, NOT a GtkGesture handler */
static void
update_pointer (GtkWidget      *widget,
               GdkDragContext *context,
               gpointer        user_data)
{
  /* This forces the pointer to move.  For some reason, without it,
     Dnd icon stays at 0,0 until the user moves the mouse. */
  GdkDevice *device = gdk_drag_context_get_device (context);
  GdkScreen *screen = NULL;
  gint x, y;
  gdk_device_get_position (device, &screen, &x, &y);
  gdk_device_warp (device, screen, x, y);
}

static void
setup_drag_operation (GtkGesture *g, gdouble x_, gdouble y_, gpointer ud)
{
  SswSheetAxis *axis = SSW_SHEET_AXIS (ud);
  PRIV_DECL (axis);

  GdkEventSequence *seq =
    gtk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (g));

  gdouble x, y;
  gtk_gesture_get_point (g, seq, &x, &y);

  gint posn;
  switch (gtk_orientable_get_orientation (GTK_ORIENTABLE (axis)))
    {
    case GTK_ORIENTATION_HORIZONTAL:
      posn = x;
      break;
    case GTK_ORIENTATION_VERTICAL:
      posn = y;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  gint which = ssw_sheet_axis_find_cell (axis,  posn, NULL, NULL);

  if (which < ssw_sheet_axis_get_size (axis))
    {
      gtk_gesture_set_sequence_state (g, seq, GTK_EVENT_SEQUENCE_CLAIMED);
      const GdkEvent *ev = gtk_gesture_get_last_event (g, seq);
      GdkEvent *e =  gdk_event_copy (ev);
      gtk_drag_begin_with_coordinates (GTK_WIDGET (axis),
					 priv->drag_target_list,
					 GDK_ACTION_MOVE, 1,
					 e, x, y);
      g_object_set_data (G_OBJECT (axis), "from", GINT_TO_POINTER (which));
      gdk_event_free (e);
    }
}

static void
ssw_sheet_axis_init (SswSheetAxis *axis)
{
  PRIV_DECL (axis);
  GtkStyleContext *context = gtk_widget_get_style_context (GTK_WIDGET (axis));

  priv->widgets = g_ptr_array_new ();
  priv->pool = g_ptr_array_new ();
  priv->model_from = 0;
  priv->model_to = 0;
  priv->bin_start_diff = 0;

  priv->size_override = g_hash_table_new (g_direct_hash, g_direct_equal);

  gtk_style_context_add_class (context, "list");
  axis->cell_limits = NULL;

  priv->resize_gest = gtk_gesture_drag_new (GTK_WIDGET (axis));
  priv->button_gest = gtk_gesture_multi_press_new (GTK_WIDGET (axis));
  priv->drag_gest = gtk_gesture_long_press_new (GTK_WIDGET (axis));

  g_object_set (priv->drag_gest, "delay-factor", 0.5, NULL);

  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (priv->drag_gest),
                                              GTK_PHASE_CAPTURE);

  g_signal_connect (axis, "drag-begin", G_CALLBACK (update_pointer),  axis);

  g_signal_connect (priv->drag_gest, "pressed", G_CALLBACK (setup_drag_operation),
		    axis);

  /* Listen on all buttons */
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (priv->button_gest), 0);

  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (priv->button_gest),
  					      GTK_PHASE_CAPTURE);

  g_signal_connect (priv->button_gest, "begin", G_CALLBACK (on_multi_press_begin), axis);
  g_signal_connect (priv->button_gest, "released", G_CALLBACK (button_released), axis);
  g_signal_connect (priv->button_gest, "pressed", G_CALLBACK (button_pressed), axis);
  g_signal_connect (priv->button_gest, "stopped", G_CALLBACK (stopped), axis);
  priv->n_press = 0;
  g_signal_connect (axis, "motion-notify-event", G_CALLBACK (on_motion_notify), axis);


  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (priv->resize_gest),
  					      GTK_PHASE_CAPTURE);

  g_signal_connect (priv->resize_gest, "begin", G_CALLBACK (on_resize_begin), axis);
  g_signal_connect (priv->resize_gest, "drag-begin", G_CALLBACK (on_resize_drag_begin),
		    axis);

  g_signal_connect (priv->resize_gest, "drag-end", G_CALLBACK (on_resize_end), axis);


  GtkTargetEntry te = {"move-axis-item", GTK_TARGET_SAME_APP, 0};
  gtk_drag_dest_set (GTK_WIDGET (axis), GTK_DEST_DEFAULT_ALL, &te, 1, GDK_ACTION_MOVE);
  priv->drag_target_list = gtk_target_list_new (&te, 1);

  g_signal_connect (axis, "drag-drop", G_CALLBACK (on_drag_drop),
		    NULL);
}

static void ssw_sheet_axis_jump_start_with_offset (SswSheetAxis *axis, gint whereto, gint offs);

static void ssw_sheet_axis_jump_end_with_offset (SswSheetAxis *axis, gint whereto, gint offs);


/* Request a specific size for item POS.  Overriding the natural
   size for the item. */
void
ssw_sheet_axis_override_size (SswSheetAxis *axis, gint pos, gint size)
{
  PRIV_DECL (axis);

  g_hash_table_insert (priv->size_override,
		       GINT_TO_POINTER (pos),
		       GINT_TO_POINTER (size));


  guint width = gtk_widget_get_allocated_width (GTK_WIDGET (axis));


  gint loc;
  gint start_cell =
    ssw_sheet_axis_find_cell (axis,
			      ssw_sheet_axis_rtl (axis) ? width : 0,
			      &loc, NULL);

  ensure_visible_widgets (axis, TRUE);

  if (ssw_sheet_axis_rtl (axis))
    ssw_sheet_axis_jump_start_with_offset (axis, 1 + start_cell, loc - width);
  else
    ssw_sheet_axis_jump_start_with_offset (axis, start_cell, loc);
}


/*
   Find the row/column at POS.
   This is linear in the number of visible items.
   Returns -1 id there is no row/column at POS.
*/
gint
ssw_sheet_axis_find_cell (SswSheetAxis *axis,  gdouble pos, gint *location, gint *size)
{
  gint x = ssw_sheet_axis_rtl (axis) ? axis->first_cell - 1: axis->last_cell;
  gint prev = G_MAXINT;

  if (axis->cell_limits->len == 0)
    return -1;

  int i;
  const int step = ssw_sheet_axis_rtl (axis) ? -1 : +1;
  for (i = axis->cell_limits->len - 1; i >= 0; --i)
    {
      const SswGeometry *geom = g_ptr_array_index (axis->cell_limits,
						   ssw_sheet_axis_rtl (axis)? axis->cell_limits->len - 1 - i: i);
      const gint end = geom->position;

      /* This list should be monotonically decreasing.
	 If it isn't, then something has gone wrong */
      g_return_val_if_fail (end < prev, -1);
      x -= step;
      if (pos >= end)
	{
	  if (location)
	    *location = end;
	  if (size)
	    *size = geom->size;
	  break;
	}
      prev = end;
    }

  return x;
}


/*
  Find the dimensions of the row/column indexed by POS and
  store the results in LOCATION and SIZE.

  Returns zero on success.  If POS is outside the visible
  range, then returns -1 if it is above/left-of the visible
  range or +1 if it is below/right-of it.
*/
gint
ssw_sheet_axis_find_boundary (SswSheetAxis *axis,  gint pos,
			      gint *location, gint *size)
{
  if (pos >= axis->last_cell)
    return +1;

  if (pos < axis->first_cell)
    return -1;

  const gint i = pos - axis->first_cell;
  const SswGeometry *geom = g_ptr_array_index (axis->cell_limits, i);
  const gint end = geom->position;

  if (location)
    *location = end;
  if (size)
    *size = geom->size;

  return 0;
}





/* ssw_sheet_axis_get_size and ssw_sheet_axis_get_extent return similar
   things.

   ssw_sheet_axis_get_size returns the number of items in the axis.

   ssw_sheet_axis_get_extent returns the number of items in the axis PLUS
   the number of items needed to fill 90% of the screen beyond the last item.

   This allows for the arrangement where, when the sheet is scrolled to its
   upper extreme, the real estate beyond the last cell gets filled with
   "inactive" rows/columns.
*/

gint
ssw_sheet_axis_get_size (SswSheetAxis *axis)
{
  GListModel *model = ssw_sheet_axis_get_model (axis);
  g_return_val_if_fail (model, 0);
  return g_list_model_get_n_items (model);
}

gint
ssw_sheet_axis_get_extent (SswSheetAxis *axis)
{
  PRIV_DECL (axis);

  gint avg_widget_size = estimated_widget_size (axis);
  if (avg_widget_size == 0)
    avg_widget_size = 28;

  gint n_items = ssw_sheet_axis_get_size (axis) ;

  gint overshoot = priv->get_allocated_p_size (GTK_WIDGET (axis));

  /* If the sheet is empty, then increase the overall size
     by 1, so that an ugly empty space at the end of the axis is not visible */
  if (n_items == 0)
    n_items += 1;
  else
    overshoot *= 0.9;

  return n_items + overshoot / avg_widget_size;
}


/* Return the number of the first cell which is *fully* visible */
gint
ssw_sheet_axis_get_first (SswSheetAxis *axis)
{
  PRIV_DECL (axis);
  gint widget_size = priv->get_allocated_p_size (GTK_WIDGET (axis));
  const gint extremity = ssw_sheet_axis_rtl (axis) ? widget_size : 0;
  gint position, size;
  gint cell = ssw_sheet_axis_find_cell (axis, extremity, &position, &size);

  if (cell == -1)
    return 0;

  if (ssw_sheet_axis_rtl (axis))
    {
      if (position + size > widget_size)
	cell++;
    }
  else
    {
      if (position < 0)
	cell++;
    }

  return cell;
}

gint
ssw_sheet_axis_get_last (SswSheetAxis *axis)
{
  PRIV_DECL (axis);
  gint widget_size = priv->get_allocated_p_size (GTK_WIDGET (axis));
  const gint extremity = ssw_sheet_axis_rtl (axis) ? 0 : widget_size;
  gint position, size;
  gint cell = ssw_sheet_axis_find_cell (axis, extremity, &position, &size);

  if (cell == -1)
    return ssw_sheet_axis_get_size (axis);

  if (ssw_sheet_axis_rtl (axis))
    {
      if (position < 0)
	cell--;
    }
  else
    {
      if (position + size > widget_size)
	cell--;
    }

  return cell;
}

gint
ssw_sheet_axis_get_visible_size (SswSheetAxis *axis)
{
  return ssw_sheet_axis_get_last (axis) - ssw_sheet_axis_get_first (axis) + 1;
}

void
ssw_sheet_axis_info (SswSheetAxis *axis)
{
  PRIV_DECL (axis);
  Foreach_Item
    g_print ("Size %d\n", item_size (axis, item));
  EndFor;
}

static gint
adj_start (gint widget_size, gint location, gint extent)
{
  return -location;
}

static gint
adj_end (gint widget_size, gint location, gint extent)
{
  return widget_size - (location + extent);
}

/*
   Adjusts the scroll position to point to WHERETO.
   INIT_LOCATION and INIT_EXTENT are the initial
   values of WHERETO's geometry.
 */
static void
fine_adjust (SswSheetAxis *axis, gint whereto, gint init_location,
	     gint init_extent,  gint (*adj) (gint, gint, gint), gint offs)
{
  PRIV_DECL (axis);
  gint widget_size = priv->get_allocated_p_size (GTK_WIDGET (axis));

  gint delta = adj (widget_size, init_location, init_extent) + offs;
  gint old_delta = delta + 1;

  gdouble old_v = -1;
  while (abs(delta) > 0)
    {
      gint location, extent;
      gdouble v = __axis_get_value (axis);
      if (ssw_sheet_axis_rtl (axis))
	delta = - delta;
      __axis_set_value (axis, v - delta);
      ssw_sheet_axis_find_boundary (axis,  whereto, &location, &extent);
      delta = adj (widget_size, location, extent) + offs;
      if (delta == old_delta && old_v == v)
	{
	  /* Should never happen ... */
	  g_warning ("%s %p: Cannot seek to item %d.\n",
		     G_OBJECT_TYPE_NAME (axis), axis, whereto);
	  /* ... but avoid an infinite loop if something goes wrong */
	  break;
	}
      old_delta = delta;
      old_v = v;
    }
  priv->bin_start_diff += delta;
  ensure_visible_widgets (axis, FALSE);
}

/* Scrolls the axis, such that the item  WHERETO is approximately
   in the middle of the screen.
   As a side effect, the LOCATION and EXTENT are populated
   with the location and extent of the WHERETO.
   This function is bounded by O(log n) in the number of items.
 */
static void
course_adjust (SswSheetAxis *axis, gint whereto, gint *location, gint *extent)
{
  PRIV_DECL (axis);
  gint direction ;
  gdouble k =
    gtk_adjustment_get_upper (priv->adjustment) /
    gtk_adjustment_get_page_size (priv->adjustment);
  gint old_direction = 0;
  do
    {
      direction = ssw_sheet_axis_find_boundary (axis,  whereto,
						location, extent);
      gdouble v = __axis_get_value (axis);
      gdouble ps = gtk_adjustment_get_page_size (priv->adjustment);
      gdouble u = gtk_adjustment_get_upper (priv->adjustment);
      if (v + ps * k * direction > u - ps)
	gtk_adjustment_set_upper (priv->adjustment, u + ps);

      __axis_set_value (axis, v + ps * k * direction);
      if (old_direction == -direction)
	k /= 2.0;
      old_direction = direction;
    }
  while (direction != 0);
}


/* Scroll the axis such that WHERETO is at the end */
void
ssw_sheet_axis_jump_end_with_offset (SswSheetAxis *axis, gint whereto, gint offs)
{
  PRIV_DECL (axis);
  g_return_if_fail (whereto <= ssw_sheet_axis_get_size (axis));

  gdouble upper = gtk_adjustment_get_upper (priv->adjustment);
  gdouble page_size = gtk_adjustment_get_page_size (priv->adjustment);

  __axis_set_value (axis,   (upper - page_size) * (whereto + 1)
			    / (gdouble) ssw_sheet_axis_get_extent (axis));

  gint location, extent;
  course_adjust (axis, whereto, &location, &extent);
  fine_adjust (axis, whereto, location, extent,
	       ssw_sheet_axis_rtl (axis) ? adj_start : adj_end, offs);
}


/* Scroll the axis such that WHERETO is at the start */
static void
ssw_sheet_axis_jump_start_with_offset (SswSheetAxis *axis, gint whereto, gint offs)
{
  PRIV_DECL (axis);
  g_return_if_fail (whereto < ssw_sheet_axis_get_size (axis));

  gdouble upper = gtk_adjustment_get_upper (priv->adjustment);
  gdouble page_size = gtk_adjustment_get_page_size (priv->adjustment);

  __axis_set_value (axis, (upper - page_size) * (whereto)
			    / (gdouble) ssw_sheet_axis_get_extent (axis));

  gint location, extent;
  course_adjust (axis, whereto, &location, &extent);
  fine_adjust (axis, whereto, location, extent,
	       ssw_sheet_axis_rtl (axis) ? adj_end : adj_start, offs);
}


/* Scroll the axis such that WHERETO's start is (approxiamately) in the
   middle */
void
ssw_sheet_axis_jump_center (SswSheetAxis *axis, gint whereto)
{
  PRIV_DECL (axis);
  g_return_if_fail (whereto < ssw_sheet_axis_get_size (axis));

  gdouble upper = gtk_adjustment_get_upper (priv->adjustment);
  gdouble page_size = gtk_adjustment_get_page_size (priv->adjustment);

  __axis_set_value (axis, (upper - page_size) * (whereto)
			    / (gdouble) ssw_sheet_axis_get_extent (axis));

  gint location, extent;
  course_adjust (axis, whereto, &location, &extent);
}


/* Scroll the axis such that WHERETO is at the start */
void
ssw_sheet_axis_jump_start (SswSheetAxis *axis, gint whereto)
{
  ssw_sheet_axis_jump_start_with_offset (axis, whereto, 0);
}


/* Scroll the axis such that WHERETO is at the end */
void
ssw_sheet_axis_jump_end (SswSheetAxis *axis, gint whereto)
{
  ssw_sheet_axis_jump_end_with_offset (axis, whereto, 0);
}

