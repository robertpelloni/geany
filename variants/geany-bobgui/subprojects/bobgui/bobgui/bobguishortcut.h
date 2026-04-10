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

#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SHORTCUT         (bobgui_shortcut_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiShortcut, bobgui_shortcut, BOBGUI, SHORTCUT, GObject)

GDK_AVAILABLE_IN_ALL
BobguiShortcut *   bobgui_shortcut_new                                (BobguiShortcutTrigger     *trigger,
                                                                 BobguiShortcutAction      *action);
GDK_AVAILABLE_IN_ALL
BobguiShortcut *   bobgui_shortcut_new_with_arguments                 (BobguiShortcutTrigger     *trigger,
                                                                 BobguiShortcutAction      *action,
                                                                 const char             *format_string,
                                                                 ...);

GDK_AVAILABLE_IN_ALL
BobguiShortcutTrigger *
                bobgui_shortcut_get_trigger                        (BobguiShortcut            *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_shortcut_set_trigger                        (BobguiShortcut            *self,
                                                                 BobguiShortcutTrigger     *trigger);
GDK_AVAILABLE_IN_ALL
BobguiShortcutAction *
                bobgui_shortcut_get_action                         (BobguiShortcut            *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_shortcut_set_action                         (BobguiShortcut            *self,
                                                                 BobguiShortcutAction      *action);

GDK_AVAILABLE_IN_ALL
GVariant *      bobgui_shortcut_get_arguments                      (BobguiShortcut            *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_shortcut_set_arguments                      (BobguiShortcut            *self,
                                                                 GVariant               *args);

G_END_DECLS

