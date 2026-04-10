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

#include "bobguicssarrayvalueprivate.h"
#include "bobguicssimagevalueprivate.h"
#include "bobguicssstylepropertyprivate.h"

#include <string.h>

struct _BobguiCssValue {
  BOBGUI_CSS_VALUE_BASE
  guint         n_values;
  BobguiCssValue  *values[1];
};

static void
bobgui_css_value_array_free (BobguiCssValue *value)
{
  guint i;

  for (i = 0; i < value->n_values; i++)
    bobgui_css_value_unref (value->values[i]);

  g_free (value);
}

static BobguiCssValue *
bobgui_css_value_array_compute (BobguiCssValue          *value,
                             guint                 property_id,
                             BobguiCssComputeContext *context)
{
  BobguiCssValue *result;
  BobguiCssValue *i_value;
  guint i, j;
  gboolean contains_current_color = FALSE;;

  result = NULL;
  for (i = 0; i < value->n_values; i++)
    {
      i_value =  bobgui_css_value_compute (value->values[i], property_id, context);
      contains_current_color |= bobgui_css_value_contains_current_color (i_value);

      if (result == NULL &&
          i_value != value->values[i])
        {
          result = _bobgui_css_array_value_new_from_array (value->values, value->n_values);
          for (j = 0; j < i; j++)
            bobgui_css_value_ref (result->values[j]);
        }

      if (result != NULL)
        result->values[i] = i_value;
      else
        bobgui_css_value_unref (i_value);
    }

  if (result == NULL)
    return bobgui_css_value_ref (value);

  result->is_computed = TRUE;
  result->contains_current_color = contains_current_color;

  return result;
}

static BobguiCssValue *
bobgui_css_value_array_resolve (BobguiCssValue          *value,
                             BobguiCssComputeContext *context,
                             BobguiCssValue          *current)
{
  BobguiCssValue *result;
  BobguiCssValue *i_value;
  guint i, j;

  result = NULL;
  for (i = 0; i < value->n_values; i++)
    {
      i_value =  bobgui_css_value_resolve (value->values[i], context, current);

      if (result == NULL &&
          i_value != value->values[i])
        {
          result = _bobgui_css_array_value_new_from_array (value->values, value->n_values);
          for (j = 0; j < i; j++)
            bobgui_css_value_ref (result->values[j]);
        }

      if (result != NULL)
        result->values[i] = i_value;
      else
        bobgui_css_value_unref (i_value);
    }

  if (result == NULL)
    return bobgui_css_value_ref (value);

  result->is_computed = TRUE;
  result->contains_current_color = FALSE;

  return result;
}

static gboolean
bobgui_css_value_array_equal (const BobguiCssValue *value1,
                           const BobguiCssValue *value2)
{
  guint i;

  if (value1->n_values != value2->n_values)
    return FALSE;

  for (i = 0; i < value1->n_values; i++)
    {
      if (!bobgui_css_value_equal (value1->values[i],
                                value2->values[i]))
        return FALSE;
    }

  return TRUE;
}

static inline guint
gcd (guint a, guint b)
{
  while (b != 0)
    {
      guint t = b;
      b = a % b;
      a = t;
    }
  return a;
}

static inline guint
lcm (guint a, guint b)
{
  return a / gcd (a, b) * b;
}

static BobguiCssValue *
bobgui_css_value_array_transition_repeat (BobguiCssValue *start,
                                       BobguiCssValue *end,
                                       guint        property_id,
                                       double       progress)
{
  BobguiCssValue **transitions;
  guint i, n;

  n = lcm (start->n_values, end->n_values);
  transitions = g_newa (BobguiCssValue *, n);

  for (i = 0; i < n; i++)
    {
      transitions[i] = bobgui_css_value_transition (start->values[i % start->n_values],
                                                 end->values[i % end->n_values],
                                                 property_id,
                                                 progress);
      if (transitions[i] == NULL)
        {
          while (i--)
            bobgui_css_value_unref (transitions[i]);
          return NULL;
        }
    }

  return _bobgui_css_array_value_new_from_array (transitions, n);
}

static BobguiCssValue *
bobgui_css_array_value_create_default_transition_value (guint property_id)
{
  switch (property_id)
    {
    case BOBGUI_CSS_PROPERTY_BACKGROUND_IMAGE:
      return _bobgui_css_image_value_new (NULL);
    default:
      g_return_val_if_reached (NULL);
    }
}

static BobguiCssValue *
bobgui_css_value_array_transition_extend (BobguiCssValue *start,
                                       BobguiCssValue *end,
                                       guint        property_id,
                                       double       progress)
{
  BobguiCssValue **transitions;
  guint i, n;

  n = MAX (start->n_values, end->n_values);
  transitions = g_newa (BobguiCssValue *, n);

  for (i = 0; i < MIN (start->n_values, end->n_values); i++)
    {
      transitions[i] = bobgui_css_value_transition (start->values[i],
                                                 end->values[i],
                                                 property_id,
                                                 progress);
      if (transitions[i] == NULL)
        {
          while (i--)
            bobgui_css_value_unref (transitions[i]);
          return NULL;
        }
    }

  if (start->n_values != end->n_values)
    {
      BobguiCssValue *default_value;

      default_value = bobgui_css_array_value_create_default_transition_value (property_id);

      for (; i < start->n_values; i++)
        {
          transitions[i] = bobgui_css_value_transition (start->values[i],
                                                     default_value,
                                                     property_id,
                                                     progress);
          if (transitions[i] == NULL)
            {
              while (i--)
                bobgui_css_value_unref (transitions[i]);
              return NULL;
            }
        }

      for (; i < end->n_values; i++)
        {
          transitions[i] = bobgui_css_value_transition (default_value,
                                                     end->values[i],
                                                     property_id,
                                                     progress);
          if (transitions[i] == NULL)
            {
              while (i--)
                bobgui_css_value_unref (transitions[i]);
              return NULL;
            }
        }

    }

  g_assert (i == n);

  return _bobgui_css_array_value_new_from_array (transitions, n);
}

static BobguiCssValue *
bobgui_css_value_array_transition (BobguiCssValue *start,
                                BobguiCssValue *end,
                                guint        property_id,
                                double       progress)
{
  switch (property_id)
    {
    case BOBGUI_CSS_PROPERTY_BACKGROUND_CLIP:
    case BOBGUI_CSS_PROPERTY_BACKGROUND_ORIGIN:
    case BOBGUI_CSS_PROPERTY_BACKGROUND_SIZE:
    case BOBGUI_CSS_PROPERTY_BACKGROUND_POSITION:
    case BOBGUI_CSS_PROPERTY_BACKGROUND_REPEAT:
      return bobgui_css_value_array_transition_repeat (start, end, property_id, progress);
    case BOBGUI_CSS_PROPERTY_BACKGROUND_IMAGE:
      return bobgui_css_value_array_transition_extend (start, end, property_id, progress);
    case BOBGUI_CSS_PROPERTY_COLOR:
    case BOBGUI_CSS_PROPERTY_FONT_SIZE:
    case BOBGUI_CSS_PROPERTY_BACKGROUND_COLOR:
    case BOBGUI_CSS_PROPERTY_FONT_FAMILY:
    case BOBGUI_CSS_PROPERTY_FONT_STYLE:
    case BOBGUI_CSS_PROPERTY_FONT_WEIGHT:
    case BOBGUI_CSS_PROPERTY_TEXT_SHADOW:
    case BOBGUI_CSS_PROPERTY_ICON_SHADOW:
    case BOBGUI_CSS_PROPERTY_BOX_SHADOW:
    case BOBGUI_CSS_PROPERTY_MARGIN_TOP:
    case BOBGUI_CSS_PROPERTY_MARGIN_LEFT:
    case BOBGUI_CSS_PROPERTY_MARGIN_BOTTOM:
    case BOBGUI_CSS_PROPERTY_MARGIN_RIGHT:
    case BOBGUI_CSS_PROPERTY_PADDING_TOP:
    case BOBGUI_CSS_PROPERTY_PADDING_LEFT:
    case BOBGUI_CSS_PROPERTY_PADDING_BOTTOM:
    case BOBGUI_CSS_PROPERTY_PADDING_RIGHT:
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_STYLE:
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_WIDTH:
    case BOBGUI_CSS_PROPERTY_BORDER_LEFT_STYLE:
    case BOBGUI_CSS_PROPERTY_BORDER_LEFT_WIDTH:
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_STYLE:
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_WIDTH:
    case BOBGUI_CSS_PROPERTY_BORDER_RIGHT_STYLE:
    case BOBGUI_CSS_PROPERTY_BORDER_RIGHT_WIDTH:
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_LEFT_RADIUS:
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_RIGHT_RADIUS:
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_RIGHT_RADIUS:
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_LEFT_RADIUS:
    case BOBGUI_CSS_PROPERTY_OUTLINE_STYLE:
    case BOBGUI_CSS_PROPERTY_OUTLINE_WIDTH:
    case BOBGUI_CSS_PROPERTY_OUTLINE_OFFSET:
    case BOBGUI_CSS_PROPERTY_BORDER_TOP_COLOR:
    case BOBGUI_CSS_PROPERTY_BORDER_RIGHT_COLOR:
    case BOBGUI_CSS_PROPERTY_BORDER_BOTTOM_COLOR:
    case BOBGUI_CSS_PROPERTY_BORDER_LEFT_COLOR:
    case BOBGUI_CSS_PROPERTY_OUTLINE_COLOR:
    case BOBGUI_CSS_PROPERTY_BORDER_IMAGE_SOURCE:
    case BOBGUI_CSS_PROPERTY_BORDER_IMAGE_REPEAT:
    case BOBGUI_CSS_PROPERTY_BORDER_IMAGE_SLICE:
    case BOBGUI_CSS_PROPERTY_BORDER_IMAGE_WIDTH:
    default:
      /* keep all values that are not arrays here, so we get a warning if we ever turn them
       * into arrays and start animating them. */
      g_warning ("Don't know how to transition arrays for property '%s'", 
                 _bobgui_style_property_get_name (BOBGUI_STYLE_PROPERTY (_bobgui_css_style_property_lookup_by_id (property_id))));
      return NULL;
    case BOBGUI_CSS_PROPERTY_TRANSITION_PROPERTY:
    case BOBGUI_CSS_PROPERTY_TRANSITION_DURATION:
    case BOBGUI_CSS_PROPERTY_TRANSITION_TIMING_FUNCTION:
    case BOBGUI_CSS_PROPERTY_TRANSITION_DELAY:
      return NULL;
    }
}

static gboolean
bobgui_css_value_array_is_dynamic (const BobguiCssValue *value)
{
  guint i;

  for (i = 0; i < value->n_values; i++)
    {
      if (bobgui_css_value_is_dynamic (value->values[i]))
        return TRUE;
    }

  return FALSE;
}

static BobguiCssValue *
bobgui_css_value_array_get_dynamic_value (BobguiCssValue *value,
                                       gint64       monotonic_time)
{
  BobguiCssValue *result;
  BobguiCssValue *i_value;
  guint i, j;

  if (!bobgui_css_value_is_dynamic (value))
    return bobgui_css_value_ref (value);

  result = NULL;
  for (i = 0; i < value->n_values; i++)
    {
      i_value = bobgui_css_value_get_dynamic_value (value->values[i], monotonic_time);

      if (result == NULL &&
	  i_value != value->values[i])
	{
	  result = _bobgui_css_array_value_new_from_array (value->values, value->n_values);
	  for (j = 0; j < i; j++)
	    bobgui_css_value_ref (result->values[j]);
	}

      if (result != NULL)
	result->values[i] = i_value;
      else
	bobgui_css_value_unref (i_value);
    }

  if (result == NULL)
    return bobgui_css_value_ref (value);

  return result;
}

static void
bobgui_css_value_array_print (const BobguiCssValue *value,
                           GString           *string)
{
  guint i;

  if (value->n_values == 0)
    {
      g_string_append (string, "none");
      return;
    }

  for (i = 0; i < value->n_values; i++)
    {
      if (i > 0)
        g_string_append (string, ", ");
      bobgui_css_value_print (value->values[i], string);
    }
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_ARRAY = {
  "BobguiCssArrayValue",
  bobgui_css_value_array_free,
  bobgui_css_value_array_compute,
  bobgui_css_value_array_resolve,
  bobgui_css_value_array_equal,
  bobgui_css_value_array_transition,
  bobgui_css_value_array_is_dynamic,
  bobgui_css_value_array_get_dynamic_value,
  bobgui_css_value_array_print
};

BobguiCssValue *
_bobgui_css_array_value_new (BobguiCssValue *content)
{
  g_return_val_if_fail (content != NULL, NULL);

  return _bobgui_css_array_value_new_from_array (&content, 1);
}

BobguiCssValue *
_bobgui_css_array_value_new_from_array (BobguiCssValue **values,
                                     guint         n_values)
{
  BobguiCssValue *result;
  guint i;

  g_return_val_if_fail (values != NULL, NULL);
  g_return_val_if_fail (n_values > 0, NULL);

  if (n_values == 1)
    return values[0];

  result = bobgui_css_value_alloc (&BOBGUI_CSS_VALUE_ARRAY, sizeof (BobguiCssValue) + sizeof (BobguiCssValue *) * (n_values - 1));
  result->n_values = n_values;
  memcpy (&result->values[0], values, sizeof (BobguiCssValue *) * n_values);

  result->is_computed = TRUE;
  result->contains_variables = FALSE;
  result->contains_current_color = FALSE;
  for (i = 0; i < n_values; i ++)
    {
      if (!bobgui_css_value_is_computed (values[i]))
        result->is_computed = FALSE;

      if (bobgui_css_value_contains_variables (values[i]))
        result->contains_variables = TRUE;

      if (bobgui_css_value_contains_current_color (values[i]))
        result->contains_current_color = TRUE;

      if (!result->is_computed && result->contains_variables && result->contains_current_color)
        break;
    }

  return result;
}

BobguiCssValue *
_bobgui_css_array_value_parse (BobguiCssParser *parser,
                            BobguiCssValue  *(* parse_func) (BobguiCssParser *parser))
{
  BobguiCssValue *value, *result;
  BobguiCssValue *values[128];
  guint n_values = 0;
  guint i;

  do {
    value = parse_func (parser);

    if (value == NULL)
      {
        for (i = 0; i < n_values; i ++)
          bobgui_css_value_unref (values[i]);

        return NULL;
      }

    values[n_values] = value;
    n_values ++;
    if (G_UNLIKELY (n_values > G_N_ELEMENTS (values)))
      g_error ("Only %d elements in a css array are allowed", (int)G_N_ELEMENTS (values));
  } while (bobgui_css_parser_try_token (parser, BOBGUI_CSS_TOKEN_COMMA));

  result = _bobgui_css_array_value_new_from_array (values, n_values);
  return result;
}

BobguiCssValue *
_bobgui_css_array_value_get_nth (BobguiCssValue *value,
                              guint        i)
{
  if (value->class != &BOBGUI_CSS_VALUE_ARRAY)
      return value;

  g_return_val_if_fail (value != NULL, NULL);
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_ARRAY, NULL);
  g_return_val_if_fail (value->n_values > 0, NULL);

  return value->values[i % value->n_values];
}

guint
_bobgui_css_array_value_get_n_values (const BobguiCssValue *value)
{
  if (value->class != &BOBGUI_CSS_VALUE_ARRAY)
    return 1;

  g_return_val_if_fail (value != NULL, 0);
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_ARRAY, 0);

  return value->n_values;
}
