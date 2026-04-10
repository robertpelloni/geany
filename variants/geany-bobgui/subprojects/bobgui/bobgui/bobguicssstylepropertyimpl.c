/*
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

#include "config.h"

#include "bobguistylepropertyprivate.h"

#include <gobject/gvaluecollector.h>
#include <math.h>

#include <bobgui/css/bobguicss.h>
#include "bobgui/css/bobguicsstokenizerprivate.h"
#include "bobgui/css/bobguicssparserprivate.h"
#include "bobguicssstylepropertyprivate.h"
#include "bobguicsstypesprivate.h"
#include "bobguiprivatetypebuiltins.h"

/* the actual parsers we have */
#include "bobguicssarrayvalueprivate.h"
#include "bobguicssbgsizevalueprivate.h"
#include "bobguicssbordervalueprivate.h"
#include "bobguicsscolorvalueprivate.h"
#include "bobguicsscornervalueprivate.h"
#include "bobguicsseasevalueprivate.h"
#include "bobguicssfiltervalueprivate.h"
#include "bobguicssfontfeaturesvalueprivate.h"
#include "bobguicssimageprivate.h"
#include "bobguicssimagevalueprivate.h"
#include "bobguicssinitialvalueprivate.h"
#include "bobguicssenumvalueprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicsspalettevalueprivate.h"
#include "bobguicsspositionvalueprivate.h"
#include "bobguicssrepeatvalueprivate.h"
#include "bobguicssshadowvalueprivate.h"
#include "bobguicssstringvalueprivate.h"
#include "bobguicsstransformvalueprivate.h"
#include "bobguicssfontvariationsvalueprivate.h"
#include "bobguicsslineheightvalueprivate.h"
#include "bobguitypebuiltins.h"

/*** REGISTRATION ***/

typedef enum {
  BOBGUI_STYLE_PROPERTY_INHERIT = (1 << 0),
  BOBGUI_STYLE_PROPERTY_ANIMATED = (1 << 1),
} BobguiStylePropertyFlags;

static void
bobgui_css_style_property_register (const char *                   name,
                                 guint                          expected_id,
                                 BobguiStylePropertyFlags          flags,
                                 BobguiCssAffects                  affects,
                                 BobguiCssStylePropertyParseFunc   parse_value,
                                 BobguiCssValue *                  initial_value)
{
  BobguiCssStyleProperty *node;

  g_assert (initial_value != NULL);
  g_assert (parse_value != NULL);

  node = g_object_new (BOBGUI_TYPE_CSS_STYLE_PROPERTY,
                       "affects", affects,
                       "animated", (flags & BOBGUI_STYLE_PROPERTY_ANIMATED) ? TRUE : FALSE,
                       "inherit", (flags & BOBGUI_STYLE_PROPERTY_INHERIT) ? TRUE : FALSE,
                       "initial-value", initial_value,
                       "name", name,
                       NULL);

  node->parse_value = parse_value;

  bobgui_css_value_unref (initial_value);

  g_assert (_bobgui_css_style_property_get_id (node) == expected_id);
}

/*** IMPLEMENTATIONS ***/

static BobguiCssValue *
color_parse (BobguiCssStyleProperty *property,
             BobguiCssParser        *parser)
{
  return bobgui_css_color_value_parse (parser);
}

static BobguiCssValue *
font_family_parse_one (BobguiCssParser *parser)
{
  char *name;

  if (bobgui_css_parser_has_token (parser, BOBGUI_CSS_TOKEN_IDENT))
    {
      GString *string = g_string_new (NULL);

      name = bobgui_css_parser_consume_ident (parser);
      g_string_append (string, name);
      g_free (name);
      while (bobgui_css_parser_has_token (parser, BOBGUI_CSS_TOKEN_IDENT))
        {
          name = bobgui_css_parser_consume_ident (parser);
          g_string_append_c (string, ' ');
          g_string_append (string, name);
          g_free (name);
        }
      name = g_string_free (string, FALSE);
    }
  else 
    {
      name = bobgui_css_parser_consume_string (parser);
      if (name == NULL)
        return NULL;
    }

  return _bobgui_css_string_value_new_take (name);
}

BobguiCssValue *
bobgui_css_font_family_value_parse (BobguiCssParser *parser)
{
  return _bobgui_css_array_value_parse (parser, font_family_parse_one);
}

static BobguiCssValue *
font_family_parse (BobguiCssStyleProperty *property,
                   BobguiCssParser        *parser)
{
  return bobgui_css_font_family_value_parse (parser);
}

static BobguiCssValue *
font_style_parse (BobguiCssStyleProperty *property,
                  BobguiCssParser        *parser)
{
  BobguiCssValue *value = _bobgui_css_font_style_value_try_parse (parser);
  
  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "unknown font style value");

  return value;
}

static BobguiCssValue *
font_weight_parse (BobguiCssStyleProperty *property,
                   BobguiCssParser        *parser)
{
  BobguiCssValue *value;
  
  value = bobgui_css_font_weight_value_try_parse (parser);
  if (value == NULL)
    {
      value = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER | BOBGUI_CSS_POSITIVE_ONLY);
      if (value == NULL)
        return NULL;

      if (bobgui_css_number_value_get (value, 100) < 1 || 
          bobgui_css_number_value_get (value, 100) > 1000)
        {
          bobgui_css_parser_error_value (parser, "Font weight values must be between 1 and 1000");
          g_clear_pointer (&value, bobgui_css_value_unref);
        }
    }

  return value;
}

static BobguiCssValue *
font_stretch_parse (BobguiCssStyleProperty *property,
                    BobguiCssParser        *parser)
{
  BobguiCssValue *value = _bobgui_css_font_stretch_value_try_parse (parser);

  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "unknown font stretch value");

  return value;
}

static BobguiCssValue *
parse_border_style (BobguiCssStyleProperty *property,
                    BobguiCssParser        *parser)
{
  BobguiCssValue *value = _bobgui_css_border_style_value_try_parse (parser);
  
  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "unknown border style value");

  return value;
}

static BobguiCssValue *
parse_css_area_one (BobguiCssParser *parser)
{
  BobguiCssValue *value = _bobgui_css_area_value_try_parse (parser);
  
  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "unknown box value");

  return value;
}

static BobguiCssValue *
parse_css_area (BobguiCssStyleProperty *property,
                BobguiCssParser        *parser)
{
  return _bobgui_css_array_value_parse (parser, parse_css_area_one);
}

static BobguiCssValue *
parse_one_css_direction (BobguiCssParser *parser)
{
  BobguiCssValue *value = _bobgui_css_direction_value_try_parse (parser);
  
  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "unknown direction value");

  return value;
}

static BobguiCssValue *
parse_css_direction (BobguiCssStyleProperty *property,
                     BobguiCssParser        *parser)
{
  return _bobgui_css_array_value_parse (parser, parse_one_css_direction);
}

static BobguiCssValue *
opacity_parse (BobguiCssStyleProperty *property,
               BobguiCssParser        *parser)
{
  return bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER
                                             | BOBGUI_CSS_PARSE_PERCENT);
}

static BobguiCssValue *
parse_one_css_play_state (BobguiCssParser *parser)
{
  BobguiCssValue *value = _bobgui_css_play_state_value_try_parse (parser);
  
  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "unknown play state value");

  return value;
}

static BobguiCssValue *
parse_css_play_state (BobguiCssStyleProperty *property,
                      BobguiCssParser        *parser)
{
  return _bobgui_css_array_value_parse (parser, parse_one_css_play_state);
}

static BobguiCssValue *
parse_one_css_fill_mode (BobguiCssParser *parser)
{
  BobguiCssValue *value = _bobgui_css_fill_mode_value_try_parse (parser);
  
  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "unknown fill mode value");

  return value;
}

static BobguiCssValue *
parse_css_fill_mode (BobguiCssStyleProperty *property,
                     BobguiCssParser        *parser)
{
  return _bobgui_css_array_value_parse (parser, parse_one_css_fill_mode);
}

static BobguiCssValue *
icon_size_parse (BobguiCssStyleProperty *property,
		 BobguiCssParser        *parser)
{
  return bobgui_css_number_value_parse (parser, 
                                     BOBGUI_CSS_PARSE_LENGTH
                                     | BOBGUI_CSS_PARSE_PERCENT
                                     | BOBGUI_CSS_POSITIVE_ONLY);
}

static BobguiCssValue *
icon_palette_parse (BobguiCssStyleProperty *property,
		    BobguiCssParser        *parser)
{
  return bobgui_css_palette_value_parse (parser);
}

static BobguiCssValue *
icon_style_parse (BobguiCssStyleProperty *property,
		  BobguiCssParser        *parser)
{
  BobguiCssValue *value = _bobgui_css_icon_style_value_try_parse (parser);

  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "unknown icon style value");

  return value;
}

static BobguiCssValue *
icon_weight_parse (BobguiCssStyleProperty *property,
                   BobguiCssParser        *parser)
{
  BobguiCssValue *value;

  value = bobgui_css_font_weight_value_try_parse (parser);
  if (value == NULL)
    {
      value = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER | BOBGUI_CSS_POSITIVE_ONLY);
      if (value == NULL)
        return NULL;

      if (bobgui_css_number_value_get (value, 100) < 1 ||
          bobgui_css_number_value_get (value, 100) > 1000)
        {
          bobgui_css_parser_error_value (parser, "Icon weight values must be between 1 and 1000");
          g_clear_pointer (&value, bobgui_css_value_unref);
        }
    }

  return value;
}

static BobguiCssValue *
parse_letter_spacing (BobguiCssStyleProperty *property,
                      BobguiCssParser        *parser)
{
  return bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_LENGTH);
}

static gboolean
value_is_done_parsing (BobguiCssParser *parser)
{
  return bobgui_css_parser_has_token (parser, BOBGUI_CSS_TOKEN_EOF) ||
         bobgui_css_parser_has_token (parser, BOBGUI_CSS_TOKEN_COMMA) ||
         bobgui_css_parser_has_token (parser, BOBGUI_CSS_TOKEN_SEMICOLON) ||
         bobgui_css_parser_has_token (parser, BOBGUI_CSS_TOKEN_CLOSE_CURLY);
}

static BobguiCssValue *
parse_text_decoration_line (BobguiCssStyleProperty *property,
                            BobguiCssParser        *parser)
{
  BobguiCssValue *value = NULL;
  BobguiTextDecorationLine line;

  line = 0;
  do {
    BobguiTextDecorationLine parsed;

    parsed = _bobgui_css_text_decoration_line_try_parse_one (parser, line);
    if (parsed == 0 || parsed == line)
      {
        bobgui_css_parser_error_syntax (parser, "Not a valid value");
        return NULL;
      }
    line = parsed;
  } while (!value_is_done_parsing (parser));

  value = _bobgui_css_text_decoration_line_value_new (line);
  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "Invalid combination of values");

  return value;
}

static BobguiCssValue *
parse_text_decoration_style (BobguiCssStyleProperty *property,
                             BobguiCssParser        *parser)
{
  BobguiCssValue *value = _bobgui_css_text_decoration_style_value_try_parse (parser);

  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "unknown text decoration style value");

  return value;
}

static BobguiCssValue *
parse_text_transform (BobguiCssStyleProperty *property,
                      BobguiCssParser        *parser)
{
  BobguiCssValue *value = _bobgui_css_text_transform_value_try_parse (parser);

  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "unknown text transform value");

  return value;
}

static BobguiCssValue *
parse_font_kerning (BobguiCssStyleProperty *property,
                    BobguiCssParser        *parser)
{
  BobguiCssValue *value = _bobgui_css_font_kerning_value_try_parse (parser);

  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "unknown font kerning value");

  return value;
}

static BobguiCssValue *
parse_font_variant_ligatures (BobguiCssStyleProperty *property,
                              BobguiCssParser        *parser)
{
  BobguiCssValue *value = NULL;
  BobguiCssFontVariantLigature ligatures;

  ligatures = 0;
  do {
    BobguiCssFontVariantLigature parsed;

    parsed = _bobgui_css_font_variant_ligature_try_parse_one (parser, ligatures);
    if (parsed == 0 || parsed == ligatures)
      {
        bobgui_css_parser_error_syntax (parser, "Not a valid value");
        return NULL;
      }
    ligatures = parsed;
  } while (!value_is_done_parsing (parser));

  value = _bobgui_css_font_variant_ligature_value_new (ligatures);
  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "Invalid combination of values");

  return value;
}

static BobguiCssValue *
parse_font_variant_position (BobguiCssStyleProperty *property,
                             BobguiCssParser        *parser)
{
  BobguiCssValue *value = _bobgui_css_font_variant_position_value_try_parse (parser);

  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "unknown font variant position value");

  return value;
}

static BobguiCssValue *
parse_font_variant_caps (BobguiCssStyleProperty *property,
                         BobguiCssParser        *parser)
{
  BobguiCssValue *value = _bobgui_css_font_variant_caps_value_try_parse (parser);

  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "unknown font variant caps value");

  return value;
}

static BobguiCssValue *
parse_font_variant_numeric (BobguiCssStyleProperty *property,
                            BobguiCssParser        *parser)
{
  BobguiCssValue *value = NULL;
  BobguiCssFontVariantNumeric numeric;

  numeric = 0;
  do {
    BobguiCssFontVariantNumeric parsed;

    parsed = _bobgui_css_font_variant_numeric_try_parse_one (parser, numeric);
    if (parsed == 0 || parsed == numeric)
      {
        bobgui_css_parser_error_syntax (parser, "Not a valid value");
        return NULL;
      }
    numeric = parsed;
  } while (!value_is_done_parsing (parser));

  value = _bobgui_css_font_variant_numeric_value_new (numeric);
  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "Invalid combination of values");

  return value;
}

static BobguiCssValue *
parse_font_variant_alternates (BobguiCssStyleProperty *property,
                               BobguiCssParser        *parser)
{
  BobguiCssValue *value = _bobgui_css_font_variant_alternate_value_try_parse (parser);

  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "unknown font variant alternate value");

  return value;
}

static BobguiCssValue *
parse_font_variant_east_asian (BobguiCssStyleProperty *property,
                               BobguiCssParser        *parser)
{
  BobguiCssValue *value = NULL;
  BobguiCssFontVariantEastAsian east_asian;

  east_asian = 0;
  do {
    BobguiCssFontVariantEastAsian parsed;

    parsed = _bobgui_css_font_variant_east_asian_try_parse_one (parser, east_asian);
    if (parsed == 0 || parsed == east_asian)
      {
        bobgui_css_parser_error_syntax (parser, "Not a valid value");
        return NULL;
      }
    east_asian = parsed;
  } while (!value_is_done_parsing (parser));

  value = _bobgui_css_font_variant_east_asian_value_new (east_asian);
  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "Invalid combination of values");

  return value;
}

static BobguiCssValue *
parse_font_feature_settings (BobguiCssStyleProperty *property,
                             BobguiCssParser        *parser)
{
  return bobgui_css_font_features_value_parse (parser);
}

static BobguiCssValue *
parse_font_variation_settings (BobguiCssStyleProperty *property,
                               BobguiCssParser        *parser)
{
  return bobgui_css_font_variations_value_parse (parser);
}

static BobguiCssValue *
box_shadow_value_parse (BobguiCssStyleProperty *property,
                        BobguiCssParser        *parser)
{
  return bobgui_css_shadow_value_parse (parser, TRUE);
}

static BobguiCssValue *
shadow_value_parse (BobguiCssStyleProperty *property,
                    BobguiCssParser        *parser)
{
  return bobgui_css_shadow_value_parse (parser, FALSE);
}

static BobguiCssValue *
transform_value_parse (BobguiCssStyleProperty *property,
                       BobguiCssParser        *parser)
{
  return _bobgui_css_transform_value_parse (parser);
}

static BobguiCssValue *
filter_value_parse (BobguiCssStyleProperty *property,
                    BobguiCssParser        *parser)
{
  return bobgui_css_filter_value_parse (parser);
}

static BobguiCssValue *
border_spacing_value_parse (BobguiCssStyleProperty *property,
                            BobguiCssParser        *parser)
{
  return bobgui_css_position_value_parse_spacing (parser);
}

static BobguiCssValue *
border_corner_radius_value_parse (BobguiCssStyleProperty *property,
                                  BobguiCssParser        *parser)
{
  return _bobgui_css_corner_value_parse (parser);
}

static BobguiCssValue *
css_image_value_parse (BobguiCssStyleProperty *property,
                       BobguiCssParser        *parser)
{
  BobguiCssImage *image;

  if (bobgui_css_parser_try_ident (parser, "none"))
    image = NULL;
  else
    {
      image = _bobgui_css_image_new_parse (parser);
      if (image == NULL)
        return NULL;
    }

  return _bobgui_css_image_value_new (image);
}

static BobguiCssValue *
background_image_value_parse_one (BobguiCssParser *parser)
{
  return css_image_value_parse (NULL, parser);
}

static BobguiCssValue *
background_image_value_parse (BobguiCssStyleProperty *property,
                              BobguiCssParser        *parser)
{
  return _bobgui_css_array_value_parse (parser, background_image_value_parse_one);
}

static BobguiCssValue *
dpi_parse (BobguiCssStyleProperty *property,
	   BobguiCssParser        *parser)
{
  return bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER);
}

BobguiCssValue *
bobgui_css_font_size_value_parse (BobguiCssParser *parser)
{
  BobguiCssValue *value;

  value = _bobgui_css_font_size_value_try_parse (parser);
  if (value)
    return value;

  return bobgui_css_number_value_parse (parser,
                                     BOBGUI_CSS_PARSE_LENGTH
                                     | BOBGUI_CSS_PARSE_PERCENT
                                     | BOBGUI_CSS_POSITIVE_ONLY);
}

static BobguiCssValue *
font_size_parse (BobguiCssStyleProperty *property,
                 BobguiCssParser        *parser)
{
  return bobgui_css_font_size_value_parse (parser);
}

static BobguiCssValue *
outline_parse (BobguiCssStyleProperty *property,
               BobguiCssParser        *parser)
{
  return bobgui_css_number_value_parse (parser,
                                     BOBGUI_CSS_PARSE_LENGTH);
}

static BobguiCssValue *
border_image_repeat_parse (BobguiCssStyleProperty *property,
                           BobguiCssParser        *parser)
{
  BobguiCssValue *value = _bobgui_css_border_repeat_value_try_parse (parser);

  if (value == NULL)
    {
      bobgui_css_parser_error_syntax (parser, "Not a valid border repeat value");
      return NULL;
    }

  return value;
}

static BobguiCssValue *
border_image_slice_parse (BobguiCssStyleProperty *property,
                          BobguiCssParser        *parser)
{
  return _bobgui_css_border_value_parse (parser,
                                      BOBGUI_CSS_PARSE_PERCENT
                                      | BOBGUI_CSS_PARSE_NUMBER
                                      | BOBGUI_CSS_POSITIVE_ONLY,
                                      FALSE,
                                      TRUE);
}

static BobguiCssValue *
border_image_width_parse (BobguiCssStyleProperty *property,
                          BobguiCssParser        *parser)
{
  return _bobgui_css_border_value_parse (parser,
                                      BOBGUI_CSS_PARSE_PERCENT
                                      | BOBGUI_CSS_PARSE_LENGTH
                                      | BOBGUI_CSS_PARSE_NUMBER
                                      | BOBGUI_CSS_POSITIVE_ONLY,
                                      TRUE,
                                      FALSE);
}

static BobguiCssValue *
minmax_parse (BobguiCssStyleProperty *property,
              BobguiCssParser        *parser)
{
  return bobgui_css_number_value_parse (parser,
                                     BOBGUI_CSS_PARSE_LENGTH
                                     | BOBGUI_CSS_POSITIVE_ONLY);
}

static BobguiCssValue *
transition_property_parse_one (BobguiCssParser *parser)
{
  BobguiCssValue *value;

  value = _bobgui_css_ident_value_try_parse (parser);

  if (value == NULL)
    {
      bobgui_css_parser_error_syntax (parser, "Expected an identifier");
      return NULL;
    }

  return value;
}

static BobguiCssValue *
transition_property_parse (BobguiCssStyleProperty *property,
                           BobguiCssParser        *parser)
{
  return _bobgui_css_array_value_parse (parser, transition_property_parse_one);
}

static BobguiCssValue *
transition_time_parse_one (BobguiCssParser *parser)
{
  return bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_TIME);
}

static BobguiCssValue *
transition_time_parse (BobguiCssStyleProperty *property,
                       BobguiCssParser        *parser)
{
  return _bobgui_css_array_value_parse (parser, transition_time_parse_one);
}

static BobguiCssValue *
transition_timing_function_parse (BobguiCssStyleProperty *property,
                                  BobguiCssParser        *parser)
{
  return _bobgui_css_array_value_parse (parser, _bobgui_css_ease_value_parse);
}

static BobguiCssValue *
iteration_count_parse_one (BobguiCssParser *parser)
{
  if (bobgui_css_parser_try_ident (parser, "infinite"))
    return bobgui_css_number_value_new (HUGE_VAL, BOBGUI_CSS_NUMBER);

  return bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER | BOBGUI_CSS_POSITIVE_ONLY);
}

static BobguiCssValue *
iteration_count_parse (BobguiCssStyleProperty *property,
                       BobguiCssParser        *parser)
{
  return _bobgui_css_array_value_parse (parser, iteration_count_parse_one);
}

static BobguiCssValue *
parse_margin (BobguiCssStyleProperty *property,
              BobguiCssParser        *parser)
{
  return bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_LENGTH);
}

static BobguiCssValue *
parse_padding (BobguiCssStyleProperty *property,
               BobguiCssParser        *parser)
{
  return bobgui_css_number_value_parse (parser,
                                     BOBGUI_CSS_POSITIVE_ONLY
                                     | BOBGUI_CSS_PARSE_LENGTH);
}

static BobguiCssValue *
parse_border_width (BobguiCssStyleProperty *property,
                    BobguiCssParser        *parser)
{
  return bobgui_css_number_value_parse (parser,
                                     BOBGUI_CSS_POSITIVE_ONLY
                                     | BOBGUI_CSS_PARSE_LENGTH);
}

static BobguiCssValue *
blend_mode_value_parse_one (BobguiCssParser        *parser)
{
  BobguiCssValue *value = _bobgui_css_blend_mode_value_try_parse (parser);

  if (value == NULL)
    bobgui_css_parser_error_syntax (parser, "Unknown blend mode value");

  return value;
}

static BobguiCssValue *
blend_mode_value_parse (BobguiCssStyleProperty *property,
                        BobguiCssParser        *parser)
{
  return _bobgui_css_array_value_parse (parser, blend_mode_value_parse_one);
}

static BobguiCssValue *
background_repeat_value_parse_one (BobguiCssParser *parser)
{
  BobguiCssValue *value = _bobgui_css_background_repeat_value_try_parse (parser);

  if (value == NULL)
    {
      bobgui_css_parser_error_syntax (parser, "Unknown repeat value");
      return NULL;
    }

  return value;
}

static BobguiCssValue *
background_repeat_value_parse (BobguiCssStyleProperty *property,
                               BobguiCssParser        *parser)
{
  return _bobgui_css_array_value_parse (parser, background_repeat_value_parse_one);
}

static BobguiCssValue *
background_size_parse (BobguiCssStyleProperty *property,
                       BobguiCssParser        *parser)
{
  return _bobgui_css_array_value_parse (parser, _bobgui_css_bg_size_value_parse);
}

static BobguiCssValue *
background_position_parse (BobguiCssStyleProperty *property,
			   BobguiCssParser        *parser)
{
  return _bobgui_css_array_value_parse (parser, _bobgui_css_position_value_parse);
}

static BobguiCssValue *
transform_origin_parse (BobguiCssStyleProperty *property,
                        BobguiCssParser        *parser)
{
  return _bobgui_css_position_value_parse (parser);
}

static BobguiCssValue *
parse_line_height (BobguiCssStyleProperty *property,
                   BobguiCssParser        *parser)
{
  return bobgui_css_line_height_value_parse (parser);
}

/*** REGISTRATION ***/

G_STATIC_ASSERT (BOBGUI_CSS_PROPERTY_COLOR == 0);
G_STATIC_ASSERT (BOBGUI_CSS_PROPERTY_DPI < BOBGUI_CSS_PROPERTY_FONT_SIZE);

void
_bobgui_css_style_property_init_properties (void)
{
  /* Initialize "color", "-bobgui-dpi" and "font-size" first,
   * so that when computing values later they are
   * done first. That way, 'currentColor' and font
   * sizes in em can be looked up properly */
  bobgui_css_style_property_register        ("color",
                                          BOBGUI_CSS_PROPERTY_COLOR,
                                          BOBGUI_STYLE_PROPERTY_INHERIT | BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_CONTENT | BOBGUI_CSS_AFFECTS_ICON_REDRAW_SYMBOLIC,
                                          color_parse,
                                          bobgui_css_color_value_new_white ());
  bobgui_css_style_property_register        ("-bobgui-dpi",
                                          BOBGUI_CSS_PROPERTY_DPI,
                                          BOBGUI_STYLE_PROPERTY_INHERIT | BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_SIZE | BOBGUI_CSS_AFFECTS_TEXT_SIZE,
                                          dpi_parse,
                                          bobgui_css_number_value_new (96.0, BOBGUI_CSS_NUMBER));
  bobgui_css_style_property_register        ("font-size",
                                          BOBGUI_CSS_PROPERTY_FONT_SIZE,
                                          BOBGUI_STYLE_PROPERTY_INHERIT | BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_SIZE | BOBGUI_CSS_AFFECTS_TEXT_SIZE,
                                          font_size_parse,
                                          _bobgui_css_font_size_value_new (BOBGUI_CSS_FONT_SIZE_MEDIUM));
  bobgui_css_style_property_register        ("-bobgui-icon-palette",
					  BOBGUI_CSS_PROPERTY_ICON_PALETTE,
					  BOBGUI_STYLE_PROPERTY_ANIMATED | BOBGUI_STYLE_PROPERTY_INHERIT,
                                          BOBGUI_CSS_AFFECTS_ICON_REDRAW_SYMBOLIC,
					  icon_palette_parse,
					  bobgui_css_palette_value_new_default ());


  /* properties that aren't referenced when computing values
   * start here */
  bobgui_css_style_property_register        ("background-color",
                                          BOBGUI_CSS_PROPERTY_BACKGROUND_COLOR,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_BACKGROUND,
                                          color_parse,
                                          bobgui_css_color_value_new_transparent ());

  bobgui_css_style_property_register        ("font-family",
                                          BOBGUI_CSS_PROPERTY_FONT_FAMILY,
                                          BOBGUI_STYLE_PROPERTY_INHERIT,
                                          BOBGUI_CSS_AFFECTS_TEXT_SIZE,
                                          font_family_parse,
                                          _bobgui_css_string_value_new ("Sans"));
  bobgui_css_style_property_register        ("font-style",
                                          BOBGUI_CSS_PROPERTY_FONT_STYLE,
                                          BOBGUI_STYLE_PROPERTY_INHERIT,
                                          BOBGUI_CSS_AFFECTS_TEXT_SIZE,
                                          font_style_parse,
                                          _bobgui_css_font_style_value_new (PANGO_STYLE_NORMAL));
  bobgui_css_style_property_register        ("font-weight",
                                          BOBGUI_CSS_PROPERTY_FONT_WEIGHT,
                                          BOBGUI_STYLE_PROPERTY_INHERIT | BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_TEXT_SIZE,
                                          font_weight_parse,
                                          bobgui_css_number_value_new (PANGO_WEIGHT_NORMAL, BOBGUI_CSS_NUMBER));
  bobgui_css_style_property_register        ("font-stretch",
                                          BOBGUI_CSS_PROPERTY_FONT_STRETCH,
                                          BOBGUI_STYLE_PROPERTY_INHERIT,
                                          BOBGUI_CSS_AFFECTS_TEXT_SIZE,
                                          font_stretch_parse,
                                          _bobgui_css_font_stretch_value_new (PANGO_STRETCH_NORMAL));

  bobgui_css_style_property_register        ("letter-spacing",
                                          BOBGUI_CSS_PROPERTY_LETTER_SPACING,
                                          BOBGUI_STYLE_PROPERTY_INHERIT | BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_TEXT_ATTRS | BOBGUI_CSS_AFFECTS_TEXT_SIZE,
                                          parse_letter_spacing,
                                          bobgui_css_number_value_new (0.0, BOBGUI_CSS_PX));

  bobgui_css_style_property_register        ("text-decoration-line",
                                          BOBGUI_CSS_PROPERTY_TEXT_DECORATION_LINE,
                                          0,
                                          BOBGUI_CSS_AFFECTS_TEXT_ATTRS,
                                          parse_text_decoration_line,
                                          _bobgui_css_text_decoration_line_value_new (BOBGUI_CSS_TEXT_DECORATION_LINE_NONE));
  bobgui_css_style_property_register        ("text-decoration-color",
                                          BOBGUI_CSS_PROPERTY_TEXT_DECORATION_COLOR,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_TEXT_ATTRS,
                                          color_parse,
                                          bobgui_css_color_value_new_current_color ());
  bobgui_css_style_property_register        ("text-decoration-style",
                                          BOBGUI_CSS_PROPERTY_TEXT_DECORATION_STYLE,
                                          0,
                                          BOBGUI_CSS_AFFECTS_TEXT_ATTRS,
                                          parse_text_decoration_style,
                                          _bobgui_css_text_decoration_style_value_new (BOBGUI_CSS_TEXT_DECORATION_STYLE_SOLID));
  bobgui_css_style_property_register        ("text-transform",
                                          BOBGUI_CSS_PROPERTY_TEXT_TRANSFORM,
                                          BOBGUI_STYLE_PROPERTY_INHERIT,
                                          BOBGUI_CSS_AFFECTS_TEXT_ATTRS | BOBGUI_CSS_AFFECTS_TEXT_SIZE,
                                          parse_text_transform,
                                          _bobgui_css_text_transform_value_new (BOBGUI_CSS_TEXT_TRANSFORM_NONE));
  bobgui_css_style_property_register        ("font-kerning",
                                          BOBGUI_CSS_PROPERTY_FONT_KERNING,
                                          BOBGUI_STYLE_PROPERTY_INHERIT,
                                          BOBGUI_CSS_AFFECTS_TEXT_ATTRS | BOBGUI_CSS_AFFECTS_TEXT_SIZE,
                                          parse_font_kerning,
                                          _bobgui_css_font_kerning_value_new (BOBGUI_CSS_FONT_KERNING_AUTO));
  bobgui_css_style_property_register        ("font-variant-ligatures",
                                          BOBGUI_CSS_PROPERTY_FONT_VARIANT_LIGATURES,
                                          BOBGUI_STYLE_PROPERTY_INHERIT,
                                          BOBGUI_CSS_AFFECTS_TEXT_ATTRS,
                                          parse_font_variant_ligatures,
                                          _bobgui_css_font_variant_ligature_value_new (BOBGUI_CSS_FONT_VARIANT_LIGATURE_NORMAL));
  bobgui_css_style_property_register        ("font-variant-position",
                                          BOBGUI_CSS_PROPERTY_FONT_VARIANT_POSITION,
                                          BOBGUI_STYLE_PROPERTY_INHERIT,
                                          BOBGUI_CSS_AFFECTS_TEXT_ATTRS,
                                          parse_font_variant_position,
                                          _bobgui_css_font_variant_position_value_new (BOBGUI_CSS_FONT_VARIANT_POSITION_NORMAL));
  bobgui_css_style_property_register        ("font-variant-caps",
                                          BOBGUI_CSS_PROPERTY_FONT_VARIANT_CAPS,
                                          BOBGUI_STYLE_PROPERTY_INHERIT,
                                          BOBGUI_CSS_AFFECTS_TEXT_ATTRS,
                                          parse_font_variant_caps,
                                          _bobgui_css_font_variant_caps_value_new (BOBGUI_CSS_FONT_VARIANT_CAPS_NORMAL));
  bobgui_css_style_property_register        ("font-variant-numeric",
                                          BOBGUI_CSS_PROPERTY_FONT_VARIANT_NUMERIC,
                                          BOBGUI_STYLE_PROPERTY_INHERIT,
                                          BOBGUI_CSS_AFFECTS_TEXT_ATTRS,
                                          parse_font_variant_numeric,
                                          _bobgui_css_font_variant_numeric_value_new (BOBGUI_CSS_FONT_VARIANT_NUMERIC_NORMAL));
  bobgui_css_style_property_register        ("font-variant-alternates",
                                          BOBGUI_CSS_PROPERTY_FONT_VARIANT_ALTERNATES,
                                          BOBGUI_STYLE_PROPERTY_INHERIT,
                                          BOBGUI_CSS_AFFECTS_TEXT_ATTRS,
                                          parse_font_variant_alternates,
                                          _bobgui_css_font_variant_alternate_value_new (BOBGUI_CSS_FONT_VARIANT_ALTERNATE_NORMAL));
  bobgui_css_style_property_register        ("font-variant-east-asian",
                                          BOBGUI_CSS_PROPERTY_FONT_VARIANT_EAST_ASIAN,
                                          BOBGUI_STYLE_PROPERTY_INHERIT,
                                          BOBGUI_CSS_AFFECTS_TEXT_ATTRS,
                                          parse_font_variant_east_asian,
                                          _bobgui_css_font_variant_east_asian_value_new (BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_NORMAL));
  bobgui_css_style_property_register        ("text-shadow",
                                          BOBGUI_CSS_PROPERTY_TEXT_SHADOW,
                                          BOBGUI_STYLE_PROPERTY_INHERIT | BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_TEXT_CONTENT,
                                          shadow_value_parse,
                                          bobgui_css_shadow_value_new_none ());

  bobgui_css_style_property_register        ("box-shadow",
                                          BOBGUI_CSS_PROPERTY_BOX_SHADOW,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_BACKGROUND,
                                          box_shadow_value_parse,
                                          bobgui_css_shadow_value_new_none ());

  bobgui_css_style_property_register        ("margin-top",
                                          BOBGUI_CSS_PROPERTY_MARGIN_TOP,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_SIZE,
                                          parse_margin,
                                          bobgui_css_number_value_new (0.0, BOBGUI_CSS_PX));
  bobgui_css_style_property_register        ("margin-left",
                                          BOBGUI_CSS_PROPERTY_MARGIN_LEFT,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_SIZE,
                                          parse_margin,
                                          bobgui_css_number_value_new (0.0, BOBGUI_CSS_PX));
  bobgui_css_style_property_register        ("margin-bottom",
                                          BOBGUI_CSS_PROPERTY_MARGIN_BOTTOM,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_SIZE,
                                          parse_margin,
                                          bobgui_css_number_value_new (0.0, BOBGUI_CSS_PX));
  bobgui_css_style_property_register        ("margin-right",
                                          BOBGUI_CSS_PROPERTY_MARGIN_RIGHT,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_SIZE,
                                          parse_margin,
                                          bobgui_css_number_value_new (0.0, BOBGUI_CSS_PX));
  bobgui_css_style_property_register        ("padding-top",
                                          BOBGUI_CSS_PROPERTY_PADDING_TOP,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_SIZE,
                                          parse_padding,
                                          bobgui_css_number_value_new (0.0, BOBGUI_CSS_PX));
  bobgui_css_style_property_register        ("padding-left",
                                          BOBGUI_CSS_PROPERTY_PADDING_LEFT,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_SIZE,
                                          parse_padding,
                                          bobgui_css_number_value_new (0.0, BOBGUI_CSS_PX));
  bobgui_css_style_property_register        ("padding-bottom",
                                          BOBGUI_CSS_PROPERTY_PADDING_BOTTOM,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_SIZE,
                                          parse_padding,
                                          bobgui_css_number_value_new (0.0, BOBGUI_CSS_PX));
  bobgui_css_style_property_register        ("padding-right",
                                          BOBGUI_CSS_PROPERTY_PADDING_RIGHT,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_SIZE,
                                          parse_padding,
                                          bobgui_css_number_value_new (0.0, BOBGUI_CSS_PX));
  /* IMPORTANT: the border-width properties must come after border-style properties,
   * they depend on them for their value computation.
   */
  bobgui_css_style_property_register        ("border-top-style",
                                          BOBGUI_CSS_PROPERTY_BORDER_TOP_STYLE,
                                          0,
                                          BOBGUI_CSS_AFFECTS_BORDER,
                                          parse_border_style,
                                          _bobgui_css_border_style_value_new (BOBGUI_BORDER_STYLE_NONE));
  bobgui_css_style_property_register        ("border-top-width",
                                          BOBGUI_CSS_PROPERTY_BORDER_TOP_WIDTH,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_BORDER | BOBGUI_CSS_AFFECTS_SIZE,
                                          parse_border_width,
                                          bobgui_css_number_value_new (0.0, BOBGUI_CSS_PX));
  bobgui_css_style_property_register        ("border-left-style",
                                          BOBGUI_CSS_PROPERTY_BORDER_LEFT_STYLE,
                                          0,
                                          BOBGUI_CSS_AFFECTS_BORDER,
                                          parse_border_style,
                                          _bobgui_css_border_style_value_new (BOBGUI_BORDER_STYLE_NONE));
  bobgui_css_style_property_register        ("border-left-width",
                                          BOBGUI_CSS_PROPERTY_BORDER_LEFT_WIDTH,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_BORDER | BOBGUI_CSS_AFFECTS_SIZE,
                                          parse_border_width,
                                          bobgui_css_number_value_new (0.0, BOBGUI_CSS_PX));
  bobgui_css_style_property_register        ("border-bottom-style",
                                          BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_STYLE,
                                          0,
                                          BOBGUI_CSS_AFFECTS_BORDER,
                                          parse_border_style,
                                          _bobgui_css_border_style_value_new (BOBGUI_BORDER_STYLE_NONE));
  bobgui_css_style_property_register        ("border-bottom-width",
                                          BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_WIDTH,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_BORDER | BOBGUI_CSS_AFFECTS_SIZE,
                                          parse_border_width,
                                          bobgui_css_number_value_new (0.0, BOBGUI_CSS_PX));
  bobgui_css_style_property_register        ("border-right-style",
                                          BOBGUI_CSS_PROPERTY_BORDER_RIGHT_STYLE,
                                          0,
                                          BOBGUI_CSS_AFFECTS_BORDER,
                                          parse_border_style,
                                          _bobgui_css_border_style_value_new (BOBGUI_BORDER_STYLE_NONE));
  bobgui_css_style_property_register        ("border-right-width",
                                          BOBGUI_CSS_PROPERTY_BORDER_RIGHT_WIDTH,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_BORDER | BOBGUI_CSS_AFFECTS_SIZE,
                                          parse_border_width,
                                          bobgui_css_number_value_new (0.0, BOBGUI_CSS_PX));

  bobgui_css_style_property_register        ("border-top-left-radius",
                                          BOBGUI_CSS_PROPERTY_BORDER_TOP_LEFT_RADIUS,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_BACKGROUND | BOBGUI_CSS_AFFECTS_BORDER,
                                          border_corner_radius_value_parse,
                                          _bobgui_css_corner_value_new (bobgui_css_number_value_new (0, BOBGUI_CSS_PX),
                                                                     bobgui_css_number_value_new (0, BOBGUI_CSS_PX)));
  bobgui_css_style_property_register        ("border-top-right-radius",
                                          BOBGUI_CSS_PROPERTY_BORDER_TOP_RIGHT_RADIUS,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_BACKGROUND | BOBGUI_CSS_AFFECTS_BORDER,
                                          border_corner_radius_value_parse,
                                          _bobgui_css_corner_value_new (bobgui_css_number_value_new (0, BOBGUI_CSS_PX),
                                                                     bobgui_css_number_value_new (0, BOBGUI_CSS_PX)));
  bobgui_css_style_property_register        ("border-bottom-right-radius",
                                          BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_RIGHT_RADIUS,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_BACKGROUND | BOBGUI_CSS_AFFECTS_BORDER,
                                          border_corner_radius_value_parse,
                                          _bobgui_css_corner_value_new (bobgui_css_number_value_new (0, BOBGUI_CSS_PX),
                                                                     bobgui_css_number_value_new (0, BOBGUI_CSS_PX)));
  bobgui_css_style_property_register        ("border-bottom-left-radius",
                                          BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_LEFT_RADIUS,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_BACKGROUND | BOBGUI_CSS_AFFECTS_BORDER,
                                          border_corner_radius_value_parse,
                                          _bobgui_css_corner_value_new (bobgui_css_number_value_new (0, BOBGUI_CSS_PX),
                                                                     bobgui_css_number_value_new (0, BOBGUI_CSS_PX)));

  bobgui_css_style_property_register        ("outline-style",
                                          BOBGUI_CSS_PROPERTY_OUTLINE_STYLE,
                                          0,
                                          BOBGUI_CSS_AFFECTS_OUTLINE,
                                          parse_border_style,
                                          _bobgui_css_border_style_value_new (BOBGUI_BORDER_STYLE_NONE));
  bobgui_css_style_property_register        ("outline-width",
                                          BOBGUI_CSS_PROPERTY_OUTLINE_WIDTH,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_OUTLINE,
                                          parse_border_width,
                                          bobgui_css_number_value_new (0.0, BOBGUI_CSS_PX));
  bobgui_css_style_property_register        ("outline-offset",
                                          BOBGUI_CSS_PROPERTY_OUTLINE_OFFSET,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_OUTLINE,
                                          outline_parse,
                                          bobgui_css_number_value_new (0.0, BOBGUI_CSS_PX));
  bobgui_css_style_property_register        ("background-clip",
                                          BOBGUI_CSS_PROPERTY_BACKGROUND_CLIP,
                                          0,
                                          BOBGUI_CSS_AFFECTS_BACKGROUND,
                                          parse_css_area,
                                          _bobgui_css_area_value_new (BOBGUI_CSS_AREA_BORDER_BOX));
  bobgui_css_style_property_register        ("background-origin",
                                          BOBGUI_CSS_PROPERTY_BACKGROUND_ORIGIN,
                                          0,
                                          BOBGUI_CSS_AFFECTS_BACKGROUND,
                                          parse_css_area,
                                          _bobgui_css_area_value_new (BOBGUI_CSS_AREA_PADDING_BOX));
  bobgui_css_style_property_register        ("background-size",
                                          BOBGUI_CSS_PROPERTY_BACKGROUND_SIZE,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_BACKGROUND,
                                          background_size_parse,
                                          _bobgui_css_bg_size_value_new (NULL, NULL));
  bobgui_css_style_property_register        ("background-position",
                                          BOBGUI_CSS_PROPERTY_BACKGROUND_POSITION,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_BACKGROUND,
                                          background_position_parse,
                                          _bobgui_css_position_value_new (bobgui_css_number_value_new (0, BOBGUI_CSS_PERCENT),
                                                                       bobgui_css_number_value_new (0, BOBGUI_CSS_PERCENT)));

  bobgui_css_style_property_register        ("border-top-color",
                                          BOBGUI_CSS_PROPERTY_BORDER_TOP_COLOR,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_BORDER,
                                          color_parse,
                                          bobgui_css_color_value_new_current_color ());
  bobgui_css_style_property_register        ("border-right-color",
                                          BOBGUI_CSS_PROPERTY_BORDER_RIGHT_COLOR,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_BORDER,
                                          color_parse,
                                          bobgui_css_color_value_new_current_color ());
  bobgui_css_style_property_register        ("border-bottom-color",
                                          BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_COLOR,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_BORDER,
                                          color_parse,
                                          bobgui_css_color_value_new_current_color ());
  bobgui_css_style_property_register        ("border-left-color",
                                          BOBGUI_CSS_PROPERTY_BORDER_LEFT_COLOR,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_BORDER,
                                          color_parse,
                                          bobgui_css_color_value_new_current_color ());
  bobgui_css_style_property_register        ("outline-color",
                                          BOBGUI_CSS_PROPERTY_OUTLINE_COLOR,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_OUTLINE,
                                          color_parse,
                                          bobgui_css_color_value_new_current_color ());

  bobgui_css_style_property_register        ("background-repeat",
                                          BOBGUI_CSS_PROPERTY_BACKGROUND_REPEAT,
                                          0,
                                          BOBGUI_CSS_AFFECTS_BACKGROUND,
                                          background_repeat_value_parse,
                                          _bobgui_css_background_repeat_value_new (BOBGUI_CSS_REPEAT_STYLE_REPEAT,
                                                                                BOBGUI_CSS_REPEAT_STYLE_REPEAT));
  bobgui_css_style_property_register        ("background-image",
                                          BOBGUI_CSS_PROPERTY_BACKGROUND_IMAGE,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_BACKGROUND,
                                          background_image_value_parse,
                                          _bobgui_css_image_value_new (NULL));

  bobgui_css_style_property_register        ("background-blend-mode",
                                          BOBGUI_CSS_PROPERTY_BACKGROUND_BLEND_MODE,
                                          0,
                                          BOBGUI_CSS_AFFECTS_BACKGROUND,
                                          blend_mode_value_parse,
                                          _bobgui_css_blend_mode_value_new (GSK_BLEND_MODE_DEFAULT));

  bobgui_css_style_property_register        ("border-image-source",
                                          BOBGUI_CSS_PROPERTY_BORDER_IMAGE_SOURCE,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_BORDER,
                                          css_image_value_parse,
                                          _bobgui_css_image_value_new (NULL));
  bobgui_css_style_property_register        ("border-image-repeat",
                                          BOBGUI_CSS_PROPERTY_BORDER_IMAGE_REPEAT,
                                          0,
                                          BOBGUI_CSS_AFFECTS_BORDER,
                                          border_image_repeat_parse,
                                          _bobgui_css_border_repeat_value_new (BOBGUI_CSS_REPEAT_STYLE_STRETCH,
                                                                            BOBGUI_CSS_REPEAT_STYLE_STRETCH));

  bobgui_css_style_property_register        ("border-image-slice",
                                          BOBGUI_CSS_PROPERTY_BORDER_IMAGE_SLICE,
                                          0,
                                          BOBGUI_CSS_AFFECTS_BORDER,
                                          border_image_slice_parse,
                                          _bobgui_css_border_value_new (bobgui_css_number_value_new (100, BOBGUI_CSS_PERCENT),
                                                                     bobgui_css_number_value_new (100, BOBGUI_CSS_PERCENT),
                                                                     bobgui_css_number_value_new (100, BOBGUI_CSS_PERCENT),
                                                                     bobgui_css_number_value_new (100, BOBGUI_CSS_PERCENT)));
  bobgui_css_style_property_register        ("border-image-width",
                                          BOBGUI_CSS_PROPERTY_BORDER_IMAGE_WIDTH,
                                          0,
                                          BOBGUI_CSS_AFFECTS_BORDER,
                                          border_image_width_parse,
                                          _bobgui_css_border_value_new (bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER),
                                                                     bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER),
                                                                     bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER),
                                                                     bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER)));

  bobgui_css_style_property_register        ("-bobgui-icon-source",
                                          BOBGUI_CSS_PROPERTY_ICON_SOURCE,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_ICON_TEXTURE,
                                          css_image_value_parse,
                                          _bobgui_css_image_value_new (NULL));
  bobgui_css_style_property_register        ("-bobgui-icon-size",
                                          BOBGUI_CSS_PROPERTY_ICON_SIZE,
                                          BOBGUI_STYLE_PROPERTY_INHERIT | BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_SIZE | BOBGUI_CSS_AFFECTS_ICON_SIZE,
                                          icon_size_parse,
                                          bobgui_css_number_value_new (16, BOBGUI_CSS_PX));
  bobgui_css_style_property_register        ("-bobgui-icon-shadow",
                                          BOBGUI_CSS_PROPERTY_ICON_SHADOW,
                                          BOBGUI_STYLE_PROPERTY_INHERIT | BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_ICON_REDRAW,
                                          shadow_value_parse,
                                          bobgui_css_shadow_value_new_none ());
  bobgui_css_style_property_register        ("-bobgui-icon-style",
                                          BOBGUI_CSS_PROPERTY_ICON_STYLE,
                                          BOBGUI_STYLE_PROPERTY_INHERIT,
                                          BOBGUI_CSS_AFFECTS_ICON_TEXTURE,
                                          icon_style_parse,
                                          _bobgui_css_icon_style_value_new (BOBGUI_CSS_ICON_STYLE_REQUESTED));
  bobgui_css_style_property_register        ("-bobgui-icon-transform",
                                          BOBGUI_CSS_PROPERTY_ICON_TRANSFORM,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_CONTENT,
                                          transform_value_parse,
                                          _bobgui_css_transform_value_new_none ());
  bobgui_css_style_property_register        ("-bobgui-icon-filter",
                                          BOBGUI_CSS_PROPERTY_ICON_FILTER,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_CONTENT,
                                          filter_value_parse,
                                          bobgui_css_filter_value_new_none ());
  bobgui_css_style_property_register        ("-bobgui-icon-weight",
                                          BOBGUI_CSS_PROPERTY_ICON_WEIGHT,
                                          BOBGUI_STYLE_PROPERTY_INHERIT | BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_ICON_REDRAW_SYMBOLIC,
                                          icon_weight_parse,
                                          bobgui_css_number_value_new (PANGO_WEIGHT_NORMAL, BOBGUI_CSS_NUMBER));
  bobgui_css_style_property_register        ("border-spacing",
                                          BOBGUI_CSS_PROPERTY_BORDER_SPACING,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_SIZE,
                                          border_spacing_value_parse,
                                          _bobgui_css_position_value_new (bobgui_css_number_value_new (0, BOBGUI_CSS_PX),
                                                                       bobgui_css_number_value_new (0, BOBGUI_CSS_PX)));

  bobgui_css_style_property_register        ("transform",
                                          BOBGUI_CSS_PROPERTY_TRANSFORM,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_TRANSFORM,
                                          transform_value_parse,
                                          _bobgui_css_transform_value_new_none ());
  bobgui_css_style_property_register        ("transform-origin",
                                          BOBGUI_CSS_PROPERTY_TRANSFORM_ORIGIN,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_TRANSFORM,
                                          transform_origin_parse,
                                          _bobgui_css_position_value_new (bobgui_css_number_value_new (50, BOBGUI_CSS_PERCENT),
                                                                       bobgui_css_number_value_new (50, BOBGUI_CSS_PERCENT)));
  bobgui_css_style_property_register        ("min-width",
                                          BOBGUI_CSS_PROPERTY_MIN_WIDTH,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_SIZE,
                                          minmax_parse,
                                          bobgui_css_number_value_new (0, BOBGUI_CSS_PX));
  bobgui_css_style_property_register        ("min-height",
                                          BOBGUI_CSS_PROPERTY_MIN_HEIGHT,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_SIZE,
                                          minmax_parse,
                                          bobgui_css_number_value_new (0, BOBGUI_CSS_PX));

  bobgui_css_style_property_register        ("transition-property",
                                          BOBGUI_CSS_PROPERTY_TRANSITION_PROPERTY,
                                          0,
                                          0,
                                          transition_property_parse,
                                          _bobgui_css_ident_value_new ("all"));
  bobgui_css_style_property_register        ("transition-duration",
                                          BOBGUI_CSS_PROPERTY_TRANSITION_DURATION,
                                          0,
                                          0,
                                          transition_time_parse,
                                          bobgui_css_number_value_new (0, BOBGUI_CSS_S));
  bobgui_css_style_property_register        ("transition-timing-function",
                                          BOBGUI_CSS_PROPERTY_TRANSITION_TIMING_FUNCTION,
                                          0,
                                          0,
                                          transition_timing_function_parse,
                                          _bobgui_css_ease_value_new_cubic_bezier (0.25, 0.1, 0.25, 1.0));
  bobgui_css_style_property_register        ("transition-delay",
                                          BOBGUI_CSS_PROPERTY_TRANSITION_DELAY,
                                          0,
                                          0,
                                          transition_time_parse,
                                          bobgui_css_number_value_new (0, BOBGUI_CSS_S));

  bobgui_css_style_property_register        ("animation-name",
                                          BOBGUI_CSS_PROPERTY_ANIMATION_NAME,
                                          0,
                                          0,
                                          transition_property_parse,
                                          _bobgui_css_ident_value_new ("none"));
  bobgui_css_style_property_register        ("animation-duration",
                                          BOBGUI_CSS_PROPERTY_ANIMATION_DURATION,
                                          0,
                                          0,
                                          transition_time_parse,
                                          bobgui_css_number_value_new (0, BOBGUI_CSS_S));
  bobgui_css_style_property_register        ("animation-timing-function",
                                          BOBGUI_CSS_PROPERTY_ANIMATION_TIMING_FUNCTION,
                                          0,
                                          0,
                                          transition_timing_function_parse,
                                          _bobgui_css_ease_value_new_cubic_bezier (0.25, 0.1, 0.25, 1.0));
  bobgui_css_style_property_register        ("animation-iteration-count",
                                          BOBGUI_CSS_PROPERTY_ANIMATION_ITERATION_COUNT,
                                          0,
                                          0,
                                          iteration_count_parse,
                                          bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER));
  bobgui_css_style_property_register        ("animation-direction",
                                          BOBGUI_CSS_PROPERTY_ANIMATION_DIRECTION,
                                          0,
                                          0,
                                          parse_css_direction,
                                          _bobgui_css_direction_value_new (BOBGUI_CSS_DIRECTION_NORMAL));
  bobgui_css_style_property_register        ("animation-play-state",
                                          BOBGUI_CSS_PROPERTY_ANIMATION_PLAY_STATE,
                                          0,
                                          0,
                                          parse_css_play_state,
                                          _bobgui_css_play_state_value_new (BOBGUI_CSS_PLAY_STATE_RUNNING));
  bobgui_css_style_property_register        ("animation-delay",
                                          BOBGUI_CSS_PROPERTY_ANIMATION_DELAY,
                                          0,
                                          0,
                                          transition_time_parse,
                                          bobgui_css_number_value_new (0, BOBGUI_CSS_S));
  bobgui_css_style_property_register        ("animation-fill-mode",
                                          BOBGUI_CSS_PROPERTY_ANIMATION_FILL_MODE,
                                          0,
                                          0,
                                          parse_css_fill_mode,
                                          _bobgui_css_fill_mode_value_new (BOBGUI_CSS_FILL_NONE));

  bobgui_css_style_property_register        ("opacity",
                                          BOBGUI_CSS_PROPERTY_OPACITY,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_POSTEFFECT,
                                          opacity_parse,
                                          bobgui_css_number_value_new (1, BOBGUI_CSS_NUMBER));
  bobgui_css_style_property_register        ("backdrop-filter",
                                          BOBGUI_CSS_PROPERTY_BACKDROP_FILTER,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_POSTEFFECT,
                                          filter_value_parse,
                                          bobgui_css_filter_value_new_none ());
  bobgui_css_style_property_register        ("filter",
                                          BOBGUI_CSS_PROPERTY_FILTER,
                                          BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_POSTEFFECT,
                                          filter_value_parse,
                                          bobgui_css_filter_value_new_none ());

  bobgui_css_style_property_register        ("caret-color",
                                          BOBGUI_CSS_PROPERTY_CARET_COLOR,
                                          BOBGUI_STYLE_PROPERTY_INHERIT | BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_CONTENT,
                                          color_parse,
                                          bobgui_css_color_value_new_current_color ());
  bobgui_css_style_property_register        ("-bobgui-secondary-caret-color",
                                          BOBGUI_CSS_PROPERTY_SECONDARY_CARET_COLOR,
                                          BOBGUI_STYLE_PROPERTY_INHERIT | BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_CONTENT,
                                          color_parse,
                                          bobgui_css_color_value_new_current_color ());
  bobgui_css_style_property_register        ("font-feature-settings",
                                          BOBGUI_CSS_PROPERTY_FONT_FEATURE_SETTINGS,
                                          BOBGUI_STYLE_PROPERTY_INHERIT | BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_TEXT_ATTRS | BOBGUI_CSS_AFFECTS_TEXT_SIZE,
                                          parse_font_feature_settings,
                                          bobgui_css_font_features_value_new_default ());
  bobgui_css_style_property_register        ("font-variation-settings",
                                          BOBGUI_CSS_PROPERTY_FONT_VARIATION_SETTINGS,
                                          BOBGUI_STYLE_PROPERTY_INHERIT | BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_TEXT_ATTRS | BOBGUI_CSS_AFFECTS_TEXT_SIZE,
                                          parse_font_variation_settings,
                                          bobgui_css_font_variations_value_new_default ());
  bobgui_css_style_property_register        ("line-height",
                                          BOBGUI_CSS_PROPERTY_LINE_HEIGHT,
                                          BOBGUI_STYLE_PROPERTY_INHERIT | BOBGUI_STYLE_PROPERTY_ANIMATED,
                                          BOBGUI_CSS_AFFECTS_TEXT_ATTRS | BOBGUI_CSS_AFFECTS_TEXT_SIZE,
                                          parse_line_height,
                                          bobgui_css_value_ref (bobgui_css_line_height_value_get_default ()));
}
