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
#include "bobguicsstypesprivate.h"
#include "bobguicssvalueprivate.h"
#include "bobguicsscolorprivate.h"

G_BEGIN_DECLS

typedef enum /*< skip >*/ {
  BOBGUI_CSS_POSITIVE_ONLY = (1 << 0),
  BOBGUI_CSS_PARSE_PERCENT = (1 << 1),
  BOBGUI_CSS_PARSE_NUMBER = (1 << 2),
  BOBGUI_CSS_PARSE_LENGTH = (1 << 3),
  BOBGUI_CSS_PARSE_ANGLE = (1 << 4),
  BOBGUI_CSS_PARSE_TIME = (1 << 5)
} BobguiCssNumberParseFlags;

typedef struct
{
  /* Context needed when parsing numbers */
  BobguiCssValue *color;
  BobguiCssColorSpace color_space;
  gboolean legacy_rgb_scale; /* r, g, b must be scaled to 255 */
} BobguiCssNumberParseContext;

#define BOBGUI_CSS_PARSE_DIMENSION (BOBGUI_CSS_PARSE_LENGTH|BOBGUI_CSS_PARSE_ANGLE|BOBGUI_CSS_PARSE_TIME)

BobguiCssValue *   bobgui_css_dimension_value_new         (double                  value,
                                                     BobguiCssUnit              unit);

BobguiCssValue *   bobgui_css_number_value_new            (double                  value,
                                                     BobguiCssUnit              unit);
gboolean        bobgui_css_number_value_can_parse      (BobguiCssParser           *parser);
BobguiCssValue *   bobgui_css_number_value_parse          (BobguiCssParser           *parser,
                                                     BobguiCssNumberParseFlags  flags);

BobguiCssValue *   bobgui_css_number_value_parse_with_context (BobguiCssParser             *parser,
                                                         BobguiCssNumberParseFlags    flags,
                                                         BobguiCssNumberParseContext *context);

BobguiCssDimension bobgui_css_number_value_get_dimension  (const BobguiCssValue      *value) G_GNUC_PURE;
gboolean        bobgui_css_number_value_has_percent    (const BobguiCssValue      *value) G_GNUC_PURE;
BobguiCssValue *   bobgui_css_number_value_multiply       (BobguiCssValue            *value,
                                                     double                  factor);
BobguiCssValue *   bobgui_css_number_value_add            (BobguiCssValue            *value1,
                                                     BobguiCssValue            *value2);
BobguiCssValue *   bobgui_css_number_value_try_add        (BobguiCssValue            *value1,
                                                     BobguiCssValue            *value2);
double          bobgui_css_number_value_get            (const BobguiCssValue      *number,
                                                     double                  one_hundred_percent) G_GNUC_PURE;
double          bobgui_css_number_value_get_canonical  (BobguiCssValue            *number,
                                                     double                  one_hundred_percent) G_GNUC_PURE;

gboolean        bobgui_css_dimension_value_is_zero     (const BobguiCssValue      *value) G_GNUC_PURE;

BobguiCssValue *   bobgui_css_number_value_new_color_component (BobguiCssValue      *color,
                                                          BobguiCssColorSpace  color_space,
                                                          gboolean          legacy_srgb,
                                                          guint             coord);

enum {
  ROUND_NEAREST,
  ROUND_UP,
  ROUND_DOWN,
  ROUND_TO_ZERO,
};

BobguiCssValue *   bobgui_css_math_value_new              (guint                    type,
                                                     guint                    mode,
                                                     BobguiCssValue            **values,
                                                     guint                    n_values);

G_END_DECLS

