/* BobguiCellRendererCombo
 * Copyright (C) 2004 Lorenzo Gil Sanchez
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/deprecated/bobguitreemodel.h>
#include <bobgui/deprecated/bobguicellrenderertext.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_CELL_RENDERER_COMBO		(bobgui_cell_renderer_combo_get_type ())
#define BOBGUI_CELL_RENDERER_COMBO(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_CELL_RENDERER_COMBO, BobguiCellRendererCombo))
#define BOBGUI_IS_CELL_RENDERER_COMBO(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_CELL_RENDERER_COMBO))

typedef struct _BobguiCellRendererCombo              BobguiCellRendererCombo;

GDK_AVAILABLE_IN_ALL
GType            bobgui_cell_renderer_combo_get_type (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10
BobguiCellRenderer *bobgui_cell_renderer_combo_new      (void);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiCellRendererCombo, g_object_unref)

G_END_DECLS

