/*
 * Copyright © 2020 Red Hat, Inc.
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
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <gio/gio.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_JOINED_MENU (bobgui_joined_menu_get_type())

G_DECLARE_FINAL_TYPE (BobguiJoinedMenu, bobgui_joined_menu, BOBGUI, JOINED_MENU, GMenuModel)

BobguiJoinedMenu *bobgui_joined_menu_new          (void);
guint          bobgui_joined_menu_get_n_joined (BobguiJoinedMenu *self);
void           bobgui_joined_menu_append_menu  (BobguiJoinedMenu *self,
                                             GMenuModel    *model);
void           bobgui_joined_menu_prepend_menu (BobguiJoinedMenu *self,
                                             GMenuModel    *model);
void           bobgui_joined_menu_remove_menu  (BobguiJoinedMenu *self,
                                             GMenuModel    *model);
void           bobgui_joined_menu_remove_index (BobguiJoinedMenu *self,
                                             guint          index);

G_END_DECLS
