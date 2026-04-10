/* bobguigridlayout.h: Layout manager for grid-like widgets
 * Copyright 2019  GNOME Foundation
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
 */
#pragma once

#include <bobgui/bobguilayoutmanager.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_GRID_LAYOUT (bobgui_grid_layout_get_type ())
#define BOBGUI_TYPE_GRID_LAYOUT_CHILD (bobgui_grid_layout_child_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiGridLayout, bobgui_grid_layout, BOBGUI, GRID_LAYOUT, BobguiLayoutManager)

GDK_AVAILABLE_IN_ALL
BobguiLayoutManager *      bobgui_grid_layout_new                             (void);

GDK_AVAILABLE_IN_ALL
void                    bobgui_grid_layout_set_row_homogeneous             (BobguiGridLayout       *grid,
                                                                         gboolean             homogeneous);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_grid_layout_get_row_homogeneous             (BobguiGridLayout       *grid);
GDK_AVAILABLE_IN_ALL
void                    bobgui_grid_layout_set_row_spacing                 (BobguiGridLayout       *grid,
                                                                         guint                spacing);
GDK_AVAILABLE_IN_ALL
guint                   bobgui_grid_layout_get_row_spacing                 (BobguiGridLayout       *grid);
GDK_AVAILABLE_IN_ALL
void                    bobgui_grid_layout_set_column_homogeneous          (BobguiGridLayout       *grid,
                                                                         gboolean             homogeneous);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_grid_layout_get_column_homogeneous          (BobguiGridLayout       *grid);
GDK_AVAILABLE_IN_ALL
void                    bobgui_grid_layout_set_column_spacing              (BobguiGridLayout       *grid,
                                                                         guint                spacing);
GDK_AVAILABLE_IN_ALL
guint                   bobgui_grid_layout_get_column_spacing              (BobguiGridLayout       *grid);
GDK_AVAILABLE_IN_ALL
void                    bobgui_grid_layout_set_row_baseline_position       (BobguiGridLayout       *grid,
                                                                         int                  row,
                                                                         BobguiBaselinePosition  pos);
GDK_AVAILABLE_IN_ALL
BobguiBaselinePosition     bobgui_grid_layout_get_row_baseline_position       (BobguiGridLayout       *grid,
                                                                         int                  row);
GDK_AVAILABLE_IN_ALL
void                    bobgui_grid_layout_set_baseline_row                (BobguiGridLayout       *grid,
                                                                         int                  row);
GDK_AVAILABLE_IN_ALL
int                     bobgui_grid_layout_get_baseline_row                (BobguiGridLayout       *grid);

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiGridLayoutChild, bobgui_grid_layout_child, BOBGUI, GRID_LAYOUT_CHILD, BobguiLayoutChild)

GDK_AVAILABLE_IN_ALL
void                    bobgui_grid_layout_child_set_row                   (BobguiGridLayoutChild  *child,
                                                                         int                  row);
GDK_AVAILABLE_IN_ALL
int                     bobgui_grid_layout_child_get_row                   (BobguiGridLayoutChild  *child);
GDK_AVAILABLE_IN_ALL
void                    bobgui_grid_layout_child_set_column                (BobguiGridLayoutChild  *child,
                                                                         int                  column);
GDK_AVAILABLE_IN_ALL
int                     bobgui_grid_layout_child_get_column                (BobguiGridLayoutChild  *child);
GDK_AVAILABLE_IN_ALL
void                    bobgui_grid_layout_child_set_column_span           (BobguiGridLayoutChild  *child,
                                                                         int                  span);
GDK_AVAILABLE_IN_ALL
int                     bobgui_grid_layout_child_get_column_span           (BobguiGridLayoutChild  *child);
GDK_AVAILABLE_IN_ALL
void                    bobgui_grid_layout_child_set_row_span              (BobguiGridLayoutChild  *child,
                                                                         int                  span);
GDK_AVAILABLE_IN_ALL
int                     bobgui_grid_layout_child_get_row_span              (BobguiGridLayoutChild  *child);

G_END_DECLS
