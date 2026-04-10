/* bobguishortcutswindow.h
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwindow.h>
#include <bobgui/deprecated/bobguishortcutssection.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SHORTCUTS_WINDOW            (bobgui_shortcuts_window_get_type ())
#define BOBGUI_SHORTCUTS_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_SHORTCUTS_WINDOW, BobguiShortcutsWindow))
#define BOBGUI_IS_SHORTCUTS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_SHORTCUTS_WINDOW))

typedef struct _BobguiShortcutsWindow BobguiShortcutsWindow;

GDK_AVAILABLE_IN_ALL
GType bobgui_shortcuts_window_get_type (void) G_GNUC_CONST;

GDK_DEPRECATED_IN_4_18
void bobgui_shortcuts_window_add_section (BobguiShortcutsWindow  *self,
                                       BobguiShortcutsSection *section);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiShortcutsWindow, g_object_unref)

G_END_DECLS

