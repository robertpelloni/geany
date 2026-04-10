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

#pragma once

#include <glib.h>

#include <bobgui/css/bobguicsslocation.h>

G_BEGIN_DECLS

typedef enum {
  /* no content */
  BOBGUI_CSS_TOKEN_EOF,
  BOBGUI_CSS_TOKEN_WHITESPACE,
  BOBGUI_CSS_TOKEN_OPEN_PARENS,
  BOBGUI_CSS_TOKEN_CLOSE_PARENS,
  BOBGUI_CSS_TOKEN_OPEN_SQUARE,
  BOBGUI_CSS_TOKEN_CLOSE_SQUARE,
  BOBGUI_CSS_TOKEN_OPEN_CURLY,
  BOBGUI_CSS_TOKEN_CLOSE_CURLY,
  BOBGUI_CSS_TOKEN_COMMA,
  BOBGUI_CSS_TOKEN_COLON,
  BOBGUI_CSS_TOKEN_SEMICOLON,
  BOBGUI_CSS_TOKEN_CDO,
  BOBGUI_CSS_TOKEN_CDC,
  BOBGUI_CSS_TOKEN_INCLUDE_MATCH,
  BOBGUI_CSS_TOKEN_DASH_MATCH,
  BOBGUI_CSS_TOKEN_PREFIX_MATCH,
  BOBGUI_CSS_TOKEN_SUFFIX_MATCH,
  BOBGUI_CSS_TOKEN_SUBSTRING_MATCH,
  BOBGUI_CSS_TOKEN_COLUMN,
  BOBGUI_CSS_TOKEN_BAD_STRING,
  BOBGUI_CSS_TOKEN_BAD_URL,
  BOBGUI_CSS_TOKEN_COMMENT,
  /* delim */
  BOBGUI_CSS_TOKEN_DELIM,
  /* string */
  BOBGUI_CSS_TOKEN_STRING,
  BOBGUI_CSS_TOKEN_IDENT,
  BOBGUI_CSS_TOKEN_FUNCTION,
  BOBGUI_CSS_TOKEN_AT_KEYWORD,
  BOBGUI_CSS_TOKEN_HASH_UNRESTRICTED,
  BOBGUI_CSS_TOKEN_HASH_ID,
  BOBGUI_CSS_TOKEN_URL,
  /* number */
  BOBGUI_CSS_TOKEN_SIGNED_INTEGER,
  BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER,
  BOBGUI_CSS_TOKEN_SIGNED_NUMBER,
  BOBGUI_CSS_TOKEN_SIGNLESS_NUMBER,
  BOBGUI_CSS_TOKEN_PERCENTAGE,
  /* dimension */
  BOBGUI_CSS_TOKEN_SIGNED_INTEGER_DIMENSION,
  BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER_DIMENSION,
  BOBGUI_CSS_TOKEN_SIGNED_DIMENSION,
  BOBGUI_CSS_TOKEN_SIGNLESS_DIMENSION
} BobguiCssTokenType;

typedef union _BobguiCssToken BobguiCssToken;
typedef struct _BobguiCssTokenizer BobguiCssTokenizer;

typedef struct _BobguiCssStringToken BobguiCssStringToken;
typedef struct _BobguiCssDelimToken BobguiCssDelimToken;
typedef struct _BobguiCssNumberToken BobguiCssNumberToken;
typedef struct _BobguiCssDimensionToken BobguiCssDimensionToken;

struct _BobguiCssStringToken {
  BobguiCssTokenType  type;
  char            *string;
  char             buf[16];
};

struct _BobguiCssDelimToken {
  BobguiCssTokenType  type;
  gunichar         delim;
};

struct _BobguiCssNumberToken {
  BobguiCssTokenType  type;
  double           number;
};

struct _BobguiCssDimensionToken {
  BobguiCssTokenType  type;
  double           value;
  char             dimension[8];
};

union _BobguiCssToken {
  BobguiCssTokenType type;
  BobguiCssStringToken string;
  BobguiCssDelimToken delim;
  BobguiCssNumberToken number;
  BobguiCssDimensionToken dimension;
};

static inline const char *
bobgui_css_token_get_string (const BobguiCssToken *token)
{
  return token->string.string;
}

void                    bobgui_css_token_clear                     (BobguiCssToken            *token);

gboolean                bobgui_css_token_is_finite                 (const BobguiCssToken      *token) G_GNUC_PURE;
gboolean                bobgui_css_token_is_preserved              (const BobguiCssToken      *token,
                                                                 BobguiCssTokenType        *out_closing) G_GNUC_PURE;
#define bobgui_css_token_is(token, _type) ((token)->type == (_type))
gboolean                bobgui_css_token_is_ident                  (const BobguiCssToken      *token,
                                                                 const char             *ident) G_GNUC_PURE;
gboolean                bobgui_css_token_is_function               (const BobguiCssToken      *token,
                                                                 const char             *ident) G_GNUC_PURE;
gboolean                bobgui_css_token_is_delim                  (const BobguiCssToken      *token,
                                                                 gunichar                delim) G_GNUC_PURE;

void                    bobgui_css_token_print                     (const BobguiCssToken      *token,
                                                                 GString                *string);
char *                  bobgui_css_token_to_string                 (const BobguiCssToken      *token);

BobguiCssTokenizer *       bobgui_css_tokenizer_new                   (GBytes                 *bytes);
BobguiCssTokenizer *       bobgui_css_tokenizer_new_for_range         (GBytes                 *bytes,
                                                                 gsize                   offset,
                                                                 gsize                   length);

BobguiCssTokenizer *       bobgui_css_tokenizer_ref                   (BobguiCssTokenizer        *tokenizer);
void                    bobgui_css_tokenizer_unref                 (BobguiCssTokenizer        *tokenizer);

GBytes *                bobgui_css_tokenizer_get_bytes             (BobguiCssTokenizer        *tokenizer);
const BobguiCssLocation *  bobgui_css_tokenizer_get_location          (BobguiCssTokenizer        *tokenizer) G_GNUC_CONST;

gboolean                bobgui_css_tokenizer_read_token            (BobguiCssTokenizer        *tokenizer,
                                                                 BobguiCssToken            *token,
                                                                 GError                **error);

void                     bobgui_css_tokenizer_save                 (BobguiCssTokenizer        *tokenizer);
void                     bobgui_css_tokenizer_restore              (BobguiCssTokenizer        *tokenizer);

G_END_DECLS

