/* bobguicellview.h
 * Copyright (C) 2002, 2003  Kristian Rietveld <kris@bobgui.org>
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

#include <bobgui/bobguiwidget.h>
#include <bobgui/deprecated/bobguicellrenderer.h>
#include <bobgui/deprecated/bobguicellarea.h>
#include <bobgui/deprecated/bobguicellareacontext.h>
#include <bobgui/deprecated/bobguitreemodel.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_CELL_VIEW                (bobgui_cell_view_get_type ())
#define BOBGUI_CELL_VIEW(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_CELL_VIEW, BobguiCellView))
#define BOBGUI_IS_CELL_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_CELL_VIEW))

typedef struct _BobguiCellView             BobguiCellView;

GDK_AVAILABLE_IN_ALL
GType             bobgui_cell_view_get_type                (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10
BobguiWidget        *bobgui_cell_view_new                     (void);
GDK_DEPRECATED_IN_4_10
BobguiWidget        *bobgui_cell_view_new_with_context        (BobguiCellArea        *area,
                                                         BobguiCellAreaContext *context);
GDK_DEPRECATED_IN_4_10
BobguiWidget        *bobgui_cell_view_new_with_text           (const char      *text);
GDK_DEPRECATED_IN_4_10
BobguiWidget        *bobgui_cell_view_new_with_markup         (const char      *markup);
GDK_DEPRECATED_IN_4_10
BobguiWidget        *bobgui_cell_view_new_with_texture        (GdkTexture      *texture);
GDK_DEPRECATED_IN_4_10
void              bobgui_cell_view_set_model               (BobguiCellView     *cell_view,
                                                         BobguiTreeModel    *model);
GDK_DEPRECATED_IN_4_10
BobguiTreeModel     *bobgui_cell_view_get_model               (BobguiCellView     *cell_view);
GDK_DEPRECATED_IN_4_10
void              bobgui_cell_view_set_displayed_row       (BobguiCellView     *cell_view,
                                                         BobguiTreePath     *path);
GDK_DEPRECATED_IN_4_10
BobguiTreePath      *bobgui_cell_view_get_displayed_row       (BobguiCellView     *cell_view);
GDK_DEPRECATED_IN_4_10
gboolean          bobgui_cell_view_get_draw_sensitive      (BobguiCellView     *cell_view);
GDK_DEPRECATED_IN_4_10
void              bobgui_cell_view_set_draw_sensitive      (BobguiCellView     *cell_view,
                                                         gboolean         draw_sensitive);
GDK_DEPRECATED_IN_4_10
gboolean          bobgui_cell_view_get_fit_model           (BobguiCellView     *cell_view);
GDK_DEPRECATED_IN_4_10
void              bobgui_cell_view_set_fit_model           (BobguiCellView     *cell_view,
                                                         gboolean         fit_model);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiCellView, g_object_unref)

G_END_DECLS

