/*
 * Copyright © 2018 Benjamin Otte
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

#include <gio/gio.h>
#include <bobgui/bobguifilter.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_FILTER_LIST_MODEL (bobgui_filter_list_model_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiFilterListModel, bobgui_filter_list_model, BOBGUI, FILTER_LIST_MODEL, GObject)

GDK_AVAILABLE_IN_ALL
BobguiFilterListModel *    bobgui_filter_list_model_new               (GListModel             *model,
                                                                 BobguiFilter              *filter);

GDK_AVAILABLE_IN_ALL
void                    bobgui_filter_list_model_set_filter        (BobguiFilterListModel     *self,
                                                                 BobguiFilter              *filter);
GDK_AVAILABLE_IN_ALL
BobguiFilter *             bobgui_filter_list_model_get_filter        (BobguiFilterListModel     *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_filter_list_model_set_model         (BobguiFilterListModel     *self,
                                                                 GListModel             *model);
GDK_AVAILABLE_IN_ALL
GListModel *            bobgui_filter_list_model_get_model         (BobguiFilterListModel     *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_filter_list_model_set_incremental   (BobguiFilterListModel     *self,
                                                                 gboolean                incremental);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_filter_list_model_get_incremental   (BobguiFilterListModel     *self);
GDK_AVAILABLE_IN_ALL
guint                   bobgui_filter_list_model_get_pending       (BobguiFilterListModel     *self);

GDK_AVAILABLE_IN_4_20
gboolean                bobgui_filter_list_model_get_watch_items   (BobguiFilterListModel     *self);
GDK_AVAILABLE_IN_4_20
void                    bobgui_filter_list_model_set_watch_items   (BobguiFilterListModel     *self,
                                                                 gboolean                watch_items);

G_END_DECLS

