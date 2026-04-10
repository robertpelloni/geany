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

#define BOBGUI_TYPE_INSPECTOR_LIST_DATA            (bobgui_inspector_list_data_get_type())

G_DECLARE_FINAL_TYPE (BobguiInspectorListData, bobgui_inspector_list_data, BOBGUI, INSPECTOR_LIST_DATA, BobguiWidget)

typedef struct _BobguiInspectorListData BobguiInspectorListData;

G_BEGIN_DECLS

void       bobgui_inspector_list_data_set_object (BobguiInspectorListData *sl,
                                               GObject              *object);

G_END_DECLS


// vim: set et sw=2 ts=2:
