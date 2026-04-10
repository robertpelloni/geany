/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2011 Red Hat, Inc.
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

#include "bobguicssenumvalueprivate.h"

#include "bobguicssstyleprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguistyleproviderprivate.h"
#include "bobguisettingsprivate.h"

#include "bobguipopcountprivate.h"

/* repeated API */

struct _BobguiCssValue {
  BOBGUI_CSS_VALUE_BASE
  int value;
  const char *name;
};

static void
bobgui_css_value_enum_free (BobguiCssValue *value)
{
  g_free (value);
}

static BobguiCssValue *
bobgui_css_value_enum_compute (BobguiCssValue          *value,
                            guint                 property_id,
                            BobguiCssComputeContext *context)
{
  return bobgui_css_value_ref (value);
}

static gboolean
bobgui_css_value_enum_equal (const BobguiCssValue *enum1,
                          const BobguiCssValue *enum2)
{
  return enum1 == enum2;
}

static BobguiCssValue *
bobgui_css_value_enum_transition (BobguiCssValue *start,
                               BobguiCssValue *end,
                               guint        property_id,
                               double       progress)
{
  return NULL;
}

static void
bobgui_css_value_enum_print (const BobguiCssValue *value,
                          GString           *string)
{
  g_string_append (string, value->name);
}

/* {{{ BobguiBorderStyle */

static const BobguiCssValueClass BOBGUI_CSS_VALUE_BORDER_STYLE = {
  "BobguiCssBorderStyleValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_enum_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_value_enum_print
};

static BobguiCssValue border_style_values[] = {
  { &BOBGUI_CSS_VALUE_BORDER_STYLE, 1, 1, 0, 0, BOBGUI_BORDER_STYLE_NONE, "none" },
  { &BOBGUI_CSS_VALUE_BORDER_STYLE, 1, 1, 0, 0, BOBGUI_BORDER_STYLE_SOLID, "solid" },
  { &BOBGUI_CSS_VALUE_BORDER_STYLE, 1, 1, 0, 0, BOBGUI_BORDER_STYLE_INSET, "inset" },
  { &BOBGUI_CSS_VALUE_BORDER_STYLE, 1, 1, 0, 0, BOBGUI_BORDER_STYLE_OUTSET, "outset" },
  { &BOBGUI_CSS_VALUE_BORDER_STYLE, 1, 1, 0, 0, BOBGUI_BORDER_STYLE_HIDDEN, "hidden" },
  { &BOBGUI_CSS_VALUE_BORDER_STYLE, 1, 1, 0, 0, BOBGUI_BORDER_STYLE_DOTTED, "dotted" },
  { &BOBGUI_CSS_VALUE_BORDER_STYLE, 1, 1, 0, 0, BOBGUI_BORDER_STYLE_DASHED, "dashed" },
  { &BOBGUI_CSS_VALUE_BORDER_STYLE, 1, 1, 0, 0, BOBGUI_BORDER_STYLE_DOUBLE, "double" },
  { &BOBGUI_CSS_VALUE_BORDER_STYLE, 1, 1, 0, 0, BOBGUI_BORDER_STYLE_GROOVE, "groove" },
  { &BOBGUI_CSS_VALUE_BORDER_STYLE, 1, 1, 0, 0, BOBGUI_BORDER_STYLE_RIDGE, "ridge" }
};

BobguiCssValue *
_bobgui_css_border_style_value_new (BobguiBorderStyle border_style)
{
  g_return_val_if_fail (border_style < G_N_ELEMENTS (border_style_values), NULL);

  return bobgui_css_value_ref (&border_style_values[border_style]);
}

BobguiCssValue *
_bobgui_css_border_style_value_try_parse (BobguiCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (border_style_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, border_style_values[i].name))
        return bobgui_css_value_ref (&border_style_values[i]);
    }

  return NULL;
}

BobguiBorderStyle
_bobgui_css_border_style_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_BORDER_STYLE, BOBGUI_BORDER_STYLE_NONE);

  return value->value;
}

/* }}} */
/* {{{ BobguiCssBlendMode */

static const BobguiCssValueClass BOBGUI_CSS_VALUE_BLEND_MODE = {
  "BobguiCssBlendModeValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_enum_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_value_enum_print
};

static BobguiCssValue blend_mode_values[] = {
  { &BOBGUI_CSS_VALUE_BLEND_MODE, 1, 1, 0, 0, GSK_BLEND_MODE_DEFAULT, "normal" },
  { &BOBGUI_CSS_VALUE_BLEND_MODE, 1, 1, 0, 0, GSK_BLEND_MODE_MULTIPLY, "multiply" },
  { &BOBGUI_CSS_VALUE_BLEND_MODE, 1, 1, 0, 0, GSK_BLEND_MODE_SCREEN, "screen" },
  { &BOBGUI_CSS_VALUE_BLEND_MODE, 1, 1, 0, 0, GSK_BLEND_MODE_OVERLAY, "overlay" },
  { &BOBGUI_CSS_VALUE_BLEND_MODE, 1, 1, 0, 0, GSK_BLEND_MODE_DARKEN, "darken" },
  { &BOBGUI_CSS_VALUE_BLEND_MODE, 1, 1, 0, 0, GSK_BLEND_MODE_LIGHTEN, "lighten" },
  { &BOBGUI_CSS_VALUE_BLEND_MODE, 1, 1, 0, 0, GSK_BLEND_MODE_COLOR_DODGE, "color-dodge" },
  { &BOBGUI_CSS_VALUE_BLEND_MODE, 1, 1, 0, 0, GSK_BLEND_MODE_COLOR_BURN, "color-burn" },
  { &BOBGUI_CSS_VALUE_BLEND_MODE, 1, 1, 0, 0, GSK_BLEND_MODE_HARD_LIGHT, "hard-light" },
  { &BOBGUI_CSS_VALUE_BLEND_MODE, 1, 1, 0, 0, GSK_BLEND_MODE_SOFT_LIGHT, "soft-light" },
  { &BOBGUI_CSS_VALUE_BLEND_MODE, 1, 1, 0, 0, GSK_BLEND_MODE_DIFFERENCE, "difference" },
  { &BOBGUI_CSS_VALUE_BLEND_MODE, 1, 1, 0, 0, GSK_BLEND_MODE_EXCLUSION, "exclusion" },
  { &BOBGUI_CSS_VALUE_BLEND_MODE, 1, 1, 0, 0, GSK_BLEND_MODE_COLOR, "color" },
  { &BOBGUI_CSS_VALUE_BLEND_MODE, 1, 1, 0, 0, GSK_BLEND_MODE_HUE, "hue" },
  { &BOBGUI_CSS_VALUE_BLEND_MODE, 1, 1, 0, 0, GSK_BLEND_MODE_SATURATION, "saturation" },
  { &BOBGUI_CSS_VALUE_BLEND_MODE, 1, 1, 0, 0, GSK_BLEND_MODE_LUMINOSITY, "luminosity" }
};

BobguiCssValue *
_bobgui_css_blend_mode_value_new (GskBlendMode blend_mode)
{
  g_return_val_if_fail (blend_mode < G_N_ELEMENTS (blend_mode_values), NULL);

  return bobgui_css_value_ref (&blend_mode_values[blend_mode]);
}

BobguiCssValue *
_bobgui_css_blend_mode_value_try_parse (BobguiCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (blend_mode_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, blend_mode_values[i].name))
        return bobgui_css_value_ref (&blend_mode_values[i]);
    }

  return NULL;
}

GskBlendMode
_bobgui_css_blend_mode_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_BLEND_MODE, GSK_BLEND_MODE_DEFAULT);

  return value->value;
}

/* }}} */
/* {{{ BobguiCssFontSize */

static double
get_dpi (BobguiCssStyle *style)
{
  return bobgui_css_number_value_get (style->core->dpi, 96);
}

/* XXX: Kinda bad to have that machinery here, nobody expects vital font
 * size code to appear in bobguicssvalueenum.c.
 */
#define DEFAULT_FONT_SIZE_PT 10

double
bobgui_css_font_size_get_default_px (BobguiStyleProvider *provider,
                                  BobguiCssStyle      *style)
{
  BobguiSettings *settings;
  int font_size;

  settings = bobgui_style_provider_get_settings (provider);
  if (settings == NULL)
    return DEFAULT_FONT_SIZE_PT * get_dpi (style) / 72.0;

  font_size = bobgui_settings_get_font_size (settings);
  if (font_size == 0)
    return DEFAULT_FONT_SIZE_PT * get_dpi (style) / 72.0;
  else if (bobgui_settings_get_font_size_is_absolute (settings))
    return (double) font_size / PANGO_SCALE;
  else
    return ((double) font_size / PANGO_SCALE) * get_dpi (style) / 72.0;
}

static BobguiCssValue *
bobgui_css_value_font_size_compute (BobguiCssValue          *value,
                                 guint                 property_id,
                                 BobguiCssComputeContext *context)
{
  BobguiStyleProvider *provider = context->provider;
  BobguiCssStyle *style = context->style;
  BobguiCssStyle *parent_style = context->parent_style;

  double font_size;

  switch (value->value)
    {
    case BOBGUI_CSS_FONT_SIZE_XX_SMALL:
      font_size = bobgui_css_font_size_get_default_px (provider, style) * 3. / 5;
      break;
    case BOBGUI_CSS_FONT_SIZE_X_SMALL:
      font_size = bobgui_css_font_size_get_default_px (provider, style) * 3. / 4;
      break;
    case BOBGUI_CSS_FONT_SIZE_SMALL:
      font_size = bobgui_css_font_size_get_default_px (provider, style) * 8. / 9;
      break;
    default:
      g_assert_not_reached ();
      /* fall thru */
    case BOBGUI_CSS_FONT_SIZE_MEDIUM:
      font_size = bobgui_css_font_size_get_default_px (provider, style);
      break;
    case BOBGUI_CSS_FONT_SIZE_LARGE:
      font_size = bobgui_css_font_size_get_default_px (provider, style) * 6. / 5;
      break;
    case BOBGUI_CSS_FONT_SIZE_X_LARGE:
      font_size = bobgui_css_font_size_get_default_px (provider, style) * 3. / 2;
      break;
    case BOBGUI_CSS_FONT_SIZE_XX_LARGE:
      font_size = bobgui_css_font_size_get_default_px (provider, style) * 2;
      break;
    case BOBGUI_CSS_FONT_SIZE_SMALLER:
      if (parent_style)
        font_size = bobgui_css_number_value_get (parent_style->core->font_size, 100);
      else
        font_size = bobgui_css_font_size_get_default_px (provider, style);
      /* This is what WebKit does... */
      font_size /= 1.2;
      break;
    case BOBGUI_CSS_FONT_SIZE_LARGER:
      if (parent_style)
        font_size = bobgui_css_number_value_get (parent_style->core->font_size, 100);
      else
        font_size = bobgui_css_font_size_get_default_px (provider, style);
      /* This is what WebKit does... */
      font_size *= 1.2;
      break;
  }

  return bobgui_css_number_value_new (font_size, BOBGUI_CSS_PX);
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_FONT_SIZE = {
  "BobguiCssFontSizeValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_font_size_compute,
  NULL,
  bobgui_css_value_enum_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_value_enum_print
};

static BobguiCssValue font_size_values[] = {
  { &BOBGUI_CSS_VALUE_FONT_SIZE, 1, 0, 0, 0, BOBGUI_CSS_FONT_SIZE_SMALLER, "smaller" },
  { &BOBGUI_CSS_VALUE_FONT_SIZE, 1, 0, 0, 0, BOBGUI_CSS_FONT_SIZE_LARGER, "larger" },
  { &BOBGUI_CSS_VALUE_FONT_SIZE, 1, 0, 0, 0, BOBGUI_CSS_FONT_SIZE_XX_SMALL, "xx-small" },
  { &BOBGUI_CSS_VALUE_FONT_SIZE, 1, 0, 0, 0, BOBGUI_CSS_FONT_SIZE_X_SMALL, "x-small" },
  { &BOBGUI_CSS_VALUE_FONT_SIZE, 1, 0, 0, 0, BOBGUI_CSS_FONT_SIZE_SMALL, "small" },
  { &BOBGUI_CSS_VALUE_FONT_SIZE, 1, 0, 0, 0, BOBGUI_CSS_FONT_SIZE_MEDIUM, "medium" },
  { &BOBGUI_CSS_VALUE_FONT_SIZE, 1, 0, 0, 0, BOBGUI_CSS_FONT_SIZE_LARGE, "large" },
  { &BOBGUI_CSS_VALUE_FONT_SIZE, 1, 0, 0, 0, BOBGUI_CSS_FONT_SIZE_X_LARGE, "x-large" },
  { &BOBGUI_CSS_VALUE_FONT_SIZE, 1, 0, 0, 0, BOBGUI_CSS_FONT_SIZE_XX_LARGE, "xx-large" }
};

BobguiCssValue *
_bobgui_css_font_size_value_new (BobguiCssFontSize font_size)
{
  g_return_val_if_fail (font_size < G_N_ELEMENTS (font_size_values), NULL);

  return bobgui_css_value_ref (&font_size_values[font_size]);
}

BobguiCssValue *
_bobgui_css_font_size_value_try_parse (BobguiCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (font_size_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, font_size_values[i].name))
        return bobgui_css_value_ref (&font_size_values[i]);
    }

  return NULL;
}

BobguiCssFontSize
_bobgui_css_font_size_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_FONT_SIZE, BOBGUI_CSS_FONT_SIZE_MEDIUM);

  return value->value;
}

/* }}} */
/* {{{ PangoStyle */

static const BobguiCssValueClass BOBGUI_CSS_VALUE_FONT_STYLE = {
  "BobguiCssFontStyleValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_enum_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_value_enum_print
};

static BobguiCssValue font_style_values[] = {
  { &BOBGUI_CSS_VALUE_FONT_STYLE, 1, 1, 0, 0, PANGO_STYLE_NORMAL, "normal" },
  { &BOBGUI_CSS_VALUE_FONT_STYLE, 1, 1, 0, 0, PANGO_STYLE_OBLIQUE, "oblique" },
  { &BOBGUI_CSS_VALUE_FONT_STYLE, 1, 1, 0, 0, PANGO_STYLE_ITALIC, "italic" }
};

BobguiCssValue *
_bobgui_css_font_style_value_new (PangoStyle font_style)
{
  g_return_val_if_fail (font_style < G_N_ELEMENTS (font_style_values), NULL);

  return bobgui_css_value_ref (&font_style_values[font_style]);
}

BobguiCssValue *
_bobgui_css_font_style_value_try_parse (BobguiCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (font_style_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, font_style_values[i].name))
        return bobgui_css_value_ref (&font_style_values[i]);
    }

  return NULL;
}

PangoStyle
_bobgui_css_font_style_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_FONT_STYLE, PANGO_STYLE_NORMAL);

  return value->value;
}

/* }}} */
/* {{{ PangoWeight */

#define BOLDER -1
#define LIGHTER -2

static BobguiCssValue *
bobgui_css_value_font_weight_compute (BobguiCssValue          *value,
                                   guint                 property_id,
                                   BobguiCssComputeContext *context)
{
  PangoWeight new_weight;
  int parent_value;

  if (value->value >= 0)
    return bobgui_css_value_ref (value);

  if (context->parent_style)
    parent_value = bobgui_css_number_value_get (context->parent_style->font->font_weight, 100);
  else
    parent_value = 400;

  if (value->value == BOLDER)
    {
      if (parent_value < 350)
        new_weight = 400;
      else if (parent_value < 550)
        new_weight = 700;
      else
        new_weight = 900;
    }
  else if (value->value == LIGHTER)
    {
      if (parent_value > 750)
        new_weight = 700;
      else if (parent_value > 550)
        new_weight = 400;
      else
        new_weight = 100;
    }
  else
    {
      g_assert_not_reached ();
      new_weight = PANGO_WEIGHT_NORMAL;
    }

  return bobgui_css_number_value_new (new_weight, BOBGUI_CSS_NUMBER);
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_FONT_WEIGHT = {
  "BobguiCssFontWeightValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_font_weight_compute,
  NULL,
  bobgui_css_value_enum_equal,
  NULL,
  NULL,
  NULL,
  bobgui_css_value_enum_print
};

static BobguiCssValue font_weight_values[] = {
  { &BOBGUI_CSS_VALUE_FONT_WEIGHT, 1, 0, 0, 0, BOLDER, "bolder" },
  { &BOBGUI_CSS_VALUE_FONT_WEIGHT, 1, 0, 0, 0, LIGHTER, "lighter" },
};

BobguiCssValue *
bobgui_css_font_weight_value_try_parse (BobguiCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (font_weight_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, font_weight_values[i].name))
        return bobgui_css_value_ref (&font_weight_values[i]);
    }

  if (bobgui_css_parser_try_ident (parser, "normal"))
    return bobgui_css_number_value_new (PANGO_WEIGHT_NORMAL, BOBGUI_CSS_NUMBER);
  if (bobgui_css_parser_try_ident (parser, "bold"))
    return bobgui_css_number_value_new (PANGO_WEIGHT_BOLD, BOBGUI_CSS_NUMBER);

  return NULL;
}

PangoWeight
bobgui_css_font_weight_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_FONT_WEIGHT, PANGO_WEIGHT_NORMAL);

  return value->value;
}

#undef BOLDER
#undef LIGHTER

/* }}} */
/* {{{ PangoStretch */

static const BobguiCssValueClass BOBGUI_CSS_VALUE_FONT_STRETCH = {
  "BobguiCssFontStretchValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_enum_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_value_enum_print
};

static BobguiCssValue font_stretch_values[] = {
  { &BOBGUI_CSS_VALUE_FONT_STRETCH, 1, 1, 0, 0, PANGO_STRETCH_ULTRA_CONDENSED, "ultra-condensed" },
  { &BOBGUI_CSS_VALUE_FONT_STRETCH, 1, 1, 0, 0, PANGO_STRETCH_EXTRA_CONDENSED, "extra-condensed" },
  { &BOBGUI_CSS_VALUE_FONT_STRETCH, 1, 1, 0, 0, PANGO_STRETCH_CONDENSED, "condensed" },
  { &BOBGUI_CSS_VALUE_FONT_STRETCH, 1, 1, 0, 0, PANGO_STRETCH_SEMI_CONDENSED, "semi-condensed" },
  { &BOBGUI_CSS_VALUE_FONT_STRETCH, 1, 1, 0, 0, PANGO_STRETCH_NORMAL, "normal" },
  { &BOBGUI_CSS_VALUE_FONT_STRETCH, 1, 1, 0, 0, PANGO_STRETCH_SEMI_EXPANDED, "semi-expanded" },
  { &BOBGUI_CSS_VALUE_FONT_STRETCH, 1, 1, 0, 0, PANGO_STRETCH_EXPANDED, "expanded" },
  { &BOBGUI_CSS_VALUE_FONT_STRETCH, 1, 1, 0, 0, PANGO_STRETCH_EXTRA_EXPANDED, "extra-expanded" },
  { &BOBGUI_CSS_VALUE_FONT_STRETCH, 1, 1, 0, 0, PANGO_STRETCH_ULTRA_EXPANDED, "ultra-expanded" },
};

BobguiCssValue *
_bobgui_css_font_stretch_value_new (PangoStretch font_stretch)
{
  g_return_val_if_fail (font_stretch < G_N_ELEMENTS (font_stretch_values), NULL);

  return bobgui_css_value_ref (&font_stretch_values[font_stretch]);
}

BobguiCssValue *
_bobgui_css_font_stretch_value_try_parse (BobguiCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (font_stretch_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, font_stretch_values[i].name))
        return bobgui_css_value_ref (&font_stretch_values[i]);
    }

  return NULL;
}

PangoStretch
_bobgui_css_font_stretch_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_FONT_STRETCH, PANGO_STRETCH_NORMAL);

  return value->value;
}

/* }}} */
/* {{{ BobguiTextDecorationStyle */

static const BobguiCssValueClass BOBGUI_CSS_VALUE_TEXT_DECORATION_STYLE = {
  "BobguiCssTextDecorationStyleValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_enum_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_value_enum_print
};

static BobguiCssValue text_decoration_style_values[] = {
  { &BOBGUI_CSS_VALUE_TEXT_DECORATION_STYLE, 1, 1, 0, 0, BOBGUI_CSS_TEXT_DECORATION_STYLE_SOLID, "solid" },
  { &BOBGUI_CSS_VALUE_TEXT_DECORATION_STYLE, 1, 1, 0, 0, BOBGUI_CSS_TEXT_DECORATION_STYLE_DOUBLE, "double" },
  { &BOBGUI_CSS_VALUE_TEXT_DECORATION_STYLE, 1, 1, 0, 0, BOBGUI_CSS_TEXT_DECORATION_STYLE_WAVY, "wavy" },
};

BobguiCssValue *
_bobgui_css_text_decoration_style_value_new (BobguiTextDecorationStyle style)
{
  g_return_val_if_fail (style < G_N_ELEMENTS (text_decoration_style_values), NULL);

  return bobgui_css_value_ref (&text_decoration_style_values[style]);
}

BobguiCssValue *
_bobgui_css_text_decoration_style_value_try_parse (BobguiCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (text_decoration_style_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, text_decoration_style_values[i].name))
        return bobgui_css_value_ref (&text_decoration_style_values[i]);
    }

  return NULL;
}

BobguiTextDecorationStyle
_bobgui_css_text_decoration_style_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_TEXT_DECORATION_STYLE, BOBGUI_CSS_TEXT_DECORATION_STYLE_SOLID);

  return value->value;
}

/* }}} */
/* {{{ BobguiCssArea */

static const BobguiCssValueClass BOBGUI_CSS_VALUE_AREA = {
  "BobguiCssAreaValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_enum_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_value_enum_print
};

static BobguiCssValue area_values[] = {
  { &BOBGUI_CSS_VALUE_AREA, 1, 1, 0, 0, BOBGUI_CSS_AREA_BORDER_BOX, "border-box" },
  { &BOBGUI_CSS_VALUE_AREA, 1, 1, 0, 0, BOBGUI_CSS_AREA_PADDING_BOX, "padding-box" },
  { &BOBGUI_CSS_VALUE_AREA, 1, 1, 0, 0, BOBGUI_CSS_AREA_CONTENT_BOX, "content-box" }
};

BobguiCssValue *
_bobgui_css_area_value_new (BobguiCssArea area)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (area_values); i++)
    {
      if (area_values[i].value == area)
        return bobgui_css_value_ref (&area_values[i]);
    }

  g_return_val_if_reached (NULL);
}

BobguiCssValue *
_bobgui_css_area_value_try_parse (BobguiCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (area_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, area_values[i].name))
        return bobgui_css_value_ref (&area_values[i]);
    }

  return NULL;
}

BobguiCssArea
_bobgui_css_area_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_AREA, BOBGUI_CSS_AREA_BORDER_BOX);

  return value->value;
}

/* }}} */
/* {{{ BobguiCssDirection */

static const BobguiCssValueClass BOBGUI_CSS_VALUE_DIRECTION = {
  "BobguiCssDirectionValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_enum_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_value_enum_print
};

static BobguiCssValue direction_values[] = {
  { &BOBGUI_CSS_VALUE_DIRECTION, 1, 1, 0, 0, BOBGUI_CSS_DIRECTION_NORMAL, "normal" },
  { &BOBGUI_CSS_VALUE_DIRECTION, 1, 1, 0, 0, BOBGUI_CSS_DIRECTION_REVERSE, "reverse" },
  { &BOBGUI_CSS_VALUE_DIRECTION, 1, 1, 0, 0, BOBGUI_CSS_DIRECTION_ALTERNATE, "alternate" },
  { &BOBGUI_CSS_VALUE_DIRECTION, 1, 1, 0, 0, BOBGUI_CSS_DIRECTION_ALTERNATE_REVERSE, "alternate-reverse" }
};

BobguiCssValue *
_bobgui_css_direction_value_new (BobguiCssDirection direction)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (direction_values); i++)
    {
      if (direction_values[i].value == direction)
        return bobgui_css_value_ref (&direction_values[i]);
    }

  g_return_val_if_reached (NULL);
}

BobguiCssValue *
_bobgui_css_direction_value_try_parse (BobguiCssParser *parser)
{
  int i;

  g_return_val_if_fail (parser != NULL, NULL);

  /* need to parse backwards here, otherwise "alternate" will also match "alternate-reverse".
   * Our parser rocks!
   */
  for (i = G_N_ELEMENTS (direction_values) - 1; i >= 0; i--)
    {
      if (bobgui_css_parser_try_ident (parser, direction_values[i].name))
        return bobgui_css_value_ref (&direction_values[i]);
    }

  return NULL;
}

BobguiCssDirection
_bobgui_css_direction_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_DIRECTION, BOBGUI_CSS_DIRECTION_NORMAL);

  return value->value;
}

/* }}} */
/* {{{ BobguiCssPlayState */

static const BobguiCssValueClass BOBGUI_CSS_VALUE_PLAY_STATE = {
  "BobguiCssPlayStateValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_enum_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_value_enum_print
};

static BobguiCssValue play_state_values[] = {
  { &BOBGUI_CSS_VALUE_PLAY_STATE, 1, 1, 0, 0, BOBGUI_CSS_PLAY_STATE_RUNNING, "running" },
  { &BOBGUI_CSS_VALUE_PLAY_STATE, 1, 1, 0, 0, BOBGUI_CSS_PLAY_STATE_PAUSED, "paused" }
};

BobguiCssValue *
_bobgui_css_play_state_value_new (BobguiCssPlayState play_state)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (play_state_values); i++)
    {
      if (play_state_values[i].value == play_state)
        return bobgui_css_value_ref (&play_state_values[i]);
    }

  g_return_val_if_reached (NULL);
}

BobguiCssValue *
_bobgui_css_play_state_value_try_parse (BobguiCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (play_state_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, play_state_values[i].name))
        return bobgui_css_value_ref (&play_state_values[i]);
    }

  return NULL;
}

BobguiCssPlayState
_bobgui_css_play_state_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_PLAY_STATE, BOBGUI_CSS_PLAY_STATE_RUNNING);

  return value->value;
}

/* }}} */
/* {{{ BobguiCssFillMode */

static const BobguiCssValueClass BOBGUI_CSS_VALUE_FILL_MODE = {
  "BobguiCssFillModeValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_enum_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_value_enum_print
};

static BobguiCssValue fill_mode_values[] = {
  { &BOBGUI_CSS_VALUE_FILL_MODE, 1, 1, 0, 0, BOBGUI_CSS_FILL_NONE, "none" },
  { &BOBGUI_CSS_VALUE_FILL_MODE, 1, 1, 0, 0, BOBGUI_CSS_FILL_FORWARDS, "forwards" },
  { &BOBGUI_CSS_VALUE_FILL_MODE, 1, 1, 0, 0, BOBGUI_CSS_FILL_BACKWARDS, "backwards" },
  { &BOBGUI_CSS_VALUE_FILL_MODE, 1, 1, 0, 0, BOBGUI_CSS_FILL_BOTH, "both" }
};

BobguiCssValue *
_bobgui_css_fill_mode_value_new (BobguiCssFillMode fill_mode)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (fill_mode_values); i++)
    {
      if (fill_mode_values[i].value == fill_mode)
        return bobgui_css_value_ref (&fill_mode_values[i]);
    }

  g_return_val_if_reached (NULL);
}

BobguiCssValue *
_bobgui_css_fill_mode_value_try_parse (BobguiCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (fill_mode_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, fill_mode_values[i].name))
        return bobgui_css_value_ref (&fill_mode_values[i]);
    }

  return NULL;
}

BobguiCssFillMode
_bobgui_css_fill_mode_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_FILL_MODE, BOBGUI_CSS_FILL_NONE);

  return value->value;
}

/* }}} */
/* {{{ BobguiCssIconStyle */

static const BobguiCssValueClass BOBGUI_CSS_VALUE_ICON_STYLE = {
  "BobguiCssIconStyleValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_enum_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_value_enum_print
};

static BobguiCssValue icon_style_values[] = {
  { &BOBGUI_CSS_VALUE_ICON_STYLE, 1, 1, 0, 0, BOBGUI_CSS_ICON_STYLE_REQUESTED, "requested" },
  { &BOBGUI_CSS_VALUE_ICON_STYLE, 1, 1, 0, 0, BOBGUI_CSS_ICON_STYLE_REGULAR, "regular" },
  { &BOBGUI_CSS_VALUE_ICON_STYLE, 1, 1, 0, 0, BOBGUI_CSS_ICON_STYLE_SYMBOLIC, "symbolic" }
};

BobguiCssValue *
_bobgui_css_icon_style_value_new (BobguiCssIconStyle icon_style)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (icon_style_values); i++)
    {
      if (icon_style_values[i].value == icon_style)
        return bobgui_css_value_ref (&icon_style_values[i]);
    }

  g_return_val_if_reached (NULL);
}

BobguiCssValue *
_bobgui_css_icon_style_value_try_parse (BobguiCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (icon_style_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, icon_style_values[i].name))
        return bobgui_css_value_ref (&icon_style_values[i]);
    }

  return NULL;
}

BobguiCssIconStyle
_bobgui_css_icon_style_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_ICON_STYLE, BOBGUI_CSS_ICON_STYLE_REQUESTED);

  return value->value;
}

/* }}} */
/* {{{ BobguiCssFontKerning */

static const BobguiCssValueClass BOBGUI_CSS_VALUE_FONT_KERNING = {
  "BobguiCssFontKerningValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_enum_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_value_enum_print
};

static BobguiCssValue font_kerning_values[] = {
  { &BOBGUI_CSS_VALUE_FONT_KERNING, 1, 1, 0, 0, BOBGUI_CSS_FONT_KERNING_AUTO, "auto" },
  { &BOBGUI_CSS_VALUE_FONT_KERNING, 1, 1, 0, 0, BOBGUI_CSS_FONT_KERNING_NORMAL, "normal" },
  { &BOBGUI_CSS_VALUE_FONT_KERNING, 1, 1, 0, 0, BOBGUI_CSS_FONT_KERNING_NONE, "none" }
};

BobguiCssValue *
_bobgui_css_font_kerning_value_new (BobguiCssFontKerning kerning)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (font_kerning_values); i++)
    {
      if (font_kerning_values[i].value == kerning)
        return bobgui_css_value_ref (&font_kerning_values[i]);
    }

  g_return_val_if_reached (NULL);
}

BobguiCssValue *
_bobgui_css_font_kerning_value_try_parse (BobguiCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (font_kerning_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, font_kerning_values[i].name))
        return bobgui_css_value_ref (&font_kerning_values[i]);
    }

  return NULL;
}

BobguiCssFontKerning
_bobgui_css_font_kerning_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_FONT_KERNING, BOBGUI_CSS_FONT_KERNING_AUTO);

  return value->value;
}

/* }}} */
/* {{{ BobguiCssFontVariantPos */

static const BobguiCssValueClass BOBGUI_CSS_VALUE_FONT_VARIANT_POSITION = {
  "BobguiCssFontVariationPositionValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_enum_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_value_enum_print
};

static BobguiCssValue font_variant_position_values[] = {
  { &BOBGUI_CSS_VALUE_FONT_VARIANT_POSITION, 1, 1, 0, 0, BOBGUI_CSS_FONT_VARIANT_POSITION_NORMAL, "normal" },
  { &BOBGUI_CSS_VALUE_FONT_VARIANT_POSITION, 1, 1, 0, 0, BOBGUI_CSS_FONT_VARIANT_POSITION_SUB, "sub" },
  { &BOBGUI_CSS_VALUE_FONT_VARIANT_POSITION, 1, 1, 0, 0, BOBGUI_CSS_FONT_VARIANT_POSITION_SUPER, "super" }
};

BobguiCssValue *
_bobgui_css_font_variant_position_value_new (BobguiCssFontVariantPosition position)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (font_variant_position_values); i++)
    {
      if (font_variant_position_values[i].value == position)
        return bobgui_css_value_ref (&font_variant_position_values[i]);
    }

  g_return_val_if_reached (NULL);
}

BobguiCssValue *
_bobgui_css_font_variant_position_value_try_parse (BobguiCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (font_variant_position_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, font_variant_position_values[i].name))
        return bobgui_css_value_ref (&font_variant_position_values[i]);
    }

  return NULL;
}

BobguiCssFontVariantPosition
_bobgui_css_font_variant_position_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_FONT_VARIANT_POSITION, BOBGUI_CSS_FONT_VARIANT_POSITION_NORMAL);

  return value->value;
}

/* }}} */
/* {{{ BobguiCssFontVariantCaps */

static const BobguiCssValueClass BOBGUI_CSS_VALUE_FONT_VARIANT_CAPS = {
  "BobguiCssFontVariantCapsValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_enum_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_value_enum_print
};

static BobguiCssValue font_variant_caps_values[] = {
  { &BOBGUI_CSS_VALUE_FONT_VARIANT_CAPS, 1, 1, 0, 0, BOBGUI_CSS_FONT_VARIANT_CAPS_NORMAL, "normal" },
  { &BOBGUI_CSS_VALUE_FONT_VARIANT_CAPS, 1, 1, 0, 0, BOBGUI_CSS_FONT_VARIANT_CAPS_SMALL_CAPS, "small-caps" },
  { &BOBGUI_CSS_VALUE_FONT_VARIANT_CAPS, 1, 1, 0, 0, BOBGUI_CSS_FONT_VARIANT_CAPS_ALL_SMALL_CAPS, "all-small-caps" },
  { &BOBGUI_CSS_VALUE_FONT_VARIANT_CAPS, 1, 1, 0, 0, BOBGUI_CSS_FONT_VARIANT_CAPS_PETITE_CAPS, "petite-caps" },
  { &BOBGUI_CSS_VALUE_FONT_VARIANT_CAPS, 1, 1, 0, 0, BOBGUI_CSS_FONT_VARIANT_CAPS_ALL_PETITE_CAPS, "all-petite-caps" },
  { &BOBGUI_CSS_VALUE_FONT_VARIANT_CAPS, 1, 1, 0, 0, BOBGUI_CSS_FONT_VARIANT_CAPS_UNICASE, "unicase" },
  { &BOBGUI_CSS_VALUE_FONT_VARIANT_CAPS, 1, 1, 0, 0, BOBGUI_CSS_FONT_VARIANT_CAPS_TITLING_CAPS, "titling-caps" }
};

BobguiCssValue *
_bobgui_css_font_variant_caps_value_new (BobguiCssFontVariantCaps caps)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (font_variant_caps_values); i++)
    {
      if (font_variant_caps_values[i].value == caps)
        return bobgui_css_value_ref (&font_variant_caps_values[i]);
    }

  g_return_val_if_reached (NULL);
}

BobguiCssValue *
_bobgui_css_font_variant_caps_value_try_parse (BobguiCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (font_variant_caps_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, font_variant_caps_values[i].name))
        return bobgui_css_value_ref (&font_variant_caps_values[i]);
    }

  return NULL;
}

BobguiCssFontVariantCaps
_bobgui_css_font_variant_caps_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_FONT_VARIANT_CAPS, BOBGUI_CSS_FONT_VARIANT_CAPS_NORMAL);

  return value->value;
}

/* }}} */
/* {{{ BobguiCssFontVariantAlternate */

static const BobguiCssValueClass BOBGUI_CSS_VALUE_FONT_VARIANT_ALTERNATE = {
  "BobguiCssFontVariantAlternateValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_enum_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_value_enum_print
};

static BobguiCssValue font_variant_alternate_values[] = {
  { &BOBGUI_CSS_VALUE_FONT_VARIANT_ALTERNATE, 1, 1, 0, 0, BOBGUI_CSS_FONT_VARIANT_ALTERNATE_NORMAL, "normal" },
  { &BOBGUI_CSS_VALUE_FONT_VARIANT_ALTERNATE, 1, 1, 0, 0, BOBGUI_CSS_FONT_VARIANT_ALTERNATE_HISTORICAL_FORMS, "historical-forms" }
};

BobguiCssValue *
_bobgui_css_font_variant_alternate_value_new (BobguiCssFontVariantAlternate alternate)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (font_variant_alternate_values); i++)
    {
      if (font_variant_alternate_values[i].value == alternate)
        return bobgui_css_value_ref (&font_variant_alternate_values[i]);
    }

  g_return_val_if_reached (NULL);
}

BobguiCssValue *
_bobgui_css_font_variant_alternate_value_try_parse (BobguiCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (font_variant_alternate_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, font_variant_alternate_values[i].name))
        return bobgui_css_value_ref (&font_variant_alternate_values[i]);
    }

  return NULL;
}

BobguiCssFontVariantAlternate
_bobgui_css_font_variant_alternate_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_FONT_VARIANT_ALTERNATE, BOBGUI_CSS_FONT_VARIANT_ALTERNATE_NORMAL);

  return value->value;
}

/* below are flags, which are handle a bit differently. We allocate values dynamically,
 * and we parse one bit at a time, while allowing for detection of invalid combinations.
 */

typedef struct {
  int value;
  const char *name;
} FlagsValue;

static gboolean
bobgui_css_value_flags_equal (const BobguiCssValue *enum1,
                           const BobguiCssValue *enum2)
{
  return enum1->value == enum2->value;
}

static void
bobgui_css_value_flags_print (const FlagsValue  *values,
                           guint              n_values,
                           const BobguiCssValue *value,
                           GString           *string)
{
  guint i;
  const char *sep = "";

  for (i = 0; i < n_values; i++)
    {
      if (value->value & values[i].value)
        {
          g_string_append (string, sep);
          g_string_append (string, values[i].name);
          sep = " ";
        }
    }
}

/* }}} */
/* {{{ BobguiTextDecorationLine */

static FlagsValue text_decoration_line_values[] = {
  { BOBGUI_CSS_TEXT_DECORATION_LINE_NONE, "none" },
  { BOBGUI_CSS_TEXT_DECORATION_LINE_UNDERLINE, "underline" },
  { BOBGUI_CSS_TEXT_DECORATION_LINE_OVERLINE, "overline" },
  { BOBGUI_CSS_TEXT_DECORATION_LINE_LINE_THROUGH, "line-through" },
};

static void
bobgui_css_text_decoration_line_value_print (const BobguiCssValue *value,
                                          GString           *string)
{
  bobgui_css_value_flags_print (text_decoration_line_values,
                             G_N_ELEMENTS (text_decoration_line_values),
                             value, string);
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_TEXT_DECORATION_LINE = {
  "BobguiCssTextDecorationLine",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_flags_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_text_decoration_line_value_print
};

static gboolean
text_decoration_line_is_valid (BobguiTextDecorationLine line)
{
  if ((line & BOBGUI_CSS_TEXT_DECORATION_LINE_NONE) &&
      (line != BOBGUI_CSS_TEXT_DECORATION_LINE_NONE))
    return FALSE;

  return TRUE;
}

BobguiCssValue *
_bobgui_css_text_decoration_line_value_new (BobguiTextDecorationLine line)
{
  BobguiCssValue *value;

  if (!text_decoration_line_is_valid (line))
    return NULL;

  value = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_TEXT_DECORATION_LINE);
  value->value = line;
  value->name = NULL;
  value->is_computed = TRUE;

  return value;
}

BobguiTextDecorationLine
_bobgui_css_text_decoration_line_try_parse_one (BobguiCssParser          *parser,
                                             BobguiTextDecorationLine  base)
{
  guint i;
  BobguiTextDecorationLine value = 0;

  g_return_val_if_fail (parser != NULL, 0);

  for (i = 0; i < G_N_ELEMENTS (text_decoration_line_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, text_decoration_line_values[i].name))
        {
          value = text_decoration_line_values[i].value;
          break;
        }
    }

  if (value == 0)
    return base; /* not parsing this value */

  if ((base | value) == base)
    return 0; /* repeated value */

  if (!text_decoration_line_is_valid (base | value))
    return 0; /* bad combination */

  return base | value;
}

BobguiTextDecorationLine
_bobgui_css_text_decoration_line_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_TEXT_DECORATION_LINE, BOBGUI_CSS_TEXT_DECORATION_LINE_NONE);

  return value->value;
}

/* }}} */
/* {{{ BobguiCssFontVariantLigature */

static FlagsValue font_variant_ligature_values[] = {
  { BOBGUI_CSS_FONT_VARIANT_LIGATURE_NORMAL, "normal" },
  { BOBGUI_CSS_FONT_VARIANT_LIGATURE_NONE, "none" },
  { BOBGUI_CSS_FONT_VARIANT_LIGATURE_COMMON_LIGATURES, "common-ligatures" },
  { BOBGUI_CSS_FONT_VARIANT_LIGATURE_NO_COMMON_LIGATURES, "no-common-ligatures" },
  { BOBGUI_CSS_FONT_VARIANT_LIGATURE_DISCRETIONARY_LIGATURES, "discretionary-ligatures" },
  { BOBGUI_CSS_FONT_VARIANT_LIGATURE_NO_DISCRETIONARY_LIGATURES, "no-discretionary-ligatures" },
  { BOBGUI_CSS_FONT_VARIANT_LIGATURE_HISTORICAL_LIGATURES, "historical-ligatures" },
  { BOBGUI_CSS_FONT_VARIANT_LIGATURE_NO_HISTORICAL_LIGATURES, "no-historical-ligatures" },
  { BOBGUI_CSS_FONT_VARIANT_LIGATURE_CONTEXTUAL, "contextual" },
  { BOBGUI_CSS_FONT_VARIANT_LIGATURE_NO_CONTEXTUAL, "no-contextual" }
};

static void
bobgui_css_font_variant_ligature_value_print (const BobguiCssValue *value,
                                           GString           *string)
{
  bobgui_css_value_flags_print (font_variant_ligature_values,
                             G_N_ELEMENTS (font_variant_ligature_values),
                             value, string);
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_FONT_VARIANT_LIGATURE = {
  "BobguiCssFontVariantLigatureValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_flags_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_font_variant_ligature_value_print
};

static gboolean
ligature_value_is_valid (BobguiCssFontVariantLigature ligatures)
{
  if (((ligatures & BOBGUI_CSS_FONT_VARIANT_LIGATURE_NORMAL) &&
       (ligatures != BOBGUI_CSS_FONT_VARIANT_LIGATURE_NORMAL)) ||
      ((ligatures & BOBGUI_CSS_FONT_VARIANT_LIGATURE_NONE) &&
       (ligatures != BOBGUI_CSS_FONT_VARIANT_LIGATURE_NONE)) ||
      ((ligatures & BOBGUI_CSS_FONT_VARIANT_LIGATURE_COMMON_LIGATURES) &&
       (ligatures & BOBGUI_CSS_FONT_VARIANT_LIGATURE_NO_COMMON_LIGATURES)) ||
      ((ligatures & BOBGUI_CSS_FONT_VARIANT_LIGATURE_DISCRETIONARY_LIGATURES) &&
       (ligatures & BOBGUI_CSS_FONT_VARIANT_LIGATURE_NO_DISCRETIONARY_LIGATURES)) ||
      ((ligatures & BOBGUI_CSS_FONT_VARIANT_LIGATURE_HISTORICAL_LIGATURES) &&
       (ligatures & BOBGUI_CSS_FONT_VARIANT_LIGATURE_NO_HISTORICAL_LIGATURES)) ||
      ((ligatures & BOBGUI_CSS_FONT_VARIANT_LIGATURE_CONTEXTUAL) &&
       (ligatures & BOBGUI_CSS_FONT_VARIANT_LIGATURE_NO_CONTEXTUAL)))
    return FALSE;

  return TRUE;
}

BobguiCssValue *
_bobgui_css_font_variant_ligature_value_new (BobguiCssFontVariantLigature ligatures)
{
  BobguiCssValue *value;

  if (!ligature_value_is_valid (ligatures))
    return NULL;

  value = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_FONT_VARIANT_LIGATURE);
  value->value = ligatures;
  value->name = NULL;
  value->is_computed = TRUE;

  return value;
}

BobguiCssFontVariantLigature
_bobgui_css_font_variant_ligature_try_parse_one (BobguiCssParser              *parser,
                                              BobguiCssFontVariantLigature  base)
{
  guint i;
  BobguiCssFontVariantLigature value = 0;

  g_return_val_if_fail (parser != NULL, 0);

  for (i = 0; i < G_N_ELEMENTS (font_variant_ligature_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, font_variant_ligature_values[i].name))
        {
          value = font_variant_ligature_values[i].value;
          break;
        }
    }

  if (value == 0)
    return base; /* not parsing this value */

  if ((base | value) == base)
    return 0; /* repeated value */

  if (!ligature_value_is_valid (base | value))
    return 0; /* bad combination */

  return base | value;
}

BobguiCssFontVariantLigature
_bobgui_css_font_variant_ligature_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_FONT_VARIANT_LIGATURE, BOBGUI_CSS_FONT_VARIANT_LIGATURE_NORMAL);

  return value->value;
}

/* }}} */
/* {{{ BobguiCssFontVariantNumeric */

static FlagsValue font_variant_numeric_values[] = {
  { BOBGUI_CSS_FONT_VARIANT_NUMERIC_NORMAL, "normal" },
  { BOBGUI_CSS_FONT_VARIANT_NUMERIC_LINING_NUMS, "lining-nums" },
  { BOBGUI_CSS_FONT_VARIANT_NUMERIC_OLDSTYLE_NUMS, "oldstyle-nums" },
  { BOBGUI_CSS_FONT_VARIANT_NUMERIC_PROPORTIONAL_NUMS, "proportional-nums" },
  { BOBGUI_CSS_FONT_VARIANT_NUMERIC_TABULAR_NUMS, "tabular-nums" },
  { BOBGUI_CSS_FONT_VARIANT_NUMERIC_DIAGONAL_FRACTIONS, "diagonal-fractions" },
  { BOBGUI_CSS_FONT_VARIANT_NUMERIC_STACKED_FRACTIONS, "stacked-fractions" },
  { BOBGUI_CSS_FONT_VARIANT_NUMERIC_ORDINAL, "ordinal" },
  { BOBGUI_CSS_FONT_VARIANT_NUMERIC_SLASHED_ZERO, "slashed-zero" }
};

static void
bobgui_css_font_variant_numeric_value_print (const BobguiCssValue *value,
                                          GString           *string)
{
  bobgui_css_value_flags_print (font_variant_numeric_values,
                             G_N_ELEMENTS (font_variant_numeric_values),
                             value, string);
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_FONT_VARIANT_NUMERIC = {
  "BobguiCssFontVariantNumbericValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_flags_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_font_variant_numeric_value_print
};

static gboolean
numeric_value_is_valid (BobguiCssFontVariantNumeric numeric)
{
  if (((numeric & BOBGUI_CSS_FONT_VARIANT_NUMERIC_NORMAL) &&
       (numeric != BOBGUI_CSS_FONT_VARIANT_NUMERIC_NORMAL)) ||
      ((numeric & BOBGUI_CSS_FONT_VARIANT_NUMERIC_LINING_NUMS) &&
       (numeric & BOBGUI_CSS_FONT_VARIANT_NUMERIC_OLDSTYLE_NUMS)) ||
      ((numeric & BOBGUI_CSS_FONT_VARIANT_NUMERIC_PROPORTIONAL_NUMS) &&
       (numeric & BOBGUI_CSS_FONT_VARIANT_NUMERIC_TABULAR_NUMS)) ||
      ((numeric & BOBGUI_CSS_FONT_VARIANT_NUMERIC_DIAGONAL_FRACTIONS) &&
       (numeric & BOBGUI_CSS_FONT_VARIANT_NUMERIC_STACKED_FRACTIONS)))
    return FALSE;

  return TRUE;
}

BobguiCssValue *
_bobgui_css_font_variant_numeric_value_new (BobguiCssFontVariantNumeric numeric)
{
  BobguiCssValue *value;

  if (!numeric_value_is_valid (numeric))
    return NULL;

  value = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_FONT_VARIANT_NUMERIC);
  value->value = numeric;
  value->name = NULL;
  value->is_computed = TRUE;

  return value;
}

BobguiCssFontVariantNumeric
_bobgui_css_font_variant_numeric_try_parse_one (BobguiCssParser             *parser,
                                             BobguiCssFontVariantNumeric  base)
{
  guint i;
  BobguiCssFontVariantNumeric value = 0;

  g_return_val_if_fail (parser != NULL, 0);

  for (i = 0; i < G_N_ELEMENTS (font_variant_numeric_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, font_variant_numeric_values[i].name))
        {
          value = font_variant_numeric_values[i].value;
          break;
        }
    }

  if (value == 0)
    return base; /* not parsing this value */

  if ((base | value) == base)
    return 0; /* repeated value */

  if (!numeric_value_is_valid (base | value))
    return 0; /* bad combination */

  return base | value;
}

BobguiCssFontVariantNumeric
_bobgui_css_font_variant_numeric_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_FONT_VARIANT_NUMERIC, BOBGUI_CSS_FONT_VARIANT_NUMERIC_NORMAL);

  return value->value;
}

/* }}} */
/* {{{ BobguiCssFontVariantEastAsian */

static FlagsValue font_variant_east_asian_values[] = {
  { BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_NORMAL, "normal" },
  { BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_JIS78, "jis78" },
  { BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_JIS83, "jis83" },
  { BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_JIS90, "jis90" },
  { BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_JIS04, "jis04" },
  { BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_SIMPLIFIED, "simplified" },
  { BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_TRADITIONAL, "traditional" },
  { BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_FULL_WIDTH, "full-width" },
  { BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_PROPORTIONAL, "proportional-width" },
  { BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_RUBY, "ruby" }
};

static void
bobgui_css_font_variant_east_asian_value_print (const BobguiCssValue *value,
                                             GString           *string)
{
  bobgui_css_value_flags_print (font_variant_east_asian_values,
                             G_N_ELEMENTS (font_variant_east_asian_values),
                             value, string);
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_FONT_VARIANT_EAST_ASIAN = {
  "BobguiCssFontVariantEastAsianValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_flags_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_font_variant_east_asian_value_print
};

static gboolean
east_asian_value_is_valid (BobguiCssFontVariantEastAsian east_asian)
{
  if ((east_asian & BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_NORMAL) &&
      (east_asian != BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_NORMAL))
    return FALSE;

  if (bobgui_popcount (east_asian & (BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_JIS78 |
                                  BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_JIS83 |
                                  BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_JIS90 |
                                  BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_JIS04 |
                                  BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_SIMPLIFIED |
                                  BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_TRADITIONAL)) > 1)
    return FALSE;

  if (bobgui_popcount (east_asian & (BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_FULL_WIDTH |
                                  BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_PROPORTIONAL)) > 1)
    return FALSE;

  return TRUE;
}

BobguiCssValue *
_bobgui_css_font_variant_east_asian_value_new (BobguiCssFontVariantEastAsian east_asian)
{
  BobguiCssValue *value;

  if (!east_asian_value_is_valid (east_asian))
    return NULL;

  value = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_FONT_VARIANT_EAST_ASIAN);
  value->value = east_asian;
  value->name = NULL;
  value->is_computed = TRUE;

  return value;
}

BobguiCssFontVariantEastAsian
_bobgui_css_font_variant_east_asian_try_parse_one (BobguiCssParser               *parser,
                                                BobguiCssFontVariantEastAsian  base)
{
  guint i;
  BobguiCssFontVariantEastAsian value = 0;

  g_return_val_if_fail (parser != NULL, 0);

  for (i = 0; i < G_N_ELEMENTS (font_variant_east_asian_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, font_variant_east_asian_values[i].name))
        {
          value = font_variant_east_asian_values[i].value;
          break;
        }
    }

  if (value == 0)
    return base; /* not parsing this value */

  if ((base | value) == base)
    return 0; /* repeated value */

  if (!east_asian_value_is_valid (base | value))
    return 0; /* bad combination */

  return base | value;
}

BobguiCssFontVariantEastAsian
_bobgui_css_font_variant_east_asian_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_FONT_VARIANT_EAST_ASIAN, BOBGUI_CSS_FONT_VARIANT_EAST_ASIAN_NORMAL);

  return value->value;
}

/* }}} */
/* {{{ BobguiTextTransform */

static const BobguiCssValueClass BOBGUI_CSS_VALUE_TEXT_TRANSFORM = {
  "BobguiCssTextTransformValue",
  bobgui_css_value_enum_free,
  bobgui_css_value_enum_compute,
  NULL,
  bobgui_css_value_enum_equal,
  bobgui_css_value_enum_transition,
  NULL,
  NULL,
  bobgui_css_value_enum_print
};

static BobguiCssValue text_transform_values[] = {
  { &BOBGUI_CSS_VALUE_TEXT_TRANSFORM, 1, 1, 0, 0, BOBGUI_CSS_TEXT_TRANSFORM_NONE, "none" },
  { &BOBGUI_CSS_VALUE_TEXT_TRANSFORM, 1, 1, 0, 0, BOBGUI_CSS_TEXT_TRANSFORM_LOWERCASE, "lowercase" },
  { &BOBGUI_CSS_VALUE_TEXT_TRANSFORM, 1, 1, 0, 0, BOBGUI_CSS_TEXT_TRANSFORM_UPPERCASE, "uppercase" },
  { &BOBGUI_CSS_VALUE_TEXT_TRANSFORM, 1, 1, 0, 0, BOBGUI_CSS_TEXT_TRANSFORM_CAPITALIZE, "capitalize" },
};

BobguiCssValue *
_bobgui_css_text_transform_value_new (BobguiTextTransform transform)
{
  g_return_val_if_fail (transform < G_N_ELEMENTS (text_transform_values), NULL);

  return bobgui_css_value_ref (&text_transform_values[transform]);
}

BobguiCssValue *
_bobgui_css_text_transform_value_try_parse (BobguiCssParser *parser)
{
  guint i;

  g_return_val_if_fail (parser != NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (text_transform_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, text_transform_values[i].name))
        return bobgui_css_value_ref (&text_transform_values[i]);
    }

  return NULL;
}

BobguiTextTransform
_bobgui_css_text_transform_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_TEXT_TRANSFORM, BOBGUI_CSS_TEXT_TRANSFORM_NONE);

  return value->value;
}

/* }}} */
/* vim:set foldmethod=marker: */
