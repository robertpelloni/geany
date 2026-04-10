/*
 * Copyright © 2019 Zander Brown
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

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_INSPECTOR_TYPE_POPOVER (bobgui_inspector_type_popover_get_type ())

G_DECLARE_FINAL_TYPE (BobguiInspectorTypePopover, bobgui_inspector_type_popover,
                      BOBGUI, INSPECTOR_TYPE_POPOVER, BobguiPopover)

void bobgui_inspector_type_popover_set_gtype (BobguiInspectorTypePopover *self,
                                           GType                    gtype);

G_END_DECLS

