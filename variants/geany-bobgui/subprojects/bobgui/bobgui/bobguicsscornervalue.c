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

#include "bobguicsscornervalueprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicssdimensionvalueprivate.h"

struct _BobguiCssValue {
  BOBGUI_CSS_VALUE_BASE
  BobguiCssValue *x;
  BobguiCssValue *y;
};

static void
bobgui_css_value_corner_free (BobguiCssValue *value)
{
  bobgui_css_value_unref (value->x);
  bobgui_css_value_unref (value->y);

  g_free (value);
}

static BobguiCssValue *
bobgui_css_value_corner_compute (BobguiCssValue          *corner,
                              guint                 property_id,
                              BobguiCssComputeContext *context)
{
  BobguiCssValue *x, *y;

  x = bobgui_css_value_compute (corner->x, property_id, context);
  y = bobgui_css_value_compute (corner->y, property_id, context);
  if (x == corner->x && y == corner->y)
    {
      bobgui_css_value_unref (x);
      bobgui_css_value_unref (y);
      return bobgui_css_value_ref (corner);
    }

  return _bobgui_css_corner_value_new (x, y);
}

static gboolean
bobgui_css_value_corner_equal (const BobguiCssValue *corner1,
                            const BobguiCssValue *corner2)
{
  return bobgui_css_value_equal (corner1->x, corner2->x)
      && bobgui_css_value_equal (corner1->y, corner2->y);
}

static BobguiCssValue *
bobgui_css_value_corner_transition (BobguiCssValue *start,
                                 BobguiCssValue *end,
                                 guint        property_id,
                                 double       progress)
{
  BobguiCssValue *x, *y;

  x = bobgui_css_value_transition (start->x, end->x, property_id, progress);
  if (x == NULL)
    return NULL;
  y = bobgui_css_value_transition (start->y, end->y, property_id, progress);
  if (y == NULL)
    {
      bobgui_css_value_unref (x);
      return NULL;
    }

  return _bobgui_css_corner_value_new (x, y);
}

static void
bobgui_css_value_corner_print (const BobguiCssValue *corner,
                            GString           *string)
{
  bobgui_css_value_print (corner->x, string);
  if (!bobgui_css_value_equal (corner->x, corner->y))
    {
      g_string_append_c (string, ' ');
      bobgui_css_value_print (corner->y, string);
    }
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_CORNER = {
  "BobguiCssCornerValue",
  bobgui_css_value_corner_free,
  bobgui_css_value_corner_compute,
  NULL,
  bobgui_css_value_corner_equal,
  bobgui_css_value_corner_transition,
  NULL,
  NULL,
  bobgui_css_value_corner_print
};

static BobguiCssValue corner_singletons[] = {
  { &BOBGUI_CSS_VALUE_CORNER, 1, 1, 0, 0, NULL, NULL },
  { &BOBGUI_CSS_VALUE_CORNER, 1, 1, 0, 0, NULL, NULL },
  { &BOBGUI_CSS_VALUE_CORNER, 1, 1, 0, 0, NULL, NULL },
  { &BOBGUI_CSS_VALUE_CORNER, 1, 1, 0, 0, NULL, NULL },
  { &BOBGUI_CSS_VALUE_CORNER, 1, 1, 0, 0, NULL, NULL },
  { &BOBGUI_CSS_VALUE_CORNER, 1, 1, 0, 0, NULL, NULL },
  { &BOBGUI_CSS_VALUE_CORNER, 1, 1, 0, 0, NULL, NULL },
  { &BOBGUI_CSS_VALUE_CORNER, 1, 1, 0, 0, NULL, NULL },
};

static inline void
initialize_corner_singletons (void)
{
  static gboolean initialized = FALSE;

  if (initialized)
    return;

  for (unsigned int i = 0; i < G_N_ELEMENTS (corner_singletons); i++)
    {
      corner_singletons[i].x = bobgui_css_dimension_value_new (i, BOBGUI_CSS_PX);
      corner_singletons[i].y = bobgui_css_value_ref (corner_singletons[i].x);
    }

  initialized = TRUE;
}

BobguiCssValue *
_bobgui_css_corner_value_new (BobguiCssValue *x,
                           BobguiCssValue *y)
{
  BobguiCssValue *result;

  if (x == y &&
      bobgui_css_number_value_get_dimension (x) == BOBGUI_CSS_DIMENSION_LENGTH)
    {
      initialize_corner_singletons ();

      for (unsigned int i = 0; i < G_N_ELEMENTS (corner_singletons); i++)
        {
          if (corner_singletons[i].x == x)
            {
              bobgui_css_value_unref (x);
              bobgui_css_value_unref (y);

              return bobgui_css_value_ref (&corner_singletons[i]);
            }
        }
    }

  result = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_CORNER);
  result->x = x;
  result->y = y;

  return result;
}

BobguiCssValue *
_bobgui_css_corner_value_parse (BobguiCssParser *parser)
{
  BobguiCssValue *x, *y;

  x = bobgui_css_number_value_parse (parser,
                                  BOBGUI_CSS_POSITIVE_ONLY
                                  | BOBGUI_CSS_PARSE_PERCENT
                                  | BOBGUI_CSS_PARSE_LENGTH);
  if (x == NULL)
    return NULL;

  if (!bobgui_css_number_value_can_parse (parser))
    y = bobgui_css_value_ref (x);
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

  return _bobgui_css_corner_value_new (x, y);
}

double
_bobgui_css_corner_value_get_x (const BobguiCssValue *corner,
                             double             one_hundred_percent)
{
  g_return_val_if_fail (corner != NULL, 0.0);
  g_return_val_if_fail (corner->class == &BOBGUI_CSS_VALUE_CORNER, 0.0);

  return bobgui_css_number_value_get (corner->x, one_hundred_percent);
}

double
_bobgui_css_corner_value_get_y (const BobguiCssValue *corner,
                             double             one_hundred_percent)
{
  g_return_val_if_fail (corner != NULL, 0.0);
  g_return_val_if_fail (corner->class == &BOBGUI_CSS_VALUE_CORNER, 0.0);

  return bobgui_css_number_value_get (corner->y, one_hundred_percent);
}

gboolean
bobgui_css_corner_value_is_zero (const BobguiCssValue *corner)
{
  if (corner->class != &BOBGUI_CSS_VALUE_CORNER)
    return bobgui_css_dimension_value_is_zero (corner);

  return bobgui_css_dimension_value_is_zero (corner->x) &&
         bobgui_css_dimension_value_is_zero (corner->y);
}

