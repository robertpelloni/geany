/*
 * Copyright © 2018 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "bobguirootprivate.h"
#include "bobguinative.h"
#include "bobguinativeprivate.h"
#include "bobguicssnodeprivate.h"
#include "bobguiwidgetprivate.h"
#include "gdk/gdkprivate.h"
#include "bobguiprivate.h"

#include "bobguishortcutmanager.h"

/**
 * BobguiRoot:
 *
 * An interface for widgets that can act as the root of a widget hierarchy.
 *
 * The root widget takes care of providing the connection to the windowing
 * system and manages layout, drawing and event delivery for its widget
 * hierarchy.
 *
 * The obvious example of a `BobguiRoot` is `BobguiWindow`.
 *
 * To get the display to which a `BobguiRoot` belongs, use
 * [method@Bobgui.Root.get_display].
 *
 * `BobguiRoot` also maintains the location of keyboard focus inside its widget
 * hierarchy, with [method@Bobgui.Root.set_focus] and [method@Bobgui.Root.get_focus].
 */

G_DEFINE_INTERFACE_WITH_CODE (BobguiRoot, bobgui_root, BOBGUI_TYPE_WIDGET,
                              g_type_interface_add_prerequisite (g_define_type_id, BOBGUI_TYPE_NATIVE))

static GdkDisplay *
bobgui_root_default_get_display (BobguiRoot *self)
{
  return gdk_display_get_default ();
}


static BobguiConstraintSolver *
bobgui_root_default_get_constraint_solver (BobguiRoot *self)
{
  return NULL;
}

static BobguiWidget *
bobgui_root_default_get_focus (BobguiRoot *self)
{
  return NULL;
}

static void
bobgui_root_default_set_focus (BobguiRoot   *self,
                            BobguiWidget *focus)
{
}

static void
bobgui_root_default_init (BobguiRootInterface *iface)
{
  iface->get_display = bobgui_root_default_get_display;
  iface->get_constraint_solver = bobgui_root_default_get_constraint_solver;
  iface->get_focus = bobgui_root_default_get_focus;
  iface->set_focus = bobgui_root_default_set_focus;
}

/**
 * bobgui_root_get_display:
 * @self: a `BobguiRoot`
 *
 * Returns the display that this `BobguiRoot` is on.
 *
 * Returns: (transfer none): the display of @root
 */
GdkDisplay *
bobgui_root_get_display (BobguiRoot *self)
{
  BobguiRootInterface *iface;

  g_return_val_if_fail (BOBGUI_IS_ROOT (self), NULL);

  iface = BOBGUI_ROOT_GET_IFACE (self);
  return iface->get_display (self);
}

BobguiConstraintSolver *
bobgui_root_get_constraint_solver (BobguiRoot *self)
{
  BobguiRootInterface *iface;

  g_return_val_if_fail (BOBGUI_IS_ROOT (self), NULL);

  iface = BOBGUI_ROOT_GET_IFACE (self);
  return iface->get_constraint_solver (self);
}

/**
 * bobgui_root_set_focus:
 * @self: a `BobguiRoot`
 * @focus: (nullable): widget to be the new focus widget, or %NULL
 *    to unset the focus widget
 *
 * If @focus is not the current focus widget, and is focusable, sets
 * it as the focus widget for the root.
 *
 * If @focus is %NULL, unsets the focus widget for the root.
 *
 * To set the focus to a particular widget in the root, it is usually
 * more convenient to use [method@Bobgui.Widget.grab_focus] instead of
 * this function.
 */
void
bobgui_root_set_focus (BobguiRoot   *self,
                    BobguiWidget *focus)
{
  g_return_if_fail (BOBGUI_IS_ROOT (self));
  g_return_if_fail (focus == NULL || BOBGUI_IS_WIDGET (focus));

  BOBGUI_ROOT_GET_IFACE (self)->set_focus (self, focus);
}

/**
 * bobgui_root_get_focus:
 * @self: a `BobguiRoot`
 *
 * Retrieves the current focused widget within the root.
 *
 * Note that this is the widget that would have the focus
 * if the root is active; if the root is not focused then
 * `bobgui_widget_has_focus (widget)` will be %FALSE for the
 * widget.
 *
 * Returns: (nullable) (transfer none): the currently focused widget
 */
BobguiWidget *
bobgui_root_get_focus (BobguiRoot *self)
{
  g_return_val_if_fail (BOBGUI_IS_ROOT (self), NULL);

  return BOBGUI_ROOT_GET_IFACE (self)->get_focus (self);
}

void
bobgui_root_start_layout (BobguiRoot *self)
{
  bobgui_native_queue_relayout (BOBGUI_NATIVE (self));
}

void
bobgui_root_stop_layout (BobguiRoot *self)
{
}

void
bobgui_root_queue_restyle (BobguiRoot *self)
{
  bobgui_root_start_layout (self);
}
