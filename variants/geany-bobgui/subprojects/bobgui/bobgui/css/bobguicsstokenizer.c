/* GSK - The Bobgui Framework
 * Copyright (C) 2011 Benjamin Otte <otte@gnome.org>
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

#include "bobguicsstokenizerprivate.h"

#include "bobguicssenums.h"
#include "bobguicsserror.h"
#include "bobguicsslocationprivate.h"

#include <math.h>
#include <string.h>

struct _BobguiCssTokenizer
{
  int                    ref_count;
  GBytes                *bytes;
  GString               *name_buffer;

  const char            *data;
  const char            *end;

  BobguiCssLocation         position;

  BobguiCssLocation         saved_position;
  const char            *saved_data;
};

void
bobgui_css_token_clear (BobguiCssToken *token)
{
  switch (token->type)
    {
    case BOBGUI_CSS_TOKEN_STRING:
    case BOBGUI_CSS_TOKEN_IDENT:
    case BOBGUI_CSS_TOKEN_FUNCTION:
    case BOBGUI_CSS_TOKEN_AT_KEYWORD:
    case BOBGUI_CSS_TOKEN_HASH_UNRESTRICTED:
    case BOBGUI_CSS_TOKEN_HASH_ID:
    case BOBGUI_CSS_TOKEN_URL:
      if (token->string.string != token->string.buf)
        g_free (token->string.string);
      break;

    case BOBGUI_CSS_TOKEN_SIGNED_INTEGER_DIMENSION:
    case BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER_DIMENSION:
    case BOBGUI_CSS_TOKEN_SIGNED_DIMENSION:
    case BOBGUI_CSS_TOKEN_SIGNLESS_DIMENSION:
    case BOBGUI_CSS_TOKEN_EOF:
    case BOBGUI_CSS_TOKEN_WHITESPACE:
    case BOBGUI_CSS_TOKEN_OPEN_PARENS:
    case BOBGUI_CSS_TOKEN_CLOSE_PARENS:
    case BOBGUI_CSS_TOKEN_OPEN_SQUARE:
    case BOBGUI_CSS_TOKEN_CLOSE_SQUARE:
    case BOBGUI_CSS_TOKEN_OPEN_CURLY:
    case BOBGUI_CSS_TOKEN_CLOSE_CURLY:
    case BOBGUI_CSS_TOKEN_COMMA:
    case BOBGUI_CSS_TOKEN_COLON:
    case BOBGUI_CSS_TOKEN_SEMICOLON:
    case BOBGUI_CSS_TOKEN_CDC:
    case BOBGUI_CSS_TOKEN_CDO:
    case BOBGUI_CSS_TOKEN_DELIM:
    case BOBGUI_CSS_TOKEN_SIGNED_INTEGER:
    case BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER:
    case BOBGUI_CSS_TOKEN_SIGNED_NUMBER:
    case BOBGUI_CSS_TOKEN_SIGNLESS_NUMBER:
    case BOBGUI_CSS_TOKEN_PERCENTAGE:
    case BOBGUI_CSS_TOKEN_INCLUDE_MATCH:
    case BOBGUI_CSS_TOKEN_DASH_MATCH:
    case BOBGUI_CSS_TOKEN_PREFIX_MATCH:
    case BOBGUI_CSS_TOKEN_SUFFIX_MATCH:
    case BOBGUI_CSS_TOKEN_SUBSTRING_MATCH:
    case BOBGUI_CSS_TOKEN_COLUMN:
    case BOBGUI_CSS_TOKEN_BAD_STRING:
    case BOBGUI_CSS_TOKEN_BAD_URL:
    case BOBGUI_CSS_TOKEN_COMMENT:
      break;

    default:
      g_assert_not_reached ();
    }

  token->type = BOBGUI_CSS_TOKEN_EOF;
}

static void
bobgui_css_token_init (BobguiCssToken     *token,
                    BobguiCssTokenType  type)
{
  token->type = type;

  switch ((guint)type)
    {
    case BOBGUI_CSS_TOKEN_EOF:
    case BOBGUI_CSS_TOKEN_WHITESPACE:
    case BOBGUI_CSS_TOKEN_OPEN_PARENS:
    case BOBGUI_CSS_TOKEN_CLOSE_PARENS:
    case BOBGUI_CSS_TOKEN_OPEN_SQUARE:
    case BOBGUI_CSS_TOKEN_CLOSE_SQUARE:
    case BOBGUI_CSS_TOKEN_OPEN_CURLY:
    case BOBGUI_CSS_TOKEN_CLOSE_CURLY:
    case BOBGUI_CSS_TOKEN_COMMA:
    case BOBGUI_CSS_TOKEN_COLON:
    case BOBGUI_CSS_TOKEN_SEMICOLON:
    case BOBGUI_CSS_TOKEN_CDC:
    case BOBGUI_CSS_TOKEN_CDO:
    case BOBGUI_CSS_TOKEN_INCLUDE_MATCH:
    case BOBGUI_CSS_TOKEN_DASH_MATCH:
    case BOBGUI_CSS_TOKEN_PREFIX_MATCH:
    case BOBGUI_CSS_TOKEN_SUFFIX_MATCH:
    case BOBGUI_CSS_TOKEN_SUBSTRING_MATCH:
    case BOBGUI_CSS_TOKEN_COLUMN:
    case BOBGUI_CSS_TOKEN_BAD_STRING:
    case BOBGUI_CSS_TOKEN_BAD_URL:
    case BOBGUI_CSS_TOKEN_COMMENT:
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
append_ident (GString    *string,
              const char *ident)
{
  /* XXX */
  g_string_append (string, ident);
}

static void
append_string (GString    *string,
               const char *s)
{
  g_string_append_c (string, '"');
  /* XXX */
  g_string_append (string, s);
  g_string_append_c (string, '"');
}

/*
 * bobgui_css_token_is_finite:
 * @token: a `BobguiCssToken`
 *
 * A token is considered finite when it would stay the same no matter
 * what bytes follow it in the data stream.
 *
 * An obvious example for this is the ';' token.
 *
 * Returns: %TRUE if the token is considered finite.
 **/
gboolean
bobgui_css_token_is_finite (const BobguiCssToken *token)
{
  switch (token->type)
    {
    case BOBGUI_CSS_TOKEN_EOF:
    case BOBGUI_CSS_TOKEN_STRING:
    case BOBGUI_CSS_TOKEN_FUNCTION:
    case BOBGUI_CSS_TOKEN_URL:
    case BOBGUI_CSS_TOKEN_PERCENTAGE:
    case BOBGUI_CSS_TOKEN_OPEN_PARENS:
    case BOBGUI_CSS_TOKEN_CLOSE_PARENS:
    case BOBGUI_CSS_TOKEN_OPEN_SQUARE:
    case BOBGUI_CSS_TOKEN_CLOSE_SQUARE:
    case BOBGUI_CSS_TOKEN_OPEN_CURLY:
    case BOBGUI_CSS_TOKEN_CLOSE_CURLY:
    case BOBGUI_CSS_TOKEN_COMMA:
    case BOBGUI_CSS_TOKEN_COLON:
    case BOBGUI_CSS_TOKEN_SEMICOLON:
    case BOBGUI_CSS_TOKEN_CDC:
    case BOBGUI_CSS_TOKEN_CDO:
    case BOBGUI_CSS_TOKEN_INCLUDE_MATCH:
    case BOBGUI_CSS_TOKEN_DASH_MATCH:
    case BOBGUI_CSS_TOKEN_PREFIX_MATCH:
    case BOBGUI_CSS_TOKEN_SUFFIX_MATCH:
    case BOBGUI_CSS_TOKEN_SUBSTRING_MATCH:
    case BOBGUI_CSS_TOKEN_COLUMN:
    case BOBGUI_CSS_TOKEN_COMMENT:
      return TRUE;

    default:
      g_assert_not_reached ();
    case BOBGUI_CSS_TOKEN_WHITESPACE:
    case BOBGUI_CSS_TOKEN_IDENT:
    case BOBGUI_CSS_TOKEN_AT_KEYWORD:
    case BOBGUI_CSS_TOKEN_HASH_UNRESTRICTED:
    case BOBGUI_CSS_TOKEN_HASH_ID:
    case BOBGUI_CSS_TOKEN_DELIM:
    case BOBGUI_CSS_TOKEN_SIGNED_INTEGER:
    case BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER:
    case BOBGUI_CSS_TOKEN_SIGNED_NUMBER:
    case BOBGUI_CSS_TOKEN_SIGNLESS_NUMBER:
    case BOBGUI_CSS_TOKEN_BAD_STRING:
    case BOBGUI_CSS_TOKEN_BAD_URL:
    case BOBGUI_CSS_TOKEN_SIGNED_INTEGER_DIMENSION:
    case BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER_DIMENSION:
    case BOBGUI_CSS_TOKEN_SIGNED_DIMENSION:
    case BOBGUI_CSS_TOKEN_SIGNLESS_DIMENSION:
      return FALSE;
    }
}

/*
 * bobgui_css_token_is_preserved:
 * @token: a `BobguiCssToken`
 * @out_closing: (nullable): Type of the token that closes a block
 *   started with this token
 *
 * A token is considered preserved when it does not start a block.
 *
 * Tokens that start a block require different error recovery when parsing,
 * so CSS parsers want to look at this function
 *
 * Returns: %TRUE if the token is considered preserved.
 */
gboolean
bobgui_css_token_is_preserved (const BobguiCssToken *token,
                            BobguiCssTokenType   *out_closing)
{
  switch (token->type)
    {
    case BOBGUI_CSS_TOKEN_FUNCTION:
    case BOBGUI_CSS_TOKEN_OPEN_PARENS:
      if (out_closing)
        *out_closing = BOBGUI_CSS_TOKEN_CLOSE_PARENS;
      return FALSE;

    case BOBGUI_CSS_TOKEN_OPEN_SQUARE:
      if (out_closing)
        *out_closing = BOBGUI_CSS_TOKEN_CLOSE_SQUARE;
      return FALSE;

    case BOBGUI_CSS_TOKEN_OPEN_CURLY:
      if (out_closing)
        *out_closing = BOBGUI_CSS_TOKEN_CLOSE_CURLY;
      return FALSE;

    default:
      g_assert_not_reached ();
    case BOBGUI_CSS_TOKEN_EOF:
    case BOBGUI_CSS_TOKEN_WHITESPACE:
    case BOBGUI_CSS_TOKEN_STRING:
    case BOBGUI_CSS_TOKEN_URL:
    case BOBGUI_CSS_TOKEN_PERCENTAGE:
    case BOBGUI_CSS_TOKEN_CLOSE_PARENS:
    case BOBGUI_CSS_TOKEN_CLOSE_SQUARE:
    case BOBGUI_CSS_TOKEN_CLOSE_CURLY:
    case BOBGUI_CSS_TOKEN_COMMA:
    case BOBGUI_CSS_TOKEN_COLON:
    case BOBGUI_CSS_TOKEN_SEMICOLON:
    case BOBGUI_CSS_TOKEN_CDC:
    case BOBGUI_CSS_TOKEN_CDO:
    case BOBGUI_CSS_TOKEN_INCLUDE_MATCH:
    case BOBGUI_CSS_TOKEN_DASH_MATCH:
    case BOBGUI_CSS_TOKEN_PREFIX_MATCH:
    case BOBGUI_CSS_TOKEN_SUFFIX_MATCH:
    case BOBGUI_CSS_TOKEN_SUBSTRING_MATCH:
    case BOBGUI_CSS_TOKEN_COLUMN:
    case BOBGUI_CSS_TOKEN_COMMENT:
    case BOBGUI_CSS_TOKEN_IDENT:
    case BOBGUI_CSS_TOKEN_AT_KEYWORD:
    case BOBGUI_CSS_TOKEN_HASH_UNRESTRICTED:
    case BOBGUI_CSS_TOKEN_HASH_ID:
    case BOBGUI_CSS_TOKEN_DELIM:
    case BOBGUI_CSS_TOKEN_SIGNED_INTEGER:
    case BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER:
    case BOBGUI_CSS_TOKEN_SIGNED_NUMBER:
    case BOBGUI_CSS_TOKEN_SIGNLESS_NUMBER:
    case BOBGUI_CSS_TOKEN_BAD_STRING:
    case BOBGUI_CSS_TOKEN_BAD_URL:
    case BOBGUI_CSS_TOKEN_SIGNED_INTEGER_DIMENSION:
    case BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER_DIMENSION:
    case BOBGUI_CSS_TOKEN_SIGNED_DIMENSION:
    case BOBGUI_CSS_TOKEN_SIGNLESS_DIMENSION:
      if (out_closing)
        *out_closing = BOBGUI_CSS_TOKEN_EOF;
      return TRUE;
    }
}

gboolean
bobgui_css_token_is_ident (const BobguiCssToken *token,
                        const char        *ident)
{
  return bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_IDENT)
      && (g_ascii_strcasecmp (bobgui_css_token_get_string (token), ident) == 0);
}

gboolean
bobgui_css_token_is_function (const BobguiCssToken *token,
                           const char        *ident)
{
  return bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_FUNCTION)
      && (g_ascii_strcasecmp (bobgui_css_token_get_string (token), ident) == 0);
}

gboolean
bobgui_css_token_is_delim (const BobguiCssToken *token,
                        gunichar           delim)
{
  return bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_DELIM)
      && token->delim.delim == delim;
}

void
bobgui_css_token_print (const BobguiCssToken *token,
                     GString           *string)
{
  char buf[G_ASCII_DTOSTR_BUF_SIZE];

  switch (token->type)
    {
    case BOBGUI_CSS_TOKEN_STRING:
      append_string (string, bobgui_css_token_get_string (token));
      break;

    case BOBGUI_CSS_TOKEN_IDENT:
      append_ident (string, bobgui_css_token_get_string (token));
      break;

    case BOBGUI_CSS_TOKEN_URL:
      g_string_append (string, "url(");
      append_ident (string, bobgui_css_token_get_string (token));
      g_string_append (string, ")");
      break;

    case BOBGUI_CSS_TOKEN_FUNCTION:
      append_ident (string, bobgui_css_token_get_string (token));
      g_string_append_c (string, '(');
      break;

    case BOBGUI_CSS_TOKEN_AT_KEYWORD:
      g_string_append_c (string, '@');
      append_ident (string, bobgui_css_token_get_string (token));
      break;

    case BOBGUI_CSS_TOKEN_HASH_UNRESTRICTED:
    case BOBGUI_CSS_TOKEN_HASH_ID:
      g_string_append_c (string, '#');
      append_ident (string, bobgui_css_token_get_string (token));
      break;

    case BOBGUI_CSS_TOKEN_DELIM:
      g_string_append_unichar (string, token->delim.delim);
      break;

    case BOBGUI_CSS_TOKEN_SIGNED_INTEGER:
    case BOBGUI_CSS_TOKEN_SIGNED_NUMBER:
      if (token->number.number >= 0)
        g_string_append_c (string, '+');
      G_GNUC_FALLTHROUGH;
    case BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER:
    case BOBGUI_CSS_TOKEN_SIGNLESS_NUMBER:
      g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, token->number.number);
      g_string_append (string, buf);
      break;

    case BOBGUI_CSS_TOKEN_PERCENTAGE:
      g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, token->number.number);
      g_string_append (string, buf);
      g_string_append_c (string, '%');
      break;

    case BOBGUI_CSS_TOKEN_SIGNED_INTEGER_DIMENSION:
    case BOBGUI_CSS_TOKEN_SIGNED_DIMENSION:
      if (token->dimension.value >= 0)
        g_string_append_c (string, '+');
      G_GNUC_FALLTHROUGH;
    case BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER_DIMENSION:
    case BOBGUI_CSS_TOKEN_SIGNLESS_DIMENSION:
      g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, token->dimension.value);
      g_string_append (string, buf);
      append_ident (string, token->dimension.dimension);
      break;

    case BOBGUI_CSS_TOKEN_EOF:
      break;

    case BOBGUI_CSS_TOKEN_WHITESPACE:
      g_string_append (string, " ");
      break;

    case BOBGUI_CSS_TOKEN_OPEN_PARENS:
      g_string_append (string, "(");
      break;

    case BOBGUI_CSS_TOKEN_CLOSE_PARENS:
      g_string_append (string, ")");
      break;

    case BOBGUI_CSS_TOKEN_OPEN_SQUARE:
      g_string_append (string, "[");
      break;

    case BOBGUI_CSS_TOKEN_CLOSE_SQUARE:
      g_string_append (string, "]");
      break;

    case BOBGUI_CSS_TOKEN_OPEN_CURLY:
      g_string_append (string, "{");
      break;

    case BOBGUI_CSS_TOKEN_CLOSE_CURLY:
      g_string_append (string, "}");
      break;

    case BOBGUI_CSS_TOKEN_COMMA:
      g_string_append (string, ",");
      break;

    case BOBGUI_CSS_TOKEN_COLON:
      g_string_append (string, ":");
      break;

    case BOBGUI_CSS_TOKEN_SEMICOLON:
      g_string_append (string, ";");
      break;

    case BOBGUI_CSS_TOKEN_CDO:
      g_string_append (string, "<!--");
      break;

    case BOBGUI_CSS_TOKEN_CDC:
      g_string_append (string, "-->");
      break;

    case BOBGUI_CSS_TOKEN_INCLUDE_MATCH:
      g_string_append (string, "~=");
      break;

    case BOBGUI_CSS_TOKEN_DASH_MATCH:
      g_string_append (string, "|=");
      break;

    case BOBGUI_CSS_TOKEN_PREFIX_MATCH:
      g_string_append (string, "^=");
      break;

    case BOBGUI_CSS_TOKEN_SUFFIX_MATCH:
      g_string_append (string, "$=");
      break;

    case BOBGUI_CSS_TOKEN_SUBSTRING_MATCH:
      g_string_append (string, "*=");
      break;

    case BOBGUI_CSS_TOKEN_COLUMN:
      g_string_append (string, "||");
      break;

    case BOBGUI_CSS_TOKEN_BAD_STRING:
      g_string_append (string, "\"\n");
      break;

    case BOBGUI_CSS_TOKEN_BAD_URL:
      g_string_append (string, "url(bad url)");
      break;

    case BOBGUI_CSS_TOKEN_COMMENT:
      g_string_append (string, "/* comment */");
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}

char *
bobgui_css_token_to_string (const BobguiCssToken *token)
{
  GString *string;

  string = g_string_new (NULL);
  bobgui_css_token_print (token, string);
  return g_string_free (string, FALSE);
}

static void
bobgui_css_token_init_string (BobguiCssToken     *token,
                           BobguiCssTokenType  type,
                           GString         *string)
{
  token->type = type;

  switch ((guint)type)
    {
    case BOBGUI_CSS_TOKEN_STRING:
    case BOBGUI_CSS_TOKEN_IDENT:
    case BOBGUI_CSS_TOKEN_FUNCTION:
    case BOBGUI_CSS_TOKEN_AT_KEYWORD:
    case BOBGUI_CSS_TOKEN_HASH_UNRESTRICTED:
    case BOBGUI_CSS_TOKEN_HASH_ID:
    case BOBGUI_CSS_TOKEN_URL:
      if (string->len < G_N_ELEMENTS (token->string.buf))
        {
          g_strlcpy (token->string.buf, string->str, G_N_ELEMENTS (token->string.buf));
          token->string.string = token->string.buf;
        }
      else
        {
          token->string.string = g_strdup (string->str);
        }
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
bobgui_css_token_init_delim (BobguiCssToken *token,
                          gunichar     delim)
{
  token->type = BOBGUI_CSS_TOKEN_DELIM;
  token->delim.delim = delim;
}

static void
bobgui_css_token_init_number (BobguiCssToken     *token,
                           BobguiCssTokenType  type,
                           double           value)
{
  token->type = type;

  switch ((guint)type)
    {
    case BOBGUI_CSS_TOKEN_SIGNED_INTEGER:
    case BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER:
    case BOBGUI_CSS_TOKEN_SIGNED_NUMBER:
    case BOBGUI_CSS_TOKEN_SIGNLESS_NUMBER:
    case BOBGUI_CSS_TOKEN_PERCENTAGE:
      token->number.number = value;
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
bobgui_css_token_init_dimension (BobguiCssToken     *token,
                              BobguiCssTokenType  type,
                              double           value,
                              GString         *string)
{
  token->type = type;

  switch ((guint)type)
    {
    case BOBGUI_CSS_TOKEN_SIGNED_INTEGER_DIMENSION:
    case BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER_DIMENSION:
    case BOBGUI_CSS_TOKEN_SIGNED_DIMENSION:
    case BOBGUI_CSS_TOKEN_SIGNLESS_DIMENSION:
      token->dimension.value = value;
      g_strlcpy (token->dimension.dimension, string->str, 8);
      break;
    default:
      g_assert_not_reached ();
    }
}

BobguiCssTokenizer *
bobgui_css_tokenizer_new (GBytes *bytes)
{
  return bobgui_css_tokenizer_new_for_range (bytes, 0, g_bytes_get_size (bytes));
}

BobguiCssTokenizer *
bobgui_css_tokenizer_new_for_range (GBytes *bytes,
                                 gsize   offset,
                                 gsize   length)
{
  BobguiCssTokenizer *tokenizer;

  tokenizer = g_new0 (BobguiCssTokenizer, 1);
  tokenizer->ref_count = 1;
  tokenizer->bytes = g_bytes_ref (bytes);
  tokenizer->name_buffer = g_string_new (NULL);

  tokenizer->data = g_bytes_get_region (bytes, 1, offset, length);
  tokenizer->end = tokenizer->data + length;

  bobgui_css_location_init (&tokenizer->position);

  return tokenizer;
}

BobguiCssTokenizer *
bobgui_css_tokenizer_ref (BobguiCssTokenizer *tokenizer)
{
  tokenizer->ref_count++;
  
  return tokenizer;
}

void
bobgui_css_tokenizer_unref (BobguiCssTokenizer *tokenizer)
{
  tokenizer->ref_count--;
  if (tokenizer->ref_count > 0)
    return;

  g_string_free (tokenizer->name_buffer, TRUE);
  g_bytes_unref (tokenizer->bytes);
  g_free (tokenizer);
}

GBytes *
bobgui_css_tokenizer_get_bytes (BobguiCssTokenizer *tokenizer)
{
  return tokenizer->bytes;
}

const BobguiCssLocation *
bobgui_css_tokenizer_get_location (BobguiCssTokenizer *tokenizer)
{
  return &tokenizer->position;
}

static void G_GNUC_PRINTF(2, 3)
bobgui_css_tokenizer_parse_error (GError     **error,
                               const char  *format,
                               ...)
{
  va_list args;

  va_start (args, format);
  if (error)
    {
      *error = g_error_new_valist (BOBGUI_CSS_PARSER_ERROR,
                                   BOBGUI_CSS_PARSER_ERROR_SYNTAX,
                                   format, args);
    }
  else
    {
      char *s = g_strdup_vprintf (format, args);
      g_print ("error: %s\n", s);
      g_free (s);
    }
  va_end (args);
}

static inline gboolean
is_newline (char c)
{
  return c == '\n'
      || c == '\r'
      || c == '\f';
}

static inline gboolean
is_whitespace (char c)
{
  return is_newline (c)
      || c == '\t'
      || c == ' ';
}

static inline gboolean
is_multibyte (char c)
{
  return c & 0x80;
}

static inline gboolean
is_name_start (char c)
{
   return is_multibyte (c)
       || g_ascii_isalpha (c)
       || c == '_';
}

static inline gboolean
is_name (char c)
{
  return is_name_start (c)
      || g_ascii_isdigit (c)
      || c == '-';
}

static inline gboolean
is_non_printable (char c)
{
  return (c >= 0 && c <= 0x08)
      || c == 0x0B
      || c == 0x0E
      || c == 0x1F
      || c == 0x7F;
}

static inline gboolean
is_valid_escape (const char *data,
                 const char *end)
{
  switch (end - data)
    {
      default:
        if (is_newline (data[1]))
          return FALSE;
        G_GNUC_FALLTHROUGH;

      case 1:
        return data[0] == '\\';

      case 0:
        return FALSE;
    }
}

static inline gsize
bobgui_css_tokenizer_remaining (BobguiCssTokenizer *tokenizer)
{
  return tokenizer->end - tokenizer->data;
}

static inline gboolean
bobgui_css_tokenizer_has_valid_escape (BobguiCssTokenizer *tokenizer)
{
  return is_valid_escape (tokenizer->data, tokenizer->end);
}

static gboolean
bobgui_css_tokenizer_has_identifier (BobguiCssTokenizer *tokenizer)
{
  const char *data = tokenizer->data;

  if (data == tokenizer->end)
    return FALSE;

  if (*data == '-')
    {
      data++;
      if (data == tokenizer->end)
        return FALSE;
      if (*data == '-')
        return TRUE;
    }

  if (is_name_start (*data))
    return TRUE;

  if (*data == '\\')
    {
      data++;
      if (data == tokenizer->end)
        return TRUE; /* really? */
      if (is_newline (*data))
        return FALSE;
      return TRUE;
    }

  return FALSE;
}

static gboolean
bobgui_css_tokenizer_has_number (BobguiCssTokenizer *tokenizer)
{
  const char *data = tokenizer->data;

  if (data == tokenizer->end)
    return FALSE;

  if (*data == '-' || *data == '+')
    {
      data++;
      if (data == tokenizer->end)
        return FALSE;
    }

  if (*data == '.')
    {
      data++;
      if (data == tokenizer->end)
        return FALSE;
    }

  return g_ascii_isdigit (*data);
}

static void
bobgui_css_tokenizer_consume_newline (BobguiCssTokenizer *tokenizer)
{
  gsize n;

  if (bobgui_css_tokenizer_remaining (tokenizer) > 1 &&
      tokenizer->data[0] == '\r' && tokenizer->data[1] == '\n')
    n = 2;
  else
    n = 1;
  
  tokenizer->data += n;
  bobgui_css_location_advance_newline (&tokenizer->position, n == 2 ? TRUE : FALSE);
}

static inline void
bobgui_css_tokenizer_consume (BobguiCssTokenizer *tokenizer,
                           gsize            n_bytes,
                           gsize            n_characters)
{
  /* NB: must not contain newlines! */
  tokenizer->data += n_bytes;

  bobgui_css_location_advance (&tokenizer->position, n_bytes, n_characters);
}

static inline void
bobgui_css_tokenizer_consume_ascii (BobguiCssTokenizer *tokenizer)
{
  /* NB: must not contain newlines! */
  bobgui_css_tokenizer_consume (tokenizer, 1, 1);
}

static inline void
bobgui_css_tokenizer_consume_whitespace (BobguiCssTokenizer *tokenizer)
{
  if (is_newline (*tokenizer->data))
    bobgui_css_tokenizer_consume_newline (tokenizer);
  else
    bobgui_css_tokenizer_consume_ascii (tokenizer);
}

static inline void
bobgui_css_tokenizer_consume_char (BobguiCssTokenizer *tokenizer,
                                GString         *string)
{
  if (is_newline (*tokenizer->data))
    bobgui_css_tokenizer_consume_newline (tokenizer);
  else
    {
      gsize char_size = g_utf8_next_char (tokenizer->data) - tokenizer->data;

      if (string)
        g_string_append_len (string, tokenizer->data, char_size);
      bobgui_css_tokenizer_consume (tokenizer, char_size, 1);
    }
}

static void
bobgui_css_tokenizer_read_whitespace (BobguiCssTokenizer *tokenizer,
                                   BobguiCssToken     *token)
{
  do {
    bobgui_css_tokenizer_consume_whitespace (tokenizer);
  } while (tokenizer->data != tokenizer->end &&
           is_whitespace (*tokenizer->data));

  bobgui_css_token_init (token, BOBGUI_CSS_TOKEN_WHITESPACE);
}

static gunichar 
bobgui_css_tokenizer_read_escape (BobguiCssTokenizer *tokenizer)
{
  gunichar value = 0;
  guint i;

  bobgui_css_tokenizer_consume (tokenizer, 1, 1);

  for (i = 0; i < 6 && tokenizer->data < tokenizer->end && g_ascii_isxdigit (*tokenizer->data); i++)
    {
      value = value * 16 + g_ascii_xdigit_value (*tokenizer->data);
      bobgui_css_tokenizer_consume (tokenizer, 1, 1);
    }

  if (i == 0)
    {
      gsize remaining = bobgui_css_tokenizer_remaining (tokenizer);
      if (remaining == 0)
        return 0xFFFD;

      value = g_utf8_get_char_validated (tokenizer->data, remaining);
      if (value == (gunichar) -1 || value == (gunichar) -2)
        value = 0;

      bobgui_css_tokenizer_consume_char (tokenizer, NULL);
    }
  else
    {
      if (is_whitespace (*tokenizer->data))
        bobgui_css_tokenizer_consume_ascii (tokenizer);
    }

  if (!g_unichar_validate (value) || g_unichar_type (value) == G_UNICODE_SURROGATE)
    return 0xFFFD;

  return value;
}

static void
bobgui_css_tokenizer_read_name (BobguiCssTokenizer *tokenizer)
{
  g_string_set_size (tokenizer->name_buffer, 0);

  do {
      if (*tokenizer->data == '\\')
        {
          if (bobgui_css_tokenizer_has_valid_escape (tokenizer))
            {
              gunichar value = bobgui_css_tokenizer_read_escape (tokenizer);
              g_string_append_unichar (tokenizer->name_buffer, value);
            }
          else
            {
              bobgui_css_tokenizer_consume_ascii (tokenizer);

              if (tokenizer->data == tokenizer->end)
                {
                  g_string_append_unichar (tokenizer->name_buffer, 0xFFFD);
                  break;
                }

              bobgui_css_tokenizer_consume_char (tokenizer, tokenizer->name_buffer);
            }
        }
      else if (is_name (*tokenizer->data))
        {
          bobgui_css_tokenizer_consume_char (tokenizer, tokenizer->name_buffer);
        }
      else
        {
          break;
        }
    }
  while (tokenizer->data != tokenizer->end);
}

static void
bobgui_css_tokenizer_read_bad_url (BobguiCssTokenizer  *tokenizer,
                                BobguiCssToken      *token)
{
  while (tokenizer->data < tokenizer->end && *tokenizer->data != ')')
    {
      if (bobgui_css_tokenizer_has_valid_escape (tokenizer))
        bobgui_css_tokenizer_read_escape (tokenizer);
      else
        bobgui_css_tokenizer_consume_char (tokenizer, NULL);
    }
  
  if (tokenizer->data < tokenizer->end)
    bobgui_css_tokenizer_consume_ascii (tokenizer);

  bobgui_css_token_init (token, BOBGUI_CSS_TOKEN_BAD_URL);
}

static gboolean
bobgui_css_tokenizer_read_url (BobguiCssTokenizer  *tokenizer,
                            BobguiCssToken      *token,
                            GError          **error)
{
  GString *url = g_string_new (NULL);

  while (tokenizer->data < tokenizer->end && is_whitespace (*tokenizer->data))
    bobgui_css_tokenizer_consume_whitespace (tokenizer);

  while (tokenizer->data < tokenizer->end)
    {
      if (*tokenizer->data == ')')
        {
          bobgui_css_tokenizer_consume_ascii (tokenizer);
          break;
        }
      else if (is_whitespace (*tokenizer->data))
        {
          do
            bobgui_css_tokenizer_consume_whitespace (tokenizer);
          while (tokenizer->data < tokenizer->end && is_whitespace (*tokenizer->data));
          
          if (*tokenizer->data == ')')
            {
              bobgui_css_tokenizer_consume_ascii (tokenizer);
              break;
            }
          else if (tokenizer->data >= tokenizer->end)
            {
              break;
            }
          else
            {
              bobgui_css_tokenizer_read_bad_url (tokenizer, token);
              bobgui_css_tokenizer_parse_error (error, "Whitespace only allowed at start and end of url");
              g_string_free (url, TRUE);
              return FALSE;
            }
        }
      else if (is_non_printable (*tokenizer->data))
        {
          bobgui_css_tokenizer_read_bad_url (tokenizer, token);
          g_string_free (url, TRUE);
          bobgui_css_tokenizer_parse_error (error, "Nonprintable character 0x%02X in url", *tokenizer->data);
          return FALSE;
        }
      else if (*tokenizer->data == '"' ||
               *tokenizer->data == '\'' ||
               *tokenizer->data == '(')
        {
          bobgui_css_tokenizer_read_bad_url (tokenizer, token);
          bobgui_css_tokenizer_parse_error (error, "Invalid character %c in url", *tokenizer->data);
          g_string_free (url, TRUE);
          return FALSE;
        }
      else if (bobgui_css_tokenizer_has_valid_escape (tokenizer))
        {
          g_string_append_unichar (url, bobgui_css_tokenizer_read_escape (tokenizer));
        }
      else if (*tokenizer->data == '\\')
        {
          bobgui_css_tokenizer_read_bad_url (tokenizer, token);
          bobgui_css_tokenizer_parse_error (error, "Newline may not follow '\' escape character");
          g_string_free (url, TRUE);
          return FALSE;
        }
      else
        {
          bobgui_css_tokenizer_consume_char (tokenizer, url);
        }
    }

  bobgui_css_token_init_string (token, BOBGUI_CSS_TOKEN_URL, url);
  g_string_free (url, TRUE);

  return TRUE;
}

static gboolean
bobgui_css_tokenizer_read_ident_like (BobguiCssTokenizer  *tokenizer,
                                   BobguiCssToken      *token,
                                   GError          **error)
{
  bobgui_css_tokenizer_read_name (tokenizer);

  if (bobgui_css_tokenizer_remaining (tokenizer) > 0 && *tokenizer->data == '(')
    {
      bobgui_css_tokenizer_consume_ascii (tokenizer);
      if (g_ascii_strcasecmp (tokenizer->name_buffer->str, "url") == 0)
        {
          const char *data = tokenizer->data;

          while (is_whitespace (*data))
            data++;

          if (*data != '"' && *data != '\'')
            return bobgui_css_tokenizer_read_url (tokenizer, token, error);
        }

      bobgui_css_token_init_string (token, BOBGUI_CSS_TOKEN_FUNCTION, tokenizer->name_buffer);
      return TRUE;
    }
  else
    {
      bobgui_css_token_init_string (token, BOBGUI_CSS_TOKEN_IDENT, tokenizer->name_buffer);
      return TRUE;
    }
}

static inline double
exp10i (gint64 exp)
{
  switch (exp)
    {
    case -6: return 0.000001;
    case -5: return 0.00001;
    case -4: return 0.0001;
    case -3: return 0.001;
    case -2: return 0.01;
    case -1: return 0.1;
    case 0: return 1;
    case 1: return 10;
    case 2: return 100;
    case 3: return 1000;
    case 4: return 10000;
    case 5: return 100000;
    case 6: return 1000000;
    default: return pow (10, exp);
    }
}

static void
bobgui_css_tokenizer_read_numeric (BobguiCssTokenizer *tokenizer,
                                BobguiCssToken     *token)
{
  int sign = 1, exponent_sign = 1;
  gint64 integer, fractional = 0, fractional_length = 1, exponent = 0;
  gboolean is_int = TRUE, has_sign = FALSE;
  const char *data = tokenizer->data;
  double value;

  if (*data == '-')
    {
      has_sign = TRUE;
      sign = -1;
      data++;
    }
  else if (*data == '+')
    {
      has_sign = TRUE;
      data++;
    }

  for (integer = 0; data < tokenizer->end && g_ascii_isdigit (*data); data++)
    {
      /* check for overflow here? */
      integer = 10 * integer + g_ascii_digit_value (*data);
    }

  if (data + 1 < tokenizer->end && *data == '.' && g_ascii_isdigit (data[1]))
    {
      is_int = FALSE;
      data++;

      fractional = g_ascii_digit_value (*data);
      fractional_length = 10;
      data++;

      while (data < tokenizer->end && g_ascii_isdigit (*data))
        {
          if (fractional_length < G_MAXINT64 / 10)
            {
              fractional = 10 * fractional + g_ascii_digit_value (*data);
              fractional_length *= 10;
            }
          data++;
        }
    }

  if (data + 1 < tokenizer->end && (*data == 'e' || *data == 'E') &&
      (g_ascii_isdigit (data[1]) ||
       (data + 2 < tokenizer->end && (data[1] == '+' || data[1] == '-') && g_ascii_isdigit (data[2]))))
    {
      is_int = FALSE;
      data++;

      if (*data == '-')
        {
          exponent_sign = -1;
          data++;
        }
      else if (*data == '+')
        {
          data++;
        }

      while (data < tokenizer->end && g_ascii_isdigit (*data))
        {
          exponent = 10 * exponent + g_ascii_digit_value (*data);
          data++;
        }
    }

  bobgui_css_tokenizer_consume (tokenizer, data - tokenizer->data, data - tokenizer->data);

  value = sign * (integer + ((double) fractional / fractional_length)) * exp10i (exponent_sign * exponent);

  if (bobgui_css_tokenizer_has_identifier (tokenizer))
    {
      BobguiCssTokenType type;

      if (is_int)
        type = has_sign ? BOBGUI_CSS_TOKEN_SIGNED_INTEGER_DIMENSION : BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER_DIMENSION;
      else
        type = has_sign ? BOBGUI_CSS_TOKEN_SIGNED_DIMENSION : BOBGUI_CSS_TOKEN_SIGNLESS_DIMENSION;

      bobgui_css_tokenizer_read_name (tokenizer);
      bobgui_css_token_init_dimension (token, type, value, tokenizer->name_buffer);
    }
  else if (bobgui_css_tokenizer_remaining (tokenizer) > 0 && *tokenizer->data == '%')
    {
      bobgui_css_token_init_number (token, BOBGUI_CSS_TOKEN_PERCENTAGE, value);
      bobgui_css_tokenizer_consume_ascii (tokenizer);
    }
  else
    {
      BobguiCssTokenType type;

      if (is_int)
        type = has_sign ? BOBGUI_CSS_TOKEN_SIGNED_INTEGER : BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER;
      else
        type = has_sign ? BOBGUI_CSS_TOKEN_SIGNED_NUMBER : BOBGUI_CSS_TOKEN_SIGNLESS_NUMBER;

      bobgui_css_token_init_number (token, type, value);
    }
}

static void
bobgui_css_tokenizer_read_delim (BobguiCssTokenizer *tokenizer,
                              BobguiCssToken     *token)
{
  bobgui_css_token_init_delim (token, g_utf8_get_char (tokenizer->data));
  bobgui_css_tokenizer_consume_char (tokenizer, NULL);
}

static gboolean
bobgui_css_tokenizer_read_dash (BobguiCssTokenizer  *tokenizer,
                             BobguiCssToken      *token,
                             GError          **error)
{
  if (bobgui_css_tokenizer_remaining (tokenizer) == 1)
    {
      bobgui_css_tokenizer_read_delim (tokenizer, token);
      return TRUE;
    }
  else if (bobgui_css_tokenizer_has_number (tokenizer))
    {
      bobgui_css_tokenizer_read_numeric (tokenizer, token);
      return TRUE;
    }
  else if (bobgui_css_tokenizer_remaining (tokenizer) >= 3 &&
           tokenizer->data[1] == '-' &&
           tokenizer->data[2] == '>')
    {
      bobgui_css_token_init (token, BOBGUI_CSS_TOKEN_CDC);
      bobgui_css_tokenizer_consume (tokenizer, 3, 3);
      return TRUE;
    }
  else if (bobgui_css_tokenizer_has_identifier (tokenizer))
    {
      return bobgui_css_tokenizer_read_ident_like (tokenizer, token, error);
    }
  else
    {
      bobgui_css_tokenizer_read_delim (tokenizer, token);
      return TRUE;
    }
}

static gboolean
bobgui_css_tokenizer_read_string (BobguiCssTokenizer  *tokenizer,
                               BobguiCssToken      *token,
                               GError          **error)
{
  g_string_set_size (tokenizer->name_buffer, 0);

  char end = *tokenizer->data;

  bobgui_css_tokenizer_consume_ascii (tokenizer);

  while (tokenizer->data < tokenizer->end)
    {
      gsize n_characters = 0;
      const char *data;

      for (data = tokenizer->data;
           data < tokenizer->end &&
           *data != end &&
           *data != '\\' &&
           !is_newline (*data);
           data = g_utf8_next_char (data))
        {
          n_characters++;
        }
      if (data > tokenizer->data)
        {
          g_string_append_len (tokenizer->name_buffer, tokenizer->data, data - tokenizer->data);
          bobgui_css_tokenizer_consume (tokenizer, data - tokenizer->data, n_characters);
          if (tokenizer->data >= tokenizer->end)
            break;
        }

      if (*tokenizer->data == end)
        {
          bobgui_css_tokenizer_consume_ascii (tokenizer);
          break;
        }
      else if (*tokenizer->data == '\\')
        {
          if (bobgui_css_tokenizer_remaining (tokenizer) == 1)
            {
              bobgui_css_tokenizer_consume_ascii (tokenizer);
              break;
            }
          else if (is_newline (tokenizer->data[1]))
            {
              bobgui_css_tokenizer_consume_ascii (tokenizer);
              bobgui_css_tokenizer_consume_newline (tokenizer);
            }
          else
            {
              g_string_append_unichar (tokenizer->name_buffer, bobgui_css_tokenizer_read_escape (tokenizer));
            }
        }
      else if (is_newline (*tokenizer->data))
        {
          bobgui_css_token_init (token, BOBGUI_CSS_TOKEN_BAD_STRING);
          bobgui_css_tokenizer_parse_error (error, "Newlines inside strings must be escaped");
          return FALSE;
        }
      else
        {
          bobgui_css_tokenizer_consume_char (tokenizer, tokenizer->name_buffer);
        }
    }

  bobgui_css_token_init_string (token, BOBGUI_CSS_TOKEN_STRING, tokenizer->name_buffer);

  return TRUE;
}

static gboolean
bobgui_css_tokenizer_read_comment (BobguiCssTokenizer  *tokenizer,
                                BobguiCssToken      *token,
                                GError          **error)
{
  bobgui_css_tokenizer_consume (tokenizer, 2, 2);

  while (tokenizer->data < tokenizer->end)
    {
      if (bobgui_css_tokenizer_remaining (tokenizer) > 1 &&
          tokenizer->data[0] == '*' && tokenizer->data[1] == '/')
        {
          bobgui_css_tokenizer_consume (tokenizer, 2, 2);
          bobgui_css_token_init (token, BOBGUI_CSS_TOKEN_COMMENT);
          return TRUE;
        }
      bobgui_css_tokenizer_consume_char (tokenizer, NULL);
    }

  bobgui_css_token_init (token, BOBGUI_CSS_TOKEN_COMMENT);
  bobgui_css_tokenizer_parse_error (error, "Comment not terminated at end of document.");
  return FALSE;
}

static void
bobgui_css_tokenizer_read_match (BobguiCssTokenizer *tokenizer,
                              BobguiCssToken     *token,
                              BobguiCssTokenType  type)
{
  if (bobgui_css_tokenizer_remaining (tokenizer) > 1 && tokenizer->data[1] == '=')
    {
      bobgui_css_token_init (token, type);
      bobgui_css_tokenizer_consume (tokenizer, 2, 2);
    }
  else
    {
      bobgui_css_tokenizer_read_delim (tokenizer, token);
    }
}

gboolean
bobgui_css_tokenizer_read_token (BobguiCssTokenizer  *tokenizer,
                              BobguiCssToken      *token,
                              GError          **error)
{
  if (tokenizer->data == tokenizer->end)
    {
      bobgui_css_token_init (token, BOBGUI_CSS_TOKEN_EOF);
      return TRUE;
    }

  if (tokenizer->data[0] == '/' && bobgui_css_tokenizer_remaining (tokenizer) > 1 &&
      tokenizer->data[1] == '*')
    return bobgui_css_tokenizer_read_comment (tokenizer, token, error);

  switch (*tokenizer->data)
    {
    case '\n':
    case '\r':
    case '\t':
    case '\f':
    case ' ':
      bobgui_css_tokenizer_read_whitespace (tokenizer, token);
      return TRUE;

    case '"':
      return bobgui_css_tokenizer_read_string (tokenizer, token, error);

    case '#':
      bobgui_css_tokenizer_consume_ascii (tokenizer);
      if (bobgui_css_tokenizer_remaining (tokenizer) > 0 &&
          (is_name (*tokenizer->data) || bobgui_css_tokenizer_has_valid_escape (tokenizer)))
        {
          BobguiCssTokenType type;

          if (bobgui_css_tokenizer_has_identifier (tokenizer))
            type = BOBGUI_CSS_TOKEN_HASH_ID;
          else
            type = BOBGUI_CSS_TOKEN_HASH_UNRESTRICTED;

          bobgui_css_tokenizer_read_name (tokenizer);
          bobgui_css_token_init_string (token, type, tokenizer->name_buffer);
        }
      else
        {
          bobgui_css_token_init_delim (token, '#');
        }
      return TRUE;

    case '$':
      bobgui_css_tokenizer_read_match (tokenizer, token, BOBGUI_CSS_TOKEN_SUFFIX_MATCH);
      return TRUE;

    case '\'':
      return bobgui_css_tokenizer_read_string (tokenizer, token, error);

    case '(':
      bobgui_css_token_init (token, BOBGUI_CSS_TOKEN_OPEN_PARENS);
      bobgui_css_tokenizer_consume_ascii (tokenizer);
      return TRUE;

    case ')':
      bobgui_css_token_init (token, BOBGUI_CSS_TOKEN_CLOSE_PARENS);
      bobgui_css_tokenizer_consume_ascii (tokenizer);
      return TRUE;

    case '*':
      bobgui_css_tokenizer_read_match (tokenizer, token, BOBGUI_CSS_TOKEN_SUBSTRING_MATCH);
      return TRUE;

    case '+':
      if (bobgui_css_tokenizer_has_number (tokenizer))
        bobgui_css_tokenizer_read_numeric (tokenizer, token);
      else
        bobgui_css_tokenizer_read_delim (tokenizer, token);
      return TRUE;

    case ',':
      bobgui_css_token_init (token, BOBGUI_CSS_TOKEN_COMMA);
      bobgui_css_tokenizer_consume_ascii (tokenizer);
      return TRUE;

    case '-':
      return bobgui_css_tokenizer_read_dash (tokenizer, token, error);

    case '.':
      if (bobgui_css_tokenizer_has_number (tokenizer))
        bobgui_css_tokenizer_read_numeric (tokenizer, token);
      else
        bobgui_css_tokenizer_read_delim (tokenizer, token);
      return TRUE;

    case ':':
      bobgui_css_token_init (token, BOBGUI_CSS_TOKEN_COLON);
      bobgui_css_tokenizer_consume_ascii (tokenizer);
      return TRUE;

    case ';':
      bobgui_css_token_init (token, BOBGUI_CSS_TOKEN_SEMICOLON);
      bobgui_css_tokenizer_consume_ascii (tokenizer);
      return TRUE;

    case '<':
      if (bobgui_css_tokenizer_remaining (tokenizer) >= 4 &&
          tokenizer->data[1] == '!' &&
          tokenizer->data[2] == '-' &&
          tokenizer->data[3] == '-')
        {
          bobgui_css_token_init (token, BOBGUI_CSS_TOKEN_CDO);
          bobgui_css_tokenizer_consume (tokenizer, 4, 4);
        }
      else
        {
          bobgui_css_tokenizer_read_delim (tokenizer, token);
        }
      return TRUE;

    case '@':
      bobgui_css_tokenizer_consume_ascii (tokenizer);
      if (bobgui_css_tokenizer_has_identifier (tokenizer))
        {
          bobgui_css_tokenizer_read_name (tokenizer);
          bobgui_css_token_init_string (token, BOBGUI_CSS_TOKEN_AT_KEYWORD, tokenizer->name_buffer);
        }
      else
        {
          bobgui_css_token_init_delim (token, '@');
        }
      return TRUE;

    case '[':
      bobgui_css_token_init (token, BOBGUI_CSS_TOKEN_OPEN_SQUARE);
      bobgui_css_tokenizer_consume_ascii (tokenizer);
      return TRUE;

    case '\\':
      if (bobgui_css_tokenizer_has_valid_escape (tokenizer))
        {
          return bobgui_css_tokenizer_read_ident_like (tokenizer, token, error);
        }
      else
        {
          bobgui_css_token_init_delim (token, '\\');
          bobgui_css_tokenizer_consume_ascii (tokenizer);
          bobgui_css_tokenizer_parse_error (error, "Newline may not follow '\' escape character");
          return FALSE;
        }

    case ']':
      bobgui_css_token_init (token, BOBGUI_CSS_TOKEN_CLOSE_SQUARE);
      bobgui_css_tokenizer_consume_ascii (tokenizer);
      return TRUE;

    case '^':
      bobgui_css_tokenizer_read_match (tokenizer, token, BOBGUI_CSS_TOKEN_PREFIX_MATCH);
      return TRUE;

    case '{':
      bobgui_css_token_init (token, BOBGUI_CSS_TOKEN_OPEN_CURLY);
      bobgui_css_tokenizer_consume_ascii (tokenizer);
      return TRUE;

    case '}':
      bobgui_css_token_init (token, BOBGUI_CSS_TOKEN_CLOSE_CURLY);
      bobgui_css_tokenizer_consume_ascii (tokenizer);
      return TRUE;

    case '|':
      if (bobgui_css_tokenizer_remaining (tokenizer) > 1 && tokenizer->data[1] == '|')
        {
          bobgui_css_token_init (token, BOBGUI_CSS_TOKEN_COLUMN);
          bobgui_css_tokenizer_consume (tokenizer, 2, 2);
        }
      else
        {
          bobgui_css_tokenizer_read_match (tokenizer, token, BOBGUI_CSS_TOKEN_DASH_MATCH);
        }
      return TRUE;

    case '~':
      bobgui_css_tokenizer_read_match (tokenizer, token, BOBGUI_CSS_TOKEN_INCLUDE_MATCH);
      return TRUE;

    default:
      if (g_ascii_isdigit (*tokenizer->data))
        {
          bobgui_css_tokenizer_read_numeric (tokenizer, token);
          return TRUE;
        }
      else if (is_name_start (*tokenizer->data))
        {
          return bobgui_css_tokenizer_read_ident_like (tokenizer, token, error);
        }
      else
        {
          bobgui_css_tokenizer_read_delim (tokenizer, token);
          return TRUE;
        }
    }
}

void
bobgui_css_tokenizer_save (BobguiCssTokenizer *tokenizer)
{
  g_assert (!tokenizer->saved_data);

  tokenizer->saved_position = tokenizer->position;
  tokenizer->saved_data = tokenizer->data;
}

void
bobgui_css_tokenizer_restore (BobguiCssTokenizer *tokenizer)
{
  g_assert (tokenizer->saved_data);

  tokenizer->position = tokenizer->saved_position;
  tokenizer->data = tokenizer->saved_data;

  bobgui_css_location_init (&tokenizer->saved_position);
  tokenizer->saved_data = NULL;
}
