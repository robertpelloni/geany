/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * bobguiorientable.c
 * Copyright (C) 2008 Imendio AB
 * Contact: Michael Natterer <mitch@imendio.com>
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

#include "config.h"

#include "bobguiorientable.h"

#include "bobguiprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguitypebuiltins.h"


/**
 * BobguiOrientable:
 *
 * An interface for widgets that can be oriented horizontally or vertically.
 *
 * `BobguiOrientable` is more flexible in that it allows the orientation to be
 * changed at runtime, allowing the widgets to “flip”.
 *
 * ## CSS nodes
 *
 * `BobguiWidget` types implementing the `BobguiOrientable` interface will
 * automatically acquire the `horizontal` or `vertical` CSS class depending on
 * the value of the [property@Bobgui.Orientable:orientation] property.

 */


typedef BobguiOrientableIface BobguiOrientableInterface;
G_DEFINE_INTERFACE (BobguiOrientable, bobgui_orientable, G_TYPE_OBJECT)

static void
bobgui_orientable_default_init (BobguiOrientableInterface *iface)
{
  /**
   * BobguiOrientable:orientation:
   *
   * The orientation of the orientable.
   **/
  g_object_interface_install_property (iface,
                                       g_param_spec_enum ("orientation", NULL, NULL,
                                                          BOBGUI_TYPE_ORIENTATION,
                                                          BOBGUI_ORIENTATION_HORIZONTAL,
                                                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
}

/**
 * bobgui_orientable_set_orientation:
 * @orientable: a `BobguiOrientable`
 * @orientation: the orientable’s new orientation
 *
 * Sets the orientation of the @orientable.
 */
void
bobgui_orientable_set_orientation (BobguiOrientable  *orientable,
                                BobguiOrientation  orientation)
{
  g_return_if_fail (BOBGUI_IS_ORIENTABLE (orientable));

  g_object_set (orientable,
                "orientation", orientation,
                NULL);

  if (BOBGUI_IS_WIDGET (orientable))
    bobgui_widget_update_orientation (BOBGUI_WIDGET (orientable), orientation);
}

/**
 * bobgui_orientable_get_orientation:
 * @orientable: a `BobguiOrientable`
 *
 * Retrieves the orientation of the @orientable.
 *
 * Returns: the orientation of the @orientable
 */
BobguiOrientation
bobgui_orientable_get_orientation (BobguiOrientable *orientable)
{
  BobguiOrientation orientation;

  g_return_val_if_fail (BOBGUI_IS_ORIENTABLE (orientable),
                        BOBGUI_ORIENTATION_HORIZONTAL);

  g_object_get (orientable,
                "orientation", &orientation,
                NULL);

  return orientation;
}
