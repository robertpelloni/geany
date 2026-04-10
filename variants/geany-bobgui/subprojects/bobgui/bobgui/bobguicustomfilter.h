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

G_BEGIN_DECLS

/**
 * BobguiCustomFilterFunc:
 * @item: (type GObject): the item to be matched
 * @user_data: user data
 *
 * User function that is called to determine if the @item should be matched.
 *
 * If the filter matches the item, this function must return true.
 * If the item should be filtered out, false must be returned.
 *
 * Returns: true to keep the item around
 */
typedef gboolean (* BobguiCustomFilterFunc) (gpointer item, gpointer user_data);

#define BOBGUI_TYPE_CUSTOM_FILTER             (bobgui_custom_filter_get_type ())
GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiCustomFilter, bobgui_custom_filter, BOBGUI, CUSTOM_FILTER, BobguiFilter)
GDK_AVAILABLE_IN_ALL
BobguiCustomFilter *       bobgui_custom_filter_new                   (BobguiCustomFilterFunc     match_func,
                                                                 gpointer                user_data,
                                                                 GDestroyNotify          user_destroy);

GDK_AVAILABLE_IN_ALL
void                    bobgui_custom_filter_set_filter_func       (BobguiCustomFilter        *self,
                                                                 BobguiCustomFilterFunc     match_func,
                                                                 gpointer                user_data,
                                                                 GDestroyNotify          user_destroy);

G_END_DECLS

