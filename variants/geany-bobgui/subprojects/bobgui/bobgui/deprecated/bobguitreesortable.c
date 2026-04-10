/* bobguitreesortable.c
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


#include "config.h"
#include "bobguitreesortable.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"


G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiTreeSortable:
 *
 * The interface for sortable models used by BobguiTreeView
 *
 * `BobguiTreeSortable` is an interface to be implemented by tree models which
 * support sorting. The `BobguiTreeView` uses the methods provided by this interface
 * to sort the model.
 *
 * Deprecated: 4.10: There is no replacement for this interface. You should
 *   use [class@Bobgui.SortListModel] to wrap your list model instead
 */


static void bobgui_tree_sortable_base_init (gpointer g_class);

GType
bobgui_tree_sortable_get_type (void)
{
  static GType tree_sortable_type = 0;

  if (! tree_sortable_type)
    {
      const GTypeInfo tree_sortable_info =
      {
	sizeof (BobguiTreeSortableIface), /* class_size */
	bobgui_tree_sortable_base_init,   /* base_init */
	NULL,		/* base_finalize */
	NULL,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	0,
	0,
	NULL
      };

      tree_sortable_type =
	g_type_register_static (G_TYPE_INTERFACE, I_("BobguiTreeSortable"),
				&tree_sortable_info, 0);

      g_type_interface_add_prerequisite (tree_sortable_type, BOBGUI_TYPE_TREE_MODEL);
    }

  return tree_sortable_type;
}

static void
bobgui_tree_sortable_base_init (gpointer g_class)
{
  static gboolean initialized = FALSE;

  if (! initialized)
    {
      /**
       * BobguiTreeSortable::sort-column-changed:
       * @sortable: the object on which the signal is emitted
       *
       * The ::sort-column-changed signal is emitted when the sort column
       * or sort order of @sortable is changed. The signal is emitted before
       * the contents of @sortable are resorted.
       */
      g_signal_new (I_("sort-column-changed"),
                    BOBGUI_TYPE_TREE_SORTABLE,
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (BobguiTreeSortableIface, sort_column_changed),
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE, 0);
      initialized = TRUE;
    }
}

/**
 * bobgui_tree_sortable_sort_column_changed:
 * @sortable: A `BobguiTreeSortable`
 *
 * Emits a `BobguiTreeSortable::sort-column-changed` signal on @sortable.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_sortable_sort_column_changed (BobguiTreeSortable *sortable)
{
  g_return_if_fail (BOBGUI_IS_TREE_SORTABLE (sortable));

  g_signal_emit_by_name (sortable, "sort-column-changed");
}

/**
 * bobgui_tree_sortable_get_sort_column_id:
 * @sortable: A `BobguiTreeSortable`
 * @sort_column_id: (out): The sort column id to be filled in
 * @order: (out): The `BobguiSortType` to be filled in
 *
 * Fills in @sort_column_id and @order with the current sort column and the
 * order. It returns %TRUE unless the @sort_column_id is
 * %BOBGUI_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID or
 * %BOBGUI_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID.
 *
 * Returns: %TRUE if the sort column is not one of the special sort
 *   column ids.
 *
 * Deprecated: 4.10
 **/
gboolean
bobgui_tree_sortable_get_sort_column_id (BobguiTreeSortable  *sortable,
				      int              *sort_column_id,
				      BobguiSortType      *order)
{
  BobguiTreeSortableIface *iface;

  g_return_val_if_fail (BOBGUI_IS_TREE_SORTABLE (sortable), FALSE);

  iface = BOBGUI_TREE_SORTABLE_GET_IFACE (sortable);

  g_return_val_if_fail (iface != NULL, FALSE);
  g_return_val_if_fail (iface->get_sort_column_id != NULL, FALSE);

  return (* iface->get_sort_column_id) (sortable, sort_column_id, order);
}

/**
 * bobgui_tree_sortable_set_sort_column_id:
 * @sortable: A `BobguiTreeSortable`
 * @sort_column_id: the sort column id to set
 * @order: The sort order of the column
 *
 * Sets the current sort column to be @sort_column_id. The @sortable will
 * resort itself to reflect this change, after emitting a
 * `BobguiTreeSortable::sort-column-changed` signal. @sort_column_id may either be
 * a regular column id, or one of the following special values:
 *
 * - %BOBGUI_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID: the default sort function
 *   will be used, if it is set
 *
 * - %BOBGUI_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID: no sorting will occur
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_sortable_set_sort_column_id (BobguiTreeSortable  *sortable,
				      int               sort_column_id,
				      BobguiSortType       order)
{
  BobguiTreeSortableIface *iface;

  g_return_if_fail (BOBGUI_IS_TREE_SORTABLE (sortable));

  iface = BOBGUI_TREE_SORTABLE_GET_IFACE (sortable);

  g_return_if_fail (iface != NULL);
  g_return_if_fail (iface->set_sort_column_id != NULL);

  (* iface->set_sort_column_id) (sortable, sort_column_id, order);
}

/**
 * bobgui_tree_sortable_set_sort_func:
 * @sortable: A `BobguiTreeSortable`
 * @sort_column_id: the sort column id to set the function for
 * @sort_func: (scope notified) (closure user_data) (destroy destroy): The comparison function
 * @user_data: User data to pass to @sort_func
 * @destroy: (nullable): Destroy notifier of @user_data
 *
 * Sets the comparison function used when sorting to be @sort_func. If the
 * current sort column id of @sortable is the same as @sort_column_id, then
 * the model will sort using this function.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_sortable_set_sort_func (BobguiTreeSortable        *sortable,
				 int                     sort_column_id,
				 BobguiTreeIterCompareFunc  sort_func,
				 gpointer                user_data,
				 GDestroyNotify          destroy)
{
  BobguiTreeSortableIface *iface;

  g_return_if_fail (BOBGUI_IS_TREE_SORTABLE (sortable));
  g_return_if_fail (sort_func != NULL);

  iface = BOBGUI_TREE_SORTABLE_GET_IFACE (sortable);

  g_return_if_fail (iface != NULL);
  g_return_if_fail (iface->set_sort_func != NULL);
  g_return_if_fail (sort_column_id >= 0);

  (* iface->set_sort_func) (sortable, sort_column_id, sort_func, user_data, destroy);
}

/**
 * bobgui_tree_sortable_set_default_sort_func:
 * @sortable: A `BobguiTreeSortable`
 * @sort_func: (scope notified) (closure user_data) (destroy destroy): The comparison function
 * @user_data: User data to pass to @sort_func
 * @destroy: (nullable): Destroy notifier of @user_data
 *
 * Sets the default comparison function used when sorting to be @sort_func.
 * If the current sort column id of @sortable is
 * %BOBGUI_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, then the model will sort using
 * this function.
 *
 * If @sort_func is %NULL, then there will be no default comparison function.
 * This means that once the model  has been sorted, it can’t go back to the
 * default state. In this case, when the current sort column id of @sortable
 * is %BOBGUI_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, the model will be unsorted.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_sortable_set_default_sort_func (BobguiTreeSortable        *sortable,
					 BobguiTreeIterCompareFunc  sort_func,
					 gpointer                user_data,
					 GDestroyNotify          destroy)
{
  BobguiTreeSortableIface *iface;

  g_return_if_fail (BOBGUI_IS_TREE_SORTABLE (sortable));

  iface = BOBGUI_TREE_SORTABLE_GET_IFACE (sortable);

  g_return_if_fail (iface != NULL);
  g_return_if_fail (iface->set_default_sort_func != NULL);

  (* iface->set_default_sort_func) (sortable, sort_func, user_data, destroy);
}

/**
 * bobgui_tree_sortable_has_default_sort_func:
 * @sortable: A `BobguiTreeSortable`
 *
 * Returns %TRUE if the model has a default sort function. This is used
 * primarily by BobguiTreeViewColumns in order to determine if a model can
 * go back to the default state, or not.
 *
 * Returns: %TRUE, if the model has a default sort function
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_tree_sortable_has_default_sort_func (BobguiTreeSortable *sortable)
{
  BobguiTreeSortableIface *iface;

  g_return_val_if_fail (BOBGUI_IS_TREE_SORTABLE (sortable), FALSE);

  iface = BOBGUI_TREE_SORTABLE_GET_IFACE (sortable);

  g_return_val_if_fail (iface != NULL, FALSE);
  g_return_val_if_fail (iface->has_default_sort_func != NULL, FALSE);

  return (* iface->has_default_sort_func) (sortable);
}
