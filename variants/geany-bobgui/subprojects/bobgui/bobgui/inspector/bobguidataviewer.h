/*
 * Copyright © 2021 Benjamin Otte
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

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_DATA_VIEWER         (bobgui_data_viewer_get_type ())

G_DECLARE_FINAL_TYPE (BobguiDataViewer, bobgui_data_viewer, BOBGUI, DATA_VIEWER, BobguiWidget)

BobguiWidget *     bobgui_data_viewer_new                             (void);

gboolean        bobgui_data_viewer_is_loading                      (BobguiDataViewer          *self);

void            bobgui_data_viewer_reset                           (BobguiDataViewer          *self);

void            bobgui_data_viewer_load_value                      (BobguiDataViewer          *self,
                                                                 const GValue           *value);
void            bobgui_data_viewer_load_stream                     (BobguiDataViewer          *self,
                                                                 GInputStream           *stream,
                                                                 const char             *mime_type);
void            bobgui_data_viewer_load_error                      (BobguiDataViewer          *self,
                                                                 GError                 *error);

G_END_DECLS

