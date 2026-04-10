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

#include "bobguicsspositionvalueprivate.h"

#include "bobguicssnumbervalueprivate.h"

struct _BobguiCssValue {
  BOBGUI_CSS_VALUE_BASE
  BobguiCssValue *x;
  BobguiCssValue *y;
};

static void
bobgui_css_value_position_free (BobguiCssValue *value)
{
  bobgui_css_value_unref (value->x);
  bobgui_css_value_unref (value->y);
  g_free (value);
}

static BobguiCssValue *
bobgui_css_value_position_compute (BobguiCssValue          *position,
                                guint                 property_id,
                                BobguiCssComputeContext *context)
{
  BobguiCssValue *x, *y;

  x = bobgui_css_value_compute (position->x, property_id, context);
  y = bobgui_css_value_compute (position->y, property_id, context);
  if (x == position->x && y == position->y)
    {
      bobgui_css_value_unref (x);
      bobgui_css_value_unref (y);
      return bobgui_css_value_ref (position);
    }

  return _bobgui_css_position_value_new (x, y);
}

static gboolean
bobgui_css_value_position_equal (const BobguiCssValue *position1,
                              const BobguiCssValue *position2)
{
  return bobgui_css_value_equal (position1->x, position2->x)
      && bobgui_css_value_equal (position1->y, position2->y);
}

static BobguiCssValue *
bobgui_css_value_position_transition (BobguiCssValue *start,
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

  return _bobgui_css_position_value_new (x, y);
}

static void
bobgui_css_value_position_print (const BobguiCssValue *position,
                              GString           *string)
{
  struct {
    const char *x_name;
    const char *y_name;
    BobguiCssValue *number;
  } values[] = { 
    { "left",   "top",    bobgui_css_number_value_new (0, BOBGUI_CSS_PERCENT) },
    { "right",  "bottom", bobgui_css_number_value_new (100, BOBGUI_CSS_PERCENT) }
  };
  BobguiCssValue *center = bobgui_css_number_value_new (50, BOBGUI_CSS_PERCENT);
  guint i;

  if (bobgui_css_value_equal (position->x, center))
    {
      if (bobgui_css_value_equal (position->y, center))
        {
          g_string_append (string, "center");
          goto done;
        }
    }
  else
    {
      for (i = 0; i < G_N_ELEMENTS (values); i++)
        {
          if (bobgui_css_value_equal (position->x, values[i].number))
            {
              g_string_append (string, values[i].x_name);
              break;
            }
        }
      if (i == G_N_ELEMENTS (values))
        bobgui_css_value_print (position->x, string);

      if (bobgui_css_value_equal (position->y, center))
        goto done;

      g_string_append_c (string, ' ');
    }

  for (i = 0; i < G_N_ELEMENTS (values); i++)
    {
      if (bobgui_css_value_equal (position->y, values[i].number))
        {
          g_string_append (string, values[i].y_name);
          goto done;
        }
    }
  if (i == G_N_ELEMENTS (values))
    {
      if (bobgui_css_value_equal (position->x, center))
        g_string_append (string, "center ");
      bobgui_css_value_print (position->y, string);
    }

done:
  for (i = 0; i < G_N_ELEMENTS (values); i++)
    bobgui_css_value_unref (values[i].number);
  bobgui_css_value_unref (center);
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_POSITION = {
  "BobguiCssPositionValue",
  bobgui_css_value_position_free,
  bobgui_css_value_position_compute,
  NULL,
  bobgui_css_value_position_equal,
  bobgui_css_value_position_transition,
  NULL,
  NULL,
  bobgui_css_value_position_print
};

BobguiCssValue *
_bobgui_css_position_value_new (BobguiCssValue *x,
                             BobguiCssValue *y)
{
  BobguiCssValue *result;

  result = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_POSITION);
  result->x = x;
  result->y = y;
  result->is_computed = bobgui_css_value_is_computed (x) &&
                        bobgui_css_value_is_computed (y);

  return result;
}

static BobguiCssValue *
position_value_parse (BobguiCssParser *parser, gboolean try)
{
  static const struct {
    const char *name;
    guint       percentage;
    gboolean    horizontal;
    gboolean    swap;
  } names[] = {
    { "left",     0, TRUE,  FALSE },
    { "right",  100, TRUE,  FALSE },
    { "center",  50, TRUE,  TRUE  },
    { "top",      0, FALSE, FALSE },
    { "bottom", 100, FALSE, FALSE  },
  };
  BobguiCssValue *x = NULL, *y = NULL;
  gboolean swap = FALSE;
  guint i;

  for (i = 0; i < G_N_ELEMENTS (names); i++)
    {
      if (bobgui_css_parser_try_ident (parser, names[i].name))
        {
          if (names[i].horizontal)
	    x = bobgui_css_number_value_new (names[i].percentage, BOBGUI_CSS_PERCENT);
          else
	    y = bobgui_css_number_value_new (names[i].percentage, BOBGUI_CSS_PERCENT);
          swap = names[i].swap;
          break;
        }
    }
  if (i == G_N_ELEMENTS (names))
    {
      if (bobgui_css_number_value_can_parse (parser))
        {
          x = bobgui_css_number_value_parse (parser,
                                          BOBGUI_CSS_PARSE_PERCENT
                                          | BOBGUI_CSS_PARSE_LENGTH);

          if (x == NULL)
            return NULL;
        }
      else
        {
          if (!try)
            bobgui_css_parser_error_syntax (parser, "Unrecognized position value");
          return NULL;
        }
    }

  for (i = 0; i < G_N_ELEMENTS (names); i++)
    {
      if (!swap && !names[i].swap)
        {
          if (names[i].horizontal && x != NULL)
            continue;
          if (!names[i].horizontal && y != NULL)
            continue;
        }

      if (bobgui_css_parser_try_ident (parser, names[i].name))
        {
          if (x)
            {
              if (names[i].horizontal && !names[i].swap)
                {
                  y = x;
	          x = bobgui_css_number_value_new (names[i].percentage, BOBGUI_CSS_PERCENT);
                }
              else
                {
	          y = bobgui_css_number_value_new (names[i].percentage, BOBGUI_CSS_PERCENT);
                }
            }
          else
            {
              g_assert (names[i].horizontal || names[i].swap);
	      x = bobgui_css_number_value_new (names[i].percentage, BOBGUI_CSS_PERCENT);
            }
          break;
        }
    }

  if (i == G_N_ELEMENTS (names))
    {
      if (bobgui_css_number_value_can_parse (parser))
        {
          if (y != NULL)
            {
              if (!try)
                bobgui_css_parser_error_syntax (parser, "Invalid combination of values");
              bobgui_css_value_unref (y);
              return NULL;
            }
          y = bobgui_css_number_value_parse (parser,
                                          BOBGUI_CSS_PARSE_PERCENT
                                          | BOBGUI_CSS_PARSE_LENGTH);
          if (y == NULL)
            {
              bobgui_css_value_unref (x);
	      return NULL;
            }
        }
      else
        {
          if (y)
            x = bobgui_css_number_value_new (50, BOBGUI_CSS_PERCENT);
          else
            y = bobgui_css_number_value_new (50, BOBGUI_CSS_PERCENT);
        }
    }

  return _bobgui_css_position_value_new (x, y);
}

BobguiCssValue *
_bobgui_css_position_value_parse (BobguiCssParser *parser)
{
  return position_value_parse (parser, FALSE);
}

BobguiCssValue *
_bobgui_css_position_value_try_parse (BobguiCssParser *parser)
{
  return position_value_parse (parser, TRUE);
}

BobguiCssValue *
bobgui_css_position_value_parse_spacing (BobguiCssParser *parser)
{
  BobguiCssValue *x, *y;

  x = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_LENGTH | BOBGUI_CSS_POSITIVE_ONLY);
  if (x == NULL)
    return NULL;

  if (bobgui_css_number_value_can_parse (parser))
    {
      y = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_LENGTH | BOBGUI_CSS_POSITIVE_ONLY);
      if (y == NULL)
        {
          bobgui_css_value_unref (x);
          return NULL;
        }
    }
  else
    {
      y = bobgui_css_value_ref (x);
    }

  return _bobgui_css_position_value_new (x, y);
}

double
_bobgui_css_position_value_get_x (const BobguiCssValue *position,
                               double             one_hundred_percent)
{
  g_return_val_if_fail (position != NULL, 0.0);
  g_return_val_if_fail (position->class == &BOBGUI_CSS_VALUE_POSITION, 0.0);

  return bobgui_css_number_value_get (position->x, one_hundred_percent);
}

double
_bobgui_css_position_value_get_y (const BobguiCssValue *position,
                               double             one_hundred_percent)
{
  g_return_val_if_fail (position != NULL, 0.0);
  g_return_val_if_fail (position->class == &BOBGUI_CSS_VALUE_POSITION, 0.0);

  return bobgui_css_number_value_get (position->y, one_hundred_percent);
}

