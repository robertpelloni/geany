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

#include "bobguimultisorter.h"

#include "bobguibuildable.h"
#include "bobguisorterprivate.h"
#include "bobguitypebuiltins.h"

#define GDK_ARRAY_TYPE_NAME BobguiSorters
#define GDK_ARRAY_NAME bobgui_sorters
#define GDK_ARRAY_ELEMENT_TYPE BobguiSorter *
#define GDK_ARRAY_FREE_FUNC g_object_unref

#include "gdk/gdkarrayimpl.c"

/**
 * BobguiMultiSorter:
 *
 * Combines multiple sorters by trying them in turn.
 *
 * If the first sorter compares two items as equal,
 * the second is tried next, and so on.
 */
struct _BobguiMultiSorter
{
  BobguiSorter parent_instance;

  BobguiSorters sorters;
};

typedef struct _BobguiMultiSortKey BobguiMultiSortKey;
typedef struct _BobguiMultiSortKeys BobguiMultiSortKeys;

struct _BobguiMultiSortKey
{
  gsize offset;
  BobguiSortKeys *keys;
};

struct _BobguiMultiSortKeys
{
  BobguiSortKeys parent_keys;

  guint n_keys;
  BobguiMultiSortKey keys[];
};

enum {
  PROP_0,
  PROP_ITEM_TYPE,
  PROP_N_ITEMS,

  N_PROPS
};

static GParamSpec *properties[N_PROPS] = { NULL, };

static void
bobgui_multi_sort_keys_free (BobguiSortKeys *keys)
{
  BobguiMultiSortKeys *self = (BobguiMultiSortKeys *) keys;
  gsize i;

  for (i = 0; i < self->n_keys; i++)
    bobgui_sort_keys_unref (self->keys[i].keys);

  g_free (self);
}

static int
bobgui_multi_sort_keys_compare (gconstpointer a,
                             gconstpointer b,
                             gpointer      data)
{
  BobguiMultiSortKeys *self = (BobguiMultiSortKeys *) data;
  gsize i;

  for (i = 0; i < self->n_keys; i++)
    {
      BobguiOrdering result = bobgui_sort_keys_compare (self->keys[i].keys,
                                                  ((const char *) a) + self->keys[i].offset,
                                                  ((const char *) b) + self->keys[i].offset);
      if (result != BOBGUI_ORDERING_EQUAL)
        return result;
    }

  return BOBGUI_ORDERING_EQUAL;
}

static gboolean
bobgui_multi_sort_keys_is_compatible (BobguiSortKeys *keys,
                                   BobguiSortKeys *other)
{
  BobguiMultiSortKeys *self = (BobguiMultiSortKeys *) keys;
  BobguiMultiSortKeys *compare = (BobguiMultiSortKeys *) other;
  gsize i;

  if (keys->klass != other->klass)
    return FALSE;

  if (self->n_keys != compare->n_keys)
    return FALSE;

  for (i = 0; i < self->n_keys; i++)
    {
      if (!bobgui_sort_keys_is_compatible (self->keys[i].keys, compare->keys[i].keys))
        return FALSE;
    }

  return TRUE;
}

static void
bobgui_multi_sort_keys_init_key (BobguiSortKeys *keys,
                              gpointer     item,
                              gpointer     key_memory)
{
  BobguiMultiSortKeys *self = (BobguiMultiSortKeys *) keys;
  char *key = (char *) key_memory;
  gsize i;

  for (i = 0; i < self->n_keys; i++)
    bobgui_sort_keys_init_key (self->keys[i].keys, item, key + self->keys[i].offset);
}

static void
bobgui_multi_sort_keys_clear_key (BobguiSortKeys *keys,
                               gpointer     key_memory)
{
  BobguiMultiSortKeys *self = (BobguiMultiSortKeys *) keys;
  char *key = (char *) key_memory;
  gsize i;

  for (i = 0; i < self->n_keys; i++)
    bobgui_sort_keys_clear_key (self->keys[i].keys, key + self->keys[i].offset);
}

static const BobguiSortKeysClass BOBGUI_MULTI_SORT_KEYS_CLASS =
{
  bobgui_multi_sort_keys_free,
  bobgui_multi_sort_keys_compare,
  bobgui_multi_sort_keys_is_compatible,
  bobgui_multi_sort_keys_init_key,
  bobgui_multi_sort_keys_clear_key,
};

static BobguiSortKeys *
bobgui_multi_sort_keys_new (BobguiMultiSorter *self)
{
  BobguiMultiSortKeys *result;
  BobguiSortKeys *keys;
  gsize i;

  if (bobgui_sorters_get_size (&self->sorters) == 0)
    return bobgui_sort_keys_new_equal ();
  else if (bobgui_sorters_get_size (&self->sorters) == 1)
    return bobgui_sorter_get_keys (bobgui_sorters_get (&self->sorters, 0));

  keys = bobgui_sort_keys_alloc (&BOBGUI_MULTI_SORT_KEYS_CLASS,
                              sizeof (BobguiMultiSortKeys) + bobgui_sorters_get_size (&self->sorters) * sizeof (BobguiMultiSortKey),
                              0, 1);
  result = (BobguiMultiSortKeys *) keys;

  result->n_keys = bobgui_sorters_get_size (&self->sorters);
  for (i = 0; i < result->n_keys; i++)
    {
      result->keys[i].keys = bobgui_sorter_get_keys (bobgui_sorters_get (&self->sorters, i));
      result->keys[i].offset = BOBGUI_SORT_KEYS_ALIGN (keys->key_size, bobgui_sort_keys_get_key_align (result->keys[i].keys));
      keys->key_size = result->keys[i].offset + BOBGUI_SORT_KEYS_ALIGN (bobgui_sort_keys_get_key_size (result->keys[i].keys),
                                                                     bobgui_sort_keys_get_key_align (result->keys[i].keys));
      keys->key_align = MAX (keys->key_align, bobgui_sort_keys_get_key_align (result->keys[i].keys));
    }

  return keys;
}

static GType
bobgui_multi_sorter_get_item_type (GListModel *list)
{
  return BOBGUI_TYPE_SORTER;
}

static guint
bobgui_multi_sorter_get_n_items (GListModel *list)
{
  BobguiMultiSorter *self = BOBGUI_MULTI_SORTER (list);

  return bobgui_sorters_get_size (&self->sorters);
}

static gpointer
bobgui_multi_sorter_get_item (GListModel *list,
                           guint       position)
{
  BobguiMultiSorter *self = BOBGUI_MULTI_SORTER (list);

  if (position < bobgui_sorters_get_size (&self->sorters))
    return g_object_ref (bobgui_sorters_get (&self->sorters, position));
  else
    return NULL;
}

static void
bobgui_multi_sorter_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = bobgui_multi_sorter_get_item_type;
  iface->get_n_items = bobgui_multi_sorter_get_n_items;
  iface->get_item = bobgui_multi_sorter_get_item;
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_multi_sorter_buildable_add_child (BobguiBuildable *buildable,
                                      BobguiBuilder   *builder,
                                      GObject      *child,
                                      const char   *type)
{
  if (BOBGUI_IS_SORTER (child))
    bobgui_multi_sorter_append (BOBGUI_MULTI_SORTER (buildable), g_object_ref (BOBGUI_SORTER (child)));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bobgui_multi_sorter_buildable_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_multi_sorter_buildable_add_child;
}

G_DEFINE_TYPE_WITH_CODE (BobguiMultiSorter, bobgui_multi_sorter, BOBGUI_TYPE_SORTER,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, bobgui_multi_sorter_list_model_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE, bobgui_multi_sorter_buildable_init))

static BobguiOrdering
bobgui_multi_sorter_compare (BobguiSorter *sorter,
                          gpointer   item1,
                          gpointer   item2)
{
  BobguiMultiSorter *self = BOBGUI_MULTI_SORTER (sorter);
  BobguiOrdering result = BOBGUI_ORDERING_EQUAL;
  guint i;

  for (i = 0; i < bobgui_sorters_get_size (&self->sorters); i++)
    {
      BobguiSorter *child = bobgui_sorters_get (&self->sorters, i);

      result = bobgui_sorter_compare (child, item1, item2);
      if (result != BOBGUI_ORDERING_EQUAL)
        break;
    }

  return result;
}

static BobguiSorterOrder
bobgui_multi_sorter_get_order (BobguiSorter *sorter)
{
  BobguiMultiSorter *self = BOBGUI_MULTI_SORTER (sorter);
  BobguiSorterOrder result = BOBGUI_SORTER_ORDER_NONE;
  guint i;

  for (i = 0; i < bobgui_sorters_get_size (&self->sorters); i++)
    {
      BobguiSorter *child = bobgui_sorters_get (&self->sorters, i);
      BobguiSorterOrder child_order;

      child_order = bobgui_sorter_get_order (child);
      switch (child_order)
        {
          case BOBGUI_SORTER_ORDER_PARTIAL:
            result = BOBGUI_SORTER_ORDER_PARTIAL;
            break;
          case BOBGUI_SORTER_ORDER_NONE:
            break;
          case BOBGUI_SORTER_ORDER_TOTAL:
            return BOBGUI_SORTER_ORDER_TOTAL;
          default:
            g_assert_not_reached ();
            break;
        }
    }

  return result;
}

static void
bobgui_multi_sorter_changed_cb (BobguiSorter       *sorter,
                             BobguiSorterChange  change,
                             BobguiMultiSorter  *self)
{
  /* Using an enum on purpose, so gcc complains about this case if
   * new values are added to the enum
   */
  switch (change)
  {
    case BOBGUI_SORTER_CHANGE_INVERTED:
      /* This could do a lot better with change handling, in particular in
       * cases where self->n_sorters == 1 or if sorter == self->sorters[0]
       */
      change = BOBGUI_SORTER_CHANGE_DIFFERENT;
      break;

    case BOBGUI_SORTER_CHANGE_DIFFERENT:
    case BOBGUI_SORTER_CHANGE_LESS_STRICT:
    case BOBGUI_SORTER_CHANGE_MORE_STRICT:
      break;

    default:
      g_assert_not_reached ();
      change = BOBGUI_SORTER_CHANGE_DIFFERENT;
  }
  bobgui_sorter_changed_with_keys (BOBGUI_SORTER (self),
                                change,
                                bobgui_multi_sort_keys_new (self));
}

static void
bobgui_multi_sorter_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  BobguiMultiSorter *self = BOBGUI_MULTI_SORTER (object);

  switch (prop_id)
    {
    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, BOBGUI_TYPE_SORTER);
      break;

    case PROP_N_ITEMS:
      g_value_set_uint (value, bobgui_sorters_get_size (&self->sorters));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_multi_sorter_dispose (GObject *object)
{
  BobguiMultiSorter *self = BOBGUI_MULTI_SORTER (object);
  guint i;

  for (i = 0; i < bobgui_sorters_get_size (&self->sorters); i++)
    {
      BobguiSorter *sorter = bobgui_sorters_get (&self->sorters, i);
      g_signal_handlers_disconnect_by_func (sorter, bobgui_multi_sorter_changed_cb, self);
    }
  bobgui_sorters_clear (&self->sorters);

  G_OBJECT_CLASS (bobgui_multi_sorter_parent_class)->dispose (object);
}

static void
bobgui_multi_sorter_class_init (BobguiMultiSorterClass *class)
{
  BobguiSorterClass *sorter_class = BOBGUI_SORTER_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  sorter_class->compare = bobgui_multi_sorter_compare;
  sorter_class->get_order = bobgui_multi_sorter_get_order;

  object_class->get_property = bobgui_multi_sorter_get_property;
  object_class->dispose = bobgui_multi_sorter_dispose;

  /**
   * BobguiMultiSorter:item-type:
   *
   * The type of items. See [method@Gio.ListModel.get_item_type].
   *
   * Since: 4.8
   **/
  properties[PROP_ITEM_TYPE] =
    g_param_spec_gtype ("item-type", NULL, NULL,
                        BOBGUI_TYPE_SORTER,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiMultiSorter:n-items:
   *
   * The number of items. See [method@Gio.ListModel.get_n_items].
   *
   * Since: 4.8
   **/
  properties[PROP_N_ITEMS] =
    g_param_spec_uint ("n-items", NULL, NULL,
                       0, G_MAXUINT, 0,
                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
bobgui_multi_sorter_init (BobguiMultiSorter *self)
{
  bobgui_sorters_init (&self->sorters);

  bobgui_sorter_changed_with_keys (BOBGUI_SORTER (self),
                                BOBGUI_SORTER_CHANGE_DIFFERENT,
                                bobgui_multi_sort_keys_new (self));
}

/**
 * bobgui_multi_sorter_new:
 *
 * Creates a new multi sorter.
 *
 * This sorter compares items by trying each of the sorters
 * in turn, until one returns non-zero. In particular, if
 * no sorter has been added to it, it will always compare
 * items as equal.
 *
 * Returns: a new `BobguiMultiSorter`
 */
BobguiMultiSorter *
bobgui_multi_sorter_new (void)
{
  return g_object_new (BOBGUI_TYPE_MULTI_SORTER, NULL);
}

/**
 * bobgui_multi_sorter_append:
 * @self: a `BobguiMultiSorter`
 * @sorter: (transfer full): a sorter to add
 *
 * Add @sorter to @self to use for sorting at the end.
 *
 * @self will consult all existing sorters before it will
 * sort with the given @sorter.
 */
void
bobgui_multi_sorter_append (BobguiMultiSorter *self,
                         BobguiSorter      *sorter)
{
  g_return_if_fail (BOBGUI_IS_MULTI_SORTER (self));
  g_return_if_fail (BOBGUI_IS_SORTER (sorter));

  g_signal_connect (sorter, "changed", G_CALLBACK (bobgui_multi_sorter_changed_cb), self);
  bobgui_sorters_append (&self->sorters, sorter);
  g_list_model_items_changed (G_LIST_MODEL (self), bobgui_sorters_get_size (&self->sorters) - 1, 0, 1);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);

  bobgui_sorter_changed_with_keys (BOBGUI_SORTER (self),
                                BOBGUI_SORTER_CHANGE_MORE_STRICT,
                                bobgui_multi_sort_keys_new (self));
}

/**
 * bobgui_multi_sorter_remove:
 * @self: a `BobguiMultiSorter`
 * @position: position of sorter to remove
 *
 * Removes the sorter at the given @position from the list of sorter
 * used by @self.
 *
 * If @position is larger than the number of sorters, nothing happens.
 */
void
bobgui_multi_sorter_remove (BobguiMultiSorter *self,
                         guint           position)
{
  guint length;
  BobguiSorter *sorter;

  g_return_if_fail (BOBGUI_IS_MULTI_SORTER (self));

  length = bobgui_sorters_get_size (&self->sorters);
  if (position >= length)
    return;

  sorter = bobgui_sorters_get (&self->sorters, position);
  g_signal_handlers_disconnect_by_func (sorter, bobgui_multi_sorter_changed_cb, self);
  bobgui_sorters_splice (&self->sorters, position, 1, FALSE, NULL, 0);
  g_list_model_items_changed (G_LIST_MODEL (self), position, 1, 0);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);

  bobgui_sorter_changed_with_keys (BOBGUI_SORTER (self),
                                BOBGUI_SORTER_CHANGE_LESS_STRICT,
                                bobgui_multi_sort_keys_new (self));
}
