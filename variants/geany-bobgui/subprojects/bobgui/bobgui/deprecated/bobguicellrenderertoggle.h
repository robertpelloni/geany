/* bobguicellrenderertoggle.h
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
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

#include <bobgui/deprecated/bobguicellrenderer.h>


G_BEGIN_DECLS


#define BOBGUI_TYPE_CELL_RENDERER_TOGGLE			(bobgui_cell_renderer_toggle_get_type ())
#define BOBGUI_CELL_RENDERER_TOGGLE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_CELL_RENDERER_TOGGLE, BobguiCellRendererToggle))
#define BOBGUI_IS_CELL_RENDERER_TOGGLE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_CELL_RENDERER_TOGGLE))

typedef struct _BobguiCellRendererToggle              BobguiCellRendererToggle;


GDK_AVAILABLE_IN_ALL
GType            bobgui_cell_renderer_toggle_get_type       (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10
BobguiCellRenderer *bobgui_cell_renderer_toggle_new            (void);

GDK_DEPRECATED_IN_4_10
gboolean         bobgui_cell_renderer_toggle_get_radio      (BobguiCellRendererToggle *toggle);
GDK_DEPRECATED_IN_4_10
void             bobgui_cell_renderer_toggle_set_radio      (BobguiCellRendererToggle *toggle,
                                                          gboolean               radio);

GDK_DEPRECATED_IN_4_10
gboolean        bobgui_cell_renderer_toggle_get_active      (BobguiCellRendererToggle *toggle);
GDK_DEPRECATED_IN_4_10
void            bobgui_cell_renderer_toggle_set_active      (BobguiCellRendererToggle *toggle,
                                                          gboolean               setting);

GDK_DEPRECATED_IN_4_10
gboolean        bobgui_cell_renderer_toggle_get_activatable (BobguiCellRendererToggle *toggle);
GDK_DEPRECATED_IN_4_10
void            bobgui_cell_renderer_toggle_set_activatable (BobguiCellRendererToggle *toggle,
                                                          gboolean               setting);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiCellRendererToggle, g_object_unref)

G_END_DECLS

