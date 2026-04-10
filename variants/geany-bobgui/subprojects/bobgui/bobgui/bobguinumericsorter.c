/*
 * Copyright © 2019 Matthias Clasen
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
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */

#include "config.h"

#include "bobguinumericsorter.h"

#include "bobguisorterprivate.h"
#include "bobguitypebuiltins.h"

#include <math.h>

/**
 * BobguiNumericSorter:
 *
 * Sorts items numerically.
 *
 * To obtain the numbers to compare, this sorter evaluates a
 * [class@Bobgui.Expression].
 */

struct _BobguiNumericSorter
{
  BobguiSorter parent_instance;

  BobguiSortType sort_order;

  BobguiExpression *expression;
};

enum {
  PROP_0,
  PROP_EXPRESSION,
  PROP_SORT_ORDER,
  NUM_PROPERTIES
};

G_DEFINE_TYPE (BobguiNumericSorter, bobgui_numeric_sorter, BOBGUI_TYPE_SORTER)

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

#define DO_COMPARISON(_result, _type, _getter, _order) G_STMT_START{ \
  _type num1 = _getter (&value1); \
  _type num2 = _getter (&value2); \
\
  if (num1 < num2) \
    _result = _order == BOBGUI_SORT_ASCENDING ? BOBGUI_ORDERING_SMALLER : BOBGUI_ORDERING_LARGER; \
  else if (num1 > num2) \
    _result = _order == BOBGUI_SORT_ASCENDING ? BOBGUI_ORDERING_LARGER : BOBGUI_ORDERING_SMALLER; \
  else \
    _result = BOBGUI_ORDERING_EQUAL; \
}G_STMT_END

typedef struct _BobguiNumericSortKeys BobguiNumericSortKeys;
struct _BobguiNumericSortKeys
{
  BobguiSortKeys keys;

  BobguiExpression *expression;
};

static void
bobgui_numeric_sort_keys_free (BobguiSortKeys *keys)
{
  BobguiNumericSortKeys *self = (BobguiNumericSortKeys *) keys;

  bobgui_expression_unref (self->expression);
  g_free (self);
}

#define COMPARE_FUNC(type, name, _a, _b) \
static int \
bobgui_ ## type ## _sort_keys_compare_ ## name (gconstpointer a, \
                                             gconstpointer b, \
                                             gpointer      unused) \
{ \
  type num1 = *(type *) _a; \
  type num2 = *(type *) _b; \
\
  if (num1 < num2) \
    return BOBGUI_ORDERING_SMALLER; \
  else if (num1 > num2) \
    return BOBGUI_ORDERING_LARGER; \
  else \
    return BOBGUI_ORDERING_EQUAL; \
}
#define COMPARE_FUNCS(type) \
  COMPARE_FUNC(type, ascending, a, b) \
  COMPARE_FUNC(type, descending, b, a)

#define FLOAT_COMPARE_FUNC(type, name, _a, _b) \
static int \
bobgui_ ## type ## _sort_keys_compare_ ## name (gconstpointer a, \
                                             gconstpointer b, \
                                             gpointer      unused) \
{ \
  type num1 = *(type *) _a; \
  type num2 = *(type *) _b; \
\
  if (isnan (num1) && isnan (num2)) \
    return BOBGUI_ORDERING_EQUAL; \
  else if (isnan (num1)) \
    return BOBGUI_ORDERING_LARGER; \
  else if (isnan (num2)) \
    return BOBGUI_ORDERING_SMALLER; \
  else if (num1 < num2) \
    return BOBGUI_ORDERING_SMALLER; \
  else if (num1 > num2) \
    return BOBGUI_ORDERING_LARGER; \
  else \
    return BOBGUI_ORDERING_EQUAL; \
}
#define FLOAT_COMPARE_FUNCS(type) \
  FLOAT_COMPARE_FUNC(type, ascending, a, b) \
  FLOAT_COMPARE_FUNC(type, descending, b, a)

COMPARE_FUNCS(char)
COMPARE_FUNCS(guchar)
COMPARE_FUNCS(int)
COMPARE_FUNCS(guint)
FLOAT_COMPARE_FUNCS(float)
FLOAT_COMPARE_FUNCS(double)
COMPARE_FUNCS(long)
COMPARE_FUNCS(gulong)
COMPARE_FUNCS(gint64)
COMPARE_FUNCS(guint64)

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

#define NUMERIC_SORT_KEYS(TYPE, key_type, type, default_value) \
static void \
bobgui_ ## type ## _sort_keys_init_key (BobguiSortKeys *keys, \
                                     gpointer     item, \
                                     gpointer     key_memory) \
{ \
  BobguiNumericSortKeys *self = (BobguiNumericSortKeys *) keys; \
  key_type *key = (key_type *) key_memory; \
  GValue value = G_VALUE_INIT; \
\
  if (bobgui_expression_evaluate (self->expression, item, &value)) \
    *key = g_value_get_ ## type (&value); \
  else \
    *key = default_value; \
\
  g_value_unset (&value); \
} \
\
static gboolean \
bobgui_ ## type ## _sort_keys_is_compatible (BobguiSortKeys *keys, \
                                          BobguiSortKeys *other); \
\
static const BobguiSortKeysClass BOBGUI_ASCENDING_ ## TYPE ## _SORT_KEYS_CLASS = \
{ \
  bobgui_numeric_sort_keys_free, \
  bobgui_ ## key_type ## _sort_keys_compare_ascending, \
  bobgui_ ## type ## _sort_keys_is_compatible, \
  bobgui_ ## type ## _sort_keys_init_key, \
  NULL \
}; \
\
static const BobguiSortKeysClass BOBGUI_DESCENDING_ ## TYPE ## _SORT_KEYS_CLASS = \
{ \
  bobgui_numeric_sort_keys_free, \
  bobgui_ ## key_type ## _sort_keys_compare_descending, \
  bobgui_ ## type ## _sort_keys_is_compatible, \
  bobgui_ ## type ## _sort_keys_init_key, \
  NULL \
}; \
\
static gboolean \
bobgui_ ## type ## _sort_keys_is_compatible (BobguiSortKeys *keys, \
                                          BobguiSortKeys *other) \
{ \
  BobguiNumericSorter *self = (BobguiNumericSorter *) keys; \
  BobguiNumericSorter *compare = (BobguiNumericSorter *) other; \
\
  if (other->klass != &BOBGUI_ASCENDING_ ## TYPE ## _SORT_KEYS_CLASS && \
      other->klass != &BOBGUI_DESCENDING_ ## TYPE ## _SORT_KEYS_CLASS) \
    return FALSE; \
\
  return self->expression == compare->expression; \
}

NUMERIC_SORT_KEYS(BOOLEAN, char, boolean, FALSE)
NUMERIC_SORT_KEYS(CHAR, char, char, G_MININT8)
NUMERIC_SORT_KEYS(UCHAR, guchar, uchar, G_MAXUINT8)
NUMERIC_SORT_KEYS(INT, int, int, G_MININT)
NUMERIC_SORT_KEYS(UINT, guint, uint, G_MAXUINT)
NUMERIC_SORT_KEYS(FLOAT, float, float, NAN)
NUMERIC_SORT_KEYS(DOUBLE, double, double, NAN)
NUMERIC_SORT_KEYS(LONG, long, long, G_MINLONG)
NUMERIC_SORT_KEYS(ULONG, gulong, ulong, G_MAXLONG)
NUMERIC_SORT_KEYS(INT64, gint64, int64, G_MININT64)
NUMERIC_SORT_KEYS(UINT64, guint64, uint64, G_MAXUINT64)

G_GNUC_END_IGNORE_DEPRECATIONS

static BobguiSortKeys *
bobgui_numeric_sort_keys_new (BobguiNumericSorter *self)
{
  BobguiNumericSortKeys *result;

  if (self->expression == NULL)
    return bobgui_sort_keys_new_equal ();

  switch (bobgui_expression_get_value_type (self->expression))
    {
    case G_TYPE_BOOLEAN:
      result = bobgui_sort_keys_new (BobguiNumericSortKeys,
                                  self->sort_order == BOBGUI_SORT_ASCENDING
                                                      ? &BOBGUI_ASCENDING_BOOLEAN_SORT_KEYS_CLASS
                                                      : &BOBGUI_DESCENDING_BOOLEAN_SORT_KEYS_CLASS,
                                  sizeof (char),
                                  G_ALIGNOF (char));
      break;

    case G_TYPE_CHAR:
      result = bobgui_sort_keys_new (BobguiNumericSortKeys,
                                  self->sort_order == BOBGUI_SORT_ASCENDING
                                                      ? &BOBGUI_ASCENDING_CHAR_SORT_KEYS_CLASS
                                                      : &BOBGUI_DESCENDING_CHAR_SORT_KEYS_CLASS,
                                  sizeof (char),
                                  G_ALIGNOF (char));
      break;

    case G_TYPE_UCHAR:
      result = bobgui_sort_keys_new (BobguiNumericSortKeys,
                                  self->sort_order == BOBGUI_SORT_ASCENDING
                                                      ? &BOBGUI_ASCENDING_UCHAR_SORT_KEYS_CLASS
                                                      : &BOBGUI_DESCENDING_UCHAR_SORT_KEYS_CLASS,
                                  sizeof (guchar),
                                  G_ALIGNOF (guchar));
      break;

    case G_TYPE_INT:
      result = bobgui_sort_keys_new (BobguiNumericSortKeys,
                                  self->sort_order == BOBGUI_SORT_ASCENDING
                                                      ? &BOBGUI_ASCENDING_INT_SORT_KEYS_CLASS
                                                      : &BOBGUI_DESCENDING_INT_SORT_KEYS_CLASS,
                                  sizeof (int),
                                  G_ALIGNOF (int));
      break;

    case G_TYPE_UINT:
      result = bobgui_sort_keys_new (BobguiNumericSortKeys,
                                  self->sort_order == BOBGUI_SORT_ASCENDING
                                                      ? &BOBGUI_ASCENDING_UINT_SORT_KEYS_CLASS
                                                      : &BOBGUI_DESCENDING_UINT_SORT_KEYS_CLASS,
                                  sizeof (guint),
                                  G_ALIGNOF (guint));
      break;

    case G_TYPE_FLOAT:
      result = bobgui_sort_keys_new (BobguiNumericSortKeys,
                                  self->sort_order == BOBGUI_SORT_ASCENDING
                                                      ? &BOBGUI_ASCENDING_FLOAT_SORT_KEYS_CLASS
                                                      : &BOBGUI_DESCENDING_FLOAT_SORT_KEYS_CLASS,
                                  sizeof (float),
                                  G_ALIGNOF (float));
      break;

    case G_TYPE_DOUBLE:
      result = bobgui_sort_keys_new (BobguiNumericSortKeys,
                                  self->sort_order == BOBGUI_SORT_ASCENDING
                                                      ? &BOBGUI_ASCENDING_DOUBLE_SORT_KEYS_CLASS
                                                      : &BOBGUI_DESCENDING_DOUBLE_SORT_KEYS_CLASS,
                                  sizeof (double),
                                  G_ALIGNOF (double));
      break;

    case G_TYPE_LONG:
      result = bobgui_sort_keys_new (BobguiNumericSortKeys,
                                  self->sort_order == BOBGUI_SORT_ASCENDING
                                                      ? &BOBGUI_ASCENDING_LONG_SORT_KEYS_CLASS
                                                      : &BOBGUI_DESCENDING_LONG_SORT_KEYS_CLASS,
                                  sizeof (long),
                                  G_ALIGNOF (long));
      break;

    case G_TYPE_ULONG:
      result = bobgui_sort_keys_new (BobguiNumericSortKeys,
                                  self->sort_order == BOBGUI_SORT_ASCENDING
                                                      ? &BOBGUI_ASCENDING_ULONG_SORT_KEYS_CLASS
                                                      : &BOBGUI_DESCENDING_ULONG_SORT_KEYS_CLASS,
                                  sizeof (gulong),
                                  G_ALIGNOF (gulong));
      break;

    case G_TYPE_INT64:
      result = bobgui_sort_keys_new (BobguiNumericSortKeys,
                                  self->sort_order == BOBGUI_SORT_ASCENDING
                                                      ? &BOBGUI_ASCENDING_INT64_SORT_KEYS_CLASS
                                                      : &BOBGUI_DESCENDING_INT64_SORT_KEYS_CLASS,
                                  sizeof (gint64),
                                  G_ALIGNOF (gint64));
      break;

    case G_TYPE_UINT64:
      result = bobgui_sort_keys_new (BobguiNumericSortKeys,
                                  self->sort_order == BOBGUI_SORT_ASCENDING
                                                      ? &BOBGUI_ASCENDING_UINT64_SORT_KEYS_CLASS
                                                      : &BOBGUI_DESCENDING_UINT64_SORT_KEYS_CLASS,
                                  sizeof (guint64),
                                  G_ALIGNOF (guint64));
      break;

    default:
      g_critical ("Invalid value type %s for expression\n", g_type_name (bobgui_expression_get_value_type (self->expression)));
      return bobgui_sort_keys_new_equal ();
    }

  result->expression = bobgui_expression_ref (self->expression);

  return (BobguiSortKeys *) result;
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static BobguiOrdering
bobgui_numeric_sorter_compare (BobguiSorter *sorter,
                            gpointer   item1,
                            gpointer   item2)
{
  BobguiNumericSorter *self = BOBGUI_NUMERIC_SORTER (sorter);
  GValue value1 = G_VALUE_INIT;
  GValue value2 = G_VALUE_INIT;
  gboolean res1, res2;
  BobguiOrdering result;

  if (self->expression == NULL)
    return BOBGUI_ORDERING_EQUAL;

  res1 = bobgui_expression_evaluate (self->expression, item1, &value1);
  res2 = bobgui_expression_evaluate (self->expression, item2, &value2);

  /* If items don't evaluate, order them at the end, so they aren't
   * in the way. */
  if (!res1)
    {
      result = res2 ? BOBGUI_ORDERING_LARGER : BOBGUI_ORDERING_EQUAL;
      goto out;
    }
  else if (!res2)
    {
      result = BOBGUI_ORDERING_SMALLER;
      goto out;
    }

  switch (g_type_fundamental (G_VALUE_TYPE (&value1)))
    {
    case G_TYPE_BOOLEAN:
      DO_COMPARISON (result, gboolean, g_value_get_boolean, self->sort_order);
      break;

    case G_TYPE_CHAR:
      DO_COMPARISON (result, char, g_value_get_char, self->sort_order);
      break;

    case G_TYPE_UCHAR:
      DO_COMPARISON (result, guchar, g_value_get_uchar, self->sort_order);
      break;

    case G_TYPE_INT:
      DO_COMPARISON (result, int, g_value_get_int, self->sort_order);
      break;

    case G_TYPE_UINT:
      DO_COMPARISON (result, guint, g_value_get_uint, self->sort_order);
      break;

    case G_TYPE_FLOAT:
      {
        float num1 = g_value_get_float (&value1);
        float num2 = g_value_get_float (&value2);

        if (isnan (num1) && isnan (num2))
          result = BOBGUI_ORDERING_EQUAL;
        else if (isnan (num1))
          result = self->sort_order == BOBGUI_SORT_ASCENDING ? BOBGUI_ORDERING_LARGER : BOBGUI_ORDERING_SMALLER;
        else if (isnan (num2))
          result = self->sort_order == BOBGUI_SORT_ASCENDING ? BOBGUI_ORDERING_SMALLER : BOBGUI_ORDERING_LARGER;
        else if (num1 < num2)
          result = self->sort_order == BOBGUI_SORT_ASCENDING ? BOBGUI_ORDERING_SMALLER : BOBGUI_ORDERING_LARGER;
        else if (num1 > num2)
          result = self->sort_order == BOBGUI_SORT_ASCENDING ? BOBGUI_ORDERING_LARGER : BOBGUI_ORDERING_SMALLER;
        else
          result = BOBGUI_ORDERING_EQUAL;
        break;
      }

    case G_TYPE_DOUBLE:
      {
        double num1 = g_value_get_double (&value1);
        double num2 = g_value_get_double (&value2);

        if (isnan (num1) && isnan (num2))
          result = BOBGUI_ORDERING_EQUAL;
        else if (isnan (num1))
          result = self->sort_order == BOBGUI_SORT_ASCENDING ? BOBGUI_ORDERING_LARGER : BOBGUI_ORDERING_SMALLER;
        else if (isnan (num2))
          result = self->sort_order == BOBGUI_SORT_ASCENDING ? BOBGUI_ORDERING_SMALLER : BOBGUI_ORDERING_LARGER;
        else if (num1 < num2)
          result = self->sort_order == BOBGUI_SORT_ASCENDING ? BOBGUI_ORDERING_SMALLER : BOBGUI_ORDERING_LARGER;
        else if (num1 > num2)
          result = self->sort_order == BOBGUI_SORT_ASCENDING ? BOBGUI_ORDERING_LARGER : BOBGUI_ORDERING_SMALLER;
        else
          result = BOBGUI_ORDERING_EQUAL;
        break;
      }

    case G_TYPE_LONG:
      DO_COMPARISON (result, long, g_value_get_long, self->sort_order);
      break;

    case G_TYPE_ULONG:
      DO_COMPARISON (result, gulong, g_value_get_ulong, self->sort_order);
      break;

    case G_TYPE_INT64:
      DO_COMPARISON (result, gint64, g_value_get_int64, self->sort_order);
      break;

    case G_TYPE_UINT64:
      DO_COMPARISON (result, guint64, g_value_get_uint64, self->sort_order);
      break;

    default:
      g_critical ("Invalid value type %s for expression\n", g_type_name (bobgui_expression_get_value_type (self->expression)));
      result = BOBGUI_ORDERING_EQUAL;
      break;
    }

out:
  g_value_unset (&value1);
  g_value_unset (&value2);

  return result;
}

G_GNUC_END_IGNORE_DEPRECATIONS

static BobguiSorterOrder
bobgui_numeric_sorter_get_order (BobguiSorter *sorter)
{
  BobguiNumericSorter *self = BOBGUI_NUMERIC_SORTER (sorter);

  if (self->expression == NULL)
    return BOBGUI_SORTER_ORDER_NONE;

  return BOBGUI_SORTER_ORDER_PARTIAL;
}

static void
bobgui_numeric_sorter_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  BobguiNumericSorter *self = BOBGUI_NUMERIC_SORTER (object);

  switch (prop_id)
    {
    case PROP_EXPRESSION:
      bobgui_numeric_sorter_set_expression (self, bobgui_value_get_expression (value));
      break;

    case PROP_SORT_ORDER:
      bobgui_numeric_sorter_set_sort_order (self, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
bobgui_numeric_sorter_get_property (GObject     *object,
                                guint        prop_id,
                                GValue      *value,
                                GParamSpec  *pspec)
{
  BobguiNumericSorter *self = BOBGUI_NUMERIC_SORTER (object);

  switch (prop_id)
    {
    case PROP_EXPRESSION:
      bobgui_value_set_expression (value, self->expression);
      break;

    case PROP_SORT_ORDER:
      g_value_set_enum (value, self->sort_order);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_numeric_sorter_dispose (GObject *object)
{
  BobguiNumericSorter *self = BOBGUI_NUMERIC_SORTER (object);

  g_clear_pointer (&self->expression, bobgui_expression_unref);

  G_OBJECT_CLASS (bobgui_numeric_sorter_parent_class)->dispose (object);
}

static void
bobgui_numeric_sorter_class_init (BobguiNumericSorterClass *class)
{
  BobguiSorterClass *sorter_class = BOBGUI_SORTER_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  sorter_class->compare = bobgui_numeric_sorter_compare;
  sorter_class->get_order = bobgui_numeric_sorter_get_order;

  object_class->get_property = bobgui_numeric_sorter_get_property;
  object_class->set_property = bobgui_numeric_sorter_set_property;
  object_class->dispose = bobgui_numeric_sorter_dispose;

  /**
   * BobguiNumericSorter:expression: (type BobguiExpression)
   *
   * The expression to evaluate on items to get a number to compare with.
   */
  properties[PROP_EXPRESSION] =
    bobgui_param_spec_expression ("expression", NULL, NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiNumericSorter:sort-order:
   *
   * Whether the sorter will sort smaller numbers first.
   */
  properties[PROP_SORT_ORDER] =
    g_param_spec_enum ("sort-order", NULL, NULL,
                       BOBGUI_TYPE_SORT_TYPE,
                       BOBGUI_SORT_ASCENDING,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

}

static void
bobgui_numeric_sorter_init (BobguiNumericSorter *self)
{
  self->sort_order = BOBGUI_SORT_ASCENDING;

  bobgui_sorter_changed_with_keys (BOBGUI_SORTER (self),
                                BOBGUI_SORTER_CHANGE_DIFFERENT,
                                bobgui_numeric_sort_keys_new (self));
}

/**
 * bobgui_numeric_sorter_new:
 * @expression: (transfer full) (nullable): The expression to evaluate
 *
 * Creates a new numeric sorter using the given @expression.
 *
 * Smaller numbers will be sorted first. You can call
 * [method@Bobgui.NumericSorter.set_sort_order] to change this.
 *
 * Returns: a new `BobguiNumericSorter`
 */
BobguiNumericSorter *
bobgui_numeric_sorter_new (BobguiExpression *expression)
{
  BobguiNumericSorter *result;

  result = g_object_new (BOBGUI_TYPE_NUMERIC_SORTER,
                         "expression", expression,
                         NULL);

  g_clear_pointer (&expression, bobgui_expression_unref);

  return result;
}

/**
 * bobgui_numeric_sorter_get_expression:
 * @self: a `BobguiNumericSorter`
 *
 * Gets the expression that is evaluated to obtain numbers from items.
 *
 * Returns: (transfer none) (nullable): a `BobguiExpression`
 */
BobguiExpression *
bobgui_numeric_sorter_get_expression (BobguiNumericSorter *self)
{
  g_return_val_if_fail (BOBGUI_IS_NUMERIC_SORTER (self), NULL);

  return self->expression;
}

/**
 * bobgui_numeric_sorter_set_expression:
 * @self: a `BobguiNumericSorter`
 * @expression: (nullable) (transfer none): a `BobguiExpression`
 *
 * Sets the expression that is evaluated to obtain numbers from items.
 *
 * Unless an expression is set on @self, the sorter will always
 * compare items as invalid.
 *
 * The expression must have a return type that can be compared
 * numerically, such as %G_TYPE_INT or %G_TYPE_DOUBLE.
 */
void
bobgui_numeric_sorter_set_expression (BobguiNumericSorter *self,
                                  BobguiExpression   *expression)
{
  g_return_if_fail (BOBGUI_IS_NUMERIC_SORTER (self));

  if (self->expression == expression)
    return;

  g_clear_pointer (&self->expression, bobgui_expression_unref);
  if (expression)
    self->expression = bobgui_expression_ref (expression);

  bobgui_sorter_changed_with_keys (BOBGUI_SORTER (self),
                                BOBGUI_SORTER_CHANGE_DIFFERENT,
                                bobgui_numeric_sort_keys_new (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_EXPRESSION]);
}

/**
 * bobgui_numeric_sorter_set_sort_order:
 * @self: a `BobguiNumericSorter`
 * @sort_order: whether to sort smaller numbers first
 *
 * Sets whether to sort smaller numbers before larger ones.
 */
void
bobgui_numeric_sorter_set_sort_order (BobguiNumericSorter *self,
                                   BobguiSortType       sort_order)
{
  g_return_if_fail (BOBGUI_IS_NUMERIC_SORTER (self));

  if (self->sort_order == sort_order)
    return;

  self->sort_order = sort_order;

  bobgui_sorter_changed_with_keys (BOBGUI_SORTER (self),
                                BOBGUI_SORTER_CHANGE_INVERTED,
                                bobgui_numeric_sort_keys_new (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SORT_ORDER]);
}

/**
 * bobgui_numeric_sorter_get_sort_order:
 * @self: a `BobguiNumericSorter`
 *
 * Gets whether this sorter will sort smaller numbers first.
 *
 * Returns: the order of the numbers
 */
BobguiSortType
bobgui_numeric_sorter_get_sort_order (BobguiNumericSorter *self)
{
  g_return_val_if_fail (BOBGUI_IS_NUMERIC_SORTER (self), BOBGUI_SORT_ASCENDING);

  return self->sort_order;
}
