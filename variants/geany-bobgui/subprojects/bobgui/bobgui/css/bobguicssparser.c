/*
 * Copyright © 2019 Benjamin Otte
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
 * Authors: Benjamin Otte <otte@gnome.org>
 */


#include "config.h"

#include "bobguicssparserprivate.h"

#include "bobguicssenums.h"
#include "bobguicsserror.h"
#include "bobguicsslocationprivate.h"

/* We cannot include bobguidebug.h, so we must keep this in sync */
extern unsigned int bobgui_get_debug_flags (void);
#define DEBUG_CHECK_CSS ((bobgui_get_debug_flags () & BOBGUI_CSS_PARSER_DEBUG_CSS) != 0)

static void clear_ref (BobguiCssVariableValueReference *ref);

#define GDK_ARRAY_NAME bobgui_css_parser_references
#define GDK_ARRAY_TYPE_NAME BobguiCssParserReferences
#define GDK_ARRAY_ELEMENT_TYPE BobguiCssVariableValueReference
#define GDK_ARRAY_BY_VALUE 1
#define GDK_ARRAY_NO_MEMSET 1
#define GDK_ARRAY_FREE_FUNC clear_ref
#include "gdk/gdkarrayimpl.c"

typedef struct _BobguiCssParserBlock BobguiCssParserBlock;

struct _BobguiCssParserBlock
{
  BobguiCssLocation start_location;
  BobguiCssTokenType end_token;
  BobguiCssTokenType inherited_end_token;
  BobguiCssTokenType alternative_token;
};

#define GDK_ARRAY_NAME bobgui_css_parser_blocks
#define GDK_ARRAY_TYPE_NAME BobguiCssParserBlocks
#define GDK_ARRAY_ELEMENT_TYPE BobguiCssParserBlock
#define GDK_ARRAY_PREALLOC 12
#define GDK_ARRAY_NO_MEMSET 1
#include "gdk/gdkarrayimpl.c"

static inline BobguiCssParserBlock *
bobgui_css_parser_blocks_get_last (BobguiCssParserBlocks *blocks)
{
  return bobgui_css_parser_blocks_index (blocks, bobgui_css_parser_blocks_get_size (blocks) - 1);
}

static inline void
bobgui_css_parser_blocks_drop_last (BobguiCssParserBlocks *blocks)
{
  bobgui_css_parser_blocks_set_size (blocks, bobgui_css_parser_blocks_get_size (blocks) - 1);
}

typedef struct _BobguiCssTokenizerData BobguiCssTokenizerData;

struct _BobguiCssTokenizerData
{
  BobguiCssTokenizer *tokenizer;
  char *var_name;
  BobguiCssVariableValue *variable;
};

static void
bobgui_css_tokenizer_data_clear (gpointer data)
{
  BobguiCssTokenizerData *td = data;

  bobgui_css_tokenizer_unref (td->tokenizer);
  if (td->var_name)
    g_free (td->var_name);
  if (td->variable)
    bobgui_css_variable_value_unref (td->variable);
}

#define GDK_ARRAY_NAME bobgui_css_tokenizers
#define GDK_ARRAY_TYPE_NAME BobguiCssTokenizers
#define GDK_ARRAY_ELEMENT_TYPE BobguiCssTokenizerData
#define GDK_ARRAY_FREE_FUNC bobgui_css_tokenizer_data_clear
#define GDK_ARRAY_BY_VALUE 1
#define GDK_ARRAY_PREALLOC 16
#define GDK_ARRAY_NO_MEMSET 1
#include "gdk/gdkarrayimpl.c"

static inline BobguiCssTokenizerData *
bobgui_css_tokenizers_get_last (BobguiCssTokenizers *tokenizers)
{
  return bobgui_css_tokenizers_index (tokenizers, bobgui_css_tokenizers_get_size (tokenizers) - 1);
}

static inline void
bobgui_css_tokenizers_drop_last (BobguiCssTokenizers *tokenizers)
{
  bobgui_css_tokenizers_set_size (tokenizers, bobgui_css_tokenizers_get_size (tokenizers) - 1);
}

struct _BobguiCssParser
{
  volatile int ref_count;

  BobguiCssTokenizers tokenizers;
  GFile *file;
  GFile *directory;
  BobguiCssParserErrorFunc error_func;
  gpointer user_data;
  GDestroyNotify user_destroy;

  BobguiCssParserBlocks blocks;
  BobguiCssLocation location;
  BobguiCssToken token;

  BobguiCssVariableValue **refs;
  gsize n_refs;
  gsize next_ref;
  gboolean var_fallback;
};

static inline BobguiCssTokenizer *
get_tokenizer (BobguiCssParser *self)
{
  return bobgui_css_tokenizers_get_last (&self->tokenizers)->tokenizer;
}

static BobguiCssParser *
bobgui_css_parser_new (BobguiCssTokenizer       *tokenizer,
                    BobguiCssVariableValue   *value,
                    GFile                 *file,
                    BobguiCssParserErrorFunc  error_func,
                    gpointer               user_data,
                    GDestroyNotify         user_destroy)
{
  BobguiCssParser *self;

  self = g_new0 (BobguiCssParser, 1);

  self->ref_count = 1;

  bobgui_css_tokenizers_init (&self->tokenizers);
  bobgui_css_tokenizers_append (&self->tokenizers,
                             &(BobguiCssTokenizerData) {
                               bobgui_css_tokenizer_ref (tokenizer),
                               NULL,
                               value ? bobgui_css_variable_value_ref (value) : NULL });

  if (file)
    self->file = g_object_ref (file);

  self->error_func = error_func;
  self->user_data = user_data;
  self->user_destroy = user_destroy;
  bobgui_css_parser_blocks_init (&self->blocks);

  return self;
}

BobguiCssParser *
bobgui_css_parser_new_for_file (GFile                 *file,
                             BobguiCssParserErrorFunc  error_func,
                             gpointer               user_data,
                             GDestroyNotify         user_destroy,
                             GError               **error)
{
  GBytes *bytes;
  BobguiCssParser *result;

  bytes = g_file_load_bytes (file, NULL, NULL, error);
  if (bytes == NULL)
    return NULL;

  result = bobgui_css_parser_new_for_bytes (bytes, file, error_func, user_data, user_destroy);

  g_bytes_unref (bytes);

  return result;
}

BobguiCssParser *
bobgui_css_parser_new_for_bytes (GBytes                *bytes,
                              GFile                 *file,
                              BobguiCssParserErrorFunc  error_func,
                              gpointer               user_data,
                              GDestroyNotify         user_destroy)
{
  BobguiCssTokenizer *tokenizer;
  BobguiCssParser *result;

  tokenizer = bobgui_css_tokenizer_new (bytes);
  result = bobgui_css_parser_new (tokenizer, NULL, file, error_func, user_data, user_destroy);
  bobgui_css_tokenizer_unref (tokenizer);

  return result;
}

BobguiCssParser *
bobgui_css_parser_new_for_token_stream (BobguiCssVariableValue    *value,
                                     GFile                  *file,
                                     BobguiCssVariableValue   **refs,
                                     gsize                   n_refs,
                                     BobguiCssParserErrorFunc   error_func,
                                     gpointer                user_data,
                                     GDestroyNotify          user_destroy)
{
  BobguiCssTokenizer *tokenizer;
  BobguiCssParser *result;

  tokenizer = bobgui_css_tokenizer_new_for_range (value->bytes, value->offset,
                                               value->end_offset - value->offset);
  result = bobgui_css_parser_new (tokenizer, value, file, error_func, user_data, user_destroy);
  bobgui_css_tokenizer_unref (tokenizer);

  result->refs = refs;
  result->n_refs = n_refs;
  result->next_ref = 0;

  return result;
}

static void
bobgui_css_parser_finalize (BobguiCssParser *self)
{
  if (self->user_destroy)
    self->user_destroy (self->user_data);

  bobgui_css_tokenizers_clear (&self->tokenizers);
  g_clear_object (&self->file);
  g_clear_object (&self->directory);
  if (bobgui_css_parser_blocks_get_size (&self->blocks) > 0)
    g_critical ("Finalizing CSS parser with %" G_GSIZE_FORMAT " remaining blocks", bobgui_css_parser_blocks_get_size (&self->blocks));
  bobgui_css_parser_blocks_clear (&self->blocks);

  g_free (self);
}

BobguiCssParser *
bobgui_css_parser_ref (BobguiCssParser *self)
{
  g_atomic_int_inc (&self->ref_count);

  return self;
}

void
bobgui_css_parser_unref (BobguiCssParser *self)
{
  if (g_atomic_int_dec_and_test (&self->ref_count))
    bobgui_css_parser_finalize (self);
}

/**
 * bobgui_css_parser_get_file:
 * @self: a `BobguiCssParser`
 *
 * Gets the file being parsed. If no file is associated with @self -
 * for example when raw data is parsed - %NULL is returned.
 *
 * Returns: (nullable) (transfer none): The file being parsed
 */
GFile *
bobgui_css_parser_get_file (BobguiCssParser *self)
{
  return self->file;
}

GBytes *
bobgui_css_parser_get_bytes (BobguiCssParser *self)
{
  return bobgui_css_tokenizer_get_bytes (bobgui_css_tokenizers_get (&self->tokenizers, 0)->tokenizer);
}

/**
 * bobgui_css_parser_resolve_url:
 * @self: a `BobguiCssParser`
 * @url: the URL to resolve
 *
 * Resolves a given URL against the parser's location.
 *
 * Returns: (nullable) (transfer full): a new `GFile` for the
 *   resolved URL
 */
GFile *
bobgui_css_parser_resolve_url (BobguiCssParser *self,
                            const char   *url)
{
  char *scheme;

  scheme = g_uri_parse_scheme (url);
  if (scheme != NULL)
    {
      GFile *file = g_file_new_for_uri (url);
      g_free (scheme);
      return file;
    }

  if (self->directory == NULL)
    {
      if (self->file)
        self->directory = g_file_get_parent (self->file);
      if (self->directory == NULL)
        return NULL;
    }

  return g_file_resolve_relative_path (self->directory, url);
}

/**
 * bobgui_css_parser_get_start_location:
 * @self: a `BobguiCssParser`
 *
 * Queries the location of the current token.
 *
 * This function will return the location of the start of the
 * current token. In the case a token has been consumed, but no
 * new token has been queried yet via bobgui_css_parser_peek_token()
 * or bobgui_css_parser_get_token(), the previous token's start
 * location will be returned.
 *
 * This function may return the same location as
 * bobgui_css_parser_get_end_location() - in particular at the
 * beginning and end of the document.
 *
 * Returns: the start location
 **/
const BobguiCssLocation *
bobgui_css_parser_get_start_location (BobguiCssParser *self)
{
  return &self->location;
}

/**
 * bobgui_css_parser_get_end_location:
 * @self: a `BobguiCssParser`
 * @out_location: (caller-allocates) Place to store the location
 *
 * Queries the location of the current token.
 *
 * This function will return the location of the end of the
 * current token. In the case a token has been consumed, but no
 * new token has been queried yet via bobgui_css_parser_peek_token()
 * or bobgui_css_parser_get_token(), the previous token's end location
 * will be returned.
 *
 * This function may return the same location as
 * bobgui_css_parser_get_start_location() - in particular at the
 * beginning and end of the document.
 *
 * Returns: the end location
 **/
const BobguiCssLocation *
bobgui_css_parser_get_end_location (BobguiCssParser *self)
{
  return bobgui_css_tokenizer_get_location (get_tokenizer (self));
}

/**
 * bobgui_css_parser_get_block_location:
 * @self: a `BobguiCssParser`
 *
 * Queries the start location of the token that started the current
 * block that is being parsed.
 *
 * If no block is currently parsed, the beginning of the document
 * is returned.
 *
 * Returns: The start location of the current block
 */
const BobguiCssLocation *
bobgui_css_parser_get_block_location (BobguiCssParser *self)
{
  const BobguiCssParserBlock *block;

  if (bobgui_css_parser_blocks_get_size (&self->blocks) == 0)
    {
      static const BobguiCssLocation start_of_document = { 0, };
      return &start_of_document;
    }

  block = bobgui_css_parser_blocks_get_last (&self->blocks);
  return &block->start_location;
}

static void
bobgui_css_parser_ensure_token (BobguiCssParser *self)
{
  GError *error = NULL;
  BobguiCssTokenizer *tokenizer;

  if (!bobgui_css_token_is (&self->token, BOBGUI_CSS_TOKEN_EOF))
    return;

  tokenizer = get_tokenizer (self);
  self->location = *bobgui_css_tokenizer_get_location (tokenizer);
  if (!bobgui_css_tokenizer_read_token (tokenizer, &self->token, &error))
    {
      /* We ignore the error here, because the resulting token will
       * likely already trigger an error in the parsing code and
       * duplicate errors are rather useless.
       */
      g_clear_error (&error);
    }

  if (bobgui_css_tokenizers_get_size (&self->tokenizers) > 1 && bobgui_css_token_is (&self->token, BOBGUI_CSS_TOKEN_EOF))
    {
      bobgui_css_tokenizers_drop_last (&self->tokenizers);
      bobgui_css_parser_ensure_token (self);
      return;
    }

  /* Resolve var(--name): skip it and insert the resolved reference instead */
  if (self->n_refs > 0 && bobgui_css_token_is_function (&self->token, "var") && self->var_fallback == 0)
    {
      BobguiCssVariableValue *ref;
      BobguiCssTokenizer *ref_tokenizer;

      bobgui_css_parser_start_block (self);

      g_assert (bobgui_css_parser_has_token (self, BOBGUI_CSS_TOKEN_IDENT));

      char *var_name = bobgui_css_parser_consume_ident (self);
      g_assert (var_name[0] == '-' && var_name[1] == '-');

      /* If we encounter var() in a fallback when we can already resolve the
       * actual variable, skip it */
      self->var_fallback++;
      bobgui_css_parser_skip (self);
      bobgui_css_parser_end_block (self);
      self->var_fallback--;

      g_assert (self->next_ref < self->n_refs);

      ref = self->refs[self->next_ref++];
      ref_tokenizer = bobgui_css_tokenizer_new_for_range (ref->bytes, ref->offset,
                                                       ref->end_offset - ref->offset);
      bobgui_css_tokenizers_append (&self->tokenizers,
                                 &(BobguiCssTokenizerData) {
                                   ref_tokenizer,
                                   g_strdup (var_name),
                                   bobgui_css_variable_value_ref (ref)
                                 });

      bobgui_css_parser_ensure_token (self);
      g_free (var_name);
    }
}

const BobguiCssToken *
bobgui_css_parser_peek_token (BobguiCssParser *self)
{
  static const BobguiCssToken eof_token = { BOBGUI_CSS_TOKEN_EOF, };

  bobgui_css_parser_ensure_token (self);

  if (bobgui_css_parser_blocks_get_size (&self->blocks) > 0)
    {
      const BobguiCssParserBlock *block = bobgui_css_parser_blocks_get_last (&self->blocks);
      if (bobgui_css_token_is (&self->token, block->end_token) ||
          bobgui_css_token_is (&self->token, block->inherited_end_token) ||
          bobgui_css_token_is (&self->token, block->alternative_token))
        return &eof_token;
    }

  return &self->token;
}

const BobguiCssToken *
bobgui_css_parser_get_token (BobguiCssParser *self)
{
  const BobguiCssToken *token;

  for (token = bobgui_css_parser_peek_token (self);
       bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_COMMENT) ||
       bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_WHITESPACE);
       token = bobgui_css_parser_peek_token (self))
    {
      bobgui_css_parser_consume_token (self);
    }

  return token;
}

void
bobgui_css_parser_consume_token (BobguiCssParser *self)
{
  bobgui_css_parser_ensure_token (self);

  /* unpreserved tokens MUST be consumed via start_block() */
  g_assert (bobgui_css_token_is_preserved (&self->token, NULL));

  /* Don't consume any tokens at the end of a block */
  if (!bobgui_css_token_is (bobgui_css_parser_peek_token (self), BOBGUI_CSS_TOKEN_EOF))
    bobgui_css_token_clear (&self->token);
}

void
bobgui_css_parser_start_block (BobguiCssParser *self)
{
  BobguiCssParserBlock block;

  bobgui_css_parser_ensure_token (self);

  if (bobgui_css_token_is_preserved (&self->token, &block.end_token))
    {
      g_critical ("bobgui_css_parser_start_block() may only be called for non-preserved tokens");
      return;
    }

  block.inherited_end_token = BOBGUI_CSS_TOKEN_EOF;
  block.alternative_token = BOBGUI_CSS_TOKEN_EOF;
  block.start_location = self->location;
  bobgui_css_parser_blocks_append (&self->blocks, block);

  bobgui_css_token_clear (&self->token);
}

void
bobgui_css_parser_start_semicolon_block (BobguiCssParser    *self,
                                      BobguiCssTokenType  alternative_token)
{
  BobguiCssParserBlock block;

  block.end_token = BOBGUI_CSS_TOKEN_SEMICOLON;
  if (bobgui_css_parser_blocks_get_size (&self->blocks) > 0)
    block.inherited_end_token = bobgui_css_parser_blocks_get_last (&self->blocks)->end_token;
  else
    block.inherited_end_token = BOBGUI_CSS_TOKEN_EOF;
  block.alternative_token = alternative_token;
  block.start_location = self->location;
  bobgui_css_parser_blocks_append (&self->blocks, block);
}

void
bobgui_css_parser_end_block_prelude (BobguiCssParser *self)
{
  BobguiCssParserBlock *block;

  g_return_if_fail (bobgui_css_parser_blocks_get_size (&self->blocks) > 0);

  block = bobgui_css_parser_blocks_get_last (&self->blocks);

  if (block->alternative_token == BOBGUI_CSS_TOKEN_EOF)
    return;

  bobgui_css_parser_skip_until (self, BOBGUI_CSS_TOKEN_EOF);

  if (bobgui_css_token_is (&self->token, block->alternative_token))
    {
      if (bobgui_css_token_is_preserved (&self->token, &block->end_token))
        {
          g_critical ("alternative token is not preserved");
          return;
        }
      block->alternative_token = BOBGUI_CSS_TOKEN_EOF;
      block->inherited_end_token = BOBGUI_CSS_TOKEN_EOF;
      bobgui_css_token_clear (&self->token);
    }
}

void
bobgui_css_parser_end_block (BobguiCssParser *self)
{
  BobguiCssParserBlock *block;

  g_return_if_fail (bobgui_css_parser_blocks_get_size (&self->blocks) > 0);

  bobgui_css_parser_skip_until (self, BOBGUI_CSS_TOKEN_EOF);

  block = bobgui_css_parser_blocks_get_last (&self->blocks);

  if (bobgui_css_token_is (&self->token, BOBGUI_CSS_TOKEN_EOF))
    {
      bobgui_css_parser_warn (self,
                           BOBGUI_CSS_PARSER_WARNING_SYNTAX,
                           bobgui_css_parser_get_block_location (self),
                           bobgui_css_parser_get_start_location (self),
                           "Unterminated block at end of document");
      bobgui_css_parser_blocks_drop_last (&self->blocks);
    }
  else if (bobgui_css_token_is (&self->token, block->inherited_end_token))
    {
      g_assert (block->end_token == BOBGUI_CSS_TOKEN_SEMICOLON);
      bobgui_css_parser_warn (self,
                           BOBGUI_CSS_PARSER_WARNING_SYNTAX,
                           bobgui_css_parser_get_block_location (self),
                           bobgui_css_parser_get_start_location (self),
                           "Expected ';' at end of block");
      bobgui_css_parser_blocks_drop_last (&self->blocks);
    }
  else
    {
      bobgui_css_parser_blocks_drop_last (&self->blocks);
      if (bobgui_css_token_is_preserved (&self->token, NULL))
        {
          bobgui_css_token_clear (&self->token);
        }
      else
        {
          bobgui_css_parser_start_block (self);
          bobgui_css_parser_end_block (self);
        }
    }
}

/*
 * bobgui_css_parser_skip:
 * @self: a `BobguiCssParser`
 *
 * Skips a component value.
 *
 * This means that if the token is a preserved token, only
 * this token will be skipped. If the token starts a block,
 * the whole block will be skipped.
 **/
void
bobgui_css_parser_skip (BobguiCssParser *self)
{
  const BobguiCssToken *token;

  token = bobgui_css_parser_get_token (self);
  if (bobgui_css_token_is_preserved (token, NULL))
    {
      bobgui_css_parser_consume_token (self);
    }
  else
    {
      bobgui_css_parser_start_block (self);
      bobgui_css_parser_end_block (self);
    }
}

/*
 * bobgui_css_parser_skip_until:
 * @self: a `BobguiCssParser`
 * @token_type: type of token to skip to
 *
 * Repeatedly skips a token until a certain type is reached.
 * After this called, bobgui_css_parser_get_token() will either
 * return a token of this type or the eof token.
 *
 * This function is useful for resyncing a parser after encountering
 * an error.
 *
 * If you want to skip until the end, use %GSK_TOKEN_TYPE_EOF
 * as the token type.
 **/
void
bobgui_css_parser_skip_until (BobguiCssParser    *self,
                           BobguiCssTokenType  token_type)
{
  const BobguiCssToken *token;

  for (token = bobgui_css_parser_get_token (self);
       !bobgui_css_token_is (token, token_type) &&
       !bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_EOF);
       token = bobgui_css_parser_get_token (self))
    {
      bobgui_css_parser_skip (self);
    }
}

void
bobgui_css_parser_skip_whitespace (BobguiCssParser *self)
{
  const BobguiCssToken *token;

  for (token = bobgui_css_parser_peek_token (self);
       bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_WHITESPACE);
       token = bobgui_css_parser_peek_token (self))
    {
      bobgui_css_parser_consume_token (self);
    }
}

void
bobgui_css_parser_emit_error (BobguiCssParser         *self,
                           const BobguiCssLocation *start,
                           const BobguiCssLocation *end,
                           const GError         *error)
{
  if (self->error_func)
    self->error_func (self, start, end, error, self->user_data);
}

void
bobgui_css_parser_error (BobguiCssParser         *self,
                      BobguiCssParserError     code,
                      const BobguiCssLocation *start,
                      const BobguiCssLocation *end,
                      const char           *format,
                      ...)
{
  va_list args;
  GError *error;

  va_start (args, format);
  error = g_error_new_valist (BOBGUI_CSS_PARSER_ERROR,
                              code,
                              format, args);
  bobgui_css_parser_emit_error (self, start, end, error);
  g_error_free (error);
  va_end (args);
}

void
bobgui_css_parser_error_syntax (BobguiCssParser *self,
                             const char   *format,
                             ...)
{
  va_list args;
  GError *error;

  va_start (args, format);
  error = g_error_new_valist (BOBGUI_CSS_PARSER_ERROR,
                              BOBGUI_CSS_PARSER_ERROR_SYNTAX,
                              format, args);
  bobgui_css_parser_emit_error (self,
                             bobgui_css_parser_get_start_location (self),
                             bobgui_css_parser_get_end_location (self),
                             error);
  g_error_free (error);
  va_end (args);
}

void
bobgui_css_parser_error_value (BobguiCssParser *self,
                            const char   *format,
                            ...)
{
  va_list args;
  GError *error;

  va_start (args, format);
  error = g_error_new_valist (BOBGUI_CSS_PARSER_ERROR,
                              BOBGUI_CSS_PARSER_ERROR_UNKNOWN_VALUE,
                              format, args);
  bobgui_css_parser_emit_error (self,
                             bobgui_css_parser_get_start_location (self),
                             bobgui_css_parser_get_end_location (self),
                             error);
  g_error_free (error);
  va_end (args);
}

void
bobgui_css_parser_error_import (BobguiCssParser *self,
                             const char   *format,
                             ...)
{
  va_list args;
  GError *error;

  va_start (args, format);
  error = g_error_new_valist (BOBGUI_CSS_PARSER_ERROR,
                              BOBGUI_CSS_PARSER_ERROR_IMPORT,
                              format, args);
  bobgui_css_parser_emit_error (self,
                             bobgui_css_parser_get_start_location (self),
                             bobgui_css_parser_get_end_location (self),
                             error);
  g_error_free (error);
  va_end (args);
}

void
bobgui_css_parser_warn (BobguiCssParser         *self,
                     BobguiCssParserWarning   code,
                     const BobguiCssLocation *start,
                     const BobguiCssLocation *end,
                     const char           *format,
                     ...)
{
  va_list args;
  GError *error;

  va_start (args, format);
  error = g_error_new_valist (BOBGUI_CSS_PARSER_WARNING,
                              code,
                              format, args);
  bobgui_css_parser_emit_error (self, start, end, error);
  g_error_free (error);
  va_end (args);
}

void
bobgui_css_parser_warn_syntax (BobguiCssParser *self,
                            const char   *format,
                            ...)
{
  va_list args;
  GError *error;

  va_start (args, format);
  error = g_error_new_valist (BOBGUI_CSS_PARSER_WARNING,
                              BOBGUI_CSS_PARSER_WARNING_SYNTAX,
                              format, args);
  bobgui_css_parser_emit_error (self,
                             bobgui_css_parser_get_start_location (self),
                             bobgui_css_parser_get_end_location (self),
                             error);
  g_error_free (error);
  va_end (args);
}

void
bobgui_css_parser_warn_deprecated (BobguiCssParser *self,
                                 const char   *format,
                                 ...)
{
  if (DEBUG_CHECK_CSS)
    {
      va_list args;
      GError *error;

      va_start (args, format);
      error = g_error_new_valist (BOBGUI_CSS_PARSER_WARNING,
                                  BOBGUI_CSS_PARSER_WARNING_DEPRECATED,
                                  format, args);
      bobgui_css_parser_emit_error (self,
                                 bobgui_css_parser_get_start_location (self),
                                 bobgui_css_parser_get_end_location (self),
                                 error);
      g_error_free (error);
      va_end (args);
    }
}

gboolean
bobgui_css_parser_consume_function (BobguiCssParser *self,
                                 guint         min_args,
                                 guint         max_args,
                                 guint (* parse_func) (BobguiCssParser *, guint, gpointer),
                                 gpointer      data)
{
  const BobguiCssToken *token;
  gboolean result = FALSE;
  char function_name[64];
  guint arg;

  token = bobgui_css_parser_get_token (self);
  g_return_val_if_fail (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_FUNCTION), FALSE);

  g_strlcpy (function_name, bobgui_css_token_get_string (token), 64);
  bobgui_css_parser_start_block (self);

  arg = 0;
  while (TRUE)
    {
      token = bobgui_css_parser_get_token (self);
      if (!bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_EOF))
        {
          guint parse_args = parse_func (self, arg, data);
          if (parse_args == 0)
            break;
          arg += parse_args;
        }

      token = bobgui_css_parser_get_token (self);
      if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_EOF))
        {
          if (arg < min_args)
            {
              bobgui_css_parser_error_syntax (self, "%s() requires at least %u arguments", function_name, min_args);
              break;
            }
          else
            {
              result = TRUE;
              break;
            }
        }
      else if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_COMMA))
        {
          if (arg >= max_args)
            {
              bobgui_css_parser_error_syntax (self, "Expected ')' at end of %s()", function_name);
              break;
            }

          bobgui_css_parser_consume_token (self);
          continue;
        }
      else
        {
          bobgui_css_parser_error_syntax (self, "Unexpected data at end of %s() argument", function_name);
          break;
        }

    }

  bobgui_css_parser_end_block (self);

  return result;
}

/**
 * bobgui_css_parser_has_token:
 * @self: a `BobguiCssParser`
 * @token_type: type of the token to check
 *
 * Checks if the next token is of @token_type.
 *
 * Returns: %TRUE if the next token is of @token_type
 **/
gboolean
bobgui_css_parser_has_token (BobguiCssParser    *self,
                          BobguiCssTokenType  token_type)
{
  const BobguiCssToken *token;

  token = bobgui_css_parser_get_token (self);

  return bobgui_css_token_is (token, token_type);
}

/**
 * bobgui_css_parser_has_ident:
 * @self: a `BobguiCssParser`
 * @ident: name of identifier
 *
 * Checks if the next token is an identifier with the given @name.
 *
 * Returns: %TRUE if the next token is an identifier with the given @name
 **/
gboolean
bobgui_css_parser_has_ident (BobguiCssParser *self,
                          const char   *ident)
{
  const BobguiCssToken *token;

  token = bobgui_css_parser_get_token (self);

  return bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_IDENT) &&
         g_ascii_strcasecmp (bobgui_css_token_get_string (token), ident) == 0;
}

gboolean
bobgui_css_parser_has_integer (BobguiCssParser *self)
{
  const BobguiCssToken *token;

  token = bobgui_css_parser_get_token (self);

  return bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_SIGNED_INTEGER) ||
         bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER);
}

gboolean
bobgui_css_parser_has_percentage (BobguiCssParser *self)
{
  const BobguiCssToken *token;

  token = bobgui_css_parser_get_token (self);

  return bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_PERCENTAGE);
}

/**
 * bobgui_css_parser_has_function:
 * @self: a `BobguiCssParser`
 * @name: name of function
 *
 * Checks if the next token is a function with the given @name.
 *
 * Returns: %TRUE if the next token is a function with the given @name
 */
gboolean
bobgui_css_parser_has_function (BobguiCssParser *self,
                             const char   *name)
{
  const BobguiCssToken *token;

  token = bobgui_css_parser_get_token (self);

  return bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_FUNCTION) &&
         g_ascii_strcasecmp (bobgui_css_token_get_string (token), name) == 0;
}

/**
 * bobgui_css_parser_try_delim:
 * @self: a `BobguiCssParser`
 * @codepoint: unicode character codepoint to check
 *
 * Checks if the current token is a delimiter matching the given
 * @codepoint. If that is the case, the token is consumed and
 * %TRUE is returned.
 *
 * Keep in mind that not every unicode codepoint can be a delim
 * token.
 *
 * Returns: %TRUE if the token matched and was consumed.
 **/
gboolean
bobgui_css_parser_try_delim (BobguiCssParser *self,
                          gunichar      codepoint)
{
  const BobguiCssToken *token;

  token = bobgui_css_parser_get_token (self);

  if (!bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_DELIM) ||
      codepoint != token->delim.delim)
    return FALSE;

  bobgui_css_parser_consume_token (self);
  return TRUE;
}

/**
 * bobgui_css_parser_try_ident:
 * @self: a `BobguiCssParser`
 * @ident: identifier to check for
 *
 * Checks if the current token is an identifier matching the given
 * @ident string. If that is the case, the token is consumed
 * and %TRUE is returned.
 *
 * Returns: %TRUE if the token matched and was consumed.
 **/
gboolean
bobgui_css_parser_try_ident (BobguiCssParser *self,
                          const char   *ident)
{
  const BobguiCssToken *token;

  token = bobgui_css_parser_get_token (self);

  if (!bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_IDENT) ||
      g_ascii_strcasecmp (bobgui_css_token_get_string (token), ident) != 0)
    return FALSE;

  bobgui_css_parser_consume_token (self);
  return TRUE;
}

/**
 * bobgui_css_parser_try_at_keyword:
 * @self: a `BobguiCssParser`
 * @keyword: name of keyword to check for
 *
 * Checks if the current token is an at-keyword token with the
 * given @keyword. If that is the case, the token is consumed
 * and %TRUE is returned.
 *
 * Returns: %TRUE if the token matched and was consumed.
 **/
gboolean
bobgui_css_parser_try_at_keyword (BobguiCssParser *self,
                               const char   *keyword)
{
  const BobguiCssToken *token;

  token = bobgui_css_parser_get_token (self);

  if (!bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_AT_KEYWORD) ||
      g_ascii_strcasecmp (bobgui_css_token_get_string (token), keyword) != 0)
    return FALSE;

  bobgui_css_parser_consume_token (self);
  return TRUE;
}

/**
 * bobgui_css_parser_try_token:
 * @self: a `BobguiCssParser`
 * @token_type: type of token to try
 *
 * Consumes the next token if it matches the given @token_type.
 *
 * This function can be used in loops like this:
 * do {
 *   ... parse one element ...
 * } while (bobgui_css_parser_try_token (parser, BOBGUI_CSS_TOKEN_COMMA);
 *
 * Returns: %TRUE if a token was consumed
 **/
gboolean
bobgui_css_parser_try_token (BobguiCssParser    *self,
                          BobguiCssTokenType  token_type)
{
  const BobguiCssToken *token;

  token = bobgui_css_parser_get_token (self);

  if (!bobgui_css_token_is (token, token_type))
    return FALSE;

  bobgui_css_parser_consume_token (self);
  return TRUE;
}

/**
 * bobgui_css_parser_consume_ident:
 * @self: a `BobguiCssParser`
 *
 * If the current token is an identifier, consumes it and returns
 * its name.
 *
 * If the current token is not an identifier, an error is emitted
 * and %NULL is returned.
 *
 * Returns: (transfer full): the name of the consumed identifier
 */
char *
bobgui_css_parser_consume_ident (BobguiCssParser *self)
{
  const BobguiCssToken *token;
  char *ident;

  token = bobgui_css_parser_get_token (self);

  if (!bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_IDENT))
    {
      bobgui_css_parser_error_syntax (self, "Expected an identifier");
      return NULL;
    }

  ident = g_strdup (bobgui_css_token_get_string (token));
  bobgui_css_parser_consume_token (self);

  return ident;
}

/**
 * bobgui_css_parser_consume_string:
 * @self: a `BobguiCssParser`
 *
 * If the current token is a string, consumes it and return the string.
 *
 * If the current token is not a string, an error is emitted
 * and %NULL is returned.
 *
 * Returns: (transfer full): the name of the consumed string
 **/
char *
bobgui_css_parser_consume_string (BobguiCssParser *self)
{
  const BobguiCssToken *token;
  char *ident;

  token = bobgui_css_parser_get_token (self);

  if (!bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_STRING))
    {
      bobgui_css_parser_error_syntax (self, "Expected a string");
      return NULL;
    }

  ident = g_strdup (bobgui_css_token_get_string (token));
  bobgui_css_parser_consume_token (self);

  return ident;
}

static guint
bobgui_css_parser_parse_url_arg (BobguiCssParser *parser,
                              guint         arg,
                              gpointer      data)
{
  char **out_url = data;

  *out_url = bobgui_css_parser_consume_string (parser);
  if (*out_url == NULL)
    return 0;

  return 1;
}

gboolean
bobgui_css_parser_has_url (BobguiCssParser *self)
{
  return bobgui_css_parser_has_token (self, BOBGUI_CSS_TOKEN_URL)
      || bobgui_css_parser_has_token (self, BOBGUI_CSS_TOKEN_BAD_URL)
      || bobgui_css_parser_has_function (self, "url");
}

/**
 * bobgui_css_parser_consume_url:
 * @self: a `BobguiCssParser`
 *
 * If the parser matches the `<url>` token from the [CSS
 * specification](https://drafts.csswg.org/css-values-4/#url-value),
 * consumes it, resolves the URL and returns the resulting `GFile`.
 * On failure, an error is emitted and %NULL is returned.
 *
 * Returns: (nullable) (transfer full): the resulting URL
 **/
char *
bobgui_css_parser_consume_url (BobguiCssParser *self)
{
  const BobguiCssToken *token;
  char *url;

  token = bobgui_css_parser_get_token (self);

  if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_URL))
    {
      url = g_strdup (bobgui_css_token_get_string (token));
      bobgui_css_parser_consume_token (self);
    }
  else if (bobgui_css_token_is_function (token, "url"))
    {
      if (!bobgui_css_parser_consume_function (self, 1, 1, bobgui_css_parser_parse_url_arg, &url))
        return NULL;
    }
  else
    {
      bobgui_css_parser_error_syntax (self, "Expected a URL");
      return NULL;
    }

  return url;
}

gboolean
bobgui_css_parser_has_number (BobguiCssParser *self)
{
  return bobgui_css_parser_has_token (self, BOBGUI_CSS_TOKEN_SIGNED_NUMBER)
      || bobgui_css_parser_has_token (self, BOBGUI_CSS_TOKEN_SIGNLESS_NUMBER)
      || bobgui_css_parser_has_token (self, BOBGUI_CSS_TOKEN_SIGNED_INTEGER)
      || bobgui_css_parser_has_token (self, BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER);
}

gboolean
bobgui_css_parser_consume_number (BobguiCssParser *self,
                               double       *number)
{
  const BobguiCssToken *token;

  token = bobgui_css_parser_get_token (self);
  if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_SIGNED_NUMBER) ||
      bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_SIGNLESS_NUMBER) ||
      bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_SIGNED_INTEGER) ||
      bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER))
    {
      *number = token->number.number;
      bobgui_css_parser_consume_token (self);
      return TRUE;
    }

  bobgui_css_parser_error_syntax (self, "Expected a number");
  /* FIXME: Implement calc() */
  return FALSE;
}

gboolean
bobgui_css_parser_consume_integer (BobguiCssParser *self,
                                int          *number)
{
  const BobguiCssToken *token;

  token = bobgui_css_parser_get_token (self);
  if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_SIGNED_INTEGER) ||
      bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_SIGNLESS_INTEGER))
    {
      *number = token->number.number;
      bobgui_css_parser_consume_token (self);
      return TRUE;
    }

  bobgui_css_parser_error_syntax (self, "Expected an integer");
  /* FIXME: Implement calc() */
  return FALSE;
}

gboolean
bobgui_css_parser_consume_percentage (BobguiCssParser *self,
                                   double       *number)
{
  const BobguiCssToken *token;

  token = bobgui_css_parser_get_token (self);
  if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_PERCENTAGE))
    {
      *number = token->number.number;
      bobgui_css_parser_consume_token (self);
      return TRUE;
    }

  bobgui_css_parser_error_syntax (self, "Expected a percentage");
  /* FIXME: Implement calc() */
  return FALSE;
}

gboolean
bobgui_css_parser_consume_number_or_percentage (BobguiCssParser *parser,
                                             double        min,
                                             double        max,
                                             double       *value)
{
  double number = 0;

  if (bobgui_css_parser_has_percentage (parser))
    {
      if (bobgui_css_parser_consume_percentage (parser, &number))
        {
          *value = min + (number / 100.0) * (max - min);
          return TRUE;
        }
    }
  else if (bobgui_css_parser_has_number (parser))
    {
      if (bobgui_css_parser_consume_number (parser, &number))
        {
          *value = number;
          return TRUE;
        }
    }

  bobgui_css_parser_error_syntax (parser, "Expected a number or percentage");
  return FALSE;
}

gsize
bobgui_css_parser_consume_any (BobguiCssParser            *parser,
                            const BobguiCssParseOption *options,
                            gsize                    n_options,
                            gpointer                 user_data)
{
  gsize result;
  gsize i;

  g_return_val_if_fail (parser != NULL, 0);
  g_return_val_if_fail (options != NULL, 0);
  g_return_val_if_fail (n_options < sizeof (gsize) * 8 - 1, 0);

  result = 0;
  while (result != (1u << n_options) - 1u)
    {
      for (i = 0; i < n_options; i++)
        {
          if (result & (1 << i))
            continue;
          if (options[i].can_parse && !options[i].can_parse (parser, options[i].data, user_data))
            continue;
          if (!options[i].parse (parser, options[i].data, user_data))
            return 0;
          result |= 1 << i;
          break;
        }
      if (i == n_options)
        break;
    }

  if (result == 0)
    {
      bobgui_css_parser_error_syntax (parser, "No valid value given");
      return result;
    }

  return result;
}

gboolean
bobgui_css_parser_has_references (BobguiCssParser *self)
{
  BobguiCssTokenizer *tokenizer = get_tokenizer (self);
  gboolean ret = FALSE;
  int inner_blocks = 0, i;

  /* We don't want bobgui_css_parser_ensure_token to expand references on us here */
  g_assert (self->n_refs == 0);

  bobgui_css_tokenizer_save (tokenizer);

  do {
      const BobguiCssToken *token;

      token = bobgui_css_parser_get_token (self);

      if (inner_blocks == 0)
        {
          if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_EOF))
            break;

          if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_CLOSE_PARENS) ||
              bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_CLOSE_SQUARE))
            {
              goto done;
            }
        }

      if (bobgui_css_token_is_preserved (token, NULL))
        {
          if (inner_blocks > 0 && bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_EOF))
            {
              bobgui_css_parser_end_block (self);
              inner_blocks--;
            }
          else
            {
              bobgui_css_parser_consume_token (self);
            }
        }
      else
        {
          gboolean is_var = bobgui_css_token_is_function (token, "var");

          inner_blocks++;
          bobgui_css_parser_start_block (self);

          if (is_var)
            {
              token = bobgui_css_parser_get_token (self);

              if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_IDENT))
                {
                  const char *var_name = bobgui_css_token_get_string (token);

                  if (strlen (var_name) < 3 || var_name[0] != '-' || var_name[1] != '-')
                    goto done;

                  bobgui_css_parser_consume_token (self);

                  if (!bobgui_css_parser_has_token (self, BOBGUI_CSS_TOKEN_EOF) &&
                      !bobgui_css_parser_has_token (self, BOBGUI_CSS_TOKEN_COMMA))
                    goto done;

                  ret = TRUE;
                  /* We got our answer. Now get it out as fast as possible! */
                  goto done;
                }
            }
        }
    }
  while (!bobgui_css_parser_has_token (self, BOBGUI_CSS_TOKEN_SEMICOLON) &&
         !bobgui_css_parser_has_token (self, BOBGUI_CSS_TOKEN_CLOSE_CURLY));

done:
  for (i = 0; i < inner_blocks; i++)
    bobgui_css_parser_end_block (self);

  g_assert (tokenizer == get_tokenizer (self));

  bobgui_css_tokenizer_restore (tokenizer);
  self->location = *bobgui_css_tokenizer_get_location (tokenizer);
  bobgui_css_tokenizer_read_token (tokenizer, &self->token, NULL);

  return ret;
}

static void
clear_ref (BobguiCssVariableValueReference *ref)
{
  g_free (ref->name);
  if (ref->fallback)
    bobgui_css_variable_value_unref (ref->fallback);
}

#define GDK_ARRAY_NAME bobgui_css_parser_references
#define GDK_ARRAY_TYPE_NAME BobguiCssParserReferences
#define GDK_ARRAY_ELEMENT_TYPE BobguiCssVariableValueReference

BobguiCssVariableValue *
bobgui_css_parser_parse_value_into_token_stream (BobguiCssParser *self)
{
  GBytes *bytes = bobgui_css_tokenizer_get_bytes (get_tokenizer (self));
  const BobguiCssToken *token;
  gsize offset;
  gsize length = 0;
  BobguiCssParserReferences refs;
  int inner_blocks = 0, i;
  gboolean is_initial = FALSE;

  for (token = bobgui_css_parser_peek_token (self);
       bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_WHITESPACE);
       token = bobgui_css_parser_peek_token (self))
    {
      bobgui_css_parser_consume_token (self);
    }

  bobgui_css_parser_references_init (&refs);

  offset = self->location.bytes;

  do {
      token = bobgui_css_parser_get_token (self);

      if (length == 0 && bobgui_css_token_is_ident (token, "initial"))
        is_initial = TRUE;

      if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_BAD_STRING) ||
          bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_BAD_URL))
        {
          bobgui_css_parser_error_syntax (self, "Invalid property value");
          goto error;
        }

      if (inner_blocks == 0)
        {
          if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_EOF))
            break;

          if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_CLOSE_PARENS) ||
              bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_CLOSE_SQUARE))
            {
              bobgui_css_parser_error_syntax (self, "Invalid property value");
              goto error;
            }
        }

      if (bobgui_css_token_is_preserved (token, NULL))
        {
          if (inner_blocks > 0 && bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_EOF))
            {
              length++;
              bobgui_css_parser_end_block (self);

              inner_blocks--;
            }
          else
            {
              length++;
              bobgui_css_parser_consume_token (self);
            }
        }
      else
        {
          gboolean is_var = bobgui_css_token_is_function (token, "var");

          length++;
          inner_blocks++;

          bobgui_css_parser_start_block (self);

          if (is_var)
            {
              token = bobgui_css_parser_get_token (self);

              if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_IDENT))
                {
                  BobguiCssVariableValueReference ref;
                  char *var_name = g_strdup (bobgui_css_token_get_string (token));

                  if (strlen (var_name) < 3 || var_name[0] != '-' || var_name[1] != '-')
                    {
                      bobgui_css_parser_error_syntax (self, "Invalid variable name: %s", var_name);
                      g_free (var_name);
                      goto error;
                    }

                  length++;
                  bobgui_css_parser_consume_token (self);

                  if (!bobgui_css_parser_has_token (self, BOBGUI_CSS_TOKEN_EOF) &&
                      !bobgui_css_parser_has_token (self, BOBGUI_CSS_TOKEN_COMMA))
                    {
                      bobgui_css_parser_error_syntax (self, "Invalid property value");
                      g_free (var_name);
                      goto error;
                    }

                  ref.name = var_name;

                  if (bobgui_css_parser_has_token (self, BOBGUI_CSS_TOKEN_EOF))
                    {
                      ref.length = 3;
                      ref.fallback = NULL;
                    }
                  else
                    {
                      length++;
                      bobgui_css_parser_consume_token (self);

                      ref.fallback = bobgui_css_parser_parse_value_into_token_stream (self);

                      if (ref.fallback == NULL)
                        {
                          bobgui_css_parser_error_value (self, "Invalid fallback for: %s", var_name);
                          g_free (var_name);
                          goto error;
                        }

                      ref.length = 4 + ref.fallback->length;
                      length += ref.fallback->length;
                    }

                  bobgui_css_parser_references_append (&refs, &ref);
                }
              else
                {
                  if (bobgui_css_token_is (token, BOBGUI_CSS_TOKEN_EOF))
                    {
                      bobgui_css_parser_error_syntax (self, "Missing variable name");
                    }
                  else
                    {
                      char *s = bobgui_css_token_to_string (token);
                      bobgui_css_parser_error_syntax (self, "Expected a variable name, not %s", s);
                      g_free (s);
                    }
                  goto error;
                }
            }
        }
    }
  while (!bobgui_css_parser_has_token (self, BOBGUI_CSS_TOKEN_SEMICOLON) &&
         !bobgui_css_parser_has_token (self, BOBGUI_CSS_TOKEN_CLOSE_CURLY));

  if (inner_blocks > 0)
    {
      bobgui_css_parser_error_syntax (self, "Invalid property value");
      goto error;
    }

  if (is_initial && length == 1)
    {
      bobgui_css_parser_references_clear (&refs);

      return bobgui_css_variable_value_new_initial (bytes,
                                                 offset,
                                                 self->location.bytes);
    }
  else
    {
      BobguiCssVariableValueReference *out_refs;
      gsize n_refs;

      n_refs = bobgui_css_parser_references_get_size (&refs);
      out_refs = bobgui_css_parser_references_steal (&refs);

      return bobgui_css_variable_value_new (bytes,
                                         offset,
                                         self->location.bytes,
                                         length,
                                         out_refs,
                                         n_refs);
    }

error:
  for (i = 0; i < inner_blocks; i++)
    bobgui_css_parser_end_block (self);

  bobgui_css_parser_references_clear (&refs);

  return NULL;
}

void
bobgui_css_parser_get_expanding_variables (BobguiCssParser          *self,
                                        BobguiCssVariableValue ***variables,
                                        char                ***variable_names,
                                        gsize                 *n_variables)
{
  gsize len = bobgui_css_tokenizers_get_size (&self->tokenizers);
  BobguiCssVariableValue **vars = NULL;
  char **names = NULL;
  gsize n;

  if (variables)
    vars = g_new0 (BobguiCssVariableValue *, len + 1);
  if (variable_names)
    names = g_new0 (char *, len + 1);

  n = 0;
  for (gsize i = 0; i < bobgui_css_tokenizers_get_size (&self->tokenizers); i++)
    {
      BobguiCssTokenizerData *data = bobgui_css_tokenizers_index (&self->tokenizers, i);
      if (variables && data->variable)
        vars[n] = bobgui_css_variable_value_ref (data->variable);
      if (variable_names)
        names[n] = g_strdup (data->var_name);
      n++;
    }

  if (variables)
    *variables = vars;

  if (variable_names)
    *variable_names = names;

  if (n_variables)
    *n_variables = n;
}
