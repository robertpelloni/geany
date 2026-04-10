/* bobguiscrollable.c
 * Copyright (C) 2008 Tadej Borovšak <tadeboro@gmail.com>
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

/**
 * BobguiScrollable:
 *
 * An interface for widgets with native scrolling ability.
 *
 * To implement this interface you should override the
 * [property@Bobgui.Scrollable:hadjustment] and
 * [property@Bobgui.Scrollable:vadjustment] properties.
 *
 * ## Creating a scrollable widget
 *
 * All scrollable widgets should do the following.
 *
 * - When a parent widget sets the scrollable child widget’s adjustments,
 *   the widget should connect to the [signal@Bobgui.Adjustment::value-changed]
 *   signal. The child widget should then populate the adjustments’ properties
 *   as soon as possible, which usually means queueing an allocation right away
 *   and populating the properties in the [vfunc@Bobgui.Widget.size_allocate]
 *   implementation.
 *
 * - Because its preferred size is the size for a fully expanded widget,
 *   the scrollable widget must be able to cope with underallocations.
 *   This means that it must accept any value passed to its
 *   [vfunc@Bobgui.Widget.size_allocate] implementation.
 *
 * - When the parent allocates space to the scrollable child widget,
 *   the widget must ensure the adjustments’ property values are correct and up
 *   to date, for example using [method@Bobgui.Adjustment.configure].
 *
 * - When any of the adjustments emits the [signal@Bobgui.Adjustment::value-changed]
 *   signal, the scrollable widget should scroll its contents.
 */

#include "config.h"

#include "bobguiscrollable.h"

#include "bobguiadjustment.h"
#include "bobguiprivate.h"
#include "bobguitypebuiltins.h"

G_DEFINE_INTERFACE (BobguiScrollable, bobgui_scrollable, G_TYPE_OBJECT)

static void
bobgui_scrollable_default_init (BobguiScrollableInterface *iface)
{
  GParamSpec *pspec;

  /**
   * BobguiScrollable:hadjustment:
   *
   * Horizontal `BobguiAdjustment` of the scrollable widget.
   *
   * This adjustment is shared between the scrollable widget and its parent.
   */
  pspec = g_param_spec_object ("hadjustment", NULL, NULL,
                               BOBGUI_TYPE_ADJUSTMENT,
                               BOBGUI_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_interface_install_property (iface, pspec);

  /**
   * BobguiScrollable:vadjustment:
   *
   * Vertical `BobguiAdjustment` of the scrollable widget.
   *
   * This adjustment is shared between the scrollable widget and its parent.
   */
  pspec = g_param_spec_object ("vadjustment", NULL, NULL,
                               BOBGUI_TYPE_ADJUSTMENT,
                               BOBGUI_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_interface_install_property (iface, pspec);

  /**
   * BobguiScrollable:hscroll-policy:
   *
   * Determines when horizontal scrolling should start.
   */
  pspec = g_param_spec_enum ("hscroll-policy", NULL, NULL,
			     BOBGUI_TYPE_SCROLLABLE_POLICY,
			     BOBGUI_SCROLL_MINIMUM,
			     BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);
  g_object_interface_install_property (iface, pspec);

  /**
   * BobguiScrollable:vscroll-policy:
   *
   * Determines when vertical scrolling should start.
   */
  pspec = g_param_spec_enum ("vscroll-policy", NULL, NULL,
			     BOBGUI_TYPE_SCROLLABLE_POLICY,
			     BOBGUI_SCROLL_MINIMUM,
			     BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);
  g_object_interface_install_property (iface, pspec);
}

/**
 * bobgui_scrollable_get_hadjustment:
 * @scrollable: a `BobguiScrollable`
 *
 * Retrieves the `BobguiAdjustment` used for horizontal scrolling.
 *
 * Returns: (transfer none) (nullable): horizontal `BobguiAdjustment`.
 */
BobguiAdjustment *
bobgui_scrollable_get_hadjustment (BobguiScrollable *scrollable)
{
  BobguiAdjustment *adj = NULL;

  g_return_val_if_fail (BOBGUI_IS_SCROLLABLE (scrollable), NULL);

  g_object_get (scrollable, "hadjustment", &adj, NULL);

  /* Horrid hack; g_object_get() returns a new reference but
   * that contradicts the memory management conventions
   * for accessors.
   */
  if (adj)
    g_object_unref (adj);

  return adj;
}

/**
 * bobgui_scrollable_set_hadjustment:
 * @scrollable: a `BobguiScrollable`
 * @hadjustment: (nullable): a `BobguiAdjustment`
 *
 * Sets the horizontal adjustment of the `BobguiScrollable`.
 */
void
bobgui_scrollable_set_hadjustment (BobguiScrollable *scrollable,
                                BobguiAdjustment *hadjustment)
{
  g_return_if_fail (BOBGUI_IS_SCROLLABLE (scrollable));
  g_return_if_fail (hadjustment == NULL || BOBGUI_IS_ADJUSTMENT (hadjustment));

  g_object_set (scrollable, "hadjustment", hadjustment, NULL);
}

/**
 * bobgui_scrollable_get_vadjustment:
 * @scrollable: a `BobguiScrollable`
 *
 * Retrieves the `BobguiAdjustment` used for vertical scrolling.
 *
 * Returns: (transfer none) (nullable): vertical `BobguiAdjustment`.
 */
BobguiAdjustment *
bobgui_scrollable_get_vadjustment (BobguiScrollable *scrollable)
{
  BobguiAdjustment *adj = NULL;

  g_return_val_if_fail (BOBGUI_IS_SCROLLABLE (scrollable), NULL);

  g_object_get (scrollable, "vadjustment", &adj, NULL);

  /* Horrid hack; g_object_get() returns a new reference but
   * that contradicts the memory management conventions
   * for accessors.
   */
  if (adj)
    g_object_unref (adj);

  return adj;
}

/**
 * bobgui_scrollable_set_vadjustment:
 * @scrollable: a `BobguiScrollable`
 * @vadjustment: (nullable): a `BobguiAdjustment`
 *
 * Sets the vertical adjustment of the `BobguiScrollable`.
 */
void
bobgui_scrollable_set_vadjustment (BobguiScrollable *scrollable,
                                BobguiAdjustment *vadjustment)
{
  g_return_if_fail (BOBGUI_IS_SCROLLABLE (scrollable));
  g_return_if_fail (vadjustment == NULL || BOBGUI_IS_ADJUSTMENT (vadjustment));

  g_object_set (scrollable, "vadjustment", vadjustment, NULL);
}


/**
 * bobgui_scrollable_get_hscroll_policy:
 * @scrollable: a `BobguiScrollable`
 *
 * Gets the horizontal `BobguiScrollablePolicy`.
 *
 * Returns: The horizontal `BobguiScrollablePolicy`.
 */
BobguiScrollablePolicy
bobgui_scrollable_get_hscroll_policy (BobguiScrollable *scrollable)
{
  BobguiScrollablePolicy policy;

  g_return_val_if_fail (BOBGUI_IS_SCROLLABLE (scrollable), BOBGUI_SCROLL_MINIMUM);

  g_object_get (scrollable, "hscroll-policy", &policy, NULL);

  return policy;
}

/**
 * bobgui_scrollable_set_hscroll_policy:
 * @scrollable: a `BobguiScrollable`
 * @policy: the horizontal `BobguiScrollablePolicy`
 *
 * Sets the `BobguiScrollablePolicy`.
 *
 * The policy determines whether horizontal scrolling should start
 * below the minimum width or below the natural width.
 */
void
bobgui_scrollable_set_hscroll_policy (BobguiScrollable       *scrollable,
				   BobguiScrollablePolicy  policy)
{
  g_return_if_fail (BOBGUI_IS_SCROLLABLE (scrollable));

  g_object_set (scrollable, "hscroll-policy", policy, NULL);
}

/**
 * bobgui_scrollable_get_vscroll_policy:
 * @scrollable: a `BobguiScrollable`
 *
 * Gets the vertical `BobguiScrollablePolicy`.
 *
 * Returns: The vertical `BobguiScrollablePolicy`.
 */
BobguiScrollablePolicy
bobgui_scrollable_get_vscroll_policy (BobguiScrollable *scrollable)
{
  BobguiScrollablePolicy policy;

  g_return_val_if_fail (BOBGUI_IS_SCROLLABLE (scrollable), BOBGUI_SCROLL_MINIMUM);

  g_object_get (scrollable, "vscroll-policy", &policy, NULL);

  return policy;
}

/**
 * bobgui_scrollable_set_vscroll_policy:
 * @scrollable: a `BobguiScrollable`
 * @policy: the vertical `BobguiScrollablePolicy`
 *
 * Sets the `BobguiScrollablePolicy`.
 *
 * The policy determines whether vertical scrolling should start
 * below the minimum height or below the natural height.
 */
void
bobgui_scrollable_set_vscroll_policy (BobguiScrollable       *scrollable,
				   BobguiScrollablePolicy  policy)
{
  g_return_if_fail (BOBGUI_IS_SCROLLABLE (scrollable));

  g_object_set (scrollable, "vscroll-policy", policy, NULL);
}

/**
 * bobgui_scrollable_get_border:
 * @scrollable: a `BobguiScrollable`
 * @border: (out caller-allocates): return location for the results
 *
 * Returns the size of a non-scrolling border around the
 * outside of the scrollable.
 *
 * An example for this would be treeview headers. BOBGUI can use
 * this information to display overlaid graphics, like the
 * overshoot indication, at the right position.
 *
 * Returns: %TRUE if @border has been set
 */
gboolean
bobgui_scrollable_get_border (BobguiScrollable *scrollable,
                           BobguiBorder     *border)
{
  g_return_val_if_fail (BOBGUI_IS_SCROLLABLE (scrollable), FALSE);
  g_return_val_if_fail (border != NULL, FALSE);

  if (BOBGUI_SCROLLABLE_GET_IFACE (scrollable)->get_border)
    return BOBGUI_SCROLLABLE_GET_IFACE (scrollable)->get_border (scrollable, border);

  return FALSE;
}
