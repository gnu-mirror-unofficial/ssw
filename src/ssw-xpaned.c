/*
  A widget to display and manipulate tabular data
  Copyright (C) 2017  John Darrington

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

#include "config.h"

#include <gtk/gtk.h>
#include "ssw-xpaned.h"

#include <string.h>

const gint HANDLE_WIDTH = 10;
const gint HANDLE_LENGTH = 100;

#define P_(X) X

enum
  {
    LAST_PROP
  };

G_DEFINE_TYPE (SswXpaned, ssw_xpaned, GTK_TYPE_CONTAINER)

static void
ssw_xpaned_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
  SswXpaned *box = SSW_XPANED (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ssw_xpaned_get_property (GObject    *object,
			 guint       prop_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
  SswXpaned *box = SSW_XPANED (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

struct _SswXpanedChild
{
  GtkWidget *w;
  gint left;
  gint top;
};

typedef struct _SswXpanedChild SswXpanedChild;

static void
get_adjusted_position (SswXpaned *xpaned, gfloat *hpos, gfloat *vpos)
{
  *hpos = xpaned->hpos;
  *vpos = xpaned->vpos;

  gboolean visibility[4] = {FALSE};

  /* If an entire row/column is not visible, then the vpos/hpos must
     be 1.0 */
  GSList *node;
  for (node = xpaned->childs; node; node = node->next)
    {
      SswXpanedChild *ssw_child = node->data;
      GtkWidget *child = ssw_child->w;

      if (gtk_widget_is_visible (child))
	{
	  visibility [ssw_child->top * 2 + ssw_child->left] = TRUE;
	}
    }

  if (!visibility[0] && !visibility[1])
    *vpos = 1.0;
  if (!visibility[2] && !visibility[3])
    *vpos = 1.0;

  const gfloat end =
    (gtk_widget_get_direction (GTK_WIDGET (xpaned)) == GTK_TEXT_DIR_RTL)
    ? 0.0 : 1.0;

  if (!visibility[0] && !visibility[2])
    *hpos = end;
  if (!visibility[1] && !visibility[3])
    *hpos = end;
}

static gboolean
__draw (GtkWidget *w, cairo_t   *cr)
{
  SswXpaned *xpaned = SSW_XPANED (w);

  gboolean blocked = GTK_WIDGET_CLASS (ssw_xpaned_parent_class)->draw (w, cr);
  if (blocked)
    return TRUE;

  gfloat vpos ;
  gfloat hpos ;

  gint height = gtk_widget_get_allocated_height (w);
  gint width = gtk_widget_get_allocated_width (w);

  get_adjusted_position (xpaned, &hpos, &vpos);

  GtkStyleContext *sc = gtk_widget_get_style_context (w);
  {
    GdkRGBA *color = NULL;

    GtkStateFlags flags = gtk_style_context_get_state (sc);

    gtk_style_context_get (sc,
			   flags,
			   "background-color",
			   &color, NULL);

    GtkCssProvider *cp = gtk_css_provider_new ();
    gchar *css = g_strdup_printf ("* {background-color: rgba(%d, %d, %d, 0.25);}",
				  (int) (100.0 * color->red),
				  (int) (100.0 * color->green),
				  (int) (100.0 * color->blue));

    gtk_css_provider_load_from_data (cp, css, strlen (css), 0);
    g_free (css);
    gdk_rgba_free (color);

    gtk_style_context_add_provider (sc, GTK_STYLE_PROVIDER (cp),
				    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_object_unref (cp);
  }

  /* Draw the horizontal handle */
  if (vpos > 0 && vpos < 1.0)
    gtk_render_handle (sc, cr,
		       width * hpos - HANDLE_LENGTH / 2,
		       height * vpos - HANDLE_WIDTH / 2,
		       HANDLE_LENGTH,
		       HANDLE_WIDTH);

  /* Draw the vertical handle */
  if (hpos > 0 && hpos < 1.0)
    gtk_render_handle (sc, cr,
		       width * hpos - HANDLE_WIDTH / 2,
		       height * vpos - HANDLE_LENGTH / 2,
		       HANDLE_WIDTH,
		       HANDLE_LENGTH);

  return FALSE;
}

static void
__size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  SswXpaned *xpaned = SSW_XPANED (widget);
  gint i;
  gfloat vpos ;
  gfloat hpos ;

  get_adjusted_position (xpaned, &hpos, &vpos);

  GSList *node;
  for (node = xpaned->childs; node; node = node->next)
    {
      SswXpanedChild *ssw_child = node->data;
      GtkWidget *child = ssw_child->w;

      GtkAllocation alloc;

      if (!gtk_widget_is_visible (child))
        continue;

      {
	gint minimum;
	gtk_widget_get_preferred_width (child, &minimum, NULL);


	if (ssw_child->left == (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL))
	  {
	    alloc.x = allocation->x;
	    alloc.width = allocation->width * hpos;
	  }
	else
	  {
	    alloc.x = allocation->x + allocation->width * hpos;
	    if (hpos > 0 && hpos < 1)
	      alloc.x += HANDLE_WIDTH / 2;
	    alloc.width = allocation->width * (1.0 - hpos);
	  }
	if (hpos > 0 && hpos < 1)
	  alloc.width -= HANDLE_WIDTH / 2;
	if (alloc.width < minimum)
	  alloc.width = minimum;

      }
      {
	gint minimum;
	gtk_widget_get_preferred_height (child, &minimum, NULL);
	if (ssw_child->top == 0)
	  {
	    alloc.y = allocation->y;
	    alloc.height = allocation->height * vpos;
	  }
	else
	  {
	    alloc.y = allocation->y + allocation->height * vpos;
	    if (vpos > 0 && vpos < 1)
	      alloc.y += HANDLE_WIDTH / 2;
	    alloc.height = allocation->height * (1.0 - vpos);
	  }
	if (vpos > 0 && vpos < 1)
	  alloc.height -= HANDLE_WIDTH / 2;

	if (alloc.height < minimum)
	  alloc.height = minimum;
      }

      gtk_widget_size_allocate (child, &alloc);
    }

  if (xpaned->handle)
    {
      gint x = hpos * allocation->width - HANDLE_WIDTH / 2;
      gint y = vpos * allocation->height - HANDLE_WIDTH / 2;
      x += allocation->x;
      y += allocation->y;
      gdk_window_move (xpaned->handle, x, y);
    }

  GTK_WIDGET_CLASS (ssw_xpaned_parent_class)->size_allocate (widget, allocation);
}

static void
__get_preferred_width (GtkWidget       *widget,
		       gint            *minimum_width,
		       gint            *natural_width)
{
  SswXpaned *xpaned = SSW_XPANED (widget);

  gint left_half_min = 0;
  gint right_half_min = 0;
  gint left_half_nat = 0;
  gint right_half_nat = 0;

  GSList *node;
  for (node = xpaned->childs; node; node = node->next)
    {
      SswXpanedChild *ssw_child = node->data;
      GtkWidget *child = ssw_child->w;

      gint quadm, quadn;
      gtk_widget_get_preferred_width (child, &quadm, &quadn);

      if (ssw_child->left == 0)
	{
	  left_half_min = MAX (left_half_min, quadm);
	  left_half_nat = MAX (left_half_nat, quadm);
	}
      else
	{
	  right_half_min = MAX (right_half_min, quadn);
	  right_half_nat = MAX (right_half_nat, quadn);
	}
    }

  if (minimum_width)
    {
      *minimum_width = left_half_min + right_half_min;
      if (left_half_min > 0 && right_half_min > 0)
	*minimum_width += HANDLE_WIDTH;
    }

  if (natural_width)
    {
      *natural_width = left_half_nat + right_half_nat;
      if (left_half_nat > 0 && right_half_nat > 0)
	*natural_width += HANDLE_WIDTH;
    }
}

static void
__get_preferred_height (GtkWidget       *widget,
			gint            *minimum_height,
			gint            *natural_height)
{
  SswXpaned *xpaned = SSW_XPANED (widget);

  gint top_half_min = 0;
  gint bottom_half_min = 0;
  gint top_half_nat = 0;
  gint bottom_half_nat = 0;

  GSList *node;
  for (node = xpaned->childs; node; node = node->next)
    {
      SswXpanedChild *ssw_child = node->data;
      GtkWidget *child = ssw_child->w;

      gint quadm, quadn;
      gtk_widget_get_preferred_height (child, &quadm, &quadn);

      if (ssw_child->top == 0)
	{
	  top_half_min = MAX (top_half_min, quadm);
	  top_half_nat = MAX (top_half_nat, quadm);
	}
      else
	{
	  bottom_half_min = MAX (bottom_half_min, quadn);
	  bottom_half_nat = MAX (bottom_half_nat, quadn);
	}
    }

  if (minimum_height)
    {
      *minimum_height = top_half_min + bottom_half_min;
      if (top_half_min > 0 && bottom_half_min > 0)
	*minimum_height += HANDLE_WIDTH;
    }

  if (natural_height)
    {
      *natural_height = top_half_nat + bottom_half_nat;
      if (top_half_nat > 0 && bottom_half_nat > 0)
	*natural_height += HANDLE_WIDTH;
    }
}


static gboolean
__button_event (GtkWidget *widget,
		GdkEventButton  *event)
{
  SswXpaned *xpaned = SSW_XPANED (widget);

  if (event->button != 1)
    return FALSE;

  if (event->type == GDK_BUTTON_PRESS)
    xpaned->button_pressed = TRUE;
  else if (event->type == GDK_BUTTON_RELEASE)
    xpaned->button_pressed = FALSE;

  return FALSE;
}


static void
__direction_changed (GtkWidget *widget, GtkTextDirection previous_direction)
{
  gtk_widget_queue_resize (widget);
}


static SswXpanedChild *
find_xpaned_child (SswXpaned *xpaned, GtkWidget *widget)
{
  GSList *node;
  for (node = xpaned->childs; node; node = node->next)
    {
      SswXpanedChild *ssw_child = node->data;
      if (ssw_child->w == widget)
        return ssw_child;
    }

  return NULL;
}

static void
__add (GtkContainer *cont, GtkWidget *child)
{
  g_return_if_fail (SSW_IS_XPANED (cont));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == NULL);

  SswXpaned *xpaned = SSW_XPANED (cont);

  g_return_if_fail (xpaned->n_children < 4);

  SswXpanedChild *ssw_child = g_malloc (sizeof (SswXpanedChild));
  ssw_child->left = -1;
  ssw_child->top = -1;
  ssw_child->w = child;

  xpaned->childs = g_slist_prepend (xpaned->childs, ssw_child);

  gtk_widget_set_parent (child, GTK_WIDGET (cont));
}

static void
__remove (GtkContainer *cont, GtkWidget *child)
{
  g_return_if_fail (SSW_IS_XPANED (cont));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == GTK_WIDGET (cont));

  SswXpaned *xpaned = SSW_XPANED (cont);
  SswXpanedChild *ssw_child = find_xpaned_child (xpaned, child);

  g_return_if_fail (ssw_child);

  xpaned->childs = g_slist_remove (xpaned->childs, ssw_child);
  g_free (ssw_child);

  xpaned->n_children--;

  //  g_critical ("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
  gtk_widget_unparent (child);
}

static void
__forall (GtkContainer *cont, gboolean include_internals,
          GtkCallback callback, gpointer callback_data)
{
  SswXpaned *xpaned = SSW_XPANED (cont);

  GSList *copy = g_slist_copy (xpaned->childs);
  GSList *node;
  for (node = copy; node; node = node->next)
    {
      SswXpanedChild *child = node->data;
      callback (child->w, callback_data);
    }
  g_slist_free (copy);
}

static GType
__child_type (GtkContainer   *container)
{
  g_print ("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
  return GTK_TYPE_WIDGET;
}

static GtkWidgetPath *
__get_path_for_child (GtkContainer *container,
		      GtkWidget    *child)

{
  g_print ("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
  return NULL;
}

enum
  {
    CHILD_PROP_0,
    CHILD_PROP_LEFT_ATTACH,
    CHILD_PROP_TOP_ATTACH,
    N_CHILD_PROPERTIES
  };

static GParamSpec *child_properties[N_CHILD_PROPERTIES] = { NULL, };

static void
__get_child_property (GtkContainer *container,
		      GtkWidget    *child,
		      guint         property_id,
		      GValue       *value,
		      GParamSpec   *pspec)
{
  SswXpaned *xpaned = SSW_XPANED (container);
  SswXpanedChild *xpaned_child = find_xpaned_child (xpaned, child);

  if (xpaned_child == NULL)
    {
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      return;
    }

  switch (property_id)
    {
    case CHILD_PROP_LEFT_ATTACH:
      g_value_set_int (value, xpaned_child->left);
      break;

    case CHILD_PROP_TOP_ATTACH:
      g_value_set_int (value, xpaned_child->top);
      break;

    default:
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
__set_child_property (GtkContainer *container,
		      GtkWidget    *child,
		      guint         property_id,
		      const GValue *value,
		      GParamSpec   *pspec)
{
  SswXpaned *xpaned = SSW_XPANED (container);
  SswXpanedChild *xpaned_child = find_xpaned_child (xpaned, child);

  if (xpaned_child == NULL)
    {
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      return;
    }

  switch (property_id)
    {
    case CHILD_PROP_LEFT_ATTACH:
      xpaned_child->left = g_value_get_int (value);
      break;

    case CHILD_PROP_TOP_ATTACH:
      xpaned_child->top = g_value_get_int (value);
      break;

    default:
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }

  if (xpaned_child->left == -1 || xpaned_child->top == -1)
    return;

  if (gtk_widget_is_visible (child) &&
      gtk_widget_is_visible (GTK_WIDGET (xpaned)))
    gtk_widget_queue_resize (child);
}

static gboolean
__motion_notify_event (GtkWidget *widget,
		       GdkEventMotion  *e)
{
  SswXpaned *xpaned = SSW_XPANED (widget);

  if (!xpaned->button_pressed)
    return FALSE;

  gdouble x = e->x;
  gdouble y = e->y;

  GdkWindow *window = gdk_window_get_parent (xpaned->handle);

  if (e->window != xpaned->handle)
    return FALSE;

  gdk_window_coords_to_parent (xpaned->handle,
			       e->x, e->y,
			       &x, &y);

  GtkAllocation allocation;
  gtk_widget_get_allocated_size (widget, &allocation, NULL);

  GSList *node;
  for (node = xpaned->childs; node; node = node->next)
    {
      SswXpanedChild *ssw_child = node->data;
      GtkWidget *child = ssw_child->w;

      int hmin, wmin;
      gtk_widget_get_preferred_width (child, &wmin, NULL);
      gtk_widget_get_preferred_height (child, &hmin, NULL);

      if (ssw_child->left ==
	  (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL))
	{
	  if (x < wmin + allocation.x )
	    x = wmin + allocation.x;
	}
      else
	{
	  if (x > allocation.x + allocation.width - wmin)
	    x = allocation.x + allocation.width - wmin;
	}

      if (ssw_child->top == 0)
	{
	  if (y < hmin + allocation.y)
	    y = hmin + allocation.y;
	}
      else
	{
	  if (y > allocation.y + allocation.height - hmin)
	    y = allocation.y + allocation.height - hmin;
	}
    }

  xpaned->hpos = (x - allocation.x) / (gfloat) allocation.width;
  xpaned->vpos = (y - allocation.y) / (gfloat) allocation.height;

  if (xpaned->hpos < 0)
    xpaned->hpos = 0;
  if (xpaned->hpos > 1)
    xpaned->hpos = 1;
  if (xpaned->vpos < 0)
    xpaned->vpos = 0;
  if (xpaned->vpos > 1)
    xpaned->vpos = 1;

  gtk_widget_queue_resize (widget);

  return FALSE;
}

static void
__realize (GtkWidget *widget)
{
  SswXpaned *xpaned = SSW_XPANED (widget);
  GdkWindow *window;
  GdkWindowAttr attributes;
  gint attributes_mask = 0;

  window = gtk_widget_get_parent_window (widget);
  gtk_widget_set_window (widget, window);
  g_object_ref (window);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.width = HANDLE_WIDTH;
  attributes.height = HANDLE_WIDTH;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK |
			    GDK_ENTER_NOTIFY_MASK |
			    GDK_LEAVE_NOTIFY_MASK |
			    GDK_POINTER_MOTION_MASK);
  xpaned->handle = gdk_window_new (window,
				   &attributes, attributes_mask);
  gtk_widget_register_window (widget, xpaned->handle);

  xpaned->cursor = gdk_cursor_new_from_name (gtk_widget_get_display (widget),
					     "move");

  gdk_window_set_cursor (xpaned->handle, xpaned->cursor);

  gtk_widget_set_realized (widget, TRUE);

  gdk_window_show (xpaned->handle);
}


static void
__unrealize (GtkWidget *widget)
{
  SswXpaned *xpaned = SSW_XPANED (widget);

  g_object_unref (xpaned->cursor);

  gtk_widget_unregister_window (GTK_WIDGET (xpaned), xpaned->handle);
  gdk_window_destroy (xpaned->handle);
}


static void
ssw_xpaned_class_init (SswXpanedClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (class);

  child_properties[CHILD_PROP_LEFT_ATTACH] =
    g_param_spec_int ("left-attach",
		      P_("Left attachment"),
		      P_("The column number to which the left side of the child should be attached"),
		      0, 1, 0,
		      G_PARAM_READWRITE);

  child_properties[CHILD_PROP_TOP_ATTACH] =
    g_param_spec_int ("top-attach",
		      P_("Top attachment"),
		      P_("The column number to which the top side of the child should be attached"),
		      0, 1, 0,
		      G_PARAM_READWRITE);



  object_class->set_property = ssw_xpaned_set_property;
  object_class->get_property = ssw_xpaned_get_property;

  widget_class->realize                        = __realize;
  widget_class->unrealize                      = __unrealize;
  widget_class->draw                           = __draw;
  widget_class->size_allocate                  = __size_allocate;
  widget_class->get_preferred_width            = __get_preferred_width;
  widget_class->get_preferred_height           = __get_preferred_height;
  widget_class->direction_changed              = __direction_changed;
  widget_class->motion_notify_event            = __motion_notify_event;
  widget_class->button_press_event             = __button_event;
  widget_class->button_release_event           = __button_event;

  container_class->add = __add;
  container_class->remove = __remove;
  container_class->forall = __forall;
  container_class->child_type = __child_type;
  //  container_class->check_resize = __check_resize;
  container_class->set_child_property = __set_child_property;
  container_class->get_child_property = __get_child_property;
  //  container_class->get_path_for_child = __get_path_for_child;
  //  gtk_container_class_handle_border_width (container_class);

  //  g_object_class_install_properties (object_class, LAST_PROP, props);
  gtk_container_class_install_child_properties (container_class,
						N_CHILD_PROPERTIES,
						child_properties);

  gtk_widget_class_set_accessible_role (widget_class, ATK_ROLE_FILLER);
  gtk_widget_class_set_css_name (widget_class, "xpaned");
}


static void
ssw_xpaned_init (SswXpaned *xpaned)
{
  gint i;
  GtkWidget *widget = GTK_WIDGET (xpaned);

  gtk_widget_set_has_window (widget, FALSE);
  gtk_widget_set_can_focus (widget, TRUE);

  gtk_widget_set_redraw_on_allocate (widget, TRUE);

  gtk_widget_set_hexpand (widget, TRUE);
  gtk_widget_set_vexpand (widget, TRUE);
  gtk_widget_set_hexpand_set (widget, TRUE);
  gtk_widget_set_vexpand_set (widget, TRUE);

  xpaned->vpos = 0.5;
  xpaned->hpos = 0.5;
  xpaned->cursor = NULL;
  xpaned->handle = NULL;
  xpaned->button_pressed = FALSE;
  xpaned->childs = NULL;
}
