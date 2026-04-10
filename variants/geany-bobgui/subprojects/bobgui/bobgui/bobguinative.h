/*
 * Copyright © 2019 Red Hat, Inc.
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
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_NATIVE               (bobgui_native_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_INTERFACE (BobguiNative, bobgui_native, BOBGUI, NATIVE, BobguiWidget)

GDK_AVAILABLE_IN_ALL
void        bobgui_native_realize         (BobguiNative *self);

GDK_AVAILABLE_IN_ALL
void        bobgui_native_unrealize       (BobguiNative *self);

GDK_AVAILABLE_IN_ALL
BobguiNative * bobgui_native_get_for_surface (GdkSurface *surface);

GDK_AVAILABLE_IN_ALL
GdkSurface *bobgui_native_get_surface     (BobguiNative *self);

GDK_AVAILABLE_IN_ALL
GskRenderer *bobgui_native_get_renderer   (BobguiNative *self);

GDK_AVAILABLE_IN_ALL
void         bobgui_native_get_surface_transform (BobguiNative *self,
                                               double    *x,
                                               double    *y);

G_END_DECLS

