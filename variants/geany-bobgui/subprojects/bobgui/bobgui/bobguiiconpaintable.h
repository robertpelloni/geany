/* BobguiIconPaintable
 * bobguiiconpaintable.h Copyright (C) 2002, 2003 Red Hat, Inc.
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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <bobgui/bobguienums.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_ICON_PAINTABLE    (bobgui_icon_paintable_get_type ())
GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiIconPaintable, bobgui_icon_paintable, BOBGUI, ICON_PAINTABLE, GObject)

GDK_AVAILABLE_IN_ALL
BobguiIconPaintable *      bobgui_icon_paintable_new_for_file         (GFile                  *file,
                                                                 int                     size,
                                                                 int                     scale);

GDK_AVAILABLE_IN_ALL
GFile *                 bobgui_icon_paintable_get_file             (BobguiIconPaintable       *self);
GDK_DEPRECATED_IN_4_20
const char *            bobgui_icon_paintable_get_icon_name        (BobguiIconPaintable       *self);
GDK_DEPRECATED_IN_4_20
gboolean                bobgui_icon_paintable_is_symbolic          (BobguiIconPaintable       *self);

G_END_DECLS

