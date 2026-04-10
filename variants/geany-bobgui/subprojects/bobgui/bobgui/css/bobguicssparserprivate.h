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


#pragma once

#include "bobguicssenums.h"
#include "bobguicsstokenizerprivate.h"
#include "bobguicssvariablevalueprivate.h"

#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct _BobguiCssParser BobguiCssParser;

typedef struct _BobguiCssParseOption BobguiCssParseOption;

struct _BobguiCssParseOption
{
  gboolean (* can_parse)  (BobguiCssParser *parser,
                           gpointer      option_data,
                           gpointer      user_data);
  gboolean (* parse)      (BobguiCssParser *parser,
                           gpointer      option_data,
                           gpointer      user_data);
  gpointer data;
};

typedef void            (* BobguiCssParserErrorFunc)               (BobguiCssParser                   *parser,
                                                                 const BobguiCssLocation           *start,
                                                                 const BobguiCssLocation           *end,
                                                                 const GError                   *error,
                                                                 gpointer                        user_data);

BobguiCssParser *          bobgui_css_parser_new_for_file             (GFile                          *file,
                                                                 BobguiCssParserErrorFunc           error_func,
                                                                 gpointer                        user_data,
                                                                 GDestroyNotify                  user_destroy,
                                                                 GError                        **error);
BobguiCssParser *          bobgui_css_parser_new_for_bytes            (GBytes                         *bytes,
                                                                 GFile                          *file,
                                                                 BobguiCssParserErrorFunc           error_func,
                                                                 gpointer                        user_data,
                                                                 GDestroyNotify                  user_destroy);
BobguiCssParser *          bobgui_css_parser_new_for_token_stream     (BobguiCssVariableValue            *value,
                                                                 GFile                          *file,
                                                                 BobguiCssVariableValue           **refs,
                                                                 gsize                           n_refs,
                                                                 BobguiCssParserErrorFunc           error_func,
                                                                 gpointer                        user_data,
                                                                 GDestroyNotify                  user_destroy);
BobguiCssParser *          bobgui_css_parser_ref                      (BobguiCssParser                   *self);
void                    bobgui_css_parser_unref                    (BobguiCssParser                   *self);

GFile *                 bobgui_css_parser_get_file                 (BobguiCssParser                   *self) G_GNUC_PURE;
GBytes *                bobgui_css_parser_get_bytes                (BobguiCssParser                   *self) G_GNUC_PURE;
GFile *                 bobgui_css_parser_resolve_url              (BobguiCssParser                   *self,
                                                                 const char                     *url);

const BobguiCssLocation *  bobgui_css_parser_get_start_location       (BobguiCssParser                   *self) G_GNUC_PURE;
const BobguiCssLocation *  bobgui_css_parser_get_end_location         (BobguiCssParser                   *self) G_GNUC_PURE;
const BobguiCssLocation *  bobgui_css_parser_get_block_location       (BobguiCssParser                   *self) G_GNUC_PURE;

const BobguiCssToken *     bobgui_css_parser_peek_token               (BobguiCssParser                   *self);
const BobguiCssToken *     bobgui_css_parser_get_token                (BobguiCssParser                   *self);
void                    bobgui_css_parser_consume_token            (BobguiCssParser                   *self);

void                    bobgui_css_parser_start_block              (BobguiCssParser                   *self); 
void                    bobgui_css_parser_start_semicolon_block    (BobguiCssParser                   *self,
                                                                 BobguiCssTokenType                 alternative_token);
void                    bobgui_css_parser_end_block_prelude        (BobguiCssParser                   *self);
void                    bobgui_css_parser_end_block                (BobguiCssParser                   *self); 
void                    bobgui_css_parser_skip                     (BobguiCssParser                   *self);
void                    bobgui_css_parser_skip_until               (BobguiCssParser                   *self,
                                                                 BobguiCssTokenType                 token_type);
void                    bobgui_css_parser_skip_whitespace          (BobguiCssParser                   *self);

void                    bobgui_css_parser_emit_error               (BobguiCssParser                   *self,
                                                                 const BobguiCssLocation           *start,
                                                                 const BobguiCssLocation           *end,
                                                                 const GError                   *error);
void                    bobgui_css_parser_error                    (BobguiCssParser                   *self,
                                                                 BobguiCssParserError               code,
                                                                 const BobguiCssLocation           *start,
                                                                 const BobguiCssLocation           *end,
                                                                 const char                     *format,
                                                                 ...) G_GNUC_PRINTF(5, 6);
void                    bobgui_css_parser_error_syntax             (BobguiCssParser                   *self,
                                                                 const char                     *format,
                                                                 ...) G_GNUC_PRINTF(2, 3);
void                    bobgui_css_parser_error_value              (BobguiCssParser                   *self,
                                                                 const char                     *format,
                                                                 ...) G_GNUC_PRINTF(2, 3);
void                    bobgui_css_parser_error_import             (BobguiCssParser                   *self,
                                                                 const char                     *format,
                                                                 ...) G_GNUC_PRINTF(2, 3);
void                    bobgui_css_parser_warn                     (BobguiCssParser                   *self,
                                                                 BobguiCssParserWarning             code,
                                                                 const BobguiCssLocation           *start,
                                                                 const BobguiCssLocation           *end,
                                                                 const char                     *format,
                                                                 ...) G_GNUC_PRINTF(5, 6);
void                    bobgui_css_parser_warn_syntax              (BobguiCssParser                   *self,
                                                                 const char                     *format,
                                                                 ...) G_GNUC_PRINTF(2, 3);
void                    bobgui_css_parser_warn_deprecated          (BobguiCssParser                   *self,
                                                                 const char                     *format,
                                                                 ...) G_GNUC_PRINTF(2, 3);


gboolean                bobgui_css_parser_has_token                (BobguiCssParser                   *self,
                                                                 BobguiCssTokenType                 token_type);
gboolean                bobgui_css_parser_has_ident                (BobguiCssParser                   *self,
                                                                 const char                     *ident);
gboolean                bobgui_css_parser_has_url                  (BobguiCssParser                   *self);
gboolean                bobgui_css_parser_has_number               (BobguiCssParser                   *self);
gboolean                bobgui_css_parser_has_integer              (BobguiCssParser                   *self);
gboolean                bobgui_css_parser_has_percentage           (BobguiCssParser                   *self);
gboolean                bobgui_css_parser_has_function             (BobguiCssParser                   *self,
                                                                 const char                     *name);

gboolean                bobgui_css_parser_try_delim                (BobguiCssParser                   *self,
                                                                 gunichar                        codepoint);
gboolean                bobgui_css_parser_try_ident                (BobguiCssParser                   *self,
                                                                 const char                     *ident);
gboolean                bobgui_css_parser_try_at_keyword           (BobguiCssParser                   *self,
                                                                 const char                     *keyword);
gboolean                bobgui_css_parser_try_token                (BobguiCssParser                   *self,
                                                                 BobguiCssTokenType                 token_type);

char *                  bobgui_css_parser_consume_ident            (BobguiCssParser                   *self) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
char *                  bobgui_css_parser_consume_string           (BobguiCssParser                   *self) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
char *                  bobgui_css_parser_consume_url              (BobguiCssParser                   *self) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
gboolean                bobgui_css_parser_consume_number           (BobguiCssParser                   *self,
                                                                 double                         *number);
gboolean                bobgui_css_parser_consume_integer          (BobguiCssParser                   *self,
                                                                 int                            *number);
gboolean                bobgui_css_parser_consume_percentage       (BobguiCssParser                   *self,
                                                                 double                         *number);
gboolean                bobgui_css_parser_consume_number_or_percentage
                                                                (BobguiCssParser                   *parser,
                                                                 double                          min,
                                                                 double                          max,
                                                                 double                         *value);
gboolean                bobgui_css_parser_consume_function         (BobguiCssParser                   *self,
                                                                 guint                           min_args,
                                                                 guint                           max_args,
                                                                 guint (* parse_func) (BobguiCssParser *, guint, gpointer),
                                                                 gpointer                        data);
gsize                   bobgui_css_parser_consume_any              (BobguiCssParser                   *parser,
                                                                 const BobguiCssParseOption        *options,
                                                                 gsize                           n_options,
                                                                 gpointer                        user_data);

gboolean                bobgui_css_parser_has_references           (BobguiCssParser                   *parser);

BobguiCssVariableValue *   bobgui_css_parser_parse_value_into_token_stream (BobguiCssParser              *parser);

void                    bobgui_css_parser_get_expanding_variables (BobguiCssParser              *parser,
                                                                BobguiCssVariableValue     ***variables,
                                                                char                    ***names,
                                                                gsize                     *n_variables);


/* We cannot include bobguidebug.h, so we must keep this in sync */
#define BOBGUI_CSS_PARSER_DEBUG_CSS (1 << 20)

G_END_DECLS

