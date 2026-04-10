/*
 * Copyright © 2019 Benjamin Otte
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
/* for GDK_AVAILABLE_IN_ALL */
#include <gdk/gdk.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_DIRECTORY_LIST (bobgui_directory_list_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiDirectoryList, bobgui_directory_list, BOBGUI, DIRECTORY_LIST, GObject)

GDK_AVAILABLE_IN_ALL
BobguiDirectoryList *      bobgui_directory_list_new                  (const char             *attributes,
                                                                 GFile                  *file);

GDK_AVAILABLE_IN_ALL
void                    bobgui_directory_list_set_file             (BobguiDirectoryList       *self,
                                                                 GFile                  *file);
GDK_AVAILABLE_IN_ALL
GFile *                 bobgui_directory_list_get_file             (BobguiDirectoryList       *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_directory_list_set_attributes       (BobguiDirectoryList       *self,
                                                                 const char             *attributes);
GDK_AVAILABLE_IN_ALL
const char *            bobgui_directory_list_get_attributes       (BobguiDirectoryList       *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_directory_list_set_io_priority      (BobguiDirectoryList       *self,
                                                                 int                     io_priority);
GDK_AVAILABLE_IN_ALL
int                     bobgui_directory_list_get_io_priority      (BobguiDirectoryList       *self);

GDK_AVAILABLE_IN_ALL
gboolean                bobgui_directory_list_is_loading           (BobguiDirectoryList       *self);
GDK_AVAILABLE_IN_ALL
const GError *          bobgui_directory_list_get_error            (BobguiDirectoryList       *self);

GDK_AVAILABLE_IN_ALL
void                    bobgui_directory_list_set_monitored        (BobguiDirectoryList       *self,
                                                                 gboolean                monitored);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_directory_list_get_monitored        (BobguiDirectoryList       *self);

G_END_DECLS

