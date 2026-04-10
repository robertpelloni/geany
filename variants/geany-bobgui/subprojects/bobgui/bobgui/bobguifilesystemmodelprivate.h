/* BOBGUI - The Bobgui Framework
 * bobguifilesystemmodelprivate.h: BobguiTreeModel wrapping a BobguiFileSystem
 * Copyright (C) 2003, Red Hat, Inc.
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

#include <gio/gio.h>
#include <bobgui/bobguifilefilter.h>
#include <bobgui/deprecated/bobguifilechooser.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_FILE_SYSTEM_MODEL (bobgui_file_system_model_get_type ())
G_DECLARE_FINAL_TYPE (BobguiFileSystemModel, bobgui_file_system_model, BOBGUI, FILE_SYSTEM_MODEL, GObject)

BobguiFileSystemModel *_bobgui_file_system_model_new              (void);
BobguiFileSystemModel *_bobgui_file_system_model_new_for_directory(GFile              *dir,
                                                             const char         *attributes);
GFile *             _bobgui_file_system_model_get_directory    (BobguiFileSystemModel *model);
GCancellable *      _bobgui_file_system_model_get_cancellable  (BobguiFileSystemModel *model);
GFileInfo *         _bobgui_file_system_model_get_info_for_file(BobguiFileSystemModel *model,
							     GFile              *file);

void                _bobgui_file_system_model_add_and_query_file  (BobguiFileSystemModel *model,
                                                                GFile              *file,
                                                                const char         *attributes);
void                _bobgui_file_system_model_add_and_query_files (BobguiFileSystemModel *model,
                                                                GList              *files,
                                                                const char         *attributes);
void                _bobgui_file_system_model_update_files     (BobguiFileSystemModel *model,
                                                             GList              *files,
                                                             GList              *infos);

void                _bobgui_file_system_model_set_show_hidden  (BobguiFileSystemModel *model,
							     gboolean            show_hidden);
void                _bobgui_file_system_model_set_show_folders (BobguiFileSystemModel *model,
							     gboolean            show_folders);
void                _bobgui_file_system_model_set_show_files   (BobguiFileSystemModel *model,
							     gboolean            show_files);
void                _bobgui_file_system_model_set_filter_folders (BobguiFileSystemModel *model,
							     gboolean            show_folders);

void                _bobgui_file_system_model_set_filter       (BobguiFileSystemModel *model,
                                                             BobguiFileFilter      *filter);

void                _bobgui_file_system_model_set_can_select_files (BobguiFileSystemModel   *model,
                                                                 gboolean              can_select);

G_END_DECLS

