/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2015 Benjamin Otte <otte@gnome.org>
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

G_BEGIN_DECLS

typedef union _BobguiImageDefinition BobguiImageDefinition;

BobguiImageDefinition *    bobgui_image_definition_new_empty          (void);
BobguiImageDefinition *    bobgui_image_definition_new_icon_name      (const char                     *icon_name);
BobguiImageDefinition *    bobgui_image_definition_new_gicon          (GIcon                          *gicon);
BobguiImageDefinition *    bobgui_image_definition_new_paintable      (GdkPaintable                   *paintable);

BobguiImageDefinition *    bobgui_image_definition_ref                (BobguiImageDefinition             *def);
void                    bobgui_image_definition_unref              (BobguiImageDefinition             *def);

BobguiImageType            bobgui_image_definition_get_storage_type   (const BobguiImageDefinition       *def);
int                     bobgui_image_definition_get_scale          (const BobguiImageDefinition       *def);
const char *           bobgui_image_definition_get_icon_name      (const BobguiImageDefinition       *def);
GIcon *                 bobgui_image_definition_get_gicon          (const BobguiImageDefinition       *def);
GdkPaintable *          bobgui_image_definition_get_paintable      (const BobguiImageDefinition       *def);


G_END_DECLS

