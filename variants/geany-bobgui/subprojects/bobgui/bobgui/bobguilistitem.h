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

#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_LIST_ITEM (bobgui_list_item_get_type ())
GDK_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BobguiListItem, bobgui_list_item, BOBGUI, LIST_ITEM, GObject)

GDK_AVAILABLE_IN_ALL
gpointer        bobgui_list_item_get_item                          (BobguiListItem            *self);
GDK_AVAILABLE_IN_ALL
guint           bobgui_list_item_get_position                      (BobguiListItem            *self) G_GNUC_PURE;
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_list_item_get_selected                      (BobguiListItem            *self) G_GNUC_PURE;
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_list_item_get_selectable                    (BobguiListItem            *self) G_GNUC_PURE;
GDK_AVAILABLE_IN_ALL
void            bobgui_list_item_set_selectable                    (BobguiListItem            *self,
                                                                 gboolean                selectable);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_list_item_get_activatable                   (BobguiListItem            *self) G_GNUC_PURE;
GDK_AVAILABLE_IN_ALL
void            bobgui_list_item_set_activatable                   (BobguiListItem            *self,
                                                                 gboolean                activatable);
GDK_AVAILABLE_IN_4_12
gboolean        bobgui_list_item_get_focusable                     (BobguiListItem            *self) G_GNUC_PURE;
GDK_AVAILABLE_IN_4_12
void            bobgui_list_item_set_focusable                     (BobguiListItem            *self,
                                                                 gboolean                focusable);

GDK_AVAILABLE_IN_ALL
void            bobgui_list_item_set_child                         (BobguiListItem            *self,
                                                                 BobguiWidget              *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_list_item_get_child                         (BobguiListItem            *self);

GDK_AVAILABLE_IN_4_12
void            bobgui_list_item_set_accessible_description        (BobguiListItem            *self,
                                                                 const char             *description);
GDK_AVAILABLE_IN_4_12
const char *    bobgui_list_item_get_accessible_description        (BobguiListItem            *self);

GDK_AVAILABLE_IN_4_12
void            bobgui_list_item_set_accessible_label              (BobguiListItem            *self,
                                                                 const char             *label);
GDK_AVAILABLE_IN_4_12
const char *    bobgui_list_item_get_accessible_label              (BobguiListItem            *self);

G_END_DECLS

