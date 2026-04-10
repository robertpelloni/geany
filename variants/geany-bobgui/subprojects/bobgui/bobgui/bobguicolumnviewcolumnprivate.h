/*
 * Copyright © 2019 Benjamin Otte
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

#include "bobgui/bobguicolumnviewcolumn.h"

#include "bobgui/bobguicolumnviewcellwidgetprivate.h"


void                    bobgui_column_view_column_set_column_view          (BobguiColumnViewColumn    *self,
                                                                         BobguiColumnView          *view);

void                    bobgui_column_view_column_set_position             (BobguiColumnViewColumn    *self,
                                                                         guint                   position);

void                    bobgui_column_view_column_add_cell                 (BobguiColumnViewColumn    *self,
                                                                         BobguiColumnViewCellWidget      *cell);
void                    bobgui_column_view_column_remove_cell              (BobguiColumnViewColumn    *self,
                                                                         BobguiColumnViewCellWidget      *cell);
BobguiColumnViewCellWidget *     bobgui_column_view_column_get_first_cell           (BobguiColumnViewColumn    *self);
BobguiWidget *             bobgui_column_view_column_get_header               (BobguiColumnViewColumn    *self);

void                    bobgui_column_view_column_update_factory           (BobguiColumnViewColumn    *self,
                                                                         gboolean                inert);
void                    bobgui_column_view_column_queue_resize             (BobguiColumnViewColumn    *self);
void                    bobgui_column_view_column_measure                  (BobguiColumnViewColumn    *self,
                                                                         int                    *minimum,
                                                                         int                    *natural);
void                    bobgui_column_view_column_allocate                 (BobguiColumnViewColumn    *self,
                                                                         int                     offset,
                                                                         int                     size);
void                    bobgui_column_view_column_get_allocation           (BobguiColumnViewColumn    *self,
                                                                         int                    *offset,
                                                                         int                    *size);

void                    bobgui_column_view_column_notify_sort              (BobguiColumnViewColumn    *self);

void                    bobgui_column_view_column_set_header_position      (BobguiColumnViewColumn    *self,
                                                                         int                     offset);
void                    bobgui_column_view_column_get_header_allocation    (BobguiColumnViewColumn    *self,
                                                                         int                    *offset,
                                                                         int                    *size);

