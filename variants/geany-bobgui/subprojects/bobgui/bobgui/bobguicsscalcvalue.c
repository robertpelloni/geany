/* BOBGUI - The Bobgui Framework
 * Copyright © 2016 Benjamin Otte <otte@gnome.org>
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

#include "bobguicsscalcvalueprivate.h"

#include <string.h>

BobguiCssValue * bobgui_css_calc_value_parse_sum (BobguiCssParser             *parser,
                                            BobguiCssNumberParseFlags    flags,
                                            BobguiCssNumberParseContext *ctx);

static BobguiCssValue *
bobgui_css_calc_value_parse_value (BobguiCssParser             *parser,
                                BobguiCssNumberParseFlags    flags,
                                BobguiCssNumberParseContext *ctx)
{
  if (bobgui_css_parser_has_token (parser, BOBGUI_CSS_TOKEN_OPEN_PARENS))
    {
      BobguiCssValue *result;

      bobgui_css_parser_start_block (parser);

      result = bobgui_css_calc_value_parse_sum (parser, flags, ctx);
      if (result == NULL)
        {
          bobgui_css_parser_end_block (parser);
          return NULL;
        }

      if (!bobgui_css_parser_has_token (parser, BOBGUI_CSS_TOKEN_EOF))
        {
          BobguiCssLocation start = *bobgui_css_parser_get_start_location (parser);
          bobgui_css_parser_skip_until (parser, BOBGUI_CSS_TOKEN_EOF);
          bobgui_css_parser_error (parser,
                                BOBGUI_CSS_PARSER_ERROR_SYNTAX,
                                &start,
                                bobgui_css_parser_get_start_location (parser),
                                "Expected closing ')' in calc() subterm");
          bobgui_css_value_unref (result);
          bobgui_css_parser_end_block (parser);
          return NULL;
        }

      bobgui_css_parser_end_block (parser);

      return result;
    }

  return bobgui_css_number_value_parse_with_context (parser, flags, ctx);
}

static gboolean
is_number (BobguiCssValue *value)
{
  return bobgui_css_number_value_get_dimension (value) == BOBGUI_CSS_DIMENSION_NUMBER
      && !bobgui_css_number_value_has_percent (value);
}

static BobguiCssValue *
bobgui_css_calc_value_parse_product (BobguiCssParser             *parser,
                                  BobguiCssNumberParseFlags    flags,
                                  BobguiCssNumberParseContext *ctx)
{
  BobguiCssValue *result, *value, *temp;
  BobguiCssNumberParseFlags actual_flags;
  BobguiCssLocation start;

  actual_flags = flags | BOBGUI_CSS_PARSE_NUMBER;
  bobgui_css_parser_get_token (parser);
  start = *bobgui_css_parser_get_start_location (parser);
  result = bobgui_css_calc_value_parse_value (parser, actual_flags, ctx);
  if (result == NULL)
    return NULL;

  while (TRUE)
    {
      if (actual_flags != BOBGUI_CSS_PARSE_NUMBER && !is_number (result))
        actual_flags = BOBGUI_CSS_PARSE_NUMBER;

      if (bobgui_css_parser_try_delim (parser, '*'))
        {
          value = bobgui_css_calc_value_parse_product (parser, actual_flags, ctx);
          if (value == NULL)
            goto fail;
          if (is_number (value))
            temp = bobgui_css_number_value_multiply (result, bobgui_css_number_value_get (value, 100));
          else
            temp = bobgui_css_number_value_multiply (value, bobgui_css_number_value_get (result, 100));
          bobgui_css_value_unref (value);
          bobgui_css_value_unref (result);
          result = temp;
        }
      else if (bobgui_css_parser_try_delim (parser, '/'))
        {
          value = bobgui_css_calc_value_parse_product (parser, BOBGUI_CSS_PARSE_NUMBER, ctx);
          if (value == NULL)
            goto fail;
          temp = bobgui_css_number_value_multiply (result, 1.0 / bobgui_css_number_value_get (value, 100));
          bobgui_css_value_unref (value);
          bobgui_css_value_unref (result);
          result = temp;
        }
      else
        {
          break;
        }
    }

  if (is_number (result) && !(flags & BOBGUI_CSS_PARSE_NUMBER))
    {
      bobgui_css_parser_error (parser,
                            BOBGUI_CSS_PARSER_ERROR_SYNTAX,
                            &start,
                            bobgui_css_parser_get_start_location (parser),
                            "calc() product term has no units");
      goto fail;
    }

  return result;

fail:
  bobgui_css_value_unref (result);
  return NULL;
}

BobguiCssValue *
bobgui_css_calc_value_parse_sum (BobguiCssParser             *parser,
                              BobguiCssNumberParseFlags    flags,
                              BobguiCssNumberParseContext *ctx)
{
  BobguiCssValue *result;

  result = bobgui_css_calc_value_parse_product (parser, flags, ctx);
  if (result == NULL)
    return NULL;

  while (TRUE)
    {
      BobguiCssValue *next, *temp;

      if (bobgui_css_parser_try_delim (parser, '+'))
        {
          next = bobgui_css_calc_value_parse_product (parser, flags, ctx);
          if (next == NULL)
            goto fail;
        }
      else if (bobgui_css_parser_try_delim (parser, '-'))
        {
          temp = bobgui_css_calc_value_parse_product (parser, flags, ctx);
          if (temp == NULL)
            goto fail;
          next = bobgui_css_number_value_multiply (temp, -1);
          bobgui_css_value_unref (temp);
        }
      else
        {
          if (bobgui_css_parser_has_token (parser, BOBGUI_CSS_TOKEN_SIGNED_INTEGER) ||
              bobgui_css_parser_has_token (parser, BOBGUI_CSS_TOKEN_SIGNED_NUMBER) ||
              bobgui_css_parser_has_token (parser, BOBGUI_CSS_TOKEN_SIGNED_INTEGER_DIMENSION) ||
              bobgui_css_parser_has_token (parser, BOBGUI_CSS_TOKEN_SIGNED_DIMENSION))
            {
              bobgui_css_parser_error_syntax (parser, "Unexpected signed number, did you forget a space between sign and number?");
              bobgui_css_parser_consume_token (parser);
            }
          break;
        }

      temp = bobgui_css_number_value_add (result, next);
      bobgui_css_value_unref (result);
      bobgui_css_value_unref (next);
      result = temp;
    }

  return result;

fail:
  bobgui_css_value_unref (result);
  return NULL;
}

typedef struct
{
  BobguiCssNumberParseFlags flags;
  BobguiCssNumberParseContext *ctx;
  BobguiCssValue *value;
} ParseCalcData;

static guint
bobgui_css_calc_value_parse_arg (BobguiCssParser *parser,
                              guint         arg,
                              gpointer      data_)
{
  ParseCalcData *data = data_;

  data->value = bobgui_css_calc_value_parse_sum (parser, data->flags, data->ctx);
  if (data->value == NULL)
    return 0;

  return 1;
}

BobguiCssValue *
bobgui_css_calc_value_parse (BobguiCssParser             *parser,
                          BobguiCssNumberParseFlags    flags,
                          BobguiCssNumberParseContext *ctx)
{
  ParseCalcData data;

  /* This can only be handled at compute time, we allow '-' after all */
  data.flags = flags & ~BOBGUI_CSS_POSITIVE_ONLY;
  data.value = NULL;
  data.ctx = ctx;

  if (!bobgui_css_parser_has_function (parser, "calc"))
    {
      bobgui_css_parser_error_syntax (parser, "Expected 'calc('");
      return NULL;
    }

  if (!bobgui_css_parser_consume_function (parser, 1, 1, bobgui_css_calc_value_parse_arg, &data))
    return NULL;

  return data.value;
}

typedef struct
{
  BobguiCssNumberParseFlags flags;
  BobguiCssNumberParseContext *ctx;
  GPtrArray *values;
} ParseArgnData;

static guint
bobgui_css_argn_value_parse_arg (BobguiCssParser *parser,
                              guint         arg,
                              gpointer      data_)
{
  ParseArgnData *data = data_;
  BobguiCssValue *value;

  value = bobgui_css_calc_value_parse_sum (parser, data->flags, data->ctx);
  if (value == NULL)
    return 0;

  g_ptr_array_add (data->values, value);

  return 1;
}

typedef struct
{
  BobguiCssNumberParseFlags flags;
  BobguiCssNumberParseContext *ctx;
  BobguiCssValue *values[3];
} ParseClampData;

static guint
bobgui_css_clamp_value_parse_arg (BobguiCssParser *parser,
                               guint         arg,
                               gpointer      data_)
{
  ParseClampData *data = data_;

  if ((arg == 0 || arg == 2))
    {
      if (bobgui_css_parser_try_ident (parser, "none"))
        {
          data->values[arg] = NULL;
          return 1;
        }
    }

  data->values[arg] = bobgui_css_calc_value_parse_sum (parser, data->flags, data->ctx);
  if (data->values[arg] == NULL)
    return 0;

  return 1;
}

BobguiCssValue *
bobgui_css_clamp_value_parse (BobguiCssParser             *parser,
                           BobguiCssNumberParseFlags    flags,
                           BobguiCssNumberParseContext *ctx,
                           guint                     type)
{
  ParseClampData data;
  BobguiCssValue *result = NULL;

  if (!bobgui_css_parser_has_function (parser, "clamp"))
    {
      bobgui_css_parser_error_syntax (parser, "Expected 'clamp('");
      return NULL;
    }

  /* This can only be handled at compute time, we allow '-' after all */
  data.flags = flags & ~BOBGUI_CSS_POSITIVE_ONLY;
  data.ctx = ctx;
  data.values[0] = NULL;
  data.values[1] = NULL;
  data.values[2] = NULL;

  if (bobgui_css_parser_consume_function (parser, 3, 3, bobgui_css_clamp_value_parse_arg, &data))
    {
      BobguiCssDimension dim = bobgui_css_number_value_get_dimension (data.values[1]);
      if ((data.values[0] && bobgui_css_number_value_get_dimension (data.values[0]) != dim) ||
          (data.values[2] && bobgui_css_number_value_get_dimension (data.values[2]) != dim))
        bobgui_css_parser_error_syntax (parser, "Inconsistent types in 'clamp('");
      else
        result = bobgui_css_math_value_new (type, 0, data.values, 3);
    }

  if (result == NULL)
    {
      g_clear_pointer (&data.values[0], bobgui_css_value_unref);
      g_clear_pointer (&data.values[1], bobgui_css_value_unref);
      g_clear_pointer (&data.values[2], bobgui_css_value_unref);
    }

  return result;
}

typedef struct {
  BobguiCssNumberParseFlags flags;
  BobguiCssNumberParseContext *ctx;
  guint mode;
  gboolean has_mode;
  BobguiCssValue *values[2];
} ParseRoundData;

static guint
bobgui_css_round_value_parse_arg (BobguiCssParser *parser,
                               guint         arg,
                               gpointer      data_)
{
  ParseRoundData *data = data_;

  if (arg == 0)
    {
      const char *modes[] = { "nearest", "up", "down", "to-zero" };

      for (guint i = 0; i < G_N_ELEMENTS (modes); i++)
        {
          if (bobgui_css_parser_try_ident (parser, modes[i]))
            {
              data->mode = i;
              data->has_mode = TRUE;
              return 1;
            }
        }

      data->values[0] = bobgui_css_calc_value_parse_sum (parser, data->flags, data->ctx);
      if (data->values[0] == NULL)
        return 0;
    }
  else if (arg == 1)
    {
      BobguiCssValue *value = bobgui_css_calc_value_parse_sum (parser, data->flags, data->ctx);

      if (value == NULL)
        return 0;

      if (data->has_mode)
        data->values[0] = value;
      else
        data->values[1] = value;
    }
  else
    {
      if (!data->has_mode)
        {
          bobgui_css_parser_error_syntax (parser, "Too many argument for 'round'");
          return 0;
        }

      data->values[1] = bobgui_css_calc_value_parse_sum (parser, data->flags, data->ctx);

      if (data->values[1] == NULL)
        return 0;
    }

  return 1;
}

BobguiCssValue *
bobgui_css_round_value_parse (BobguiCssParser             *parser,
                           BobguiCssNumberParseFlags    flags,
                           BobguiCssNumberParseContext *ctx,
                           guint                     type)
{
  ParseRoundData data;
  BobguiCssValue *result = NULL;

  if (!bobgui_css_parser_has_function (parser, "round"))
    {
      bobgui_css_parser_error_syntax (parser, "Expected 'round('");
      return NULL;
    }

  data.flags = flags & ~BOBGUI_CSS_POSITIVE_ONLY;
  data.ctx = ctx;
  data.mode = ROUND_NEAREST;
  data.has_mode = FALSE;
  data.values[0] = NULL;
  data.values[1] = NULL;

  if (bobgui_css_parser_consume_function (parser, 1, 3, bobgui_css_round_value_parse_arg, &data) &&
      data.values[0] != NULL)
    {
      if (data.values[1] != NULL &&
          bobgui_css_number_value_get_dimension (data.values[0]) !=
          bobgui_css_number_value_get_dimension (data.values[1]))
        bobgui_css_parser_error_syntax (parser, "Inconsistent types in 'round('");
      else if (data.values[1] == NULL &&
               bobgui_css_number_value_get_dimension (data.values[0]) != BOBGUI_CSS_DIMENSION_NUMBER)
        bobgui_css_parser_error_syntax (parser, "Can't omit second argument to 'round(' here");
      else
        result = bobgui_css_math_value_new (type, data.mode, data.values, data.values[1] != NULL ? 2 : 1);
    }

  if (result == NULL)
    {
      g_clear_pointer (&data.values[0], bobgui_css_value_unref);
      g_clear_pointer (&data.values[1], bobgui_css_value_unref);
    }

  return result;
}

typedef struct {
  BobguiCssNumberParseFlags flags;
  BobguiCssNumberParseContext *ctx;
  BobguiCssValue *values[2];
} ParseArg2Data;

static guint
bobgui_css_arg2_value_parse_arg (BobguiCssParser *parser,
                              guint         arg,
                              gpointer      data_)
{
  ParseArg2Data *data = data_;

  data->values[arg] = bobgui_css_calc_value_parse_sum (parser, data->flags, data->ctx);
  if (data->values[arg] == NULL)
    return 0;

  return 1;
}

BobguiCssValue *
bobgui_css_arg2_value_parse (BobguiCssParser             *parser,
                          BobguiCssNumberParseFlags    flags,
                          BobguiCssNumberParseContext *ctx,
                          guint                     min_args,
                          guint                     max_args,
                          const char               *function,
                          guint                     type)
{
  ParseArg2Data data;
  BobguiCssValue *result = NULL;

  g_assert (1 <= min_args && min_args <= max_args && max_args <= 2);

  if (!bobgui_css_parser_has_function (parser, function))
    {
      bobgui_css_parser_error_syntax (parser, "Expected '%s('", function);
      return NULL;
    }

  data.flags = flags & ~BOBGUI_CSS_POSITIVE_ONLY;
  data.ctx = ctx;
  data.values[0] = NULL;
  data.values[1] = NULL;

  if (bobgui_css_parser_consume_function (parser, min_args, max_args, bobgui_css_arg2_value_parse_arg, &data))
    {
      if (data.values[1] != NULL &&
          bobgui_css_number_value_get_dimension (data.values[0]) !=
          bobgui_css_number_value_get_dimension (data.values[1]))
        bobgui_css_parser_error_syntax (parser, "Inconsistent types in '%s('", function);
      else
        result = bobgui_css_math_value_new (type, 0, data.values, data.values[1] != NULL ? 2 : 1);
    }

  if (result == NULL)
    {
      g_clear_pointer (&data.values[0], bobgui_css_value_unref);
      g_clear_pointer (&data.values[1], bobgui_css_value_unref);
    }

  return result;
}

BobguiCssValue *
bobgui_css_argn_value_parse (BobguiCssParser             *parser,
                          BobguiCssNumberParseFlags    flags,
                          BobguiCssNumberParseContext *ctx,
                          const char               *function,
                          guint                     type)
{
  ParseArgnData data;
  BobguiCssValue *result = NULL;

  if (!bobgui_css_parser_has_function (parser, function))
    {
      bobgui_css_parser_error_syntax (parser, "Expected '%s('", function);
      return NULL;
    }

  /* This can only be handled at compute time, we allow '-' after all */
  data.flags = flags & ~BOBGUI_CSS_POSITIVE_ONLY;
  data.values = g_ptr_array_new ();
  data.ctx = ctx;

  if (bobgui_css_parser_consume_function (parser, 1, G_MAXUINT, bobgui_css_argn_value_parse_arg, &data))
    {
      BobguiCssValue *val = (BobguiCssValue *) g_ptr_array_index (data.values, 0);
      BobguiCssDimension dim = bobgui_css_number_value_get_dimension (val);
      guint i;
      for (i = 1; i < data.values->len; i++)
        {
          val = (BobguiCssValue *) g_ptr_array_index (data.values, i);
          if (bobgui_css_number_value_get_dimension (val) != dim)
            break;
        }
      if (i < data.values->len)
        bobgui_css_parser_error_syntax (parser, "Inconsistent types in '%s('", function);
      else
        result = bobgui_css_math_value_new (type, 0, (BobguiCssValue **)data.values->pdata, data.values->len);
    }

  if (result == NULL)
    {
      for (guint i = 0; i < data.values->len; i++)
        bobgui_css_value_unref ((BobguiCssValue *)g_ptr_array_index (data.values, i));
    }

  g_ptr_array_unref (data.values);

  return result;
}
