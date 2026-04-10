/* GSK - The Bobgui Framework
 * Copyright (C) 2011 Benjamin Otte <otte@gnome.org>
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

#if !defined (__BOBGUI_CSS_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/css/bobguicss.h> can be included directly."
#endif

#include <glib.h>

G_BEGIN_DECLS

typedef struct _BobguiCssLocation BobguiCssLocation;

/**
 * BobguiCssLocation:
 *
 * Points at a location inside a CSS stream.
 */
struct _BobguiCssLocation
{
  gsize                  bytes;
  gsize                  chars;
  gsize                  lines;
  gsize                  line_bytes;
  gsize                  line_chars;
};


G_END_DECLS

