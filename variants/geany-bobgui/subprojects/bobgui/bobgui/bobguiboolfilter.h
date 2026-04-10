/*
 * Copyright © 2020 Benjamin Otte
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

#define BOBGUI_TYPE_BOOL_FILTER             (bobgui_bool_filter_get_type ())
GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiBoolFilter, bobgui_bool_filter, BOBGUI, BOOL_FILTER, BobguiFilter)

GDK_AVAILABLE_IN_ALL
BobguiBoolFilter *         bobgui_bool_filter_new                     (BobguiExpression          *expression);

GDK_AVAILABLE_IN_ALL
BobguiExpression *         bobgui_bool_filter_get_expression          (BobguiBoolFilter          *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_bool_filter_set_expression          (BobguiBoolFilter          *self,
                                                                 BobguiExpression          *expression);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_bool_filter_get_invert              (BobguiBoolFilter          *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_bool_filter_set_invert              (BobguiBoolFilter          *self,
                                                                 gboolean                invert);


G_END_DECLS

