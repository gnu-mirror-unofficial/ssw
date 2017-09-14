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

#include <config.h>
#include <atk/atk.h>
#include "ssw-cell.h"

#define P_(X) (X)

static  gchar *
__get_text (AtkText *text,
	    gint    start_offset,
	    gint    end_offset)
{
  const gchar *s = SSW_CELL (text)->text;
  if (end_offset == -1)
    end_offset = g_utf8_strlen (s, -1);

  return g_utf8_substring (s, start_offset, end_offset);
}



static void
__atk_text_init (AtkTextIface *iface)
{
  iface->get_text = __get_text;
}



G_DEFINE_TYPE_WITH_CODE (SswCell, ssw_cell, ATK_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (ATK_TYPE_TABLE_CELL, NULL)
			 G_IMPLEMENT_INTERFACE (ATK_TYPE_TEXT, __atk_text_init));

static void
__finalize (GObject *obj)
{
  SswCell *cell = SSW_CELL (obj);
  g_free (cell->text);
}


enum
  {
    PROP_0,
    PROP_CONTENT
  };

static void
__set_property (GObject *object,
                guint prop_id, const GValue *value, GParamSpec *pspec)
{
  SswCell *cell = SSW_CELL (object);

  switch (prop_id)
    {
    case PROP_CONTENT:
      g_free (cell->text);
      cell->text = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
__get_property (GObject * object,
                guint prop_id, GValue *value, GParamSpec *pspec)
{
  SswCell *cell = SSW_CELL (object);

  switch (prop_id)
    {
    case PROP_CONTENT:
      g_value_set_pointer (value, cell->text);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ssw_cell_class_init (SswCellClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = __finalize;


  GParamSpec *content_spec =
    g_param_spec_pointer ("content",
			  P_("Content"),
			  P_("The contents of the cell in utf8"),
			  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  object_class->set_property = __set_property;
  object_class->get_property = __get_property;


  g_object_class_install_property (object_class,
				   PROP_CONTENT,
				   content_spec);
}

static void
ssw_cell_init (SswCell *obj)
{
  SswCell *cell = SSW_CELL (obj);
  cell->text = NULL;
}
