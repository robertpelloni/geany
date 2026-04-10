/*
 * Copyright © 2019 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include <locale.h>

#include <bobgui/bobgui.h>

static void
inc_counter (gpointer data)
{
  guint *counter = data;

  *counter += 1;
}

static void
test_property (void)
{
  GValue value = G_VALUE_INIT;
  BobguiExpression *expr;
  BobguiExpressionWatch *watch;
  BobguiStringFilter *filter;
  guint counter = 0;
  gboolean ret;

  filter = bobgui_string_filter_new (NULL);
  expr = bobgui_property_expression_new (BOBGUI_TYPE_STRING_FILTER, NULL, "search");
  watch = bobgui_expression_watch (expr, filter, inc_counter, &counter, NULL);

  ret = bobgui_expression_evaluate (expr, filter, &value);
  g_assert_true (ret);
  g_assert_cmpstr (g_value_get_string (&value), ==, NULL);
  g_value_unset (&value);

  bobgui_string_filter_set_search (filter, "Hello World");
  g_assert_cmpint (counter, ==, 1);
  counter = 0;

  ret = bobgui_expression_evaluate (expr, filter , &value);
  g_assert_true (ret);
  g_assert_cmpstr (g_value_get_string (&value), ==, "Hello World");
  g_value_unset (&value);

  bobgui_expression_watch_unwatch (watch);
  g_assert_cmpint (counter, ==, 0);

  bobgui_expression_unref (expr);
  g_object_unref (filter);
}

static void
test_interface_property (void)
{
  BobguiExpression *expr;

  expr = bobgui_property_expression_new (BOBGUI_TYPE_ORIENTABLE, NULL, "orientation");
  g_assert_cmpstr (bobgui_property_expression_get_pspec (expr)->name, ==, "orientation");
  bobgui_expression_unref (expr);
}

static char *
print_filter_info (BobguiStringFilter         *filter,
                   const char              *search,
                   gboolean                 ignore_case,
                   BobguiStringFilterMatchMode match_mode)
{
  g_assert_cmpstr (search, ==, bobgui_string_filter_get_search (filter));
  g_assert_cmpint (ignore_case, ==, bobgui_string_filter_get_ignore_case (filter));
  g_assert_cmpint (match_mode, ==, bobgui_string_filter_get_match_mode (filter));

  return g_strdup ("OK");
}

static void
test_cclosure (void)
{
  GValue value = G_VALUE_INIT;
  BobguiExpression *expr, *pexpr[3];
  BobguiExpressionWatch *watch;
  BobguiStringFilter *filter;
  guint counter = 0;
  gboolean ret;

  filter = BOBGUI_STRING_FILTER (bobgui_string_filter_new (NULL));
  pexpr[0] = bobgui_property_expression_new (BOBGUI_TYPE_STRING_FILTER, NULL, "search");
  pexpr[1] = bobgui_property_expression_new (BOBGUI_TYPE_STRING_FILTER, NULL, "ignore-case");
  pexpr[2] = bobgui_property_expression_new (BOBGUI_TYPE_STRING_FILTER, NULL, "match-mode");
  expr = bobgui_cclosure_expression_new (G_TYPE_STRING,
                                      NULL,
                                      3,
                                      pexpr,
                                      G_CALLBACK (print_filter_info),
                                      NULL,
                                      NULL);
  watch = bobgui_expression_watch (expr, filter, inc_counter, &counter, NULL);

  ret = bobgui_expression_evaluate (expr, filter, &value);
  g_assert_true (ret);
  g_assert_cmpstr (g_value_get_string (&value), ==, "OK");
  g_value_unset (&value);

  bobgui_string_filter_set_search (filter, "Hello World");
  g_assert_cmpint (counter, ==, 1);
  ret = bobgui_expression_evaluate (expr, filter , &value);
  g_assert_true (ret);
  g_assert_cmpstr (g_value_get_string (&value), ==, "OK");
  g_value_unset (&value);

  bobgui_string_filter_set_ignore_case (filter, FALSE);
  g_assert_cmpint (counter, ==, 2);
  ret = bobgui_expression_evaluate (expr, filter , &value);
  g_assert_true (ret);
  g_assert_cmpstr (g_value_get_string (&value), ==, "OK");
  g_value_unset (&value);

  bobgui_string_filter_set_search (filter, "Hello");
  bobgui_string_filter_set_ignore_case (filter, TRUE);
  bobgui_string_filter_set_match_mode (filter, BOBGUI_STRING_FILTER_MATCH_MODE_EXACT);
  g_assert_cmpint (counter, ==, 5);
  ret = bobgui_expression_evaluate (expr, filter , &value);
  g_assert_true (ret);
  g_assert_cmpstr (g_value_get_string (&value), ==, "OK");
  g_value_unset (&value);

  bobgui_expression_watch_unwatch (watch);
  g_assert_cmpint (counter, ==, 5);

  bobgui_expression_unref (expr);
  g_object_unref (filter);
}

static char *
make_string (void)
{
  return g_strdup ("Hello");
}

static void
test_closure (void)
{
  GValue value = G_VALUE_INIT;
  BobguiExpression *expr;
  GClosure *closure;
  gboolean ret;

  closure = g_cclosure_new (G_CALLBACK (make_string), NULL, NULL);
  expr = bobgui_closure_expression_new (G_TYPE_STRING, closure, 0, NULL);
  ret = bobgui_expression_evaluate (expr, NULL, &value);
  g_assert_true (ret);
  g_assert_cmpstr (g_value_get_string (&value), ==, "Hello");
  g_value_unset (&value);

  bobgui_expression_unref (expr);
}

static void
test_constant (void)
{
  BobguiExpression *expr;
  GValue value = G_VALUE_INIT;
  const GValue *v;
  gboolean res;

  expr = bobgui_constant_expression_new (G_TYPE_INT, 22);
  g_assert_cmpint (bobgui_expression_get_value_type (expr), ==, G_TYPE_INT);
  g_assert_true (bobgui_expression_is_static (expr));

  res = bobgui_expression_evaluate (expr, NULL, &value);
  g_assert_true (res);
  g_assert_cmpint (g_value_get_int (&value), ==, 22);

  v = bobgui_constant_expression_get_value (expr);
  g_assert_cmpint (g_value_get_int (v), ==, 22);

  bobgui_expression_unref (expr);
}

/* Test that object expressions fail to evaluate when
 * the object is gone.
 */
static void
test_object (void)
{
  BobguiExpression *expr;
  GObject *obj;
  GValue value = G_VALUE_INIT;
  gboolean res;
  GObject *o;

  obj = G_OBJECT (bobgui_string_filter_new (NULL));

  expr = bobgui_object_expression_new (obj);
  g_assert_true (!bobgui_expression_is_static (expr));
  g_assert_cmpint (bobgui_expression_get_value_type (expr), ==, BOBGUI_TYPE_STRING_FILTER);

  res = bobgui_expression_evaluate (expr, NULL, &value);
  g_assert_true (res);
  g_assert_true (g_value_get_object (&value) == obj);
  g_value_unset (&value);

  o = bobgui_object_expression_get_object (expr);
  g_assert_true (o == obj);

  g_clear_object (&obj);
  res = bobgui_expression_evaluate (expr, NULL, &value);
  g_assert_false (res);

  bobgui_expression_unref (expr);
}

/* Some basic tests that nested expressions work; in particular test
 * that watching works when things change deeper in the expression tree
 *
 * The setup we use is BobguiFilterListModel -> BobguiFilter -> "search" property,
 * which gives us an expression tree like
 *
 * BobguiPropertyExpression "search"
 *    -> BobguiPropertyExpression "filter"
 *         -> BobguiObjectExpression listmodel
 *
 * We test setting both the search property and the filter property.
 */
static void
test_nested (void)
{
  BobguiExpression *list_expr;
  BobguiExpression *filter_expr;
  BobguiExpression *expr;
  BobguiStringFilter *filter;
  GListModel *list;
  BobguiFilterListModel *filtered;
  GValue value = G_VALUE_INIT;
  gboolean res;
  BobguiExpressionWatch *watch;
  guint counter = 0;

  filter = bobgui_string_filter_new (NULL);
  bobgui_string_filter_set_search (filter, "word");
  list = G_LIST_MODEL (g_list_store_new (G_TYPE_OBJECT));
  filtered = bobgui_filter_list_model_new (list, g_object_ref (BOBGUI_FILTER (filter)));

  list_expr = bobgui_object_expression_new (G_OBJECT (filtered));
  filter_expr = bobgui_property_expression_new (BOBGUI_TYPE_FILTER_LIST_MODEL, list_expr, "filter");
  expr = bobgui_property_expression_new (BOBGUI_TYPE_STRING_FILTER, filter_expr, "search");

  g_assert_true (!bobgui_expression_is_static (expr));
  g_assert_cmpint (bobgui_expression_get_value_type (expr), ==, G_TYPE_STRING);

  res = bobgui_expression_evaluate (expr, NULL, &value);
  g_assert_true (res);
  g_assert_cmpstr (g_value_get_string (&value), ==, "word");
  g_value_unset (&value);

  watch = bobgui_expression_watch (expr, NULL, inc_counter, &counter, NULL);
  bobgui_string_filter_set_search (BOBGUI_STRING_FILTER (filter), "salad");
  g_assert_cmpint (counter, ==, 1);
  counter = 0;

  res = bobgui_expression_evaluate (expr, NULL, &value);
  g_assert_true (res);
  g_assert_cmpstr (g_value_get_string (&value), ==, "salad");
  g_value_unset (&value);

  bobgui_filter_list_model_set_filter (filtered, BOBGUI_FILTER (filter));
  g_assert_cmpint (counter, ==, 0);

  g_clear_object (&filter);
  filter = bobgui_string_filter_new (NULL);
  bobgui_string_filter_set_search (filter, "salad");
  bobgui_filter_list_model_set_filter (filtered, BOBGUI_FILTER (filter));
  g_assert_cmpint (counter, ==, 1);
  counter = 0;

  res = bobgui_expression_evaluate (expr, NULL, &value);
  g_assert_true (res);
  g_assert_cmpstr (g_value_get_string (&value), ==, "salad");
  g_value_unset (&value);

  bobgui_string_filter_set_search (filter, "bar");
  g_assert_cmpint (counter, ==, 1);
  counter = 0;

  res = bobgui_expression_evaluate (expr, NULL, &value);
  g_assert_true (res);
  g_assert_cmpstr (g_value_get_string (&value), ==, "bar");
  g_value_unset (&value);

  bobgui_filter_list_model_set_filter (filtered, NULL);
  g_assert_cmpint (counter, ==, 1);
  counter = 0;

  res = bobgui_expression_evaluate (expr, NULL, &value);
  g_assert_false (res);

  bobgui_expression_watch_unwatch (watch);
  g_assert_cmpint (counter, ==, 0);

  g_object_unref (filtered);
  bobgui_expression_unref (expr);
}

/* This test uses the same setup as the last test, but
 * passes the filter as the "this" object when creating
 * the watch.
 *
 * So when we set a new filter and the old one gets desroyed,
 * the watch should invalidate itself because its this object
 * is gone.
 */
static void
test_nested_this_destroyed (void)
{
  BobguiExpression *list_expr;
  BobguiExpression *filter_expr;
  BobguiExpression *expr;
  BobguiStringFilter *filter;
  GListModel *list;
  BobguiFilterListModel *filtered;
  GValue value = G_VALUE_INIT;
  gboolean res;
  BobguiExpressionWatch *watch;
  guint counter = 0;

  filter = bobgui_string_filter_new (NULL);
  bobgui_string_filter_set_search (filter, "word");
  list = G_LIST_MODEL (g_list_store_new (G_TYPE_OBJECT));
  filtered = bobgui_filter_list_model_new (list, g_object_ref (BOBGUI_FILTER (filter)));

  list_expr = bobgui_object_expression_new (G_OBJECT (filtered));
  filter_expr = bobgui_property_expression_new (BOBGUI_TYPE_FILTER_LIST_MODEL, list_expr, "filter");
  expr = bobgui_property_expression_new (BOBGUI_TYPE_STRING_FILTER, filter_expr, "search");

  watch = bobgui_expression_watch (expr, filter, inc_counter, &counter, NULL);
  bobgui_expression_watch_ref (watch);
  res = bobgui_expression_watch_evaluate (watch, &value);
  g_assert_true (res);
  g_assert_cmpstr (g_value_get_string (&value), ==, "word");
  g_value_unset (&value);

  g_clear_object (&filter);
  g_assert_cmpint (counter, ==, 0);

  filter = bobgui_string_filter_new (NULL);
  bobgui_string_filter_set_search (filter, "salad");
  bobgui_filter_list_model_set_filter (filtered, BOBGUI_FILTER (filter));
  g_assert_cmpint (counter, ==, 1);
  counter = 0;

  res = bobgui_expression_watch_evaluate (watch, &value);
  g_assert_false (res);

  bobgui_string_filter_set_search (filter, "bar");
  g_assert_cmpint (counter, ==, 0);

  bobgui_filter_list_model_set_filter (filtered, NULL);
  g_assert_cmpint (counter, ==, 0);

  res = bobgui_expression_watch_evaluate (watch, &value);
  g_assert_false (res);
  g_assert_false (G_IS_VALUE (&value));

  /* We unwatch on purpose here to make sure it doesn't do bad things. */
  bobgui_expression_watch_unwatch (watch);
  bobgui_expression_watch_unref (watch);
  g_assert_cmpint (counter, ==, 0);

  g_object_unref (filtered);
  g_object_unref (filter);
  bobgui_expression_unref (expr);
}

/* Test that property expressions fail to evaluate if the
 * expression evaluates to an object of the wrong type
 */
static void
test_type_mismatch (void)
{
  BobguiFilter *filter;
  BobguiExpression *expr;
  GValue value = G_VALUE_INIT;
  gboolean res;

  filter = BOBGUI_FILTER (bobgui_any_filter_new ());

  expr = bobgui_property_expression_new (BOBGUI_TYPE_STRING_FILTER, bobgui_constant_expression_new (BOBGUI_TYPE_ANY_FILTER, filter), "search");

  res = bobgui_expression_evaluate (expr, NULL, &value);
  g_assert_false (res);
  g_assert_false (G_IS_VALUE (&value));

  bobgui_expression_unref (expr);
  g_object_unref (filter);
}

/* Some basic tests around 'this' */
static void
test_this (void)
{
  BobguiStringFilter *filter;
  BobguiStringFilter *filter2;
  BobguiExpression *expr;
  GValue value = G_VALUE_INIT;
  gboolean res;

  expr = bobgui_property_expression_new (BOBGUI_TYPE_STRING_FILTER, NULL, "search");

  filter = bobgui_string_filter_new (NULL);
  bobgui_string_filter_set_search (filter, "word");

  filter2 = bobgui_string_filter_new (NULL);
  bobgui_string_filter_set_search (filter2, "sausage");

  res = bobgui_expression_evaluate (expr, filter, &value);
  g_assert_true (res);
  g_assert_cmpstr (g_value_get_string (&value), ==, "word");
  g_value_unset (&value);

  res = bobgui_expression_evaluate (expr, filter2, &value);
  g_assert_true (res);
  g_assert_cmpstr (g_value_get_string (&value), ==, "sausage");
  g_value_unset (&value);

  bobgui_expression_unref (expr);
  g_object_unref (filter2);
  g_object_unref (filter);
}

/* Check that even for static expressions, watches can be created
 * and destroying the "this" argument does invalidate the
 * expression.
 */
static void
test_constant_watch_this_destroyed (void)
{
  BobguiExpression *expr;
  GObject *this;
  guint counter = 0;

  this = g_object_new (G_TYPE_OBJECT, NULL);
  expr = bobgui_constant_expression_new (G_TYPE_INT, 42);
  bobgui_expression_watch (expr, this, inc_counter, &counter, NULL);
  g_assert_cmpint (counter, ==, 0);

  g_clear_object (&this);
  g_assert_cmpint (counter, ==, 1);

  bobgui_expression_unref (expr);
}

/* Basic test of bobgui_expression_bind */
static void
test_bind (void)
{
  BobguiStringFilter *target;
  BobguiStringFilter *source;
  BobguiExpression *expr;
  BobguiExpressionWatch *watch;
  GValue value = G_VALUE_INIT;
  gboolean res;

  expr = bobgui_property_expression_new (BOBGUI_TYPE_STRING_FILTER, NULL, "search");

  target = bobgui_string_filter_new (NULL);
  bobgui_string_filter_set_search (target, "word");
  g_assert_cmpstr (bobgui_string_filter_get_search (target), ==, "word");

  source = bobgui_string_filter_new (NULL);
  bobgui_string_filter_set_search (source, "sausage");

  watch = bobgui_expression_bind (expr, target, "search", source);
  bobgui_expression_watch_ref (watch);
  g_assert_cmpstr (bobgui_string_filter_get_search (target), ==, "sausage");

  bobgui_string_filter_set_search (source, "salad");
  g_assert_cmpstr (bobgui_string_filter_get_search (target), ==, "salad");
  res = bobgui_expression_watch_evaluate (watch, &value);
  g_assert_true (res);
  g_assert_cmpstr (g_value_get_string (&value), ==, "salad"); 
  g_value_unset (&value);

  g_object_unref (source);
  g_assert_cmpstr (bobgui_string_filter_get_search (target), ==, "salad");
  res = bobgui_expression_watch_evaluate (watch, &value);
  g_assert_false (res);
  g_assert_false (G_IS_VALUE (&value));

  g_object_unref (target);
  bobgui_expression_watch_unref (watch);
}

/* Another test of bind, this time we watch ourselves */
static void
test_bind_self (void)
{
  BobguiStringFilter *filter;
  BobguiExpression *expr;

  expr = bobgui_property_expression_new (BOBGUI_TYPE_STRING_FILTER,
                                      NULL,
                                      "ignore-case");

  filter = bobgui_string_filter_new (NULL);
  bobgui_string_filter_set_search (filter, "word");
  g_assert_cmpstr (bobgui_string_filter_get_search (filter), ==, "word");

  bobgui_expression_bind (expr, filter, "search", filter);
  g_assert_cmpstr (bobgui_string_filter_get_search (filter), ==, "TRUE");

  g_object_unref (filter);
}

/* Test bind does the right memory management if the target's
 * dispose() kills the source */
static void
test_bind_child (void)
{
  BobguiStringFilter *filter;
  BobguiFilterListModel *child, *target;
  BobguiExpression *expr;

  expr = bobgui_property_expression_new (BOBGUI_TYPE_FILTER_LIST_MODEL,
                                      NULL,
                                      "filter");

  filter = bobgui_string_filter_new (NULL);
  child = bobgui_filter_list_model_new (NULL, BOBGUI_FILTER (filter));
  target = bobgui_filter_list_model_new (G_LIST_MODEL (child), NULL);

  bobgui_expression_bind (expr, target, "filter", child);
  g_assert_true (bobgui_filter_list_model_get_filter (child) == bobgui_filter_list_model_get_filter (target));

  filter = bobgui_string_filter_new (NULL);
  bobgui_filter_list_model_set_filter (child, BOBGUI_FILTER (filter));
  g_assert_true (BOBGUI_FILTER (filter) == bobgui_filter_list_model_get_filter (target));
  g_assert_true (bobgui_filter_list_model_get_filter (child) == bobgui_filter_list_model_get_filter (target));
  g_object_unref (filter);

  g_object_unref (target);
}

/* Another test of bobgui_expression_bind that exercises the subwatch code paths */
static void
test_nested_bind (void)
{
  BobguiStringFilter *filter;
  BobguiStringFilter *filter2;
  BobguiStringFilter *filter3;
  GListModel *list;
  BobguiFilterListModel *filtered;
  BobguiExpression *expr;
  BobguiExpression *filter_expr;
  gboolean res;
  GValue value = G_VALUE_INIT;

  filter2 = bobgui_string_filter_new (NULL);
  bobgui_string_filter_set_search (filter2, "sausage");

  list = G_LIST_MODEL (g_list_store_new (G_TYPE_OBJECT));
  filtered = bobgui_filter_list_model_new (list, g_object_ref (BOBGUI_FILTER (filter2)));

  filter_expr = bobgui_property_expression_new (BOBGUI_TYPE_FILTER_LIST_MODEL,
                                             bobgui_object_expression_new (G_OBJECT (filtered)),
                                             "filter");
  expr = bobgui_property_expression_new (BOBGUI_TYPE_STRING_FILTER, bobgui_expression_ref (filter_expr), "search");

  filter = bobgui_string_filter_new (NULL);
  bobgui_string_filter_set_search (filter, "word");
  g_assert_cmpstr (bobgui_string_filter_get_search (filter), ==, "word");

  bobgui_expression_bind (bobgui_expression_ref (expr), filter, "search", NULL);

  bobgui_string_filter_set_search (filter2, "sausage");
  g_assert_cmpstr (bobgui_string_filter_get_search (filter), ==, "sausage");

  filter3 = bobgui_string_filter_new (NULL);
  bobgui_string_filter_set_search (filter3, "banana");
  bobgui_filter_list_model_set_filter (filtered, BOBGUI_FILTER (filter3));

  /* check that the expressions evaluate correctly */
  res = bobgui_expression_evaluate (filter_expr, NULL, &value);
  g_assert_true (res);
  g_assert_true (g_value_get_object (&value) == filter3);
  g_value_unset (&value);

  res = bobgui_expression_evaluate (expr, NULL, &value);
  g_assert_true (res);
  g_assert_cmpstr (g_value_get_string (&value), ==, "banana");
  g_value_unset (&value);

  /* and the bind too */
  g_assert_cmpstr (bobgui_string_filter_get_search (filter), ==, "banana");

  g_object_unref (filter);
  g_object_unref (filter2);
  g_object_unref (filter3);
  g_object_unref (filtered);

  bobgui_expression_unref (expr);
  bobgui_expression_unref (filter_expr);
}

static char *
some_cb (gpointer    this,
         const char *search,
         gboolean    ignore_case,
         gpointer    data)
{
  if (!search)
    return NULL;

  if (ignore_case)
    return g_utf8_strdown (search, -1);
  else
    return g_strdup (search);
}

/* Test that things work as expected when the same object is used multiple times in an
 * expression or its subexpressions.
 */
static void
test_double_bind (void)
{
  BobguiStringFilter *filter1;
  BobguiStringFilter *filter2;
  BobguiExpression *expr;
  BobguiExpression *filter_expr;
  BobguiExpression *params[2];

  filter1 = bobgui_string_filter_new (NULL);
  filter2 = bobgui_string_filter_new (NULL);

  filter_expr = bobgui_object_expression_new (G_OBJECT (filter1));

  params[0] = bobgui_property_expression_new (BOBGUI_TYPE_STRING_FILTER, bobgui_expression_ref (filter_expr), "search");
  params[1] = bobgui_property_expression_new (BOBGUI_TYPE_STRING_FILTER, bobgui_expression_ref (filter_expr), "ignore-case");
  expr = bobgui_cclosure_expression_new (G_TYPE_STRING,
                                      NULL,
                                      2, params,
                                      (GCallback)some_cb,
                                      NULL, NULL);

  bobgui_expression_bind (bobgui_expression_ref (expr), filter2, "search", NULL);

  bobgui_string_filter_set_search (filter1, "Banana");
  g_assert_cmpstr (bobgui_string_filter_get_search (filter2), ==, "banana");

  bobgui_string_filter_set_ignore_case (filter1, FALSE);
  g_assert_cmpstr (bobgui_string_filter_get_search (filter2), ==, "Banana");

  bobgui_expression_unref (expr);
  bobgui_expression_unref (filter_expr);

  g_object_unref (filter1);
  g_object_unref (filter2);
}

/* Test that having multiple binds on the same object works. */
static void
test_binds (void)
{
  BobguiStringFilter *filter1;
  BobguiStringFilter *filter2;
  BobguiStringFilter *filter3;
  BobguiExpression *expr;
  BobguiExpression *expr2;
  BobguiExpression *filter1_expr;
  BobguiExpression *filter2_expr;
  BobguiExpression *params[2];

  filter1 = bobgui_string_filter_new (NULL);
  filter2 = bobgui_string_filter_new (NULL);
  filter3 = bobgui_string_filter_new (NULL);

  filter1_expr = bobgui_object_expression_new (G_OBJECT (filter1));
  filter2_expr = bobgui_object_expression_new (G_OBJECT (filter2));

  params[0] = bobgui_property_expression_new (BOBGUI_TYPE_STRING_FILTER, bobgui_expression_ref (filter1_expr), "search");
  params[1] = bobgui_property_expression_new (BOBGUI_TYPE_STRING_FILTER, bobgui_expression_ref (filter2_expr), "ignore-case");
  expr = bobgui_cclosure_expression_new (G_TYPE_STRING,
                                      NULL,
                                      2, params,
                                      (GCallback)some_cb,
                                      NULL, NULL);

  expr2 = bobgui_property_expression_new (BOBGUI_TYPE_STRING_FILTER, bobgui_expression_ref (filter2_expr), "ignore-case");

  g_assert_true (bobgui_property_expression_get_expression (expr2) == filter2_expr);
  g_assert_cmpstr (bobgui_property_expression_get_pspec (expr2)->name, ==, "ignore-case");

  bobgui_expression_bind (bobgui_expression_ref (expr), filter3, "search", NULL);
  bobgui_expression_bind (bobgui_expression_ref (expr2), filter3, "ignore-case", NULL);

  bobgui_string_filter_set_search (filter1, "Banana");
  g_assert_cmpstr (bobgui_string_filter_get_search (filter3), ==, "banana");
  g_assert_true (bobgui_string_filter_get_ignore_case (filter3));

  bobgui_string_filter_set_ignore_case (filter2, FALSE);
  g_assert_cmpstr (bobgui_string_filter_get_search (filter3), ==, "Banana");
  g_assert_false (bobgui_string_filter_get_ignore_case (filter3));

  /* invalidate the first bind */
  g_object_unref (filter1);

  bobgui_string_filter_set_ignore_case (filter2, TRUE);
  g_assert_cmpstr (bobgui_string_filter_get_search (filter3), ==, "Banana");
  g_assert_true (bobgui_string_filter_get_ignore_case (filter3));

  bobgui_expression_unref (expr);
  bobgui_expression_unref (expr2);
  bobgui_expression_unref (filter1_expr);
  bobgui_expression_unref (filter2_expr);

  g_object_unref (filter2);
  g_object_unref (filter3);
}

/* test that binds work ok with object expressions */
static void
test_bind_object (void)
{
  BobguiStringFilter *filter;
  GListStore *store;
  BobguiFilterListModel *model;
  BobguiExpression *expr;

  filter = bobgui_string_filter_new (NULL);
  store = g_list_store_new (G_TYPE_OBJECT);
  model = bobgui_filter_list_model_new (G_LIST_MODEL (store), NULL);

  expr = bobgui_object_expression_new (G_OBJECT (filter));

  bobgui_expression_bind (bobgui_expression_ref (expr), model, "filter", NULL);

  g_assert_true (bobgui_filter_list_model_get_filter (model) == BOBGUI_FILTER (filter));

  g_object_unref (filter);

  g_assert_true (bobgui_filter_list_model_get_filter (model) == BOBGUI_FILTER (filter));

  bobgui_expression_unref (expr);
  g_object_unref (model);
}

static void
test_value (void)
{
  GValue value = G_VALUE_INIT;
  BobguiExpression *expr;

  expr = bobgui_constant_expression_new (G_TYPE_INT, 22);

  g_value_init (&value, BOBGUI_TYPE_EXPRESSION);
  bobgui_value_take_expression (&value, expr);
  g_assert_true (G_VALUE_TYPE (&value) == BOBGUI_TYPE_EXPRESSION);

  expr = bobgui_value_dup_expression (&value);
  bobgui_expression_unref (expr);

  expr = bobgui_constant_expression_new (G_TYPE_INT, 23);
  bobgui_value_set_expression (&value, expr);
  bobgui_expression_unref (expr);

  g_value_unset (&value);
}

static void
test_try (void)
{
  BobguiExpression *root_expr;
  BobguiExpression *expressions[2];
  BobguiExpression *try_expr;
  BobguiLabel *label;
  BobguiWindow *window;

  root_expr = bobgui_property_expression_new (BOBGUI_TYPE_LABEL, NULL, "root");
  expressions[0] = bobgui_property_expression_new (BOBGUI_TYPE_WINDOW, root_expr, "title");
  expressions[1] = bobgui_constant_expression_new (G_TYPE_STRING, "fallback");
  try_expr = bobgui_try_expression_new (2, expressions);

  label = BOBGUI_LABEL (bobgui_label_new (NULL));
  g_object_ref_sink (label);
  bobgui_expression_bind (try_expr, label, "label", label);
  g_assert_cmpstr (bobgui_label_get_label (label), ==, "fallback");

  window = BOBGUI_WINDOW (bobgui_window_new ());
  bobgui_window_set_title (window, "window");
  bobgui_window_set_child (window, BOBGUI_WIDGET (label));
  g_assert_cmpstr (bobgui_label_get_label (label), ==, "window");

  bobgui_window_set_title (window, "still window");
  g_assert_cmpstr (bobgui_label_get_label (label), ==, "still window");

  bobgui_window_set_child (window, NULL);
  g_assert_cmpstr (bobgui_label_get_label (label), ==, "fallback");

  g_object_unref (label);
  bobgui_window_destroy (window);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);
  setlocale (LC_ALL, "C");

  g_test_add_func ("/expression/property", test_property);
  g_test_add_func ("/expression/interface-property", test_interface_property);
  g_test_add_func ("/expression/cclosure", test_cclosure);
  g_test_add_func ("/expression/closure", test_closure);
  g_test_add_func ("/expression/constant", test_constant);
  g_test_add_func ("/expression/constant-watch-this-destroyed", test_constant_watch_this_destroyed);
  g_test_add_func ("/expression/object", test_object);
  g_test_add_func ("/expression/nested", test_nested);
  g_test_add_func ("/expression/nested-this-destroyed", test_nested_this_destroyed);
  g_test_add_func ("/expression/type-mismatch", test_type_mismatch);
  g_test_add_func ("/expression/this", test_this);
  g_test_add_func ("/expression/bind", test_bind);
  g_test_add_func ("/expression/bind-self", test_bind_self);
  g_test_add_func ("/expression/bind-child", test_bind_child);
  g_test_add_func ("/expression/nested-bind", test_nested_bind);
  g_test_add_func ("/expression/double-bind", test_double_bind);
  g_test_add_func ("/expression/binds", test_binds);
  g_test_add_func ("/expression/bind-object", test_bind_object);
  g_test_add_func ("/expression/value", test_value);
  g_test_add_func ("/expression/try", test_try);

  return g_test_run ();
}
