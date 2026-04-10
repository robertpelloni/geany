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

#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_NO_SELECTION (bobgui_no_selection_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (BobguiNoSelection, bobgui_no_selection, BOBGUI, NO_SELECTION, GObject)

GDK_AVAILABLE_IN_ALL
BobguiNoSelection *        bobgui_no_selection_new                    (GListModel             *model);

GDK_AVAILABLE_IN_ALL
GListModel *            bobgui_no_selection_get_model              (BobguiNoSelection         *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_no_selection_set_model              (BobguiNoSelection         *self,
                                                                 GListModel             *model);

G_END_DECLS

