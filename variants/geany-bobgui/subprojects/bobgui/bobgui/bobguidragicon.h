/*
 * Copyright © 2020 Matthias Clasen
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

#include <gio/gio.h>
#include <bobgui/bobguiwidget.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_DRAG_ICON (bobgui_drag_icon_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiDragIcon, bobgui_drag_icon, BOBGUI, DRAG_ICON, BobguiWidget)

GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_drag_icon_get_for_drag                      (GdkDrag                *drag);

GDK_AVAILABLE_IN_ALL
void            bobgui_drag_icon_set_child                         (BobguiDragIcon            *self,
                                                                 BobguiWidget              *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_drag_icon_get_child                         (BobguiDragIcon            *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_drag_icon_set_from_paintable (GdkDrag      *drag,
                                                  GdkPaintable *paintable,
                                                  int           hot_x,
                                                  int           hot_y);

GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_drag_icon_create_widget_for_value           (const GValue           *value);

G_END_DECLS


