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

#define BOBGUI_TYPE_URI_LAUNCHER (bobgui_uri_launcher_get_type ())

GDK_AVAILABLE_IN_4_10
G_DECLARE_FINAL_TYPE (BobguiUriLauncher, bobgui_uri_launcher, BOBGUI, URI_LAUNCHER, GObject)

GDK_AVAILABLE_IN_4_10
BobguiUriLauncher * bobgui_uri_launcher_new                         (const char          *uri);

GDK_AVAILABLE_IN_4_10
const char     * bobgui_uri_launcher_get_uri                     (BobguiUriLauncher      *self);
GDK_AVAILABLE_IN_4_10
void             bobgui_uri_launcher_set_uri                     (BobguiUriLauncher      *self,
                                                               const char          *uri);

GDK_AVAILABLE_IN_4_10
void             bobgui_uri_launcher_launch                      (BobguiUriLauncher      *self,
                                                               BobguiWindow           *parent,
                                                               GCancellable        *cancellable,
                                                               GAsyncReadyCallback  callback,
                                                               gpointer             user_data);

GDK_AVAILABLE_IN_4_10
gboolean         bobgui_uri_launcher_launch_finish               (BobguiUriLauncher      *self,
                                                               GAsyncResult        *result,
                                                               GError             **error);


GDK_AVAILABLE_IN_4_20
gboolean        bobgui_uri_launcher_can_launch                   (BobguiUriLauncher      *self,
                                                               BobguiWindow           *parent);

G_END_DECLS
