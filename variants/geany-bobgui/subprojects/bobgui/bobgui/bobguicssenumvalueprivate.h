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

#include "bobguienums.h"
#include <bobgui/css/bobguicss.h>
#include "bobgui/css/bobguicsstokenizerprivate.h"
#include "bobgui/css/bobguicssparserprivate.h"
#include "bobguicsstypesprivate.h"
#include "bobguicssvalueprivate.h"

G_BEGIN_DECLS

BobguiCssValue *   _bobgui_css_blend_mode_value_new         (GskBlendMode       blend_mode);
BobguiCssValue *   _bobgui_css_blend_mode_value_try_parse   (BobguiCssParser      *parser);
GskBlendMode    _bobgui_css_blend_mode_value_get         (const BobguiCssValue *value);

BobguiCssValue *   _bobgui_css_border_style_value_new       (BobguiBorderStyle     border_style);
BobguiCssValue *   _bobgui_css_border_style_value_try_parse (BobguiCssParser      *parser);
BobguiBorderStyle  _bobgui_css_border_style_value_get       (const BobguiCssValue *value);

BobguiCssValue *   _bobgui_css_font_size_value_new          (BobguiCssFontSize     size);
BobguiCssValue *   _bobgui_css_font_size_value_try_parse    (BobguiCssParser      *parser);
BobguiCssFontSize  _bobgui_css_font_size_value_get          (const BobguiCssValue *value);
double          bobgui_css_font_size_get_default_px      (BobguiStyleProvider  *provider,
                                                       BobguiCssStyle       *style);

BobguiCssValue *   _bobgui_css_font_style_value_new         (PangoStyle         style);
BobguiCssValue *   _bobgui_css_font_style_value_try_parse   (BobguiCssParser      *parser);
PangoStyle      _bobgui_css_font_style_value_get         (const BobguiCssValue *value);

BobguiCssValue *   bobgui_css_font_weight_value_try_parse   (BobguiCssParser      *parser);
PangoWeight     bobgui_css_font_weight_value_get         (const BobguiCssValue *value);

BobguiCssValue *   _bobgui_css_font_stretch_value_new       (PangoStretch       stretch);
BobguiCssValue *   _bobgui_css_font_stretch_value_try_parse (BobguiCssParser      *parser);
PangoStretch    _bobgui_css_font_stretch_value_get       (const BobguiCssValue *value);

BobguiCssValue *         _bobgui_css_text_decoration_line_value_new     (BobguiTextDecorationLine  line);
BobguiTextDecorationLine _bobgui_css_text_decoration_line_try_parse_one (BobguiCssParser          *parser,
                                                                   BobguiTextDecorationLine  base);
BobguiTextDecorationLine _bobgui_css_text_decoration_line_value_get     (const BobguiCssValue     *value);

BobguiCssValue *          _bobgui_css_text_decoration_style_value_new   (BobguiTextDecorationStyle  style);
BobguiCssValue *          _bobgui_css_text_decoration_style_value_try_parse (BobguiCssParser           *parser);
BobguiTextDecorationStyle _bobgui_css_text_decoration_style_value_get       (const BobguiCssValue      *value);

BobguiCssValue *   _bobgui_css_area_value_new               (BobguiCssArea         area);
BobguiCssValue *   _bobgui_css_area_value_try_parse         (BobguiCssParser      *parser);
BobguiCssArea      _bobgui_css_area_value_get               (const BobguiCssValue *value);

BobguiCssValue *   _bobgui_css_direction_value_new          (BobguiCssDirection    direction);
BobguiCssValue *   _bobgui_css_direction_value_try_parse    (BobguiCssParser      *parser);
BobguiCssDirection _bobgui_css_direction_value_get          (const BobguiCssValue *value);

BobguiCssValue *   _bobgui_css_play_state_value_new         (BobguiCssPlayState    play_state);
BobguiCssValue *   _bobgui_css_play_state_value_try_parse   (BobguiCssParser      *parser);
BobguiCssPlayState _bobgui_css_play_state_value_get         (const BobguiCssValue *value);

BobguiCssValue *   _bobgui_css_fill_mode_value_new          (BobguiCssFillMode     fill_mode);
BobguiCssValue *   _bobgui_css_fill_mode_value_try_parse    (BobguiCssParser      *parser);
BobguiCssFillMode  _bobgui_css_fill_mode_value_get          (const BobguiCssValue *value);

BobguiCssValue *   _bobgui_css_icon_style_value_new         (BobguiCssIconStyle    icon_style);
BobguiCssValue *   _bobgui_css_icon_style_value_try_parse   (BobguiCssParser      *parser);
BobguiCssIconStyle _bobgui_css_icon_style_value_get         (const BobguiCssValue *value);

BobguiCssValue *     _bobgui_css_font_kerning_value_new       (BobguiCssFontKerning  kerning);
BobguiCssValue *     _bobgui_css_font_kerning_value_try_parse (BobguiCssParser      *parser);
BobguiCssFontKerning _bobgui_css_font_kerning_value_get       (const BobguiCssValue *value);

BobguiCssValue *        _bobgui_css_font_variant_position_value_new       (BobguiCssFontVariantPosition  position);
BobguiCssValue *        _bobgui_css_font_variant_position_value_try_parse (BobguiCssParser         *parser);
BobguiCssFontVariantPosition _bobgui_css_font_variant_position_value_get       (const BobguiCssValue    *value);

BobguiCssValue *         _bobgui_css_font_variant_caps_value_new       (BobguiCssFontVariantCaps  caps);
BobguiCssValue *         _bobgui_css_font_variant_caps_value_try_parse (BobguiCssParser          *parser);
BobguiCssFontVariantCaps _bobgui_css_font_variant_caps_value_get       (const BobguiCssValue     *value);

BobguiCssValue *        _bobgui_css_font_variant_alternate_value_new       (BobguiCssFontVariantAlternate  alternates);
BobguiCssValue *        _bobgui_css_font_variant_alternate_value_try_parse (BobguiCssParser         *parser);
BobguiCssFontVariantAlternate _bobgui_css_font_variant_alternate_value_get       (const BobguiCssValue    *value);

BobguiCssValue *        _bobgui_css_font_variant_ligature_value_new          (BobguiCssFontVariantLigature  ligatures);
BobguiCssFontVariantLigature _bobgui_css_font_variant_ligature_try_parse_one (BobguiCssParser               *parser,
                                                                        BobguiCssFontVariantLigature   base);
BobguiCssFontVariantLigature _bobgui_css_font_variant_ligature_value_get     (const BobguiCssValue          *value);

BobguiCssValue *        _bobgui_css_font_variant_numeric_value_new          (BobguiCssFontVariantNumeric     numeric);
BobguiCssFontVariantNumeric _bobgui_css_font_variant_numeric_try_parse_one (BobguiCssParser               *parser,
                                                                      BobguiCssFontVariantNumeric    base);
BobguiCssFontVariantNumeric _bobgui_css_font_variant_numeric_value_get     (const BobguiCssValue          *value);


BobguiCssValue *        _bobgui_css_font_variant_east_asian_value_new          (BobguiCssFontVariantEastAsian east_asian);
BobguiCssFontVariantEastAsian _bobgui_css_font_variant_east_asian_try_parse_one (BobguiCssParser               *parser,
                                                                      BobguiCssFontVariantEastAsian base);
BobguiCssFontVariantEastAsian _bobgui_css_font_variant_east_asian_value_get     (const BobguiCssValue          *value);

BobguiCssValue *          _bobgui_css_text_transform_value_new       (BobguiTextTransform transform);
BobguiCssValue *          _bobgui_css_text_transform_value_try_parse (BobguiCssParser           *parser);
BobguiTextTransform       _bobgui_css_text_transform_value_get       (const BobguiCssValue      *value);
G_END_DECLS

