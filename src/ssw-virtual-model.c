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
#include <gtk/gtk.h>
#include "ssw-virtual-model.h"

#define P_(X) (X)

enum  {ITEMS_CHANGED,
       n_SIGNALS};

static guint signals [n_SIGNALS];

static  gint
__iter_n_children (GtkTreeModel *tree_model,
                   GtkTreeIter  *iter)
{
  SswVirtualModel *m = SSW_VIRTUAL_MODEL (tree_model);
  return m->rows;
}

static gint
__get_n_columns  (GtkTreeModel *tree_model)
{
  SswVirtualModel *m = SSW_VIRTUAL_MODEL (tree_model);

  return m->cols;
}


static gboolean
__iter_nth_child  (GtkTreeModel *tree_model,
                   GtkTreeIter  *iter,
                   GtkTreeIter  *parent,
                   gint          n)
{
  SswVirtualModel *m = SSW_VIRTUAL_MODEL (tree_model);

  g_assert (parent == NULL);

  iter->stamp = m->stamp;
  iter->user_data = GINT_TO_POINTER (n);
  return TRUE;
}

static void
__get_value (GtkTreeModel *tree_model,
             GtkTreeIter  *iter,
             gint          column,
             GValue       *value)
{
  SswVirtualModel *m = SSW_VIRTUAL_MODEL (tree_model);
  g_return_if_fail (iter->stamp == m->stamp);

  g_value_init (value, G_TYPE_STRING);

  gint row = GPOINTER_TO_INT (iter->user_data);
  g_value_take_string (value, g_strdup_printf ("r%dc%d", row, column));
}

static GType
__get_type (GtkTreeModel *tree_model,
            gint col)
{
  SswVirtualModel *m = SSW_VIRTUAL_MODEL (tree_model);
  return G_TYPE_STRING;
}


static GtkTreePath *
__get_path (GtkTreeModel *tree_model,
            GtkTreeIter  *iter)
{
  SswVirtualModel *m = SSW_VIRTUAL_MODEL (tree_model);
  g_return_val_if_fail (iter->stamp == m->stamp, NULL);
  return gtk_tree_path_new_from_indices (GPOINTER_TO_INT (iter->user_data), -1);
}

static GtkTreeModelFlags
__get_flags (GtkTreeModel *tm)
{
  return GTK_TREE_MODEL_LIST_ONLY;
}


static void
__init_iface (GtkTreeModelIface *iface)
{
  iface->iter_n_children = __iter_n_children;
  iface->get_n_columns = __get_n_columns;
  iface->iter_nth_child = __iter_nth_child;
  iface->get_value = __get_value;
  iface->get_column_type = __get_type;
  iface->get_path = __get_path;
  iface->get_flags = __get_flags;
}

G_DEFINE_TYPE_WITH_CODE (SswVirtualModel, ssw_virtual_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL, __init_iface));


static void
__finalize (GObject *obj)
{
  SswVirtualModel *model = SSW_VIRTUAL_MODEL (obj);
}


enum
  {
   PROP_0,
   PROP_COLS,
   PROP_ROWS
  };


/* GObject vfuncs {{{ */
static void
__set_property (GObject *object,
                guint prop_id, const GValue *value, GParamSpec * pspec)
{
  SswVirtualModel *m = SSW_VIRTUAL_MODEL (object);
  gint r;

  switch (prop_id)
    {
    case PROP_COLS:
      m->cols = g_value_get_uint (value);
      break;
    case PROP_ROWS:
      {
        gint old_rows = m->rows;
        gint n = g_value_get_uint (value);
        g_return_if_fail (n >= 0);
        m->rows = n;
        if (old_rows != -1)
          {
            /* Rows inserted */
            for (r = old_rows; r < m->rows; ++r)
              {
                GtkTreeIter iter;
                gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (m), &iter, NULL, r);
                GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL(m), &iter);
                g_signal_emit_by_name (object, "row-inserted",
                                       path, &iter);

                gtk_tree_path_free (path);
              }
            /* Rows deleted */
            for (r = m->rows; r < old_rows; ++r)
              {
                GtkTreePath *path =
                  gtk_tree_path_new_from_indices (r, -1);

                g_signal_emit_by_name (object, "row-deleted", path);

                gtk_tree_path_free (path);
              }
            gint diff = (m->rows - old_rows);
            g_signal_emit_by_name (object, "items-changed", old_rows,
                                   (diff > 0) ? 0 : -diff,
                                   (diff > 0) ? diff : 0);
          }
      }
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
  SswVirtualModel *m = SSW_VIRTUAL_MODEL (object);
  switch (prop_id)
    {
    case PROP_COLS:
      g_value_set_uint (value, m->cols);
      break;
    case PROP_ROWS:
      g_value_set_uint (value, m->rows);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
ssw_virtual_model_class_init (SswVirtualModelClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = __set_property;
  object_class->get_property = __get_property;

  GParamSpec *cols_spec =
    g_param_spec_uint ("columns",
                       P_("Columns"),
                       P_("The number of columns in the model"),
                       0, UINT_MAX, 10000,
                       G_PARAM_READWRITE);

  GParamSpec *rows_spec =
    g_param_spec_uint ("rows",
                       P_("Rows"),
                       P_("The number of rows in the model"),
                       0, UINT_MAX, 10000,
                       G_PARAM_READWRITE);


  g_object_class_install_property (object_class,
                                   PROP_COLS,
                                   cols_spec);

  g_object_class_install_property (object_class,
                                   PROP_ROWS,
                                   rows_spec);

  object_class->finalize = __finalize;

  signals [ITEMS_CHANGED] =
    g_signal_new ("items-changed",
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_generic,
                  G_TYPE_NONE,
                  3,
                  G_TYPE_UINT,
                  G_TYPE_UINT,
                  G_TYPE_UINT);
}

static void
ssw_virtual_model_init (SswVirtualModel *obj)
{
  SswVirtualModel *model = SSW_VIRTUAL_MODEL (obj);
  model->rows = -1;
  model->cols = -1;
  model->stamp = g_random_int ();
}


GObject *
ssw_virtual_model_new (void)
{
  return g_object_new (SSW_TYPE_VIRTUAL_MODEL, NULL);
}
