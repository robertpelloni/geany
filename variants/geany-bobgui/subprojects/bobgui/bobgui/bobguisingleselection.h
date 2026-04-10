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

#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SINGLE_SELECTION (bobgui_single_selection_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiSingleSelection, bobgui_single_selection, BOBGUI, SINGLE_SELECTION, GObject)

GDK_AVAILABLE_IN_ALL
BobguiSingleSelection *    bobgui_single_selection_new                (GListModel             *model);

GDK_AVAILABLE_IN_ALL
GListModel *            bobgui_single_selection_get_model          (BobguiSingleSelection     *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_single_selection_set_model          (BobguiSingleSelection     *self,
                                                                 GListModel             *model);
GDK_AVAILABLE_IN_ALL
guint                   bobgui_single_selection_get_selected       (BobguiSingleSelection     *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_single_selection_set_selected       (BobguiSingleSelection     *self,
                                                                 guint                   position);
GDK_AVAILABLE_IN_ALL
gpointer                bobgui_single_selection_get_selected_item  (BobguiSingleSelection     *self);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_single_selection_get_autoselect     (BobguiSingleSelection     *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_single_selection_set_autoselect     (BobguiSingleSelection     *self,
                                                                 gboolean                autoselect);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_single_selection_get_can_unselect   (BobguiSingleSelection     *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_single_selection_set_can_unselect   (BobguiSingleSelection     *self,
                                                                 gboolean                can_unselect);

G_END_DECLS

