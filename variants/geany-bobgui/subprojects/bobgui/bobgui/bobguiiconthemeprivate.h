/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2015 Benjamin Otte <otte@gnome.org>
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

#include <bobgui/bobguiicontheme.h>
#include <bobgui/bobguicssstyleprivate.h>

typedef struct _BobguiStringSet BobguiStringSet;
const char *bobgui_string_set_add (BobguiStringSet *set,
                                const char   *string);

#define IMAGE_MISSING_RESOURCE_PATH "/org/bobgui/libbobgui/icons/16x16/status/image-missing.png"

int bobgui_icon_theme_get_serial (BobguiIconTheme *self);

void icon_cache_remove              (BobguiIconPaintable *icon);
void icon_cache_mark_used_if_cached (BobguiIconPaintable *icon);

