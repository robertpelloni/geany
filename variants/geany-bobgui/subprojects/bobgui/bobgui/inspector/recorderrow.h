/*
 * Copyright (c) 2021 Red Hat, Inc.
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

#define BOBGUI_TYPE_INSPECTOR_RECORDER_ROW (bobgui_inspector_recorder_row_get_type ())

G_DECLARE_FINAL_TYPE (BobguiInspectorRecorderRow, bobgui_inspector_recorder_row, BOBGUI, INSPECTOR_RECORDER_ROW, BobguiWidget)
