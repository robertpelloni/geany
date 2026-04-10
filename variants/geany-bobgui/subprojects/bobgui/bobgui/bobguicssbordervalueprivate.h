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

#include <bobgui/css/bobguicss.h>
#include "bobgui/css/bobguicsstokenizerprivate.h"
#include "bobgui/css/bobguicssparserprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicssvalueprivate.h"

G_BEGIN_DECLS

BobguiCssValue *   _bobgui_css_border_value_new           (BobguiCssValue            *top,
                                                     BobguiCssValue            *right,
                                                     BobguiCssValue            *bottom,
                                                     BobguiCssValue            *left);
BobguiCssValue *   _bobgui_css_border_value_parse         (BobguiCssParser           *parser,
                                                     BobguiCssNumberParseFlags  flags,
                                                     gboolean                allow_auto,
                                                     gboolean                allow_fill);

BobguiCssValue *   _bobgui_css_border_value_get_top       (const BobguiCssValue      *value);
BobguiCssValue *   _bobgui_css_border_value_get_right     (const BobguiCssValue      *value);
BobguiCssValue *   _bobgui_css_border_value_get_bottom    (const BobguiCssValue      *value);
BobguiCssValue *   _bobgui_css_border_value_get_left      (const BobguiCssValue      *value);


G_END_DECLS

