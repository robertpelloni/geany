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
#include <bobgui/bobguisorter.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_SORT_LIST_MODEL (bobgui_sort_list_model_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiSortListModel, bobgui_sort_list_model, BOBGUI, SORT_LIST_MODEL, GObject)

GDK_AVAILABLE_IN_ALL
BobguiSortListModel *      bobgui_sort_list_model_new                 (GListModel            *model,
                                                                 BobguiSorter             *sorter);
GDK_AVAILABLE_IN_ALL
void                    bobgui_sort_list_model_set_sorter          (BobguiSortListModel       *self,
                                                                 BobguiSorter              *sorter);
GDK_AVAILABLE_IN_ALL
BobguiSorter *             bobgui_sort_list_model_get_sorter          (BobguiSortListModel       *self);

GDK_AVAILABLE_IN_4_12
void                    bobgui_sort_list_model_set_section_sorter  (BobguiSortListModel       *self,
                                                                 BobguiSorter              *sorter);
GDK_AVAILABLE_IN_4_12
BobguiSorter *             bobgui_sort_list_model_get_section_sorter  (BobguiSortListModel       *self);

GDK_AVAILABLE_IN_ALL
void                    bobgui_sort_list_model_set_model           (BobguiSortListModel       *self,
                                                                 GListModel             *model);
GDK_AVAILABLE_IN_ALL
GListModel *            bobgui_sort_list_model_get_model           (BobguiSortListModel       *self);

GDK_AVAILABLE_IN_ALL
void                    bobgui_sort_list_model_set_incremental     (BobguiSortListModel       *self,
                                                                 gboolean                incremental);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_sort_list_model_get_incremental     (BobguiSortListModel       *self);

GDK_AVAILABLE_IN_ALL
guint                   bobgui_sort_list_model_get_pending         (BobguiSortListModel       *self);

G_END_DECLS

