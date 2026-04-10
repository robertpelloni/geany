/*
 * Copyright © 2019 Red Hat, Inc.
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
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */

#pragma once

#include <bobgui/bobguitypes.h>
#include <bobgui/bobguiselectionmodel.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_MULTI_SELECTION (bobgui_multi_selection_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiMultiSelection, bobgui_multi_selection, BOBGUI, MULTI_SELECTION, GObject)

GDK_AVAILABLE_IN_ALL
BobguiMultiSelection * bobgui_multi_selection_new                (GListModel           *model);

GDK_AVAILABLE_IN_ALL
GListModel *        bobgui_multi_selection_get_model          (BobguiMultiSelection    *self);
GDK_AVAILABLE_IN_ALL
void                bobgui_multi_selection_set_model          (BobguiMultiSelection    *self,
                                                            GListModel           *model);

G_END_DECLS

