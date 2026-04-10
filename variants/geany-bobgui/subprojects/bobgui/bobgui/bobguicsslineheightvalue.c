/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2021 Red Hat, Inc.
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

#include "bobguicsslineheightvalueprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicssstyleprivate.h"

struct _BobguiCssValue {
  BOBGUI_CSS_VALUE_BASE
  BobguiCssValue *height;
};

static BobguiCssValue *default_line_height;

static BobguiCssValue *bobgui_css_value_line_height_new_empty (void);
static BobguiCssValue *bobgui_css_value_line_height_new       (BobguiCssValue *height);

static void
bobgui_css_value_line_height_free (BobguiCssValue *value)
{
  bobgui_css_value_unref (value->height);
  g_free (value);
}

static BobguiCssValue *
bobgui_css_value_line_height_compute (BobguiCssValue          *value,
                                   guint                 property_id,
                                   BobguiCssComputeContext *context)
{
  BobguiCssValue *height;

  height = bobgui_css_value_compute (value->height, property_id, context);

  if (bobgui_css_number_value_get_dimension (height) == BOBGUI_CSS_DIMENSION_PERCENTAGE)
    {
      double factor;
      BobguiCssValue *computed;

      factor = bobgui_css_number_value_get (height, 1);
      computed = bobgui_css_number_value_multiply (context->style->core->font_size, factor);

      bobgui_css_value_unref (height);

      return computed;
    }
  else
    {
      return height;
    }
}

static gboolean
bobgui_css_value_line_height_equal (const BobguiCssValue *value1,
                                 const BobguiCssValue *value2)
{
  if (value1->height == NULL || value2->height == NULL)
    return FALSE;

  return bobgui_css_value_equal (value1->height, value2->height);
}

static BobguiCssValue *
bobgui_css_value_line_height_transition (BobguiCssValue *start,
                                      BobguiCssValue *end,
                                      guint        property_id,
                                      double       progress)
{
  BobguiCssValue *height;

  if (start->height == NULL || end->height == NULL)
    return NULL;

  height = bobgui_css_value_transition (start->height, end->height, property_id, progress);
  if (height == NULL)
    return NULL;

  return bobgui_css_value_line_height_new (height);
}

static void
bobgui_css_value_line_height_print (const BobguiCssValue *value,
                                 GString           *string)
{
  if (value->height == NULL)
    g_string_append (string, "normal");
  else
    bobgui_css_value_print (value->height, string);
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_LINE_HEIGHT = {
  "BobguiCssLineHeightValue",
  bobgui_css_value_line_height_free,
  bobgui_css_value_line_height_compute,
  NULL,
  bobgui_css_value_line_height_equal,
  bobgui_css_value_line_height_transition,
  NULL,
  NULL,
  bobgui_css_value_line_height_print
};

static BobguiCssValue *
bobgui_css_value_line_height_new_empty (void)
{
  BobguiCssValue *result;

  result = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_LINE_HEIGHT);
  result->height = NULL;
  result->is_computed = TRUE;

  return result;
}

static BobguiCssValue *
bobgui_css_value_line_height_new (BobguiCssValue *height)
{
  BobguiCssValue *result;

  result = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_LINE_HEIGHT);
  result->height = height;

  return result;
}

BobguiCssValue *
bobgui_css_line_height_value_get_default (void)
{
  if (default_line_height == NULL)
    default_line_height = bobgui_css_value_line_height_new_empty ();

  return default_line_height;
}

BobguiCssValue *
bobgui_css_line_height_value_parse (BobguiCssParser *parser)
{
  BobguiCssValue *height;

  if (bobgui_css_parser_try_ident (parser, "normal"))
    return bobgui_css_value_ref (bobgui_css_line_height_value_get_default ());

  height = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_NUMBER |
                                               BOBGUI_CSS_PARSE_PERCENT |
                                               BOBGUI_CSS_PARSE_LENGTH |
                                               BOBGUI_CSS_POSITIVE_ONLY);
  if (!height)
    return NULL;

  return bobgui_css_value_line_height_new (height);
}

double
bobgui_css_line_height_value_get (const BobguiCssValue *value)
{
  if (value->class == &BOBGUI_CSS_VALUE_LINE_HEIGHT)
    return 0.0;

  return bobgui_css_number_value_get (value, 1);
}
