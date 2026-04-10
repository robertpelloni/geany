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
#include <bobgui/bobguiwidget.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_MAP_LIST_MODEL (bobgui_map_list_model_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiMapListModel, bobgui_map_list_model, BOBGUI, MAP_LIST_MODEL, GObject)

/**
 * BobguiMapListModelMapFunc:
 * @item: (type GObject) (transfer full): The item to map
 * @user_data: user data
 *
 * User function that is called to map an @item of the original model to
 * an item expected by the map model.
 *
 * The returned items must conform to the item type of the model they are
 * used with.
 *
 * Returns: (type GObject) (transfer full): The item to map to
 */
typedef gpointer (* BobguiMapListModelMapFunc) (gpointer item, gpointer user_data);

GDK_AVAILABLE_IN_ALL
BobguiMapListModel *       bobgui_map_list_model_new                  (GListModel             *model,
                                                                 BobguiMapListModelMapFunc  map_func,
                                                                 gpointer                user_data,
                                                                 GDestroyNotify          user_destroy);

GDK_AVAILABLE_IN_ALL
void                    bobgui_map_list_model_set_map_func         (BobguiMapListModel        *self,
                                                                 BobguiMapListModelMapFunc  map_func,
                                                                 gpointer                user_data,
                                                                 GDestroyNotify          user_destroy);
GDK_AVAILABLE_IN_ALL
void                    bobgui_map_list_model_set_model            (BobguiMapListModel        *self,
                                                                 GListModel             *model);
GDK_AVAILABLE_IN_ALL
GListModel *            bobgui_map_list_model_get_model            (BobguiMapListModel        *self);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_map_list_model_has_map              (BobguiMapListModel        *self);

G_END_DECLS

