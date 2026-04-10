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

#include "bobguicssvariablevalueprivate.h"

BobguiCssVariableValue *
bobgui_css_variable_value_new (GBytes                       *bytes,
                            gsize                         offset,
                            gsize                         end_offset,
                            gsize                         length,
                            BobguiCssVariableValueReference *references,
                            gsize                         n_references)
{
  BobguiCssVariableValue *self = g_new0 (BobguiCssVariableValue, 1);

  self->ref_count = 1;

  self->bytes = g_bytes_ref (bytes);
  self->offset = offset;
  self->end_offset = end_offset;
  self->length = length;

  self->references = references;
  self->n_references = n_references;

  return self;
}

BobguiCssVariableValue *
bobgui_css_variable_value_new_initial (GBytes *bytes,
                                    gsize   offset,
                                    gsize   end_offset)
{
  BobguiCssVariableValue *self = bobgui_css_variable_value_new (bytes, offset, end_offset, 1, NULL, 0);

  self->is_invalid = TRUE;

  return self;
}

BobguiCssVariableValue *
bobgui_css_variable_value_ref (BobguiCssVariableValue *self)
{
  self->ref_count++;

  return self;
}

void
bobgui_css_variable_value_unref (BobguiCssVariableValue *self)
{
  gsize i;

  self->ref_count--;
  if (self->ref_count > 0)
    return;

  g_bytes_unref (self->bytes);

  for (i = 0; i < self->n_references; i++)
    {
      BobguiCssVariableValueReference *ref = &self->references[i];

      g_free (ref->name);
      if (ref->fallback)
        bobgui_css_variable_value_unref (ref->fallback);
    }

  if (self->section)
    bobgui_css_section_unref (self->section);

  g_free (self->references);
  g_free (self);
}

void
bobgui_css_variable_value_print (BobguiCssVariableValue *self,
                              GString             *string)
{
  gsize len = self->end_offset - self->offset;
  gconstpointer data = g_bytes_get_region (self->bytes, 1, self->offset, len);

  g_assert (data != NULL);

  g_string_append_len (string, (const char *) data, len);
}

char *
bobgui_css_variable_value_to_string (BobguiCssVariableValue *self)
{
  GString *string = g_string_new (NULL);
  bobgui_css_variable_value_print (self, string);
  return g_string_free (string, FALSE);
}

gboolean
bobgui_css_variable_value_equal (const BobguiCssVariableValue *value1,
                              const BobguiCssVariableValue *value2)
{
  if (value1 == value2)
    return TRUE;

  if (value1 == NULL || value2 == NULL)
    return FALSE;

  if (value1->bytes != value2->bytes)
    return FALSE;

  if (value1->offset != value2->offset)
    return FALSE;

  if (value1->end_offset != value2->end_offset)
    return FALSE;

  return TRUE;
}

BobguiCssVariableValue *
bobgui_css_variable_value_transition  (BobguiCssVariableValue *start,
                                    BobguiCssVariableValue *end,
                                    double               progress)
{
  BobguiCssVariableValue *ret = progress < 0.5 ? start : end;

  if (ret == NULL)
    return NULL;

  return bobgui_css_variable_value_ref (ret);
}

void
bobgui_css_variable_value_set_section (BobguiCssVariableValue *self,
                                    BobguiCssSection       *section)
{
  self->section = bobgui_css_section_ref (section);
}

void
bobgui_css_variable_value_taint (BobguiCssVariableValue *self)
{
  self->is_animation_tainted = TRUE;
}
