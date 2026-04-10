/* bobguiaccessibletextprivate.h: Private definitions for BobguiAccessibleText
 *
 * SPDX-FileCopyrightText: 2023 Emmanuele Bassi
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "bobguiaccessibletext.h"

G_BEGIN_DECLS

GBytes *
bobgui_accessible_text_get_contents (BobguiAccessibleText *self,
                                  unsigned int       start,
                                  unsigned int       end);

GBytes *
bobgui_accessible_text_get_contents_at (BobguiAccessibleText            *self,
                                     unsigned int                  offset,
                                     BobguiAccessibleTextGranularity  granularity,
                                     unsigned int                 *start,
                                     unsigned int                 *end);

unsigned int
bobgui_accessible_text_get_character_count (BobguiAccessibleText *self);

unsigned int
bobgui_accessible_text_get_caret_position (BobguiAccessibleText *self);

gboolean
bobgui_accessible_text_get_selection (BobguiAccessibleText       *self,
                                   gsize                   *n_ranges,
                                   BobguiAccessibleTextRange **ranges);

gboolean
bobgui_accessible_text_get_attributes (BobguiAccessibleText        *self,
                                    unsigned int              offset,
                                    gsize                    *n_ranges,
                                    BobguiAccessibleTextRange  **ranges,
                                    char                   ***attribute_names,
                                    char                   ***attribute_values);

void
bobgui_accessible_text_get_default_attributes (BobguiAccessibleText   *self,
                                            char              ***attribute_names,
                                            char              ***attribute_values);

gboolean
bobgui_accessible_text_get_attributes_run (BobguiAccessibleText        *self,
                                        unsigned int              offset,
                                        gboolean                  include_defaults,
                                        gsize                    *n_attributes,
                                        char                   ***attribute_names,
                                        char                   ***attribute_values,
                                        int                      *start,
                                        int                      *end);

gboolean
bobgui_accessible_text_get_extents (BobguiAccessibleText *self,
                                 unsigned int       start,
                                 unsigned int       end,
                                 graphene_rect_t   *extents);

gboolean
bobgui_accessible_text_get_offset (BobguiAccessibleText      *self,
                                const graphene_point_t *point,
                                unsigned int           *offset);

gboolean
bobgui_accessible_text_set_caret_position (BobguiAccessibleText *self,
                                        unsigned int       offset);

gboolean
bobgui_accessible_text_set_selection (BobguiAccessibleText      *self,
                                   gsize                   i,
                                   BobguiAccessibleTextRange *range);

G_END_DECLS
