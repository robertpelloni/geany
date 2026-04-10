/* bobguitreeprivate.h
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


#include <bobgui/deprecated/bobguitreeview.h>
#include <bobgui/deprecated/bobguitreeselection.h>
#include <bobgui/deprecated/bobguitreerbtreeprivate.h>

G_BEGIN_DECLS

#define TREE_VIEW_DRAG_WIDTH 6

typedef enum
{
  BOBGUI_TREE_SELECT_MODE_TOGGLE = 1 << 0,
  BOBGUI_TREE_SELECT_MODE_EXTEND = 1 << 1
}
BobguiTreeSelectMode;

/* functions that shouldn't be exported */
void         _bobgui_tree_selection_internal_select_node (BobguiTreeSelection  *selection,
						       BobguiTreeRBNode     *node,
						       BobguiTreeRBTree     *tree,
						       BobguiTreePath       *path,
                                                       BobguiTreeSelectMode  mode,
						       gboolean           override_browse_mode);
void         _bobgui_tree_selection_emit_changed         (BobguiTreeSelection  *selection);
gboolean     _bobgui_tree_view_find_node                 (BobguiTreeView       *tree_view,
						       BobguiTreePath       *path,
						       BobguiTreeRBTree    **tree,
						       BobguiTreeRBNode    **node);
gboolean     _bobgui_tree_view_get_cursor_node           (BobguiTreeView       *tree_view,
						       BobguiTreeRBTree    **tree,
						       BobguiTreeRBNode    **node);
BobguiTreePath *_bobgui_tree_path_new_from_rbtree           (BobguiTreeRBTree     *tree,
						       BobguiTreeRBNode     *node);

void         _bobgui_tree_view_add_editable              (BobguiTreeView       *tree_view,
                                                       BobguiTreeViewColumn *column,
                                                       BobguiTreePath       *path,
                                                       BobguiCellEditable   *cell_editable,
                                                       GdkRectangle      *cell_area);
void         _bobgui_tree_view_remove_editable           (BobguiTreeView       *tree_view,
                                                       BobguiTreeViewColumn *column,
                                                       BobguiCellEditable   *cell_editable);

void       _bobgui_tree_view_install_mark_rows_col_dirty (BobguiTreeView *tree_view,
						       gboolean     install_handler);
void         _bobgui_tree_view_column_autosize           (BobguiTreeView       *tree_view,
						       BobguiTreeViewColumn *column);

void         _bobgui_tree_view_get_row_separator_func    (BobguiTreeView                 *tree_view,
						       BobguiTreeViewRowSeparatorFunc *func,
						       gpointer                    *data);
BobguiTreePath *_bobgui_tree_view_get_anchor_path           (BobguiTreeView                 *tree_view);
void         _bobgui_tree_view_set_anchor_path           (BobguiTreeView                 *tree_view,
						       BobguiTreePath                 *anchor_path);
BobguiTreeRBTree *    _bobgui_tree_view_get_rbtree          (BobguiTreeView                 *tree_view);

BobguiTreeViewColumn *_bobgui_tree_view_get_focus_column    (BobguiTreeView                 *tree_view);
void               _bobgui_tree_view_set_focus_column    (BobguiTreeView                 *tree_view,
						       BobguiTreeViewColumn           *column);

BobguiTreeSelection* _bobgui_tree_selection_new                (void);
BobguiTreeSelection* _bobgui_tree_selection_new_with_tree_view (BobguiTreeView      *tree_view);
void              _bobgui_tree_selection_set_tree_view      (BobguiTreeSelection *selection,
                                                          BobguiTreeView      *tree_view);
gboolean          _bobgui_tree_selection_row_is_selectable  (BobguiTreeSelection *selection,
							  BobguiTreeRBNode    *node,
							  BobguiTreePath      *path);


void _bobgui_tree_view_column_realize_button   (BobguiTreeViewColumn *column);

void _bobgui_tree_view_column_set_tree_view    (BobguiTreeViewColumn *column,
					     BobguiTreeView       *tree_view);
int _bobgui_tree_view_column_request_width     (BobguiTreeViewColumn *tree_column);
void _bobgui_tree_view_column_allocate         (BobguiTreeViewColumn *tree_column,
					     int                x_offset,
					     int                width,
					     int                height);
void _bobgui_tree_view_column_unset_model      (BobguiTreeViewColumn *column,
					     BobguiTreeModel      *old_model);
void _bobgui_tree_view_column_unset_tree_view  (BobguiTreeViewColumn *column);
void _bobgui_tree_view_column_start_drag       (BobguiTreeView       *tree_view,
					     BobguiTreeViewColumn *column,
                                             GdkDevice         *device);
gboolean _bobgui_tree_view_column_cell_event   (BobguiTreeViewColumn  *tree_column,
					     GdkEvent           *event,
					     const GdkRectangle *cell_area,
					     guint               flags);
gboolean          _bobgui_tree_view_column_has_editable_cell(BobguiTreeViewColumn  *column);
BobguiCellRenderer  *_bobgui_tree_view_column_get_edited_cell  (BobguiTreeViewColumn  *column);
BobguiCellRenderer  *_bobgui_tree_view_column_get_cell_at_pos  (BobguiTreeViewColumn  *column,
                                                          GdkRectangle       *cell_area,
                                                          GdkRectangle       *background_area,
                                                          int                 x,
                                                          int                 y);
gboolean          _bobgui_tree_view_column_is_blank_at_pos  (BobguiTreeViewColumn  *column,
                                                          GdkRectangle       *cell_area,
                                                          GdkRectangle       *background_area,
                                                          int                 x,
                                                          int                 y);

void		  bobgui_tree_view_column_cell_snapshot     (BobguiTreeViewColumn  *tree_column,
							  BobguiSnapshot        *snapshot,
							  const GdkRectangle *background_area,
							  const GdkRectangle *cell_area,
							  guint               flags,
                                                          gboolean            draw_focus);
void		  _bobgui_tree_view_column_cell_set_dirty	 (BobguiTreeViewColumn  *tree_column,
							  gboolean            install_handler);
gboolean          _bobgui_tree_view_column_cell_get_dirty   (BobguiTreeViewColumn  *tree_column);

void              _bobgui_tree_view_column_push_padding          (BobguiTreeViewColumn  *column,
							       int                 padding);
int               _bobgui_tree_view_column_get_requested_width   (BobguiTreeViewColumn  *column);
int               _bobgui_tree_view_column_get_drag_x            (BobguiTreeViewColumn  *column);
BobguiCellAreaContext *_bobgui_tree_view_column_get_context         (BobguiTreeViewColumn  *column);
gboolean         _bobgui_tree_view_column_coords_in_resize_rect  (BobguiTreeViewColumn *column,
                                                               double             x,
                                                               double             y);


G_END_DECLS



