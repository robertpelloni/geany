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

#include <bobgui/bobguicolumnview.h>
#include <bobgui/bobguisorter.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_COLUMN_VIEW_COLUMN         (bobgui_column_view_column_get_type ())
#define BOBGUI_COLUMN_VIEW_COLUMN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_COLUMN_VIEW_COLUMN, BobguiColumnViewColumn))
#define BOBGUI_COLUMN_VIEW_COLUMN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_COLUMN_VIEW_COLUMN, BobguiColumnViewColumnClass))
#define BOBGUI_IS_COLUMN_VIEW_COLUMN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_COLUMN_VIEW_COLUMN))
#define BOBGUI_IS_COLUMN_VIEW_COLUMN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_COLUMN_VIEW_COLUMN))
#define BOBGUI_COLUMN_VIEW_COLUMN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_COLUMN_VIEW_COLUMN, BobguiColumnViewColumnClass))
G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiColumnViewColumn, g_object_unref)

typedef struct _BobguiColumnViewColumnClass BobguiColumnViewColumnClass;

GDK_AVAILABLE_IN_ALL
GType                   bobgui_column_view_column_get_type                 (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiColumnViewColumn *   bobgui_column_view_column_new                      (const char             *title,
                                                                         BobguiListItemFactory     *factory);

GDK_AVAILABLE_IN_ALL
BobguiColumnView *         bobgui_column_view_column_get_column_view          (BobguiColumnViewColumn    *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_column_view_column_set_factory              (BobguiColumnViewColumn    *self,
                                                                         BobguiListItemFactory     *factory);
GDK_AVAILABLE_IN_ALL
BobguiListItemFactory *    bobgui_column_view_column_get_factory              (BobguiColumnViewColumn    *self);

GDK_AVAILABLE_IN_ALL
void                    bobgui_column_view_column_set_title                (BobguiColumnViewColumn    *self,
                                                                         const char             *title);
GDK_AVAILABLE_IN_ALL
const char *            bobgui_column_view_column_get_title                (BobguiColumnViewColumn    *self);

GDK_AVAILABLE_IN_ALL
void                    bobgui_column_view_column_set_sorter               (BobguiColumnViewColumn    *self,
                                                                         BobguiSorter              *sorter);
GDK_AVAILABLE_IN_ALL
BobguiSorter *             bobgui_column_view_column_get_sorter               (BobguiColumnViewColumn    *self);

GDK_AVAILABLE_IN_ALL
void                    bobgui_column_view_column_set_visible              (BobguiColumnViewColumn    *self,
                                                                         gboolean                visible);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_column_view_column_get_visible              (BobguiColumnViewColumn    *self);

GDK_AVAILABLE_IN_ALL

void                    bobgui_column_view_column_set_header_menu          (BobguiColumnViewColumn    *self,
                                                                         GMenuModel             *menu);
GDK_AVAILABLE_IN_ALL
GMenuModel *            bobgui_column_view_column_get_header_menu          (BobguiColumnViewColumn    *self);

GDK_AVAILABLE_IN_ALL
void                    bobgui_column_view_column_set_fixed_width          (BobguiColumnViewColumn    *self,
                                                                         int                     fixed_width);
GDK_AVAILABLE_IN_ALL
int                     bobgui_column_view_column_get_fixed_width          (BobguiColumnViewColumn    *self);

GDK_AVAILABLE_IN_ALL
void                    bobgui_column_view_column_set_resizable            (BobguiColumnViewColumn    *self,
                                                                         gboolean                resizable);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_column_view_column_get_resizable            (BobguiColumnViewColumn    *self);

GDK_AVAILABLE_IN_ALL
void                    bobgui_column_view_column_set_expand               (BobguiColumnViewColumn    *self,
                                                                         gboolean                expand);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_column_view_column_get_expand               (BobguiColumnViewColumn    *self);

GDK_AVAILABLE_IN_4_10
void                    bobgui_column_view_column_set_id                   (BobguiColumnViewColumn    *self,
                                                                         const char             *id);
GDK_AVAILABLE_IN_4_10
const char *            bobgui_column_view_column_get_id                   (BobguiColumnViewColumn    *self);

G_END_DECLS

