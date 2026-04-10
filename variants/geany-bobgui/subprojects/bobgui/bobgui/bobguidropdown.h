/*
 * Copyright © 2018 Red Hat, Inc.
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
 * Authors: Matthias Clasen
 */

#pragma once

#include <bobgui/bobguiwidget.h>
#include <bobgui/bobguiexpression.h>
#include "bobgui/bobguistringfilter.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_DROP_DOWN         (bobgui_drop_down_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiDropDown, bobgui_drop_down, BOBGUI, DROP_DOWN, BobguiWidget)

GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_drop_down_new                               (GListModel             *model,
                                                                 BobguiExpression          *expression);

GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_drop_down_new_from_strings                  (const char * const *    strings);

GDK_AVAILABLE_IN_ALL
void            bobgui_drop_down_set_model                         (BobguiDropDown            *self,
                                                                 GListModel             *model);
GDK_AVAILABLE_IN_ALL
GListModel *    bobgui_drop_down_get_model                         (BobguiDropDown            *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_drop_down_set_selected                      (BobguiDropDown            *self,
                                                                 guint                   position);
GDK_AVAILABLE_IN_ALL
guint           bobgui_drop_down_get_selected                      (BobguiDropDown            *self);

GDK_AVAILABLE_IN_ALL
gpointer        bobgui_drop_down_get_selected_item                 (BobguiDropDown            *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_drop_down_set_factory                       (BobguiDropDown            *self,
                                                                 BobguiListItemFactory     *factory);
GDK_AVAILABLE_IN_ALL
BobguiListItemFactory *
                bobgui_drop_down_get_factory                       (BobguiDropDown            *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_drop_down_set_list_factory                  (BobguiDropDown            *self,
                                                                 BobguiListItemFactory     *factory);
GDK_AVAILABLE_IN_ALL
BobguiListItemFactory *
                bobgui_drop_down_get_list_factory                  (BobguiDropDown            *self);

GDK_AVAILABLE_IN_4_12
void            bobgui_drop_down_set_header_factory                (BobguiDropDown            *self,
                                                                 BobguiListItemFactory     *factory);
GDK_AVAILABLE_IN_4_12
BobguiListItemFactory *
                bobgui_drop_down_get_header_factory                (BobguiDropDown            *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_drop_down_set_expression                    (BobguiDropDown            *self,
                                                                 BobguiExpression          *expression);
GDK_AVAILABLE_IN_ALL
BobguiExpression * bobgui_drop_down_get_expression                    (BobguiDropDown            *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_drop_down_set_enable_search                 (BobguiDropDown            *self,
                                                                 gboolean                enable_search);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_drop_down_get_enable_search                 (BobguiDropDown            *self);

GDK_AVAILABLE_IN_4_6
void            bobgui_drop_down_set_show_arrow                    (BobguiDropDown            *self,
                                                                 gboolean                show_arrow);
GDK_AVAILABLE_IN_4_6
gboolean        bobgui_drop_down_get_show_arrow                    (BobguiDropDown            *self);

GDK_AVAILABLE_IN_4_12
void            bobgui_drop_down_set_search_match_mode             (BobguiDropDown            *self,
                                                                 BobguiStringFilterMatchMode search_match_mode);
GDK_AVAILABLE_IN_4_12
BobguiStringFilterMatchMode
                bobgui_drop_down_get_search_match_mode             (BobguiDropDown            *self);

G_END_DECLS

