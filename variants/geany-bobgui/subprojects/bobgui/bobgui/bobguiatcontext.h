/* bobguiatcontext.h: Assistive technology context
 *
 * Copyright 2020  GNOME Foundation
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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguitypes.h>
#include <bobgui/bobguienums.h>
#include <bobgui/bobguiaccessible.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_AT_CONTEXT (bobgui_at_context_get_type())

GDK_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BobguiATContext, bobgui_at_context, BOBGUI, AT_CONTEXT, GObject)

GDK_AVAILABLE_IN_ALL
BobguiAccessible *         bobgui_at_context_get_accessible           (BobguiATContext      *self);
GDK_AVAILABLE_IN_ALL
BobguiAccessibleRole       bobgui_at_context_get_accessible_role      (BobguiATContext      *self);

GDK_AVAILABLE_IN_ALL
BobguiATContext *          bobgui_at_context_create                   (BobguiAccessibleRole  accessible_role,
                                                                 BobguiAccessible     *accessible,
                                                                 GdkDisplay        *display);

G_END_DECLS
