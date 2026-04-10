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

#define BOBGUI_TYPE_SLICE_LIST_MODEL (bobgui_slice_list_model_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiSliceListModel, bobgui_slice_list_model, BOBGUI, SLICE_LIST_MODEL, GObject)

GDK_AVAILABLE_IN_ALL
BobguiSliceListModel *     bobgui_slice_list_model_new                (GListModel             *model,
                                                                 guint                   offset,
                                                                 guint                   size);

GDK_AVAILABLE_IN_ALL
void                    bobgui_slice_list_model_set_model          (BobguiSliceListModel      *self,
                                                                 GListModel             *model);
GDK_AVAILABLE_IN_ALL
GListModel *            bobgui_slice_list_model_get_model          (BobguiSliceListModel      *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_slice_list_model_set_offset         (BobguiSliceListModel      *self,
                                                                 guint                   offset);
GDK_AVAILABLE_IN_ALL
guint                   bobgui_slice_list_model_get_offset         (BobguiSliceListModel      *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_slice_list_model_set_size           (BobguiSliceListModel      *self,
                                                                 guint                   size);
GDK_AVAILABLE_IN_ALL
guint                   bobgui_slice_list_model_get_size           (BobguiSliceListModel      *self);

G_END_DECLS

