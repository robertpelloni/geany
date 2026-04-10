/*
 * BOBGUI - The Bobgui Framework
 * Copyright (C) 2022 Red Hat, Inc.
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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguibutton.h>
#include <bobgui/bobguicolordialog.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_COLOR_DIALOG_BUTTON (bobgui_color_dialog_button_get_type ())

GDK_AVAILABLE_IN_4_10
G_DECLARE_FINAL_TYPE (BobguiColorDialogButton, bobgui_color_dialog_button, BOBGUI, COLOR_DIALOG_BUTTON, BobguiWidget)

GDK_AVAILABLE_IN_4_10
BobguiWidget *     bobgui_color_dialog_button_new             (BobguiColorDialog       *dialog);

GDK_AVAILABLE_IN_4_10
BobguiColorDialog *bobgui_color_dialog_button_get_dialog      (BobguiColorDialogButton *self);

GDK_AVAILABLE_IN_4_10
void            bobgui_color_dialog_button_set_dialog      (BobguiColorDialogButton *self,
                                                         BobguiColorDialog       *dialog);

GDK_AVAILABLE_IN_4_10
const GdkRGBA * bobgui_color_dialog_button_get_rgba        (BobguiColorDialogButton *self);

GDK_AVAILABLE_IN_4_10
void            bobgui_color_dialog_button_set_rgba        (BobguiColorDialogButton *self,
                                                         const GdkRGBA        *color);

G_END_DECLS
