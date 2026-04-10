/* BobguiTreePath tests.
 *
 * Copyright (C) 2011, Red Hat, Inc.
 * Authors: Matthias Clasen <mclasen@redhat.com>
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

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
test_append (void)
{
  BobguiTreePath *p;
  int i;
  int *indices;

  p = bobgui_tree_path_new ();
  for (i = 0; i < 100; i++)
    {
      g_assert_cmpint (bobgui_tree_path_get_depth (p), ==, i);
      bobgui_tree_path_append_index (p, i);
    }

  indices = bobgui_tree_path_get_indices (p);
  for (i = 0; i < 100; i++)
    g_assert_cmpint (indices[i], ==, i);

  bobgui_tree_path_free (p);
}

static void
test_prepend (void)
{
  BobguiTreePath *p;
  int i;
  int *indices;

  p = bobgui_tree_path_new ();
  for (i = 0; i < 100; i++)
    {
      g_assert_cmpint (bobgui_tree_path_get_depth (p), ==, i);
      bobgui_tree_path_prepend_index (p, i);
    }

  indices = bobgui_tree_path_get_indices (p);
  for (i = 0; i < 100; i++)
    g_assert_cmpint (indices[i], ==, 99 - i);

  bobgui_tree_path_free (p);
}

static void
test_to_string (void)
{
  const char *str = "0:1:2:3:4:5:6:7:8:9:10";
  BobguiTreePath *p;
  int *indices;
  char *s;
  int i;

  p = bobgui_tree_path_new_from_string (str);
  indices = bobgui_tree_path_get_indices (p);
  for (i = 0; i < 10; i++)
    g_assert_cmpint (indices[i], ==, i);
  s = bobgui_tree_path_to_string (p);
  g_assert_cmpstr (s, ==, str);

  bobgui_tree_path_free (p);
  g_free (s);
}

static void
test_from_indices (void)
{
  BobguiTreePath *p;
  int *indices;
  int i;

  p = bobgui_tree_path_new_from_indices (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1);
  g_assert_cmpint (bobgui_tree_path_get_depth (p), ==, 10);
  indices = bobgui_tree_path_get_indices (p);
  for (i = 0; i < 10; i++)
    g_assert_cmpint (indices[i], ==, i);
  bobgui_tree_path_free (p);
}

static void
test_first (void)
{
  BobguiTreePath *p;
  p = bobgui_tree_path_new_first ();
  g_assert_cmpint (bobgui_tree_path_get_depth (p), ==, 1);
  g_assert_cmpint (bobgui_tree_path_get_indices (p)[0], ==, 0);
  bobgui_tree_path_free (p);
}

static void
test_navigation (void)
{
  BobguiTreePath *p;
  BobguiTreePath *q;
  int *pi;
  int *qi;
  int i;
  gboolean res;

  p = bobgui_tree_path_new_from_indices (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1);
  q = bobgui_tree_path_copy (p);
  g_assert_true (bobgui_tree_path_compare (p, q) == 0);
  bobgui_tree_path_next (q);
  pi = bobgui_tree_path_get_indices (p);
  qi = bobgui_tree_path_get_indices (q);
  for (i = 0; i < 9; i++)
    g_assert_cmpint (pi[i], ==, qi[i]);
  g_assert_cmpint (qi[9], ==, pi[9] + 1);

  g_assert_false (bobgui_tree_path_is_ancestor (p, q));
  g_assert_false (bobgui_tree_path_is_ancestor (q, p));
  g_assert_false (bobgui_tree_path_is_descendant (p, q));
  g_assert_false (bobgui_tree_path_is_descendant (q, p));

  res = bobgui_tree_path_prev (q);
  g_assert_true (res);
  g_assert_true (bobgui_tree_path_compare (p, q) == 0);

  g_assert_false (bobgui_tree_path_is_ancestor (p, q));
  g_assert_false (bobgui_tree_path_is_ancestor (q, p));
  g_assert_false (bobgui_tree_path_is_descendant (p, q));
  g_assert_false (bobgui_tree_path_is_descendant (q, p));

  bobgui_tree_path_down (q);

  g_assert_true (bobgui_tree_path_compare (p, q) < 0);

  g_assert_true (bobgui_tree_path_is_ancestor (p, q));
  g_assert_false (bobgui_tree_path_is_ancestor (q, p));
  g_assert_false (bobgui_tree_path_is_descendant (p, q));
  g_assert_true (bobgui_tree_path_is_descendant (q, p));

  res = bobgui_tree_path_prev (q);
  g_assert_false (res);

  res = bobgui_tree_path_up (q);
  g_assert_true (res);
  g_assert_true (bobgui_tree_path_compare (p, q) == 0);

  g_assert_cmpint (bobgui_tree_path_get_depth (q), ==, 10);
  res = bobgui_tree_path_up (q);
  g_assert_true (res);
  g_assert_cmpint (bobgui_tree_path_get_depth (q), ==, 9);

  bobgui_tree_path_free (p);
  bobgui_tree_path_free (q);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/tree-path/append", test_append);
  g_test_add_func ("/tree-path/prepend", test_prepend);
  g_test_add_func ("/tree-path/to-string", test_to_string);
  g_test_add_func ("/tree-path/from-indices", test_from_indices);
  g_test_add_func ("/tree-path/first", test_first);
  g_test_add_func ("/tree-path/navigation", test_navigation);

  return g_test_run ();
}
