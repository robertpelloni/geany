/* bobguitreeviewcolumn.h
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
#include <bobgui/deprecated/bobguitreemodel.h>
#include <bobgui/deprecated/bobguitreesortable.h>
#include <bobgui/deprecated/bobguicellarea.h>


G_BEGIN_DECLS


#define BOBGUI_TYPE_TREE_VIEW_COLUMN	     (bobgui_tree_view_column_get_type ())
#define BOBGUI_TREE_VIEW_COLUMN(obj)	     (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TREE_VIEW_COLUMN, BobguiTreeViewColumn))
#define BOBGUI_IS_TREE_VIEW_COLUMN(obj)	     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TREE_VIEW_COLUMN))

typedef struct _BobguiTreeViewColumn        BobguiTreeViewColumn;

/**
 * BobguiTreeViewColumnSizing:
 * @BOBGUI_TREE_VIEW_COLUMN_GROW_ONLY: Columns only get bigger in reaction to changes in the model
 * @BOBGUI_TREE_VIEW_COLUMN_AUTOSIZE: Columns resize to be the optimal size every time the model changes.
 * @BOBGUI_TREE_VIEW_COLUMN_FIXED: Columns are a fixed numbers of pixels wide.
 *
 * The sizing method the column uses to determine its width.  Please note
 * that %BOBGUI_TREE_VIEW_COLUMN_AUTOSIZE are inefficient for large views, and
 * can make columns appear choppy.
 *
 * Deprecated: 4.20: There is no replacement.
 */
typedef enum
{
  BOBGUI_TREE_VIEW_COLUMN_GROW_ONLY,
  BOBGUI_TREE_VIEW_COLUMN_AUTOSIZE,
  BOBGUI_TREE_VIEW_COLUMN_FIXED
} BobguiTreeViewColumnSizing;

/**
 * BobguiTreeCellDataFunc:
 * @tree_column: A `BobguiTreeViewColumn`
 * @cell: The `BobguiCellRenderer` that is being rendered by @tree_column
 * @tree_model: The `BobguiTreeModel` being rendered
 * @iter: A `BobguiTreeIter` of the current row rendered
 * @data: (closure): user data
 *
 * A function to set the properties of a cell instead of just using the
 * straight mapping between the cell and the model.
 *
 * This function is useful for customizing the cell renderer. For example,
 * a function might get an* integer from the @tree_model, and render it to
 * the “text” attribute of “cell” by converting it to its written equivalent.
 *
 * See also: bobgui_tree_view_column_set_cell_data_func()
 *
 * Deprecated: 4.20: There is no replacement
 */
typedef void (* BobguiTreeCellDataFunc) (BobguiTreeViewColumn *tree_column,
				      BobguiCellRenderer   *cell,
				      BobguiTreeModel      *tree_model,
				      BobguiTreeIter       *iter,
				      gpointer           data);


GDK_AVAILABLE_IN_ALL
GType                   bobgui_tree_view_column_get_type            (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
BobguiTreeViewColumn      *bobgui_tree_view_column_new                 (void);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
BobguiTreeViewColumn      *bobgui_tree_view_column_new_with_area       (BobguiCellArea             *area);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
BobguiTreeViewColumn      *bobgui_tree_view_column_new_with_attributes (const char              *title,
								  BobguiCellRenderer         *cell,
								  ...) G_GNUC_NULL_TERMINATED;
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_pack_start          (BobguiTreeViewColumn       *tree_column,
								  BobguiCellRenderer         *cell,
								  gboolean                 expand);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_pack_end            (BobguiTreeViewColumn       *tree_column,
								  BobguiCellRenderer         *cell,
								  gboolean                 expand);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_clear               (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_add_attribute       (BobguiTreeViewColumn       *tree_column,
								  BobguiCellRenderer         *cell_renderer,
								  const char              *attribute,
								  int                      column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_set_attributes      (BobguiTreeViewColumn       *tree_column,
								  BobguiCellRenderer         *cell_renderer,
								  ...) G_GNUC_NULL_TERMINATED;
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_set_cell_data_func  (BobguiTreeViewColumn       *tree_column,
								  BobguiCellRenderer         *cell_renderer,
								  BobguiTreeCellDataFunc      func,
								  gpointer                 func_data,
								  GDestroyNotify           destroy);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_clear_attributes    (BobguiTreeViewColumn       *tree_column,
								  BobguiCellRenderer         *cell_renderer);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_set_spacing         (BobguiTreeViewColumn       *tree_column,
								  int                      spacing);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
int                     bobgui_tree_view_column_get_spacing         (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_set_visible         (BobguiTreeViewColumn       *tree_column,
								  gboolean                 visible);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
gboolean                bobgui_tree_view_column_get_visible         (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_set_resizable       (BobguiTreeViewColumn       *tree_column,
								  gboolean                 resizable);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
gboolean                bobgui_tree_view_column_get_resizable       (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_set_sizing          (BobguiTreeViewColumn       *tree_column,
								  BobguiTreeViewColumnSizing  type);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
BobguiTreeViewColumnSizing bobgui_tree_view_column_get_sizing          (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
int                     bobgui_tree_view_column_get_x_offset        (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
int                     bobgui_tree_view_column_get_width           (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
int                     bobgui_tree_view_column_get_fixed_width     (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_set_fixed_width     (BobguiTreeViewColumn       *tree_column,
								  int                      fixed_width);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_set_min_width       (BobguiTreeViewColumn       *tree_column,
								  int                      min_width);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
int                     bobgui_tree_view_column_get_min_width       (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_set_max_width       (BobguiTreeViewColumn       *tree_column,
								  int                      max_width);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
int                     bobgui_tree_view_column_get_max_width       (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_clicked             (BobguiTreeViewColumn       *tree_column);



/* Options for manipulating the column headers
 */
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_set_title           (BobguiTreeViewColumn       *tree_column,
								  const char              *title);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
const char *           bobgui_tree_view_column_get_title           (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_set_expand          (BobguiTreeViewColumn       *tree_column,
								  gboolean                 expand);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
gboolean                bobgui_tree_view_column_get_expand          (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_set_clickable       (BobguiTreeViewColumn       *tree_column,
								  gboolean                 clickable);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
gboolean                bobgui_tree_view_column_get_clickable       (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_set_widget          (BobguiTreeViewColumn       *tree_column,
								  BobguiWidget               *widget);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
BobguiWidget              *bobgui_tree_view_column_get_widget          (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_set_alignment       (BobguiTreeViewColumn       *tree_column,
								  float                    xalign);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
float                   bobgui_tree_view_column_get_alignment       (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_set_reorderable     (BobguiTreeViewColumn       *tree_column,
								  gboolean                 reorderable);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
gboolean                bobgui_tree_view_column_get_reorderable     (BobguiTreeViewColumn       *tree_column);



/* You probably only want to use bobgui_tree_view_column_set_sort_column_id.  The
 * other sorting functions exist primarily to let others do their own custom sorting.
 */
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_set_sort_column_id  (BobguiTreeViewColumn       *tree_column,
								  int                      sort_column_id);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
int                     bobgui_tree_view_column_get_sort_column_id  (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_set_sort_indicator  (BobguiTreeViewColumn       *tree_column,
								  gboolean                 setting);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
gboolean                bobgui_tree_view_column_get_sort_indicator  (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_set_sort_order      (BobguiTreeViewColumn       *tree_column,
								  BobguiSortType              order);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
BobguiSortType             bobgui_tree_view_column_get_sort_order      (BobguiTreeViewColumn       *tree_column);


/* These functions are meant primarily for interaction between the BobguiTreeView and the column.
 */
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_cell_set_cell_data  (BobguiTreeViewColumn       *tree_column,
								  BobguiTreeModel            *tree_model,
								  BobguiTreeIter             *iter,
								  gboolean                 is_expander,
								  gboolean                 is_expanded);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_cell_get_size       (BobguiTreeViewColumn       *tree_column,
                                                                  int                     *x_offset,
                                                                  int                     *y_offset,
                                                                  int                     *width,
                                                                  int                     *height);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
gboolean                bobgui_tree_view_column_cell_is_visible     (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_focus_cell          (BobguiTreeViewColumn       *tree_column,
								  BobguiCellRenderer         *cell);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
gboolean                bobgui_tree_view_column_cell_get_position   (BobguiTreeViewColumn       *tree_column,
					                          BobguiCellRenderer         *cell_renderer,
					                          int                     *x_offset,
					                          int                     *width);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
void                    bobgui_tree_view_column_queue_resize        (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
BobguiWidget              *bobgui_tree_view_column_get_tree_view       (BobguiTreeViewColumn       *tree_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiColumnView and BobguiColumnViewColumn)
BobguiWidget              *bobgui_tree_view_column_get_button          (BobguiTreeViewColumn       *tree_column);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTreeViewColumn, g_object_unref)

G_END_DECLS
