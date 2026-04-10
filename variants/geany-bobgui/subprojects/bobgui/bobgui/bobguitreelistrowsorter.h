/*
 * Copyright © 2019 Matthias Clasen
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

#include <bobgui/bobguiexpression.h>
#include <bobgui/bobguisorter.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_TREE_LIST_ROW_SORTER             (bobgui_tree_list_row_sorter_get_type ())
GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiTreeListRowSorter, bobgui_tree_list_row_sorter, BOBGUI, TREE_LIST_ROW_SORTER, BobguiSorter)

GDK_AVAILABLE_IN_ALL
BobguiTreeListRowSorter *  bobgui_tree_list_row_sorter_new                   (BobguiSorter            *sorter);

GDK_AVAILABLE_IN_ALL
BobguiSorter *             bobgui_tree_list_row_sorter_get_sorter            (BobguiTreeListRowSorter *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_tree_list_row_sorter_set_sorter            (BobguiTreeListRowSorter *self,
                                                                        BobguiSorter            *sorter);

G_END_DECLS

