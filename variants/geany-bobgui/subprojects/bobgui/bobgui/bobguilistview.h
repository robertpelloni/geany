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

#include <bobgui/bobguilistbase.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_LIST_VIEW         (bobgui_list_view_get_type ())
#define BOBGUI_LIST_VIEW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_LIST_VIEW, BobguiListView))
#define BOBGUI_LIST_VIEW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_LIST_VIEW, BobguiListViewClass))
#define BOBGUI_IS_LIST_VIEW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_LIST_VIEW))
#define BOBGUI_IS_LIST_VIEW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_LIST_VIEW))
#define BOBGUI_LIST_VIEW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_LIST_VIEW, BobguiListViewClass))

typedef struct _BobguiListView BobguiListView;
typedef struct _BobguiListViewClass BobguiListViewClass;

GDK_AVAILABLE_IN_ALL
GType           bobgui_list_view_get_type                          (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_list_view_new                               (BobguiSelectionModel      *model,
                                                                 BobguiListItemFactory     *factory);

GDK_AVAILABLE_IN_ALL
BobguiSelectionModel *
                bobgui_list_view_get_model                         (BobguiListView            *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_list_view_set_model                         (BobguiListView            *self,
                                                                 BobguiSelectionModel      *model);
GDK_AVAILABLE_IN_ALL
void            bobgui_list_view_set_factory                       (BobguiListView            *self,
                                                                 BobguiListItemFactory     *factory);
GDK_AVAILABLE_IN_ALL
BobguiListItemFactory *
                bobgui_list_view_get_factory                       (BobguiListView            *self);

GDK_AVAILABLE_IN_4_12
void            bobgui_list_view_set_header_factory                (BobguiListView            *self,
                                                                 BobguiListItemFactory     *factory);
GDK_AVAILABLE_IN_4_12
BobguiListItemFactory *
                bobgui_list_view_get_header_factory                (BobguiListView            *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_list_view_set_show_separators               (BobguiListView            *self,
                                                                 gboolean                show_separators);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_list_view_get_show_separators               (BobguiListView            *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_list_view_set_single_click_activate         (BobguiListView            *self,
                                                                 gboolean                single_click_activate);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_list_view_get_single_click_activate         (BobguiListView            *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_list_view_set_enable_rubberband             (BobguiListView            *self,
                                                                 gboolean                enable_rubberband);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_list_view_get_enable_rubberband             (BobguiListView            *self);

GDK_AVAILABLE_IN_4_12
void            bobgui_list_view_set_tab_behavior                  (BobguiListView            *self,
                                                                 BobguiListTabBehavior      tab_behavior);
GDK_AVAILABLE_IN_4_12
BobguiListTabBehavior
                bobgui_list_view_get_tab_behavior                  (BobguiListView            *self);

GDK_AVAILABLE_IN_4_12
void            bobgui_list_view_scroll_to                         (BobguiListView            *self,
                                                                 guint                   pos,
                                                                 BobguiListScrollFlags      flags,
                                                                 BobguiScrollInfo          *scroll);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiListView, g_object_unref)

G_END_DECLS

