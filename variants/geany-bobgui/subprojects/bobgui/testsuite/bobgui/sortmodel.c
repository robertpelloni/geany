/* Extensive BobguiTreeModelSort tests.
 * Copyright (C) 2009,2011  Kristian Rietveld  <kris@bobgui.org>
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

#include <bobgui/bobgui.h>

#include "treemodel.h"
#include "bobguitreemodelrefcount.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
ref_count_single_level (void)
{
  BobguiTreeIter iter;
  BobguiTreeModel *model;
  BobguiTreeModelRefCount *ref_model;
  BobguiTreeModel *sort_model;
  BobguiWidget *tree_view;

  model = bobgui_tree_model_ref_count_new ();
  ref_model = BOBGUI_TREE_MODEL_REF_COUNT (model);

  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter, NULL);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter, NULL);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter, NULL);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter, NULL);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter, NULL);

  assert_root_level_unreferenced (ref_model);

  sort_model = bobgui_tree_model_sort_new_with_model (model);
  tree_view = bobgui_tree_view_new_with_model (sort_model);

  assert_entire_model_referenced (ref_model, 1);

  g_object_unref (g_object_ref_sink (tree_view));

  assert_entire_model_unreferenced (ref_model);

  g_object_unref (sort_model);
  g_object_unref (ref_model);
}

static void
ref_count_two_levels (void)
{
  BobguiTreeIter parent1, parent2, iter;
  BobguiTreeModel *model;
  BobguiTreeModelRefCount *ref_model;
  BobguiTreeModel *sort_model;
  BobguiWidget *tree_view;

  model = bobgui_tree_model_ref_count_new ();
  ref_model = BOBGUI_TREE_MODEL_REF_COUNT (model);

  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &parent1, NULL);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &parent2, NULL);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter, &parent2);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter, &parent2);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter, &parent2);

  assert_entire_model_unreferenced (ref_model);

  sort_model = bobgui_tree_model_sort_new_with_model (model);
  tree_view = bobgui_tree_view_new_with_model (sort_model);

  assert_root_level_referenced (ref_model, 1);
  assert_node_ref_count (ref_model, &iter, 0);

  bobgui_tree_view_expand_all (BOBGUI_TREE_VIEW (tree_view));

  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter, 1);

  bobgui_tree_view_collapse_all (BOBGUI_TREE_VIEW (tree_view));

  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter, 0);

  bobgui_tree_model_sort_clear_cache (BOBGUI_TREE_MODEL_SORT (sort_model));

  assert_root_level_referenced (ref_model, 1);
  assert_node_ref_count (ref_model, &iter, 0);

  g_object_unref (g_object_ref_sink (tree_view));

  assert_entire_model_unreferenced (ref_model);

  g_object_unref (sort_model);
  g_object_unref (ref_model);
}

static void
ref_count_three_levels (void)
{
  BobguiTreeIter grandparent1, grandparent2, parent1, parent2;
  BobguiTreeIter iter_parent1, iter_parent2;
  BobguiTreeModel *model;
  BobguiTreeModelRefCount *ref_model;
  BobguiTreeModel *sort_model;
  BobguiTreePath *path;
  BobguiWidget *tree_view;

  model = bobgui_tree_model_ref_count_new ();
  ref_model = BOBGUI_TREE_MODEL_REF_COUNT (model);

  /* + grandparent1
   * + grandparent2
   *   + parent1
   *     + iter_parent1
   *   + parent2
   *     + iter_parent2
   *     + iter_parent2
   */

  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &grandparent1, NULL);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &grandparent2, NULL);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &parent1, &grandparent2);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter_parent1, &parent1);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &parent2, &grandparent2);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter_parent2, &parent2);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter_parent2, &parent2);

  assert_entire_model_unreferenced (ref_model);

  sort_model = bobgui_tree_model_sort_new_with_model (model);
  tree_view = bobgui_tree_view_new_with_model (sort_model);

  assert_root_level_referenced (ref_model, 1);
  assert_node_ref_count (ref_model, &parent1, 0);
  assert_node_ref_count (ref_model, &parent2, 0);
  assert_level_unreferenced (ref_model, &parent1);
  assert_level_unreferenced (ref_model, &parent2);

  path = bobgui_tree_path_new_from_indices (1, -1);
  bobgui_tree_view_expand_row (BOBGUI_TREE_VIEW (tree_view), path, FALSE);

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 1);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  bobgui_tree_view_expand_row (BOBGUI_TREE_VIEW (tree_view), path, TRUE);

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 2);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_parent1, 1);
  assert_node_ref_count (ref_model, &iter_parent2, 1);

  bobgui_tree_view_collapse_all (BOBGUI_TREE_VIEW (tree_view));

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 1);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  bobgui_tree_model_sort_clear_cache (BOBGUI_TREE_MODEL_SORT (sort_model));

  assert_root_level_referenced (ref_model, 1);
  assert_node_ref_count (ref_model, &parent1, 0);
  assert_node_ref_count (ref_model, &parent2, 0);

  bobgui_tree_view_expand_row (BOBGUI_TREE_VIEW (tree_view), path, FALSE);

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 1);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  bobgui_tree_path_append_index (path, 1);
  bobgui_tree_view_expand_row (BOBGUI_TREE_VIEW (tree_view), path, FALSE);

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2, 1);

  bobgui_tree_view_collapse_row (BOBGUI_TREE_VIEW (tree_view), path);

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  bobgui_tree_model_sort_clear_cache (BOBGUI_TREE_MODEL_SORT (sort_model));

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 1);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  bobgui_tree_path_up (path);
  bobgui_tree_view_collapse_row (BOBGUI_TREE_VIEW (tree_view), path);
  bobgui_tree_path_free (path);

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 0);
  assert_node_ref_count (ref_model, &parent2, 0);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  bobgui_tree_model_sort_clear_cache (BOBGUI_TREE_MODEL_SORT (sort_model));

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 1);
  assert_node_ref_count (ref_model, &parent1, 0);
  assert_node_ref_count (ref_model, &parent2, 0);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  g_object_unref (g_object_ref_sink (tree_view));

  assert_entire_model_unreferenced (ref_model);

  g_object_unref (sort_model);
  g_object_unref (ref_model);
}

static void
ref_count_delete_row (void)
{
  BobguiTreeIter grandparent1, grandparent2, parent1, parent2;
  BobguiTreeIter iter_parent1, iter_parent2;
  BobguiTreeModel *model;
  BobguiTreeModelRefCount *ref_model;
  BobguiTreeModel *sort_model;
  BobguiTreePath *path;
  BobguiWidget *tree_view;

  model = bobgui_tree_model_ref_count_new ();
  ref_model = BOBGUI_TREE_MODEL_REF_COUNT (model);

  /* + grandparent1
   * + grandparent2
   *   + parent1
   *     + iter_parent1
   *   + parent2
   *     + iter_parent2
   *     + iter_parent2
   */

  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &grandparent1, NULL);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &grandparent2, NULL);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &parent1, &grandparent2);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter_parent1, &parent1);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &parent2, &grandparent2);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter_parent2, &parent2);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter_parent2, &parent2);

  assert_entire_model_unreferenced (ref_model);

  sort_model = bobgui_tree_model_sort_new_with_model (model);
  tree_view = bobgui_tree_view_new_with_model (sort_model);

  assert_root_level_referenced (ref_model, 1);
  assert_node_ref_count (ref_model, &parent1, 0);
  assert_node_ref_count (ref_model, &parent2, 0);
  assert_level_unreferenced (ref_model, &parent1);
  assert_level_unreferenced (ref_model, &parent2);

  path = bobgui_tree_path_new_from_indices (1, -1);
  bobgui_tree_view_expand_row (BOBGUI_TREE_VIEW (tree_view), path, TRUE);
  bobgui_tree_path_free (path);

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 2);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_parent1, 1);
  assert_node_ref_count (ref_model, &iter_parent2, 1);

  bobgui_tree_store_remove (BOBGUI_TREE_STORE (model), &iter_parent2);

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 2);
  assert_level_referenced (ref_model, 1, &parent1);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_level_referenced (ref_model, 1, &parent2);

  bobgui_tree_store_remove (BOBGUI_TREE_STORE (model), &parent1);

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_level_referenced (ref_model, 1, &parent2);

  bobgui_tree_store_remove (BOBGUI_TREE_STORE (model), &grandparent2);

  assert_node_ref_count (ref_model, &grandparent1, 1);

  bobgui_tree_model_sort_clear_cache (BOBGUI_TREE_MODEL_SORT (sort_model));

  assert_node_ref_count (ref_model, &grandparent1, 1);

  g_object_unref (g_object_ref_sink (tree_view));

  assert_entire_model_unreferenced (ref_model);

  g_object_unref (sort_model);
  g_object_unref (ref_model);
}

static void
ref_count_cleanup (void)
{
  BobguiTreeIter grandparent1, grandparent2, parent1, parent2;
  BobguiTreeIter iter_parent1, iter_parent2;
  BobguiTreeModel *model;
  BobguiTreeModelRefCount *ref_model;
  BobguiTreeModel *sort_model;
  BobguiWidget *tree_view;

  model = bobgui_tree_model_ref_count_new ();
  ref_model = BOBGUI_TREE_MODEL_REF_COUNT (model);

  /* + grandparent1
   * + grandparent2
   *   + parent1
   *     + iter_parent1
   *   + parent2
   *     + iter_parent2
   *     + iter_parent2
   */

  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &grandparent1, NULL);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &grandparent2, NULL);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &parent1, &grandparent2);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter_parent1, &parent1);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &parent2, &grandparent2);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter_parent2, &parent2);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter_parent2, &parent2);

  sort_model = bobgui_tree_model_sort_new_with_model (model);
  tree_view = bobgui_tree_view_new_with_model (sort_model);

  bobgui_tree_view_expand_all (BOBGUI_TREE_VIEW (tree_view));

  g_object_unref (g_object_ref_sink (tree_view));

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 1);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 1);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  bobgui_tree_model_sort_clear_cache (BOBGUI_TREE_MODEL_SORT (sort_model));

  assert_entire_model_unreferenced (ref_model);

  g_object_unref (sort_model);
  g_object_unref (ref_model);
}

static void
ref_count_row_ref (void)
{
  BobguiTreeIter grandparent1, grandparent2, parent1, parent2;
  BobguiTreeIter iter_parent1, iter_parent2;
  BobguiTreeModel *model;
  BobguiTreeModelRefCount *ref_model;
  BobguiTreeModel *sort_model;
  BobguiWidget *tree_view;
  BobguiTreePath *path;
  BobguiTreeRowReference *row_ref;

  model = bobgui_tree_model_ref_count_new ();
  ref_model = BOBGUI_TREE_MODEL_REF_COUNT (model);

  /* + grandparent1
   * + grandparent2
   *   + parent1
   *     + iter_parent1
   *   + parent2
   *     + iter_parent2
   *     + iter_parent2
   */

  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &grandparent1, NULL);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &grandparent2, NULL);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &parent1, &grandparent2);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter_parent1, &parent1);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &parent2, &grandparent2);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter_parent2, &parent2);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter_parent2, &parent2);

  sort_model = bobgui_tree_model_sort_new_with_model (model);
  tree_view = bobgui_tree_view_new_with_model (sort_model);

  path = bobgui_tree_path_new_from_indices (1, 1, 1, -1);
  row_ref = bobgui_tree_row_reference_new (sort_model, path);
  bobgui_tree_path_free (path);

  assert_node_ref_count (ref_model, &grandparent1, 1);
  /* Referenced because the node is visible, its child level is built
   * and referenced by the row ref.
   */
  assert_node_ref_count (ref_model, &grandparent2, 3);
  assert_node_ref_count (ref_model, &parent1, 0);
  /* Referenced by the row ref and because its child level is built. */
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2, 1);

  bobgui_tree_row_reference_free (row_ref);

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 0);
  assert_node_ref_count (ref_model, &parent2, 1);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  path = bobgui_tree_path_new_from_indices (1, 1, 1, -1);
  row_ref = bobgui_tree_row_reference_new (sort_model, path);
  bobgui_tree_path_free (path);

  assert_node_ref_count (ref_model, &grandparent1, 1);
  /* Referenced because the node is visible, its child level is built
   * and referenced by the row ref.
   */
  assert_node_ref_count (ref_model, &grandparent2, 3);
  assert_node_ref_count (ref_model, &parent1, 0);
  /* Referenced by the row ref and because its child level is built. */
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2, 1);

  bobgui_tree_store_remove (BOBGUI_TREE_STORE (model), &parent2);

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 1);
  assert_node_ref_count (ref_model, &parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent1, 0);

  bobgui_tree_row_reference_free (row_ref);

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 1);
  assert_node_ref_count (ref_model, &parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent1, 0);

  g_object_unref (g_object_ref_sink (tree_view));
  g_object_unref (sort_model);

  assert_entire_model_unreferenced (ref_model);

  g_object_unref (ref_model);
}

static void
ref_count_reorder_single (void)
{
  BobguiTreeIter iter1, iter2, iter3, iter4, iter5;
  BobguiTreeIter siter1, siter2, siter3, siter4, siter5;
  BobguiTreeModel *model;
  BobguiTreeModelRefCount *ref_model;
  BobguiTreeModel *sort_model;
  BobguiWidget *tree_view;
  GType column_types[] = { G_TYPE_INT };

  model = bobgui_tree_model_ref_count_new ();
  ref_model = BOBGUI_TREE_MODEL_REF_COUNT (model);

  bobgui_tree_store_set_column_types (BOBGUI_TREE_STORE (model), 1,
                                   column_types);

  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter1, NULL, 0,
                                     0, 30, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter2, NULL, 1,
                                     0, 40, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter3, NULL, 2,
                                     0, 10, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter4, NULL, 3,
                                     0, 20, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter5, NULL, 4,
                                     0, 60, -1);

  assert_root_level_unreferenced (ref_model);

  sort_model = bobgui_tree_model_sort_new_with_model (model);
  tree_view = bobgui_tree_view_new_with_model (sort_model);

  assert_entire_model_referenced (ref_model, 1);

  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter1, &iter1);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter2, &iter2);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter3, &iter3);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter4, &iter4);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter5, &iter5);

  bobgui_tree_model_ref_node (BOBGUI_TREE_MODEL (sort_model), &siter1);
  bobgui_tree_model_ref_node (BOBGUI_TREE_MODEL (sort_model), &siter1);

  bobgui_tree_model_ref_node (BOBGUI_TREE_MODEL (sort_model), &siter3);
  bobgui_tree_model_ref_node (BOBGUI_TREE_MODEL (sort_model), &siter3);
  bobgui_tree_model_ref_node (BOBGUI_TREE_MODEL (sort_model), &siter3);

  bobgui_tree_model_ref_node (BOBGUI_TREE_MODEL (sort_model), &siter5);

  assert_node_ref_count (ref_model, &iter1, 3);
  assert_node_ref_count (ref_model, &iter2, 1);
  assert_node_ref_count (ref_model, &iter3, 4);
  assert_node_ref_count (ref_model, &iter4, 1);
  assert_node_ref_count (ref_model, &iter5, 2);

  /* Sort */
  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (sort_model),
                                        0, BOBGUI_SORT_ASCENDING);

  assert_node_ref_count (ref_model, &iter1, 3);
  assert_node_ref_count (ref_model, &iter2, 1);
  assert_node_ref_count (ref_model, &iter3, 4);
  assert_node_ref_count (ref_model, &iter4, 1);
  assert_node_ref_count (ref_model, &iter5, 2);

  /* Re-translate the iters after sorting */
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter1, &iter1);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter2, &iter2);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter3, &iter3);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter4, &iter4);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter5, &iter5);

  bobgui_tree_model_unref_node (BOBGUI_TREE_MODEL (sort_model), &siter1);
  bobgui_tree_model_unref_node (BOBGUI_TREE_MODEL (sort_model), &siter1);

  bobgui_tree_model_unref_node (BOBGUI_TREE_MODEL (sort_model), &siter3);
  bobgui_tree_model_unref_node (BOBGUI_TREE_MODEL (sort_model), &siter3);
  bobgui_tree_model_unref_node (BOBGUI_TREE_MODEL (sort_model), &siter3);

  bobgui_tree_model_unref_node (BOBGUI_TREE_MODEL (sort_model), &siter5);

  assert_entire_model_referenced (ref_model, 1);

  g_object_unref (g_object_ref_sink (tree_view));
  g_object_unref (sort_model);

  assert_entire_model_unreferenced (ref_model);

  g_object_unref (ref_model);
}

static void
ref_count_reorder_two (void)
{
  BobguiTreeIter iter1, iter2, iter3, iter4, iter5;
  BobguiTreeIter citer1, citer2, citer3, citer4, citer5;
  BobguiTreeIter siter1, siter2, siter3, siter4, siter5;
  BobguiTreeIter sciter1, sciter2, sciter3, sciter4, sciter5;
  BobguiTreeModel *model;
  BobguiTreeModelRefCount *ref_model;
  BobguiTreeModel *sort_model;
  BobguiWidget *tree_view;
  GType column_types[] = { G_TYPE_INT };

  model = bobgui_tree_model_ref_count_new ();
  ref_model = BOBGUI_TREE_MODEL_REF_COUNT (model);

  bobgui_tree_store_set_column_types (BOBGUI_TREE_STORE (model), 1,
                                   column_types);

  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter1, NULL, 0,
                                     0, 30, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter2, NULL, 1,
                                     0, 40, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter3, NULL, 2,
                                     0, 10, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter4, NULL, 3,
                                     0, 20, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter5, NULL, 4,
                                     0, 60, -1);

  /* Child level */
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &citer1, &iter1, 0,
                                     0, 30, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &citer2, &iter1, 1,
                                     0, 40, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &citer3, &iter1, 2,
                                     0, 10, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &citer4, &iter1, 3,
                                     0, 20, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &citer5, &iter1, 4,
                                     0, 60, -1);

  assert_root_level_unreferenced (ref_model);

  sort_model = bobgui_tree_model_sort_new_with_model (model);
  tree_view = bobgui_tree_view_new_with_model (sort_model);
  bobgui_tree_view_expand_all (BOBGUI_TREE_VIEW (tree_view));

  assert_node_ref_count (ref_model, &iter1, 2);
  assert_node_ref_count (ref_model, &iter2, 1);
  assert_node_ref_count (ref_model, &iter3, 1);
  assert_node_ref_count (ref_model, &iter4, 1);
  assert_node_ref_count (ref_model, &iter5, 1);

  assert_level_referenced (ref_model, 1, &iter1);

  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter1, &iter1);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter2, &iter2);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter3, &iter3);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter4, &iter4);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter5, &iter5);

  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &sciter1, &citer1);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &sciter2, &citer2);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &sciter3, &citer3);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &sciter4, &citer4);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &sciter5, &citer5);

  bobgui_tree_model_ref_node (BOBGUI_TREE_MODEL (sort_model), &siter1);
  bobgui_tree_model_ref_node (BOBGUI_TREE_MODEL (sort_model), &siter1);

  bobgui_tree_model_ref_node (BOBGUI_TREE_MODEL (sort_model), &siter3);
  bobgui_tree_model_ref_node (BOBGUI_TREE_MODEL (sort_model), &siter3);
  bobgui_tree_model_ref_node (BOBGUI_TREE_MODEL (sort_model), &siter3);

  bobgui_tree_model_ref_node (BOBGUI_TREE_MODEL (sort_model), &siter5);

  assert_node_ref_count (ref_model, &iter1, 4);
  assert_node_ref_count (ref_model, &iter2, 1);
  assert_node_ref_count (ref_model, &iter3, 4);
  assert_node_ref_count (ref_model, &iter4, 1);
  assert_node_ref_count (ref_model, &iter5, 2);

  bobgui_tree_model_ref_node (BOBGUI_TREE_MODEL (sort_model), &sciter3);
  bobgui_tree_model_ref_node (BOBGUI_TREE_MODEL (sort_model), &sciter3);

  bobgui_tree_model_ref_node (BOBGUI_TREE_MODEL (sort_model), &sciter5);
  bobgui_tree_model_ref_node (BOBGUI_TREE_MODEL (sort_model), &sciter5);
  bobgui_tree_model_ref_node (BOBGUI_TREE_MODEL (sort_model), &sciter5);

  bobgui_tree_model_ref_node (BOBGUI_TREE_MODEL (sort_model), &sciter1);

  assert_node_ref_count (ref_model, &citer1, 2);
  assert_node_ref_count (ref_model, &citer2, 1);
  assert_node_ref_count (ref_model, &citer3, 3);
  assert_node_ref_count (ref_model, &citer4, 1);
  assert_node_ref_count (ref_model, &citer5, 4);

  /* Sort */
  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (sort_model),
                                        0, BOBGUI_SORT_ASCENDING);

  assert_node_ref_count (ref_model, &iter1, 4);
  assert_node_ref_count (ref_model, &iter2, 1);
  assert_node_ref_count (ref_model, &iter3, 4);
  assert_node_ref_count (ref_model, &iter4, 1);
  assert_node_ref_count (ref_model, &iter5, 2);

  assert_node_ref_count (ref_model, &citer1, 2);
  assert_node_ref_count (ref_model, &citer2, 1);
  assert_node_ref_count (ref_model, &citer3, 3);
  assert_node_ref_count (ref_model, &citer4, 1);
  assert_node_ref_count (ref_model, &citer5, 4);

  /* Re-translate the iters after sorting */
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter1, &iter1);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter2, &iter2);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter3, &iter3);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter4, &iter4);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &siter5, &iter5);

  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &sciter1, &citer1);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &sciter2, &citer2);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &sciter3, &citer3);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &sciter4, &citer4);
  bobgui_tree_model_sort_convert_child_iter_to_iter (BOBGUI_TREE_MODEL_SORT (sort_model), &sciter5, &citer5);

  bobgui_tree_model_unref_node (BOBGUI_TREE_MODEL (sort_model), &siter1);
  bobgui_tree_model_unref_node (BOBGUI_TREE_MODEL (sort_model), &siter1);

  bobgui_tree_model_unref_node (BOBGUI_TREE_MODEL (sort_model), &siter3);
  bobgui_tree_model_unref_node (BOBGUI_TREE_MODEL (sort_model), &siter3);
  bobgui_tree_model_unref_node (BOBGUI_TREE_MODEL (sort_model), &siter3);

  bobgui_tree_model_unref_node (BOBGUI_TREE_MODEL (sort_model), &siter5);

  assert_node_ref_count (ref_model, &iter1, 2);
  assert_node_ref_count (ref_model, &iter2, 1);
  assert_node_ref_count (ref_model, &iter3, 1);
  assert_node_ref_count (ref_model, &iter4, 1);
  assert_node_ref_count (ref_model, &iter5, 1);

  bobgui_tree_model_unref_node (BOBGUI_TREE_MODEL (sort_model), &sciter3);
  bobgui_tree_model_unref_node (BOBGUI_TREE_MODEL (sort_model), &sciter3);

  bobgui_tree_model_unref_node (BOBGUI_TREE_MODEL (sort_model), &sciter5);
  bobgui_tree_model_unref_node (BOBGUI_TREE_MODEL (sort_model), &sciter5);
  bobgui_tree_model_unref_node (BOBGUI_TREE_MODEL (sort_model), &sciter5);

  bobgui_tree_model_unref_node (BOBGUI_TREE_MODEL (sort_model), &sciter1);

  assert_level_referenced (ref_model, 1, &iter1);

  g_object_unref (g_object_ref_sink (tree_view));
  g_object_unref (sort_model);

  assert_entire_model_unreferenced (ref_model);

  g_object_unref (ref_model);
}

static void
check_sort_order (BobguiTreeModel *sort_model,
                  BobguiSortType   sort_order,
                  const char   *parent_path)
{
  int prev_value;
  BobguiTreeIter siter;

  if (!parent_path)
    bobgui_tree_model_get_iter_first (sort_model, &siter);
  else
    {
      BobguiTreePath *path;

      path = bobgui_tree_path_new_from_string (parent_path);
      bobgui_tree_path_append_index (path, 0);

      bobgui_tree_model_get_iter (sort_model, &siter, path);

      bobgui_tree_path_free (path);
    }

  if (sort_order == BOBGUI_SORT_ASCENDING)
    prev_value = -1;
  else
    prev_value = INT_MAX;

  do
    {
      int value;

      bobgui_tree_model_get (sort_model, &siter, 0, &value, -1);
      if (sort_order == BOBGUI_SORT_ASCENDING)
        g_assert_cmpint (prev_value, <=, value);
      else
        g_assert_cmpint (prev_value, >=, value);

      prev_value = value;
    }
  while (bobgui_tree_model_iter_next (sort_model, &siter));
}

static void
rows_reordered_single_level (void)
{
  BobguiTreeIter iter1, iter2, iter3, iter4, iter5;
  BobguiTreeModel *model;
  BobguiTreeModelRefCount *ref_model;
  BobguiTreeModel *sort_model;
  BobguiWidget *tree_view;
  SignalMonitor *monitor;
  BobguiTreePath *path;
  GType column_types[] = { G_TYPE_INT };
  int order[][5] =
    {
        { 2, 3, 0, 1, 4 },
        { 4, 3, 2, 1, 0 },
        { 2, 1, 4, 3, 0 }
    };

  model = bobgui_tree_model_ref_count_new ();
  ref_model = BOBGUI_TREE_MODEL_REF_COUNT (model);

  bobgui_tree_store_set_column_types (BOBGUI_TREE_STORE (model), 1,
                                   column_types);

  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter1, NULL, 0,
                                     0, 30, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter2, NULL, 1,
                                     0, 40, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter3, NULL, 2,
                                     0, 10, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter4, NULL, 3,
                                     0, 20, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter5, NULL, 4,
                                     0, 60, -1);

  sort_model = bobgui_tree_model_sort_new_with_model (model);
  tree_view = bobgui_tree_view_new_with_model (sort_model);

  monitor = signal_monitor_new (sort_model);

  /* Sort */
  path = bobgui_tree_path_new ();
  signal_monitor_append_signal_reordered (monitor,
                                          ROWS_REORDERED,
                                          path, order[0], 5);
  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (sort_model),
                                        0, BOBGUI_SORT_ASCENDING);
  signal_monitor_assert_is_empty (monitor);
  check_sort_order (sort_model, BOBGUI_SORT_ASCENDING, NULL);

  signal_monitor_append_signal_reordered (monitor,
                                          ROWS_REORDERED,
                                          path, order[1], 5);
  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (sort_model),
                                        0, BOBGUI_SORT_DESCENDING);
  signal_monitor_assert_is_empty (monitor);
  check_sort_order (sort_model, BOBGUI_SORT_DESCENDING, NULL);

  signal_monitor_append_signal_reordered (monitor,
                                          ROWS_REORDERED,
                                          path, order[2], 5);
  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (sort_model),
                                        BOBGUI_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                        BOBGUI_SORT_ASCENDING);
  signal_monitor_assert_is_empty (monitor);

  bobgui_tree_path_free (path);
  signal_monitor_free (monitor);

  g_object_unref (g_object_ref_sink (tree_view));
  g_object_unref (sort_model);

  assert_entire_model_unreferenced (ref_model);

  g_object_unref (ref_model);
}

static void
rows_reordered_two_levels (void)
{
  BobguiTreeIter iter1, iter2, iter3, iter4, iter5;
  BobguiTreeIter citer1, citer2, citer3, citer4, citer5;
  BobguiTreeModel *model;
  BobguiTreeModelRefCount *ref_model;
  BobguiTreeModel *sort_model;
  BobguiWidget *tree_view;
  SignalMonitor *monitor;
  BobguiTreePath *path, *child_path;
  GType column_types[] = { G_TYPE_INT };
  int order[][5] =
    {
        { 2, 3, 0, 1, 4 },
        { 4, 3, 2, 1, 0 },
        { 2, 1, 4, 3, 0 }
    };

  model = bobgui_tree_model_ref_count_new ();
  ref_model = BOBGUI_TREE_MODEL_REF_COUNT (model);

  bobgui_tree_store_set_column_types (BOBGUI_TREE_STORE (model), 1,
                                   column_types);

  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter1, NULL, 0,
                                     0, 30, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter2, NULL, 1,
                                     0, 40, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter3, NULL, 2,
                                     0, 10, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter4, NULL, 3,
                                     0, 20, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter5, NULL, 4,
                                     0, 60, -1);

  /* Child level */
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &citer1, &iter1, 0,
                                     0, 30, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &citer2, &iter1, 1,
                                     0, 40, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &citer3, &iter1, 2,
                                     0, 10, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &citer4, &iter1, 3,
                                     0, 20, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &citer5, &iter1, 4,
                                     0, 60, -1);

  sort_model = bobgui_tree_model_sort_new_with_model (model);
  tree_view = bobgui_tree_view_new_with_model (sort_model);
  bobgui_tree_view_expand_all (BOBGUI_TREE_VIEW (tree_view));

  monitor = signal_monitor_new (sort_model);

  /* Sort */
  path = bobgui_tree_path_new ();
  child_path = bobgui_tree_path_new_from_indices (2, -1);
  signal_monitor_append_signal_reordered (monitor,
                                          ROWS_REORDERED,
                                          path, order[0], 5);
  signal_monitor_append_signal_reordered (monitor,
                                          ROWS_REORDERED,
                                          child_path, order[0], 5);
  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (sort_model),
                                        0, BOBGUI_SORT_ASCENDING);
  signal_monitor_assert_is_empty (monitor);
  check_sort_order (sort_model, BOBGUI_SORT_ASCENDING, NULL);
  /* The parent node of the child level moved due to sorting */
  check_sort_order (sort_model, BOBGUI_SORT_ASCENDING, "2");

  signal_monitor_append_signal_reordered (monitor,
                                          ROWS_REORDERED,
                                          path, order[1], 5);
  signal_monitor_append_signal_reordered (monitor,
                                          ROWS_REORDERED,
                                          child_path, order[1], 5);
  bobgui_tree_path_free (child_path);
  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (sort_model),
                                        0, BOBGUI_SORT_DESCENDING);
  signal_monitor_assert_is_empty (monitor);
  check_sort_order (sort_model, BOBGUI_SORT_DESCENDING, NULL);
  /* The parent node of the child level moved due to sorting */
  check_sort_order (sort_model, BOBGUI_SORT_DESCENDING, "2");

  child_path = bobgui_tree_path_new_from_indices (0, -1);
  signal_monitor_append_signal_reordered (monitor,
                                          ROWS_REORDERED,
                                          path, order[2], 5);
  signal_monitor_append_signal_reordered (monitor,
                                          ROWS_REORDERED,
                                          child_path, order[2], 5);
  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (sort_model),
                                        BOBGUI_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                        BOBGUI_SORT_ASCENDING);
  signal_monitor_assert_is_empty (monitor);

  bobgui_tree_path_free (path);
  bobgui_tree_path_free (child_path);
  signal_monitor_free (monitor);

  g_object_unref (g_object_ref_sink (tree_view));
  g_object_unref (sort_model);

  g_object_unref (ref_model);
}

static void
sorted_insert (void)
{
  BobguiTreeIter iter1, iter2, iter3, iter4, iter5, new_iter;
  BobguiTreeModel *model;
  BobguiTreeModelRefCount *ref_model;
  BobguiTreeModel *sort_model;
  BobguiWidget *tree_view;
  SignalMonitor *monitor;
  BobguiTreePath *path;
  GType column_types[] = { G_TYPE_INT };
  int order0[] = { 1, 2, 3, 0, 4, 5, 6 };

  model = bobgui_tree_model_ref_count_new ();
  ref_model = BOBGUI_TREE_MODEL_REF_COUNT (model);

  bobgui_tree_store_set_column_types (BOBGUI_TREE_STORE (model), 1,
                                   column_types);

  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter1, NULL, 0,
                                     0, 30, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter2, NULL, 1,
                                     0, 40, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter3, NULL, 2,
                                     0, 10, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter4, NULL, 3,
                                     0, 20, -1);
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &iter5, NULL, 4,
                                     0, 60, -1);

  sort_model = bobgui_tree_model_sort_new_with_model (model);
  tree_view = bobgui_tree_view_new_with_model (sort_model);

  /* Sort */
  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (sort_model),
                                        0, BOBGUI_SORT_ASCENDING);
  check_sort_order (sort_model, BOBGUI_SORT_ASCENDING, NULL);

  monitor = signal_monitor_new (sort_model);

  /* Insert a new item */
  signal_monitor_append_signal (monitor, ROW_INSERTED, "4");
  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model), &new_iter, NULL,
                                     5, 0, 50, -1);
  signal_monitor_assert_is_empty (monitor);
  check_sort_order (sort_model, BOBGUI_SORT_ASCENDING, NULL);

  /* Sort the tree sort and append a new item */
  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (model),
                                        0, BOBGUI_SORT_ASCENDING);
  check_sort_order (model, BOBGUI_SORT_ASCENDING, NULL);

  path = bobgui_tree_path_new ();
  signal_monitor_append_signal (monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal_reordered (monitor,
                                          ROWS_REORDERED,
                                          path, order0, 7);
  signal_monitor_append_signal (monitor, ROW_CHANGED, "3");
  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &new_iter, NULL);
  bobgui_tree_store_set (BOBGUI_TREE_STORE (model), &new_iter, 0, 35, -1);
  check_sort_order (model, BOBGUI_SORT_ASCENDING, NULL);
  check_sort_order (sort_model, BOBGUI_SORT_ASCENDING, NULL);

  bobgui_tree_path_free (path);
  signal_monitor_free (monitor);

  g_object_unref (g_object_ref_sink (tree_view));
  g_object_unref (sort_model);

  g_object_unref (ref_model);
}


static void
specific_bug_300089 (void)
{
  /* Test case for GNOME Bugzilla bug 300089.  Written by
   * Matthias Clasen.
   */
  BobguiTreeModel *sort_model, *child_model;
  BobguiTreePath *path;
  BobguiTreeIter iter, iter2, sort_iter;

  /*http://bugzilla.gnome.org/show_bug.cgi?id=300089 */

  child_model = BOBGUI_TREE_MODEL (bobgui_tree_store_new (1, G_TYPE_STRING));

  bobgui_tree_store_append (BOBGUI_TREE_STORE (child_model), &iter, NULL);
  bobgui_tree_store_set (BOBGUI_TREE_STORE (child_model), &iter, 0, "A", -1);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (child_model), &iter, NULL);
  bobgui_tree_store_set (BOBGUI_TREE_STORE (child_model), &iter, 0, "B", -1);

  bobgui_tree_store_append (BOBGUI_TREE_STORE (child_model), &iter2, &iter);
  bobgui_tree_store_set (BOBGUI_TREE_STORE (child_model), &iter2, 0, "D", -1);
  bobgui_tree_store_append (BOBGUI_TREE_STORE (child_model), &iter2, &iter);
  bobgui_tree_store_set (BOBGUI_TREE_STORE (child_model), &iter2, 0, "E", -1);

  bobgui_tree_store_append (BOBGUI_TREE_STORE (child_model), &iter, NULL);
  bobgui_tree_store_set (BOBGUI_TREE_STORE (child_model), &iter, 0, "C", -1);


  sort_model = BOBGUI_TREE_MODEL (bobgui_tree_model_sort_new_with_model (child_model));
  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (sort_model),
                                        0, BOBGUI_SORT_ASCENDING);

  path = bobgui_tree_path_new_from_indices (1, 1, -1);

  /* make sure a level is constructed */
  bobgui_tree_model_get_iter (sort_model, &sort_iter, path);

  /* change the "E" row in a way that causes it to change position */ 
  bobgui_tree_model_get_iter (child_model, &iter, path);
  bobgui_tree_store_set (BOBGUI_TREE_STORE (child_model), &iter, 0, "A", -1);

  bobgui_tree_path_free (path);
}

static void
specific_bug_364946 (void)
{
  /* This is a test case for GNOME Bugzilla bug 364946.  It was written
   * by Andreas Koehler.
   */
  BobguiTreeStore *store;
  BobguiTreeIter a, aa, aaa, aab, iter;
  BobguiTreeModel *s_model;

  /*http://bugzilla.gnome.org/show_bug.cgi?id=364946 */

  store = bobgui_tree_store_new (1, G_TYPE_STRING);

  bobgui_tree_store_append (store, &a, NULL);
  bobgui_tree_store_set (store, &a, 0, "0", -1);

  bobgui_tree_store_append (store, &aa, &a);
  bobgui_tree_store_set (store, &aa, 0, "0:0", -1);

  bobgui_tree_store_append (store, &aaa, &aa);
  bobgui_tree_store_set (store, &aaa, 0, "0:0:0", -1);

  bobgui_tree_store_append (store, &aab, &aa);
  bobgui_tree_store_set (store, &aab, 0, "0:0:1", -1);

  s_model = bobgui_tree_model_sort_new_with_model (BOBGUI_TREE_MODEL (store));
  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (s_model), 0,
                                        BOBGUI_SORT_ASCENDING);

  bobgui_tree_model_get_iter_from_string (s_model, &iter, "0:0:0");

  bobgui_tree_store_set (store, &aaa, 0, "0:0:0", -1);
  bobgui_tree_store_remove (store, &aaa);
  bobgui_tree_store_remove (store, &aab);

  bobgui_tree_model_sort_clear_cache (BOBGUI_TREE_MODEL_SORT (s_model));
}

static void
iter_test (BobguiTreeModel *model)
{
  BobguiTreeIter a, b;

  g_assert_true (bobgui_tree_model_get_iter_first (model, &a));

  g_assert_true (bobgui_tree_model_iter_next (model, &a));
  g_assert_true (bobgui_tree_model_iter_next (model, &a));
  b = a;
  g_assert_false (bobgui_tree_model_iter_next (model, &b));

  g_assert_true (bobgui_tree_model_iter_previous (model, &a));
  g_assert_true (bobgui_tree_model_iter_previous (model, &a));
  b = a;
  g_assert_false (bobgui_tree_model_iter_previous (model, &b));
}

static void
specific_bug_674587 (void)
{
  BobguiListStore *l;
  BobguiTreeStore *t;
  BobguiTreeModel *m;
  BobguiTreeIter a;

  l = bobgui_list_store_new (1, G_TYPE_STRING);

  bobgui_list_store_append (l, &a);
  bobgui_list_store_set (l, &a, 0, "0", -1);
  bobgui_list_store_append (l, &a);
  bobgui_list_store_set (l, &a, 0, "1", -1);
  bobgui_list_store_append (l, &a);
  bobgui_list_store_set (l, &a, 0, "2", -1);

  iter_test (BOBGUI_TREE_MODEL (l));

  g_object_unref (l);

  t = bobgui_tree_store_new (1, G_TYPE_STRING);

  bobgui_tree_store_append (t, &a, NULL);
  bobgui_tree_store_set (t, &a, 0, "0", -1);
  bobgui_tree_store_append (t, &a, NULL);
  bobgui_tree_store_set (t, &a, 0, "1", -1);
  bobgui_tree_store_append (t, &a, NULL);
  bobgui_tree_store_set (t, &a, 0, "2", -1);

  iter_test (BOBGUI_TREE_MODEL (t));

  m = bobgui_tree_model_sort_new_with_model (BOBGUI_TREE_MODEL (t));

  iter_test (m);

  g_object_unref (t);
  g_object_unref (m);
}

static void
row_deleted_cb (BobguiTreeModel *tree_model,
    BobguiTreePath  *path,
    guint *count)
{
  *count = *count + 1;
}

static void
specific_bug_698846 (void)
{
  BobguiListStore *store;
  BobguiTreeModel *sorted;
  BobguiTreeIter iter;
  guint count = 0;

  /*http://bugzilla.gnome.org/show_bug.cgi?id=698846 */

  store = bobgui_list_store_new (1, G_TYPE_STRING);
  sorted = bobgui_tree_model_sort_new_with_model (BOBGUI_TREE_MODEL (store));

  bobgui_list_store_insert_with_values (store, &iter, 0, 0, "a", -1);
  bobgui_list_store_insert_with_values (store, &iter, 1, 0, "b", -1);

  g_signal_connect (sorted, "row-deleted", G_CALLBACK (row_deleted_cb), &count);

  bobgui_list_store_clear (store);

  g_assert_cmpuint (count, ==, 2);
}

static int
sort_func (BobguiTreeModel *model,
           BobguiTreeIter  *a,
           BobguiTreeIter  *b,
           gpointer      data)
{
  return 0;
}

static int column_changed;

static void
sort_column_changed (BobguiTreeSortable *sortable)
{
  column_changed++;
}

static void
sort_column_change (void)
{
  BobguiListStore *store;
  BobguiTreeModel *sorted;
  int col;
  BobguiSortType order;
  gboolean ret;

  /*http://bugzilla.gnome.org/show_bug.cgi?id=792459 */

  store = bobgui_list_store_new (1, G_TYPE_STRING);
  sorted = bobgui_tree_model_sort_new_with_model (BOBGUI_TREE_MODEL (store));

  column_changed = 0;
  g_signal_connect (sorted, "sort-column-changed", G_CALLBACK (sort_column_changed), NULL);

  g_assert_false (bobgui_tree_sortable_has_default_sort_func (BOBGUI_TREE_SORTABLE (sorted)));
  bobgui_tree_sortable_set_default_sort_func (BOBGUI_TREE_SORTABLE (sorted), sort_func, NULL, NULL);
  g_assert_true (bobgui_tree_sortable_has_default_sort_func (BOBGUI_TREE_SORTABLE (sorted)));

  bobgui_tree_sortable_set_sort_func (BOBGUI_TREE_SORTABLE (sorted), 0, sort_func, NULL, NULL);

  ret = bobgui_tree_sortable_get_sort_column_id (BOBGUI_TREE_SORTABLE (sorted), &col, &order);
  g_assert_cmpint (column_changed, ==, 0);
  g_assert_false (ret);
  g_assert_cmpint (col, ==, BOBGUI_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID);
  g_assert_cmpint (order, ==, BOBGUI_SORT_ASCENDING);

  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (sorted),
                                        BOBGUI_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, BOBGUI_SORT_DESCENDING);
  ret = bobgui_tree_sortable_get_sort_column_id (BOBGUI_TREE_SORTABLE (sorted), &col, &order);
  g_assert_cmpint (column_changed, ==, 1);
  g_assert_false (ret);
  g_assert_cmpint (col, ==, BOBGUI_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID);
  g_assert_cmpint (order, ==, BOBGUI_SORT_DESCENDING);

  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (sorted),
                                        BOBGUI_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, BOBGUI_SORT_DESCENDING);
  ret = bobgui_tree_sortable_get_sort_column_id (BOBGUI_TREE_SORTABLE (sorted), &col, &order);
  g_assert_cmpint (column_changed, ==, 1);
  g_assert_false (ret);
  g_assert_cmpint (col, ==, BOBGUI_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID);
  g_assert_cmpint (order, ==, BOBGUI_SORT_DESCENDING);

  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (sorted),
                                        0, BOBGUI_SORT_DESCENDING);
  ret = bobgui_tree_sortable_get_sort_column_id (BOBGUI_TREE_SORTABLE (sorted), &col, &order);
  g_assert_cmpint (column_changed, ==, 2);
  g_assert_true (ret);
  g_assert_cmpint (col, ==, 0);
  g_assert_cmpint (order, ==, BOBGUI_SORT_DESCENDING);

  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (sorted),
                                        BOBGUI_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, BOBGUI_SORT_ASCENDING);
  ret = bobgui_tree_sortable_get_sort_column_id (BOBGUI_TREE_SORTABLE (sorted), &col, &order);
  g_assert_cmpint (column_changed, ==, 3);
  g_assert_false (ret);
  g_assert_cmpint (col, ==, BOBGUI_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID);
  g_assert_cmpint (order, ==, BOBGUI_SORT_ASCENDING);

  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (sorted),
                                        BOBGUI_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, BOBGUI_SORT_ASCENDING);
  ret = bobgui_tree_sortable_get_sort_column_id (BOBGUI_TREE_SORTABLE (sorted), &col, &order);
  g_assert_cmpint (column_changed, ==, 4);
  g_assert_false (ret);
  g_assert_cmpint (col, ==, BOBGUI_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID);
  g_assert_cmpint (order, ==, BOBGUI_SORT_ASCENDING);
}

/* main */

void
register_sort_model_tests (void)
{
  g_test_add_func ("/TreeModelSort/ref-count/single-level",
                   ref_count_single_level);
  g_test_add_func ("/TreeModelSort/ref-count/two-levels",
                   ref_count_two_levels);
  g_test_add_func ("/TreeModelSort/ref-count/three-levels",
                   ref_count_three_levels);
  g_test_add_func ("/TreeModelSort/ref-count/delete-row",
                   ref_count_delete_row);
  g_test_add_func ("/TreeModelSort/ref-count/cleanup",
                   ref_count_cleanup);
  g_test_add_func ("/TreeModelSort/ref-count/row-ref",
                   ref_count_row_ref);
  g_test_add_func ("/TreeModelSort/ref-count/reorder/single-level",
                   ref_count_reorder_single);
  g_test_add_func ("/TreeModelSort/ref-count/reorder/two-levels",
                   ref_count_reorder_two);

  g_test_add_func ("/TreeModelSort/rows-reordered/single-level",
                   rows_reordered_single_level);
  g_test_add_func ("/TreeModelSort/rows-reordered/two-levels",
                   rows_reordered_two_levels);
  g_test_add_func ("/TreeModelSort/sorted-insert",
                   sorted_insert);

  g_test_add_func ("/TreeModelSort/specific/bug-300089",
                   specific_bug_300089);
  g_test_add_func ("/TreeModelSort/specific/bug-364946",
                   specific_bug_364946);
  g_test_add_func ("/TreeModelSort/specific/bug-674587",
                   specific_bug_674587);
  g_test_add_func ("/TreeModelSort/specific/bug-698846",
                   specific_bug_698846);
  g_test_add_func ("/TreeModelSort/specific/bug-792459",
                   sort_column_change);
}

