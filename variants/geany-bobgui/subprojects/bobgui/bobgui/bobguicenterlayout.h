/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
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

#define BOBGUI_TYPE_CENTER_LAYOUT (bobgui_center_layout_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiCenterLayout, bobgui_center_layout, BOBGUI, CENTER_LAYOUT, BobguiLayoutManager)

GDK_AVAILABLE_IN_ALL
BobguiLayoutManager *      bobgui_center_layout_new                   (void);
GDK_AVAILABLE_IN_ALL
void                    bobgui_center_layout_set_orientation       (BobguiCenterLayout     *self,
                                                                 BobguiOrientation       orientation);
GDK_AVAILABLE_IN_ALL
BobguiOrientation          bobgui_center_layout_get_orientation       (BobguiCenterLayout     *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_center_layout_set_baseline_position (BobguiCenterLayout     *self,
                                                                 BobguiBaselinePosition  baseline_position);
GDK_AVAILABLE_IN_ALL
BobguiBaselinePosition     bobgui_center_layout_get_baseline_position (BobguiCenterLayout     *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_center_layout_set_start_widget      (BobguiCenterLayout     *self,
                                                                 BobguiWidget           *widget);
GDK_AVAILABLE_IN_ALL
BobguiWidget *             bobgui_center_layout_get_start_widget      (BobguiCenterLayout     *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_center_layout_set_center_widget     (BobguiCenterLayout     *self,
                                                                 BobguiWidget           *widget);
GDK_AVAILABLE_IN_ALL
BobguiWidget *             bobgui_center_layout_get_center_widget     (BobguiCenterLayout     *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_center_layout_set_end_widget        (BobguiCenterLayout     *self,
                                                                 BobguiWidget           *widget);
GDK_AVAILABLE_IN_ALL
BobguiWidget *             bobgui_center_layout_get_end_widget        (BobguiCenterLayout     *self);

GDK_AVAILABLE_IN_4_12
void                    bobgui_center_layout_set_shrink_center_last (BobguiCenterLayout    *self,
                                                                  gboolean            shrink_center_last);
GDK_AVAILABLE_IN_4_12
gboolean                bobgui_center_layout_get_shrink_center_last (BobguiCenterLayout    *self);

G_END_DECLS
