/* bobguiboxlayout.h: Box layout manager
 *
 * Copyright 2019  GNOME Foundation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguienums.h>
#include <bobgui/bobguilayoutmanager.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_BOX_LAYOUT (bobgui_box_layout_get_type())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiBoxLayout, bobgui_box_layout, BOBGUI, BOX_LAYOUT, BobguiLayoutManager)

GDK_AVAILABLE_IN_ALL
BobguiLayoutManager *      bobgui_box_layout_new                      (BobguiOrientation  orientation);

GDK_AVAILABLE_IN_ALL
void                    bobgui_box_layout_set_homogeneous          (BobguiBoxLayout        *box_layout,
                                                                 gboolean             homogeneous);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_box_layout_get_homogeneous          (BobguiBoxLayout        *box_layout);
GDK_AVAILABLE_IN_ALL
void                    bobgui_box_layout_set_spacing              (BobguiBoxLayout        *box_layout,
                                                                 guint                spacing);
GDK_AVAILABLE_IN_ALL
guint                   bobgui_box_layout_get_spacing              (BobguiBoxLayout        *box_layout);
GDK_AVAILABLE_IN_ALL
void                    bobgui_box_layout_set_baseline_position    (BobguiBoxLayout        *box_layout,
                                                                 BobguiBaselinePosition  position);
GDK_AVAILABLE_IN_ALL
BobguiBaselinePosition     bobgui_box_layout_get_baseline_position    (BobguiBoxLayout        *box_layout);

GDK_AVAILABLE_IN_4_12
void                    bobgui_box_layout_set_baseline_child       (BobguiBoxLayout        *box_layout,
                                                                 int                  child);
GDK_AVAILABLE_IN_4_12
int                     bobgui_box_layout_get_baseline_child       (BobguiBoxLayout        *box_layout);

G_END_DECLS
