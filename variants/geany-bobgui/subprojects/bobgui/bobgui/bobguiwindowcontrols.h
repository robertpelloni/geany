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

#define BOBGUI_TYPE_WINDOW_CONTROLS (bobgui_window_controls_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiWindowControls, bobgui_window_controls, BOBGUI, WINDOW_CONTROLS, BobguiWidget)

GDK_AVAILABLE_IN_ALL
BobguiWidget *  bobgui_window_controls_new                   (BobguiPackType        side);

GDK_AVAILABLE_IN_ALL
BobguiPackType  bobgui_window_controls_get_side              (BobguiWindowControls *self);

GDK_AVAILABLE_IN_ALL
void         bobgui_window_controls_set_side              (BobguiWindowControls *self,
                                                        BobguiPackType        side);

GDK_AVAILABLE_IN_ALL
const char * bobgui_window_controls_get_decoration_layout (BobguiWindowControls *self);

GDK_AVAILABLE_IN_ALL
void         bobgui_window_controls_set_decoration_layout (BobguiWindowControls *self,
                                                        const char        *layout);

GDK_AVAILABLE_IN_4_18
gboolean     bobgui_window_controls_get_use_native_controls (BobguiWindowControls *self);

GDK_AVAILABLE_IN_4_18
void         bobgui_window_controls_set_use_native_controls (BobguiWindowControls *self,
                                                          gboolean           setting);

GDK_AVAILABLE_IN_ALL
gboolean     bobgui_window_controls_get_empty             (BobguiWindowControls *self);

G_END_DECLS
