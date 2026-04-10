/*
 * BOBGUI - The Bobgui Framework
 * Copyright (C) 2008  Jaap Haitsma <jaap@haitsma.org>
 *
 * All rights reserved.
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

#include <bobgui/bobguiwindow.h>

G_BEGIN_DECLS

GDK_DEPRECATED_IN_4_10_FOR(bobgui_uri_launcher_launch)
void bobgui_show_uri_full (BobguiWindow           *parent,
                        const char          *uri,
                        guint32              timestamp,
                        GCancellable        *cancellable,
                        GAsyncReadyCallback  callback,
                        gpointer             user_data);

GDK_DEPRECATED_IN_4_10_FOR(bobgui_uri_launcher_launch)
gboolean bobgui_show_uri_full_finish (BobguiWindow     *parent,
                                   GAsyncResult  *result,
                                   GError       **error);

GDK_DEPRECATED_IN_4_10_FOR(bobgui_uri_launcher_launch)
void bobgui_show_uri (BobguiWindow  *parent,
                   const char *uri,
                   guint32     timestamp);

G_END_DECLS

