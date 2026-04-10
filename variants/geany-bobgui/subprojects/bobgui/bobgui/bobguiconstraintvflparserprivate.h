/* bobguiconstraintvflparserprivate.h: VFL constraint definition parser 
 *
 * Copyright 2017  Endless
 * Copyright 2019  GNOME Foundation
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "bobguiconstrainttypesprivate.h"

G_BEGIN_DECLS

typedef struct _BobguiConstraintVflParser       BobguiConstraintVflParser;

typedef struct {
  const char *view1;
  const char *attr1;
  BobguiConstraintRelation relation;
  const char *view2;
  const char *attr2;
  double constant;
  double multiplier;
  double strength;
} BobguiConstraintVfl;

BobguiConstraintVflParser *
bobgui_constraint_vfl_parser_new (void);

void
bobgui_constraint_vfl_parser_free (BobguiConstraintVflParser *parser);

void
bobgui_constraint_vfl_parser_set_default_spacing (BobguiConstraintVflParser *parser,
                                               int hspacing,
                                               int vspacing);

void
bobgui_constraint_vfl_parser_set_metrics (BobguiConstraintVflParser *parser,
                                       GHashTable *metrics);

void
bobgui_constraint_vfl_parser_set_views (BobguiConstraintVflParser *parser,
                                     GHashTable *views);

gboolean
bobgui_constraint_vfl_parser_parse_line (BobguiConstraintVflParser *parser,
                                      const char *line,
                                      gssize len,
                                      GError **error);

int
bobgui_constraint_vfl_parser_get_error_offset (BobguiConstraintVflParser *parser);

int
bobgui_constraint_vfl_parser_get_error_range (BobguiConstraintVflParser *parser);

BobguiConstraintVfl *
bobgui_constraint_vfl_parser_get_constraints (BobguiConstraintVflParser *parser,
                                           int *n_constraints);

G_END_DECLS
