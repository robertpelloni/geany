/*
 * Copyright © 2012 Red Hat Inc.
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
 * Authors: Alexander Larsson <alexl@gnome.org>
 */

#pragma once

#include <bobgui/css/bobguicss.h>
#include "bobgui/css/bobguicsstokenizerprivate.h"
#include "bobgui/css/bobguicssparserprivate.h"
#include "bobguicssvalueprivate.h"
#include "bobguitypes.h"

G_BEGIN_DECLS

typedef struct _BobguiCssKeyframes BobguiCssKeyframes;

BobguiCssKeyframes *   _bobgui_css_keyframes_parse                  (BobguiCssParser           *parser);

BobguiCssKeyframes *   _bobgui_css_keyframes_ref                    (BobguiCssKeyframes        *keyframes);
void                _bobgui_css_keyframes_unref                  (BobguiCssKeyframes        *keyframes);

void                _bobgui_css_keyframes_print                  (BobguiCssKeyframes        *keyframes,
                                                               GString                *string);

BobguiCssKeyframes *   _bobgui_css_keyframes_compute                (BobguiCssKeyframes         *keyframes,
                                                               BobguiStyleProvider        *provider,
                                                               BobguiCssStyle             *style,
                                                               BobguiCssStyle             *parent_style);

guint               _bobgui_css_keyframes_get_n_properties       (BobguiCssKeyframes        *keyframes) G_GNUC_PURE;
guint               _bobgui_css_keyframes_get_property_id        (BobguiCssKeyframes        *keyframes,
                                                               guint                   id);
BobguiCssValue *       _bobgui_css_keyframes_get_value              (BobguiCssKeyframes        *keyframes,
                                                               guint                   id,
                                                               double                  progress,
                                                               BobguiCssValue            *default_value);

guint               _bobgui_css_keyframes_get_n_variables        (BobguiCssKeyframes        *keyframes);
int                 _bobgui_css_keyframes_get_variable_id        (BobguiCssKeyframes        *keyframes,
                                                               guint                   id);
BobguiCssVariableValue *_bobgui_css_keyframes_get_variable          (BobguiCssKeyframes        *keyframes,
                                                               guint                   id,
                                                               double                  progress,
                                                               BobguiCssVariableValue    *default_value);

G_END_DECLS

