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

#define BOBGUI_TYPE_BOOKMARK_LIST (bobgui_bookmark_list_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiBookmarkList, bobgui_bookmark_list, BOBGUI, BOOKMARK_LIST, GObject)

GDK_AVAILABLE_IN_ALL
BobguiBookmarkList *    bobgui_bookmark_list_new                  (const char *filename,
                                                             const char *attributes);

GDK_AVAILABLE_IN_ALL
const char *         bobgui_bookmark_list_get_filename         (BobguiBookmarkList *self);

GDK_AVAILABLE_IN_ALL
void                 bobgui_bookmark_list_set_attributes       (BobguiBookmarkList *self,
                                                             const char      *attributes);
GDK_AVAILABLE_IN_ALL
const char *         bobgui_bookmark_list_get_attributes       (BobguiBookmarkList *self);

GDK_AVAILABLE_IN_ALL
void                 bobgui_bookmark_list_set_io_priority      (BobguiBookmarkList *self,
                                                             int              io_priority);
GDK_AVAILABLE_IN_ALL
int                  bobgui_bookmark_list_get_io_priority      (BobguiBookmarkList *self);

GDK_AVAILABLE_IN_ALL
gboolean             bobgui_bookmark_list_is_loading           (BobguiBookmarkList *self);

G_END_DECLS

