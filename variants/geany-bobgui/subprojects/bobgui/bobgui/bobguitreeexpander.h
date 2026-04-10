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

#include <bobgui/bobguitreelistmodel.h>
#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_TREE_EXPANDER         (bobgui_tree_expander_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiTreeExpander, bobgui_tree_expander, BOBGUI, TREE_EXPANDER, BobguiWidget)

GDK_AVAILABLE_IN_ALL
BobguiWidget *             bobgui_tree_expander_new                  (void);

GDK_AVAILABLE_IN_ALL
BobguiWidget *             bobgui_tree_expander_get_child            (BobguiTreeExpander        *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_tree_expander_set_child            (BobguiTreeExpander        *self,
                                                                BobguiWidget              *child);

GDK_AVAILABLE_IN_ALL
gpointer                bobgui_tree_expander_get_item             (BobguiTreeExpander        *self);
GDK_AVAILABLE_IN_ALL
BobguiTreeListRow *        bobgui_tree_expander_get_list_row         (BobguiTreeExpander        *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_tree_expander_set_list_row         (BobguiTreeExpander        *self,
                                                                BobguiTreeListRow         *list_row);
GDK_AVAILABLE_IN_4_10
gboolean                bobgui_tree_expander_get_indent_for_depth (BobguiTreeExpander        *self);
GDK_AVAILABLE_IN_4_10
void                    bobgui_tree_expander_set_indent_for_depth (BobguiTreeExpander        *self,
                                                                gboolean                indent_for_depth);
GDK_AVAILABLE_IN_4_6
gboolean                bobgui_tree_expander_get_indent_for_icon  (BobguiTreeExpander        *self);
GDK_AVAILABLE_IN_4_6
void                    bobgui_tree_expander_set_indent_for_icon  (BobguiTreeExpander        *self,
                                                                gboolean               indent_for_icon);
GDK_AVAILABLE_IN_4_10
gboolean                bobgui_tree_expander_get_hide_expander    (BobguiTreeExpander        *self);
GDK_AVAILABLE_IN_4_10
void                    bobgui_tree_expander_set_hide_expander    (BobguiTreeExpander        *self,
                                                                gboolean                hide_expander);

G_END_DECLS

