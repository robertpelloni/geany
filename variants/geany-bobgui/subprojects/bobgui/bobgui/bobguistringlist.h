/*
 * Copyright © 2020 Red Hat, Inc.
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

#include <gio/gio.h>
/* for GDK_AVAILABLE_IN_ALL */
#include <gdk/gdk.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_STRING_OBJECT (bobgui_string_object_get_type ())
GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiStringObject, bobgui_string_object, BOBGUI, STRING_OBJECT, GObject)

GDK_AVAILABLE_IN_ALL
BobguiStringObject *       bobgui_string_object_new        (const char      *string);
GDK_AVAILABLE_IN_ALL
const char *            bobgui_string_object_get_string (BobguiStringObject *self);

#define BOBGUI_TYPE_STRING_LIST (bobgui_string_list_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiStringList, bobgui_string_list, BOBGUI, STRING_LIST, GObject)

GDK_AVAILABLE_IN_ALL
BobguiStringList * bobgui_string_list_new             (const char * const    *strings);

GDK_AVAILABLE_IN_ALL
void            bobgui_string_list_append          (BobguiStringList         *self,
                                                 const char            *string);

GDK_AVAILABLE_IN_ALL
void            bobgui_string_list_take            (BobguiStringList         *self,
                                                 char                  *string);

GDK_AVAILABLE_IN_ALL
void            bobgui_string_list_remove          (BobguiStringList         *self,
                                                 guint                  position);

GDK_AVAILABLE_IN_ALL
void            bobgui_string_list_splice          (BobguiStringList         *self,
                                                 guint                  position,
                                                 guint                  n_removals,
                                                 const char * const    *additions);

GDK_AVAILABLE_IN_ALL
const char *    bobgui_string_list_get_string      (BobguiStringList         *self,
                                                 guint                  position);

GDK_AVAILABLE_IN_4_18
guint           bobgui_string_list_find            (BobguiStringList         *self,
                                                 const char            *string);

G_END_DECLS

