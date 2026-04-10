/*
 * Copyright (c) 2020 Red Hat, Inc.
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

#pragma once

#include <bobgui/bobguibox.h>

#define BOBGUI_TYPE_INSPECTOR_SHORTCUTS (bobgui_inspector_shortcuts_get_type ())

G_DECLARE_FINAL_TYPE (BobguiInspectorShortcuts, bobgui_inspector_shortcuts, BOBGUI, INSPECTOR_SHORTCUTS, BobguiWidget)


void bobgui_inspector_shortcuts_set_object (BobguiInspectorShortcuts *sl,
                                         GObject               *object);

