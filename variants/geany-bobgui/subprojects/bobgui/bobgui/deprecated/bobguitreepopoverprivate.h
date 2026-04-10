/*
 * Copyright © 2019 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Matthias Clasen
 */

#pragma once

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_TREE_POPOVER         (bobgui_tree_popover_get_type ())
G_DECLARE_FINAL_TYPE (BobguiTreePopover, bobgui_tree_popover, BOBGUI, TREE_POPOVER, BobguiPopover)

void bobgui_tree_popover_set_model              (BobguiTreePopover              *popover,
                                              BobguiTreeModel                *model);
void bobgui_tree_popover_set_row_separator_func (BobguiTreePopover              *popover,
                                              BobguiTreeViewRowSeparatorFunc  func,
                                              gpointer                     data,
                                              GDestroyNotify               destroy);
void bobgui_tree_popover_set_active             (BobguiTreePopover              *popover,
                                              int                          item);
void bobgui_tree_popover_open_submenu           (BobguiTreePopover              *popover,
                                              const char                  *name);

G_END_DECLS

