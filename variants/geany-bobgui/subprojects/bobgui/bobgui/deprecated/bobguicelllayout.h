/* bobguicelllayout.h
 * Copyright (C) 2003  Kristian Rietveld  <kris@bobgui.org>
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
#include <bobgui/deprecated/bobguicellarea.h>
#include <bobgui/bobguibuildable.h>
#include <bobgui/bobguibuilder.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_CELL_LAYOUT            (bobgui_cell_layout_get_type ())
#define BOBGUI_CELL_LAYOUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_CELL_LAYOUT, BobguiCellLayout))
#define BOBGUI_IS_CELL_LAYOUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_CELL_LAYOUT))
#define BOBGUI_CELL_LAYOUT_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BOBGUI_TYPE_CELL_LAYOUT, BobguiCellLayoutIface))

typedef struct _BobguiCellLayout           BobguiCellLayout; /* dummy typedef */
typedef struct _BobguiCellLayoutIface      BobguiCellLayoutIface;

/* keep in sync with BobguiTreeCellDataFunc */
/**
 * BobguiCellLayoutDataFunc:
 * @cell_layout: a `BobguiCellLayout`
 * @cell: the cell renderer whose value is to be set
 * @tree_model: the model
 * @iter: a `BobguiTreeIter` indicating the row to set the value for
 * @data: (closure): user data passed to bobgui_cell_layout_set_cell_data_func()
 *
 * A function which should set the value of @cell_layout’s cell renderer(s)
 * as appropriate.
 *
 * Deprecated: 4.20: There is no replacement
 */
typedef void (* BobguiCellLayoutDataFunc) (BobguiCellLayout   *cell_layout,
                                        BobguiCellRenderer *cell,
                                        BobguiTreeModel    *tree_model,
                                        BobguiTreeIter     *iter,
                                        gpointer         data);

/**
 * BobguiCellLayoutIface:
 * @pack_start: Packs the cell into the beginning of cell_layout.
 * @pack_end: Adds the cell to the end of cell_layout.
 * @clear: Unsets all the mappings on all renderers on cell_layout and
 *    removes all renderers from cell_layout.
 * @add_attribute: Adds an attribute mapping to the list in
 *    cell_layout.
 * @set_cell_data_func: Sets the `BobguiCellLayout`DataFunc to use for
 *    cell_layout.
 * @clear_attributes: Clears all existing attributes previously set
 *    with bobgui_cell_layout_set_attributes().
 * @reorder: Re-inserts cell at position.
 * @get_cells: Get the cell renderers which have been added to
 *    cell_layout.
 * @get_area: Get the underlying `BobguiCellArea` which might be
 *    cell_layout if called on a `BobguiCellArea` or might be NULL if no
 *    `BobguiCellArea` is used by cell_layout.
 */
struct _BobguiCellLayoutIface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/

  /* Virtual Table */
  void (* pack_start)         (BobguiCellLayout         *cell_layout,
                               BobguiCellRenderer       *cell,
                               gboolean               expand);
  void (* pack_end)           (BobguiCellLayout         *cell_layout,
                               BobguiCellRenderer       *cell,
                               gboolean               expand);
  void (* clear)              (BobguiCellLayout         *cell_layout);
  void (* add_attribute)      (BobguiCellLayout         *cell_layout,
                               BobguiCellRenderer       *cell,
                               const char            *attribute,
                               int                    column);
  void (* set_cell_data_func) (BobguiCellLayout         *cell_layout,
                               BobguiCellRenderer       *cell,
                               BobguiCellLayoutDataFunc  func,
                               gpointer               func_data,
                               GDestroyNotify         destroy);
  void (* clear_attributes)   (BobguiCellLayout         *cell_layout,
                               BobguiCellRenderer       *cell);
  void (* reorder)            (BobguiCellLayout         *cell_layout,
                               BobguiCellRenderer       *cell,
                               int                    position);
  GList* (* get_cells)        (BobguiCellLayout         *cell_layout);

  BobguiCellArea *(* get_area)   (BobguiCellLayout         *cell_layout);
};

GDK_AVAILABLE_IN_ALL
GType bobgui_cell_layout_get_type           (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10
void  bobgui_cell_layout_pack_start         (BobguiCellLayout         *cell_layout,
                                          BobguiCellRenderer       *cell,
                                          gboolean               expand);
GDK_DEPRECATED_IN_4_10
void  bobgui_cell_layout_pack_end           (BobguiCellLayout         *cell_layout,
                                          BobguiCellRenderer       *cell,
                                          gboolean               expand);
GDK_DEPRECATED_IN_4_10
GList *bobgui_cell_layout_get_cells         (BobguiCellLayout         *cell_layout);
GDK_DEPRECATED_IN_4_10
void  bobgui_cell_layout_clear              (BobguiCellLayout         *cell_layout);
GDK_DEPRECATED_IN_4_10
void  bobgui_cell_layout_set_attributes     (BobguiCellLayout         *cell_layout,
                                          BobguiCellRenderer       *cell,
                                          ...) G_GNUC_NULL_TERMINATED;
GDK_DEPRECATED_IN_4_10
void  bobgui_cell_layout_add_attribute      (BobguiCellLayout         *cell_layout,
                                          BobguiCellRenderer       *cell,
                                          const char            *attribute,
                                          int                    column);
GDK_DEPRECATED_IN_4_10
void  bobgui_cell_layout_set_cell_data_func (BobguiCellLayout         *cell_layout,
                                          BobguiCellRenderer       *cell,
                                          BobguiCellLayoutDataFunc  func,
                                          gpointer               func_data,
                                          GDestroyNotify         destroy);
GDK_DEPRECATED_IN_4_10
void  bobgui_cell_layout_clear_attributes   (BobguiCellLayout         *cell_layout,
                                          BobguiCellRenderer       *cell);
GDK_DEPRECATED_IN_4_10
void  bobgui_cell_layout_reorder            (BobguiCellLayout         *cell_layout,
                                          BobguiCellRenderer       *cell,
                                          int                    position);
GDK_DEPRECATED_IN_4_10
BobguiCellArea *bobgui_cell_layout_get_area    (BobguiCellLayout         *cell_layout);

gboolean _bobgui_cell_layout_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                                      BobguiBuilder         *builder,
                                                      GObject            *child,
                                                      const char         *tagname,
                                                      BobguiBuildableParser *parser,
                                                      gpointer           *data);
gboolean _bobgui_cell_layout_buildable_custom_tag_end   (BobguiBuildable       *buildable,
                                                      BobguiBuilder         *builder,
                                                      GObject            *child,
                                                      const char         *tagname,
                                                      gpointer           *data);
void     _bobgui_cell_layout_buildable_add_child        (BobguiBuildable       *buildable,
                                                      BobguiBuilder         *builder,
                                                      GObject            *child,
                                                      const char         *type);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiCellLayout, g_object_unref)

G_END_DECLS

