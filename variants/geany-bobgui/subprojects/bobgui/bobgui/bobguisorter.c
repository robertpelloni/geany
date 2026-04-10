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

#include "bobguisorterprivate.h"

#include <glib/gi18n-lib.h>
#include "bobguitypebuiltins.h"
#include "bobguiprivate.h"

/**
 * BobguiSorter:
 *
 * Describes sorting criteria for a [class@Bobgui.SortListModel].
 *
 * Its primary user is [class@Bobgui.SortListModel]
 *
 * The model will use a sorter to determine the order in which
 * its items should appear by calling [method@Bobgui.Sorter.compare]
 * for pairs of items.
 *
 * Sorters may change their sorting behavior through their lifetime.
 * In that case, they will emit the [signal@Bobgui.Sorter::changed] signal
 * to notify that the sort order is no longer valid and should be updated
 * by calling bobgui_sorter_compare() again.
 *
 * BOBGUI provides various pre-made sorter implementations for common sorting
 * operations. [class@Bobgui.ColumnView] has built-in support for sorting lists
 * via the [property@Bobgui.ColumnViewColumn:sorter] property, where the user can
 * change the sorting by clicking on list headers.
 *
 * Of course, in particular for large lists, it is also possible to subclass
 * `BobguiSorter` and provide one's own sorter.
 */

typedef struct _BobguiSorterPrivate BobguiSorterPrivate;
typedef struct _BobguiDefaultSortKeys BobguiDefaultSortKeys;

struct _BobguiSorterPrivate
{
  BobguiSortKeys *keys;
};

struct _BobguiDefaultSortKeys
{
  BobguiSortKeys keys;
  BobguiSorter *sorter;
};

enum {
  CHANGED,
  LAST_SIGNAL
};

G_DEFINE_TYPE_WITH_PRIVATE (BobguiSorter, bobgui_sorter, G_TYPE_OBJECT)

static guint signals[LAST_SIGNAL] = { 0 };

static BobguiOrdering
bobgui_sorter_default_compare (BobguiSorter *self,
                            gpointer   item1,
                            gpointer   item2)
{
  g_critical ("Sorter of type '%s' does not implement BobguiSorter::compare", G_OBJECT_TYPE_NAME (self));

  return BOBGUI_ORDERING_EQUAL;
}

static BobguiSorterOrder
bobgui_sorter_default_get_order (BobguiSorter *self)
{
  return BOBGUI_SORTER_ORDER_PARTIAL;
}

static void
bobgui_sorter_dispose (GObject *object)
{
  BobguiSorter *self = BOBGUI_SORTER (object);
  BobguiSorterPrivate *priv = bobgui_sorter_get_instance_private (self);

  g_clear_pointer (&priv->keys, bobgui_sort_keys_unref);

  G_OBJECT_CLASS (bobgui_sorter_parent_class)->dispose (object);
}

static void
bobgui_sorter_class_init (BobguiSorterClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = bobgui_sorter_dispose;

  class->compare = bobgui_sorter_default_compare;
  class->get_order = bobgui_sorter_default_get_order;

  /**
   * BobguiSorter::changed:
   * @self: The `BobguiSorter`
   * @change: how the sorter changed
   *
   * Emitted whenever the sorter changed.
   *
   * Users of the sorter should then update the sort order
   * again via bobgui_sorter_compare().
   *
   * [class@Bobgui.SortListModel] handles this signal automatically.
   *
   * Depending on the @change parameter, it may be possible to update
   * the sort order without a full resorting. Refer to the
   * [enum@Bobgui.SorterChange] documentation for details.
   */
  signals[CHANGED] =
    g_signal_new (I_("changed"),
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  BOBGUI_TYPE_SORTER_CHANGE);
  g_signal_set_va_marshaller (signals[CHANGED],
                              G_TYPE_FROM_CLASS (class),
                              g_cclosure_marshal_VOID__ENUMv);
}

static void
bobgui_sorter_init (BobguiSorter *self)
{
}

/**
 * bobgui_sorter_compare:
 * @self: a `BobguiSorter`
 * @item1: (type GObject) (transfer none): first item to compare
 * @item2: (type GObject) (transfer none): second item to compare
 *
 * Compares two given items according to the sort order implemented
 * by the sorter.
 *
 * Sorters implement a partial order:
 *
 * * It is reflexive, ie a = a
 * * It is antisymmetric, ie if a < b and b < a, then a = b
 * * It is transitive, ie given any 3 items with a ≤ b and b ≤ c,
 *   then a ≤ c
 *
 * The sorter may signal it conforms to additional constraints
 * via the return value of [method@Bobgui.Sorter.get_order].
 *
 * Returns: %BOBGUI_ORDERING_EQUAL if @item1 == @item2,
 *   %BOBGUI_ORDERING_SMALLER if @item1 < @item2,
 *   %BOBGUI_ORDERING_LARGER if @item1 > @item2
 */
BobguiOrdering
bobgui_sorter_compare (BobguiSorter *self,
                    gpointer   item1,
                    gpointer   item2)
{
  BobguiOrdering result;

  /* We turn this off because bobgui_sorter_compare() is called so much that it's too expensive */
  /* g_return_val_if_fail (BOBGUI_IS_SORTER (self), BOBGUI_ORDERING_EQUAL); */
  g_return_val_if_fail (item1 && item2, BOBGUI_ORDERING_EQUAL);

  if (item1 == item2)
    return BOBGUI_ORDERING_EQUAL;

  result = BOBGUI_SORTER_GET_CLASS (self)->compare (self, item1, item2);

#ifdef G_ENABLE_DEBUG
  if (result < -1 || result > 1)
    {
      g_critical ("A sorter of type \"%s\" returned %d, which is not a valid BobguiOrdering result.\n"
                  "Did you forget to call bobgui_ordering_from_cmpfunc()?",
                  G_OBJECT_TYPE_NAME (self), (int) result);
    }
#endif

  return result;
}

/**
 * bobgui_sorter_get_order:
 * @self: a `BobguiSorter`
 *
 * Gets the order that @self conforms to.
 *
 * See [enum@Bobgui.SorterOrder] for details
 * of the possible return values.
 *
 * This function is intended to allow optimizations.
 *
 * Returns: The order
 */
BobguiSorterOrder
bobgui_sorter_get_order (BobguiSorter *self)
{
  g_return_val_if_fail (BOBGUI_IS_SORTER (self), BOBGUI_SORTER_ORDER_PARTIAL);

  return BOBGUI_SORTER_GET_CLASS (self)->get_order (self);
}

static int
bobgui_default_sort_keys_compare (gconstpointer a,
                               gconstpointer b,
                               gpointer      data)
{
  BobguiDefaultSortKeys *self = data;
  gpointer *key_a = (gpointer *) a;
  gpointer *key_b = (gpointer *) b;

  return bobgui_sorter_compare (self->sorter, *key_a, *key_b);
}

static void
bobgui_default_sort_keys_free (BobguiSortKeys *keys)
{
  BobguiDefaultSortKeys *self = (BobguiDefaultSortKeys *) keys;

  g_object_unref (self->sorter);

  g_free (self);
}

static gboolean
bobgui_default_sort_keys_is_compatible (BobguiSortKeys *keys,
                                     BobguiSortKeys *other)
{
  if (keys->klass != other->klass)
    return FALSE;

  return TRUE;
}

static void
bobgui_default_sort_keys_init_key (BobguiSortKeys *self,
                                gpointer     item,
                                gpointer     key_memory)
{
  gpointer *key = (gpointer *) key_memory;

  *key = g_object_ref (item);
}

static void
bobgui_default_sort_keys_clear_key (BobguiSortKeys *self,
                                 gpointer     key_memory)
{
  gpointer *key = (gpointer *) key_memory;

  g_object_unref (*key);
}

static const BobguiSortKeysClass BOBGUI_DEFAULT_SORT_KEYS_CLASS = 
{
  bobgui_default_sort_keys_free,
  bobgui_default_sort_keys_compare,
  bobgui_default_sort_keys_is_compatible,
  bobgui_default_sort_keys_init_key,
  bobgui_default_sort_keys_clear_key,
};

/*<private>
 * bobgui_sorter_get_keys:
 * @self: a `BobguiSorter`
 *
 * Gets a `BobguiSortKeys` that can be used as an alternative to
 * @self for faster sorting.
 *
 * The sort keys can change every time [signal@Bobgui.Sorter::changed]
 * is emitted. When the keys change, you should redo all comparisons
 * with the new keys.
 *
 * When [method@Bobgui.SortKeys.is_compatible] for the old and new keys
 * returns %TRUE, you can reuse keys you generated previously.
 *
 * Returns: (transfer full): the sort keys to sort with
 */
BobguiSortKeys *
bobgui_sorter_get_keys (BobguiSorter *self)
{
  BobguiSorterPrivate *priv = bobgui_sorter_get_instance_private (self);
  BobguiDefaultSortKeys *fallback;

  g_return_val_if_fail (BOBGUI_IS_SORTER (self), NULL);

  if (priv->keys)
    return bobgui_sort_keys_ref (priv->keys);

  fallback = bobgui_sort_keys_new (BobguiDefaultSortKeys, &BOBGUI_DEFAULT_SORT_KEYS_CLASS, sizeof (gpointer), G_ALIGNOF (gpointer));
  fallback->sorter = g_object_ref (self);

  return (BobguiSortKeys *) fallback;
}

/**
 * bobgui_sorter_changed:
 * @self: a `BobguiSorter`
 * @change: How the sorter changed
 *
 * Notifies all users of the sorter that it has changed.
 *
 * This emits the [signal@Bobgui.Sorter::changed] signal. Users
 * of the sorter should then update the sort order via
 * [method@Bobgui.Sorter.compare].
 *
 * Depending on the @change parameter, it may be possible to
 * update the sort order without a full resorting. Refer to
 * the [enum@Bobgui.SorterChange] documentation for details.
 *
 * This function is intended for implementers of `BobguiSorter`
 * subclasses and should not be called from other functions.
 */
void
bobgui_sorter_changed (BobguiSorter       *self,
                    BobguiSorterChange  change)
{
  g_return_if_fail (BOBGUI_IS_SORTER (self));

  g_signal_emit (self, signals[CHANGED], 0, change);
}

/*<private>
 * bobgui_sorter_changed_with_keys:
 * @self: a `BobguiSorter`
 * @change: How the sorter changed
 * @keys: (not nullable) (transfer full): New keys to use
 *
 * Updates the sorter's keys to @keys and then calls bobgui_sorter_changed().
 *
 * If you do not want to update the keys, call that function instead.
 *
 * This function should also be called in your_sorter_init() to initialize
 * the keys to use with your sorter.
 */
void
bobgui_sorter_changed_with_keys (BobguiSorter       *self,
                              BobguiSorterChange  change,
                              BobguiSortKeys     *keys)
{
  BobguiSorterPrivate *priv = bobgui_sorter_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_SORTER (self));
  g_return_if_fail (keys != NULL);

  g_clear_pointer (&priv->keys, bobgui_sort_keys_unref);
  priv->keys = keys;

  bobgui_sorter_changed (self, change);
}

/* See the comment in bobguienums.h as to why we need to play
 * games with the introspection scanner for static inline
 * functions
 */
#ifdef __GI_SCANNER__
/**
 * bobgui_ordering_from_cmpfunc: (skip)
 * @cmpfunc_result: Result of a comparison function
 *
 * Converts the result of a `GCompareFunc` like strcmp() to a
 * `BobguiOrdering` value.
 *
 * Returns: the corresponding `BobguiOrdering`
 *
 * Since: 4.2
 **/
BobguiOrdering
bobgui_ordering_from_cmpfunc (int cmpfunc_result)
{
  return (BobguiOrdering) ((cmpfunc_result > 0) - (cmpfunc_result < 0));
}
#endif
