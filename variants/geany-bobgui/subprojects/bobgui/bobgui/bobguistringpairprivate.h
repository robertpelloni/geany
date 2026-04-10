/*
 * Copyright (C) 2006 Alexander Larsson  <alexl@redhat.com>
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

#include <glib-object.h>

#define BOBGUI_TYPE_STRING_PAIR (bobgui_string_pair_get_type ())

G_DECLARE_FINAL_TYPE (BobguiStringPair, bobgui_string_pair, BOBGUI, STRING_PAIR, GObject)

BobguiStringPair * bobgui_string_pair_new        (const char    *id,
                                            const char    *string);

const char *    bobgui_string_pair_get_id     (BobguiStringPair *pair);

const char *    bobgui_string_pair_get_string (BobguiStringPair *pair);


