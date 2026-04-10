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

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguishortcutcontroller.h>
#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SHORTCUT_MANAGER               (bobgui_shortcut_manager_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_INTERFACE (BobguiShortcutManager, bobgui_shortcut_manager, BOBGUI, SHORTCUT_MANAGER, BobguiWidget)

/**
 * BobguiShortcutManagerInterface:
 * @add_controller: Add a `BobguiShortcutController` to be managed.
 * @remove_controller: Remove a `BobguiShortcutController` that had previously
 *   been added
 *
 * The list of functions that can be implemented for the `BobguiShortcutManager`
 * interface.
 *
 * Note that no function is mandatory to implement, the default implementation
 * will work fine.
 */
struct _BobguiShortcutManagerInterface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/
  void                  (* add_controller)              (BobguiShortcutManager           *self,
                                                         BobguiShortcutController        *controller);
  void                  (* remove_controller)           (BobguiShortcutManager           *self,
                                                         BobguiShortcutController        *controller);
};


G_END_DECLS

