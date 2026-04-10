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
#include <bobgui/bobguifilefilter.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_FILE_DIALOG (bobgui_file_dialog_get_type ())

GDK_AVAILABLE_IN_4_10
G_DECLARE_FINAL_TYPE (BobguiFileDialog, bobgui_file_dialog, BOBGUI, FILE_DIALOG, GObject)

GDK_AVAILABLE_IN_4_10
BobguiFileDialog * bobgui_file_dialog_new                  (void);

GDK_AVAILABLE_IN_4_10
const char *    bobgui_file_dialog_get_title            (BobguiFileDialog        *self);

GDK_AVAILABLE_IN_4_10
void            bobgui_file_dialog_set_title            (BobguiFileDialog        *self,
                                                      const char           *title);

GDK_AVAILABLE_IN_4_10
gboolean        bobgui_file_dialog_get_modal            (BobguiFileDialog        *self);

GDK_AVAILABLE_IN_4_10
void            bobgui_file_dialog_set_modal            (BobguiFileDialog        *self,
                                                      gboolean              modal);

GDK_AVAILABLE_IN_4_10
GListModel *     bobgui_file_dialog_get_filters         (BobguiFileDialog        *self);

GDK_AVAILABLE_IN_4_10
void             bobgui_file_dialog_set_filters         (BobguiFileDialog        *self,
                                                      GListModel           *filters);

GDK_AVAILABLE_IN_4_10
BobguiFileFilter *  bobgui_file_dialog_get_default_filter  (BobguiFileDialog        *self);

GDK_AVAILABLE_IN_4_10
void             bobgui_file_dialog_set_default_filter  (BobguiFileDialog        *self,
                                                      BobguiFileFilter        *filter);

GDK_AVAILABLE_IN_4_10
GFile *          bobgui_file_dialog_get_initial_folder  (BobguiFileDialog        *self);

GDK_AVAILABLE_IN_4_10
void             bobgui_file_dialog_set_initial_folder  (BobguiFileDialog        *self,
                                                      GFile                *folder);

GDK_AVAILABLE_IN_4_10
const char *     bobgui_file_dialog_get_initial_name    (BobguiFileDialog        *self);

GDK_AVAILABLE_IN_4_10
void             bobgui_file_dialog_set_initial_name    (BobguiFileDialog        *self,
                                                      const char           *name);

GDK_AVAILABLE_IN_4_10
GFile *          bobgui_file_dialog_get_initial_file    (BobguiFileDialog        *self);

GDK_AVAILABLE_IN_4_10
void             bobgui_file_dialog_set_initial_file    (BobguiFileDialog        *self,
                                                      GFile                *file);

GDK_AVAILABLE_IN_4_10
const char *    bobgui_file_dialog_get_accept_label     (BobguiFileDialog        *self);

GDK_AVAILABLE_IN_4_10
void             bobgui_file_dialog_set_accept_label    (BobguiFileDialog        *self,
                                                      const char           *accept_label);

GDK_AVAILABLE_IN_4_10
void             bobgui_file_dialog_open                (BobguiFileDialog        *self,
                                                      BobguiWindow            *parent,
                                                      GCancellable         *cancellable,
                                                      GAsyncReadyCallback   callback,
                                                      gpointer              user_data);

GDK_AVAILABLE_IN_4_10
GFile *          bobgui_file_dialog_open_finish         (BobguiFileDialog        *self,
                                                      GAsyncResult         *result,
                                                      GError              **error);

GDK_AVAILABLE_IN_4_10
void             bobgui_file_dialog_select_folder       (BobguiFileDialog        *self,
                                                      BobguiWindow            *parent,
                                                      GCancellable         *cancellable,
                                                      GAsyncReadyCallback   callback,
                                                      gpointer              user_data);

GDK_AVAILABLE_IN_4_10
GFile *          bobgui_file_dialog_select_folder_finish
                                                     (BobguiFileDialog        *self,
                                                      GAsyncResult         *result,
                                                      GError              **error);

GDK_AVAILABLE_IN_4_10
void             bobgui_file_dialog_save                (BobguiFileDialog        *self,
                                                      BobguiWindow            *parent,
                                                      GCancellable         *cancellable,
                                                      GAsyncReadyCallback   callback,
                                                      gpointer              user_data);

GDK_AVAILABLE_IN_4_10
GFile *          bobgui_file_dialog_save_finish         (BobguiFileDialog        *self,
                                                      GAsyncResult         *result,
                                                      GError               **error);

GDK_AVAILABLE_IN_4_10
void             bobgui_file_dialog_open_multiple       (BobguiFileDialog        *self,
                                                      BobguiWindow            *parent,
                                                      GCancellable         *cancellable,
                                                      GAsyncReadyCallback   callback,
                                                      gpointer              user_data);

GDK_AVAILABLE_IN_4_10
GListModel *     bobgui_file_dialog_open_multiple_finish
                                                     (BobguiFileDialog        *self,
                                                      GAsyncResult         *result,
                                                      GError              **error);

GDK_AVAILABLE_IN_4_10
void             bobgui_file_dialog_select_multiple_folders
                                                     (BobguiFileDialog        *self,
                                                      BobguiWindow            *parent,
                                                      GCancellable         *cancellable,
                                                      GAsyncReadyCallback   callback,
                                                      gpointer              user_data);

GDK_AVAILABLE_IN_4_10
GListModel *     bobgui_file_dialog_select_multiple_folders_finish
                                                     (BobguiFileDialog        *self,
                                                      GAsyncResult         *result,
                                                      GError              **error);

GDK_AVAILABLE_IN_4_18
void             bobgui_file_dialog_open_text_file      (BobguiFileDialog        *self,
                                                      BobguiWindow            *parent,
                                                      GCancellable         *cancellable,
                                                      GAsyncReadyCallback   callback,
                                                      gpointer              user_data);

GDK_AVAILABLE_IN_4_18
GFile *          bobgui_file_dialog_open_text_file_finish (BobguiFileDialog        *self,
                                                        GAsyncResult         *result,
                                                        const char          **encoding,
                                                        GError              **error);

GDK_AVAILABLE_IN_4_18
void             bobgui_file_dialog_open_multiple_text_files
                                                     (BobguiFileDialog        *self,
                                                      BobguiWindow            *parent,
                                                      GCancellable         *cancellable,
                                                      GAsyncReadyCallback   callback,
                                                      gpointer              user_data);

GDK_AVAILABLE_IN_4_18
GListModel *     bobgui_file_dialog_open_multiple_text_files_finish
                                                     (BobguiFileDialog        *self,
                                                      GAsyncResult         *result,
                                                      const char          **encoding,
                                                      GError              **error);


GDK_AVAILABLE_IN_4_18
void             bobgui_file_dialog_save_text_file      (BobguiFileDialog        *self,
                                                      BobguiWindow            *parent,
                                                      GCancellable         *cancellable,
                                                      GAsyncReadyCallback   callback,
                                                      gpointer              user_data);

GDK_AVAILABLE_IN_4_18
GFile *          bobgui_file_dialog_save_text_file_finish (BobguiFileDialog        *self,
                                                        GAsyncResult         *result,
                                                        const char          **encoding,
                                                        const char          **line_ending,
                                                        GError              **error);

G_END_DECLS
