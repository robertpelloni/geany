/*
 * Copyright © 2012 Red Hat Inc.
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
 * Authors: Alexander Larsson <alexl@gnome.org>
 */

#pragma once

#include "bobguicssimageprivate.h"
#include "bobguicssvalueprivate.h"

G_BEGIN_DECLS

BobguiCssValue *   _bobgui_css_image_value_new           (BobguiCssImage         *image);

BobguiCssImage *   _bobgui_css_image_value_get_image     (const BobguiCssValue   *image);


G_END_DECLS

