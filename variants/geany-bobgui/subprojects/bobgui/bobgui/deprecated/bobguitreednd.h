/* bobguitreednd.h
 * Copyright (C) 2001  Red Hat, Inc.
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

#include <bobgui/deprecated/bobguitreemodel.h>

G_BEGIN_DECLS

/**
 * BOBGUI_TYPE_TREE_ROW_DATA:
 * Magic `GType` to use when dragging rows in a `BobguiTreeModel`.
 *
 * Data in this format will be provided by bobgui_tree_create_row_drag_content()
 * and can be consumed via bobgui_tree_get_row_drag_data().
 */
#define BOBGUI_TYPE_TREE_ROW_DATA (bobgui_tree_row_data_get_type ())
GDK_DEPRECATED_IN_4_10
GType             bobgui_tree_row_data_get_type (void) G_GNUC_CONST;


#define BOBGUI_TYPE_TREE_DRAG_SOURCE            (bobgui_tree_drag_source_get_type ())
#define BOBGUI_TREE_DRAG_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TREE_DRAG_SOURCE, BobguiTreeDragSource))
#define BOBGUI_IS_TREE_DRAG_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TREE_DRAG_SOURCE))
#define BOBGUI_TREE_DRAG_SOURCE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BOBGUI_TYPE_TREE_DRAG_SOURCE, BobguiTreeDragSourceIface))

typedef struct _BobguiTreeDragSource      BobguiTreeDragSource; /* Dummy typedef */
typedef struct _BobguiTreeDragSourceIface BobguiTreeDragSourceIface;

/**
 * BobguiTreeDragSourceIface:
 * @row_draggable: Asks the `BobguiTreeDragSource` whether a particular
 *    row can be used as the source of a DND operation.
 * @drag_data_get: Asks the `BobguiTreeDragSource` to fill in
 *    selection_data with a representation of the row at path.
 * @drag_data_delete: Asks the `BobguiTreeDragSource` to delete the row at
 *    path, because it was moved somewhere else via drag-and-drop.
 */
struct _BobguiTreeDragSourceIface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/

  /* VTable - not signals */

  gboolean     (* row_draggable)        (BobguiTreeDragSource   *drag_source,
                                         BobguiTreePath         *path);

  GdkContentProvider * (* drag_data_get)(BobguiTreeDragSource   *drag_source,
                                         BobguiTreePath         *path);

  gboolean     (* drag_data_delete)     (BobguiTreeDragSource *drag_source,
                                         BobguiTreePath       *path);
};

GDK_DEPRECATED_IN_4_10_FOR(BobguiDragSource)
GType           bobgui_tree_drag_source_get_type   (void) G_GNUC_CONST;

/* Returns whether the given row can be dragged */
GDK_DEPRECATED_IN_4_10_FOR(BobguiDragSource)
gboolean bobgui_tree_drag_source_row_draggable    (BobguiTreeDragSource *drag_source,
                                                BobguiTreePath       *path);

/* Deletes the given row, or returns FALSE if it can't */
GDK_DEPRECATED_IN_4_10_FOR(BobguiDragSource)
gboolean bobgui_tree_drag_source_drag_data_delete (BobguiTreeDragSource *drag_source,
                                                BobguiTreePath       *path);

/* Fills in selection_data with type selection_data->target based on
 * the row denoted by path, returns TRUE if it does anything
 */
GDK_DEPRECATED_IN_4_10_FOR(BobguiDragSource)
GdkContentProvider *
         bobgui_tree_drag_source_drag_data_get    (BobguiTreeDragSource *drag_source,
                                                BobguiTreePath       *path);

#define BOBGUI_TYPE_TREE_DRAG_DEST            (bobgui_tree_drag_dest_get_type ())
#define BOBGUI_TREE_DRAG_DEST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TREE_DRAG_DEST, BobguiTreeDragDest))
#define BOBGUI_IS_TREE_DRAG_DEST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TREE_DRAG_DEST))
#define BOBGUI_TREE_DRAG_DEST_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BOBGUI_TYPE_TREE_DRAG_DEST, BobguiTreeDragDestIface))

typedef struct _BobguiTreeDragDest      BobguiTreeDragDest; /* Dummy typedef */
typedef struct _BobguiTreeDragDestIface BobguiTreeDragDestIface;

/**
 * BobguiTreeDragDestIface:
 * @drag_data_received: Asks the `BobguiTreeDragDest` to insert a row
 *    before the path dest, deriving the contents of the row from
 *    selection_data.
 * @row_drop_possible: Determines whether a drop is possible before
 *    the given dest_path, at the same depth as dest_path.
 */
struct _BobguiTreeDragDestIface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/

  /* VTable - not signals */

  gboolean     (* drag_data_received) (BobguiTreeDragDest   *drag_dest,
                                       BobguiTreePath       *dest,
                                       const GValue      *value);

  gboolean     (* row_drop_possible)  (BobguiTreeDragDest   *drag_dest,
                                       BobguiTreePath       *dest_path,
                                       const GValue      *value);
};

GDK_DEPRECATED_IN_4_10_FOR(BobguiDropTarget)
GType           bobgui_tree_drag_dest_get_type   (void) G_GNUC_CONST;

/* Inserts a row before dest which contains data in selection_data,
 * or returns FALSE if it can't
 */
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropTarget)
gboolean bobgui_tree_drag_dest_drag_data_received (BobguiTreeDragDest   *drag_dest,
						BobguiTreePath       *dest,
						const GValue      *value);


/* Returns TRUE if we can drop before path; path may not exist. */
GDK_DEPRECATED_IN_4_10_FOR(BobguiDropTarget)
gboolean bobgui_tree_drag_dest_row_drop_possible  (BobguiTreeDragDest   *drag_dest,
						BobguiTreePath       *dest_path,
						const GValue      *value);


/* The selection data would normally have target type BOBGUI_TREE_MODEL_ROW in this
 * case. If the target is wrong these functions return FALSE.
 */
GDK_DEPRECATED_IN_4_10_FOR(BobguiDragSource and BobguiDropTarget)
GdkContentProvider *
         bobgui_tree_create_row_drag_content      (BobguiTreeModel      *tree_model,
						BobguiTreePath       *path);
GDK_DEPRECATED_IN_4_10_FOR(BobguiDragSource and BobguiDropTarget)
gboolean bobgui_tree_get_row_drag_data            (const GValue      *value,
						BobguiTreeModel     **tree_model,
						BobguiTreePath      **path);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTreeDragDest, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTreeDragSource, g_object_unref)

G_END_DECLS

