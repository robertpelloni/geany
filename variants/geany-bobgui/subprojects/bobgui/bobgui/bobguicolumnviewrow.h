/*
 * Copyright © 2023 Benjamin Otte
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

G_BEGIN_DECLS

#define BOBGUI_TYPE_COLUMN_VIEW_ROW (bobgui_column_view_row_get_type ())
GDK_AVAILABLE_IN_4_12
GDK_DECLARE_INTERNAL_TYPE(BobguiColumnViewRow, bobgui_column_view_row, BOBGUI, COLUMN_VIEW_ROW, GObject)

GDK_AVAILABLE_IN_4_12
gpointer        bobgui_column_view_row_get_item                    (BobguiColumnViewRow       *self);
GDK_AVAILABLE_IN_4_12
guint           bobgui_column_view_row_get_position                (BobguiColumnViewRow       *self) G_GNUC_PURE;
GDK_AVAILABLE_IN_4_12
gboolean        bobgui_column_view_row_get_selected                (BobguiColumnViewRow       *self) G_GNUC_PURE;
GDK_AVAILABLE_IN_4_12
gboolean        bobgui_column_view_row_get_selectable              (BobguiColumnViewRow       *self) G_GNUC_PURE;
GDK_AVAILABLE_IN_4_12
void            bobgui_column_view_row_set_selectable              (BobguiColumnViewRow       *self,
                                                                 gboolean                selectable);
GDK_AVAILABLE_IN_4_12
gboolean        bobgui_column_view_row_get_activatable             (BobguiColumnViewRow       *self) G_GNUC_PURE;
GDK_AVAILABLE_IN_4_12
void            bobgui_column_view_row_set_activatable             (BobguiColumnViewRow       *self,
                                                                 gboolean                activatable);
GDK_AVAILABLE_IN_4_12
gboolean        bobgui_column_view_row_get_focusable               (BobguiColumnViewRow       *self) G_GNUC_PURE;
GDK_AVAILABLE_IN_4_12
void            bobgui_column_view_row_set_focusable               (BobguiColumnViewRow       *self,
                                                                 gboolean                focusable);
GDK_AVAILABLE_IN_4_12
const char *    bobgui_column_view_row_get_accessible_description  (BobguiColumnViewRow       *self);
GDK_AVAILABLE_IN_4_12
void            bobgui_column_view_row_set_accessible_description  (BobguiColumnViewRow       *self,
                                                                 const char             *description);
GDK_AVAILABLE_IN_4_12
const char *    bobgui_column_view_row_get_accessible_label        (BobguiColumnViewRow       *self);
GDK_AVAILABLE_IN_4_12
void            bobgui_column_view_row_set_accessible_label        (BobguiColumnViewRow       *self,
                                                                 const char             *label);

G_END_DECLS

