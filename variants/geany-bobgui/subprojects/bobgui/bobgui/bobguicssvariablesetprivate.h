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

#pragma once

#include "bobgui/css/bobguicssvariablevalueprivate.h"

G_BEGIN_DECLS

typedef struct _BobguiCssVariableSet BobguiCssVariableSet;

struct _BobguiCssVariableSet
{
  int ref_count;

  GHashTable *variables;
  BobguiCssVariableSet *parent;
};

BobguiCssVariableSet *  bobgui_css_variable_set_new            (void);

BobguiCssVariableSet *  bobgui_css_variable_set_ref            (BobguiCssVariableSet   *self);
void                 bobgui_css_variable_set_unref          (BobguiCssVariableSet   *self);

BobguiCssVariableSet *  bobgui_css_variable_set_copy           (BobguiCssVariableSet   *self);

void                 bobgui_css_variable_set_set_parent     (BobguiCssVariableSet   *self,
                                                          BobguiCssVariableSet   *parent);

void                 bobgui_css_variable_set_add            (BobguiCssVariableSet   *self,
                                                          int                  id,
                                                          BobguiCssVariableValue *value);
void                 bobgui_css_variable_set_resolve_cycles (BobguiCssVariableSet   *self);

BobguiCssVariableValue *bobgui_css_variable_set_lookup         (BobguiCssVariableSet   *self,
                                                          int                  id,
                                                          BobguiCssVariableSet  **source);

GArray *             bobgui_css_variable_set_list_ids       (BobguiCssVariableSet   *self);

gboolean             bobgui_css_variable_set_equal          (BobguiCssVariableSet   *set1,
                                                          BobguiCssVariableSet   *set2);

G_END_DECLS
