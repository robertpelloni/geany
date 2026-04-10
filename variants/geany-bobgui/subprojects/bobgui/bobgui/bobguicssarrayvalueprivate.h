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
#include "bobguitypes.h"

G_BEGIN_DECLS

BobguiCssValue *       _bobgui_css_array_value_new            (BobguiCssValue           *content);
BobguiCssValue *       _bobgui_css_array_value_new_from_array (BobguiCssValue          **values,
                                                         guint                  n_values);
BobguiCssValue *       _bobgui_css_array_value_parse          (BobguiCssParser          *parser,
                                                         BobguiCssValue *          (* parse_func) (BobguiCssParser *));

BobguiCssValue *       _bobgui_css_array_value_get_nth        (BobguiCssValue           *value,
                                                         guint                  i);
guint               _bobgui_css_array_value_get_n_values   (const BobguiCssValue     *value) G_GNUC_PURE;

G_END_DECLS

