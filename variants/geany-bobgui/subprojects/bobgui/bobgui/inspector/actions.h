/*
 * Copyright (c) 2014 Red Hat, Inc.
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

#include <bobgui/bobguiwidget.h>

#define BOBGUI_TYPE_INSPECTOR_ACTIONS            (bobgui_inspector_actions_get_type())
#define BOBGUI_INSPECTOR_ACTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BOBGUI_TYPE_INSPECTOR_ACTIONS, BobguiInspectorActions))
#define BOBGUI_INSPECTOR_IS_ACTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOBGUI_TYPE_INSPECTOR_ACTIONS))

typedef struct _BobguiInspectorActions BobguiInspectorActions;

G_BEGIN_DECLS

GType      bobgui_inspector_actions_get_type   (void);
void       bobgui_inspector_actions_set_object (BobguiInspectorActions *sl,
                                             GObject              *object);

G_END_DECLS


// vim: set et sw=2 ts=2:
