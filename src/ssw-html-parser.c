/*
  A widget to display and manipulate tabular data
  Copyright (C) 2017, 2020  John Darrington

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
#include <glib.h>

#include "ssw-paste.h"

static void
start_element (GMarkupParseContext *context,
               const gchar *element_name,
               const gchar **attribute_names,
               const gchar **attribute_values,
               gpointer user_data,
               GError **error)
{
  struct paste_state *state = user_data;

  if (0 == g_ascii_strcasecmp ("table", element_name))
    {
      state->row = 0;
      state->col = 0;
      state->cell_element = FALSE;
    }
  else if (0 == g_ascii_strcasecmp ("tr", element_name))
    {
      state->col = 0;
    }
  else if (0 == g_ascii_strcasecmp ("td", element_name))
    {
      state->cell_element = TRUE;
    }
}

static void
end_element (GMarkupParseContext *context,
             const gchar *element_name,
             gpointer user_data,
             GError **error)
{
  struct paste_state *state = user_data;

  if (0 == g_ascii_strcasecmp ("table", element_name))
    {
      state->row = -1;
      state->col = -1;
    }
  else if (0 == g_ascii_strcasecmp ("tr", element_name))
    {
      state->row++;
      state->col = -1;
    }
  else if (0 == g_ascii_strcasecmp ("td", element_name))
    {
      state->col++;
      state->cell_element = FALSE;
    }
}

static void
text_i (GMarkupParseContext *context,
      const gchar *text,
      gsize text_len,
      gpointer user_data,
      GError **error)
{
  struct paste_state *state = user_data;
  if (state->cell_element)
    {
      GString *str = g_string_new_len (text, text_len);
      ssw_sheet_paste_insert_datum (str->str, str->len, state);
      g_string_free (str, TRUE);
    }
}

static const GMarkupParser my_parser =
  {
   start_element,
   end_element,
   text_i,
   0
  };


/* A GtkClipboardReceived callback function, which parses
   the selection data SD as HTML, and sets PASTE_STATE accordingly.  */
void
ssw_html_parse (GtkClipboard *clip, GtkSelectionData *sd, gpointer paste_state)
{
  struct paste_state *ps = paste_state;
  SswSheet *sheet = SSW_SHEET (ps->sheet);
  const guchar *text = gtk_selection_data_get_data (sd);
  gint len = gtk_selection_data_get_length (sd);

  if (len < 0)
    {
      g_free (ps);
      return;
    }

  ps->cell_element = FALSE;
  GMarkupParseContext *ctx = g_markup_parse_context_new (&my_parser, 0, ps, 0);
  g_markup_parse_context_parse (ctx, (const gchar *) text, len, 0);
  g_markup_parse_context_unref (ctx);

  gtk_widget_queue_draw (GTK_WIDGET (sheet));
}
