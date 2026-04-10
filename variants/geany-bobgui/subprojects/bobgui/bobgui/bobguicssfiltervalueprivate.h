/*
 * Copyright © 2017 Red Hat Inc.
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

#include <bobgui/css/bobguicss.h>
#include "bobgui/css/bobguicsstokenizerprivate.h"
#include "bobgui/css/bobguicssparserprivate.h"
#include "bobguicssvalueprivate.h"

G_BEGIN_DECLS

BobguiCssValue *   bobgui_css_filter_value_new_none           (void);
BobguiCssValue *   bobgui_css_filter_value_parse              (BobguiCssParser           *parser);

gboolean        bobgui_css_filter_value_is_none            (const BobguiCssValue      *filter);

double          bobgui_css_filter_value_push_snapshot      (const BobguiCssValue      *filter,
                                                         BobguiSnapshot            *snapshot);
void            bobgui_css_filter_value_pop_snapshot       (const BobguiCssValue      *filter,
                                                         const graphene_rect_t  *bounds,
                                                         BobguiSnapshot            *snapshot);

G_END_DECLS

