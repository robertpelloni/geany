/* testtreeview.c
 * Copyright (C) 2011 Red Hat, Inc
 * Author: Benjamin Otte <otte@gnome.org>
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

#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

#define MIN_ROWS 50
#define MAX_ROWS 150

typedef void (* DoStuffFunc) (BobguiTreeView *treeview);

static guint
count_children (BobguiTreeModel *model,
                BobguiTreeIter  *parent)
{
  BobguiTreeIter iter;
  guint count = 0;
  gboolean valid;

  for (valid = bobgui_tree_model_iter_children (model, &iter, parent);
       valid;
       valid = bobgui_tree_model_iter_next (model, &iter))
    {
      count += count_children (model, &iter) + 1;
    }

  return count;
}

static void
set_rows (BobguiTreeView *treeview, guint i)
{
  g_assert (i == count_children (bobgui_tree_view_get_model (treeview), NULL));
  g_object_set_data (G_OBJECT (treeview), "rows", GUINT_TO_POINTER (i));
}

static guint
get_rows (BobguiTreeView *treeview)
{
  return GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (treeview), "rows"));
}

static void
log_operation_for_path (BobguiTreePath *path,
                        const char  *operation_name)
{
  char *path_string;

  path_string = path ? bobgui_tree_path_to_string (path) : g_strdup ("");

  g_printerr ("%10s %s\n", operation_name, path_string);

  g_free (path_string);
}

static void
log_operation (BobguiTreeModel *model,
               BobguiTreeIter  *iter,
               const char   *operation_name)
{
  BobguiTreePath *path;

  path = bobgui_tree_model_get_path (model, iter);

  log_operation_for_path (path, operation_name);

  bobgui_tree_path_free (path);
}

/* moves iter to the next iter in the model in the display order
 * inside a treeview. Returns FALSE if no more rows exist.
 */
static gboolean
tree_model_iter_step (BobguiTreeModel *model,
                      BobguiTreeIter *iter)
{
  BobguiTreeIter tmp;
  
  if (bobgui_tree_model_iter_children (model, &tmp, iter))
    {
      *iter = tmp;
      return TRUE;
    }

  do {
    tmp = *iter;

    if (bobgui_tree_model_iter_next (model, iter))
      return TRUE;
    }
  while (bobgui_tree_model_iter_parent (model, iter, &tmp));

  return FALSE;
}

/* NB: may include invisible iters (because they are collapsed) */
static void
tree_view_random_iter (BobguiTreeView *treeview,
                       BobguiTreeIter *iter)
{
  guint n_rows = get_rows (treeview);
  guint i = g_random_int_range (0, n_rows);
  BobguiTreeModel *model;

  model = bobgui_tree_view_get_model (treeview);
  
  if (!bobgui_tree_model_get_iter_first (model, iter))
    return;

  while (i-- > 0)
    {
      if (!tree_model_iter_step (model, iter))
        {
          g_assert_not_reached ();
          return;
        }
    }

  return;
}

static void
delete (BobguiTreeView *treeview)
{
  guint n_rows = get_rows (treeview);
  BobguiTreeModel *model;
  BobguiTreeIter iter;

  model = bobgui_tree_view_get_model (treeview);
  
  tree_view_random_iter (treeview, &iter);

  n_rows -= count_children (model, &iter) + 1;
  log_operation (model, &iter, "remove");
  bobgui_tree_store_remove (BOBGUI_TREE_STORE (model), &iter);
  set_rows (treeview, n_rows);
}

static void
add_one (BobguiTreeModel *model,
         BobguiTreeIter *iter)
{
  guint n = bobgui_tree_model_iter_n_children (model, iter);
  BobguiTreeIter new_iter;
  static guint counter = 0;
  
  if (n > 0 && g_random_boolean ())
    {
      BobguiTreeIter child;
      bobgui_tree_model_iter_nth_child (model, &child, iter, g_random_int_range (0, n));
      add_one (model, &child);
      return;
    }

  bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model),
                                     &new_iter,
                                     iter,
                                     g_random_int_range (-1, n),
                                     0, ++counter,
                                     -1);
  log_operation (model, &new_iter, "add");
}

static void
add (BobguiTreeView *treeview)
{
  BobguiTreeModel *model;

  model = bobgui_tree_view_get_model (treeview);
  add_one (model, NULL);

  set_rows (treeview, get_rows (treeview) + 1);
}

static void
add_or_delete (BobguiTreeView *treeview)
{
  guint n_rows = get_rows (treeview);

  if (g_random_int_range (MIN_ROWS, MAX_ROWS) >= n_rows)
    add (treeview);
  else
    delete (treeview);
}

/* XXX: We only expand/collapse from the top and not randomly */
static void
expand (BobguiTreeView *treeview)
{
  BobguiTreeModel *model;
  BobguiTreeIter iter;
  BobguiTreePath *path;
  gboolean valid;

  model = bobgui_tree_view_get_model (treeview);
  
  for (valid = bobgui_tree_model_get_iter_first (model, &iter);
       valid;
       valid = tree_model_iter_step (model, &iter))
    {
      if (bobgui_tree_model_iter_has_child (model, &iter))
        {
          path = bobgui_tree_model_get_path (model, &iter);
          if (!bobgui_tree_view_row_expanded (treeview, path))
            {
              log_operation (model, &iter, "expand");
              bobgui_tree_view_expand_row (treeview, path, FALSE);
              bobgui_tree_path_free (path);
              return;
            }
          bobgui_tree_path_free (path);
        }
    }
}

static void
collapse (BobguiTreeView *treeview)
{
  BobguiTreeModel *model;
  BobguiTreeIter iter;
  BobguiTreePath *last, *path;
  gboolean valid;

  model = bobgui_tree_view_get_model (treeview);
  last = NULL;
  
  for (valid = bobgui_tree_model_get_iter_first (model, &iter);
       valid;
       valid = tree_model_iter_step (model, &iter))
    {
      path = bobgui_tree_model_get_path (model, &iter);
      if (bobgui_tree_view_row_expanded (treeview, path))
        {
          if (last)
            bobgui_tree_path_free (last);
          last = path;
        }
      else
        bobgui_tree_path_free (path);
    }

  if (last)
    {
      log_operation_for_path (last, "collapse");
      bobgui_tree_view_collapse_row (treeview, last);
      bobgui_tree_path_free (last);
    }
}

static void
select_ (BobguiTreeView *treeview)
{
  BobguiTreeIter iter;

  tree_view_random_iter (treeview, &iter);

  log_operation (bobgui_tree_view_get_model (treeview), &iter, "select");
  bobgui_tree_selection_select_iter (bobgui_tree_view_get_selection (treeview),
                                  &iter);
}

static void
unselect (BobguiTreeView *treeview)
{
  BobguiTreeIter iter;

  tree_view_random_iter (treeview, &iter);

  log_operation (bobgui_tree_view_get_model (treeview), &iter, "unselect");
  bobgui_tree_selection_unselect_iter (bobgui_tree_view_get_selection (treeview),
                                    &iter);
}

static void
reset_model (BobguiTreeView *treeview)
{
  BobguiTreeSelection *selection;
  BobguiTreeModel *model;
  GList *list, *selected;
  BobguiTreePath *cursor;
  
  selection = bobgui_tree_view_get_selection (treeview);
  model = g_object_ref (bobgui_tree_view_get_model (treeview));

  log_operation_for_path (NULL, "reset");

  selected = bobgui_tree_selection_get_selected_rows (selection, NULL);
  bobgui_tree_view_get_cursor (treeview, &cursor, NULL);

  bobgui_tree_view_set_model (treeview, NULL);
  bobgui_tree_view_set_model (treeview, model);

  if (cursor)
    {
      bobgui_tree_view_set_cursor (treeview, cursor, NULL, FALSE);
      bobgui_tree_path_free (cursor);
    }
  for (list = selected; list; list = list->next)
    {
      bobgui_tree_selection_select_path (selection, list->data);
    }
  g_list_free_full (selected, (GDestroyNotify) bobgui_tree_path_free);

  g_object_unref (model);
}

/* sanity checks */

static void
assert_row_reference_is_path (BobguiTreeRowReference *ref,
                              BobguiTreePath *path)
{
  BobguiTreePath *expected;

  if (ref == NULL)
    {
      g_assert (path == NULL);
      return;
    }

  g_assert (path != NULL);
  g_assert (bobgui_tree_row_reference_valid (ref));

  expected = bobgui_tree_row_reference_get_path (ref);
  g_assert (expected != NULL);
  g_assert (bobgui_tree_path_compare (expected, path) == 0);
  bobgui_tree_path_free (expected);
}

static void
check_cursor (BobguiTreeView *treeview)
{
  BobguiTreeRowReference *ref = g_object_get_data (G_OBJECT (treeview), "cursor");
  BobguiTreePath *cursor;

  bobgui_tree_view_get_cursor (treeview, &cursor, NULL);
  assert_row_reference_is_path (ref, cursor);

  if (cursor)
    bobgui_tree_path_free (cursor);
}

static void
check_selection_item (BobguiTreeModel *model,
                      BobguiTreePath  *path,
                      BobguiTreeIter  *iter,
                      gpointer      listp)
{
  GList **list = listp;

  g_assert (*list);
  assert_row_reference_is_path ((*list)->data, path);
  *list = (*list)->next;
}

static void
check_selection (BobguiTreeView *treeview)
{
  GList *selection = g_object_get_data (G_OBJECT (treeview), "selection");

  bobgui_tree_selection_selected_foreach (bobgui_tree_view_get_selection (treeview),
                                       check_selection_item,
                                       &selection);
}

static void
check_sanity (BobguiTreeView *treeview)
{
  check_cursor (treeview);
  check_selection (treeview);
}

static gboolean
dance (gpointer treeview)
{
  static const DoStuffFunc funcs[] = {
    add_or_delete,
    add_or_delete,
    expand,
    collapse,
    select_,
    unselect,
    reset_model
  };
  guint i;

  i = g_random_int_range (0, G_N_ELEMENTS(funcs));

  funcs[i] (treeview);

  check_sanity (treeview);

  return G_SOURCE_CONTINUE;
}

static void
cursor_changed_cb (BobguiTreeView *treeview,
                   gpointer     unused)
{
  BobguiTreePath *path;
  BobguiTreeRowReference *ref;

  bobgui_tree_view_get_cursor (treeview, &path, NULL);
  if (path)
    {
      ref = bobgui_tree_row_reference_new (bobgui_tree_view_get_model (treeview),
                                        path);
      bobgui_tree_path_free (path);
    }
  else
    ref = NULL;
  g_object_set_data_full (G_OBJECT (treeview), "cursor", ref, (GDestroyNotify) bobgui_tree_row_reference_free);
}

static void
selection_list_free (gpointer list)
{
  g_list_free_full (list, (GDestroyNotify) bobgui_tree_row_reference_free);
}

static void
selection_changed_cb (BobguiTreeSelection *tree_selection,
                      gpointer          unused)
{
  GList *selected, *list;
  BobguiTreeModel *model;

  selected = bobgui_tree_selection_get_selected_rows (tree_selection, &model);

  for (list = selected; list; list = list->next)
    {
      BobguiTreePath *path = list->data;

      list->data = bobgui_tree_row_reference_new (model, path);
      bobgui_tree_path_free (path);
    }

  g_object_set_data_full (G_OBJECT (bobgui_tree_selection_get_tree_view (tree_selection)),
                          "selection",
                          selected,
                          selection_list_free);
}

static void
setup_sanity_checks (BobguiTreeView *treeview)
{
  g_signal_connect (treeview, "cursor-changed", G_CALLBACK (cursor_changed_cb), NULL);
  cursor_changed_cb (treeview, NULL);
  g_signal_connect (bobgui_tree_view_get_selection (treeview), "changed", G_CALLBACK (selection_changed_cb), NULL);
  selection_changed_cb (bobgui_tree_view_get_selection (treeview), NULL);
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int
main (int    argc,
      char **argv)
{
  BobguiWidget *window;
  BobguiWidget *sw;
  BobguiWidget *treeview;
  BobguiTreeModel *model;
  guint i;
  gboolean done = FALSE;
  
  bobgui_init ();

  if (g_getenv ("RTL"))
    bobgui_widget_set_default_direction (BOBGUI_TEXT_DIR_RTL);

  window = bobgui_window_new ();
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 430, 400);

  sw = bobgui_scrolled_window_new ();
  bobgui_widget_set_hexpand (sw, TRUE);
  bobgui_widget_set_vexpand (sw, TRUE);
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                  BOBGUI_POLICY_AUTOMATIC,
                                  BOBGUI_POLICY_AUTOMATIC);
  bobgui_window_set_child (BOBGUI_WINDOW (window), sw);

  model = BOBGUI_TREE_MODEL (bobgui_tree_store_new (1, G_TYPE_UINT));
  treeview = bobgui_tree_view_new_with_model (model);
  g_object_unref (model);
  setup_sanity_checks (BOBGUI_TREE_VIEW (treeview));
  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (treeview),
                                               0,
                                               "Counter",
                                               bobgui_cell_renderer_text_new (),
                                               "text", 0,
                                               NULL);
  for (i = 0; i < (MIN_ROWS + MAX_ROWS) / 2; i++)
    add (BOBGUI_TREE_VIEW (treeview));
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), treeview);

  bobgui_window_present (BOBGUI_WINDOW (window));

  g_idle_add (dance, treeview);
  
  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}

