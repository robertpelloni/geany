/*
 * BOBGUI - The Bobgui Framework
 * Copyright (C) 2024-2025 Sergey Bugaev
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <gio/gio.h>

typedef struct _BobguiWindow BobguiWindow;

G_BEGIN_DECLS

void bobgui_show_uri_win32 (BobguiWindow          *parent,
                         const char         *uri_or_path,
                         gboolean            always_ask,
                         GCancellable       *cancellable,
                         GAsyncReadyCallback callback,
                         gpointer            user_data);

gboolean bobgui_show_uri_win32_finish (BobguiWindow    *parent,
                                    GAsyncResult *result,
                                    GError      **error);

void bobgui_open_containing_folder_win32 (const char         *path,
                                       GCancellable       *cancellable,
                                       GAsyncReadyCallback callback,
                                       gpointer            user_data);

gboolean bobgui_open_containing_folder_win32_finish (GAsyncResult *result,
                                                  GError      **error);

G_END_DECLS
