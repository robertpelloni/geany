/*
 * Copyright © 2023 Benjamin Otte
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

#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_LIST_HEADER (bobgui_list_header_get_type ())
GDK_AVAILABLE_IN_4_12
GDK_DECLARE_INTERNAL_TYPE (BobguiListHeader, bobgui_list_header, BOBGUI, LIST_HEADER, GObject)

GDK_AVAILABLE_IN_4_12
gpointer        bobgui_list_header_get_item                        (BobguiListHeader          *self);
GDK_AVAILABLE_IN_4_12
guint           bobgui_list_header_get_start                       (BobguiListHeader          *self) G_GNUC_PURE;
GDK_AVAILABLE_IN_4_12
guint           bobgui_list_header_get_end                         (BobguiListHeader          *self) G_GNUC_PURE;
GDK_AVAILABLE_IN_4_12
guint           bobgui_list_header_get_n_items                     (BobguiListHeader          *self) G_GNUC_PURE;

GDK_AVAILABLE_IN_4_12
void            bobgui_list_header_set_child                       (BobguiListHeader          *self,
                                                                 BobguiWidget              *child);
GDK_AVAILABLE_IN_4_12
BobguiWidget *     bobgui_list_header_get_child                       (BobguiListHeader          *self);

G_END_DECLS

