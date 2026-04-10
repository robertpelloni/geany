/* bobguitreesortable.h
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

#include <bobgui/bobguienums.h>
#include <bobgui/deprecated/bobguitreemodel.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_TREE_SORTABLE            (bobgui_tree_sortable_get_type ())
#define BOBGUI_TREE_SORTABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TREE_SORTABLE, BobguiTreeSortable))
#define BOBGUI_IS_TREE_SORTABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TREE_SORTABLE))
#define BOBGUI_TREE_SORTABLE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BOBGUI_TYPE_TREE_SORTABLE, BobguiTreeSortableIface))

/**
 * BOBGUI_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID:
 *
 * Uses the default sort function in a [iface@Bobgui.TreeSortable].
 *
 * See also: [method@Bobgui.TreeSortable.set_sort_column_id]
 *
 * Deprecated: 4.20: There is no replacement
 */
#define BOBGUI_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID (-1)

/**
 * BOBGUI_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID:
 *
 * Disables sorting in a [iface@Bobgui.TreeSortable].
 *
 * See also: [method@Bobgui.TreeSortable.set_sort_column_id]
 *
 * Deprecated: 4.20: There is no replacement
 */
#define BOBGUI_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID (-2)

typedef struct _BobguiTreeSortable      BobguiTreeSortable; /* Dummy typedef */
typedef struct _BobguiTreeSortableIface BobguiTreeSortableIface;

/**
 * BobguiTreeIterCompareFunc:
 * @model: The `BobguiTreeModel` the comparison is within
 * @a: A `BobguiTreeIter` in @model
 * @b: Another `BobguiTreeIter` in @model
 * @user_data: Data passed when the compare func is assigned e.g. by
 *  bobgui_tree_sortable_set_sort_func()
 *
 * A BobguiTreeIterCompareFunc should return a negative integer, zero, or a positive
 * integer if @a sorts before @b, @a sorts with @b, or @a sorts after @b
 * respectively.
 *
 * If two iters compare as equal, their order in the sorted model
 * is undefined. In order to ensure that the `BobguiTreeSortable` behaves as
 * expected, the BobguiTreeIterCompareFunc must define a partial order on
 * the model, i.e. it must be reflexive, antisymmetric and transitive.
 *
 * For example, if @model is a product catalogue, then a compare function
 * for the “price” column could be one which returns
 * `price_of(@a) - price_of(@b)`.
 *
 * Returns: a negative integer, zero or a positive integer depending on whether
 *   @a sorts before, with or after @b
 *
 * Deprecated: 4.20: There is no replacement
 */
typedef int (* BobguiTreeIterCompareFunc) (BobguiTreeModel *model,
					BobguiTreeIter  *a,
					BobguiTreeIter  *b,
					gpointer      user_data);


/**
 * BobguiTreeSortableIface:
 * @sort_column_changed: Signal emitted when the sort column or sort
 *    order of sortable is changed.
 * @get_sort_column_id: Fills in sort_column_id and order with the
 *    current sort column and the order.
 * @set_sort_column_id: Sets the current sort column to be
 *    sort_column_id.
 * @set_sort_func: Sets the comparison function used when sorting to
 *    be sort_func.
 * @set_default_sort_func: Sets the default comparison function used
 *    when sorting to be sort_func.
 * @has_default_sort_func: %TRUE if the model has a default sort
 * function.
 */
struct _BobguiTreeSortableIface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/

  /* signals */
  void     (* sort_column_changed)   (BobguiTreeSortable        *sortable);

  /* virtual table */
  gboolean (* get_sort_column_id)    (BobguiTreeSortable        *sortable,
				      int                    *sort_column_id,
				      BobguiSortType            *order);
  void     (* set_sort_column_id)    (BobguiTreeSortable        *sortable,
				      int                     sort_column_id,
				      BobguiSortType             order);
  void     (* set_sort_func)         (BobguiTreeSortable        *sortable,
				      int                     sort_column_id,
				      BobguiTreeIterCompareFunc  sort_func,
				      gpointer                user_data,
				      GDestroyNotify          destroy);
  void     (* set_default_sort_func) (BobguiTreeSortable        *sortable,
				      BobguiTreeIterCompareFunc  sort_func,
				      gpointer                user_data,
				      GDestroyNotify          destroy);
  gboolean (* has_default_sort_func) (BobguiTreeSortable        *sortable);
};


GDK_AVAILABLE_IN_ALL
GType    bobgui_tree_sortable_get_type              (void) G_GNUC_CONST;

GDK_DEPRECATED_IN_4_10
void     bobgui_tree_sortable_sort_column_changed   (BobguiTreeSortable        *sortable);
GDK_DEPRECATED_IN_4_10
gboolean bobgui_tree_sortable_get_sort_column_id    (BobguiTreeSortable        *sortable,
						  int                    *sort_column_id,
						  BobguiSortType            *order);
GDK_DEPRECATED_IN_4_10
void     bobgui_tree_sortable_set_sort_column_id    (BobguiTreeSortable        *sortable,
						  int                     sort_column_id,
						  BobguiSortType             order);
GDK_DEPRECATED_IN_4_10
void     bobgui_tree_sortable_set_sort_func         (BobguiTreeSortable        *sortable,
						  int                     sort_column_id,
						  BobguiTreeIterCompareFunc  sort_func,
						  gpointer                user_data,
						  GDestroyNotify          destroy);
GDK_DEPRECATED_IN_4_10
void     bobgui_tree_sortable_set_default_sort_func (BobguiTreeSortable        *sortable,
						  BobguiTreeIterCompareFunc  sort_func,
						  gpointer                user_data,
						  GDestroyNotify          destroy);
GDK_DEPRECATED_IN_4_10
gboolean bobgui_tree_sortable_has_default_sort_func (BobguiTreeSortable        *sortable);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTreeSortable, g_object_unref)

G_END_DECLS

