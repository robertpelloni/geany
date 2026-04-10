/* bobguiplacesviewrow.h
 *
 * Copyright (C) 2015 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include "bobguiwidget.h"
#include "bobguisizegroup.h"
#include "bobguilistbox.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_PLACES_VIEW_ROW (bobgui_places_view_row_get_type())

G_DECLARE_FINAL_TYPE (BobguiPlacesViewRow, bobgui_places_view_row, BOBGUI, PLACES_VIEW_ROW, BobguiListBoxRow)

BobguiWidget*         bobgui_places_view_row_new                       (GVolume            *volume,
                                                                  GMount             *mount);

BobguiWidget*         bobgui_places_view_row_get_eject_button          (BobguiPlacesViewRow   *row);

GMount*            bobgui_places_view_row_get_mount                 (BobguiPlacesViewRow   *row);

GVolume*           bobgui_places_view_row_get_volume                (BobguiPlacesViewRow   *row);

GFile*             bobgui_places_view_row_get_file                  (BobguiPlacesViewRow   *row);

void               bobgui_places_view_row_set_busy                  (BobguiPlacesViewRow   *row,
                                                                  gboolean            is_busy);

gboolean           bobgui_places_view_row_get_is_network            (BobguiPlacesViewRow   *row);

void               bobgui_places_view_row_set_is_network            (BobguiPlacesViewRow   *row,
                                                                  gboolean            is_network);

void               bobgui_places_view_row_set_path_size_group       (BobguiPlacesViewRow   *row,
                                                                  BobguiSizeGroup       *group);

void               bobgui_places_view_row_set_space_size_group      (BobguiPlacesViewRow   *row,
                                                                  BobguiSizeGroup       *group);

G_END_DECLS

