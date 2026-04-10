/*
 * Copyright © 2018 Benjamin Otte
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
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_ROOT               (bobgui_root_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_INTERFACE (BobguiRoot, bobgui_root, BOBGUI, ROOT, BobguiWidget)

GDK_AVAILABLE_IN_ALL
GdkDisplay * bobgui_root_get_display (BobguiRoot *self);

GDK_AVAILABLE_IN_ALL
void        bobgui_root_set_focus (BobguiRoot   *self,
                                BobguiWidget *focus);
GDK_AVAILABLE_IN_ALL
BobguiWidget * bobgui_root_get_focus (BobguiRoot   *self);

G_END_DECLS

