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

#include "bobguicssdimensionvalueprivate.h"

BobguiCssValue *
bobgui_css_dimension_value_parse (BobguiCssParser           *parser,
                               BobguiCssNumberParseFlags  flags)
{
  static const struct {
    const char *name;
    BobguiCssUnit unit;
    BobguiCssNumberParseFlags required_flags;
  } units[] = {
    { "px",   BOBGUI_CSS_PX,      BOBGUI_CSS_PARSE_LENGTH },
    { "pt",   BOBGUI_CSS_PT,      BOBGUI_CSS_PARSE_LENGTH },
    { "em",   BOBGUI_CSS_EM,      BOBGUI_CSS_PARSE_LENGTH },
    { "ex",   BOBGUI_CSS_EX,      BOBGUI_CSS_PARSE_LENGTH },
    { "rem",  BOBGUI_CSS_REM,     BOBGUI_CSS_PARSE_LENGTH },
    { "pc",   BOBGUI_CSS_PC,      BOBGUI_CSS_PARSE_LENGTH },
    { "in",   BOBGUI_CSS_IN,      BOBGUI_CSS_PARSE_LENGTH },
    { "cm",   BOBGUI_CSS_CM,      BOBGUI_CSS_PARSE_LENGTH },
    { "mm",   BOBGUI_CSS_MM,      BOBGUI_CSS_PARSE_LENGTH },
    { "rad",  BOBGUI_CSS_RAD,     BOBGUI_CSS_PARSE_ANGLE  },
    { "deg",  BOBGUI_CSS_DEG,     BOBGUI_CSS_PARSE_ANGLE  },
    { "grad", BOBGUI_CSS_GRAD,    BOBGUI_CSS_PARSE_ANGLE  },
    { "turn", BOBGUI_CSS_TURN,    BOBGUI_CSS_PARSE_ANGLE  },
    { "s",    BOBGUI_CSS_S,       BOBGUI_CSS_PARSE_TIME   },
    { "ms",   BOBGUI_CSS_MS,      BOBGUI_CSS_PARSE_TIME   }
  };
  const BobguiCssToken *token;
  BobguiCssValue *result;
  BobguiCssUnit unit;
  double number;

  token = bobgui_css_parser_get_token (parser);

  /* Handle percentages first */
  if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_PERCENTAGE))
    {
      if (!(flags & BOBGUI_CSS_PARSE_PERCENT))
        {
          bobgui_css_parser_error_value (parser, "Percentages are not allowed here");
          return NULL;
        }
      number = token->number.number;
      unit = BOBGUI_CSS_PERCENT;
    }
  else if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_SIGNED_INTEGER) ||
           bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER) ||
           bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_SIGNED_NUMBER) ||
           bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_SIGNLESS_NUMBER))
    {
      number = token->number.number;
      if (number == 0.0)
        {
          if (flags & BOBGUI_CSS_PARSE_NUMBER)
            unit = BOBGUI_CSS_NUMBER;
          else if (flags & BOBGUI_CSS_PARSE_LENGTH)
            unit = BOBGUI_CSS_PX;
          else if (flags & BOBGUI_CSS_PARSE_ANGLE)
            unit = BOBGUI_CSS_DEG;
          else if (flags & BOBGUI_CSS_PARSE_TIME)
            unit = BOBGUI_CSS_S;
          else
            unit = BOBGUI_CSS_PERCENT;
        }
      else if (flags & BOBGUI_CSS_PARSE_NUMBER)
        {
          unit = BOBGUI_CSS_NUMBER;
        }
      else
        {
          bobgui_css_parser_error_syntax (parser, "Unit is missing.");
          return NULL;
        }
    }
  else if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_SIGNED_INTEGER_DIMENSION) ||
           bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER_DIMENSION) ||
           bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_SIGNED_DIMENSION) ||
           bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_SIGNLESS_DIMENSION))
    {
      guint i;

      for (i = 0; i < G_N_ELEMENTS (units); i++)
        {
          if (flags & units[i].required_flags &&
              g_ascii_strcasecmp (token->dimension.dimension, units[i].name) == 0)
            break;
        }

      if (i >= G_N_ELEMENTS (units))
        {
          bobgui_css_parser_error_syntax (parser, "'%s' is not a valid unit", token->dimension.dimension);
          return NULL;
        }

      unit = units[i].unit;
      number = token->dimension.value;
    }
  else
    {
      bobgui_css_parser_error_syntax (parser, "Expected a number");
      return NULL;
    }

  if (flags & BOBGUI_CSS_POSITIVE_ONLY &&
      number < 0)
    {
      bobgui_css_parser_error_value (parser, "Negative values are not allowed");
      return NULL;
    }

  result = bobgui_css_dimension_value_new (number, unit);
  bobgui_css_parser_consume_token (parser);

  return result;
}
