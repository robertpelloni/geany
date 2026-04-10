/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2023 Benjamin Otte
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
#include <bobgui/bobguienums.h>
#include <bobgui/bobguitypes.h>

#include <graphene.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SCROLL_INFO    (bobgui_scroll_info_get_type ())

GDK_AVAILABLE_IN_4_12
GType                   bobgui_scroll_info_get_type                (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_4_12
BobguiScrollInfo *         bobgui_scroll_info_new                     (void);

GDK_AVAILABLE_IN_4_12
BobguiScrollInfo *         bobgui_scroll_info_ref                     (BobguiScrollInfo           *self);
GDK_AVAILABLE_IN_4_12
void                    bobgui_scroll_info_unref                   (BobguiScrollInfo           *self);

GDK_AVAILABLE_IN_4_12
void                    bobgui_scroll_info_set_enable_horizontal   (BobguiScrollInfo           *self,
                                                                 gboolean                 horizontal);
GDK_AVAILABLE_IN_4_12
gboolean                bobgui_scroll_info_get_enable_horizontal   (BobguiScrollInfo           *self);

GDK_AVAILABLE_IN_4_12
void                    bobgui_scroll_info_set_enable_vertical     (BobguiScrollInfo           *self,
                                                                 gboolean                 vertical);
GDK_AVAILABLE_IN_4_12
gboolean                bobgui_scroll_info_get_enable_vertical     (BobguiScrollInfo           *self);


G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiScrollInfo, bobgui_scroll_info_unref)

G_END_DECLS

