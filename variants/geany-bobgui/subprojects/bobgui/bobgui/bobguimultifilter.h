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

#include <bobgui/bobguifilter.h>
#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_MULTI_FILTER             (bobgui_multi_filter_get_type ())
GDK_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BobguiMultiFilter, bobgui_multi_filter, BOBGUI, MULTI_FILTER, BobguiFilter)

GDK_AVAILABLE_IN_ALL
void                    bobgui_multi_filter_append                 (BobguiMultiFilter         *self,
                                                                 BobguiFilter              *filter);
GDK_AVAILABLE_IN_ALL
void                    bobgui_multi_filter_remove                 (BobguiMultiFilter         *self,
                                                                 guint                   position);

#define BOBGUI_TYPE_ANY_FILTER             (bobgui_any_filter_get_type ())
GDK_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BobguiAnyFilter, bobgui_any_filter, BOBGUI, ANY_FILTER, BobguiMultiFilter)
GDK_AVAILABLE_IN_ALL
BobguiAnyFilter *          bobgui_any_filter_new                      (void);

#define BOBGUI_TYPE_EVERY_FILTER             (bobgui_every_filter_get_type ())
GDK_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (BobguiEveryFilter, bobgui_every_filter, BOBGUI, EVERY_FILTER, BobguiMultiFilter)
GDK_AVAILABLE_IN_ALL
BobguiEveryFilter *        bobgui_every_filter_new                    (void);


G_END_DECLS

