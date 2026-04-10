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

BobguiCssValue *   _bobgui_css_corner_value_new           (BobguiCssValue            *x,
                                                     BobguiCssValue            *y);
BobguiCssValue *   _bobgui_css_corner_value_parse         (BobguiCssParser           *parser);

double          _bobgui_css_corner_value_get_x         (const BobguiCssValue      *corner,
                                                     double                  one_hundred_percent) G_GNUC_PURE;
double          _bobgui_css_corner_value_get_y         (const BobguiCssValue      *corner,
                                                     double                  one_hundred_percent) G_GNUC_PURE;
gboolean        bobgui_css_corner_value_is_zero        (const BobguiCssValue      *corner) G_GNUC_PURE;


G_END_DECLS

