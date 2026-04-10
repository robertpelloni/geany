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

#include <gio/gio.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_LIST_LIST_MODEL         (bobgui_list_list_model_get_type ())
#define BOBGUI_LIST_LIST_MODEL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_LIST_LIST_MODEL, BobguiListListModel))
#define BOBGUI_LIST_LIST_MODEL_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_LIST_LIST_MODEL, BobguiListListModelClass))
#define BOBGUI_IS_LIST_LIST_MODEL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_LIST_LIST_MODEL))
#define BOBGUI_IS_LIST_LIST_MODEL_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_LIST_LIST_MODEL))
#define BOBGUI_LIST_LIST_MODEL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_LIST_LIST_MODEL, BobguiListListModelClass))

typedef struct _BobguiListListModel BobguiListListModel;
typedef struct _BobguiListListModelClass BobguiListListModelClass;

GType                   bobgui_list_list_model_get_type            (void) G_GNUC_CONST;

BobguiListListModel *      bobgui_list_list_model_new                 (gpointer                (* get_first) (gpointer),
                                                                 gpointer                (* get_next) (gpointer, gpointer),
                                                                 gpointer                (* get_previous) (gpointer, gpointer),
                                                                 gpointer                (* get_last) (gpointer),
                                                                 gpointer                (* get_item) (gpointer, gpointer),
                                                                 gpointer                data,
                                                                 GDestroyNotify          notify);

BobguiListListModel *      bobgui_list_list_model_new_with_size       (guint                   n_items,
                                                                 gpointer                (* get_first) (gpointer),
                                                                 gpointer                (* get_next) (gpointer, gpointer),
                                                                 gpointer                (* get_previous) (gpointer, gpointer),
                                                                 gpointer                (* get_last) (gpointer),
                                                                 gpointer                (* get_item) (gpointer, gpointer),
                                                                 gpointer                data,
                                                                 GDestroyNotify          notify);

void                    bobgui_list_list_model_item_added          (BobguiListListModel       *self,
                                                                 gpointer                item);
void                    bobgui_list_list_model_item_added_at       (BobguiListListModel       *self,
                                                                 guint                   position);
void                    bobgui_list_list_model_item_removed        (BobguiListListModel       *self,
                                                                 gpointer                previous);
void                    bobgui_list_list_model_item_removed_at     (BobguiListListModel       *self,
                                                                 guint                   position);
void                    bobgui_list_list_model_item_moved          (BobguiListListModel       *self,
                                                                 gpointer                item,
                                                                 gpointer                previous_previous);

void                    bobgui_list_list_model_clear               (BobguiListListModel       *self);


G_END_DECLS

