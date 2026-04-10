/*
 * Copyright © 2022 Matthias Clasen
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
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <bobgui/bobguisorter.h>
#include <bobgui/bobguicolumnviewcolumn.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_COLUMN_VIEW_SORTER             (bobgui_column_view_sorter_get_type ())

GDK_AVAILABLE_IN_4_10
G_DECLARE_FINAL_TYPE (BobguiColumnViewSorter, bobgui_column_view_sorter, BOBGUI, COLUMN_VIEW_SORTER, BobguiSorter)

GDK_AVAILABLE_IN_4_10
BobguiColumnViewColumn *   bobgui_column_view_sorter_get_primary_sort_column (BobguiColumnViewSorter *self);

GDK_AVAILABLE_IN_4_10
BobguiSortType             bobgui_column_view_sorter_get_primary_sort_order  (BobguiColumnViewSorter *self);

GDK_AVAILABLE_IN_4_10
guint                   bobgui_column_view_sorter_get_n_sort_columns      (BobguiColumnViewSorter *self);

GDK_AVAILABLE_IN_4_10
BobguiColumnViewColumn *   bobgui_column_view_sorter_get_nth_sort_column     (BobguiColumnViewSorter *self,
                                                                        guint                position,
                                                                        BobguiSortType         *sort_order);

G_END_DECLS


