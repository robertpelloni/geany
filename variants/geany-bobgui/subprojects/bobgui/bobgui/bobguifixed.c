/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

/**
 * BobguiFixed:
 *
 * Places its child widgets at fixed positions and with fixed sizes.
 *
 * `BobguiFixed` performs no automatic layout management.
 *
 * For most applications, you should not use this container! It keeps
 * you from having to learn about the other BOBGUI containers, but it
 * results in broken applications.  With `BobguiFixed`, the following
 * things will result in truncated text, overlapping widgets, and
 * other display bugs:
 *
 * - Themes, which may change widget sizes.
 *
 * - Fonts other than the one you used to write the app will of course
 *   change the size of widgets containing text; keep in mind that
 *   users may use a larger font because of difficulty reading the
 *   default, or they may be using a different OS that provides different fonts.
 *
 * - Translation of text into other languages changes its size. Also,
 *   display of non-English text will use a different font in many
 *   cases.
 *
 * In addition, `BobguiFixed` does not pay attention to text direction and
 * thus may produce unwanted results if your app is run under right-to-left
 * languages such as Hebrew or Arabic. That is: normally BOBGUI will order
 * containers appropriately for the text direction, e.g. to put labels to
 * the right of the thing they label when using an RTL language, but it can’t
 * do that with `BobguiFixed`. So if you need to reorder widgets depending on
 * the text direction, you would need to manually detect it and adjust child
 * positions accordingly.
 *
 * Finally, fixed positioning makes it kind of annoying to add/remove
 * UI elements, since you have to reposition all the other elements. This
 * is a long-term maintenance problem for your application.
 *
 * If you know none of these things are an issue for your application,
 * and prefer the simplicity of `BobguiFixed`, by all means use the
 * widget. But you should be aware of the tradeoffs.
 */

#include "config.h"

#include "bobguifixed.h"

#include "bobguifixedlayout.h"
#include "bobguiprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguibuildable.h"

typedef struct {
  BobguiLayoutManager *layout;
} BobguiFixedPrivate;

static void bobgui_fixed_buildable_iface_init (BobguiBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiFixed, bobgui_fixed, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiFixed)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_fixed_buildable_iface_init))

static void
bobgui_fixed_compute_expand (BobguiWidget *widget,
                          gboolean  *hexpand_p,
                          gboolean  *vexpand_p)
{
  BobguiWidget *w;
  gboolean hexpand = FALSE;
  gboolean vexpand = FALSE;

  for (w = bobgui_widget_get_first_child (widget);
       w != NULL;
       w = bobgui_widget_get_next_sibling (w))
    {
      hexpand = hexpand || bobgui_widget_compute_expand (w, BOBGUI_ORIENTATION_HORIZONTAL);
      vexpand = vexpand || bobgui_widget_compute_expand (w, BOBGUI_ORIENTATION_VERTICAL);
    }

  *hexpand_p = hexpand;
  *vexpand_p = vexpand;
}

static BobguiSizeRequestMode
bobgui_fixed_get_request_mode (BobguiWidget *widget)
{
  BobguiWidget *w;
  int wfh = 0, hfw = 0;

  for (w = bobgui_widget_get_first_child (widget);
       w != NULL;
       w = bobgui_widget_get_next_sibling (w))
    {
      BobguiSizeRequestMode mode = bobgui_widget_get_request_mode (w);

      switch (mode)
        {
        case BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH:
          hfw ++;
          break;
        case BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT:
          wfh ++;
          break;
        case BOBGUI_SIZE_REQUEST_CONSTANT_SIZE:
        default:
          break;
        }
    }

  if (hfw == 0 && wfh == 0)
    return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;
  else
    return wfh > hfw ?
        BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT :
        BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
bobgui_fixed_dispose (GObject *object)
{
  BobguiWidget *child;

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (object))))
    bobgui_fixed_remove (BOBGUI_FIXED (object), child);

  G_OBJECT_CLASS (bobgui_fixed_parent_class)->dispose (object);
}

static void
bobgui_fixed_class_init (BobguiFixedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = bobgui_fixed_dispose;

  widget_class->compute_expand = bobgui_fixed_compute_expand;
  widget_class->get_request_mode = bobgui_fixed_get_request_mode;

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_FIXED_LAYOUT);
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_fixed_buildable_add_child (BobguiBuildable *buildable,
                               BobguiBuilder   *builder,
                               GObject      *child,
                               const char   *type)
{
  if (BOBGUI_IS_WIDGET (child))
    bobgui_fixed_put (BOBGUI_FIXED (buildable), BOBGUI_WIDGET (child), 0, 0);
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bobgui_fixed_buildable_iface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_fixed_buildable_add_child;
}

static void
bobgui_fixed_init (BobguiFixed *self)
{
  BobguiFixedPrivate *priv = bobgui_fixed_get_instance_private (self);

  bobgui_widget_set_overflow (BOBGUI_WIDGET (self), BOBGUI_OVERFLOW_HIDDEN);

  priv->layout = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (self));
}

/**
 * bobgui_fixed_new:
 *
 * Creates a new `BobguiFixed`.
 *
 * Returns: a new `BobguiFixed`.
 */
BobguiWidget*
bobgui_fixed_new (void)
{
  return g_object_new (BOBGUI_TYPE_FIXED, NULL);
}

/**
 * bobgui_fixed_put:
 * @fixed: a `BobguiFixed`
 * @widget: the widget to add
 * @x: the horizontal position to place the widget at
 * @y: the vertical position to place the widget at
 *
 * Adds a widget to a `BobguiFixed` at the given position.
 */
void
bobgui_fixed_put (BobguiFixed  *fixed,
               BobguiWidget *widget,
               double     x,
               double     y)
{
  BobguiFixedPrivate *priv = bobgui_fixed_get_instance_private (fixed);
  BobguiFixedLayoutChild *child_info;
  GskTransform *transform = NULL;

  g_return_if_fail (BOBGUI_IS_FIXED (fixed));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));
  g_return_if_fail (_bobgui_widget_get_parent (widget) == NULL);

  bobgui_widget_set_parent (widget, BOBGUI_WIDGET (fixed));

  child_info = BOBGUI_FIXED_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (priv->layout, widget));

  transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (x, y));
  bobgui_fixed_layout_child_set_transform (child_info, transform);
  gsk_transform_unref (transform);
}

/**
 * bobgui_fixed_get_child_position:
 * @fixed: a `BobguiFixed`
 * @widget: a child of @fixed
 * @x: (out): the horizontal position of the @widget
 * @y: (out): the vertical position of the @widget
 *
 * Retrieves the translation transformation of the
 * given child `BobguiWidget` in the `BobguiFixed`.
 *
 * See also: [method@Bobgui.Fixed.get_child_transform].
 */
void
bobgui_fixed_get_child_position (BobguiFixed  *fixed,
                              BobguiWidget *widget,
                              double    *x,
                              double    *y)
{
  graphene_point_t p;

  g_return_if_fail (BOBGUI_IS_FIXED (fixed));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);
  g_return_if_fail (bobgui_widget_get_parent (widget) == BOBGUI_WIDGET (fixed));


  if (!bobgui_widget_compute_point (widget, BOBGUI_WIDGET (fixed),
                                 &GRAPHENE_POINT_INIT (0, 0), &p))
    graphene_point_init (&p, 0, 0);

  *x = p.x;
  *y = p.y;
}

/**
 * bobgui_fixed_set_child_transform:
 * @fixed: a `BobguiFixed`
 * @widget: a `BobguiWidget`, child of @fixed
 * @transform: (nullable): the transformation assigned to @widget
 *   to reset @widget's transform
 *
 * Sets the transformation for @widget.
 *
 * This is a convenience function that retrieves the
 * [class@Bobgui.FixedLayoutChild] instance associated to
 * @widget and calls [method@Bobgui.FixedLayoutChild.set_transform].
 */
void
bobgui_fixed_set_child_transform (BobguiFixed     *fixed,
                               BobguiWidget    *widget,
                               GskTransform *transform)
{
  BobguiFixedPrivate *priv = bobgui_fixed_get_instance_private (fixed);
  BobguiFixedLayoutChild *child_info;

  g_return_if_fail (BOBGUI_IS_FIXED (fixed));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));
  g_return_if_fail (bobgui_widget_get_parent (widget) == BOBGUI_WIDGET (fixed));

  child_info = BOBGUI_FIXED_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (priv->layout, widget));
  bobgui_fixed_layout_child_set_transform (child_info, transform);
}

/**
 * bobgui_fixed_get_child_transform:
 * @fixed: a `BobguiFixed`
 * @widget: a `BobguiWidget`, child of @fixed
 *
 * Retrieves the transformation for @widget set using
 * bobgui_fixed_set_child_transform().
 *
 * Returns: (transfer none) (nullable): a `GskTransform`
 */
GskTransform *
bobgui_fixed_get_child_transform (BobguiFixed  *fixed,
                               BobguiWidget *widget)
{
  BobguiFixedPrivate *priv = bobgui_fixed_get_instance_private (fixed);
  BobguiFixedLayoutChild *child_info;

  g_return_val_if_fail (BOBGUI_IS_FIXED (fixed), NULL);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (bobgui_widget_get_parent (widget) == BOBGUI_WIDGET (fixed), NULL);

  child_info = BOBGUI_FIXED_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (priv->layout, widget));
  return bobgui_fixed_layout_child_get_transform (child_info);
}

/**
 * bobgui_fixed_move:
 * @fixed: a `BobguiFixed`
 * @widget: the child widget
 * @x: the horizontal position to move the widget to
 * @y: the vertical position to move the widget to
 *
 * Sets a translation transformation to the given @x and @y
 * coordinates to the child @widget of the `BobguiFixed`.
 */
void
bobgui_fixed_move (BobguiFixed  *fixed,
                BobguiWidget *widget,
                double     x,
                double     y)
{
  BobguiFixedPrivate *priv = bobgui_fixed_get_instance_private (fixed);
  BobguiFixedLayoutChild *child_info;
  GskTransform *transform = NULL;

  g_return_if_fail (BOBGUI_IS_FIXED (fixed));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));
  g_return_if_fail (bobgui_widget_get_parent (widget) == BOBGUI_WIDGET (fixed));

  child_info = BOBGUI_FIXED_LAYOUT_CHILD (bobgui_layout_manager_get_layout_child (priv->layout,  widget));

  transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (x, y));
  bobgui_fixed_layout_child_set_transform (child_info, transform);
  gsk_transform_unref (transform);
}

/**
 * bobgui_fixed_remove:
 * @fixed: a `BobguiFixed`
 * @widget: the child widget to remove
 *
 * Removes a child from @fixed.
 */
void
bobgui_fixed_remove (BobguiFixed  *fixed,
                  BobguiWidget *widget)
{
  g_return_if_fail (BOBGUI_IS_FIXED (fixed));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));
  g_return_if_fail (bobgui_widget_get_parent (widget) == BOBGUI_WIDGET (fixed));

  bobgui_widget_unparent (widget);
}
