/*
 * Copyright (c) 2020 Alexander Mikhaylenko <alexm@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_WINDOW_HANDLE (bobgui_window_handle_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiWindowHandle, bobgui_window_handle, BOBGUI, WINDOW_HANDLE, BobguiWidget)

GDK_AVAILABLE_IN_ALL
BobguiWidget * bobgui_window_handle_new       (void);

GDK_AVAILABLE_IN_ALL
BobguiWidget * bobgui_window_handle_get_child (BobguiWindowHandle *self);

GDK_AVAILABLE_IN_ALL
void        bobgui_window_handle_set_child (BobguiWindowHandle *self,
                                         BobguiWidget       *child);

G_END_DECLS
