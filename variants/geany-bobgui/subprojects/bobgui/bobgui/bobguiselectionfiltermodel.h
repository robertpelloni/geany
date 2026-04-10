/*
 * Copyright © 2020 Red Hat, Inc.
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


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gio/gio.h>
#include <bobgui/bobguiselectionmodel.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SELECTION_FILTER_MODEL (bobgui_selection_filter_model_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiSelectionFilterModel, bobgui_selection_filter_model, BOBGUI, SELECTION_FILTER_MODEL, GObject)

GDK_AVAILABLE_IN_ALL
BobguiSelectionFilterModel * bobgui_selection_filter_model_new          (BobguiSelectionModel           *model);

GDK_AVAILABLE_IN_ALL
void                      bobgui_selection_filter_model_set_model    (BobguiSelectionFilterModel     *self,
                                                                   BobguiSelectionModel           *model);
GDK_AVAILABLE_IN_ALL
BobguiSelectionModel *       bobgui_selection_filter_model_get_model    (BobguiSelectionFilterModel     *self);

G_END_DECLS

