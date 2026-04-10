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

#include "bobguicssbordervalueprivate.h"

#include "bobguicssnumbervalueprivate.h"

struct _BobguiCssValue {
  BOBGUI_CSS_VALUE_BASE
  guint fill :1;
  BobguiCssValue *values[4];
};

static void
bobgui_css_value_border_free (BobguiCssValue *value)
{
  guint i;

  for (i = 0; i < 4; i++)
    {
      if (value->values[i])
        bobgui_css_value_unref (value->values[i]);
    }

  g_free (value);
}

static BobguiCssValue *
bobgui_css_value_border_compute (BobguiCssValue          *value,
                              guint                 property_id,
                              BobguiCssComputeContext *context)
{
  BobguiCssValue *values[4];
  BobguiCssValue *computed;
  gboolean changed = FALSE;
  guint i;

  for (i = 0; i < 4; i++)
    {
      if (value->values[i])
        {
          values[i] = bobgui_css_value_compute (value->values[i], property_id, context);
          changed |= (values[i] != value->values[i]);
        }
      else
        {
          values[i] = NULL;
        }
    }

  if (!changed)
    {
      for (i = 0; i < 4; i++)
        {
          if (values[i] != NULL)
            bobgui_css_value_unref (values[i]);
        }
      return bobgui_css_value_ref (value);
    }

  computed = _bobgui_css_border_value_new (values[0], values[1], values[2], values[3]);
  computed->fill = value->fill;

  return computed;
}

static gboolean
bobgui_css_value_border_equal (const BobguiCssValue *value1,
                            const BobguiCssValue *value2)
{
  guint i;

  if (value1->fill != value2->fill)
    return FALSE;

  for (i = 0; i < 4; i++)
    {
      if (!bobgui_css_value_equal0 (value1->values[i], value2->values[i]))
        return FALSE;
    }

  return TRUE;
}

static BobguiCssValue *
bobgui_css_value_border_transition (BobguiCssValue *start,
                                 BobguiCssValue *end,
                                 guint        property_id,
                                 double       progress)
{
  return NULL;
}

static void
bobgui_css_value_border_print (const BobguiCssValue *value,
                            GString           *string)
{
  guint i, n;

  if (!bobgui_css_value_equal0 (value->values[BOBGUI_CSS_RIGHT], value->values[BOBGUI_CSS_LEFT]))
    n = 4;
  else if (!bobgui_css_value_equal0 (value->values[BOBGUI_CSS_TOP], value->values[BOBGUI_CSS_BOTTOM]))
    n = 3;
  else if (!bobgui_css_value_equal0 (value->values[BOBGUI_CSS_TOP], value->values[BOBGUI_CSS_RIGHT]))
    n = 2;
  else
    n = 1;

  for (i = 0; i < n; i++)
    {
      if (i > 0)
        g_string_append_c (string, ' ');

      if (value->values[i] == NULL)
        g_string_append (string, "auto");
      else
        bobgui_css_value_print (value->values[i], string);
    }

  if (value->fill)
    g_string_append (string, " fill");
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_BORDER = {
  "BobguiCssBorderValue",
  bobgui_css_value_border_free,
  bobgui_css_value_border_compute,
  NULL,
  bobgui_css_value_border_equal,
  bobgui_css_value_border_transition,
  NULL,
  NULL,
  bobgui_css_value_border_print
};

BobguiCssValue *
_bobgui_css_border_value_new (BobguiCssValue *top,
                           BobguiCssValue *right,
                           BobguiCssValue *bottom,
                           BobguiCssValue *left)
{
  BobguiCssValue *result;

  result = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_BORDER);
  result->values[BOBGUI_CSS_TOP] = top;
  result->values[BOBGUI_CSS_RIGHT] = right;
  result->values[BOBGUI_CSS_BOTTOM] = bottom;
  result->values[BOBGUI_CSS_LEFT] = left;
  result->is_computed = (top && bobgui_css_value_is_computed (top)) &&
                        (right && bobgui_css_value_is_computed (right)) &&
                        (bottom && bobgui_css_value_is_computed (bottom)) &&
                        (left && bobgui_css_value_is_computed (left));

  return result;
}

BobguiCssValue *
_bobgui_css_border_value_parse (BobguiCssParser           *parser,
                             BobguiCssNumberParseFlags  flags,
                             gboolean                allow_auto,
                             gboolean                allow_fill)
{
  BobguiCssValue *result;
  guint i;

  result = _bobgui_css_border_value_new (NULL, NULL, NULL, NULL);

  if (allow_fill)
    result->fill = bobgui_css_parser_try_ident (parser, "fill");

  for (i = 0; i < 4; i++)
    {
      if (allow_auto && bobgui_css_parser_try_ident (parser, "auto"))
        continue;

      if (!bobgui_css_number_value_can_parse (parser))
        break;

      result->values[i] = bobgui_css_number_value_parse (parser, flags);
      if (result->values[i] == NULL)
        {
          bobgui_css_value_unref (result);
          return NULL;
        }
    }

  if (i == 0)
    {
      bobgui_css_parser_error_syntax (parser, "Expected a number");
      bobgui_css_value_unref (result);
      return NULL;
    }

  if (allow_fill && !result->fill)
    result->fill = bobgui_css_parser_try_ident (parser, "fill");

  for (; i < 4; i++)
    {
      if (result->values[(i - 1) >> 1])
        result->values[i] = bobgui_css_value_ref (result->values[(i - 1) >> 1]);
    }

  result->is_computed = TRUE;
  for (i = 0; i < 4; i++)
    if (result->values[i] && !bobgui_css_value_is_computed (result->values[i]))
      {
        result->is_computed = FALSE;
        break;
      }

  return result;
}

BobguiCssValue *
_bobgui_css_border_value_get_top (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_BORDER, NULL);

  return value->values[BOBGUI_CSS_TOP];
}

BobguiCssValue *
_bobgui_css_border_value_get_right (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_BORDER, NULL);

  return value->values[BOBGUI_CSS_RIGHT];
}

BobguiCssValue *
_bobgui_css_border_value_get_bottom (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_BORDER, NULL);

  return value->values[BOBGUI_CSS_BOTTOM];
}

BobguiCssValue *
_bobgui_css_border_value_get_left (const BobguiCssValue *value)
{
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_BORDER, NULL);

  return value->values[BOBGUI_CSS_LEFT];
}


