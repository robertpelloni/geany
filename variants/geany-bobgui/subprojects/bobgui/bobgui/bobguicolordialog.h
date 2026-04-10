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

#define BOBGUI_TYPE_COLOR_DIALOG (bobgui_color_dialog_get_type ())

GDK_AVAILABLE_IN_4_10
G_DECLARE_FINAL_TYPE (BobguiColorDialog, bobgui_color_dialog, BOBGUI, COLOR_DIALOG, GObject)

GDK_AVAILABLE_IN_4_10
BobguiColorDialog *bobgui_color_dialog_new                    (void);

GDK_AVAILABLE_IN_4_10
const char *    bobgui_color_dialog_get_title              (BobguiColorDialog       *self);

GDK_AVAILABLE_IN_4_10
void            bobgui_color_dialog_set_title              (BobguiColorDialog       *self,
                                                         const char           *title);

GDK_AVAILABLE_IN_4_10
gboolean        bobgui_color_dialog_get_modal              (BobguiColorDialog       *self);

GDK_AVAILABLE_IN_4_10
void            bobgui_color_dialog_set_modal              (BobguiColorDialog       *self,
                                                         gboolean              modal);

GDK_AVAILABLE_IN_4_10
gboolean        bobgui_color_dialog_get_with_alpha         (BobguiColorDialog       *self);

GDK_AVAILABLE_IN_4_10
void            bobgui_color_dialog_set_with_alpha         (BobguiColorDialog       *self,
                                                         gboolean              with_alpha);

GDK_AVAILABLE_IN_4_10
void            bobgui_color_dialog_choose_rgba            (BobguiColorDialog       *self,
                                                         BobguiWindow            *parent,
                                                         const GdkRGBA        *initial_color,
                                                         GCancellable         *cancellable,
                                                         GAsyncReadyCallback   callback,
                                                         gpointer              user_data);

GDK_AVAILABLE_IN_4_10
GdkRGBA *       bobgui_color_dialog_choose_rgba_finish     (BobguiColorDialog        *self,
                                                         GAsyncResult          *result,
                                                         GError               **error);

G_END_DECLS
