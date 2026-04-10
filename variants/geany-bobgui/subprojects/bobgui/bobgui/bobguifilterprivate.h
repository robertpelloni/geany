/*
 * Copyright © 2025 Igalia S.L.
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
 * Authors: Georges Basile Stavracas Neto <feaneron@igalia.com>
 */

#pragma once

#include "bobguifilter.h"

#include <bobgui/bobguiexpression.h>

G_BEGIN_DECLS

/*<private>
 * BobguiFilterWatchCallback:
 * @item: (type GObject): the item to be watched
 * @user_data: user data
 *
 * User function that is called @item changes while being watches.
 *
 * Since: 4.20
 */
typedef void (*BobguiFilterWatchCallback) (gpointer item,
                                        gpointer user_data);

typedef struct _BobguiFilterClassPrivate
{
  /* private vfuncs */
  gpointer              (* watch)                               (BobguiFilter              *self,
                                                                 gpointer                item,
                                                                 BobguiFilterWatchCallback  watch_func,
                                                                 gpointer                user_data,
                                                                 GDestroyNotify          destroy);

  void                  (* unwatch)                             (BobguiFilter              *self,
                                                                 gpointer                watch);
} BobguiFilterClassPrivate;

gpointer bobgui_filter_watch (BobguiFilter              *self,
                           gpointer                item,
                           BobguiFilterWatchCallback  watch_func,
                           gpointer                user_data,
                           GDestroyNotify          destroy);

void bobgui_filter_unwatch (BobguiFilter *self,
                         gpointer   watch);

G_END_DECLS
