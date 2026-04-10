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

#include <bobgui/bobguitypes.h>
#include <bobgui/bobguisortlistmodel.h>
#include <bobgui/bobguiselectionmodel.h>
#include <bobgui/bobguisorter.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_COLUMN_VIEW         (bobgui_column_view_get_type ())
#define BOBGUI_COLUMN_VIEW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_COLUMN_VIEW, BobguiColumnView))
#define BOBGUI_COLUMN_VIEW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_COLUMN_VIEW, BobguiColumnViewClass))
#define BOBGUI_IS_COLUMN_VIEW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_COLUMN_VIEW))
#define BOBGUI_IS_COLUMN_VIEW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_COLUMN_VIEW))
#define BOBGUI_COLUMN_VIEW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_COLUMN_VIEW, BobguiColumnViewClass))


typedef struct _BobguiColumnView BobguiColumnView;
typedef struct _BobguiColumnViewClass BobguiColumnViewClass;
/* forward declaration */
typedef struct _BobguiColumnViewColumn BobguiColumnViewColumn;

GDK_AVAILABLE_IN_ALL
GType           bobgui_column_view_get_type                        (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_column_view_new                             (BobguiSelectionModel      *model);

GDK_AVAILABLE_IN_ALL
GListModel *    bobgui_column_view_get_columns                     (BobguiColumnView          *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_column_view_append_column                   (BobguiColumnView          *self,
                                                                 BobguiColumnViewColumn    *column);
GDK_AVAILABLE_IN_ALL
void            bobgui_column_view_remove_column                   (BobguiColumnView          *self,
                                                                 BobguiColumnViewColumn    *column);
GDK_AVAILABLE_IN_ALL
void            bobgui_column_view_insert_column                   (BobguiColumnView          *self,
                                                                 guint                   position,
                                                                 BobguiColumnViewColumn    *column);

GDK_AVAILABLE_IN_ALL
BobguiSelectionModel *
                bobgui_column_view_get_model                       (BobguiColumnView          *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_column_view_set_model                       (BobguiColumnView          *self,
                                                                 BobguiSelectionModel      *model);

GDK_AVAILABLE_IN_ALL
gboolean        bobgui_column_view_get_show_row_separators         (BobguiColumnView          *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_column_view_set_show_row_separators         (BobguiColumnView          *self,
                                                                 gboolean                show_row_separators);

GDK_AVAILABLE_IN_ALL
gboolean        bobgui_column_view_get_show_column_separators      (BobguiColumnView          *self);
GDK_AVAILABLE_IN_ALL
void            bobgui_column_view_set_show_column_separators      (BobguiColumnView          *self,
                                                                 gboolean                show_column_separators);

GDK_AVAILABLE_IN_ALL
BobguiSorter *     bobgui_column_view_get_sorter                      (BobguiColumnView          *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_column_view_sort_by_column                  (BobguiColumnView          *self,
                                                                 BobguiColumnViewColumn    *column,
                                                                 BobguiSortType             direction);

GDK_AVAILABLE_IN_ALL
void            bobgui_column_view_set_single_click_activate       (BobguiColumnView          *self,
                                                                 gboolean                single_click_activate);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_column_view_get_single_click_activate       (BobguiColumnView          *self);

GDK_AVAILABLE_IN_ALL

void            bobgui_column_view_set_reorderable                 (BobguiColumnView          *self,
                                                                 gboolean                reorderable);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_column_view_get_reorderable                 (BobguiColumnView          *self);

GDK_AVAILABLE_IN_ALL
void            bobgui_column_view_set_enable_rubberband           (BobguiColumnView          *self,
                                                                 gboolean                enable_rubberband);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_column_view_get_enable_rubberband           (BobguiColumnView          *self);

GDK_AVAILABLE_IN_4_12
void            bobgui_column_view_set_tab_behavior                (BobguiColumnView          *self,
                                                                 BobguiListTabBehavior      tab_behavior);
GDK_AVAILABLE_IN_4_12
BobguiListTabBehavior
                bobgui_column_view_get_tab_behavior                (BobguiColumnView          *self);

GDK_AVAILABLE_IN_4_12
void            bobgui_column_view_set_row_factory                 (BobguiColumnView          *self,
                                                                 BobguiListItemFactory     *factory);
GDK_AVAILABLE_IN_4_12
BobguiListItemFactory *
                bobgui_column_view_get_row_factory                 (BobguiColumnView          *self);

GDK_AVAILABLE_IN_4_12
void            bobgui_column_view_set_header_factory              (BobguiColumnView          *self,
                                                                 BobguiListItemFactory     *factory);
GDK_AVAILABLE_IN_4_12
BobguiListItemFactory *
                bobgui_column_view_get_header_factory              (BobguiColumnView          *self);

GDK_AVAILABLE_IN_4_12
void            bobgui_column_view_scroll_to                       (BobguiColumnView          *self,
                                                                 guint                   pos,
                                                                 BobguiColumnViewColumn    *column,
                                                                 BobguiListScrollFlags      flags,
                                                                 BobguiScrollInfo          *scroll);

G_END_DECLS

