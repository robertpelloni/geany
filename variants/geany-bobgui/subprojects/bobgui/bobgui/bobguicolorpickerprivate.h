/*
 * BOBGUI - The Bobgui Framework
 * Copyright (C) 2018 Red Hat, Inc.
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

#include <bobgui/bobguiwindow.h>

G_BEGIN_DECLS


#define BOBGUI_TYPE_COLOR_PICKER             (bobgui_color_picker_get_type ())
#define BOBGUI_COLOR_PICKER(o)               (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_COLOR_PICKER, BobguiColorPicker))
#define BOBGUI_IS_COLOR_PICKER(o)            (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_COLOR_PICKER))
#define BOBGUI_COLOR_PICKER_GET_INTERFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), BOBGUI_TYPE_COLOR_PICKER, BobguiColorPickerInterface))


typedef struct _BobguiColorPicker            BobguiColorPicker;
typedef struct _BobguiColorPickerInterface   BobguiColorPickerInterface;

struct _BobguiColorPickerInterface {
  GTypeInterface g_iface;

  void (* pick)             (BobguiColorPicker      *picker,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data);

  GdkRGBA * (* pick_finish) (BobguiColorPicker      *picker,
                             GAsyncResult        *res,
                             GError             **error);
};

GType            bobgui_color_picker_get_type    (void) G_GNUC_CONST;
BobguiColorPicker * bobgui_color_picker_new         (void);
void             bobgui_color_picker_pick        (BobguiColorPicker       *picker,
                                               GAsyncReadyCallback   callback,
                                               gpointer              user_data);
GdkRGBA *        bobgui_color_picker_pick_finish (BobguiColorPicker       *picker,
                                               GAsyncResult         *res,
                                               GError              **error);

G_END_DECLS

