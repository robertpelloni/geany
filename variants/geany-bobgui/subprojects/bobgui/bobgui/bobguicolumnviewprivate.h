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

#include "bobgui/bobguicolumnview.h"
#include "bobgui/bobguilistview.h"
#include "bobgui/bobguisizerequest.h"

#include "bobgui/bobguicolumnviewsorterprivate.h"
#include "bobgui/bobguicolumnviewrowwidgetprivate.h"

gboolean                bobgui_column_view_is_inert                (BobguiColumnView          *self);

BobguiColumnViewRowWidget *bobgui_column_view_get_header_widget       (BobguiColumnView          *self);
BobguiListView *           bobgui_column_view_get_list_view           (BobguiColumnView          *self);

void                    bobgui_column_view_measure_across          (BobguiColumnView          *self,
                                                                 int                    *minimum,
                                                                 int                    *natural);

void                    bobgui_column_view_distribute_width        (BobguiColumnView          *self,
                                                                 int                     width,
                                                                 BobguiRequestedSize       *sizes);

void                    bobgui_column_view_set_focus_column        (BobguiColumnView          *self,
                                                                 BobguiColumnViewColumn    *focus_column,
                                                                 gboolean                scroll);
BobguiColumnViewColumn *   bobgui_column_view_get_focus_column        (BobguiColumnView          *self);

