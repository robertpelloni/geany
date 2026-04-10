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

#define BOBGUI_TYPE_TREE_LIST_MODEL (bobgui_tree_list_model_get_type ())
#define BOBGUI_TYPE_TREE_LIST_ROW (bobgui_tree_list_row_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiTreeListModel, bobgui_tree_list_model, BOBGUI, TREE_LIST_MODEL, GObject)
GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiTreeListRow, bobgui_tree_list_row, BOBGUI, TREE_LIST_ROW, GObject)

/**
 * BobguiTreeListModelCreateModelFunc:
 * @item: (type GObject): The item that is being expanded
 * @user_data: User data passed when registering the function
 *
 * Prototype of the function called to create new child models when
 * bobgui_tree_list_row_set_expanded() is called.
 *
 * This function can return %NULL to indicate that @item is guaranteed to be
 * a leaf node and will never have children. If it does not have children but
 * may get children later, it should return an empty model that is filled once
 * children arrive.
 *
 * Returns: (nullable) (transfer full): The model tracking the children of
 *   @item or %NULL if @item can never have children
 */
typedef GListModel * (* BobguiTreeListModelCreateModelFunc) (gpointer item, gpointer user_data);

GDK_AVAILABLE_IN_ALL
BobguiTreeListModel *      bobgui_tree_list_model_new                 (GListModel             *root,
                                                                 gboolean                passthrough,
                                                                 gboolean                autoexpand,
                                                                 BobguiTreeListModelCreateModelFunc create_func,
                                                                 gpointer                user_data,
                                                                 GDestroyNotify          user_destroy);

GDK_AVAILABLE_IN_ALL
GListModel *            bobgui_tree_list_model_get_model           (BobguiTreeListModel       *self);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_tree_list_model_get_passthrough     (BobguiTreeListModel       *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_tree_list_model_set_autoexpand      (BobguiTreeListModel       *self,
                                                                 gboolean                autoexpand);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_tree_list_model_get_autoexpand      (BobguiTreeListModel       *self);

GDK_AVAILABLE_IN_ALL
BobguiTreeListRow *        bobgui_tree_list_model_get_child_row       (BobguiTreeListModel       *self,
                                                                 guint                   position);
GDK_AVAILABLE_IN_ALL
BobguiTreeListRow *        bobgui_tree_list_model_get_row             (BobguiTreeListModel       *self,
                                                                 guint                   position);

GDK_AVAILABLE_IN_ALL
gpointer                bobgui_tree_list_row_get_item              (BobguiTreeListRow         *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_tree_list_row_set_expanded          (BobguiTreeListRow         *self,
                                                                 gboolean                expanded);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_tree_list_row_get_expanded          (BobguiTreeListRow         *self);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_tree_list_row_is_expandable         (BobguiTreeListRow         *self);
GDK_AVAILABLE_IN_ALL
guint                   bobgui_tree_list_row_get_position          (BobguiTreeListRow         *self);
GDK_AVAILABLE_IN_ALL
guint                   bobgui_tree_list_row_get_depth             (BobguiTreeListRow         *self);
GDK_AVAILABLE_IN_ALL
GListModel *            bobgui_tree_list_row_get_children          (BobguiTreeListRow         *self);
GDK_AVAILABLE_IN_ALL
BobguiTreeListRow *        bobgui_tree_list_row_get_parent            (BobguiTreeListRow         *self);
GDK_AVAILABLE_IN_ALL
BobguiTreeListRow *        bobgui_tree_list_row_get_child_row         (BobguiTreeListRow         *self,
                                                                 guint                   position);


G_END_DECLS

