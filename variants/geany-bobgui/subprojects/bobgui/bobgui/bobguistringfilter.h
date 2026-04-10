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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiexpression.h>
#include <bobgui/bobguifilter.h>

G_BEGIN_DECLS

/**
 * BobguiStringFilterMatchMode:
 * @BOBGUI_STRING_FILTER_MATCH_MODE_EXACT: The search string and
 *   text must match exactly
 * @BOBGUI_STRING_FILTER_MATCH_MODE_SUBSTRING: The search string
 *   must be contained as a substring inside the text
 * @BOBGUI_STRING_FILTER_MATCH_MODE_PREFIX: The text must begin
 *   with the search string
 *
 * Specifies how search strings are matched inside text.
 */
typedef enum {
  BOBGUI_STRING_FILTER_MATCH_MODE_EXACT,
  BOBGUI_STRING_FILTER_MATCH_MODE_SUBSTRING,
  BOBGUI_STRING_FILTER_MATCH_MODE_PREFIX
} BobguiStringFilterMatchMode;

#define BOBGUI_TYPE_STRING_FILTER             (bobgui_string_filter_get_type ())
GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiStringFilter, bobgui_string_filter, BOBGUI, STRING_FILTER, BobguiFilter)

GDK_AVAILABLE_IN_ALL
BobguiStringFilter *       bobgui_string_filter_new                   (BobguiExpression          *expression);

GDK_AVAILABLE_IN_ALL
const char *            bobgui_string_filter_get_search            (BobguiStringFilter        *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_string_filter_set_search            (BobguiStringFilter        *self,
                                                                 const char             *search);
GDK_AVAILABLE_IN_ALL
BobguiExpression *         bobgui_string_filter_get_expression        (BobguiStringFilter        *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_string_filter_set_expression        (BobguiStringFilter        *self,
                                                                 BobguiExpression          *expression);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_string_filter_get_ignore_case       (BobguiStringFilter        *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_string_filter_set_ignore_case       (BobguiStringFilter        *self,
                                                                 gboolean                ignore_case);
GDK_AVAILABLE_IN_ALL
BobguiStringFilterMatchMode bobgui_string_filter_get_match_mode       (BobguiStringFilter        *self);
GDK_AVAILABLE_IN_ALL
void                     bobgui_string_filter_set_match_mode       (BobguiStringFilter        *self,
                                                                 BobguiStringFilterMatchMode mode);



G_END_DECLS

