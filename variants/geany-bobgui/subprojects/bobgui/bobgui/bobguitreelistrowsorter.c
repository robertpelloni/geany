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

#include "bobguitreelistrowsorter.h"

#include "bobguitreelistmodel.h"

#include "bobguisorterprivate.h"
#include "bobguitypebuiltins.h"

/**
 * BobguiTreeListRowSorter:
 *
 * Applies a gives sorter to the levels in a tree.
 *
 * Here is an example for setting up a column view with a tree model and
 * a `BobguiTreeListSorter`:
 *
 * ```c
 * column_sorter = bobgui_column_view_get_sorter (view);
 * sorter = bobgui_tree_list_row_sorter_new (g_object_ref (column_sorter));
 * sort_model = bobgui_sort_list_model_new (tree_model, sorter);
 * selection = bobgui_single_selection_new (sort_model);
 * bobgui_column_view_set_model (view, G_LIST_MODEL (selection));
 * ```
 */

struct _BobguiTreeListRowSorter
{
  BobguiSorter parent_instance;

  BobguiSorter *sorter;
};

enum {
  PROP_0,
  PROP_SORTER,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

G_DEFINE_TYPE (BobguiTreeListRowSorter, bobgui_tree_list_row_sorter, BOBGUI_TYPE_SORTER)

#define MAX_KEY_DEPTH (8)

/* our key is a gpointer[MAX_KEY_DEPTH] and contains:
 *
 * key[0] != NULL:
 * The depth of the item is <= MAX_KEY_DEPTH so we can put the keys
 * inline. This is the key for the ancestor at depth 0.
 *
 * key[0] == NULL && key[1] != NULL:
 * The depth of the item is > MAX_KEY_DEPTH so it had to be allocated.
 * key[1] contains this allocated and NULL-terminated array.
 *
 * key[0] == NULL && key[1] == NULL:
 * The item is not a TreeListRow. To break ties, we put the item in key[2] to
 * allow a direct compare.
 */
typedef struct _BobguiTreeListRowSortKeys BobguiTreeListRowSortKeys;
typedef struct _BobguiTreeListRowCacheKey BobguiTreeListRowCacheKey;
struct _BobguiTreeListRowSortKeys
{
  BobguiSortKeys keys;

  BobguiSortKeys *sort_keys;
  GHashTable *cached_keys;
};

struct _BobguiTreeListRowCacheKey
{
  BobguiTreeListRow *row;
  guint ref_count;
};

static BobguiTreeListRowCacheKey *
cache_key_from_key (BobguiTreeListRowSortKeys *self,
                    gpointer                key)
{
  if (self->sort_keys == NULL)
    return key;

  return (BobguiTreeListRowCacheKey *) ((char *) key + BOBGUI_SORT_KEYS_ALIGN (bobgui_sort_keys_get_key_size (self->sort_keys), G_ALIGNOF (BobguiTreeListRowCacheKey)));
}

static void
bobgui_tree_list_row_sort_keys_free (BobguiSortKeys *keys)
{
  BobguiTreeListRowSortKeys *self = (BobguiTreeListRowSortKeys *) keys;

  g_assert (g_hash_table_size (self->cached_keys) == 0);
  g_hash_table_unref (self->cached_keys);
  if (self->sort_keys)
    bobgui_sort_keys_unref (self->sort_keys);
  g_free (self);
}

static inline gboolean
unpack (gpointer  *key,
        gpointer **out_keys,
        gsize     *out_max_size)
{
  if (key[0])
    {
      *out_keys = key;
      *out_max_size = MAX_KEY_DEPTH;
      return TRUE;
    }
  else if (key[1])
    {
      *out_keys = (gpointer *) key[1];
      *out_max_size = G_MAXSIZE;
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

static int
bobgui_tree_list_row_sort_keys_compare (gconstpointer a,
                                     gconstpointer b,
                                     gpointer      data)
{
  BobguiTreeListRowSortKeys *self = (BobguiTreeListRowSortKeys *) data;
  gpointer *keysa = (gpointer *) a;
  gpointer *keysb = (gpointer *) b;
  gsize sizea, sizeb;
  gboolean resa, resb;
  gsize i;
  BobguiOrdering result;

  resa = unpack (keysa, &keysa, &sizea);
  resb = unpack (keysb, &keysb, &sizeb);
  if (!resa)
    return resb ? BOBGUI_ORDERING_LARGER : (keysa[2] < keysb[2] ? BOBGUI_ORDERING_SMALLER : 
                                        (keysa[2] > keysb[2] ? BOBGUI_ORDERING_LARGER : BOBGUI_ORDERING_EQUAL));
  else if (!resb)
    return BOBGUI_ORDERING_SMALLER;

  for (i = 0; i < MIN (sizea, sizeb); i++)
    {
      if (keysa[i] == keysb[i])
        {
          if (keysa[i] == NULL)
            return BOBGUI_ORDERING_EQUAL;
          continue;
        }
      else if (keysa[i] == NULL)
        return BOBGUI_ORDERING_SMALLER;
      else if (keysb[i] == NULL)
        return BOBGUI_ORDERING_LARGER;

      if (self->sort_keys)
        result = bobgui_sort_keys_compare (self->sort_keys, keysa[i], keysb[i]);
      else
        result = BOBGUI_ORDERING_EQUAL;

      if (result == BOBGUI_ORDERING_EQUAL)
        {
          /* We must break ties here because if a ever gets a child,
           * it would need to go right in between a and b. */
          BobguiTreeListRowCacheKey *cachea = cache_key_from_key (self, keysa[i]);
          BobguiTreeListRowCacheKey *cacheb = cache_key_from_key (self, keysb[i]);
          if (bobgui_tree_list_row_get_position (cachea->row) < bobgui_tree_list_row_get_position (cacheb->row))
            result = BOBGUI_ORDERING_SMALLER;
          else
            result = BOBGUI_ORDERING_LARGER;
        }
      return result;
    }

  if (sizea < sizeb)
    return BOBGUI_ORDERING_SMALLER;
  else if (sizea > sizeb)
    return BOBGUI_ORDERING_LARGER;
  else
    return BOBGUI_ORDERING_EQUAL;
}

static gboolean
bobgui_tree_list_row_sort_keys_is_compatible (BobguiSortKeys *keys,
                                           BobguiSortKeys *other)
{
  BobguiTreeListRowSortKeys *self = (BobguiTreeListRowSortKeys *) keys;
  BobguiTreeListRowSortKeys *compare;

  /* FIXME https://gitlab.gnome.org/GNOME/bobgui/-/issues/3228 */
  return FALSE;

  if (keys->klass != other->klass)
    return FALSE;

  compare = (BobguiTreeListRowSortKeys *) other;

  if (self->sort_keys && compare->sort_keys)
    return bobgui_sort_keys_is_compatible (self->sort_keys, compare->sort_keys);
  else
    return self->sort_keys == compare->sort_keys;
}

static gpointer
bobgui_tree_list_row_sort_keys_ref_key (BobguiTreeListRowSortKeys *self,
                                     BobguiTreeListRow         *row)
{
  BobguiTreeListRowCacheKey *cache_key;
  gpointer key;

  key = g_hash_table_lookup (self->cached_keys, row);
  if (key)
    {
      cache_key = cache_key_from_key (self, key);
      cache_key->ref_count++;
      return key;
    }

  if (self->sort_keys)
    key = g_malloc (BOBGUI_SORT_KEYS_ALIGN (bobgui_sort_keys_get_key_size (self->sort_keys), G_ALIGNOF (BobguiTreeListRowCacheKey))
                    + sizeof (BobguiTreeListRowCacheKey));
  else
    key = g_malloc (sizeof (BobguiTreeListRowCacheKey));
  cache_key = cache_key_from_key (self, key);
  cache_key->row = g_object_ref (row);
  cache_key->ref_count = 1;
  if (self->sort_keys)
    {
      gpointer item = bobgui_tree_list_row_get_item (row);
      bobgui_sort_keys_init_key (self->sort_keys, item, key);
      g_object_unref (item);
    }

  g_hash_table_insert (self->cached_keys, row, key);
  return key;
}

static void
bobgui_tree_list_row_sort_keys_unref_key (BobguiTreeListRowSortKeys *self,
                                       gpointer                key)
{
  BobguiTreeListRowCacheKey *cache_key = cache_key_from_key (self, key);

  cache_key->ref_count--;
  if (cache_key->ref_count > 0)
    return;

  if (self->sort_keys)
    bobgui_sort_keys_clear_key (self->sort_keys, key);

  g_hash_table_remove (self->cached_keys, cache_key->row);
  g_object_unref (cache_key->row);
  g_free (key);
}

static void
bobgui_tree_list_row_sort_keys_init_key (BobguiSortKeys *keys,
                                      gpointer     item,
                                      gpointer     key_memory)
{
  BobguiTreeListRowSortKeys *self = (BobguiTreeListRowSortKeys *) keys;
  gpointer *key = (gpointer *) key_memory;
  BobguiTreeListRow *row, *parent;
  guint i, depth;

  if (!BOBGUI_IS_TREE_LIST_ROW (item))
    {
      key[0] = NULL;
      key[1] = NULL;
      key[2] = item;
      return;
    }

  row = BOBGUI_TREE_LIST_ROW (item);
  depth = bobgui_tree_list_row_get_depth (row) + 1;
  if (depth > MAX_KEY_DEPTH)
    {
      key[0] = NULL;
      key[1] = g_new (gpointer, depth + 1);
      key = key[1];
      key[depth] = NULL;
    }
  else if (depth < MAX_KEY_DEPTH)
    {
      key[depth] = NULL;
    }

  g_object_ref (row);
  for (i = depth; i-- > 0; )
    {
      key[i] = bobgui_tree_list_row_sort_keys_ref_key (self, row);
      parent = bobgui_tree_list_row_get_parent (row);
      g_object_unref (row);
      row = parent;
    }
  g_assert (row == NULL);
}

static void
bobgui_tree_list_row_sort_keys_clear_key (BobguiSortKeys *keys,
                                       gpointer     key_memory)
{
  BobguiTreeListRowSortKeys *self = (BobguiTreeListRowSortKeys *) keys;
  gpointer *key = (gpointer *) key_memory;
  gsize i, max;

  if (!unpack (key, &key, &max))
    return;

  for (i = 0; i < max && key[i] != NULL; i++)
    bobgui_tree_list_row_sort_keys_unref_key (self, key[i]);
  
  if (key[0] == NULL)
    g_free (key[1]);
}

static const BobguiSortKeysClass BOBGUI_TREE_LIST_ROW_SORT_KEYS_CLASS =
{
  bobgui_tree_list_row_sort_keys_free,
  bobgui_tree_list_row_sort_keys_compare,
  bobgui_tree_list_row_sort_keys_is_compatible,
  bobgui_tree_list_row_sort_keys_init_key,
  bobgui_tree_list_row_sort_keys_clear_key,
};

static BobguiSortKeys *
bobgui_tree_list_row_sort_keys_new (BobguiTreeListRowSorter *self)
{
  BobguiTreeListRowSortKeys *result;

  result = bobgui_sort_keys_new (BobguiTreeListRowSortKeys,
                              &BOBGUI_TREE_LIST_ROW_SORT_KEYS_CLASS,
                              sizeof (gpointer[MAX_KEY_DEPTH]),
                              G_ALIGNOF (gpointer));

  if (self->sorter)
    result->sort_keys = bobgui_sorter_get_keys (self->sorter);
  result->cached_keys = g_hash_table_new (NULL, NULL);

  return (BobguiSortKeys *) result;
}

static BobguiOrdering
bobgui_tree_list_row_sorter_compare (BobguiSorter *sorter,
                                  gpointer   item1,
                                  gpointer   item2)
{
  BobguiTreeListRowSorter *self = BOBGUI_TREE_LIST_ROW_SORTER (sorter);
  BobguiTreeListRow *r1, *r2;
  BobguiTreeListRow *p1, *p2;
  guint d1, d2;
  BobguiOrdering result = BOBGUI_ORDERING_EQUAL;

  /* break ties here so we really are a total order */
  if (!BOBGUI_IS_TREE_LIST_ROW (item1))
    return BOBGUI_IS_TREE_LIST_ROW (item2) ? BOBGUI_ORDERING_LARGER : (item1 < item2 ? BOBGUI_ORDERING_SMALLER : BOBGUI_ORDERING_LARGER);
  else if (!BOBGUI_IS_TREE_LIST_ROW (item2))
    return BOBGUI_ORDERING_SMALLER;

  r1 = BOBGUI_TREE_LIST_ROW (item1);
  r2 = BOBGUI_TREE_LIST_ROW (item2);

  g_object_ref (r1);
  g_object_ref (r2);

  d1 = bobgui_tree_list_row_get_depth (r1);
  d2 = bobgui_tree_list_row_get_depth (r2);

  /* First, get to the same depth */
  while (d1 > d2)
    {
      p1 = bobgui_tree_list_row_get_parent (r1);
      g_object_unref (r1);
      r1 = p1;
      d1--;
      result = BOBGUI_ORDERING_LARGER;
    }
  while (d2 > d1)
    {
      p2 = bobgui_tree_list_row_get_parent (r2);
      g_object_unref (r2);
      r2 = p2;
      d2--;
      result = BOBGUI_ORDERING_SMALLER;
    }

  /* Now walk up until we find a common parent */
  if (r1 != r2)
    {
      while (TRUE)
        {
          p1 = bobgui_tree_list_row_get_parent (r1);
          p2 = bobgui_tree_list_row_get_parent (r2);
          if (p1 == p2)
            {
              gpointer obj1 = bobgui_tree_list_row_get_item (r1);
              gpointer obj2 = bobgui_tree_list_row_get_item (r2);

              if (self->sorter == NULL)
                result = BOBGUI_ORDERING_EQUAL;
              else
                result = bobgui_sorter_compare (self->sorter, obj1, obj2);

              /* We must break ties here because if r1 ever gets a child,
               * it would need to go right in between r1 and r2. */
              if (result == BOBGUI_ORDERING_EQUAL)
                {
                  if (bobgui_tree_list_row_get_position (r1) < bobgui_tree_list_row_get_position (r2))
                    result = BOBGUI_ORDERING_SMALLER;
                  else
                    result = BOBGUI_ORDERING_LARGER;
                }

              g_object_unref (obj1);
              g_object_unref (obj2);

              break;
            }
          else
            {
              g_object_unref (r1);
              r1 = p1;
              g_object_unref (r2);
              r2 = p2;
            }
        }
    }

  g_object_unref (r1);
  g_object_unref (r2);

  return result;
}

static BobguiSorterOrder
bobgui_tree_list_row_sorter_get_order (BobguiSorter *sorter)
{
  /* Must be a total order, because we need an exact position where new items go */
  return BOBGUI_SORTER_ORDER_TOTAL;
}

static void
propagate_changed (BobguiSorter *sorter,
                   BobguiSorterChange change,
                   BobguiTreeListRowSorter *self)
{
  bobgui_sorter_changed_with_keys (BOBGUI_SORTER (self),
                                change,
                                bobgui_tree_list_row_sort_keys_new (self));
}

static void
bobgui_tree_list_row_sorter_dispose (GObject *object)
{
  BobguiTreeListRowSorter *self = BOBGUI_TREE_LIST_ROW_SORTER (object);

  if (self->sorter)
    g_signal_handlers_disconnect_by_func (self->sorter, propagate_changed, self);
  g_clear_object (&self->sorter);

  G_OBJECT_CLASS (bobgui_tree_list_row_sorter_parent_class)->dispose (object);
}

static void
bobgui_tree_list_row_sorter_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  BobguiTreeListRowSorter *self = BOBGUI_TREE_LIST_ROW_SORTER (object);

  switch (prop_id)
    {
    case PROP_SORTER:
      bobgui_tree_list_row_sorter_set_sorter (self, BOBGUI_SORTER (g_value_get_object (value)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_tree_list_row_sorter_get_property (GObject     *object,
                                       guint        prop_id,
                                       GValue      *value,
                                       GParamSpec  *pspec)
{
  BobguiTreeListRowSorter *self = BOBGUI_TREE_LIST_ROW_SORTER (object);

  switch (prop_id)
    {
    case PROP_SORTER:
      g_value_set_object (value, self->sorter);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_tree_list_row_sorter_class_init (BobguiTreeListRowSorterClass *class)
{
  BobguiSorterClass *sorter_class = BOBGUI_SORTER_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  sorter_class->compare = bobgui_tree_list_row_sorter_compare;
  sorter_class->get_order = bobgui_tree_list_row_sorter_get_order;

  object_class->dispose = bobgui_tree_list_row_sorter_dispose;
  object_class->set_property = bobgui_tree_list_row_sorter_set_property;
  object_class->get_property = bobgui_tree_list_row_sorter_get_property;

  /**
   * BobguiTreeListRowSorter:sorter:
   *
   * The underlying sorter
   */
  properties[PROP_SORTER] =
      g_param_spec_object ("sorter", NULL, NULL,
                          BOBGUI_TYPE_SORTER,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
}

static void
bobgui_tree_list_row_sorter_init (BobguiTreeListRowSorter *self)
{
  bobgui_sorter_changed_with_keys (BOBGUI_SORTER (self),
                                BOBGUI_SORTER_CHANGE_DIFFERENT,
                                bobgui_tree_list_row_sort_keys_new (self));
}

/**
 * bobgui_tree_list_row_sorter_new:
 * @sorter: (nullable) (transfer full): a `BobguiSorter`
 *
 * Create a special-purpose sorter that applies the sorting
 * of @sorter to the levels of a `BobguiTreeListModel`.
 *
 * Note that this sorter relies on [property@Bobgui.TreeListModel:passthrough]
 * being %FALSE as it can only sort [class@Bobgui.TreeListRow]s.
 *
 * Returns: a new `BobguiTreeListRowSorter`
 */
BobguiTreeListRowSorter *
bobgui_tree_list_row_sorter_new (BobguiSorter *sorter)
{
  BobguiTreeListRowSorter *result;

  g_return_val_if_fail (sorter == NULL || BOBGUI_IS_SORTER (sorter), NULL);

  result = g_object_new (BOBGUI_TYPE_TREE_LIST_ROW_SORTER,
                         "sorter", sorter,
                         NULL);

  g_clear_object (&sorter);

  return result;
}

/**
 * bobgui_tree_list_row_sorter_set_sorter:
 * @self: a `BobguiTreeListRowSorter`
 * @sorter: (nullable) (transfer none): The sorter to use
 *
 * Sets the sorter to use for items with the same parent.
 *
 * This sorter will be passed the [property@Bobgui.TreeListRow:item] of
 * the tree list rows passed to @self.
 */
void
bobgui_tree_list_row_sorter_set_sorter (BobguiTreeListRowSorter *self,
                                     BobguiSorter            *sorter)
{
  g_return_if_fail (BOBGUI_IS_TREE_LIST_ROW_SORTER (self));
  g_return_if_fail (sorter == NULL || BOBGUI_IS_SORTER (sorter));

  if (self->sorter == sorter)
    return;

  if (self->sorter)
    g_signal_handlers_disconnect_by_func (self->sorter, propagate_changed, self);
  g_set_object (&self->sorter, sorter);
  if (self->sorter)
    g_signal_connect (sorter, "changed", G_CALLBACK (propagate_changed), self);

  bobgui_sorter_changed_with_keys (BOBGUI_SORTER (self),
                                BOBGUI_SORTER_CHANGE_DIFFERENT,
                                bobgui_tree_list_row_sort_keys_new (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SORTER]);
}

/**
 * bobgui_tree_list_row_sorter_get_sorter:
 * @self: a `BobguiTreeListRowSorter`
 *
 * Returns the sorter used by @self.
 *
 * Returns: (transfer none) (nullable): the sorter used
 */
BobguiSorter *
bobgui_tree_list_row_sorter_get_sorter (BobguiTreeListRowSorter *self)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_LIST_ROW_SORTER (self), NULL);

  return self->sorter;
}
