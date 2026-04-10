/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * Authors: Cosimo Cecchi <cosimoc@gnome.org>
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

#include "bobgui/bobguiimage.h"
#include "bobgui/bobguitypes.h"

#include "bobguicssnodeprivate.h"
#include "bobguiimagedefinitionprivate.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_ICON_HELPER (bobgui_icon_helper_get_type())

G_DECLARE_FINAL_TYPE(BobguiIconHelper, bobgui_icon_helper, BOBGUI, ICON_HELPER, GObject)

BobguiIconHelper *bobgui_icon_helper_new (BobguiCssNode    *css_node,
                                    BobguiWidget     *owner);

void _bobgui_icon_helper_clear (BobguiIconHelper *self);

gboolean _bobgui_icon_helper_get_is_empty (BobguiIconHelper *self);

void _bobgui_icon_helper_set_definition (BobguiIconHelper *self,
                                      BobguiImageDefinition *def);
void _bobgui_icon_helper_set_gicon (BobguiIconHelper *self,
                                 GIcon *gicon);

void _bobgui_icon_helper_set_icon_name (BobguiIconHelper *self,
                                     const char *icon_name);
void _bobgui_icon_helper_set_paintable (BobguiIconHelper *self,
				     GdkPaintable  *paintable);

gboolean _bobgui_icon_helper_set_pixel_size   (BobguiIconHelper *self,
                                            int            pixel_size);
gboolean _bobgui_icon_helper_set_use_fallback (BobguiIconHelper *self,
                                            gboolean       use_fallback);

BobguiImageType _bobgui_icon_helper_get_storage_type (BobguiIconHelper *self);
int _bobgui_icon_helper_get_pixel_size (BobguiIconHelper *self);
gboolean _bobgui_icon_helper_get_use_fallback (BobguiIconHelper *self);

GIcon *_bobgui_icon_helper_peek_gicon (BobguiIconHelper *self);
GdkPaintable *_bobgui_icon_helper_peek_paintable (BobguiIconHelper *self);

BobguiImageDefinition *bobgui_icon_helper_get_definition (BobguiIconHelper *self);
const char *_bobgui_icon_helper_get_icon_name (BobguiIconHelper *self);

int bobgui_icon_helper_get_size (BobguiIconHelper *self);

void      bobgui_icon_helper_invalidate (BobguiIconHelper *self);
void      bobgui_icon_helper_invalidate_for_change (BobguiIconHelper     *self,
                                                 BobguiCssStyleChange *change);

void      bobgui_icon_size_set_style_classes (BobguiCssNode  *cssnode,
                                           BobguiIconSize  icon_size);

G_END_DECLS

