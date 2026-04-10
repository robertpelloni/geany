/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
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

#include <bobgui/css/bobguicss.h>
#include "bobgui/css/bobguicsstokenizerprivate.h"
#include "bobgui/css/bobguicssparserprivate.h"
#include "bobguicssvalueprivate.h"
#include "bobguicsscolorprivate.h"

G_BEGIN_DECLS

BobguiCssValue *   bobgui_css_color_value_new_transparent     (void) G_GNUC_PURE;
BobguiCssValue *   bobgui_css_color_value_new_white           (void) G_GNUC_PURE;
BobguiCssValue *   bobgui_css_color_value_new_current_color   (void) G_GNUC_PURE;
BobguiCssValue *   bobgui_css_color_value_new_name            (const char     *name) G_GNUC_PURE;

gboolean        bobgui_css_color_value_can_parse           (BobguiCssParser   *parser);
BobguiCssValue *   bobgui_css_color_value_parse               (BobguiCssParser   *parser);

const GdkRGBA * bobgui_css_color_value_get_rgba            (const BobguiCssValue *color) G_GNUC_CONST;

BobguiCssValue *   bobgui_css_color_value_new_color           (BobguiCssColorSpace color_space,
                                                         gboolean         serialize_as_rgb,
                                                         const float      values[4],
                                                         gboolean         missing[4]) G_GNUC_PURE;

const BobguiCssColor *
                bobgui_css_color_value_get_color           (const BobguiCssValue *color) G_GNUC_CONST;

float           bobgui_css_color_value_get_coord           (const BobguiCssValue *color,
                                                         BobguiCssColorSpace   color_space,
                                                         gboolean           legacy_srgb,
                                                         guint              coord);

G_END_DECLS

