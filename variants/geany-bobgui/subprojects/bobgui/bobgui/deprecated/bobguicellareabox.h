/* bobguicellareabox.h
 *
 * Copyright (C) 2010 Openismus GmbH
 *
 * Authors:
 *      Tristan Van Berkom <tristanvb@openismus.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/deprecated/bobguicellarea.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_CELL_AREA_BOX            (bobgui_cell_area_box_get_type ())
#define BOBGUI_CELL_AREA_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_CELL_AREA_BOX, BobguiCellAreaBox))
#define BOBGUI_IS_CELL_AREA_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_CELL_AREA_BOX))

typedef struct _BobguiCellAreaBox              BobguiCellAreaBox;

GDK_AVAILABLE_IN_ALL
GType        bobgui_cell_area_box_get_type    (void) G_GNUC_CONST;

GDK_DEPRECATED_IN_4_10
BobguiCellArea *bobgui_cell_area_box_new         (void);
GDK_DEPRECATED_IN_4_10
void         bobgui_cell_area_box_pack_start  (BobguiCellAreaBox  *box,
                                            BobguiCellRenderer *renderer,
                                            gboolean         expand,
                                            gboolean         align,
                                            gboolean         fixed);
GDK_DEPRECATED_IN_4_10
void         bobgui_cell_area_box_pack_end    (BobguiCellAreaBox  *box,
                                            BobguiCellRenderer *renderer,
                                            gboolean         expand,
                                            gboolean         align,
                                            gboolean         fixed);
GDK_DEPRECATED_IN_4_10
int          bobgui_cell_area_box_get_spacing (BobguiCellAreaBox  *box);
GDK_DEPRECATED_IN_4_10
void         bobgui_cell_area_box_set_spacing (BobguiCellAreaBox  *box,
                                            int              spacing);

/* Private interaction with BobguiCellAreaBoxContext */
gboolean    _bobgui_cell_area_box_group_visible (BobguiCellAreaBox  *box,
                                              int              group_idx);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiCellAreaBox, g_object_unref)

G_END_DECLS

