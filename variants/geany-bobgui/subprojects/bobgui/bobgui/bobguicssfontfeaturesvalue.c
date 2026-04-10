/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2017 Red Hat, Inc.
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
 *
 * Author: Matthias Clasen
 */

#include "config.h"

#include "bobguicsstypesprivate.h"
#include <bobgui/css/bobguicss.h>
#include "bobgui/css/bobguicsstokenizerprivate.h"
#include "bobgui/css/bobguicssparserprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicssfontfeaturesvalueprivate.h"

struct _BobguiCssValue {
  BOBGUI_CSS_VALUE_BASE
  GHashTable *features;
};

static BobguiCssValue *default_font_features;

static BobguiCssValue *bobgui_css_font_features_value_new_empty (void);

static void
bobgui_css_font_features_value_add_feature (BobguiCssValue *value,
                                         const char  *name,
                                         int          num)
{
  g_hash_table_insert (value->features, g_strdup (name), GINT_TO_POINTER (num));
}


static void
bobgui_css_value_font_features_free (BobguiCssValue *value)
{
  g_hash_table_unref (value->features);

  g_free (value);
}

static BobguiCssValue *
bobgui_css_value_font_features_compute (BobguiCssValue          *specified,
                                     guint                 property_id,
                                     BobguiCssComputeContext *context)
{
  return bobgui_css_value_ref (specified);
}

static gboolean
bobgui_css_value_font_features_equal (const BobguiCssValue *value1,
                                   const BobguiCssValue *value2)
{
  gpointer name, val1, val2;
  GHashTableIter iter;

  if (g_hash_table_size (value1->features) != g_hash_table_size (value2->features))
    return FALSE;

  g_hash_table_iter_init (&iter, value1->features);
  while (g_hash_table_iter_next (&iter, &name, &val1))
    {
      if (!g_hash_table_lookup_extended (value2->features, name, NULL, &val2))
        return FALSE;

      if (val1 != val2)
        return FALSE;
    }

  return TRUE;
}

static BobguiCssValue *
bobgui_css_value_font_features_transition (BobguiCssValue *start,
                                        BobguiCssValue *end,
                                        guint        property_id,
                                        double       progress)
{
  const char *name;
  gpointer start_val, end_val;
  GHashTableIter iter;
  gpointer transition;
  BobguiCssValue *result;

  /* XXX: For value that are only in start or end but not both,
   * we don't transition but just keep the value.
   * That causes an abrupt transition to the new value at the end.
   */

  result = bobgui_css_font_features_value_new_empty ();

  g_hash_table_iter_init (&iter, start->features);
  while (g_hash_table_iter_next (&iter, (gpointer *)&name, (gpointer *)&start_val))
    {
      if (!g_hash_table_lookup_extended (end->features, name, NULL, &end_val))
        transition = start_val;
      else
        transition = progress < 0.5 ? start_val : end_val;

      bobgui_css_font_features_value_add_feature (result, name, GPOINTER_TO_INT (transition));
    }

  g_hash_table_iter_init (&iter, end->features);
  while (g_hash_table_iter_next (&iter, (gpointer *)&name, (gpointer *)&end_val))
    {
      if (g_hash_table_lookup_extended (end->features, name, NULL, &start_val))
        continue;

      bobgui_css_font_features_value_add_feature (result, name, GPOINTER_TO_INT (end_val));
    }

  return result;
}

static void
bobgui_css_value_font_features_print (const BobguiCssValue *value,
                                   GString           *string)
{
  GHashTableIter iter;
  const char *name;
  gpointer val;
  gboolean first = TRUE;

  if (value == default_font_features)
    {
      g_string_append (string, "normal");
      return;
    }

  g_hash_table_iter_init (&iter, value->features);
  while (g_hash_table_iter_next (&iter, (gpointer *)&name, (gpointer *)&val))
    {
      if (first)
        first = FALSE;
      else
        g_string_append (string, ", ");
      g_string_append_printf (string, "\"%s\" ", name);
      g_string_append_printf (string, "%d", GPOINTER_TO_INT (val));
    }
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_FONT_FEATURES = {
  "BobguiCssFontFeaturesValue",
  bobgui_css_value_font_features_free,
  bobgui_css_value_font_features_compute,
  NULL,
  bobgui_css_value_font_features_equal,
  bobgui_css_value_font_features_transition,
  NULL,
  NULL,
  bobgui_css_value_font_features_print
};

static BobguiCssValue *
bobgui_css_font_features_value_new_empty (void)
{
  BobguiCssValue *result;

  result = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_FONT_FEATURES);
  result->features = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  result->is_computed = TRUE;

  return result;
}

BobguiCssValue *
bobgui_css_font_features_value_new_default (void)
{
  if (default_font_features == NULL)
    default_font_features = bobgui_css_font_features_value_new_empty ();

  return bobgui_css_value_ref (default_font_features);
}

static gboolean
is_valid_opentype_tag (const char *s)
{
  if (strlen (s) != 4)
    return FALSE;

  if (s[0] < 0x20 || s[0] > 0x7e ||
      s[1] < 0x20 || s[1] > 0x7e ||
      s[2] < 0x20 || s[2] > 0x7e ||
      s[3] < 0x20 || s[3] > 0x7e)
    return FALSE;

  return TRUE;
}

BobguiCssValue *
bobgui_css_font_features_value_parse (BobguiCssParser *parser)
{
  BobguiCssValue *result;
  char *name;
  int num;

  if (bobgui_css_parser_try_ident (parser, "normal"))
    return bobgui_css_font_features_value_new_default ();

  result = bobgui_css_font_features_value_new_empty ();

  do {
    name = bobgui_css_parser_consume_string (parser);
    if (name == NULL)
      {
        bobgui_css_value_unref (result);
        return NULL;
      }

    if (!is_valid_opentype_tag (name))
      {
        bobgui_css_parser_error_value (parser, "Not a valid OpenType tag.");
        g_free (name);
        bobgui_css_value_unref (result);
        return NULL;
      }

    if (bobgui_css_parser_try_ident (parser, "on"))
      num = 1;
    else if (bobgui_css_parser_try_ident (parser, "off"))
      num = 0;
    else if (bobgui_css_parser_has_integer (parser))
      {
        if (!bobgui_css_parser_consume_integer (parser, &num))
          {
            g_free (name);
            bobgui_css_value_unref (result);
            return NULL;
          }
      }
    else
      num = 1;

    bobgui_css_font_features_value_add_feature (result, name, num);
    g_free (name);
  } while (bobgui_css_parser_try_token (parser, BOBGUI_CSS_TOKEN_COMMA));

  return result;
}

char *
bobgui_css_font_features_value_get_features (BobguiCssValue *value)
{
  BobguiCssValue *val;
  GHashTableIter iter;
  gboolean first = TRUE;
  const char *name;
  GString *string;

  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_FONT_FEATURES, NULL);

  if (value == default_font_features)
    return NULL;

  string = g_string_new ("");

  g_hash_table_iter_init (&iter, value->features);
  while (g_hash_table_iter_next (&iter, (gpointer *)&name, (gpointer *)&val))
    {
      if (first)
        first = FALSE;
      else
        g_string_append (string, ", ");
      g_string_append_printf (string, "%s %d", name, GPOINTER_TO_INT (val));
    }

  return g_string_free (string, FALSE);
}
