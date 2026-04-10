/* bobguifixedlayout.h: Fixed positioning layout manager
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Copyright 2019 GNOME Foundation
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

#include <bobgui/bobguilayoutmanager.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_FIXED_LAYOUT (bobgui_fixed_layout_get_type ())
#define BOBGUI_TYPE_FIXED_LAYOUT_CHILD (bobgui_fixed_layout_child_get_type ())

/* BobguiFixedLayout */

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiFixedLayout, bobgui_fixed_layout, BOBGUI, FIXED_LAYOUT, BobguiLayoutManager)

GDK_AVAILABLE_IN_ALL
BobguiLayoutManager *      bobgui_fixed_layout_new    (void);

/* BobguiFixedLayoutChild */

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiFixedLayoutChild, bobgui_fixed_layout_child, BOBGUI, FIXED_LAYOUT_CHILD, BobguiLayoutChild)

GDK_AVAILABLE_IN_ALL
void            bobgui_fixed_layout_child_set_transform    (BobguiFixedLayoutChild *child,
                                                         GskTransform        *transform);
GDK_AVAILABLE_IN_ALL
GskTransform *  bobgui_fixed_layout_child_get_transform    (BobguiFixedLayoutChild *child);

G_END_DECLS
