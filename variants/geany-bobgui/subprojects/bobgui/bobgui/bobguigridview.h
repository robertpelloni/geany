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

#include <bobgui/bobguilistbase.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_GRID_VIEW         (bobgui_grid_view_get_type ())
#define BOBGUI_GRID_VIEW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_GRID_VIEW, BobguiGridView))
#define BOBGUI_GRID_VIEW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_GRID_VIEW, BobguiGridViewClass))
#define BOBGUI_IS_GRID_VIEW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_GRID_VIEW))
#define BOBGUI_IS_GRID_VIEW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_GRID_VIEW))
#define BOBGUI_GRID_VIEW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_GRID_VIEW, BobguiGridViewClass))

typedef struct _BobguiGridView BobguiGridView;
typedef struct _BobguiGridViewClass BobguiGridViewClass;

GDK_AVAILABLE_IN_ALL
GType           bobgui_grid_view_get_type                          (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_grid_view_new                               (BobguiSelectionModel      *model,
                                                                 BobguiListItemFactory     *factory);

GDK_AVAILABLE_IN_ALL
BobguiSelectionModel *
                bobgui_grid_view_get_model                         (BobguiGridView            *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_grid_view_set_model                         (BobguiGridView            *self,
                                                                 BobguiSelectionModel      *model);
GDK_AVAILABLE_IN_ALL
void            bobgui_grid_view_set_factory                       (BobguiGridView            *self,
                                                                 BobguiListItemFactory     *factory);
GDK_AVAILABLE_IN_ALL
BobguiListItemFactory *
                bobgui_grid_view_get_factory                       (BobguiGridView            *self);
GDK_AVAILABLE_IN_ALL
guint           bobgui_grid_view_get_min_columns                   (BobguiGridView            *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_grid_view_set_min_columns                   (BobguiGridView            *self,
                                                                 guint                   min_columns);
GDK_AVAILABLE_IN_ALL
guint           bobgui_grid_view_get_max_columns                   (BobguiGridView            *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_grid_view_set_max_columns                   (BobguiGridView            *self,
                                                                 guint                   max_columns);
GDK_AVAILABLE_IN_ALL
void            bobgui_grid_view_set_enable_rubberband             (BobguiGridView            *self,
                                                                 gboolean                enable_rubberband);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_grid_view_get_enable_rubberband             (BobguiGridView            *self);

GDK_AVAILABLE_IN_4_12
void            bobgui_grid_view_set_tab_behavior                  (BobguiGridView            *self,
                                                                 BobguiListTabBehavior      tab_behavior);
GDK_AVAILABLE_IN_4_12
BobguiListTabBehavior
                bobgui_grid_view_get_tab_behavior                  (BobguiGridView            *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_grid_view_set_single_click_activate         (BobguiGridView            *self,
                                                                 gboolean                single_click_activate);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_grid_view_get_single_click_activate         (BobguiGridView            *self);

GDK_AVAILABLE_IN_4_12
void            bobgui_grid_view_scroll_to                         (BobguiGridView            *self,
                                                                 guint                   pos,
                                                                 BobguiListScrollFlags      flags,
                                                                 BobguiScrollInfo          *scroll);


G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiGridView, g_object_unref)

G_END_DECLS

