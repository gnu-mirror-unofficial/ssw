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

/* A container widget which constrains its child's allocation to the
   its own size.
*/


#ifndef _SSW_CONSTRAINT_H
#define _SSW_CONSTRAINT_H

#include <gtk/gtk.h>

struct _SswConstraint
{
  GtkBin parent_instance;

  gint hconstraint;
  gint vconstraint;
};

struct _SswConstraintClass
{
  GtkBinClass parent_class;
};

#define SSW_TYPE_CONSTRAINT ssw_constraint_get_type ()

G_DECLARE_FINAL_TYPE (SswConstraint, ssw_constraint, SSW, CONSTRAINT, GtkBin)

GtkWidget *ssw_constraint_new (void);

#endif
