/* BOBGUI - The Bobgui Framework
 *
 * Copyright (C) 2022 Red Hat, Inc.
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
#include <bobgui/bobguiwindow.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_FILE_LAUNCHER (bobgui_file_launcher_get_type ())

GDK_AVAILABLE_IN_4_10
G_DECLARE_FINAL_TYPE (BobguiFileLauncher, bobgui_file_launcher, BOBGUI, FILE_LAUNCHER, GObject)

GDK_AVAILABLE_IN_4_10
BobguiFileLauncher * bobgui_file_launcher_new                          (GFile               *file);

GDK_AVAILABLE_IN_4_10
GFile           * bobgui_file_launcher_get_file                     (BobguiFileLauncher     *self);
GDK_AVAILABLE_IN_4_10
void              bobgui_file_launcher_set_file                     (BobguiFileLauncher     *self,
                                                                  GFile               *file);
GDK_AVAILABLE_IN_4_12
gboolean          bobgui_file_launcher_get_always_ask               (BobguiFileLauncher     *self);
GDK_AVAILABLE_IN_4_12
void              bobgui_file_launcher_set_always_ask               (BobguiFileLauncher     *self,
                                                                  gboolean             always_ask);

GDK_AVAILABLE_IN_4_14
gboolean          bobgui_file_launcher_get_writable                 (BobguiFileLauncher     *self);
GDK_AVAILABLE_IN_4_14
void              bobgui_file_launcher_set_writable                 (BobguiFileLauncher     *self,
                                                                  gboolean             writable);
GDK_AVAILABLE_IN_4_10
void             bobgui_file_launcher_launch                        (BobguiFileLauncher     *self,
                                                                  BobguiWindow           *parent,
                                                                  GCancellable        *cancellable,
                                                                  GAsyncReadyCallback  callback,
                                                                  gpointer             user_data);

GDK_AVAILABLE_IN_4_10
gboolean         bobgui_file_launcher_launch_finish                 (BobguiFileLauncher     *self,
                                                                  GAsyncResult        *result,
                                                                  GError             **error);

GDK_AVAILABLE_IN_4_10
void             bobgui_file_launcher_open_containing_folder        (BobguiFileLauncher     *self,
                                                                  BobguiWindow           *parent,
                                                                  GCancellable        *cancellable,
                                                                  GAsyncReadyCallback  callback,
                                                                  gpointer             user_data);

GDK_AVAILABLE_IN_4_10
gboolean         bobgui_file_launcher_open_containing_folder_finish (BobguiFileLauncher     *self,
                                                                  GAsyncResult        *result,
                                                                  GError             **error);

G_END_DECLS
