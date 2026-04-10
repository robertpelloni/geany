/* bobguibinlayout.h: Layout manager for bin-like widgets
 * Copyright 2019  GNOME Foundation
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

#define BOBGUI_TYPE_BIN_LAYOUT (bobgui_bin_layout_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiBinLayout, bobgui_bin_layout, BOBGUI, BIN_LAYOUT, BobguiLayoutManager)

GDK_AVAILABLE_IN_ALL
BobguiLayoutManager *      bobgui_bin_layout_new      (void);

G_END_DECLS
