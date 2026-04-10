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

#include "bobguicsspalettevalueprivate.h"

#include "bobguicsscolorvalueprivate.h"
#include "bobguicsscolorvalueprivate.h"
#include "bobguiprivate.h"

struct _BobguiCssValue {
  BOBGUI_CSS_VALUE_BASE
  guint n_colors;
  char **color_names;
  BobguiCssValue **color_values;
};

static BobguiCssValue *default_palette;

static BobguiCssValue *bobgui_css_palette_value_new_empty (void);
static BobguiCssValue *bobgui_css_palette_value_new_sized (guint size);

static void
bobgui_css_palette_value_set_color (BobguiCssValue *value,
                                 guint        i,
                                 char        *name,
                                 BobguiCssValue *color)
{
  value->color_names[i] = name; /* No strdup */
  value->color_values[i] = color;
}

static void
bobgui_css_palette_value_sort_colors (BobguiCssValue *value)
{
  guint i, j;

  /* Bubble sort. We're mostly talking about 3 elements here. */
  for (i = 0; i < value->n_colors; i ++)
    for (j = 0; j < value->n_colors; j ++)
      {
        if (strcmp (value->color_names[i], value->color_names[j]) < 0)
          {
            char *tmp_name;
            BobguiCssValue *tmp_value;

            tmp_name = value->color_names[i];
            tmp_value = value->color_values[i];

            value->color_names[i] = value->color_names[j];
            value->color_values[i] = value->color_values[j];

            value->color_names[j] = tmp_name;
            value->color_values[j] = tmp_value;
          }
      }
}

static BobguiCssValue *
bobgui_css_palette_value_find_color (BobguiCssValue *value,
                                  const char  *color_name)
{
  guint i;

  for (i = 0; i < value->n_colors; i ++)
    {
      if (strcmp (value->color_names[i], color_name) == 0)
        return value->color_values[i];
    }

  return NULL;
}

static void
bobgui_css_value_palette_free (BobguiCssValue *value)
{
  guint i;

  for (i = 0; i < value->n_colors; i ++)
    {
      g_free (value->color_names[i]);
      bobgui_css_value_unref (value->color_values[i]);
    }

  g_free (value->color_names);
  g_free (value->color_values);
  g_free (value);
}

static BobguiCssValue *
bobgui_css_value_palette_compute (BobguiCssValue          *specified,
                               guint                 property_id,
                               BobguiCssComputeContext *context)
{
  BobguiCssValue *computed_color;
  BobguiCssValue *result;
  gboolean changes = FALSE;
  guint i;

  result = bobgui_css_palette_value_new_sized (specified->n_colors);

  for (i = 0; i < specified->n_colors; i ++)
    {
      BobguiCssValue *value = specified->color_values[i];

      computed_color = bobgui_css_value_compute (value, property_id, context);
      result->color_names[i] = g_strdup (specified->color_names[i]);
      result->color_values[i] = computed_color;

      changes |= computed_color != value;
    }

  if (!changes)
    {
      bobgui_css_value_unref (result);
      result = bobgui_css_value_ref (specified);
    }

  return result;
}

static BobguiCssValue *
bobgui_css_value_palette_resolve (BobguiCssValue          *value,
                               BobguiCssComputeContext *context,
                               BobguiCssValue          *current_color)
{
  BobguiCssValue *result;

  if (!bobgui_css_value_contains_current_color (value))
    return bobgui_css_value_ref (value);

  result = bobgui_css_palette_value_new_sized (value->n_colors);
  for (guint i = 0; i < value->n_colors; i++)
    {
      result->color_names[i] = g_strdup (value->color_names[i]);
      result->color_values[i] = bobgui_css_value_resolve (value->color_values[i], context, current_color);
    }

  return result;
}

static gboolean
bobgui_css_value_palette_equal (const BobguiCssValue *value1,
                             const BobguiCssValue *value2)
{
  guint i;

  if (value1->n_colors != value2->n_colors)
    return FALSE;

  for (i = 0; i < value1->n_colors; i ++)
    {
      if (strcmp (value1->color_names[i], value2->color_names[i]) != 0)
        return FALSE;

      if (!bobgui_css_value_equal (value1->color_values[i], value2->color_values[i]))
        return FALSE;
    }

  return TRUE;
}

static BobguiCssValue *
bobgui_css_value_palette_transition (BobguiCssValue *start,
                                  BobguiCssValue *end,
                                  guint        property_id,
                                  double       progress)
{
  BobguiCssValue *result, *transition;
  BobguiCssValue *start_color, *end_color;
  const char *name;
  guint i;
  GPtrArray *new_names;
  GPtrArray *new_values;

  /* XXX: For colors that are only in start or end but not both,
   * we don't transition but just keep the value.
   * That causes an abrupt transition to currentColor at the end.
   */

  result = bobgui_css_palette_value_new_empty ();
  new_names = g_ptr_array_new ();
  new_values = g_ptr_array_new ();

  for (i = 0; i < start->n_colors; i ++)
    {
      name = start->color_names[i];
      start_color = start->color_values[i];
      end_color = bobgui_css_palette_value_find_color (end, name);

      if (end_color == NULL)
        transition = bobgui_css_value_ref (start_color);
      else
        transition = bobgui_css_value_transition (start_color, end_color, property_id, progress);

      g_ptr_array_add (new_names, g_strdup (name));
      g_ptr_array_add (new_values, transition);
    }

  for (i = 0; i < end->n_colors; i ++)
    {
      name = end->color_names[i];
      end_color = end->color_values[i];
      start_color = bobgui_css_palette_value_find_color (start, name);

      if (start_color != NULL)
        continue;

      g_ptr_array_add (new_names, g_strdup (name));
      g_ptr_array_add (new_values, bobgui_css_value_ref (end_color));
    }

  result->n_colors = new_names->len;
  result->color_names = (char **)g_ptr_array_free (new_names, FALSE);
  result->color_values = (BobguiCssValue **)g_ptr_array_free (new_values, FALSE);
  bobgui_css_palette_value_sort_colors (result);

  return result;
}

static void
bobgui_css_value_palette_print (const BobguiCssValue *value,
                             GString           *string)
{
  gboolean first = TRUE;
  guint i;

  if (value == default_palette)
    {
      g_string_append (string, "default");
      return;
    }

  for (i = 0; i < value->n_colors; i ++)
    {
      if (first)
        first = FALSE;
      else
        g_string_append (string, ", ");

      g_string_append (string, value->color_names[i]);
      g_string_append_c (string, ' ');
      bobgui_css_value_print (value->color_values[i], string);
    }
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_PALETTE = {
  "BobguiCssPaletteValue",
  bobgui_css_value_palette_free,
  bobgui_css_value_palette_compute,
  bobgui_css_value_palette_resolve,
  bobgui_css_value_palette_equal,
  bobgui_css_value_palette_transition,
  NULL,
  NULL,
  bobgui_css_value_palette_print
};

static BobguiCssValue *
bobgui_css_palette_value_new_empty (void)
{
  BobguiCssValue *result;

  result = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_PALETTE);

  return result;
}

static BobguiCssValue *
bobgui_css_palette_value_new_sized (guint size)
{
  BobguiCssValue *result;

  result = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_PALETTE);
  result->n_colors = size;
  result->color_names = g_malloc (sizeof (char *) * size);
  result->color_values = g_malloc (sizeof (BobguiCssValue *) * size);

  return result;
}

BobguiCssValue *
bobgui_css_palette_value_new_default (void)
{
  if (default_palette == NULL)
    {
      default_palette = bobgui_css_palette_value_new_sized (4);
      bobgui_css_palette_value_set_color (default_palette, 0, g_strdup ("error"),
                                       bobgui_css_color_value_new_name ("error_color"));
      bobgui_css_palette_value_set_color (default_palette, 1, g_strdup ("success"),
                                       bobgui_css_color_value_new_name ("success_color"));
      bobgui_css_palette_value_set_color (default_palette, 2, g_strdup ("warning"),
                                       bobgui_css_color_value_new_name ("warning_color"));
      bobgui_css_palette_value_set_color (default_palette, 3, g_strdup ("accent"),
                                       bobgui_css_color_value_new_name ("accent_color"));
      /* Above is already sorted */
    }

  return bobgui_css_value_ref (default_palette);
}

BobguiCssValue *
bobgui_css_palette_value_parse (BobguiCssParser *parser)
{
  BobguiCssValue *result, *color;
  GPtrArray *names;
  GPtrArray *colors;
  char *ident;

  if (bobgui_css_parser_try_ident (parser, "default"))
    return bobgui_css_palette_value_new_default ();

  result = bobgui_css_palette_value_new_empty ();
  names = g_ptr_array_new ();
  colors = g_ptr_array_new ();

  do {
    ident = bobgui_css_parser_consume_ident (parser);
    if (ident == NULL)
      {
        bobgui_css_value_unref (result);
        return NULL;
      }

    color = bobgui_css_color_value_parse (parser);
    if (color == NULL)
      {
        g_free (ident);
        bobgui_css_value_unref (result);
        return NULL;
      }

    result->is_computed = result->is_computed && bobgui_css_value_is_computed (color);
    result->contains_current_color = result->contains_current_color || bobgui_css_value_contains_current_color (color);

    g_ptr_array_add (names, ident);
    g_ptr_array_add (colors, color);
  } while (bobgui_css_parser_try_token (parser, BOBGUI_CSS_TOKEN_COMMA));

  result->n_colors = names->len;
  result->color_names = (char **)g_ptr_array_free (names, FALSE);
  result->color_values = (BobguiCssValue **) g_ptr_array_free (colors, FALSE);
  bobgui_css_palette_value_sort_colors (result);

  return result;
}

BobguiCssValue *
bobgui_css_palette_value_get_color (BobguiCssValue *value,
                                 const char  *name)
{
  guint i;

  bobgui_internal_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_PALETTE, NULL);

  for (i = 0; i < value->n_colors; i ++)
    {
      if (strcmp (value->color_names[i], name) == 0)
        return value->color_values[i];
    }

  return NULL;
}
