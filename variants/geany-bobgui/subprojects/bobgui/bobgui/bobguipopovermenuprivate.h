/* BOBGUI - The Bobgui Framework
 * Copyright © 2019 Red Hat, Inc.
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

#include "bobguipopovermenu.h"

G_BEGIN_DECLS

BobguiWidget *bobgui_popover_menu_get_active_item  (BobguiPopoverMenu *menu);
void       bobgui_popover_menu_set_active_item  (BobguiPopoverMenu *menu,
                                              BobguiWidget      *item);
BobguiWidget *bobgui_popover_menu_get_open_submenu (BobguiPopoverMenu *menu);
void       bobgui_popover_menu_set_open_submenu (BobguiPopoverMenu *menu,
                                              BobguiWidget      *submenu);
void       bobgui_popover_menu_close_submenus   (BobguiPopoverMenu *menu);

BobguiWidget *bobgui_popover_menu_get_parent_menu  (BobguiPopoverMenu *menu);
void       bobgui_popover_menu_set_parent_menu  (BobguiPopoverMenu *menu,
                                              BobguiWidget      *parent);

BobguiWidget * bobgui_popover_menu_new (void);

void  bobgui_popover_menu_add_submenu (BobguiPopoverMenu *popover,
                                    BobguiWidget      *submenu,
                                    const char     *name);
void bobgui_popover_menu_open_submenu (BobguiPopoverMenu *popover,
                                    const char     *name);

BobguiWidget * bobgui_popover_menu_get_stack (BobguiPopoverMenu *menu);

G_END_DECLS

