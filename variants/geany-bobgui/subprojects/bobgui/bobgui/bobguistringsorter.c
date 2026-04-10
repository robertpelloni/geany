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

#include "bobguistringsorter.h"

#include "bobguisorterprivate.h"
#include "bobguitypebuiltins.h"

/**
 * BobguiStringSorter:
 *
 * Sorts items by comparing strings.
 *
 * To obtain the strings to compare, this sorter evaluates a
 * [class@Bobgui.Expression].
 *
 * It does the comparison in a linguistically correct way using the
 * current locale by normalizing Unicode strings and possibly case-folding
 * them before performing the comparison.
 */

struct _BobguiStringSorter
{
  BobguiSorter parent_instance;

  gboolean ignore_case;
  BobguiCollation collation;

  BobguiExpression *expression;
};

enum {
  PROP_0,
  PROP_EXPRESSION,
  PROP_IGNORE_CASE,
  PROP_COLLATION,
  NUM_PROPERTIES
};

G_DEFINE_TYPE (BobguiStringSorter, bobgui_string_sorter, BOBGUI_TYPE_SORTER)

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

static char *
bobgui_string_sorter_get_key (BobguiExpression *expression,
                           gboolean       ignore_case,
                           BobguiCollation   collation,
                           gpointer       item1)
{
  GValue value = G_VALUE_INIT;
  const char *string;
  char *s;
  char *key;

  if (expression == NULL)
    return NULL;

  if (!bobgui_expression_evaluate (expression, item1, &value))
    return NULL;

  string = g_value_get_string (&value);
  if (string == NULL)
    {
      g_value_unset (&value);
      return NULL;
    }

  if (ignore_case)
    s = g_utf8_casefold (string, -1);
  else
    s = (char *) string;

  switch (collation)
    {
    case BOBGUI_COLLATION_NONE:
      if (ignore_case)
        key = g_steal_pointer (&s);
      else
        key = g_strdup (s);
      break;

    case BOBGUI_COLLATION_UNICODE:
      key = g_utf8_collate_key (s, -1);
      break;

    case BOBGUI_COLLATION_FILENAME:
      key = g_utf8_collate_key_for_filename (s, -1);
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  if (s != string)
    g_free (s);

  g_value_unset (&value);

  return key;
}

static BobguiOrdering
bobgui_string_sorter_compare (BobguiSorter *sorter,
                           gpointer   item1,
                           gpointer   item2)
{
  BobguiStringSorter *self = BOBGUI_STRING_SORTER (sorter);
  char *s1, *s2;
  BobguiOrdering result;

  if (self->expression == NULL)
    return BOBGUI_ORDERING_EQUAL;

  s1 = bobgui_string_sorter_get_key (self->expression, self->ignore_case, self->collation, item1);
  s2 = bobgui_string_sorter_get_key (self->expression, self->ignore_case, self->collation, item2);

  result = bobgui_ordering_from_cmpfunc (g_strcmp0 (s1, s2));

  g_free (s1);
  g_free (s2);

  return result;
}

static BobguiSorterOrder
bobgui_string_sorter_get_order (BobguiSorter *sorter)
{
  BobguiStringSorter *self = BOBGUI_STRING_SORTER (sorter);

  if (self->expression == NULL)
    return BOBGUI_SORTER_ORDER_NONE;

  return BOBGUI_SORTER_ORDER_PARTIAL;
}

typedef struct _BobguiStringSortKeys BobguiStringSortKeys;
struct _BobguiStringSortKeys
{
  BobguiSortKeys keys;

  BobguiExpression *expression;
  gboolean ignore_case;
  BobguiCollation collation;
};

static void
bobgui_string_sort_keys_free (BobguiSortKeys *keys)
{
  BobguiStringSortKeys *self = (BobguiStringSortKeys *) keys;

  bobgui_expression_unref (self->expression);
  g_free (self);
}

static int
bobgui_string_sort_keys_compare (gconstpointer a,
                              gconstpointer b,
                              gpointer      unused)
{
  const char *sa = *(const char **) a;
  const char *sb = *(const char **) b;

  if (sa == NULL)
    return sb == NULL ? BOBGUI_ORDERING_EQUAL : BOBGUI_ORDERING_LARGER;
  else if (sb == NULL)
    return BOBGUI_ORDERING_SMALLER;

  return bobgui_ordering_from_cmpfunc (strcmp (sa, sb));
}

static gboolean
bobgui_string_sort_keys_is_compatible (BobguiSortKeys *keys,
                                    BobguiSortKeys *other)
{
  return FALSE;
}

static void
bobgui_string_sort_keys_init_key (BobguiSortKeys *keys,
                               gpointer     item,
                               gpointer     key_memory)
{
  BobguiStringSortKeys *self = (BobguiStringSortKeys *) keys;
  char **key = (char **) key_memory;

  *key = bobgui_string_sorter_get_key (self->expression, self->ignore_case, self->collation, item);
}

static void
bobgui_string_sort_keys_clear_key (BobguiSortKeys *keys,
                                gpointer     key_memory)
{
  char **key = (char **) key_memory;

  g_free (*key);
}

static const BobguiSortKeysClass BOBGUI_STRING_SORT_KEYS_CLASS =
{
  bobgui_string_sort_keys_free,
  bobgui_string_sort_keys_compare,
  bobgui_string_sort_keys_is_compatible,
  bobgui_string_sort_keys_init_key,
  bobgui_string_sort_keys_clear_key,
};

static BobguiSortKeys *
bobgui_string_sort_keys_new (BobguiStringSorter *self)
{
  BobguiStringSortKeys *result;

  if (self->expression == NULL)
    return bobgui_sort_keys_new_equal ();

  result = bobgui_sort_keys_new (BobguiStringSortKeys,
                              &BOBGUI_STRING_SORT_KEYS_CLASS,
                              sizeof (char *),
                              G_ALIGNOF (char *));

  result->expression = bobgui_expression_ref (self->expression);
  result->ignore_case = self->ignore_case;
  result->collation = self->collation;

  return (BobguiSortKeys *) result;
}

static void
bobgui_string_sorter_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  BobguiStringSorter *self = BOBGUI_STRING_SORTER (object);

  switch (prop_id)
    {
    case PROP_EXPRESSION:
      bobgui_string_sorter_set_expression (self, bobgui_value_get_expression (value));
      break;

    case PROP_IGNORE_CASE:
      bobgui_string_sorter_set_ignore_case (self, g_value_get_boolean (value));
      break;

    case PROP_COLLATION:
      bobgui_string_sorter_set_collation (self, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_string_sorter_get_property (GObject     *object,
                                guint        prop_id,
                                GValue      *value,
                                GParamSpec  *pspec)
{
  BobguiStringSorter *self = BOBGUI_STRING_SORTER (object);

  switch (prop_id)
    {
    case PROP_EXPRESSION:
      bobgui_value_set_expression (value, self->expression);
      break;

    case PROP_IGNORE_CASE:
      g_value_set_boolean (value, self->ignore_case);
      break;

    case PROP_COLLATION:
      g_value_set_enum (value, self->collation);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_string_sorter_dispose (GObject *object)
{
  BobguiStringSorter *self = BOBGUI_STRING_SORTER (object);

  g_clear_pointer (&self->expression, bobgui_expression_unref);

  G_OBJECT_CLASS (bobgui_string_sorter_parent_class)->dispose (object);
}

static void
bobgui_string_sorter_class_init (BobguiStringSorterClass *class)
{
  BobguiSorterClass *sorter_class = BOBGUI_SORTER_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  sorter_class->compare = bobgui_string_sorter_compare;
  sorter_class->get_order = bobgui_string_sorter_get_order;

  object_class->get_property = bobgui_string_sorter_get_property;
  object_class->set_property = bobgui_string_sorter_set_property;
  object_class->dispose = bobgui_string_sorter_dispose;

  /**
   * BobguiStringSorter:expression: (type BobguiExpression)
   *
   * The expression to evaluate on item to get a string to compare with.
   */
  properties[PROP_EXPRESSION] =
    bobgui_param_spec_expression ("expression", NULL, NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiStringSorter:ignore-case:
   *
   * If sorting is case sensitive.
   */
  properties[PROP_IGNORE_CASE] =
      g_param_spec_boolean ("ignore-case", NULL, NULL,
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiStringSorter:collation:
   *
   * The collation method to use for sorting.
   *
   * The `BOBGUI_COLLATION_NONE` value is useful when the expression already
   * returns collation keys, or strings that need to be compared byte-by-byte.
   *
   * The default value, `BOBGUI_COLLATION_UNICODE`, compares strings according
   * to the [Unicode collation algorithm](https://www.unicode.org/reports/tr10/).
   *
   * Since: 4.10
   */
  properties[PROP_COLLATION] =
      g_param_spec_enum ("collation", NULL, NULL,
                         BOBGUI_TYPE_COLLATION,
                         BOBGUI_COLLATION_UNICODE,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

}

static void
bobgui_string_sorter_init (BobguiStringSorter *self)
{
  self->ignore_case = TRUE;
  self->collation = BOBGUI_COLLATION_UNICODE;

  bobgui_sorter_changed_with_keys (BOBGUI_SORTER (self),
                                BOBGUI_SORTER_CHANGE_DIFFERENT,
                                bobgui_string_sort_keys_new (self));
}

/**
 * bobgui_string_sorter_new:
 * @expression: (transfer full) (nullable): The expression to evaluate
 *
 * Creates a new string sorter that compares items using the given
 * @expression.
 *
 * Unless an expression is set on it, this sorter will always
 * compare items as invalid.
 *
 * Returns: a new `BobguiStringSorter`
 */
BobguiStringSorter *
bobgui_string_sorter_new (BobguiExpression *expression)
{
  BobguiStringSorter *result;

  result = g_object_new (BOBGUI_TYPE_STRING_SORTER,
                         "expression", expression,
                         NULL);

  g_clear_pointer (&expression, bobgui_expression_unref);

  return result;
}

/**
 * bobgui_string_sorter_get_expression:
 * @self: a `BobguiStringSorter`
 *
 * Gets the expression that is evaluated to obtain strings from items.
 *
 * Returns: (transfer none) (nullable): a `BobguiExpression`
 */
BobguiExpression *
bobgui_string_sorter_get_expression (BobguiStringSorter *self)
{
  g_return_val_if_fail (BOBGUI_IS_STRING_SORTER (self), NULL);

  return self->expression;
}

/**
 * bobgui_string_sorter_set_expression:
 * @self: a `BobguiStringSorter`
 * @expression: (nullable) (transfer none): a `BobguiExpression`
 *
 * Sets the expression that is evaluated to obtain strings from items.
 *
 * The expression must have the type %G_TYPE_STRING.
 */
void
bobgui_string_sorter_set_expression (BobguiStringSorter *self,
                                  BobguiExpression   *expression)
{
  g_return_if_fail (BOBGUI_IS_STRING_SORTER (self));
  g_return_if_fail (expression == NULL || bobgui_expression_get_value_type (expression) == G_TYPE_STRING);

  if (self->expression == expression)
    return;

  g_clear_pointer (&self->expression, bobgui_expression_unref);
  if (expression)
    self->expression = bobgui_expression_ref (expression);

  bobgui_sorter_changed_with_keys (BOBGUI_SORTER (self),
                                BOBGUI_SORTER_CHANGE_DIFFERENT,
                                bobgui_string_sort_keys_new (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_EXPRESSION]);
}

/**
 * bobgui_string_sorter_get_ignore_case:
 * @self: a `BobguiStringSorter`
 *
 * Gets whether the sorter ignores case differences.
 *
 * Returns: %TRUE if @self is ignoring case differences
 */
gboolean
bobgui_string_sorter_get_ignore_case (BobguiStringSorter *self)
{
  g_return_val_if_fail (BOBGUI_IS_STRING_SORTER (self), TRUE);

  return self->ignore_case;
}

/**
 * bobgui_string_sorter_set_ignore_case:
 * @self: a `BobguiStringSorter`
 * @ignore_case: %TRUE to ignore case differences
 *
 * Sets whether the sorter will ignore case differences.
 */
void
bobgui_string_sorter_set_ignore_case (BobguiStringSorter *self,
                                   gboolean         ignore_case)
{
  g_return_if_fail (BOBGUI_IS_STRING_SORTER (self));

  if (self->ignore_case == ignore_case)
    return;

  self->ignore_case = ignore_case;

  bobgui_sorter_changed_with_keys (BOBGUI_SORTER (self),
                                ignore_case ? BOBGUI_SORTER_CHANGE_LESS_STRICT : BOBGUI_SORTER_CHANGE_MORE_STRICT,
                                bobgui_string_sort_keys_new (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_IGNORE_CASE]);
}

/**
 * bobgui_string_sorter_get_collation:
 * @self: a `BobguiStringSorter`
 *
 * Gets which collation method the sorter uses.
 *
 * Returns: The collation method
 *
 * Since: 4.10
 */
BobguiCollation
bobgui_string_sorter_get_collation (BobguiStringSorter *self)
{
  g_return_val_if_fail (BOBGUI_IS_STRING_SORTER (self), BOBGUI_COLLATION_UNICODE);

  return self->collation;
}

/**
 * bobgui_string_sorter_set_collation:
 * @self: a `BobguiStringSorter`
 * @collation: the collation method
 *
 * Sets the collation method to use for sorting.
 *
 * Since: 4.10
 */
void
bobgui_string_sorter_set_collation (BobguiStringSorter *self,
                                 BobguiCollation     collation)
{
  g_return_if_fail (BOBGUI_IS_STRING_SORTER (self));

  if (self->collation == collation)
    return;

  self->collation = collation;

  bobgui_sorter_changed_with_keys (BOBGUI_SORTER (self),
                                BOBGUI_SORTER_CHANGE_DIFFERENT,
                                bobgui_string_sort_keys_new (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_COLLATION]);
}
