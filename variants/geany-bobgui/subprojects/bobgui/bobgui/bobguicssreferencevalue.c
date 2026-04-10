/*
 * Copyright (C) 2023 GNOME Foundation Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Alice Mikhaylenko <alicem@gnome.org>
 */

#include "config.h"

#include "bobguicssreferencevalueprivate.h"

#include "bobguicssarrayvalueprivate.h"
#include "bobguicsscustompropertypoolprivate.h"
#include "bobguicssshorthandpropertyprivate.h"
#include "bobguicssstyleprivate.h"
#include "bobguicssunsetvalueprivate.h"
#include "bobguicssvalueprivate.h"
#include "bobguistyleproviderprivate.h"

#define GDK_ARRAY_NAME bobgui_css_refs
#define GDK_ARRAY_TYPE_NAME BobguiCssRefs
#define GDK_ARRAY_ELEMENT_TYPE gpointer
#define GDK_ARAY_PREALLOC 32
#define GDK_ARRAY_NO_MEMSET 1
#include "gdk/gdkarrayimpl.c"

#define MAX_TOKEN_LENGTH 65536

typedef enum {
  RESOLVE_SUCCESS,
  RESOLVE_INVALID,
  RESOLVE_ANIMATION_TAINTED,
} ResolveResult;

struct _BobguiCssValue {
  BOBGUI_CSS_VALUE_BASE

  BobguiStyleProperty *property;
  BobguiCssVariableValue *value;
  GFile *file;
  guint subproperty;
};

static void
bobgui_css_value_reference_free (BobguiCssValue *value)
{
  bobgui_css_variable_value_unref (value->value);
  if (value->file)
    g_object_unref (value->file);
  g_free (value);
}

static ResolveResult
resolve_references_do (BobguiCssVariableValue *value,
                       guint                property_id,
                       BobguiCssVariableSet   *style_variables,
                       BobguiCssVariableSet   *keyframes_variables,
                       gboolean             root,
                       BobguiCssRefs          *refs,
                       gsize               *out_length,
                       gsize               *out_n_refs)
{
  BobguiCssCustomPropertyPool *pool = bobgui_css_custom_property_pool_get ();
  gsize i;
  gsize length = value->length;
  gsize n_refs = 0;
  ResolveResult ret = RESOLVE_SUCCESS;

  if (value->is_animation_tainted)
    {
      BobguiCssStyleProperty *prop = _bobgui_css_style_property_lookup_by_id (property_id);

      if (!_bobgui_css_style_property_is_animated (prop))
        {
          /* Animation-tainted variables make other variables that reference
           * them animation-tainted too, so unlike regular invalid variables it
           * propagates to the root. For example, if --test is animation-tainted,
           * --test2: var(--test, fallback1); prop: var(--test2, fallback2); will\
           * resolve to fallback2 and _not_ to fallback1. So we'll propagate it
           * up until the root, and treat it same as invalid there instead. */
          ret = RESOLVE_ANIMATION_TAINTED;
          goto error;
        }
    }

  if (value->is_invalid)
    {
      ret = RESOLVE_INVALID;
      goto error;
    }

  if (!root)
    {
      n_refs += 1;
      bobgui_css_refs_append (refs, value);
    }

  for (i = 0; i < value->n_references; i++)
    {
      BobguiCssVariableValueReference *ref = &value->references[i];
      int id = bobgui_css_custom_property_pool_lookup (pool, ref->name);
      BobguiCssVariableValue *var_value = NULL;
      gsize var_length = 0, var_refs = 0;
      BobguiCssVariableSet *source = style_variables;
      ResolveResult result = RESOLVE_INVALID;

      if (keyframes_variables)
        var_value = bobgui_css_variable_set_lookup (keyframes_variables, id, NULL);

      if (!var_value && style_variables)
        var_value = bobgui_css_variable_set_lookup (style_variables, id, &source);

      if (var_value)
        {
          result = resolve_references_do (var_value, property_id, source,
                                          keyframes_variables, FALSE,
                                          refs, &var_length, &var_refs);

          if (root && result == RESOLVE_ANIMATION_TAINTED)
            result = RESOLVE_INVALID;
        }

      if (result == RESOLVE_INVALID)
        {
          if (ref->fallback)
            {
              result = resolve_references_do (ref->fallback, property_id,
                                              style_variables, keyframes_variables,
                                              FALSE, refs, &var_length, &var_refs);

              if (root && result == RESOLVE_ANIMATION_TAINTED)
                result = RESOLVE_INVALID;
            }

          if (result != RESOLVE_SUCCESS)
            {
              ret = result;
              goto error;
            }
        }
      else if (result == RESOLVE_ANIMATION_TAINTED)
        {
          ret = RESOLVE_ANIMATION_TAINTED;
          goto error;
        }

      length += var_length - ref->length;
      n_refs += var_refs;

      if (length > MAX_TOKEN_LENGTH)
        {
          ret = RESOLVE_INVALID;
          goto error;
        }
    }

  if (out_length)
    *out_length = length;

  if (out_n_refs)
    *out_n_refs = n_refs;

  return ret;

error:
  /* Remove the references we added as if nothing happened */
  bobgui_css_refs_splice (refs, bobgui_css_refs_get_size (refs) - n_refs, n_refs, TRUE, NULL, 0);

  if (out_length)
    *out_length = 0;

  if (out_n_refs)
    *out_n_refs = 0;

  return ret;
}

static void
resolve_references (BobguiCssVariableValue *input,
                    guint                property_id,
                    BobguiCssStyle         *style,
                    BobguiCssVariableSet   *keyframes_variables,
                    BobguiCssRefs          *refs)
{
  if (resolve_references_do (input, property_id, style->variables,
                             keyframes_variables, TRUE, refs,
                             NULL, NULL) != RESOLVE_SUCCESS)
    {
      bobgui_css_refs_clear (refs);
    }
}

static void
parser_error (BobguiCssParser         *parser,
              const BobguiCssLocation *start,
              const BobguiCssLocation *end,
              const GError         *error,
              gpointer              user_data)
{
  BobguiStyleProvider *provider = user_data;
  GError *new_error = NULL;
  BobguiCssVariableValue **vars;
  char **names;
  gsize n_vars;

  bobgui_css_parser_get_expanding_variables (parser, &vars, &names, &n_vars);

  if (n_vars > 0)
    {
      for (int i = 0; i < n_vars; i++)
        {
          BobguiCssSection *section;

          if (names[i + 1])
            g_set_error (&new_error,
                         error->domain, error->code,
                         "While expanding %s: %s", names[i + 1], error->message);
          else
            g_set_error_literal (&new_error,
                                 error->domain, error->code,
                                 error->message);

          if (vars[i]->section == NULL)
            section = bobgui_css_section_new (bobgui_css_parser_get_file (parser), start, end);
          else
            section = bobgui_css_section_ref (vars[i]->section);

          bobgui_style_provider_emit_error (provider, section, new_error);

          bobgui_css_section_unref (section);
          g_clear_error (&new_error);
        }

      for (int i = 0; i < n_vars; i++)
        {
          if (vars[i])
            bobgui_css_variable_value_unref (vars[i]);
          g_free (names[i]);
        }

      g_free (vars);
      g_free (names);
    }
}

static BobguiCssValue *
bobgui_css_value_reference_compute (BobguiCssValue          *value,
                                 guint                 property_id,
                                 BobguiCssComputeContext *context)
{
  BobguiCssValue *result = NULL, *computed;
  BobguiCssRefs refs;
  guint shorthand_id = G_MAXUINT;

  if (BOBGUI_IS_CSS_SHORTHAND_PROPERTY (value->property))
    {
      shorthand_id = _bobgui_css_shorthand_property_get_id (BOBGUI_CSS_SHORTHAND_PROPERTY (value->property));
      if (context->shorthands && context->shorthands[shorthand_id])
        {
          result = bobgui_css_value_ref (context->shorthands[shorthand_id]);
          goto pick_subproperty;
        }
    }

  bobgui_css_refs_init (&refs);

  resolve_references (value->value, property_id, context->style, context->variables, &refs);

  if (bobgui_css_refs_get_size (&refs) > 0)
    {
      const BobguiCssToken *token;

      BobguiCssParser *value_parser =
        bobgui_css_parser_new_for_token_stream (value->value,
                                             value->file,
                                             (BobguiCssVariableValue **) refs.start,
                                             bobgui_css_refs_get_size (&refs),
                                             parser_error, context->provider,
                                             NULL);

      result = _bobgui_style_property_parse_value (value->property, value_parser);
      token = bobgui_css_parser_peek_token (value_parser);
      if (!bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_EOF))
        {
          char *junk;

          junk = bobgui_css_token_to_string (token);
          bobgui_css_parser_error_syntax (value_parser,
                                       "Junk at end of %s value: %s",
                                       value->property->name, junk);
          g_free (junk);

          g_clear_pointer (&result, bobgui_css_value_unref);
        }
      bobgui_css_parser_unref (value_parser);
    }

  bobgui_css_refs_clear (&refs);

  if (result == NULL)
    result = _bobgui_css_unset_value_new ();

  if (shorthand_id != G_MAXUINT)
    {
      BobguiCssValue *sub;

      if (context->shorthands)
        {
          g_assert (context->shorthands[shorthand_id] == NULL);
          context->shorthands[shorthand_id] = bobgui_css_value_ref (result);
        }

pick_subproperty:
      sub = bobgui_css_value_ref (_bobgui_css_array_value_get_nth (result, value->subproperty));
      bobgui_css_value_unref (result);
      result = sub;
    }

  computed = bobgui_css_value_compute (result, property_id, context);
  computed->is_computed = TRUE;

  bobgui_css_value_unref (result);

  return computed;
}

static gboolean
bobgui_css_value_reference_equal (const BobguiCssValue *value1,
                               const BobguiCssValue *value2)
{
  return FALSE;
}

static BobguiCssValue *
bobgui_css_value_reference_transition (BobguiCssValue *start,
                                    BobguiCssValue *end,
                                    guint        property_id,
                                    double       progress)
{
  return NULL;
}

static void
bobgui_css_value_reference_print (const BobguiCssValue *value,
                               GString           *string)
{
  bobgui_css_variable_value_print (value->value, string);
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_REFERENCE = {
  "BobguiCssReferenceValue",
  bobgui_css_value_reference_free,
  bobgui_css_value_reference_compute,
  NULL,
  bobgui_css_value_reference_equal,
  bobgui_css_value_reference_transition,
  NULL,
  NULL,
  bobgui_css_value_reference_print
};

BobguiCssValue *
_bobgui_css_reference_value_new (BobguiStyleProperty    *property,
                              BobguiCssVariableValue *value,
                              GFile               *file)
{
  BobguiCssValue *result;

  result = bobgui_css_value_new (BobguiCssValue, &BOBGUI_CSS_VALUE_REFERENCE);
  result->property = property;
  result->value = bobgui_css_variable_value_ref (value);
  result->contains_variables = TRUE;

  if (file)
    result->file = g_object_ref (file);
  else
    result->file = NULL;

  return result;
}

void
_bobgui_css_reference_value_set_subproperty (BobguiCssValue *value,
                                          guint        property)
{
  g_assert (BOBGUI_IS_CSS_SHORTHAND_PROPERTY (value->property));

  value->subproperty = property;
}

