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

#include "bobguicssbgsizevalueprivate.h"

#include "bobguicssnumbervalueprivate.h"

struct _BobguiCssValue {
  BOBGUI_CSS_VALUE_BASE
  guint cover :1;
  guint contain :1;
  BobguiCssValue *x;
  BobguiCssValue *y;
};

static void
bobgui_css_value_bg_size_free (BobguiCssValue *value)
{
  if (value->x)
    bobgui_css_value_unref (value->x);
  if (value->y)
    bobgui_css_value_unref (value->y);

  g_free (value);
}

static BobguiCssValue *
bobgui_css_value_bg_size_compute (BobguiCssValue          *value,
                               guint                 property_id,
                               BobguiCssComputeContext *context)
{
  BobguiCssValue *x, *y;

  if (value->x == NULL && value->y == NULL)
    return bobgui_css_value_ref (value);

  x = y = NULL;

  if (value->x)
    x = bobgui_css_value_compute (value->x, property_id, context);

  if (value->y)
    y = bobgui_css_value_compute (value->y, property_id, context);

  if (x == value->x && y == value->y)
    {
      if (x)
        bobgui_css_value_unref (x);
      if (y)
        bobgui_css_value_unref (y);

      return bobgui_css_value_ref (value);
    }

  return _bobgui_css_bg_size_value_new (value->x ? x : NULL,
                                     value->y ? y : NULL);
}

static gboolean
bobgui_css_value_bg_size_equal (const BobguiCssValue *value1,
                             const BobguiCssValue *value2)
{
  return value1->cover == value2->cover &&
         value1->contain == value2->contain &&
         (value1->x == value2->x ||
          (value1->x != NULL && value2->x != NULL &&
           bobgui_css_value_equal (value1->x, value2->x))) &&
         (value1->y == value2->y ||
          (value1->y != NULL && value2->y != NULL &&
           bobgui_css_value_equal (value1->y, value2->y)));
}

static BobguiCssValue *
bobgui_css_value_bg_size_transition (BobguiCssValue *start,
                                  BobguiCssValue *end,
                                  guint        property_id,
                                  double       progress)
{
  BobguiCssValue *x, *y;

  if (start->cover)
    return end->cover ? bobgui_css_value_ref (end) : NULL;
  if (start->contain)
    return end->contain ? bobgui_css_value_ref (end) : NULL;

  if ((start->x != NULL) ^ (end->x != NULL) ||
      (start->y != NULL) ^ (end->y != NULL))
    return NULL;

  if (start->x)
    {
      x = bobgui_css_value_transition (start->x, end->x, property_id, progress);
      if (x == NULL)
        return NULL;
    }
  else
    x = NULL;

  if (start->y)
    {
      y = bobgui_css_value_transition (start->y, end->y, property_id, progress);
      if (y == NULL)
        {
          bobgui_css_value_unref (x);
          return NULL;
        }
    }
  else
    y = NULL;

  return _bobgui_css_bg_size_value_new (x, y);
}

static void
bobgui_css_value_bg_size_print (const BobguiCssValue *value,
                             GString           *string)
{
  if (value->cover)
    g_string_append (string, "cover");
  else if (value->contain)
    g_string_append (string, "contain");
  else
    {
      if (value->x == NULL)
        g_string_append (string, "auto");
      else
        bobgui_css_value_print (value->x, string);

      if (value->y)
        {
          g_string_append_c (string, ' ');
          bobgui_css_value_print (value->y, string);
        }
    }
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_BG_SIZE = {
  "BobguiCssBgSizeValue",
  bobgui_css_value_bg_size_free,
  bobgui_css_value_bg_size_compute,
  NULL,
  bobgui_css_value_bg_size_equal,
  bobgui_css_value_bg_size_transition,
  NULL,
  NULL,
  bobgui_css_value_bg_size_print
};

static BobguiCssValue auto_singleton = { &BOBGUI_CSS_VALUE_BG_SIZE, 1, 1, 0, 0, FALSE, FALSE, NULL, NULL };
static BobguiCssValue cover_singleton = { &BOBGUI_CSS_VALUE_BG_SIZE, 1, 1, 0, 0, TRUE, FALSE, NULL, NULL };
static BobguiCssValue contain_singleton = { &BOBGUI_CSS_VALUE_BG_SIZE, 1, 1, 0, 0, FALSE, TRUE, NULL, NULL };

BobguiCssValue *
_bobgui_css_bg_size_value_new (BobguiCssValue *x,
                            BobguiCssValue *y)
{
  BobguiCssValue *result;

  if (x == NULL && y == NULL)
    return bobgui_css_value_ref (&auto_singleton);

  result = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_BG_SIZE);
  result->x = x;
  result->y = y;
  result->is_computed = (!x || bobgui_css_value_is_computed (x)) &&
                        (!y || bobgui_css_value_is_computed (y));

  return result;
}

BobguiCssValue *
_bobgui_css_bg_size_value_parse (BobguiCssParser *parser)
{
  BobguiCssValue *x, *y;

  if (bobgui_css_parser_try_ident (parser, "cover"))
    return bobgui_css_value_ref (&cover_singleton);
  else if (bobgui_css_parser_try_ident (parser, "contain"))
    return bobgui_css_value_ref (&contain_singleton);

  if (bobgui_css_parser_try_ident (parser, "auto"))
    x = NULL;
  else
    {
      x = bobgui_css_number_value_parse (parser,
                                      BOBGUI_CSS_POSITIVE_ONLY
                                      | BOBGUI_CSS_PARSE_PERCENT
                                      | BOBGUI_CSS_PARSE_LENGTH);
      if (x == NULL)
        return NULL;
    }

  if (bobgui_css_parser_try_ident (parser, "auto"))
    y = NULL;
  else if (!bobgui_css_number_value_can_parse (parser))
    y = NULL;
  else
    {
      y = bobgui_css_number_value_parse (parser,
                                      BOBGUI_CSS_POSITIVE_ONLY
                                      | BOBGUI_CSS_PARSE_PERCENT
                                      | BOBGUI_CSS_PARSE_LENGTH);
      if (y == NULL)
        {
          bobgui_css_value_unref (x);
          return NULL;
        }
    }

  return _bobgui_css_bg_size_value_new (x, y);
}

static void
bobgui_css_bg_size_compute_size_for_cover_contain (gboolean     cover,
                                                BobguiCssImage *image,
                                                double       width,
                                                double       height,
                                                double      *concrete_width,
                                                double      *concrete_height)
{
  double aspect, image_aspect;
  
  image_aspect = _bobgui_css_image_get_aspect_ratio (image);
  if (image_aspect == 0.0)
    {
      *concrete_width = width;
      *concrete_height = height;
      return;
    }

  aspect = width / height;

  if ((aspect >= image_aspect && cover) ||
      (aspect < image_aspect && !cover))
    {
      *concrete_width = width;
      *concrete_height = width / image_aspect;
    }
  else
    {
      *concrete_height = height;
      *concrete_width = height * image_aspect;
    }
}

void
_bobgui_css_bg_size_value_compute_size (const BobguiCssValue *value,
                                     BobguiCssImage       *image,
                                     double             area_width,
                                     double             area_height,
                                     double            *out_width,
                                     double            *out_height)
{
  g_return_if_fail (value->class == &BOBGUI_CSS_VALUE_BG_SIZE);

  if (value->contain || value->cover)
    {
      bobgui_css_bg_size_compute_size_for_cover_contain (value->cover,
                                                      image,
                                                      area_width, area_height,
                                                      out_width, out_height);
    }
  else
    {
      double x, y;

      /* note: 0 does the right thing later for 'auto' */
      x = value->x ? bobgui_css_number_value_get (value->x, area_width) : 0;
      y = value->y ? bobgui_css_number_value_get (value->y, area_height) : 0;

      if ((x <= 0 && value->x) ||
          (y <= 0 && value->y))
        {
          *out_width = 0;
          *out_height = 0;
        }
      else
        {
          _bobgui_css_image_get_concrete_size (image,
                                            x, y,
                                            area_width, area_height,
                                            out_width, out_height);
        }
    }
}

