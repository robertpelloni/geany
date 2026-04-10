/* BOBGUI - The Bobgui Framework
 *
 * Copyright (C) 2023 Red Hat, Inc.
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
 *
 * Author:
 *      Matthias Clasen <mclasen@redhat.com>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_GRAPHICS_OFFLOAD (bobgui_graphics_offload_get_type ())

GDK_AVAILABLE_IN_4_14
G_DECLARE_FINAL_TYPE (BobguiGraphicsOffload, bobgui_graphics_offload, BOBGUI, GRAPHICS_OFFLOAD, BobguiWidget)

GDK_AVAILABLE_IN_4_14
BobguiWidget *       bobgui_graphics_offload_new          (BobguiWidget                 *child);

GDK_AVAILABLE_IN_4_14
void              bobgui_graphics_offload_set_child    (BobguiGraphicsOffload        *self,
                                                     BobguiWidget                 *child);

GDK_AVAILABLE_IN_4_14
BobguiWidget *       bobgui_graphics_offload_get_child    (BobguiGraphicsOffload        *self);

/**
 * BobguiGraphicsOffloadEnabled:
 * @BOBGUI_GRAPHICS_OFFLOAD_ENABLED: Graphics offloading is enabled.
 * @BOBGUI_GRAPHICS_OFFLOAD_DISABLED: Graphics offloading is disabled.
 *
 * Represents the state of graphics offloading.
 *
 * Since: 4.14
 */
typedef enum
{
  BOBGUI_GRAPHICS_OFFLOAD_ENABLED,
  BOBGUI_GRAPHICS_OFFLOAD_DISABLED,
} BobguiGraphicsOffloadEnabled;

GDK_AVAILABLE_IN_4_14
void             bobgui_graphics_offload_set_enabled (BobguiGraphicsOffload        *self,
                                                   BobguiGraphicsOffloadEnabled  enabled);

GDK_AVAILABLE_IN_4_14
BobguiGraphicsOffloadEnabled
                 bobgui_graphics_offload_get_enabled  (BobguiGraphicsOffload        *self);

GDK_AVAILABLE_IN_4_16
void             bobgui_graphics_offload_set_black_background (BobguiGraphicsOffload *self,
                                                            gboolean            value);

GDK_AVAILABLE_IN_4_16
gboolean         bobgui_graphics_offload_get_black_background (BobguiGraphicsOffload *self);

G_END_DECLS
