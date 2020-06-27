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

#ifndef __SSW_XPANED_H__
#define __SSW_XPANED_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define SSW_TYPE_XPANED            (ssw_xpaned_get_type ())
#define SSW_XPANED(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SSW_TYPE_XPANED, SswXpaned))
#define SSW_XPANED_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SSW_TYPE_XPANED, SswXpanedClass))
#define SSW_IS_XPANED(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SSW_TYPE_XPANED))
#define SSW_IS_XPANED_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SSW_TYPE_XPANED))
#define SSW_XPANED_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SSW_TYPE_XPANED, SswXpanedClass))


typedef struct _SswXpaned              SswXpaned;
typedef struct _SswXpanedPrivate       SswXpanedPrivate;
typedef struct _SswXpanedClass         SswXpanedClass;

struct _SswXpaned
{
  GtkContainer container;

  /*< private >*/
  gint n_children;
  GSList *childs;
  gfloat vpos;
  gfloat hpos;
  GdkWindow *handle;
  GdkCursor *cursor;
  gboolean button_pressed;
};

/**
 * SswXpanedClass:
 * @parent_class: The parent class.
 */
struct _SswXpanedClass
{
  GtkContainerClass parent_class;

  /*< private >*/
};


GType       ssw_xpaned_get_type            (void) G_GNUC_CONST;
GtkWidget*  ssw_xpaned_new                 (void);
G_END_DECLS

#endif /* __SSW_XPANED_H__ */
