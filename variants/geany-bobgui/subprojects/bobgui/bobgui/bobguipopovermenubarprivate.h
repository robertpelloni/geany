/* BOBGUI - The Bobgui Framework
 * Copyright © 2019 Carlos Garnacho <carlosg@gnome.org>
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

#include "bobguipopovermenubar.h"

G_BEGIN_DECLS

void   bobgui_popover_menu_bar_select_first           (BobguiPopoverMenuBar *bar);
GList* bobgui_popover_menu_bar_get_viewable_menu_bars (BobguiWindow         *window);


G_END_DECLS

