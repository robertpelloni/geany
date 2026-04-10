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

#include "bobguicss.h"
#include "bobguicsstokenizerprivate.h"

G_BEGIN_DECLS

typedef struct _BobguiCssVariableValueReference BobguiCssVariableValueReference;
typedef struct _BobguiCssVariableValue BobguiCssVariableValue;

struct _BobguiCssVariableValueReference
{
  char *name;
  gsize length;
  BobguiCssVariableValue *fallback;
};

struct _BobguiCssVariableValue
{
  int ref_count;

  GBytes *bytes;
  gsize offset;
  gsize end_offset;
  gsize length;

  BobguiCssVariableValueReference *references;
  gsize n_references;

  BobguiCssSection *section;
  gboolean is_invalid;
  gboolean is_animation_tainted;
};

BobguiCssVariableValue *bobgui_css_variable_value_new         (GBytes                       *bytes,
                                                         gsize                         offset,
                                                         gsize                         end_offset,
                                                         gsize                         length,
                                                         BobguiCssVariableValueReference *references,
                                                         gsize                         n_references);
BobguiCssVariableValue *bobgui_css_variable_value_new_initial (GBytes                       *bytes,
                                                         gsize                         offset,
                                                         gsize                         end_offset);
BobguiCssVariableValue *bobgui_css_variable_value_ref         (BobguiCssVariableValue          *self);
void                 bobgui_css_variable_value_unref       (BobguiCssVariableValue          *self);
void                 bobgui_css_variable_value_print       (BobguiCssVariableValue          *self,
                                                         GString                      *string);
char *               bobgui_css_variable_value_to_string   (BobguiCssVariableValue          *self);
gboolean             bobgui_css_variable_value_equal       (const BobguiCssVariableValue    *value1,
                                                         const BobguiCssVariableValue    *value2) G_GNUC_PURE;
BobguiCssVariableValue *bobgui_css_variable_value_transition  (BobguiCssVariableValue          *start,
                                                         BobguiCssVariableValue          *end,
                                                         double                        progress);
void                 bobgui_css_variable_value_set_section (BobguiCssVariableValue          *self,
                                                         BobguiCssSection                *section);
void                 bobgui_css_variable_value_taint       (BobguiCssVariableValue          *self);

G_END_DECLS
