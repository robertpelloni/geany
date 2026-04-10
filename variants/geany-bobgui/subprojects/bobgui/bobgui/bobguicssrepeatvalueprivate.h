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
#include "bobguicssvalueprivate.h"

G_BEGIN_DECLS

typedef enum {
  BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT,
  BOBGUI_CSS_REPEAT_STYLE_STRETCH = BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT,
  BOBGUI_CSS_REPEAT_STYLE_REPEAT,
  BOBGUI_CSS_REPEAT_STYLE_ROUND,
  BOBGUI_CSS_REPEAT_STYLE_SPACE
} BobguiCssRepeatStyle;

BobguiCssValue *       _bobgui_css_background_repeat_value_new        (BobguiCssRepeatStyle       x,
                                                                 BobguiCssRepeatStyle       y);
BobguiCssValue *       _bobgui_css_background_repeat_value_try_parse  (BobguiCssParser           *parser);
BobguiCssRepeatStyle   _bobgui_css_background_repeat_value_get_x      (const BobguiCssValue      *repeat);
BobguiCssRepeatStyle   _bobgui_css_background_repeat_value_get_y      (const BobguiCssValue      *repeat);

BobguiCssValue *       _bobgui_css_border_repeat_value_new            (BobguiCssRepeatStyle       x,
                                                                 BobguiCssRepeatStyle       y);
BobguiCssValue *       _bobgui_css_border_repeat_value_try_parse      (BobguiCssParser           *parser);
BobguiCssRepeatStyle   _bobgui_css_border_repeat_value_get_x          (const BobguiCssValue      *repeat);
BobguiCssRepeatStyle   _bobgui_css_border_repeat_value_get_y          (const BobguiCssValue      *repeat);

G_END_DECLS

