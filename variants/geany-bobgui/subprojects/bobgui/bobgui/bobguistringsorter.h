/*
 * Copyright © 2019 Matthias Clasen
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
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiexpression.h>
#include <bobgui/bobguisorter.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_STRING_SORTER             (bobgui_string_sorter_get_type ())
GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiStringSorter, bobgui_string_sorter, BOBGUI, STRING_SORTER, BobguiSorter)

GDK_AVAILABLE_IN_ALL
BobguiStringSorter *       bobgui_string_sorter_new                   (BobguiExpression          *expression);

GDK_AVAILABLE_IN_ALL
BobguiExpression *         bobgui_string_sorter_get_expression        (BobguiStringSorter        *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_string_sorter_set_expression        (BobguiStringSorter        *self,
                                                                 BobguiExpression          *expression);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_string_sorter_get_ignore_case       (BobguiStringSorter        *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_string_sorter_set_ignore_case       (BobguiStringSorter        *self,
                                                                 gboolean                ignore_case);

/**
 * BobguiCollation:
 * @BOBGUI_COLLATION_NONE: Don't do any collation
 * @BOBGUI_COLLATION_UNICODE: Use [func@GLib.utf8_collate_key]
 * @BOBGUI_COLLATION_FILENAME: Use [func@GLib.utf8_collate_key_for_filename]
 *
 * Describes how a [class@Bobgui.StringSorter] turns strings into sort keys to
 * compare them.
 *
 * Note that the result of sorting will in general depend on the current locale
 * unless the mode is @BOBGUI_COLLATION_NONE.
 *
 * Since: 4.10
 */
typedef enum
{
  BOBGUI_COLLATION_NONE,
  BOBGUI_COLLATION_UNICODE,
  BOBGUI_COLLATION_FILENAME
} BobguiCollation;

GDK_AVAILABLE_IN_4_10
void                    bobgui_string_sorter_set_collation         (BobguiStringSorter        *self,
                                                                 BobguiCollation            collation);

GDK_AVAILABLE_IN_4_10
BobguiCollation            bobgui_string_sorter_get_collation         (BobguiStringSorter        *self);

G_END_DECLS

