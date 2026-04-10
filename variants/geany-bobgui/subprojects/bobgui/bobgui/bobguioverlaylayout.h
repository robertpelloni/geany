/* bobguioverlaylayout.h: Overlay layout manager
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Copyright 2019 Red Hat, Inc.
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

#define BOBGUI_TYPE_OVERLAY_LAYOUT (bobgui_overlay_layout_get_type ())
#define BOBGUI_TYPE_OVERLAY_LAYOUT_CHILD (bobgui_overlay_layout_child_get_type ())

/* BobguiOverlayLayout */

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiOverlayLayout, bobgui_overlay_layout, BOBGUI, OVERLAY_LAYOUT, BobguiLayoutManager)

GDK_AVAILABLE_IN_ALL
BobguiLayoutManager *      bobgui_overlay_layout_new    (void);

/* BobguiOverlayLayoutChild */

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiOverlayLayoutChild, bobgui_overlay_layout_child, BOBGUI, OVERLAY_LAYOUT_CHILD, BobguiLayoutChild)

GDK_AVAILABLE_IN_ALL
void            bobgui_overlay_layout_child_set_measure (BobguiOverlayLayoutChild *child,
                                                      gboolean               measure);

GDK_AVAILABLE_IN_ALL
gboolean        bobgui_overlay_layout_child_get_measure (BobguiOverlayLayoutChild *child);

GDK_AVAILABLE_IN_ALL
void            bobgui_overlay_layout_child_set_clip_overlay (BobguiOverlayLayoutChild *child,
                                                           gboolean               clip_overlay);

GDK_AVAILABLE_IN_ALL
gboolean        bobgui_overlay_layout_child_get_clip_overlay (BobguiOverlayLayoutChild *child);

G_END_DECLS
