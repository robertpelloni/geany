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

#include "bobguicssstringvalueprivate.h"
#include "bobgui/css/bobguicssserializerprivate.h"

#include <string.h>

struct _BobguiCssValue {
  BOBGUI_CSS_VALUE_BASE
  char *string;
};

static void
bobgui_css_value_string_free (BobguiCssValue *value)
{
  g_free (value->string);
  g_free (value);
}

static BobguiCssValue *
bobgui_css_value_string_compute (BobguiCssValue          *value,
                              guint                 property_id,
                              BobguiCssComputeContext *context)
{
  return bobgui_css_value_ref (value);
}

static gboolean
bobgui_css_value_string_equal (const BobguiCssValue *value1,
                            const BobguiCssValue *value2)
{
  return g_strcmp0 (value1->string, value2->string) == 0;
}

static BobguiCssValue *
bobgui_css_value_string_transition (BobguiCssValue *start,
                                 BobguiCssValue *end,
                                 guint        property_id,
                                 double       progress)
{
  return NULL;
}

static void
bobgui_css_value_string_print (const BobguiCssValue *value,
                            GString           *str)
{
  if (value->string == NULL)
    {
      g_string_append (str, "none");
      return;
    }

  bobgui_css_print_string (str, value->string, FALSE);
}

static void
bobgui_css_value_ident_print (const BobguiCssValue *value,
                            GString           *str)
{
  char *string = value->string;
  gsize len;

  do {
    len = strcspn (string, "\"\n\r\f");
    g_string_append_len (str, string, len);
    string += len;
    switch (*string)
      {
      case '\0':
        goto out;
      case '\n':
        g_string_append (str, "\\A ");
        break;
      case '\r':
        g_string_append (str, "\\D ");
        break;
      case '\f':
        g_string_append (str, "\\C ");
        break;
      case '\"':
        g_string_append (str, "\\\"");
        break;
      case '\'':
        g_string_append (str, "\\'");
        break;
      case '\\':
        g_string_append (str, "\\\\");
        break;
      default:
        g_assert_not_reached ();
        break;
      }
    string++;
  } while (*string);

out:
  ;
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_STRING = {
  "BobguiCssStringValue",
  bobgui_css_value_string_free,
  bobgui_css_value_string_compute,
  NULL,
  bobgui_css_value_string_equal,
  bobgui_css_value_string_transition,
  NULL,
  NULL,
  bobgui_css_value_string_print
};

static const BobguiCssValueClass BOBGUI_CSS_VALUE_IDENT = {
  "BobguiCssIdentValue",
  bobgui_css_value_string_free,
  bobgui_css_value_string_compute,
  NULL,
  bobgui_css_value_string_equal,
  bobgui_css_value_string_transition,
  NULL,
  NULL,
  bobgui_css_value_ident_print
};

BobguiCssValue *
_bobgui_css_string_value_new (const char *string)
{
  return _bobgui_css_string_value_new_take (g_strdup (string));
}

BobguiCssValue *
_bobgui_css_string_value_new_take (char *string)
{
  BobguiCssValue *result;

  result = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_STRING);
  result->string = string;
  result->is_computed = TRUE;

  return result;
}

BobguiCssValue *
_bobgui_css_string_value_parse (BobguiCssParser *parser)
{
  char *s;

  g_return_val_if_fail (parser != NULL, NULL);

  s = bobgui_css_parser_consume_string (parser);
  if (s == NULL)
    return NULL;
  
  return _bobgui_css_string_value_new_take (s);
}

const char *
_bobgui_css_string_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value != NULL, NULL);
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_STRING, NULL);

  return value->string;
}

BobguiCssValue *
_bobgui_css_ident_value_new (const char *ident)
{
  return _bobgui_css_ident_value_new_take (g_strdup (ident));
}

BobguiCssValue *
_bobgui_css_ident_value_new_take (char *ident)
{
  BobguiCssValue *result;

  result = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_IDENT);
  result->string = ident;
  result->is_computed = TRUE;

  return result;
}

BobguiCssValue *
_bobgui_css_ident_value_try_parse (BobguiCssParser *parser)
{
  char *ident;

  g_return_val_if_fail (parser != NULL, NULL);

  if (!bobgui_css_parser_has_token (parser, BOBGUI_CSS_TOKEN_IDENT))
    return NULL;

  ident = bobgui_css_parser_consume_ident (parser);
  if (ident == NULL)
    {
      g_assert_not_reached ();
    }
  
  return _bobgui_css_ident_value_new_take (ident);
}

const char *
_bobgui_css_ident_value_get (const BobguiCssValue *value)
{
  g_return_val_if_fail (value != NULL, NULL);
  g_return_val_if_fail (value->class == &BOBGUI_CSS_VALUE_IDENT, NULL);

  return value->string;
}


